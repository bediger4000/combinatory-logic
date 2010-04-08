/*
	Copyright (C) 2008-2009, Bruce Ediger

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

/* $Id$ */

#include <stdio.h>
#include <setjmp.h>  /* sigjmp_buf, siglongjmp() */
#include <stdlib.h>  /* realloc() */
#include <string.h>  /* strcmp() */

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <cycle_detector.h>

extern sigjmp_buf in_reduce_graph;

static char **cycle_stack = NULL;
static int    cycle_stack_depth = 0;
static int    cycle_stack_size = 0;

int find_trivial_cycle(struct node *node, struct node *parent, struct node *gparent, int stack_depth);

void
free_detection(void)
{
	reset_detection();
	if (cycle_stack) free(cycle_stack);
	cycle_stack = NULL;
}

void
reset_detection(void)
{
	while (cycle_stack_depth)
		free(cycle_stack[--cycle_stack_depth]);
}

int
cycle_detector(struct node *root, int max_redex_count)
{
	char *graph = NULL;
	int i;
	int detected_cycle = 0;

	graph = canonicalize_graph(root->left);

	for (i = cycle_stack_depth; i > 0; --i)
	{
		if (!strcmp(graph, cycle_stack[i-1]))
		{
			int trivial_cycle = (find_trivial_cycle(root->left, NULL, NULL, 0) == 1);

			/* Gotta say that this is a *very* low level routine in which
			 * to perform output.  Unfortunately, unless I pass a whole
			 * bunch of flags back up the call-chain, I can't communicate
			 * all the information I'd like to.  So despite the ugliness,
			 * the output stays here. */
			printf("Found a %s%scycle of length %d, %d terms evaluated, ends with \"%s\"\n",
				(max_redex_count == 1)? "pure ": "", 
				trivial_cycle? "trivial ": "", 
				(cycle_stack_depth - i + 1),
				cycle_stack_depth,
				graph
			);
			free(graph);
			graph = NULL;
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

int
find_trivial_cycle(struct node *node, struct node *parent, struct node *gparent, int stack_depth)
{
	if (node)
	{
		switch (node->typ)
		{
		case APPLICATION:
			if (!node->left && !node->right) return 0;

			if (find_trivial_cycle(node->left, node, parent, stack_depth + 1) > 0)
				return 1;

			if (find_trivial_cycle(node->right, NULL, NULL, 0) > 0)
				return 1;

			break;

		case ATOM:
			switch (node->cn)
			{
			case COMB_M:
				if (stack_depth > 0)
					if (parent->right->typ == ATOM && parent->right->cn == COMB_M)
						return 1;
				break;
			case COMB_W:
				if (stack_depth > 1)
				{
					if (parent->right->typ == ATOM && parent->right->cn == COMB_W
						&& gparent->right->typ == ATOM && gparent->right->cn == COMB_W)
							return 1;
				}
				break;
			case COMB_I:
				if (stack_depth > 0)
					return 0;
				break;
			case COMB_K:
			case COMB_T:
				if (stack_depth > 0)
					return 0;
			case COMB_B:
			case COMB_C:
			case COMB_S:
				if (stack_depth > 2)
					return 0;
			case COMB_J:
				if (stack_depth > 3)
					return 0;
				break;
			case COMB_NONE:
				break;
			}
			break;
		}
	}

	return -1;
}
