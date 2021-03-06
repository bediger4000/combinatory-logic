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


struct node *curry_bracket_abstraction(struct node *var, struct node *tree);
struct node *curry2_bracket_abstraction(struct node *var, struct node *tree);
struct node *turner_bracket_abstraction(struct node *var, struct node *tree);
struct node *tromp_bracket_abstraction(struct node *var, struct node *tree);
struct node *grzegorczyk_bracket_abstraction(struct node *var, struct node *tree);
struct node *btmk_bracket_abstraction(struct node *var, struct node *tree);
struct node *ij_bracket_abstraction(struct node *var, struct node *tree);

typedef struct node *(*bracket_abstraction_function)(struct node *, struct node *);
bracket_abstraction_function determine_bracket_abstraction(const char *algorithm_name);
extern bracket_abstraction_function default_bracket_abstraction;
int equivalent_graphs(struct node *graph1, struct node *graph2);

