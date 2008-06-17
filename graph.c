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
#include <string.h>
#include <setjmp.h>   /* longjmp(), jmp_buf */

#include <node.h>
#include <graph.h>
#include <buffer.h>
#include <spine_stack.h>
#include <cycle_detector.h>

int read_line(void);

extern int cycle_detection;
extern int trace_reduction;
extern int debug_reduction;
extern int elaborate_output;
extern int single_step;

extern int max_reduction_count;

extern sigjmp_buf in_reduce_graph;

#define C if(cycle_detection)
#define D if(debug_reduction)
#define T if(trace_reduction)
#define NT if(debug_reduction && !trace_reduction)

/* can't do single_step && read_line() - compilers optimize it away */
#define SS if (single_step) read_line()


void canonicalize(struct node *node, struct buffer *b);

void print_graph(struct node *node, int sn_to_reduce, int current_sn)
{
	print_tree(node, sn_to_reduce, current_sn);
	putc('\n', stdout);
}

char *canonicalize_graph(struct node *node)
{
	struct buffer *b;
	char *s;

	b = new_buffer(256);

	canonicalize(node, b);

	s = b->buffer;
	s[b->offset] = '\0';
	b->buffer = NULL;

	delete_buffer(b);

	return s;
}

void canonicalize(struct node *node, struct buffer *b)
{
	switch (node->typ)
	{
	case APPLICATION:
		buffer_append(b, ".", 1);
		canonicalize(node->left, b);
		if (node->right->typ == COMBINATOR)
			buffer_append(b, " ", 1);
		canonicalize(node->right, b);
		break;
	case COMBINATOR:
		buffer_append(b, node->name, strlen(node->name));
		break;
	case UNTYPED:
	default:
		break;
	}
}


/* Graph reduction function. Destructively modifies the graph passed in.
 */
void
reduce_graph(struct node *root)
{
	struct spine_stack *stack = NULL;
	unsigned long reduction_counter = 0;

	push_spine_stack(&stack);

	PUSHNODE(stack, root);

	do {

		while (STACK_NOT_EMPTY(stack))
		{

			int pop_stack_cnt = 1;
			int performed_reduction = 0;
			enum combinatorName cn = TOPNODE(stack)->cn;

			switch (TOPNODE(stack)->typ)
			{
			case APPLICATION:
				if (!LEFT_BRANCH_TRAVERSED(TOPNODE(stack)))
				{
					TOPNODE(stack)->updateable = &(TOPNODE(stack)->left);
					TOPNODE(stack)->branch_marker = LEFT;
					MARK_LEFT_BRANCH_TRAVERSED(TOPNODE(stack));
					PUSHNODE(stack, TOPNODE(stack)->left);
					D printf("push left branch on current stack\n");
					pop_stack_cnt = 0;
				} else if (!RIGHT_BRANCH_TRAVERSED(TOPNODE(stack))) {
					struct node *tmp = TOPNODE(stack);
					MARK_RIGHT_BRANCH_TRAVERSED(TOPNODE(stack));
					D printf("push right branch on new stack\n");
					TOPNODE(stack)->updateable = &(TOPNODE(stack)->right);
					TOPNODE(stack)->branch_marker = RIGHT;
					push_spine_stack(&stack);
					PUSHNODE(stack, tmp);  /* "dummy" node at top of stack */
					PUSHNODE(stack, tmp->right);
					pop_stack_cnt = 0;
				}
				break;
			case COMBINATOR:
				/* node->typ indicates a combinator, which can comprise a built-in,
				 * or it can comprise a mere variable. Let node->cn decide. */
				if (stack->top > stack->maxdepth) stack->maxdepth = stack->top;
				D printf("%s combinator %d, stack depth %d\n",
					(cn == COMB_I? "I":
					(cn == COMB_K? "K":
					(cn == COMB_S? "S":
					(cn == COMB_B? "B":
					(cn == COMB_W? "W":
					(cn == COMB_C? "C":
					(cn == COMB_M? "M":
					(cn == COMB_T? "T": TOPNODE(stack)->name)))))))),
					TOPNODE(stack)->sn, STACK_SIZE(stack));
				switch (TOPNODE(stack)->cn)
				{
				case COMB_I:
					if (STACK_SIZE(stack) > 2)
					{
						D {printf("I reduction, before: "); print_graph(root, TOPNODE(stack)->sn, TOPNODE(stack)->sn);}
						NT SS;
						*(PARENTNODE(stack, 2)->updateable)
							= PARENTNODE(stack, 1)->right;
						++PARENTNODE(stack, 1)->right->refcnt;
						PARENTNODE(stack, 2)->examined ^= PARENTNODE(stack, 2)->branch_marker;
						PARENTNODE(stack, 1)->examined = 0;
						free_node(PARENTNODE(stack, 1));
						performed_reduction = 1;
						pop_stack_cnt = 2;
					}
					break;
				case COMB_K:
					if (STACK_SIZE(stack) > 3)
					{
						D {printf("K reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						NT SS;
						*(PARENTNODE(stack, 3)->updateable) = PARENTNODE(stack, 1)->right;
						++PARENTNODE(stack, 1)->right->refcnt;
						PARENTNODE(stack, 1)->examined ^= LEFT;
						PARENTNODE(stack, 2)->examined ^= LEFT;
						PARENTNODE(stack, 3)->examined ^= PARENTNODE(stack, 3)->branch_marker;
						free_node(PARENTNODE(stack, 2));
						performed_reduction = 1;
						pop_stack_cnt = 3;
					}
					break;
				case COMB_T:
					/* T x y -> y x */
					if (STACK_SIZE(stack) > 3)
					{
						struct node *n;
						struct node *tmp = *(PARENTNODE(stack, 3)->updateable);
						D {printf("T reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						NT SS;
						n = new_application(
								PARENTNODE(stack, 2)->right,
								PARENTNODE(stack, 1)->right
							);
						PARENTNODE(stack, 1)->examined ^= LEFT;
						PARENTNODE(stack, 2)->examined ^= LEFT;
						PARENTNODE(stack, 3)->examined ^= PARENTNODE(stack, 3)->branch_marker;
						*(PARENTNODE(stack, 3)->updateable) = n;
						++n->refcnt;
						free_node(tmp);
						performed_reduction = 1;
						pop_stack_cnt = 3;
					}
					break;
				case COMB_M:
					/* M x  -> x x */
					if (STACK_SIZE(stack) > 2)
					{
						struct node *n;
						struct node *tmp = *(PARENTNODE(stack, 2)->updateable);
						D {printf("M reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						NT SS;
						n = new_application(
								PARENTNODE(stack, 1)->right,
								PARENTNODE(stack, 1)->right
							);
						PARENTNODE(stack, 1)->examined ^= LEFT;
						PARENTNODE(stack, 2)->examined ^= PARENTNODE(stack, 2)->branch_marker;
						*(PARENTNODE(stack, 2)->updateable) = n;
						++n->refcnt;
						free_node(tmp);
						performed_reduction = 1;
						pop_stack_cnt = 2;
					}
					break;
				case COMB_S:
					if (STACK_SIZE(stack) > 4)
					{
						struct node *n3 = PARENTNODE(stack, 3);
						struct node *ltmp = n3->left;
						struct node *rtmp = n3->right;
						D {printf("S reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0); }
						NT SS;
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
						performed_reduction = 1;
						pop_stack_cnt = 3;
					}
					break;
				case COMB_B:
					if (STACK_SIZE(stack) > 4)
					{
						struct node *ltmp = PARENTNODE(stack, 3)->left;
						struct node *rtmp = PARENTNODE(stack, 3)->right;
						D {printf("B reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						NT SS;
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

						performed_reduction = 1;
						pop_stack_cnt = 3;
					}
					break;
				case COMB_C:
					if (STACK_SIZE(stack) > 4)
					{
						struct node *ltmp = PARENTNODE(stack, 3)->left;
						struct node *rtmp = PARENTNODE(stack, 3)->right;
						D {printf("C reduction, before: "); print_graph(root, TOPNODE(stack)->sn, 0);}
						NT SS;
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
						performed_reduction = 1;
						pop_stack_cnt = 3;
					}
					break;
				case COMB_W:
					if (STACK_SIZE(stack) > 3)
					{
						struct node *ltmp = PARENTNODE(stack, 2)->left;
						D{printf("W reduction, before: "); print_graph(root, TOPNODE(stack)->sn, TOPNODE(stack)->sn);}
						NT SS;
						PARENTNODE(stack, 2)->left
							= new_application(
								PARENTNODE(stack, 1)->right,
								PARENTNODE(stack, 2)->right
							);
						++PARENTNODE(stack, 2)->left->refcnt;
						PARENTNODE(stack, 1)->examined = 0;
						PARENTNODE(stack, 2)->examined = 0;
						free_node(ltmp);
						performed_reduction = 1;
						pop_stack_cnt = 2;
					}
					break;
				case COMB_NONE:  /* A combinator that's not a built-in */
					D{printf("%s, no reduction: ", TOPNODE(stack)->name); print_graph(root, 0, TOPNODE(stack)->sn);}
					break;
				}
				if (performed_reduction) SS;
				break;  /* end of case COMBINATOR, switch on node->cn */
			case UNTYPED:
				break;
			}

			POP(stack, pop_stack_cnt);

			if (performed_reduction)
			{
				++reduction_counter;

				D {
					printf("%s reduction, after: ",
						(cn == COMB_I? "I":
						(cn == COMB_K? "K":
						(cn == COMB_S? "S":
						(cn == COMB_B? "B":
						(cn == COMB_W? "W":
						(cn == COMB_C? "C":
						(cn == COMB_M? "M":
						(cn == COMB_T? "T": TOPNODE(stack)->name))))))))
					);
					print_graph(root, 0, TOPNODE(stack)->sn);
				}

				T print_graph(root, 0, 0);

				if (cycle_detection && cycle_detector(root))
				{
					while (stack) pop_spine_stack(&stack);
					reset_detection();
					siglongjmp(in_reduce_graph, 5);
				}

				if (max_reduction_count > 0
					&& reduction_counter > max_reduction_count)
				{
					/* The 4 means "too many reductions" */
					while (stack) pop_spine_stack(&stack);
					C reset_detection();
					siglongjmp(in_reduce_graph, 4);
				}
			}
		}

		pop_spine_stack(&stack);
		D printf("pop spine stack\n");

	} while (stack);

	C reset_detection();
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
		switch (*buf)
		{
		case 'x': case 'e':
			exit(0);
			break;
		case 'n': case 'q':
			C reset_detection();
			siglongjmp(in_reduce_graph, 3);
			break;
		case 'c':
			single_step = 0;
			break;
		case '?':
			fprintf(stderr,
				"e, x -> exit now\n"
				"n, q -> terminate current reduction, return to top level\n"
				"c -> continue current reduction without further stops\n"
			);
			break;
		default:
			break;
		}
	} while ('?' == *buf);
	return single_step;
}
