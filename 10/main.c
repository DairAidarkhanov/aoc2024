#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define MAX_WIDTH	1024
#define MAX_HEIGHT	1024

struct Point {
	int	x;
	int	y;
};

struct Queue {
	struct Point	*items;
	size_t		capacity;
	size_t		size;
	size_t		front;
	size_t		rear;
};

static int grid[MAX_HEIGHT][MAX_WIDTH];
static int height;
static int width;

static const int dx[] = {0, 1, 0, -1};
static const int dy[] = {-1, 0, 1, 0};

static void
init_queue(struct Queue *q, size_t capacity)
{
	q->items = malloc(capacity * sizeof(struct Point));
	if (q->items == NULL)
		err(1, "malloc failed");
	q->capacity = capacity;
	q->size = 0;
	q->front = 0;
	q->rear = 0;
}

static void
enqueue(struct Queue *q, struct Point p)
{
	if (q->size == q->capacity)
		err(1, "queue overflow");

	q->items[q->rear] = p;
	q->rear = (q->rear + 1) % q->capacity;
	++q->size;
}

static struct Point
dequeue(struct Queue *q)
{
	struct Point p;

	if (q->size == 0)
		err(1, "queue underflow");

	p = q->items[q->front];
	q->front = (q->front + 1) % q->capacity;
	q->size--;
	return p;
}

static int
count_reachable_nines(int start_x, int start_y)
{
	struct Queue queue;
	int visited[MAX_HEIGHT][MAX_WIDTH];
	int count = 0;
	int i;
	struct Point current;
	struct Point new_point;

	memset(visited, 0, sizeof(visited));
	init_queue(&queue, (size_t)(height * width));

	current.x = start_x;
	current.y = start_y;
	enqueue(&queue, current);
	visited[start_y][start_x] = 1;

	while (queue.size > 0) {
		current = dequeue(&queue);

		if (grid[current.y][current.x] == 9)
			++count;

		for (i = 0; i < 4; ++i) {
			int new_x = current.x + dx[i];
			int new_y = current.y + dy[i];

			if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height &&
				!visited[new_y][new_x] &&
				grid[new_y][new_x] == grid[current.y][current.x] + 1) {
				new_point.x = new_x;
				new_point.y = new_y;
				enqueue(&queue, new_point);
				visited[new_y][new_x] = 1;
			}
		}
	}

	free(queue.items);
	return count;
}

static int
count_distinct_paths(int x, int y, int current_height, int visited[MAX_HEIGHT][MAX_WIDTH])
{
	int paths = 0;
	int i;
	int new_x;
	int new_y;

	if (current_height == 9)
		return 1;

	visited[y][x] = 1;

	for (i = 0; i < 4; ++i) {
		new_x = x + dx[i];
		new_y = y + dy[i];

		if (new_x >= 0 && new_x < width &&
			new_y >= 0 && new_y < height &&
			!visited[new_y][new_x] &&
			grid[new_y][new_x] == current_height + 1) {
			paths += count_distinct_paths(new_x, new_y, current_height + 1, visited);
		}
	}

	visited[y][x] = 0;
	return paths;
}

static int
count_trails_from_trailhead(int start_x, int start_y)
{
	int visited[MAX_HEIGHT][MAX_WIDTH];

	memset(visited, 0, sizeof(visited));
	return count_distinct_paths(start_x, start_y, 0, visited);
}

static void
read_input(void)
{
	char line[MAX_WIDTH + 2];
	size_t len;
	int x;

	height = 0;
	width = 0;

	while (fgets(line, sizeof(line), stdin) != NULL) {
		len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[--len] = '\0';

		if (width == 0)
			width = (int)len;
		else if (width != (int)len)
			errx(1, "inconsistent line length");

		if (height >= MAX_HEIGHT)
			errx(1, "input too tall");

		for (x = 0; x < width; ++x)
			grid[height][x] = line[x] - '0';

		++height;
	}
}

int
main(void)
{
	int part1_score = 0;
	int part2_score = 0;
	int x, y;

	read_input();

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			if (grid[y][x] == 0) {
				part1_score += count_reachable_nines(x, y);
				part2_score += count_trails_from_trailhead(x, y);
			}
		}
	}

	printf("part 1 =\n\t%d\n", part1_score);
	printf("part 2 =\n\t%d\n", part2_score);
	return 0;
}
