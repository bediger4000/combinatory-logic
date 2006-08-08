#include <stdio.h>

#include <node.h>
#include <graph.h>

void print_graph(struct node *node)
{
	print_tree(node);
	putc('\n', stdout);
}

/* reduce_graph() can change n0's type and children,
 * n1's type and children.
 */
void
reduce_graph(
	struct node **pn0,
	struct node **pn1,  /* parent of n0 */
	struct node **pn2,  /* parent of n1 */
	struct node **pn3   /* parent of n2 */
)
{
	struct node *n0 = *pn0;
	struct node *n1 = *pn1;
	struct node *n2 = *pn2;
	struct node *n3 = *pn3;
	int retry_n0 = 1;

	if (APPLICATION == n0->typ)
		reduce_graph(&(n0->left), pn0, pn1, pn2);

	if (COMBINATOR == n0->typ)
	{
		const char *name = n0->name;
		if (!strcmp(name, "I"))
		{
			if (n1)
			{
				struct node *tmp = n1->right;
				n1->left = tmp->left;
				n1->right = tmp->right;
				n1->name = tmp->name;
				n1->typ = tmp->typ;
				n1->refcnt = tmp->refcnt;
				free_node(n0);
				tmp->right = tmp->left = NULL;
				free_node(tmp);
				retry_n0 = 0;
			}
		} else if (!strcmp(name, "K")) {
			if (n1 && n2)
			{
				struct node *tmp = n1->right;

				free_node(n2->right);

				n2->left = tmp->left;
				n2->right = tmp->right;
				n2->name = tmp->name;
				n2->typ = tmp->typ;

				tmp->left = tmp->right = NULL;
				free_node(n1);
				retry_n0 = 0;
			}

		} else if (!strcmp(name, "S")) {
			if (n1 && n2 && n3)
			{
				free_node(n0);
				n1->left = n1->right;
				n1->right = n3->right;
				n2->left = n2->right;
				n2->right = n3->right;
				++n3->right->refcnt;
				n3->left = n1;
				n3->right = n2;
				retry_n0 = 0;
			}
		} else if (!strcmp(name, "B")) {
			if (n1 && n2 && n3)
			{
				n3->left = n1->right;
				n2->left = n2->right;
				n2->right = n3->right;
				n3->right = n2;
				n1->right = NULL;
				free_node(n1);
				retry_n0 = 0;
			}
		} else if (!strcmp(name, "C")) {
			if (n1 && n2 && n3)
			{
				struct node *tmp = n2->right;
				n2->left = n1->right;
				n2->right = n3->right;
				n3->right = tmp;
				n1->right = NULL;
				free_node(n1);
				retry_n0 = 0;
			} 
		}
	} 

	if (retry_n0 && APPLICATION == n0->typ)
		reduce_graph(&(n0->right), pn0, pn1, pn2);
}
