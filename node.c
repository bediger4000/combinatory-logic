#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <node.h>
#include <arena.h>

extern int elaborate_output;

static struct memory_arena *arena = NULL;

static int malloced_node_count = 0;

struct node *new_node(void);

struct node *
new_application(struct node *left_child, struct node *right_child)
{
	struct node *r = new_node();

	r->typ = APPLICATION;
	r->name = NULL;
	r->right = right_child;
	r->left  = left_child;

	return r;
}

struct node *
new_combinator(const char *name)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->name = name;  /* assumes name allocated as Atom_t */
	r->right = r->left = NULL;

	return r;
}

void
print_tree(struct node *node)
{
	switch (node->typ)
	{
	case APPLICATION:
		putc('(', stdout);
		print_tree(node->left);
		if (elaborate_output)
			printf(" {%d} ", node->sn);
		else
			putc(' ', stdout);
		print_tree(node->right);
		putc(')', stdout);
		break;
	case COMBINATOR:
		if (elaborate_output)
			printf("%s{%d}", node->name, node->sn);
		else
			printf("%s", node->name);
		break;
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

	r = arena_alloc(arena, sizeof(*r));
	++malloced_node_count;
	r->sn = malloced_node_count;

	r->left = NULL;
	r->typ = UNTYPED;

	return r;
}

void
free_all_nodes(void)
{
	deallocate_arena(arena);
}

void
init_node_allocation(void)
{
	arena = new_arena();
}

void
reset_node_allocation(void)
{
	free_arena_contents(arena);
}

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = arena_alloc(arena, sizeof(*r));
	r->typ = p->typ;
	r->sn = -667;

	switch (p->typ)
	{
	case APPLICATION:
		r->name = NULL;
		r->left = arena_copy_graph(p->left);
		r->right = arena_copy_graph(p->right);
		break;
	case COMBINATOR:
		r->name = p->name;
		r->left = r->right = NULL;
		break;
	case UNTYPED:
		printf("Copying an UNTYPED node\n");
		r->name = NULL;
		r->left = r->right = NULL;
		break;
	default:
		printf("Copying n node of unknown (%d) type\n", p->typ);
		r->name = NULL;
		r->left = arena_copy_graph(p->left);
		r->right = arena_copy_graph(p->right);
		break;
	}
	return r;
}
