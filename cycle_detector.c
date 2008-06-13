#include <stdio.h>
#include <setjmp.h>  /* sigjmp_buf, siglongjmp() */
#include <stdlib.h>  /* realloc() */
#include <string.h>  /* strcmp() */

#include <node.h>
#include <graph.h>
#include <buffer.h>
#include <cycle_detector.h>

extern sigjmp_buf in_reduce_graph;

static char **cycle_stack = NULL;
static int    cycle_stack_depth = 0;
static int    cycle_stack_size = 0;

void setup_detection(void)
{
}

void free_detection(void)
{
	reset_detection();
	if (cycle_stack) free(cycle_stack);
	cycle_stack = NULL;
}

void reset_detection(void)
{
	while (cycle_stack_depth)
		free(cycle_stack[--cycle_stack_depth]);
}

int cycle_detector(struct node *root)
{
	char *graph = NULL;
	int i;
	int detected_cycle = 0;

	graph = canonicalize_graph(root->left);

	for (i = cycle_stack_depth; i > 0; --i)
	{
		if (!strcmp(graph, cycle_stack[i-1]))
		{
			free(graph);
			graph = NULL;
			printf("Found a cycle of length %d\n", (cycle_stack_depth - i + 1));
			reset_detection();
			detected_cycle = 1;
		}
	}

	if (i == 0 && !detected_cycle )
	{
		if (cycle_stack_depth == cycle_stack_size)
		{
			int new_size = cycle_stack_size + 10;
			char **new_array = realloc(cycle_stack, new_size*sizeof(cycle_stack[0]));

			if (new_array)
			{
				cycle_stack = new_array;
				cycle_stack_size = new_size;
			}
			/* If realloc() returns NULL, something's gone haywire.
			 * Not sure what to do here. */
		}

		cycle_stack[cycle_stack_depth++] = graph;
	}

	return detected_cycle;
}
