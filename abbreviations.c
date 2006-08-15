/*
    Copyright (C) 2006, Bruce Ediger

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
#include <stdlib.h>  /* malloc(), free() */

#include <node.h>
#include <hashtable.h>
#include <abbreviations.h>
#include <graph.h>

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
	if (p) r = arena_copy_graph((struct node *)p);
	return r;
}

struct node *
abbreviation_add(const char *id, struct node *exp)
{
	struct node *r = NULL;
	struct hashnode *n = NULL;
	unsigned int hv;

	if ((n = node_lookup(abbr_table, id, &hv)))
	{
		r = (struct node *)n->data;
		n->data = (void *)copy_graph(exp);
	} else {
		(void)add_data(abbr_table, id, (void *)copy_graph(exp));
	}

	return r;
}
