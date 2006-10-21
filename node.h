enum nodeType { UNTYPED, APPLICATION, COMBINATOR };
enum combinatorName { COMB_NONE, COMB_S, COMB_K, COMB_I, COMB_C, COMB_B };

struct node {
	int sn;
	enum nodeType typ;
	enum combinatorName cn;
	const char *name;
	struct node *left;
	struct node *right;
	int examined;
};

#define LEFT_BRANCH_TRAVERSED(n) ((n)->examined & 0x01)
#define MARK_LEFT_BRANCH_TRAVERSED(n) ((n)->examined |= 0x01)
#define RIGHT_BRANCH_TRAVERSED(n) ((n)->examined & 0x02)
#define MARK_RIGHT_BRANCH_TRAVERSED(n) ((n)->examined |= 0x02)

struct node *new_application(struct node *left_child, struct node *right_child);
struct node *new_combinator(enum combinatorName cn);
struct node *new_term(const char *name);

void init_node_allocation(void);
void reset_node_allocation(void);
void print_tree(struct node *root, int reduction_node_sn, int current_node_sn);
void free_all_nodes(void);

struct node * arena_copy_graph(struct node *root);

void copy_node_attrs(struct node *to, struct node *from);
