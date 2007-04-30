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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <node.h>
#include <arena.h>

extern int elaborate_output;
extern int debug_reduction;

static struct memory_arena *arena = NULL;

/* sn_counter - give a serial number (sn field) to
 * all nodes, so as to distinguish them in elaborate output.
 * Note that 0 constitutes a special value. */
static int sn_counter = 0;
static int reused_node_count = 0;
static int allocated_node_count = 0;  /* Not total. In a particular arena. */
static int new_node_cnt;

extern int interpreter_interrupted;
extern int reduction_interrupted;

static struct node *node_free_list = NULL;

struct node *new_node(void);

struct node *
new_application(struct node *left_child, struct node *right_child)
{
	struct node *r = new_node();

	r->typ = APPLICATION;
	r->name = NULL;
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
new_combinator(enum combinatorName cn)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->cn = cn;
	switch (r->cn)
	{
	case COMB_S: r->name = "S"; break;
	case COMB_K: r->name = "K"; break;
	case COMB_I: r->name = "I"; break;
	case COMB_B: r->name = "B"; break;
	case COMB_C: r->name = "C"; break;
	case COMB_W: r->name = "W"; break;
	case COMB_NONE:
	default: r->name = "none"; break;
	}

	return r;
}

struct node *
new_term(const char *name)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->cn = COMB_NONE;
	r->name = name;

	return r;
}

void
print_tree(struct node *node, int reduction_node_sn, int current_node_sn)
{
	switch (node->typ)
	{
	case APPLICATION:

		if (node != node->left)
			print_tree(node->left, reduction_node_sn, current_node_sn);
		else
			printf("Left application loop: {%d}->{%d}\n",
				node->sn, node->left->sn);
		
		if (elaborate_output)
		{
			printf(" {%d}", node->sn);
			if (node->sn == current_node_sn)
				printf("+ ");
			else
				putc(' ', stdout);
		} else {
			if (node->sn == current_node_sn)
				printf(" + ");
			else
				putc(' ', stdout);
		}

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
				print_tree(node->right, reduction_node_sn, current_node_sn);
				if (print_right_paren)
					putc(')', stdout);
			} else if (elaborate_output)
				printf(" {%d}", node->sn);
		} else
			printf("Right application loop: {%d}->{%d}\n",
				node->sn, node->right->sn);

		break;
	case COMBINATOR:
		if (elaborate_output)
			printf("%s{%d}", node->name, node->sn);
		else
			printf(
				(node->sn != reduction_node_sn)? "%s": "%s*",
				node->name
			);
		if (node->sn == current_node_sn)
			putc('+', stdout);
		break;
	case UNTYPED:
		printf("UNTYPED {%d}", node->sn);
		break;
	default:
		printf("Unknown %d {%d}", node->typ, node->sn);
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
		++sn_counter;
		++allocated_node_count;
		r->sn = sn_counter;
	}

	/* r->sn stays unchanged throughout */
	r->right = r->left = NULL;
	r->typ = UNTYPED;
	r->cn  = COMB_NONE;
	r->name = NULL;
	r->examined = 0;
	r->updateable = NULL;
	r->branch_marker = 0;
	r->refcnt = 0;

	return r;
}

void
free_all_nodes(int memory_info_flag)
{

	if (memory_info_flag)
	{
		fprintf(stderr, "Gave out %d nodes of %d bytes each in toto.\n",
			new_node_cnt, sizeof(struct node));
		fprintf(stderr, "%d nodes allocated from arena, %d from free list\n",
			sn_counter, reused_node_count);
			
	}
	deallocate_arena(arena, memory_info_flag);
}

void
init_node_allocation(int memory_info_flag)
{
	arena = new_arena(memory_info_flag);
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
			if (debug_reduction)
				fprintf(stderr, "Node %d, ref cnt %d on free list\n",
					p->sn, p->refcnt);
			p = p->right;
			if (free_list_cnt > allocated_node_count) break;
		}

		if (free_list_cnt != allocated_node_count)
			fprintf(stderr, "Allocated %d nodes, but found %s %d on free list\n",
				allocated_node_count,
				free_list_cnt >allocated_node_count? "at least": "only",
				free_list_cnt);
	}

	node_free_list = 0;
	allocated_node_count = 0;

	free_arena_contents(arena);
}

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = new_node();

	r->typ = p->typ;

	switch (p->typ)
	{
	case APPLICATION:
		r->left = arena_copy_graph(p->left);
		++r->left->refcnt;
		r->right = arena_copy_graph(p->right);
		++r->right->refcnt;
		break;
	case COMBINATOR:
		r->name = p->name;
		r->cn = p->cn;
		break;
	case UNTYPED:
		printf("Copying an UNTYPED node\n");
		break;
	default:
		printf("Copying n node of unknown (%d) type\n", p->typ);
		r->left = arena_copy_graph(p->left);
		++r->left->refcnt;
		r->right = arena_copy_graph(p->right);
		++r->right->refcnt;
		break;
	}
	return r;
}

void
free_node(struct node *node)
{
	if (NULL == node) return;  /* dummy root nodes have NULL right field */

	if (debug_reduction)
		fprintf(stderr, "Freeing node %d, ref cnt %d\n",
			node->sn, node->refcnt);

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
		fprintf(stderr, "Freeing node %d, negative ref cnt %d\n",
			node->sn, node->refcnt);
}
