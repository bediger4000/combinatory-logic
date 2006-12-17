struct spine_stack {
	struct node **stack;
	int top;
	int maxdepth;
	int size;
	int sn;
	struct spine_stack *prev;
};

struct spine_stack *new_spine_stack(int sz);
void pushnode(struct spine_stack *ss, struct node *n);
void delete_spine_stack(struct spine_stack *ss);
void push_spine_stack(struct spine_stack **ss);
void pop_spine_stack(struct spine_stack **ss);
void free_all_spine_stacks(int memory_info_flag);

#define TOPNODE(ss) ((ss)->stack[(ss)->top - 1])
#define PARENTNODE(ss, N) ((ss)->stack[((ss)->top)-1-N])
/* #define PUSHNODE(ss,node)  ((ss)->stack[((ss)->top)] = node); ++(ss)->top; assert(ss->top < ss->size) */
#define PUSHNODE(ss, node) pushnode(ss, node)
#define POP(ss, N)  (((ss)->top)-=N)
#define STACK_SIZE(ss)  ((ss)->top)
#define STACK_NOT_EMPTY(ss) ((ss)->top > 0)
