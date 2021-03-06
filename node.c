/*
	Copyright (C) 2007-2011, Bruce Ediger

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <node.h>
#include <arena.h>
#include <hashtable.h>
#include <atom.h>

extern int debug_reduction;

static struct memory_arena *arena = NULL;

/* sn_counter - give a serial number (sn field) to
 * all nodes, so as to distinguish them in elaborate output.
 * Note that 0 constitutes a special value. */
static int new_node_counter = 0;
static int reused_node_count = 0;
static int allocated_node_count = 0;  /* Not total. In a particular arena. */
static int new_node_cnt;

extern int interpreter_interrupted;
extern int reduction_interrupted;

static struct node *node_free_list = NULL;

struct node *new_node(void);

/* Since algorithm_d() looks at a pattern on a character-by-character
 * basis, it's OK to use r->name = "@" instead of r->name = atom_string("@")
 */
struct node *
new_application(struct node *left_child, struct node *right_child)
{
	struct node *r = new_node();

	r->typ = APPLICATION;
	r->name = "@";  /* algorithm_d() uses this */
	r->cn = COMB_NONE;
	r->right = right_child;
	r->left  = left_child;

	if (r->right)
		++r->right->refcnt;
	if (r->left)
		++r->left->refcnt;

	return r;
}

/* Why doesn't this use Atoms to do combinator names?
 * It potentially complicates comparing (for example)
 * the name of an "S" combinator from the Atom table,
 * and the name of an "S" combintor allocated here.
 */
struct node *
new_combinator(enum primitiveName cn)
{
	struct node *r = new_node();

	r->typ = ATOM;
	r->cn = cn;
	switch (r->cn)
	{
	case COMB_S: r->name = "S"; break;
	case COMB_K: r->name = "K"; break;
	case COMB_I: r->name = "I"; break;
	case COMB_B: r->name = "B"; break;
	case COMB_C: r->name = "C"; break;
	case COMB_W: r->name = "W"; break;
	case COMB_T: r->name = "T"; break;
	case COMB_M: r->name = "M"; break;
	case COMB_J: r->name = "J"; break;
	case COMB_NONE:
	default: r->name = "none"; break;
	}

	return r;
}

struct node *
new_term(const char *name)
{
	struct node *r = new_node();

	r->typ = ATOM;
	r->cn = COMB_NONE;
	r->name = name;

	return r;
}

void
print_tree(struct node *node)
{
	switch (node->typ)
	{
	case APPLICATION:

		if (!node->left && !node->right) return;

		if (node != node->left)
			print_tree(node->left);
		else
			printf("Left application loop\n");

		putc(' ', stdout);
		
		if (node != node->right)
		{
			if (node->right)
			{
				int print_right_paren = 0;
				if (APPLICATION == node->right->typ)
				{
					putc('(', stdout);
					print_right_paren = 1;
				}
				print_tree(node->right);
				if (print_right_paren)
					putc(')', stdout);
			}
		} else
			printf("Right application loop\n");

		break;
	case ATOM:
		printf("%s", node->name);
		break;
	}
}

struct node *
new_node(void)
{
	struct node *r = NULL;

	++new_node_cnt;

	if (node_free_list)
	{
		r = node_free_list;
		node_free_list = node_free_list->right;
		++reused_node_count;
	} else {
		r = arena_alloc(arena, sizeof(*r));
		++new_node_counter;
		++allocated_node_count;
		r->right_addr = &(r->right);
		r->left_addr = &(r->left);
	}

	/* r->sn stays unchanged throughout */
	r->right = r->left = NULL;
	r->cn  = COMB_NONE;
	r->name = NULL;
	r->updateable = NULL;
	r->refcnt = 0;
	r->tree_size = 0;

	return r;
}

void
free_all_nodes(int memory_info_flag)
{

	if (memory_info_flag)
	{
		fprintf(stderr, "Gave out %d nodes of %d bytes each in toto.\n",
			new_node_cnt, (int)sizeof(struct node));
		fprintf(stderr, "%d nodes allocated from arena, %d from free list\n",
			new_node_counter, reused_node_count);
			
	}
	deallocate_arena(arena, memory_info_flag);
}

void
init_node_allocation(void)
{
	arena = new_arena();
}

void
reset_node_allocation(void)
{
	if (!reduction_interrupted)
	{
		int free_list_cnt = 0;
		struct node *p = node_free_list;

		while (p)
		{
			++free_list_cnt;
			p = p->right;
			if (free_list_cnt > allocated_node_count) break;
		}

		if (free_list_cnt != allocated_node_count)
			fprintf(stderr, "Allocated %d nodes, but found %s %d on free list\n",
				allocated_node_count,
				free_list_cnt >allocated_node_count? "at least": "only",
				free_list_cnt);
	}

	node_free_list = NULL;
	allocated_node_count = 0;

	free_arena_contents(arena);
}

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	r = new_node();

	r->typ = p->typ;
	r->name = p->name;

	switch (p->typ)
	{
	case APPLICATION:
		r->left = arena_copy_graph(p->left);
		++r->left->refcnt;
		r->right = arena_copy_graph(p->right);
		++r->right->refcnt;
		break;
	case ATOM:
		r->cn = p->cn;
		break;
	}
	return r;
}

void
free_node(struct node *node)
{
	if (NULL == node) return;  /* dummy root nodes have NULL right field */

	--node->refcnt;

	if (node->refcnt == 0)
	{
		if (APPLICATION == node->typ)
		{
			free_node(node->left);
			free_node(node->right);
		}
		node->right = node_free_list;
		node_free_list = node;
	} else if (0 > node->refcnt)
		fprintf(stderr, "Freeing node with negative ref cnt %d\n",
			node->refcnt);
}

void
preallocate_nodes(int pre_node_count)
{
	size_t sz = pre_node_count * sizeof(struct node);
	struct node *node_ary = arena_alloc(arena, sz);
	int i;

	allocated_node_count += pre_node_count;

	for (i = 0; i < pre_node_count; ++i)
	{
		struct node *n = &node_ary[i];
		n->right_addr = &(n->right);
		n->left_addr = &(n->left);
		n->right = &node_ary[i+1];
		n->tree_size = 0;
	}
	node_ary[pre_node_count - 1].right = node_free_list;
	node_free_list = &node_ary[0];
}

