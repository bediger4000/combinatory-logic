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

enum graphReductionResult { UNKNOWN, NORMAL_FORM, CYCLE_DETECTED, INTERRUPT, REDUCTION_LIMIT, TIMEOUT };

enum graphReductionResult reduce_graph(struct node *graph_root);
int  reduction_count(struct node *node, int stack_depth, int *child_reduces, struct buffer *b);
int  node_count(struct node *node, int count_interior_nodes);

void         print_graph(struct node *node, int node_sn_reducing, int current_node_sn);
char         *canonicalize_graph(struct node *node);
