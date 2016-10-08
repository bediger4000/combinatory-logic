/*
    Copyright (C) 2008-2011, Bruce Ediger

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

/* expr_parser: turn a "canonical" representation of a combinatory logic
 * expression into a human readable form.  The canonical form does not
 * use parentheses, but rather has a "." for each application.  Function
 * application is explicityly binary.
 * So, (K I) ends up as ".K I" canonically, and this program reverses the
 * form.  ".K I" on stdin ends up as "K I" on stdout.
 */


#include <stdio.h>
#include <string.h>  /* strlen() */
#include <stdlib.h>  /* malloc(), free() */


enum node_type {LEAF, APPLICATION};

struct node {
	enum node_type type;
	char *leaf_name;
	struct node *left;
	struct node *right;
};

struct buffer {
	char  *buf;
	size_t idx;
	size_t len; 
};

int main(int ac, char **av);

struct buffer *new_buffer(char *str);
char curr_char(struct buffer *b);

struct node *new_application(void);
struct node *new_leaf(char *name);
struct node *add_node(struct buffer *b);

struct node *
new_application(void)
{
	struct node *n = malloc(sizeof(*n));
	n->type = APPLICATION;
	n->leaf_name = NULL;
	n->left = NULL;
	n->right = NULL;
	return n;
}

struct node *
new_leaf(char *name)
{
	struct node *n = malloc(sizeof(*n));
	n->type = LEAF;
	n->leaf_name = name;
	n->left = NULL;
	n->right = NULL;
	return n;
}

void
free_tree(struct node *t)
{
	if (t->left) free_tree(t->left);
	t->left = NULL;
	if (t->right) free_tree(t->right);
	t->right = NULL;
	if (t->leaf_name) free(t->leaf_name);
	t->leaf_name = NULL;
	free(t);
	t = NULL;
}

struct node *
add_node(struct buffer *b)
{
	struct node *n = NULL;
	char *p;
	char c;

	switch (c = curr_char(b))
	{
	case '.':
		n = new_application();
		n->left  = add_node(b);
		if (!n->left) return NULL;
		n->right = add_node(b);
		if (!n->right)
		{
			free_tree(n);
			return NULL;
		}
		break;
	case '\n':
	case '\0':
		break;
	default:
		p = malloc(2);
		p[0] = c;
		p[1] = '\0';
		n = new_leaf(p);
		break;
	}

	return n;
}

void
print_node(struct node *n)
{
	int print_right_paren = 0;

	switch (n->type)
	{
	case APPLICATION:
		print_node(n->left);
		fputc(' ', stdout);

		/* Minimal parenthesization: only parenthesize arguments,
		 * and only then if an application comprises the argument. */
		if (APPLICATION == n->right->type)
		{
			print_right_paren = 1;
			fputc('(', stdout);
		}
		print_node(n->right);
		if (print_right_paren)
			fputc(')', stdout);

		break;
	case LEAF:
		fputs(n->leaf_name, stdout);
		break;
	}
}

int
main(int ac, char **av)
{
	char buf[BUFSIZ];

	while (fgets(buf, sizeof(buf), stdin))
	{
		struct buffer *b;
		struct node *t;
		buf[strlen(buf) - 1] = '\0';
		b = new_buffer(&buf[0]);
		t = add_node(b);
		if (t)
		{
			print_node(t);
			fputc('\n', stdout);
			free_tree(t);
		} else
			printf("Bad parse\n");
		free(b);
	}

	return 0;
}

struct buffer *
new_buffer(char *str)
{
	struct buffer *r = malloc(sizeof(*r));
	r->buf = str;
	r->len = strlen(str);
	r->idx = 0;
	return r;
}


char
curr_char(struct buffer *b)
{
	char r = '\0';
	if (b->idx < b->len)
	{
		r = b->buf[b->idx];
		++b->idx;
	}
	if (' ' == r) r = curr_char(b);
	return r;
}
