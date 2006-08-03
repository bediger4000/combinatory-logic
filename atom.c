/*
	Copyright (C) 2006, Bruce Ediger

    This file is part of lc.

    lc is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    lc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lc; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <string.h>
#include <hashtable.h>
#include <atom.h>

#ifndef _TCC_
#ident "$Id$"
#endif

static struct hashtable *atom_table = NULL;

void
setup_atom_table(struct hashtable *h)
{
	atom_table = h;
}

int
Atom_length(const char *str)
{
	int r = -1;
	unsigned int hv;
	struct hashnode *n;

	n = node_lookup(atom_table, str, &hv);
	if (n)
		r = n->string_length;
	return r;
}

const char *
Atom_new(const char *str, int len)
{
	return add_string(atom_table, str);
}

const char *
Atom_string(const char *str)
{
	return add_string(atom_table, str);
}
