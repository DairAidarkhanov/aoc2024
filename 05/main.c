#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE	4096
#define MAX_RULES	4096
#define MAX_UPDATES	4096
#define MAX_PAGES	256

struct rule {
	int	before;
	int	after;
};

struct update {
	int	*pages;
	size_t	npages;
};

struct graph {
	int	*indegree;			/* count of incoming edges for each node */
	int	**adjacency;			/* adjacency matrix */
	int	max_node;			/* highest node value */
	int	*processed;			/* track which nodes are in our update */
};

static void
free_graph(struct graph *g)
{
	int i;

	if (g->adjacency != NULL) {
		for (i = 0; i <= g->max_node; ++i)
			free(g->adjacency[i]);
		free(g->adjacency);
	}
	free(g->indegree);
	free(g->processed);
	free(g);
}

static struct graph *
create_graph(struct rule *rules, size_t nrules, struct update *update)
{
	struct graph *g;
	size_t i;
	int j, max_node;

	/* find maximum node value */
	max_node = 0;
	for (i = 0; i < nrules; ++i) {
		if (rules[i].before > max_node)
			max_node = rules[i].before;
		if (rules[i].after > max_node)
			max_node = rules[i].after;
	}

	g = malloc(sizeof(*g));
	if (g == NULL)
		return NULL;

	/* initialize graph structure */
	g->max_node = max_node;
	g->indegree = calloc(max_node + 1, sizeof(*g->indegree));
	g->processed = calloc(max_node + 1, sizeof(*g->processed));
	g->adjacency = malloc((max_node + 1) * sizeof(*g->adjacency));

	if (g->indegree == NULL || g->processed == NULL || g->adjacency == NULL) {
		free_graph(g);
		return NULL;
	}

	/* initialize adjacency matrix */
	for (j = 0; j <= max_node; ++j) {
		g->adjacency[j] = calloc(max_node + 1, sizeof(**g->adjacency));
		if (g->adjacency[j] == NULL) {
			free_graph(g);
			return NULL;
		}
	}

	/* mark which nodes are in our update */
	for (i = 0; i < update->npages; ++i)
		g->processed[update->pages[i]] = 1;

	/* build graph from rules that apply to our nodes */
	for (i = 0; i < nrules; ++i) {
		int before = rules[i].before;
		int after = rules[i].after;

		/* only use rules where both nodes are in our update */
		if (g->processed[before] && g->processed[after]) {
			g->adjacency[before][after] = 1;
			++g->indegree[after];
		}
	}

	return g;
}

static int
topological_sort(struct graph *g, int *result, size_t result_size)
{
	int *queue;
	size_t queue_head, queue_tail, count;
	int i, j;

	queue = malloc((g->max_node + 1) * sizeof(*queue));
	if (queue == NULL)
		return -1;

	queue_head = queue_tail = count = 0;

	/* find all nodes with no incoming edges */
	for (i = 0; i <= g->max_node; ++i) {
		if (g->processed[i] && g->indegree[i] == 0)
			queue[queue_tail++] = i;
	}

	/* process queue */
	while (queue_head < queue_tail && count < result_size) {
		int node = queue[queue_head++];
		result[count++] = node;

		/* reduce indegree of all adjacent nodes */
		for (j = 0; j <= g->max_node; ++j) {
			if (g->adjacency[node][j]) {
				if (--g->indegree[j] == 0)
					queue[queue_tail++] = j;
			}
		}
	}

	free(queue);
	return count == result_size ? 0 : -1;
}

static void
free_updates(struct update *updates, size_t count)
{
	size_t i;

	for (i = 0; i < count; ++i)
		free(updates[i].pages);
	free(updates);
}

static char *
parse_num(char *str, int *num)
{
	char *endptr;
	long val;

	val = strtol(str, &endptr, 10);
	if (endptr == str || val < 0 || val > INT_MAX)
		return NULL;

	*num = (int)val;
	return endptr;
}

static int
parse_rule(char *line, struct rule *rule)
{
	char *p;

	/* parse first number */
	p = parse_num(line, &rule->before);
	if (p == NULL || *p != '|')
		return -1;

	/* skip the | */
	++p;

	/* parse second number */
	if (parse_num(p, &rule->after) == NULL)
		return -1;

	return 0;
}

static int
parse_update(char *line, struct update *update)
{
	char *p, *endp;
	int *pages;
	size_t npages;
	int num;

	pages = malloc(MAX_PAGES * sizeof(*pages));
	if (pages == NULL)
		return -1;

	npages = 0;
	p = line;

	while (*p != '\0' && *p != '\n') {
		/* Skip leading whitespace */
		while (isspace((unsigned char)*p))
			++p;

		if (*p == '\0' || *p == '\n')
			break;

		/* parse number */
		endp = parse_num(p, &num);
		if (endp == NULL || (npages < MAX_PAGES - 1 &&
		    (*endp != ',' && *endp != '\n' && *endp != '\0'))) {
			free(pages);
			return -1;
		}

		if (npages >= MAX_PAGES) {
			free(pages);
			return -1;
		}

		pages[npages++] = num;

		/* move to next number */
		p = endp;
		if (*p == ',')
			++p;
	}

	update->pages = pages;
	update->npages = npages;
	return 0;
}

static int
read_input(const char *filename, struct rule **rules, size_t *nrules,
	struct update **updates, size_t *nupdates)
{
	FILE *fp;
	char line[MAX_LINE];
	struct rule *r;
	struct update *u;
	size_t nr, nu;
	int in_updates;

	fp = fopen(filename, "r");
	if (fp == NULL)
		err(1, "fopen");

	r = malloc(MAX_RULES * sizeof(*r));
	u = malloc(MAX_UPDATES * sizeof(*u));
	if (r == NULL || u == NULL) {
		free(r);
		free(u);
		fclose(fp);
		return -1;
	}

	nr = 0;
	nu = 0;
	in_updates = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		/* skip empty lines */
		if (line[0] == '\n') {
			in_updates = 1;
			continue;
		}

		if (!in_updates) {
			if (nr >= MAX_RULES) {
				free(r);
				free(u);
				fclose(fp);
				return -1;
			}
			if (parse_rule(line, &r[nr]) == -1) {
				free(r);
				free(u);
				fclose(fp);
				return -1;
			}
			++nr;
		} else {
			if (nu >= MAX_UPDATES) {
				free(r);
				free_updates(u, nu);
				fclose(fp);
				return -1;
			}
			if (parse_update(line, &u[nu]) == -1) {
				free(r);
				free_updates(u, nu);
				fclose(fp);
				return -1;
			}
			++nu;
		}
	}

	if (ferror(fp)) {
		free(r);
		free_updates(u, nu);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	*rules = r;
	*nrules = nr;
	*updates = u;
	*nupdates = nu;
	return 0;
}

int
main(void)
{
	struct rule *rules;
	struct update *updates;
	struct graph *g;

	size_t nrules, nupdates;
	size_t i, j, k;

	int sum_middle_elements_part_1, sum_middle_elements_part_2;
	int valid;
	int before_pos, after_pos;
	int *sorted_pages;

	if (read_input("input.txt", &rules, &nrules, &updates, &nupdates) == -1)
		errx(1, "failed to parse input");


	/* part 1 */

	/* process each update */
	sum_middle_elements_part_1 = 0;

	for (i = 0; i < nupdates; ++i) {
		valid = 1;

		/* check all rules for this update */
		for (j = 0; j < nrules && valid; ++j) {
			before_pos = -1;
			after_pos = -1;

			/* find positions of both numbers from rule */
			for (k = 0; k < updates[i].npages; ++k) {
				if (updates[i].pages[k] == rules[j].before)
					before_pos = k;
				if (updates[i].pages[k] == rules[j].after)
					after_pos = k;
			}

			/* if both numbers exist in this update, check order */
			if (before_pos != -1 && after_pos != -1) {
				if (before_pos > after_pos) {
					valid = 0;
					break;
				}
			}
		}

		if (valid) {
			/* add middle element to sum */
			sum_middle_elements_part_1 += updates[i].pages[updates[i].npages / 2];
		}
	}

	printf("sum of middle elements from valid updates =\n\t%d\n", sum_middle_elements_part_1);


	/* part 2 */
	sum_middle_elements_part_2 = 0;

	sorted_pages = malloc(MAX_PAGES * sizeof(*sorted_pages));
	if (sorted_pages == NULL)
		err(1, "malloc");

	for (i = 0; i < nupdates; ++i) {
		valid = 1;

		/* check if this update is invalid */
		for (j = 0; j < nrules && valid; ++j) {
			before_pos = -1;
			after_pos = -1;

			for (k = 0; k < updates[i].npages; ++k) {
				if (updates[i].pages[k] == rules[j].before)
					before_pos = k;
				if (updates[i].pages[k] == rules[j].after)
					after_pos = k;
			}

			if (before_pos != -1 && after_pos != -1) {
				if (before_pos > after_pos) {
					valid = 0;
					break;
				}
			}
		}

		if (!valid) {
			/* create graph for this invalid update */
			g = create_graph(rules, nrules, &updates[i]);
			if (g == NULL)
				err(1, "create_graph");

			/* sort it */
			if (topological_sort(g, sorted_pages, updates[i].npages) == 0) {
				int middle = sorted_pages[updates[i].npages / 2];
				sum_middle_elements_part_2 += middle;
			}

			free_graph(g);
		}
	}

	printf("sum of middle elements from corrected invalid updates =\n\t%d\n", sum_middle_elements_part_2);


	/* cleanup */
	free(sorted_pages);
	free(rules);
	free_updates(updates, nupdates);

	return 0;
}
