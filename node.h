enum nodeType { UNTYPED, UNALLOCATED, APPLICATION, COMBINATOR };

struct node {
	int sn;
	enum nodeType typ;
	int refcnt;
	const char *name;
	struct node *left;
	struct node *right;
};

struct node *new_application(struct node *left_child, struct node *right_child);
struct node *new_combinator(const char *name);

void print_tree(struct node *root);
void free_all_nodes(void);
