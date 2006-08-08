#include <stdio.h>

#include <node.h>
#include <graph.h>

extern int debug_reduction;

void print_graph(struct node *node)
{
	print_tree(node);
	putc('\n', stdout);
}

/* reduce_graph() can change n0's type and children,
 * n1 and n2's type and children.
 */
void
reduce_graph(
	struct node *n0,
	struct node *n1,  /* parent of n0 */
	struct node *n2,  /* parent of n1 */
	struct node *n3   /* parent of n2 */
)
{
	int retry_n0 = 1;

	if (APPLICATION == n0->typ)
	{
		if (debug_reduction)
			printf("current node {%d} has appliction type, reducing left child {%d}\n",
				n0->sn, n0->left->sn);
		reduce_graph(n0->left, n0, n1, n2);
	}

	if (n0 && COMBINATOR == n0->typ)
	{
		if (debug_reduction)
			printf("current node {%d} has combinator type, performing operation\n",
				n0->sn);
		const char *name = n0->name;
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
					printf("n1 {%d}, n1->left {%d}, n1->right {%d}, n1->name \"%s\", refcnt %d\n",
						n1->sn, n1->left? n1->left->sn: 0, n1->right? n1->right->sn: 0, n1->name, n1->refcnt);
				}

				n1->left = tmp->left;
				n1->right = tmp->right;
				n1->name = tmp->name;
				n1->typ = tmp->typ;
				/* can't copy refcnt: reference goes to *node*, not contents */

				if (debug_reduction)
					printf("n0 {%d}, n0->left {%d}, n0->right {%d}, n0->name \"%s\", refcnt %d\n",
						n0->sn, n0->left? n0->left->sn: 0, n0->right? n0->right->sn: 0, n0->name, n0->refcnt);

				free_node(n0);

				if (debug_reduction)
				{
					printf("n0 {%d}, n0->left {%d}, n0->right {%d}, n0->name \"%s\", refcnt %d\n",
						n0->sn, n0->left? n0->left->sn: 0, n0->right? n0->right->sn: 0, n0->name, n0->refcnt);
					printf("tmp {%d}, tmp->left {%d}, tmp->right {%d}, tmp->name \"%s\", refcnt %d\n",
						tmp->sn, tmp->left? tmp->left->sn: 0, tmp->right? tmp->right->sn: 0, tmp->name, tmp->refcnt);
				}

				free_node(tmp);

				if (debug_reduction)
				printf("tmp {%d}, tmp->left {%d}, tmp->right {%d}, tmp->name \"%s\", refcnt %d\n",
					tmp->sn, tmp->left? tmp->left->sn: 0, tmp->right? tmp->right->sn: 0, tmp->name, tmp->refcnt);

				retry_n0 = 0;

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
				/* can't copy refcnt: reference goes to *node*, not contents */

				free_node(n1);
				retry_n0 = 0;

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

				free_node(n0);
				n1->left = n1->right;
				n1->right = n3->right;
				n2->left = n2->right;
				n2->right = n3->right;
				++n3->right->refcnt;
				n3->left = n1;
				n3->right = n2;
				retry_n0 = 0;

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
				free_node(n1);
				retry_n0 = 0;
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
				free_node(n1);
				retry_n0 = 0;
			} 
		}
	} 


	if (retry_n0 && n0)
	{
		if (APPLICATION == n0->typ && COMBINATOR != n0->right->typ)
		{
			if (debug_reduction)
				printf("current node {%d} has application type, right child {%d} not combinator, reducing right side\n",
					n0->sn, n0->right->sn);
			reduce_graph(n0->right, n0, n1, n2);
		}
	}
}
