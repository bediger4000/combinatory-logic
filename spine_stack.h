struct spine_stack {
	struct node **stack;
	int top;
	int size;
	struct spine_stack *prev;
};

struct spine_stack *new_spine_stack(int sz);
void delete_spine_stack(struct spine_stack *ss);
void push_spine_stack(struct spine_stack **ss);
void pop_spine_stack(struct spine_stack **ss);

#define TOPNODE(ss) ((ss)->stack[(ss)->top - 1])
#define PARENTNODE(ss, N) ((ss)->stack[((ss)->top)-1-N])
#define PUSHNODE(ss,n)  (ss)->stack[((ss)->top)++]=n
#define POP(ss, n)  (((ss)->top)-=n)
#define STACK_SIZE(ss)  ((ss)->top)
#define STACK_NOT_EMPTY(ss) ((ss)->top > 0)
