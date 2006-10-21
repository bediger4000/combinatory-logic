#include <stdio.h>
#include <stdlib.h>  /* malloc() and free() */

#include <node.h>
#include <graph.h>
#include <spine_stack.h>

extern int debug_reduction;
extern int elaborate_output;

void print_graph(struct node *node, int sn_to_reduce, int current_sn)
{
	print_tree(node, sn_to_reduce, current_sn);
	putc('\n', stdout);
}

void
reduce_graph(struct node *root)
{
	struct spine_stack *stack = NULL;

	push_spine_stack(&stack);

	PUSHNODE(stack, root);

	do {


		while (STACK_NOT_EMPTY(stack))
		{
			switch (TOPNODE(stack)->typ)
			{
			case APPLICATION:
				if (!LEFT_BRANCH_TRAVERSED(TOPNODE(stack)))
				{
					MARK_LEFT_BRANCH_TRAVERSED(TOPNODE(stack));
					PUSHNODE(stack, TOPNODE(stack)->left);
				} else if (!RIGHT_BRANCH_TRAVERSED(TOPNODE(stack))) {
					struct node *tmp = TOPNODE(stack)->right;
					MARK_RIGHT_BRANCH_TRAVERSED(TOPNODE(stack));
					push_spine_stack(&stack);
					PUSHNODE(stack, tmp);
printf("push right branch on new stack\n");
				} else
					POP(stack, 1);
				break;
			case COMBINATOR:
				switch (TOPNODE(stack)->cn)
				{
				case COMB_I:
printf("I combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 1)
					{
printf("I reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);
						PARENTNODE(stack, 2)->left = PARENTNODE(stack, 1)->right;
						PARENTNODE(stack, 2)->examined = 0;
						POP(stack, 2);
printf("I reduction, after (%d): ", TOPNODE(stack)->sn); print_graph(root, 0, TOPNODE(stack)->sn);
					} else
						POP(stack, 1);
					break;
				case COMB_K:
printf("K combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 2)
					{
printf("K reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);
						copy_node_attrs(PARENTNODE(stack, 2),
							PARENTNODE(stack, 1)->right);
						PARENTNODE(stack, 2)->examined = 0;
						POP(stack, 2);
printf("K reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);
					} else
						POP(stack, 1);
					break;
				case COMB_S:
printf("S combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 3)
					{
						struct node *n3 = PARENTNODE(stack, 3);
printf("S reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);
						n3->left = new_application(
								PARENTNODE(stack, 1)->right,
								n3->right
							);
						n3->right = new_application(
								PARENTNODE(stack, 2)->right,
								n3->right
							);
						n3->examined = 0;
						POP(stack, 3);
printf("S reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);
					} else
						POP(stack, 1);
					break;
				case COMB_B:
printf("B combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 3)
					{
printf("B reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);
						PARENTNODE(stack, 3)->left
							= PARENTNODE(stack, 1)->right;
						PARENTNODE(stack, 3)->right
							= new_application(
								PARENTNODE(stack, 2)->right,
								PARENTNODE(stack, 3)->right
							);
						PARENTNODE(stack, 3)->examined = 0;
						POP(stack, 3);
printf("B reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);
					} else
						POP(stack, 1);
					break;
				case COMB_C:
printf("C combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 3)
					{
printf("C reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);
						PARENTNODE(stack, 3)->left
							= new_application(
								PARENTNODE(stack, 1)->right,
								PARENTNODE(stack, 3)->right
							);
						PARENTNODE(stack, 3)->right
							= PARENTNODE(stack, 2)->right;
						PARENTNODE(stack, 3)->examined = 0;
						POP(stack, 3);
printf("C reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);
					} else
						POP(stack, 1);
					break;
				case COMB_NONE:
					POP(stack, 1);
					break;
				}
				break;  /* end of case COMBINATOR */
			case UNTYPED:
				POP(stack, 1);
				break;
			}
		}

		pop_spine_stack(&stack);
printf("pop spine stack\n");

	} while (stack);
}

struct node *
copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = malloc(sizeof(*r));
	r->typ = p->typ;
	r->sn = -666;
	r->cn = COMB_NONE;

	switch (p->typ)
	{
	case APPLICATION:
		r->name = NULL;
		r->left = copy_graph(p->left);
		r->right = copy_graph(p->right);
		r->examined = 0;
		break;
	case COMBINATOR:
		r->name = p->name;
		r->cn = p->cn;
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
		r->left = copy_graph(p->left);
		r->right = copy_graph(p->right);
		break;
	}
	return r;
}

void
free_graph(struct node *p)
{
	if (!p) return;

	free_graph(p->left);
	free_graph(p->right);

	p->name = NULL;
	p->left = p->right = NULL;
	p->typ = -1;

	free(p);
}
