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
	r->cn = COMB_NONE;
	r->right = right_child;
	r->left  = left_child;

	return r;
}

struct node *
new_combinator(enum combinatorName cn)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->cn = cn;
	r->right = r->left = NULL;
	r->name = (cn == COMB_S)? "S": (cn == COMB_K)? "K": (cn == COMB_I)? "I": (cn == COMB_B)? "B": (cn == COMB_C)? "C": "none";

	return r;
}

struct node *
new_term(const char *name)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->cn = COMB_NONE;
	r->name = name;
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

		if (node != node->left)
			print_tree(node->left);
		else
			printf("Left application loop: {%d}->{%d}\n",
				node->sn, node->left->sn);
		
		if (elaborate_output)
			printf(" {%d} ", node->sn);
		else
			putc(' ', stdout);

		if (node != node->right)
			print_tree(node->right);
		else
			printf("Right application loop: {%d}->{%d}\n",
				node->sn, node->right->sn);

		putc(')', stdout);

		break;
	case COMBINATOR:
		if (elaborate_output)
			printf("%s{%d}", node->name, node->sn);
		else
			printf("%s", node->name);
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
	r->cn  = COMB_NONE;

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

static int copy_sn = 0;

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = arena_alloc(arena, sizeof(*r));
	r->typ = p->typ;
	r->sn = --copy_sn;
	r->cn = COMB_NONE;

	switch (p->typ)
	{
	case APPLICATION:
		r->name = NULL;
		r->left = arena_copy_graph(p->left);
		r->right = arena_copy_graph(p->right);
		break;
	case COMBINATOR:
		r->cn   = p->cn;
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
