#include <stdio.h>
#include <stdlib.h>  /* free() */
#include <string.h>

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <aho_corasick.h>
#include <algorithm_d.h>

static char **paths = NULL;
static int path_cnt = 0;
static int paths_used = 0;

struct stack_elem {
	struct node *n; /* 1 */
	int state_at_n; /* 2 */
	int visited;    /* 3 */
	int node_number;
};

void calculate_strings(struct node *node, struct buffer *buf);
int tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count);

int
tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count)
{
	int found_match = 0;
	int i;
	struct output_extent *oxt;

	if (state < 0)
		return found_match;

	oxt = &(g->output[state]);

	for (i = 0; i < oxt->len && !found_match; ++i)
	{
		int s = oxt->out[i];
		int x = top - s + 1;

		if (++count[stack[x].node_number] == pat_leaf_count)
			found_match = 1;
	}

	return found_match;
}


int
algorithm_d(struct gto *g, struct node *t, int pat_path_cnt)
{
	int top = 1;
	int breadth_count = 0;
	int next_state;
	int node_cnt = 0;
	int *count;
	struct stack_elem *stack;
	const char *p;
	int matched = 0;

	node_cnt = node_count(t, 1);

	stack = malloc(node_cnt * sizeof(struct stack_elem));
	count = calloc(node_cnt, sizeof(int));

	next_state = 0;
	p = t->name;
	while ('\0' != *p)
		next_state = g->delta[next_state][(int)*p++];

	stack[top].n = t;
	stack[top].state_at_n = next_state;
	stack[top].visited = 0;
	stack[top].node_number = breadth_count++;

	if (tabulate(g, stack, top, next_state, pat_path_cnt, count))
	{
		matched = 1;
		goto matched_pattern;
	}

	while (top > 0)
	{
		struct node *next_node, *this_node = stack[top].n;
		int intstate, nxt_st, this_state = stack[top].state_at_n;
		int visited = stack[top].visited;

		if (visited == 2 || this_node->typ == ATOM)
			--top;
		else {
			++visited;
			stack[top].visited = visited;
			intstate = g->delta[this_state][visited == 1?'1':'2'];

			if (tabulate(g, stack, top, intstate, pat_path_cnt, count))
			{
				matched = 1;
				goto matched_pattern;
			}

			next_node = (visited == 1)? this_node->left: this_node->right;
			nxt_st = intstate;
			p = next_node->name;
			while ('\0' != *p)
				nxt_st = g->delta[nxt_st][(int)*p++];

			++top;
			stack[top].n = next_node;
			stack[top].state_at_n = nxt_st;
			stack[top].visited = 0;
			stack[top].node_number = breadth_count++;

			if (tabulate(g, stack, top, nxt_st, pat_path_cnt, count))
			{
				matched = 1;
				goto matched_pattern;
			}
		}
	}

	matched_pattern:

	free(stack);
	free(count);

	return matched;
}

char **
get_pat_paths(void)
{
	return paths;
}

void
free_paths(void)
{
	if (paths)
	{
		int i;
		for (i = 0; i < paths_used; ++i)
		{
			free(paths[i]);
			paths[i] = NULL;
		}
		free(paths);
		paths = NULL;
	}
	path_cnt = 0;
	paths_used = 0;
}

int
set_pattern_paths(struct node *pattern)
{
	struct buffer *buf = new_buffer(512);
	calculate_strings(pattern, buf);
	delete_buffer(buf);
	return paths_used;
}

void
calculate_strings(struct node *node, struct buffer *b)
{
	int curr_offset, orig_offset = b->offset;
	char *buf;
	char *pattern_string;

	switch (node->typ)
	{
	case APPLICATION:
		buffer_append(b, node->name, strlen(node->name));
		curr_offset = b->offset;

		buffer_append(b, "1", 1);
		calculate_strings(node->left, b);

		b->offset = curr_offset;
		buffer_append(b, "2", 1);

		calculate_strings(node->right, b);

		b->offset = orig_offset;
		break;
	case ATOM:
		buf = b->buffer;
		buf[b->offset] = '\0';
		pattern_string = malloc(b->offset + 1 + 1);

		if ('*' != node->name[0])
			sprintf(pattern_string, "%s%s", buf, node->name);
		else
			sprintf(pattern_string, "%s", buf);

		if (paths_used >= path_cnt)
		{
			char **tmp;
			int alloc_bytes = (sizeof(char *))*(path_cnt + 4);

			if (paths)
				tmp = realloc(paths, alloc_bytes);
			else
				tmp = malloc(alloc_bytes);

			paths = tmp;
			path_cnt += 4;
		}

		/* XXX - If a realloc() fails, this could overwrite paths[] */
		paths[paths_used++] = pattern_string;

		break;
	}
}
