#include <sys/mman.h>
#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	void   *(*alloc)(size_t size);
	void    (*free)(void *ptr);
} Allocator;

static const Allocator default_allocator = {
	.alloc = malloc,
	.free  = free,
};

typedef struct {
	int *array;
	size_t len;
	size_t cap;
} Dynamic_Array;

void
da_append(Dynamic_Array *da, int value, const Allocator *allocator)
{
	if (da->len >= da->cap) {
		size_t new_cap = da->cap * 2;
		int *new_array = allocator->alloc(new_cap * sizeof(int));
		memcpy(new_array, da->array, da->len * sizeof(int));

		allocator->free(da->array);
		da->array = new_array;
		da->cap = new_cap;
	}

	da->array[da->len] = value;
	++da->len;
}

typedef struct {
	char     *content;
	size_t    size;
	char    **lines;
	size_t    line_count;
} Mapped_File;

static int
get_file_size(int fd, size_t *size)
{
	struct stat sb;
	if (fstat(fd, &sb) == -1) return -1;
	*size = sb.st_size;
	return 0;
}

static int
index_lines(Mapped_File *file, const Allocator *allocator)
{
	assert(file != NULL);
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	size_t newline_count = 0;
	for (size_t i = 0; i < file->size; ++i) {
		if (file->content[i] == '\n') {
			++newline_count;
		}
	}

	file->lines = allocator->alloc((newline_count + 1) * sizeof(char *));
	if (!file->lines) return -1;

	file->lines[file->line_count++] = file->content;
	for (size_t i = 0; i < file->size; ++i) {
		if (file->content[i] == '\n' && i + 1 < file->size) {
			file->lines[file->line_count++] = &file->content[i + 1];
		}
	}
	return 0;
}

Mapped_File *
map_file(const char *filename, const Allocator *allocator)
{
	assert(filename != NULL);
	assert(filename[0] != '\0');
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	int fd = open(filename, O_RDONLY);
	if (fd == -1) return NULL;

	Mapped_File *file = allocator->alloc(sizeof(Mapped_File));
	if (!file) {
		close(fd);
		return NULL;
	}

	// Zero initialize the struct.
	memset(file, 0, sizeof(*file));

	if (get_file_size(fd, &file->size) == -1) {
		close(fd);
		allocator->free(file);
		return NULL;
	}

	// Handle empty file.
	if (file->size == 0) {
		close(fd);
		file->content = NULL;
		file->lines = NULL;
		return file;
	}

	// Map the file.
	file->content = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	if (file->content == MAP_FAILED) {
		allocator->free(file);
		return NULL;
	}

	if (index_lines(file, allocator) == -1) {
		munmap(file->content, file->size);
		allocator->free(file->lines);
		allocator->free(file);
		return NULL;
	}

	return file;
}

void
unmap_file(Mapped_File *file, const Allocator *allocator)
{
	if (!file) return;
	assert(allocator != NULL);
	assert(allocator->free != NULL);

	if (file->content && file->size > 0) {
		assert(file->content != MAP_FAILED);
		munmap(file->content, file->size);
	}
	if (file->lines) {
		allocator->free(file->lines);
	}
	allocator->free(file);
}

size_t
get_line_length(const Mapped_File *file, size_t line_index)
{
	assert(file != NULL);
	if (line_index >= file->line_count) return 0;

	const char *line_start = file->lines[line_index];
	const char *file_end = file->content + file->size;
	const char *next_line = (line_index + 1 < file->line_count)
		? file->lines[line_index + 1]
		: file_end;

	assert(line_start != NULL);
	assert(file_end >= line_start);
	assert(next_line >= line_start);

	size_t len = next_line - line_start;
	if (len > 0 && line_start[len - 1] == '\n') --len;
	return len;
}

void
parse_file(
	const char *filename,
	int *safe_reports,
	const Allocator *allocator
)
{
	assert(filename != NULL);
	assert(safe_reports != NULL);
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	Mapped_File *file = map_file(filename, allocator);
	if (!file) {
		fprintf(stderr, "failed to map file\n");
		return;
	}

	*safe_reports = 0;

	for (size_t i = 0; i < file->line_count; ++i) {
		const char *current_line = file->lines[i];
		size_t line_len = get_line_length(file, i);

		Dynamic_Array da;
		da.array = allocator->alloc(32 * sizeof(int));
		if (!da.array) {
			fprintf(stderr, "failed to allocate array\n");
			unmap_file(file, allocator);
			return;
		}
		da.len = 0;
		da.cap = 32;

		char strn[16] = {0};
		size_t j = 0;
		size_t pos = 0;

		while (pos < line_len) {
			if (isdigit(current_line[pos])) {
				if (j < sizeof(strn) - 1) {
					strn[j++] = current_line[pos];
				}
			}

			if ((!isdigit(current_line[pos]) || pos == line_len - 1) && j > 0) {
				strn[j] = '\0';
				da_append(&da, atoi(strn), allocator);
				j = 0;
				memset(strn, 0, sizeof(strn));
			}
			++pos;
		}

		bool is_valid = true;
		if (da.len >= 2) {
			int direction = da.array[1] - da.array[0];
			bool inc = direction > 0;

			for (size_t k = 1; k < da.len; ++k) {
				int diff = da.array[k] - da.array[k-1];
				int abs_diff = abs(diff);

				if (abs_diff < 1 || abs_diff > 3 || (inc && diff <= 0) || (!inc && diff >= 0)) {
					is_valid = false;
					break;
				}
			}

			if (is_valid) {
				++(*safe_reports);
			}
		}

		allocator->free(da.array);
	}

	unmap_file(file, allocator);
}

void
parse_file2(
	const char *filename,
	int *safe_reports,
	const Allocator *allocator
)
{
	assert(filename != NULL);
	assert(safe_reports != NULL);
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	Mapped_File *file = map_file(filename, allocator);
	if (!file) {
		fprintf(stderr, "failed to map file\n");
		return;
	}

	*safe_reports = 0;

	for (size_t i = 0; i < file->line_count; ++i) {
		const char *current_line = file->lines[i];
		size_t line_len = get_line_length(file, i);

		Dynamic_Array da;
		da.array = allocator->alloc(32 * sizeof(int));
		if (!da.array) {
			fprintf(stderr, "failed to allocate array\n");
			unmap_file(file, allocator);
			return;
		}
		da.len = 0;
		da.cap = 32;

		char strn[16] = {0};
		size_t j = 0;
		size_t pos = 0;

		while (pos < line_len) {
			if (isdigit(current_line[pos])) {
				if (j < sizeof(strn) - 1) {
					strn[j++] = current_line[pos];
				}
			}

			if ((!isdigit(current_line[pos]) || pos == line_len - 1) && j > 0) {
				strn[j] = '\0';
				da_append(&da, atoi(strn), allocator);
				j = 0;
				memset(strn, 0, sizeof(strn));
			}
			++pos;
		}

		bool is_valid = false;
		if (da.len >= 2) {
			for (int inc = 0; inc <= 1 && !is_valid; ++inc) {
				bool valid_sequence = true;
				for (size_t k = 1; k < da.len && valid_sequence; ++k) {
					int diff = da.array[k] - da.array[k-1];
					int abs_diff = abs(diff);
					if (abs_diff < 1 || abs_diff > 3 || (inc && diff <= 0) || (!inc && diff >= 0)) {
						valid_sequence = false;
					}
				}
				if (valid_sequence) {
					is_valid = true;
				}
			}
		}

		if (!is_valid && da.len >= 3) {
			for (size_t skip = 0; skip < da.len && !is_valid; ++skip) {
				for (int inc = 0; inc <= 1 && !is_valid; ++inc) {
					bool valid_sequence = true;
					int prev = -1;

					for (size_t k = 0; k < da.len && valid_sequence; ++k) {
						if (k == skip) continue;

						if (prev != -1) {
							int diff = da.array[k] - prev;
							int abs_diff = abs(diff);
							if (abs_diff < 1 || abs_diff > 3 || (inc && diff <= 0) || (!inc && diff >= 0)) {
								valid_sequence = false;
							}
						}
						prev = da.array[k];
					}

					if (valid_sequence) {
						is_valid = true;
					}
				}
			}
		}

		if (is_valid) {
			++(*safe_reports);
		}

		allocator->free(da.array);
	}

	unmap_file(file, allocator);
}

int
main(void)
{
	const Allocator *allocator = &default_allocator;
	int safe_reports = 0;

	// Part 1.
	parse_file("input.txt", &safe_reports, allocator);
	if (!safe_reports) {
		fprintf(stderr, "failed to parse file\n");
		return 1;
	}

	printf("Number of safe reports =\n\t%d\n", safe_reports);

	// Part 2.
	parse_file2("input.txt", &safe_reports, allocator);
	if (!safe_reports) {
		fprintf(stderr, "failed to parse file\n");
		return 1;
	}

	printf("Number of safe reports (with a single bad jump) =\n\t%d\n", safe_reports);

	return 0;
}
