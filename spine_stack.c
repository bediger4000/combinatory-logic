#include <stdlib.h>
#include <stdio.h>

#include <spine_stack.h>

static struct spine_stack *spine_stack_free_list = NULL;

static int stack_malloc_cnt = 0;
static int stack_reused_cnt = 0;
static int new_stack_cnt;

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	++new_stack_cnt;

	if (spine_stack_free_list)
	{
		/* dual use of prev field: linked list of "free" spine stacks,
		 * and FIFO stack of actually in-use spine stacks. */
		r = spine_stack_free_list;
		spine_stack_free_list = r->prev;
		r->prev = NULL;
		++stack_reused_cnt;
	} else {
		r = malloc(sizeof(*r));
		r->stack = malloc(sz * sizeof(r->stack[0]));
		r->top   = 0;
		r->maxdepth = 0;
		r->size = sz;
		r->prev = NULL;
		++stack_malloc_cnt;
	}

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	/* dual use of prev field: linked list of "free" spine stacks,
	 * and FIFO stack of actually in-use spine stacks. */
	ss->prev = spine_stack_free_list;
	spine_stack_free_list = ss;
}

void
push_spine_stack(struct spine_stack **ss)
{
	/* dual use of prev field: linked list of "free" spine stacks,
	 * and FIFO stack of actually in-use spine stacks.
 	 * This constitutes the FIFO stack of in-use spine stacks usage. */
	struct spine_stack *p = new_spine_stack(*ss?(*ss)->size: 1024);
	p->prev = *ss;
	*ss = p;
}

void
pop_spine_stack(struct spine_stack **ss)
{
	/* FIFO-stack-of-in-use-stacks use of prev field */
	struct spine_stack *tmp = (*ss)->prev;
	delete_spine_stack(*ss);
	*ss = tmp;
}

void
free_all_spine_stacks(int memory_info_flag)
{
	int maxdepth = 0;
	int depth_sum = 0;
	int stack_freed_cnt = 0;
	while (spine_stack_free_list)
	{
		struct spine_stack *tmp = spine_stack_free_list->prev;
		if (spine_stack_free_list->maxdepth > maxdepth)
			maxdepth = spine_stack_free_list->maxdepth;
		depth_sum += spine_stack_free_list->maxdepth;
		free(spine_stack_free_list->stack);
		spine_stack_free_list->stack = NULL;
		free(spine_stack_free_list);
		spine_stack_free_list = tmp;
		++stack_freed_cnt;
	}

	if (stack_freed_cnt != stack_malloc_cnt)
		fprintf(stderr, "Allocated %d spine stacks, freed %d\n",
			stack_malloc_cnt, stack_freed_cnt);

	if (memory_info_flag)
	{
		float mean_depth = stack_freed_cnt? (float)depth_sum/(float)stack_freed_cnt: 0.0;
		fprintf(stderr, "Gave out %d spine stacks\n", new_stack_cnt);
		fprintf(stderr, "Allocated %d spine stacks\n", stack_malloc_cnt);
		fprintf(stderr, "Reused %d spine stacks\n", stack_reused_cnt);
		fprintf(stderr, "Maximum spine stack depth: %d\n", maxdepth);
		fprintf(stderr, "Mean spine stack depth: %.02f\n", mean_depth);
	}
}
