#include <sys/mman.h>
#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
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
	int **left_col,
	size_t *left_col_len,
	int **right_col,
	size_t *right_col_len,
	const Allocator *allocator
)
{
	assert(filename != NULL);
	assert(left_col != NULL);
	assert(left_col_len != NULL);
	assert(right_col != NULL);
	assert(right_col_len != NULL);
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	*left_col = NULL;
	*right_col = NULL;
	*left_col_len = 0;
	*right_col_len = 0;

	Mapped_File *file = map_file(filename, allocator);
	if (!file) return;

	int *left = allocator->alloc(file->line_count * sizeof(int));
	int *right = allocator->alloc(file->line_count * sizeof(int));

	if (!left || !right) {
		if (left) allocator->free(left);
		if (right) allocator->free(right);
		unmap_file(file, allocator);
		return;
	}

	for (size_t i = 0; i < file->line_count; ++i) {
		const char *current_line = file->lines[i];
		size_t line_len = get_line_length(file, i);

		if (line_len == 0) {
			left[i] = right[i] = 0;
			continue;
		}

		// Parse the first number.
		size_t j = 0;
		char first_number[16] = {0};
		while (j < line_len && j < sizeof(first_number) - 1 &&
			   isdigit(current_line[j])) {
			first_number[j] = current_line[j];
			++j;
		}
		left[i] = atoi(first_number);

		// Skip whitespace.
		while (j < line_len && isspace(current_line[j])) {
			++j;
		}

		// Parse the second number.
		size_t k = 0;
		char second_number[16] = {0};
		while (j < line_len && k < sizeof(second_number) - 1 &&
			   isdigit(current_line[j])) {
			second_number[k] = current_line[j];
			++j;
			++k;
		}
		right[i] = atoi(second_number);
	}

	*left_col = left;
	*right_col = right;
	*left_col_len = *right_col_len = file->line_count;

	unmap_file(file, allocator);
}

void
insertion_sort(int *A, int len)
{
	assert(A != NULL);
	assert(len >= 0);

	int i = 1;
	while (i < len) {
		int j = i;
		while (j > 0 && A[j - 1] > A[j]) {
			int tmp = A[j - 1];
			A[j - 1] = A[j];
			A[j] = tmp;
			--j;
		}
		++i;
	}
}

int *
filter_array(int *values, size_t len, const Allocator *allocator)
{
	assert(values != NULL);
	assert(allocator != NULL);
	assert(allocator->alloc != NULL);
	assert(allocator->free != NULL);

	int *filtered = allocator->alloc(len * sizeof(int));
	if (!filtered) return NULL;
	memset(filtered, 0, len * sizeof(int));

	for (size_t i = 0; i < len; ++i) {
		if (values[i] % len != 0) {
			filtered[i] = values[i];
		}
	}
	return filtered;
}

int
main(void)
{
	const Allocator *allocator = &default_allocator;
	int *left_col = NULL, *right_col = NULL;
	size_t col_len = 0;

	parse_file("input.txt", &left_col, &col_len, &right_col, &col_len, allocator);
	if (!left_col || !right_col) {
		fprintf(stderr, "failed to parse file\n");
		return 1;
	}

	// Part 2 is first because part 1 requires sorting the arrays.
	int *filtered = filter_array(left_col, col_len, allocator);
	if (!filtered) {
		fprintf(stderr, "failed to allocate filtered array\n");
		goto cleanup;
	}

	int similarities_sum = 0;
	for (size_t i = 0; i < col_len; ++i) {
		int current = filtered[i];
		int duplicates = 0;

		for (size_t j = 0; j < col_len; ++j) {
			if (right_col[j] == current) {
				++duplicates;
			}
		}

		similarities_sum += current * duplicates;
	}
	allocator->free(filtered);

	printf("Similarities sum =\n\t%d\n", similarities_sum);

	// Part 1.
	insertion_sort(left_col, col_len);
	insertion_sort(right_col, col_len);

	int distances_sum = 0;
	for (size_t i = 0; i < col_len; ++i) {
		distances_sum += abs(left_col[i] - right_col[i]);
	}

	printf("Distances sum =\n\t%d\n", distances_sum);

cleanup:
	allocator->free(right_col);
	allocator->free(left_col);
	return 0;
}
