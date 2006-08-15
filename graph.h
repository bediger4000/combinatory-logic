int reduce_graph(
	struct node *n0,
	struct node *n1,
	struct node *n2,
	struct node *n3,
	int ply
);

/* malloc/free based whole-graph copy and delete */
struct node *copy_graph(struct node *root);
void         free_graph(struct node *root);

void         print_graph(struct node *node);
