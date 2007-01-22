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
enum nodeType { UNTYPED, APPLICATION, COMBINATOR };
enum combinatorName { COMB_NONE, COMB_S, COMB_K, COMB_I, COMB_C, COMB_B, COMB_W };

struct node {
	int sn;
	enum nodeType typ;
	enum combinatorName cn;
	const char *name;
	struct node *left;
	struct node *right;
	struct node **updateable;
	int branch_marker;
	int examined;
	int refcnt;
};

#define LEFT  0x01
#define RIGHT 0x02
#define LEFT_BRANCH_TRAVERSED(n) ((n)->examined & LEFT)
#define MARK_LEFT_BRANCH_TRAVERSED(n) ((n)->examined |= LEFT)
#define RIGHT_BRANCH_TRAVERSED(n) ((n)->examined & RIGHT)
#define MARK_RIGHT_BRANCH_TRAVERSED(n) ((n)->examined |= RIGHT)

struct node *new_application(struct node *left_child, struct node *right_child);
struct node *new_combinator(enum combinatorName cn);
struct node *new_term(const char *name);

void init_node_allocation(int memory_info_flag);
void reset_node_allocation(void);
void print_tree(struct node *root, int reduction_node_sn, int current_node_sn);
void free_all_nodes(int memory_info_flag);
void free_node(struct node *root);

struct node * arena_copy_graph(struct node *root);

void copy_node_attrs(struct node *to, struct node *from);
