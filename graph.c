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
#include <stdio.h>
#include <stdlib.h>  /* malloc() and free() */
#include <assert.h>
#include <setjmp.h>   /* longjmp(), jmp_buf */

#include <node.h>
#include <graph.h>
#include <spine_stack.h>

int read_line(void);

extern int trace_reduction;
extern int debug_reduction;
extern int elaborate_output;
extern int single_step;

extern jmp_buf in_reduce_graph;

#define D if(debug_reduction)
#define T if(trace_reduction)

/* can't do single_step && read_line() - compilers optimize it away */
#define SS if (single_step) read_line()

void print_graph(struct node *node, int sn_to_reduce, int current_sn)
{
	print_tree(node, sn_to_reduce, current_sn);
	putc('\n', stdout);
}

/* Graph reduction function. Destructively modifies the graph passed in.
 */
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
					TOPNODE(stack)->updateable = &(TOPNODE(stack)->left);
					TOPNODE(stack)->branch_marker = LEFT;
					MARK_LEFT_BRANCH_TRAVERSED(TOPNODE(stack));
					PUSHNODE(stack, TOPNODE(stack)->left);
					T printf("push left branch on current stack\n");
				} else if (!RIGHT_BRANCH_TRAVERSED(TOPNODE(stack))) {
					struct node *tmp = TOPNODE(stack);
					MARK_RIGHT_BRANCH_TRAVERSED(TOPNODE(stack));
					T printf("push right branch on new stack\n");
					TOPNODE(stack)->updateable = &(TOPNODE(stack)->right);
					TOPNODE(stack)->branch_marker = RIGHT;
					push_spine_stack(&stack);
					PUSHNODE(stack, tmp);  /* "dummy" node at top of stack */
					PUSHNODE(stack, tmp->right);
				} else
					POP(stack, 1);  /* both sides of application traversed */
				break;
			case COMBINATOR:
				if (stack->top > stack->maxdepth) stack->maxdepth = stack->top;
				switch (TOPNODE(stack)->cn)
				{
				case COMB_I:
					D {
						printf("I combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
						printf("I combinator: "); print_graph(root, 0, TOPNODE(stack)->sn);
					}
					if (STACK_SIZE(stack) > 2)
					{
						T {printf("I reduction, before: "); print_graph(root, TOPNODE(stack)->sn, TOPNODE(stack)->sn);}
						SS;
						*(PARENTNODE(stack, 2)->updateable)
							= PARENTNODE(stack, 1)->right;
						++PARENTNODE(stack, 1)->right->refcnt;
						PARENTNODE(stack, 2)->examined ^= PARENTNODE(stack, 2)->branch_marker;
						PARENTNODE(stack, 1)->examined = 0;
						free_node(PARENTNODE(stack, 1));
						POP(stack, 2);
						T{printf("I reduction, after (%d): ", STACK_SIZE(stack)); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_K:
					D printf("K combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 3)
					{
						T {printf("K reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						SS;
						*(PARENTNODE(stack, 3)->updateable) = PARENTNODE(stack, 1)->right;
						++PARENTNODE(stack, 1)->right->refcnt;
						PARENTNODE(stack, 1)->examined ^= LEFT;
						PARENTNODE(stack, 2)->examined ^= LEFT;
						PARENTNODE(stack, 3)->examined ^= PARENTNODE(stack, 3)->branch_marker;
						free_node(PARENTNODE(stack, 2));
						POP(stack, 3);
						T {printf("K reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_S:
					D printf("S combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 4)
					{
						struct node *n3 = PARENTNODE(stack, 3);
						struct node *ltmp = n3->left;
						struct node *rtmp = n3->right;
						T {printf("S reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0); }
						SS;
						n3->left = new_application(
								PARENTNODE(stack, 1)->right,
								n3->right
							);
						++n3->left->refcnt;
						n3->right = new_application(
								PARENTNODE(stack, 2)->right,
								n3->right
							);
						++n3->right->refcnt;
						PARENTNODE(stack, 1)->examined = 0;
						PARENTNODE(stack, 2)->examined = 0;
						free_node(ltmp);
						free_node(rtmp);
						n3->examined = 0;
						POP(stack, 3);
						T {printf("S reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_B:
					D {printf("B combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));}
					if (STACK_SIZE(stack) > 4)
					{
						struct node *ltmp = PARENTNODE(stack, 3)->left;
						struct node *rtmp = PARENTNODE(stack, 3)->right;
						T {printf("B reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						SS;
						PARENTNODE(stack, 3)->left
							= PARENTNODE(stack, 1)->right;
						++PARENTNODE(stack, 3)->left->refcnt;
						PARENTNODE(stack, 3)->right
							= new_application(
								PARENTNODE(stack, 2)->right,
								PARENTNODE(stack, 3)->right
							);
						++PARENTNODE(stack, 3)->right->refcnt;

						free_node(ltmp);
						free_node(rtmp);

						PARENTNODE(stack, 1)->examined = 0;
						PARENTNODE(stack, 2)->examined = 0;
						PARENTNODE(stack, 3)->examined = 0;

						POP(stack, 3);
						T {printf("B reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_C:
					D printf("C combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 4)
					{
						struct node *ltmp = PARENTNODE(stack, 3)->left;
						struct node *rtmp = PARENTNODE(stack, 3)->right;
						T {printf("C reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						SS;
						PARENTNODE(stack, 3)->left
							= new_application(
								PARENTNODE(stack, 1)->right,
								PARENTNODE(stack, 3)->right
							);
						++PARENTNODE(stack, 3)->left->refcnt;
						PARENTNODE(stack, 3)->right
							= PARENTNODE(stack, 2)->right;
						++PARENTNODE(stack, 3)->right->refcnt;

						free_node(ltmp);
						free_node(rtmp);

						PARENTNODE(stack, 1)->examined = 0;
						PARENTNODE(stack, 2)->examined = 0;
						PARENTNODE(stack, 3)->examined = 0;
						POP(stack, 3);
						T{printf("C reduction, after: "); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_W:
					D printf("W combinator %d, stack depth %d\n", TOPNODE(stack)->sn, STACK_SIZE(stack));
					if (STACK_SIZE(stack) > 3)
					{
						struct node *ltmp = PARENTNODE(stack, 2)->left;
						T{printf("W reduction, before: "); print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
						PARENTNODE(stack, 2)->left
							= new_application(
								PARENTNODE(stack, 1)->right,
								PARENTNODE(stack, 2)->right
							);
						++PARENTNODE(stack, 2)->left->refcnt;
						PARENTNODE(stack, 1)->examined = 0;
						PARENTNODE(stack, 2)->examined = 0;
						free_node(ltmp);
						POP(stack, 2);
						T{printf("W reduction, after: ");  print_graph(root, 0, TOPNODE(stack)->sn);}
						SS;
					} else
						POP(stack, 1);
					break;
				case COMB_NONE:
					T{printf("%s, no reduction: ", TOPNODE(stack)->name); print_graph(root, 0, TOPNODE(stack)->sn);}
					POP(stack, 1);
					D{printf("after pop: "); print_graph(root, 0, TOPNODE(stack)->sn);}
					break;
				}
				break;  /* end of case COMBINATOR */
			case UNTYPED:
				POP(stack, 1);
				break;
			}
		}

		pop_spine_stack(&stack);
		T printf("pop spine stack\n");

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

/* Control can longjmp() back to reduce_tree()
 * in grammar.y for certain input(s). */
int
read_line(void)
{
	char buf[64];
	*buf = 'A';
	do {
		printf("continue? ");
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		if (*buf == 'x' || *buf == 'e') exit(0);
		if (*buf == 'n' || *buf == 'q') longjmp(in_reduce_graph, 3);
		if (*buf == 'c') single_step = 0;
		if (*buf == '?')
		{
			fprintf(stderr,
				"e, x -> exit now\n"
				"n, q -> terminate current reduction, return to top level\n"
				"c -> continue current reduction without further stops\n"
			);
		}
	} while ('?' == *buf);
	return single_step;
}
