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


enum nodeType { APPLICATION, ATOM };
enum primitiveName { COMB_NONE = 0, COMB_S = 1, COMB_K = 2, COMB_I = 3, COMB_C = 4, COMB_B = 5, COMB_W = 6, COMB_T = 7, COMB_M  = 8, COMB_J = 9};

struct node {
	enum nodeType typ;
	enum primitiveName cn;
	const char *name;
	struct node *left;
	struct node *right;
	struct node **updateable;
	struct node **left_addr;
	struct node **right_addr;
	int refcnt;
	int tree_size;
};

struct node *new_application(struct node *left_child, struct node *right_child);
struct node *new_combinator(enum primitiveName cn);
struct node *new_term(const char *name);

void init_node_allocation(void);
void reset_node_allocation(void);
void preallocate_nodes(int node_count);
void print_tree(struct node *root);
void free_all_nodes(int memory_info_flag);
void free_node(struct node *root);

struct node * arena_copy_graph(struct node *root);
