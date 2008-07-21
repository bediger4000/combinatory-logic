/*
	Copyright (C) 2008, Bruce Ediger

    This file is part of cl.

    cl is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    cl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cl; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#ifndef _TCC_
#ident "$Id$"
#endif

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
			break;
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
