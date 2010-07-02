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

struct spine_stack_element {
	struct node *node;
	int depth;
};

struct spine_stack {
	struct spine_stack_element *stack;
	int top;
	int size;
	int maxdepth;
};

struct spine_stack *new_spine_stack(int sz);
void pushnode(struct spine_stack *ss, struct node *n, int mark);
void delete_spine_stack(struct spine_stack *ss);
void free_all_spine_stacks(int memory_info_flag);

#define TOPNODE(ss) ((ss)->stack[(ss)->top - 1].node)
#define DEPTH(ss) ((ss)->stack[(ss)->top - 1].depth)
#define PARENTNODE(ss, N) ((ss)->stack[((ss)->top)-1-N].node)
#define POP(ss, N)  (((ss)->top)-=N)
#define STACK_SIZE(ss)  ((ss)->top)
#define STACK_NOT_EMPTY(ss) ((ss)->top > 0)
