/*
    Copyright (C) 2006-2009, Bruce Ediger

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
#include <stdlib.h>  /* malloc(), free() */

#include <node.h>
#include <hashtable.h>
#include <abbreviations.h>

struct hashtable *abbr_table = NULL;

void
setup_abbreviation_table(struct hashtable *h)
{
	abbr_table = h;
}

struct node *
abbreviation_lookup(const char *id)
{
	struct node *r = NULL;
	void *p = data_lookup(abbr_table, id);
	/* make a "permanent" copy of the parse tree, so that
	 * reduce_graph() can destructively reduce the resulting
	 * parse tree. */
	if (p)
	{
		preallocate_nodes(((struct node *)p)->tree_size);
		r = arena_copy_graph((struct node *)p);
	}
	return r;
}

void
abbreviation_add(const char *id, struct node *expr)
{
	struct hashnode *n = NULL;
	unsigned int hv;

	/* By the time flow-of-control arrives here,
	 * the string (variable id) has already gotten
	 * in the hashtable. Only 1 struct hashtable exists,
	 * used for both unique-string-saving, and storage
	 * of "abbreviations". The lexer code guarantess
	 * it by calling Atom_string() on all input strings.
	 * Therefore, this code *always* finds string id
	 * in the abbr_table.
	 */
	n = node_lookup(abbr_table, id, &hv);

	free_graph((struct node *)n->data);

	/* Make a "permanent" copy of the parse tree, so that
	 * we can reset the memory arena used in node.c at
	 * will and not smash the tree held by the hashtable
	 * as an abbreviation. */
	n->data = (void *)copy_graph(expr);
}

/* Copy a parse tree/reduction graph into malloc'ed,
 * *not* arena-allocated, memory. */
struct node *
copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = malloc(sizeof(*r));
	r->typ = p->typ;
	r->cn = COMB_NONE;

	switch (p->typ)
	{
	case APPLICATION:
		r->name = NULL;
		r->left = copy_graph(p->left);
		r->right = copy_graph(p->right);
		r->tree_size = r->left->tree_size + r->right->tree_size + 1;
		break;
	case ATOM:
		r->name = p->name;
		r->cn = p->cn;
		r->left = r->right = NULL;
		r->tree_size = 1;
		break;
	}
	return r;
}

void
free_graph(struct node *p)
{
	while (p)
	{
		struct node *tmp = p->right;

		free_graph(p->left);

		p->name = NULL;
		p->left = p->right = NULL;
		p->typ = -1;

		free(p);

		p = tmp;
	}
}
