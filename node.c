#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <node.h>
#include <arena.h>

extern int elaborate_output;

static struct memory_arena *arena = NULL;

/* malloced_node_count - give a serial number (sn field) to
 * all nodes, so as to distinguish them in elaborate output.
 * Note that 0 constitutes a special value. */
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
print_tree(struct node *node, int reduction_node_sn, int current_node_sn)
{
	switch (node->typ)
	{
	case APPLICATION:

		if (node != node->left)
			print_tree(node->left, reduction_node_sn, current_node_sn);
		else
			printf("Left application loop: {%d}->{%d}\n",
				node->sn, node->left->sn);
		
		if (elaborate_output)
		{
			printf(" {%d}", node->sn);
			if (node->sn == current_node_sn)
				printf("+ ");
			else
				putc(' ', stdout);
		} else {
			if (node->sn == current_node_sn)
				printf(" + ");
			else
				putc(' ', stdout);
		}

		if (node != node->right)
		{
			int print_right_paren = 0;
			if (APPLICATION == node->right->typ)
			{
				putc('(', stdout);
				print_right_paren = 1;
			}
			print_tree(node->right, reduction_node_sn, current_node_sn);
			if (print_right_paren)
				putc(')', stdout);
		} else
			printf("Right application loop: {%d}->{%d}\n",
				node->sn, node->right->sn);

		break;
	case COMBINATOR:
		if (elaborate_output)
			printf("%s{%d}", node->name, node->sn);
		else
			printf(
				(node->sn != reduction_node_sn)? "%s": "%s*",
				node->name
			);
		if (node->sn == current_node_sn)
			putc('+', stdout);
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
	r->examined = 0;

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

void
copy_node_attrs(struct node *to, struct node *from)
{
	to->typ   = from->typ;
	to->cn    = from->cn;
	to->name  = from->name;
	to->left  = from->left;
	to->right = from->right;
	to->examined = from->examined;
}

static int copy_sn = 0;

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = arena_alloc(arena, sizeof(*r));
	r->sn = --copy_sn;

	r->typ = p->typ;
	r->cn = COMB_NONE;
	r->name = NULL;
	r->left = r->right = NULL;
	r->examined = 0;

	switch (p->typ)
	{
	case APPLICATION:
		r->left = arena_copy_graph(p->left);
		r->right = arena_copy_graph(p->right);
		break;
	case COMBINATOR:
		r->name   = p->name;
		r->cn   = p->cn;
		break;
	case UNTYPED:
		printf("Copying an UNTYPED node\n");
		break;
	default:
		printf("Copying n node of unknown (%d) type\n", p->typ);
		r->left = arena_copy_graph(p->left);
		r->right = arena_copy_graph(p->right);
		break;
	}
	return r;
}
