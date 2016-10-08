/*
	Copyright (C) 2007-2009, Bruce Ediger

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


#include <stdlib.h>
#include <stdio.h>

#include <spine_stack.h>
#include <node.h>

struct spine_stack *old_spine_stack = NULL;
static int spine_stack_resizes = 0;

extern int interpreter_interrupted;

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	if (old_spine_stack)
	{
		r = old_spine_stack;
		r->top   = 0;
		/* Don't NULL out old_spine_stack: if someone control-c's
		 * the interpreter during a reduction, it might miss putting
		 * the spine stack back in place. */
	} else {
		r = malloc(sizeof(*r));
		r->stack = malloc(sz * sizeof(struct spine_stack_element));
		r->size = sz;
		r->maxdepth = 0;
		r->top   = 0;
	}

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	old_spine_stack = ss;
}


void
pushnode(struct spine_stack *ss, struct node *n, int mark)
{
	ss->stack[ss->top].node = n;

	ss->stack[ss->top].depth = (mark? mark: ss->stack[ss->top-1].depth + 1);

	++ss->top;

	if (ss->top > ss->maxdepth) ++ss->maxdepth;

	if (ss->top >= ss->size)
	{
		/* resize the allocation pointed to by stack */
		size_t new_size = ss->size * 2;  /* XXX !!! */
		++spine_stack_resizes;
		ss->stack = realloc(ss->stack, sizeof(struct spine_stack_element)*new_size);
		ss->size = new_size;
		/* assumes that realloc() never fails. */
	}
}

void
free_all_spine_stacks(int memory_info_flag)
{
	if (memory_info_flag)
	{
		fprintf(stderr, "Resized spine stack %d times\n", spine_stack_resizes);
		if (old_spine_stack)
		{
			fprintf(stderr, "Spine stack size %d elements\n", old_spine_stack->size);
			fprintf(stderr, "Spine stack reached max depth of %d\n", old_spine_stack->maxdepth);
		}
	}

	if (old_spine_stack)
	{
		free(old_spine_stack->stack);
		old_spine_stack->stack = NULL;
		old_spine_stack->top = 0;
		old_spine_stack->size = 0;
		free(old_spine_stack);
		old_spine_stack = NULL;
	}
}
