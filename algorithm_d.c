#include <stdio.h>
#include <stdlib.h>  /* free() */

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <pattern_paths.h>
#include <aho_corasick.h>
#include <algorithm_d.h>

void renumber(struct node *node, int *n);

struct stack_elem {
	struct node *n; /* 1 */
	int state_at_n; /* 2 */
	int visited;    /* 3 */
};
int tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count);

void
renumber(struct node *node, int *n)
{
	node->tree_size = *n;
	++*n;
	if (APPLICATION == node->typ)
	{
		renumber(node->left, n);
		renumber(node->right, n);
	}
}

int
tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count)
{
	int found_match = 0;
	int i;
	struct output_extent *oxt;

	if (state < 0)
		return found_match;

	oxt = &(g->output[state]);

	for (i = 0; i < oxt->len; ++i)
	{
		int s = oxt->out[i];
		struct node *n = stack[top - s + 1].n;
		count[n->tree_size] += 1;

		if (count[n->tree_size] == pat_leaf_count)
		{
			found_match = 1;
			break;
		}
	}

	return found_match;
}


int
algorithm_d(struct gto *g, struct node *t, int pat_path_cnt)
{
	int top = 1;
	int next_state;
	int i, node_cnt = 0;
	int subject_node_count = node_count(t, 1);
	int *count = malloc(subject_node_count * sizeof(int));
	struct stack_elem *stack
		= malloc(subject_node_count * sizeof(struct stack_elem));
	const char *p;

	for (i = 0; i < subject_node_count; ++i)
		count[i] = 0;

	renumber(t, &node_cnt);

	/* next_state = g->delta[0][(int)t->name]; */
	next_state = 0;
	p = t->name;
	while ('\0' != *p)
		next_state = g->delta[next_state][(int)*p++];

	stack[top].n = t;
	stack[top].state_at_n = next_state;
	stack[top].visited = 0;

	if (tabulate(g, stack, top, next_state, pat_path_cnt, count))
		return 1;

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
				return 1;

			next_node = (visited == 1)? this_node->left: this_node->right;
			/* nxt_st = g->delta[intstate][(int)next_node->name]; */
			nxt_st = intstate;
			p = next_node->name;
			while ('\0' != *p)
				nxt_st = g->delta[nxt_st][(int)*p++];

			++top;
			stack[top].n = next_node;
			stack[top].state_at_n = nxt_st;
			stack[top].visited = 0;

			if (tabulate(g, stack, top, nxt_st, pat_path_cnt, count))
				return 1;
		}
	}

	free(stack);
	free(count);

	return 0;
}
