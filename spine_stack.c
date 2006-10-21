#include <stdlib.h>

#include <spine_stack.h>

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	r = malloc(sizeof(*r));

	r->stack = malloc(sz * sizeof(r->stack[0]));
	r->top   = 0;
	r->size = sz;

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	free(ss->stack);
	ss->stack = NULL;
	free(ss);
}

void
push_spine_stack(struct spine_stack **ss)
{
	struct spine_stack *p = new_spine_stack(*ss?(*ss)->size: 256);
	p->prev = *ss;
	*ss = p;
}

void
pop_spine_stack(struct spine_stack **ss)
{
	struct spine_stack *tmp = (*ss)->prev;
	delete_spine_stack(*ss);
	*ss = tmp;
}
