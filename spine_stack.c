/*
	Copyright (C) 2007, Bruce Ediger

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

static struct spine_stack *spine_stack_free_list = NULL;

static int stack_malloc_cnt = 0;
static int stack_reused_cnt = 0;
static int new_stack_cnt   = 0;

extern int interpreter_interrupted;

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	++new_stack_cnt;

	if (spine_stack_free_list)
	{
		/* dual use of prev field: linked list of "free" spine stacks,
		 * and FIFO stack of actually in-use spine stacks. */
		r = spine_stack_free_list;
		spine_stack_free_list = r->prev;
		r->prev = NULL;
		++stack_reused_cnt;
	} else {
		r = malloc(sizeof(*r));
		r->stack = malloc(sz * sizeof(r->stack[0]));
		r->top   = 0;
		r->maxdepth = 0;
		r->size = sz;
		r->prev = NULL;
		r->sn = ++stack_malloc_cnt;
	}

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	/* dual use of prev field: linked list of "free" spine stacks,
	 * and FIFO stack of actually in-use spine stacks. */
	ss->prev = spine_stack_free_list;
	spine_stack_free_list = ss;
}

void
push_spine_stack(struct spine_stack **ss)
{
	/* dual use of prev field: linked list of "free" spine stacks,
	 * and FIFO stack of actually in-use spine stacks.
 	 * This constitutes the FIFO stack of in-use spine stacks usage. */
	struct spine_stack *p = new_spine_stack(32);
	p->prev = *ss;
	*ss = p;
}

void
pop_spine_stack(struct spine_stack **ss)
{
	/* FIFO-stack-of-in-use-stacks use of prev field */
	struct spine_stack *tmp = (*ss)->prev;
	delete_spine_stack(*ss);
	*ss = tmp;
}

void
free_all_spine_stacks(int memory_info_flag)
{
	int maxdepth = 0;
	int maxsize  = 0;
	int depth_sum = 0;
	int stack_freed_cnt = 0;
	while (spine_stack_free_list)
	{
		struct spine_stack *tmp = spine_stack_free_list->prev;
		if (spine_stack_free_list->maxdepth > maxdepth)
			maxdepth = spine_stack_free_list->maxdepth;
		if (spine_stack_free_list->size > maxsize)
			maxsize = spine_stack_free_list->size; 
		depth_sum += spine_stack_free_list->maxdepth;
		free(spine_stack_free_list->stack);
		spine_stack_free_list->stack = NULL;
		free(spine_stack_free_list);
		spine_stack_free_list = tmp;
		++stack_freed_cnt;
	}

	if (!interpreter_interrupted && stack_freed_cnt != stack_malloc_cnt)
		fprintf(stderr, "Allocated %d spine stacks, freed %d\n",
			stack_malloc_cnt, stack_freed_cnt);

	if (memory_info_flag)
	{
		float mean_depth = stack_freed_cnt? (float)depth_sum/(float)stack_freed_cnt: 0.0;
		fprintf(stderr, "Gave out %d spine stacks\n", new_stack_cnt);
		fprintf(stderr, "Allocated %d spine stacks\n", stack_malloc_cnt);
		fprintf(stderr, "Reused %d spine stacks\n", stack_reused_cnt);
		fprintf(stderr, "Maximum spine stack depth: %d\n", maxdepth);
		fprintf(stderr, "Maximum spine stack size:  %d\n", maxsize);
		fprintf(stderr, "Mean spine stack depth:    %.02f\n", mean_depth);
	}
}

void
pushnode(struct spine_stack *ss, struct node *n)
{
	ss->stack[ss->top] = n;

	++ss->top;

	if (ss->top >= ss->size)
	{
		/* resize the allocation pointed to by stack */
		struct node **old_stack = ss->stack;
		size_t new_size = ss->size * 2;  /* XXX !!! */
		ss->stack = realloc(old_stack, sizeof(struct node *)*new_size);
		if (!ss->stack)
			ss->stack = old_stack;  /* realloc failed */
		else
			ss->size = new_size;
	}
}
