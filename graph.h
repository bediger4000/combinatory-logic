void reduce_graph(struct node *graph_root);

/* malloc/free based whole-graph copy and delete */
struct node *copy_graph(struct node *root);
void         free_graph(struct node *root);

void         print_graph(struct node *node, int node_sn_reducing, int current_node_sn);
