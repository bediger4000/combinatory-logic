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
	struct node *n1, *n2, *n3;
	int retry_n0 = 1;

	if (APPLICATION == n0->typ)
		reduce_graph(&(n0->left), pn0, pn1, pn2);

	n0 = *pn0;

	if (n0 && COMBINATOR == n0->typ)
	{
		const char *name = n0->name;
		n1 = *pn1;
		if (!strcmp(name, "I"))
		{
			if (n1)
			{
				struct node *tmp = n1->right;
{
struct node *x3 = *pn3, *x2 = *pn2, *x1 = *pn1;
printf("I reduction {%p}, before:\n", n0);
print_tree(x3? x3: x2? x2: x1);
putc('\n', stdout);
printf("n1 at %p, n1->left %p, n1->right %p, n1->name \"%s\", refcnt %d\n",
	n1, n1->left, n1->right, n1->name, n1->refcnt);
}
				n1->left = tmp->left;
				n1->right = tmp->right;
				n1->name = tmp->name;
				n1->typ = tmp->typ;
				/* can't copy refcnt: reference goes to *node*, not contents */

printf("n0 at %p, n0->left %p, n0->right %p, n0->name \"%s\", refcnt %d\n",
	n0, n0->left, n0->right, n0->name, n0->refcnt);
				free_node(n0);
printf("n0 at %p, n0->left %p, n0->right %p, n0->name \"%s\", refcnt %d\n",
	n0, n0->left, n0->right, n0->name? n0->name: "NULL", n0->refcnt);
printf("tmp at %p, tmp->left %p, tmp->right %p, tmp->name \"%s\", refcnt %d\n",
	tmp, tmp->left, tmp->right, tmp->name? tmp->name: "NULL", tmp->refcnt);
				free_node(tmp);
printf("tmp at %p, tmp->left %p, tmp->right %p, tmp->name \"%s\", refcnt %d\n",
	tmp, tmp->left, tmp->right, tmp->name? tmp->name: "NULL", tmp->refcnt);
				retry_n0 = 0;
{
struct node *x3 = *pn3, *x2 = *pn2, *x1 = *pn1;
printf("I reduction, after:\n");
print_tree(x3? x3: x2? x2: x1);
putc('\n', stdout);
}
			}
		} else if (!strcmp(name, "K")) {
			n2 = *pn2;
			if (n1 && n2)
			{
				struct node *tmp = n1->right;
printf("K reduction, before:\n");
print_tree(n3? n3: n2);
putc('\n', stdout);

				n2->left  = tmp->left;
				n2->right = tmp->right;
				n2->name  = tmp->name;
				n2->typ   = tmp->typ;
				/* can't copy refcnt: reference goes to *node*, not contents */

				free_node(n1);
				retry_n0 = 0;
printf("K reduction, after:\n");
print_tree(n3? n3: n2);
putc('\n', stdout);
			}

		} else if (!strcmp(name, "S")) {
			n2 = *pn2;
			n3 = *pn3;
			if (n1 && n2 && n3)
			{
printf("S reduction, before:\n");
print_tree(n3);
putc('\n', stdout);
				free_node(n0);
				n1->left = n1->right;
				n1->right = n3->right;
				n2->left = n2->right;
				n2->right = n3->right;
				++n3->right->refcnt;
				n3->left = n1;
				n3->right = n2;
				retry_n0 = 0;
printf("S reduction, After:\n");
print_tree(n3);
putc('\n', stdout);
			}
		} else if (!strcmp(name, "B")) {
			n2 = *pn2;
			n3 = *pn3;
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
			n2 = *pn2;
			n3 = *pn3;
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


	if (retry_n0 && n0)
	{
		n0 = *pn0;
		if (APPLICATION == n0->typ && COMBINATOR != n0->right->typ)
			reduce_graph(&(n0->right), pn0, pn1, pn2);
	}
}
