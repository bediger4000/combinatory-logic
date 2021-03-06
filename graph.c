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


#include <stdio.h>
#include <stdlib.h>  /* malloc() and free() */
#include <assert.h>
#include <string.h>
#include <setjmp.h>   /* longjmp(), jmp_buf */

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <spine_stack.h>
#include <cycle_detector.h>
#include <aho_corasick.h>
#include <algorithm_d.h>

int read_line(void);

extern int multiple_reduction_detection;
extern int cycle_detection;
extern int trace_reduction;
extern int debug_reduction;
extern int single_step;
extern int stop_on_match;

extern int max_reduction_count;

extern sigjmp_buf in_reduce_graph;

extern struct gto *match_expr;

#define C if(cycle_detection)
#define D if(debug_reduction)
#define T if(trace_reduction)
#define NT if(debug_reduction && !trace_reduction)

/* can't do single_step && read_line() - compilers optimize it away */
#define SS if (single_step) read_line()


void canonicalize(struct node *node, struct buffer *b);

void
print_graph(struct node *node)
{
	print_tree(node);
	putc('\n', stdout);
}

char *
canonicalize_graph(struct node *node)
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

void
canonicalize(struct node *node, struct buffer *b)
{
	switch (node->typ)
	{
	case APPLICATION:
		buffer_append(b, ".", 1);
		canonicalize(node->left, b);
		if (ATOM == node->right->typ)
			buffer_append(b, " ", 1);
		canonicalize(node->right, b);
		break;
	case ATOM:
		buffer_append(b, node->name, strlen(node->name));
		break;
	}
}


/* Graph reduction function. Destructively modifies the graph passed in.
 */
enum graphReductionResult
reduce_graph(struct node *root)
{
	struct spine_stack *stack = NULL;
	unsigned long reduction_counter = 0;
	int max_redex_count = 0;
	enum graphReductionResult r = UNKNOWN;
	enum Direction { DIR_LEFT, DIR_RIGHT, DIR_UP };
	enum Direction dir = DIR_LEFT;

	stack = new_spine_stack(1024);

	root->updateable = root->left_addr;
	pushnode(stack, root, 1);

	D print_graph(root);

	while (STACK_NOT_EMPTY(stack))
	{

		int pop_stack_cnt = 1;
		int performed_reduction = 0;
		enum primitiveName cn = TOPNODE(stack)->cn;
		struct node *topnode;

		switch (TOPNODE(stack)->typ)
		{
		case APPLICATION:
			topnode = TOPNODE(stack);
			switch (dir)
			{
			case DIR_LEFT:
				topnode->updateable = topnode->left_addr;
				pushnode(stack, topnode->left, 0);
				D printf("push left branch on stack, depth now %d\n", DEPTH(stack));
				pop_stack_cnt = 0;
				break;

			case DIR_RIGHT:
				topnode->updateable = topnode->right_addr;
				pushnode(stack, topnode->right, 2);
				D printf("push right branch on stack, depth now %d\n", DEPTH(stack));
				pop_stack_cnt = 0;
				break;

			case DIR_UP:
				break;
			}
			break;
		case ATOM:
			/* node->typ indicates a combinator, which can comprise a built-in,
			 * or it can comprise a mere variable. Let node->cn decide. */
			D {
				printf("%s reduction, stack depth %d, before: ",
					(cn == COMB_I? "I":
					(cn == COMB_J? "J":
					(cn == COMB_K? "K":
					(cn == COMB_S? "S":
					(cn == COMB_B? "B":
					(cn == COMB_W? "W":
					(cn == COMB_C? "C":
					(cn == COMB_M? "M":
					(cn == COMB_T? "T": TOPNODE(stack)->name))))))))),
				 	DEPTH(stack));
				print_graph(root->left);
			}
			switch (TOPNODE(stack)->cn)
			{
			case COMB_I:
				if (DEPTH(stack) > 2)
				{
					NT SS;
					*(PARENTNODE(stack, 2)->updateable)
						= PARENTNODE(stack, 1)->right;
					++PARENTNODE(stack, 1)->right->refcnt;
					free_node(PARENTNODE(stack, 1));
					performed_reduction = 1;
					pop_stack_cnt = 2;
				}
				break;
			case COMB_J:
				if (DEPTH(stack) > 5)
				{
					struct node *n4 = PARENTNODE(stack, 4);
					struct node *ltmp = n4->left;
					struct node *rtmp = n4->right;

					NT SS;

					n4->left = new_application(
							PARENTNODE(stack, 1)->right,
							PARENTNODE(stack, 2)->right
						);
					++n4->left->refcnt;

					n4->right = new_application(
							new_application(
								PARENTNODE(stack, 1)->right,
								n4->right
							),
							PARENTNODE(stack, 3)->right
						);
					++n4->right->refcnt;

					free_node(ltmp);
					free_node(rtmp);
					performed_reduction = 1;
					pop_stack_cnt = 4;
				}
				break;
			case COMB_K:
				if (DEPTH(stack) > 3)
				{
					NT SS;
					*(PARENTNODE(stack, 3)->updateable) = PARENTNODE(stack, 1)->right;
					++PARENTNODE(stack, 1)->right->refcnt;
					free_node(PARENTNODE(stack, 2));
					performed_reduction = 1;
					pop_stack_cnt = 3;
				}
				break;
			case COMB_T:
				/* T x y -> y x */
				if (DEPTH(stack) > 3)
				{
					struct node *n;
					struct node *tmp = *(PARENTNODE(stack, 3)->updateable);
					NT SS;
					n = new_application(
							PARENTNODE(stack, 2)->right,
							PARENTNODE(stack, 1)->right
						);
					*(PARENTNODE(stack, 3)->updateable) = n;
					++n->refcnt;
					free_node(tmp);
					performed_reduction = 1;
					pop_stack_cnt = 3;
				}
				break;
			case COMB_M:
				/* M x  -> x x */
				if (DEPTH(stack) > 2)
				{
					struct node *n1 = PARENTNODE(stack, 1);
					NT SS;

					free_node(n1->left);

					n1->left = n1->right;
					++n1->left->refcnt;

					performed_reduction = 1;
					pop_stack_cnt = 2;
				}
				break;
			case COMB_S:
				if (DEPTH(stack) > 4)
				{
					struct node *n4 = PARENTNODE(stack, 4);
					struct node *tmp, *f = *(n4->updateable);
					NT SS;

					tmp = new_application(
						new_application(
							PARENTNODE(stack, 1)->right,
							PARENTNODE(stack, 3)->right
						),
						new_application(
							PARENTNODE(stack, 2)->right,
							PARENTNODE(stack, 3)->right
						)
					);
					*(n4->updateable) = tmp;
					++tmp->refcnt;

					free_node(f);
					performed_reduction = 1;
					pop_stack_cnt = 4;
				}
				break;
			case COMB_B:
				if (DEPTH(stack) > 4)
				{
					struct node *n3 =  PARENTNODE(stack, 3);
					struct node *ltmp = n3->left;
					struct node *rtmp = n3->right;
					NT SS;
					n3->left
						= PARENTNODE(stack, 1)->right;
					++n3->left->refcnt;
					n3->right
						= new_application(
							PARENTNODE(stack, 2)->right,
							n3->right
						);
					++n3->right->refcnt;

					free_node(ltmp);
					free_node(rtmp);

					performed_reduction = 1;
					pop_stack_cnt = 3;
				}
				break;
			case COMB_C:
				if (DEPTH(stack) > 4)
				{
					struct node *n3 =  PARENTNODE(stack, 3);
					struct node *ltmp = n3->left;
					struct node *rtmp = n3->right;
					NT SS;
					n3->left
						= new_application(
							PARENTNODE(stack, 1)->right,
							n3->right
						);
					++n3->left->refcnt;
					n3->right
						= PARENTNODE(stack, 2)->right;
					++n3->right->refcnt;

					free_node(ltmp);
					free_node(rtmp);

					performed_reduction = 1;
					pop_stack_cnt = 3;
				}
				break;
			case COMB_W:
				if (DEPTH(stack) > 3)
				{
					struct node *n2 = PARENTNODE(stack, 2);
					struct node *ltmp = n2->left;
					NT SS;
					n2->left
						= new_application(
							PARENTNODE(stack, 1)->right,
							n2->right
						);
					++n2->left->refcnt;
					free_node(ltmp);
					performed_reduction = 1;
					pop_stack_cnt = 2;
				}
				break;
			case COMB_NONE:  /* A combinator that's not a built-in */
				D{printf("%s, no reduction: ", TOPNODE(stack)->name); print_graph(root->left);}
				break;
			}
			if (performed_reduction) SS;
			break;  /* end of case ATOM, switch on node->cn */
		}

		POP(stack, pop_stack_cnt);
		D {
			printf("%sperformed reduction, popped %d, stack depth now %d: ",
				performed_reduction? "": "didn't ", pop_stack_cnt, DEPTH(stack)
			);
			print_graph(root);
			printf("direction %s\n", dir == DIR_LEFT? "left": dir == DIR_RIGHT? "right": "up");
		}

		/* Decide what to do next. Note that top-of-stack is the
		 * node we just popped to get to.
		 */
		topnode = TOPNODE(stack);

		if (performed_reduction)
		{
			if (topnode->updateable == topnode->left_addr)
				dir = DIR_LEFT;
			else
				dir = DIR_RIGHT;

		} else {
			if (pop_stack_cnt)
			{
				dir = DIR_UP;
				/* Ugly special case: popped up to root of tree */
				if (topnode == root)
					POP(stack, 1);
				else {
					if (topnode->updateable == topnode->left_addr)
						dir = DIR_RIGHT;
					/* if (topnode->updateable == topnode->right_addr)
					 * then leave dir value at DIR_UP: we've already
					 * traversed the left and right sides of the parse tree/
					 * reduction graph.  If updateable has neither left_addr
					 * nor right_addr values, your guess is as good as mine.
					 */
				}
			} else
				dir = DIR_LEFT;
		}

		D printf("direction now %s\n", dir == DIR_LEFT? "left": dir == DIR_RIGHT? "right": "up");

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
				print_graph(root->left);
			}

			if (multiple_reduction_detection)
			{
				if (trace_reduction)
				{
					struct buffer *b = new_buffer(256);
					int ignore, redex_count = reduction_count(root->left, 0, &ignore, b);  /* root: a dummy node */
					if (redex_count > max_redex_count) max_redex_count = redex_count;
					b->buffer[b->offset] = '\0';
					printf("[%d] %s\n", redex_count, b->buffer);
					delete_buffer(b);
				}
			} else
				T print_graph(root->left);

			if (cycle_detection && cycle_detector(root, max_redex_count))
			{
				r = CYCLE_DETECTED;
				goto exceptional_exit;
			}

			if (max_reduction_count > 0
				&& reduction_counter > max_reduction_count)
			{
				C reset_detection();
				r = REDUCTION_LIMIT;
				goto exceptional_exit;
			}

			if (stop_on_match)
			{
				if (algorithm_d(match_expr, root->left))
				{
					r = MATCHED_PATTERN;
					goto exceptional_exit;
				}
			}
		}
	}

	r = NORMAL_FORM;

	/* reaching reduction limit or finding a cycle or a match */
	exceptional_exit:

	delete_spine_stack(stack);

	C reset_detection();

	return r;
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

int
reduction_count(struct node *node, int stack_depth, int *child_redex, struct buffer *b)
{
	int reductions = 0;
	int left_child_redex = 0, right_child_redex = 0;
	int print_right_paren = 0;

	if (node)
	{
		switch (node->typ)
		{
		case APPLICATION:

			if (!node->left && !node->right) return 0;

			reductions += reduction_count(node->left, stack_depth + 1, &left_child_redex, b);
			if (left_child_redex) buffer_append(b, "*", 1);

			buffer_append(b, " ", 1);

			if (APPLICATION == node->right->typ)
			{
				buffer_append(b, "(", 1);
				print_right_paren = 1;
			}
			reductions += reduction_count(node->right, 0, &right_child_redex, b);
			if (right_child_redex) buffer_append(b, "*", 1);
			if (print_right_paren) buffer_append(b, ")", 1);

			break;
		case ATOM:
			buffer_append(b, node->name, strlen(node->name));
			switch (node->cn)
			{
			case COMB_I:
			case COMB_M:
				if (stack_depth > 0)
				{
					reductions = 1;
					*child_redex = 1;
				}
				break;
			case COMB_K:
			case COMB_W:
			case COMB_T:
				if (stack_depth > 1)
				{
					reductions = 1;
					*child_redex = 1;
				}
				break;
			case COMB_B:
			case COMB_C:
			case COMB_S:
				if (stack_depth > 2)
				{
					reductions = 1;
					*child_redex = 1;
				}
				break;
			case COMB_J:
				if (stack_depth > 3)
				{
					reductions = 1;
					*child_redex = 1;
				}
				break;
			case COMB_NONE:
				break;
			}
			break;
		}
	}

	return reductions;
}

/* when total_count evaluates to true (non-zero),
 * this counts interior and leaf nodes.  Otherwise,
 * it just counts leaf nodes.
 */
int
node_count(struct node *node, int count_interior_nodes)
{
	int count = 0;

	switch (node->typ)
	{
	case APPLICATION:
		if (count_interior_nodes) ++count;
		count += node_count(node->left, count_interior_nodes);
		count += node_count(node->right, count_interior_nodes);
		break;
	case ATOM:
		count = 1;
		break;
	}

	return count;
}
