#include <stdio.h>

#include <node.h>
#include <graph.h>

extern int debug_reduction;

void print_graph(struct node *node)
{
	print_tree(node);
	putc('\n', stdout);
}

/* reduce_graph() can change
 * n0, n1, n2 and n3's type and children.
 * It returns an int:
 *  0: n0, the node currently in-work, didn't change
 * -1: n0 not affected, but rather needs some work.
 *  N: N > 0.  n0 for this ply, and N - 1 plys "up",
 *  got invalidated (removed from tree or something),
 *  and the program has to back up the stack N plys to
 *  get to a stack frame where n0 has a value that it
 *  can work from.
 */
int
reduce_graph(
	struct node *n0,
	struct node *n1,  /* parent of n0 */
	struct node *n2,  /* parent of n1 */
	struct node *n3,  /* parent of n2 */
	int ply
)
{
	int uplevels_affected = -1;
	int looping = 1;

	if (debug_reduction)
		printf("enter reduce_graph(%d, %d, %d, %d, %d)\n",
			n0? n0->sn: 0,
			n1? n1->sn: 0,
			n2? n2->sn: 0,
			n3? n3->sn: 0,
			ply
		);
	

	while (looping && APPLICATION == n0->typ)
	{
		int affected;
		if (debug_reduction)
			printf("ply %d, current node {%d} has appliction type, reducing left child {%d}\n",
				ply, n0->sn, n0->left->sn);
		affected = reduce_graph(n0->left, n0, n1, n2, ply+1);

		if (affected < 0)
			looping = 0;

		if (affected > 0)
		{
			if (debug_reduction)
				printf("leave reduce_graph(%d, %d, %d, %d, %d), return %d\n",
					n0? n0->sn: 0,
					n1? n1->sn: 0,
					n2? n2->sn: 0,
					n3? n3->sn: 0,
					ply, affected - 1
				);
			return affected - 1;
		}
	}

	if (COMBINATOR == n0->typ)
	{
		const char *name = n0->name;

		if (debug_reduction)
			printf("ply %d, current node {%d} has combinator type \"%s\", performing operation\n",
				ply, n0->sn, name? name: "null");

		if (!strcmp(name, "I"))
		{
			if (n1)
			{
				struct node *tmp = n1->right;

				if (debug_reduction)
				{
					printf("I reduction {%d}, before:\n", n0->sn);
					print_tree(n3? n3: n2? n2: n1);
					putc('\n', stdout);
					printf("n1 {%d}, n1->left {%d}, n1->right {%d}, n1->name \"%s\"\n",
						n1->sn, n1->left? n1->left->sn: 0, n1->right? n1->right->sn: 0, n1->name? n1->name: "null");
				}

				n1->left = tmp->left;
				n1->right = tmp->right;
				n1->name = tmp->name;
				n1->typ = tmp->typ;

				uplevels_affected = 1;

				if (debug_reduction)
				{
					printf("I reduction, after:\n");
					print_tree(n3? n3: n2? n2: n1);
					putc('\n', stdout);
				}
			}
		} else if (!strcmp(name, "K")) {
			if (n1 && n2)
			{
				struct node *tmp = n1->right;

				if (debug_reduction)
				{
					printf("K reduction, before:\n");
					print_tree(n3? n3: n2);
					putc('\n', stdout);
				}

				n2->left  = tmp->left;
				n2->right = tmp->right;
				n2->name  = tmp->name;
				n2->typ   = tmp->typ;

				uplevels_affected = 2;

				if (debug_reduction)
				{
					printf("K reduction, after:\n");
					print_tree(n3? n3: n2);
					putc('\n', stdout);
				}

			}

		} else if (!strcmp(name, "S")) {
			if (n1 && n2 && n3)
			{
				if (debug_reduction)
				{
					printf("S reduction {%d}, before:\n", n0->sn);
					print_tree(n3);
					putc('\n', stdout);
				}

				n1->left = n1->right;
				n1->right = n3->right;
				n2->left = n2->right;
				n2->right = n3->right;
				n3->left = n1;
				n3->right = n2;
				uplevels_affected = 3;

				if (debug_reduction)
				{
					printf("S reduction, After:\n");
					print_tree(n3);
					putc('\n', stdout);
				}
			}
		} else if (!strcmp(name, "B")) {
			if (n1 && n2 && n3)
			{
				if (debug_reduction)
				{
					printf("B reduction {%d}, before:\n", n3->sn);
					print_tree(n3);
					putc('\n', stdout);
				}

				n3->left = n1->right;
				n2->left = n2->right;
				n2->right = n3->right;
				n3->right = n2;
				uplevels_affected = 3;

				if (debug_reduction)
				{
					printf("B reduction {%d}, after:\n", n3->sn);
					print_tree(n3);
					putc('\n', stdout);
				}
			}
		} else if (!strcmp(name, "C")) {
			if (n1 && n2 && n3)
			{
				struct node *tmp = n2->right;
				n2->left = n1->right;
				n2->right = n3->right;
				n3->right = tmp;
				uplevels_affected = 3;
			} 
		}
	} else
		if (debug_reduction)
			printf("ply %d, current node {%d} does not have combinator type\n",
				ply, n0->sn);

	looping = 1;

	while (looping && APPLICATION == n0->typ && COMBINATOR != n0->right->typ)
	{
		int affected;
		if (debug_reduction)
			printf("ply %d, current node {%d} has appliction type, reducing right child {%d}\n",
				ply, n0->sn, n0->right->sn);
		affected = reduce_graph(n0->right, n0, n1, n2, ply+1);

		if (affected < 0)
			looping = 0;
		else if (affected > 0)
			uplevels_affected = affected - 1;
	}
		 
	if (debug_reduction)
		printf("leave reduce_graph(%d, %d, %d, %d, %d), return %d\n",
			n0? n0->sn: 0,
			n1? n1->sn: 0,
			n2? n2->sn: 0,
			n3? n3->sn: 0,
			ply,
			uplevels_affected
		);

	return uplevels_affected;
}
