#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <node.h>

extern int elaborate_output;

static struct node *free_node_list = NULL;
static int malloced_node_count = 0;

struct node *new_node(void);
void free_node(struct node *old_node);

struct node *
new_application(struct node *left_child, struct node *right_child)
{
	struct node *r = new_node();

	r->typ = APPLICATION;
	r->name = NULL;
	r->right = right_child;
	r->left  = left_child;
	r->refcnt = 1;

	return r;
}

struct node *
new_combinator(const char *name)
{
	struct node *r = new_node();

	r->typ = COMBINATOR;
	r->name = name;  /* assumes name allocated as Atom_t */
	r->right = r->left = NULL;
	r->refcnt = 1;

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
			printf(" {%d}[%d] ", node->sn, node->refcnt);
		else
			putc(' ', stdout);
		print_tree(node->right);
		putc(')', stdout);
		break;
	case COMBINATOR:
		if (elaborate_output)
			printf("%s{%d}[%d]", node->name, node->sn, node->refcnt);
		else
			printf("%s", node->name);
		break;
	case UNALLOCATED:
		printf("UNALLOCATED {%d}[%d]",
			node->sn, node->refcnt);
		break;
	case UNTYPED:
		printf("UNTYPED {%d}[%d]",
			node->sn, node->refcnt);
		break;
	default:
		printf("Unknown %d {%d}[%d]",
			node->typ, node->sn, node->refcnt);
		break;
	}
}

struct node *
new_node(void)
{
	struct node *r = NULL;

	if (free_node_list)
	{
		r = free_node_list;
		free_node_list = r->left;
	} else {
		r = malloc(sizeof(*r));
		++malloced_node_count;
		r->sn = malloced_node_count;
	}

	r->left = NULL;
	r->typ = UNTYPED;

	return r;
}

void
free_node(struct node *old_node)
{
	return;
	if (NULL == old_node)
		return;

	--old_node->refcnt;

	if (old_node->refcnt < 0)
	{
		int tmp = elaborate_output;
		fprintf(stderr, "Freeing tree already freed:\n");
		elaborate_output = 1;
		print_tree(old_node);
		putc('\n', stdout);
		elaborate_output = tmp;
		abort();
	}

	if (old_node->refcnt > 0)
		return;

	if (APPLICATION == old_node->typ)
	{
		free_node(old_node->left);
		free_node(old_node->right);
	}

	old_node->left = free_node_list;
	free_node_list = old_node;
	free_node_list->typ = UNALLOCATED;
	free_node_list->right = NULL;
	free_node_list->name = NULL;
}

void
free_all_nodes(void)
{
	int freed_node_count = 0;
	while (free_node_list)
	{
		struct node *tmp = free_node_list->left;

		if (free_node_list->typ != UNALLOCATED)
		{
			fprintf(stderr, "Found free node on list with type %d\n", free_node_list->typ);
			abort();
		}
		if (free_node_list->refcnt != 0)
		{
			fprintf(stderr, "Found free node on list with type %d, ref cnt %d\n",
				free_node_list->typ, free_node_list->refcnt);
			abort();
		}
		free(free_node_list);
		++freed_node_count;
		free_node_list = tmp;
	}

	if (freed_node_count != malloced_node_count)
		fprintf(stderr, "Malloc %d nodes, free %d\n",
			malloced_node_count, freed_node_count);
}
