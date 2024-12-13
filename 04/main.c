#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
	FILE *fp;
	char line[1024];
	char **grid;
	int rows, cols, i, j, dx, dy;
	int xmas, mas, diag1, diag2;

	fp = fopen("input.txt", "r");
	if (fp == NULL) {
		printf("cannot open file\n");
		return 1;
	}

	/* count dimensions */
	rows = cols = 0;
	while (fgets(line, sizeof(line), fp)) {
		line[strcspn(line, "\n")] = 0;
		if (rows == 0)
			cols = strlen(line);
		rows++;
	}

	/* read grid */
	rewind(fp);
	grid = malloc(rows * sizeof(char *));
	for (i = 0; i < rows; ++i) {
		grid[i] = malloc(cols);
		if (fgets(line, sizeof(line), fp)) {
			line[strcspn(line, "\n")] = 0;
			memcpy(grid[i], line, cols);
		}
	}

	/* part 1 -- find XMAS */
	xmas = 0;
	for (i = 0; i < rows; ++i) {
		for (j = 0; j < cols; ++j) {
			for (dx = -1; dx <= 1; ++dx) {
				for (dy = -1; dy <= 1; ++dy) {
					if (dx == 0 && dy == 0)
						continue;
					if (i + 3*dx < 0 || i + 3*dx >= rows)
						continue;
					if (j + 3*dy < 0 || j + 3*dy >= cols)
						continue;
					if (grid[i][j] == 'X' &&
					    grid[i+dx][j+dy] == 'M' &&
					    grid[i+2*dx][j+2*dy] == 'A' &&
					    grid[i+3*dx][j+3*dy] == 'S')
						++xmas;
				}
			}
		}
	}
	printf("XMAS found =\n\t%d\n", xmas);

	/* part 2 -- find X pattern of MAS */
	mas = 0;
	for (i = 1; i < rows-1; ++i) {
		for (j = 1; j < cols-1; ++j) {
			if (grid[i][j] != 'A')  /* center must be 'A' */
				continue;

			/* check both diagonal pairs for MAS/SAM */
			diag1 = ((grid[i-1][j-1] == 'M' && grid[i+1][j+1] == 'S') ||
				(grid[i-1][j-1] == 'S' && grid[i+1][j+1] == 'M'));

			diag2 = ((grid[i-1][j+1] == 'M' && grid[i+1][j-1] == 'S') ||
				(grid[i-1][j+1] == 'S' && grid[i+1][j-1] == 'M'));

			if (diag1 && diag2)
				++mas;
		}
	}
	printf("MAS found =\n\t%d\n", mas);

	/* cleanup */
	for (i = 0; i < rows; ++i)
		free(grid[i]);
	free(grid);
	fclose(fp);
	return 0;
}
