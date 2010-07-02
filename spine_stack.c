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

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>

#include <spine_stack.h>
#include <node.h>

static int spine_stack_resizes = 0;

extern int interpreter_interrupted;

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	r = malloc(sizeof(*r));
	r->stack = malloc(sz * sizeof(r->stack[0]));
	r->depth = malloc(sz * sizeof(int));
	r->size = sz;
	r->maxdepth = 0;
	r->top   = 0;

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	free(ss->depth);
	free(ss->stack);
	ss->depth = NULL;
	ss->stack = NULL;
	ss->top = 0;
	ss->size = 0;
	free(ss);
}


void
pushnode(struct spine_stack *ss, struct node *n, int mark)
{
	ss->stack[ss->top] = n;

	ss->depth[ss->top] = (mark? mark: ss->depth[ss->top-1] + 1);

	++ss->top;

	if (APPLICATION == n->typ)
	{
		n->left->examined = 0;
		n->right->examined = 0;
	}

	if (ss->top > ss->maxdepth) ++ss->maxdepth;

	if (ss->top >= ss->size)
	{
		/* resize the allocation pointed to by stack */
		struct node **old_stack = ss->stack;
		int  *old_depth = ss->depth;
		size_t new_size = ss->size * 2;  /* XXX !!! */
		++spine_stack_resizes;
		ss->stack = realloc(old_stack, sizeof(struct node *)*new_size);
		if (!ss->stack)
			ss->stack = old_stack;  /* realloc failed */
		else {
			ss->size = new_size;
			ss->depth = realloc(old_depth, sizeof(int)*new_size);
			if (!ss->depth)
				ss->depth = old_depth;  /* realloc failed */
		}
	}
}
