#include <unistd.h>  /* getpagesize() */
#include <stdlib.h>  /* malloc(), free() */
#include <assert.h>  /* malloc(), free() */

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

union combo {
	char c;
	short s;
	int i;
	long l;
	long long ll;
	void *vp;
	char *cp;
	short *sp;
	int  *ip;
	long *lp;
};

struct memory_arena {
	char *first_allocation;
	char *next_allocation;
	int sz;                     /* max free size */
	int remaining;              /* how many bytes remain in allocation */
	struct memory_arena *next;
};

static int pagesize = 0;
static int headersize = 0;

/* Public way to get a new struct memory_arena
 * The first struct memory_arena in a chain constitutes a "dummy",
 * an empty head element of a FIFO (stack) of structs memory_arena.
 * The first allocation demanded from a memory arena will cause
 * the malloc() of one or more pages, and a 2nd element in the stack.
 */
struct memory_arena *
new_arena(void)
{
	struct memory_arena *ra = NULL;

	if (!pagesize)
	{
		pagesize = getpagesize();
		headersize = roundup(sizeof(struct memory_arena), sizeof(union combo));
	}

	ra = malloc(sizeof(*ra));

	ra->sz = 0;
	ra->remaining = 0;
	ra->first_allocation = ra->next_allocation = NULL;
	ra->next = NULL;

	return ra;
}

void
deallocate_arena(struct memory_arena *ma)
{
	while (ma)
	{
		struct memory_arena *tmp = ma->next;

		ma->next = NULL;
		free(ma);

		ma = tmp;
	}
}

void
free_arena_contents(struct memory_arena *ma)
{
	while (ma)
	{
		ma->remaining = ma->sz;
		ma->next_allocation = ma->first_allocation;
		ma = ma->next;
	}
}

void *
arena_alloc(struct memory_arena *ma, size_t size)
{
	void *r = NULL;
	struct memory_arena *ca;
	size_t nsize;

	assert(NULL != ma);

	/* What you actually have to allocate to get to
	 * a block "suitably aligned" for any use. */
	nsize = roundup(size, sizeof(union combo));

	ca = ma->next;

	while (ca && nsize > ca->remaining)
		ca = ca->next;

	if (NULL == ca)
	{
		/* create a new struct memory_arena of at least 1 page */
		
		size_t arena_size = roundup((nsize + headersize), pagesize);
		ca = malloc(arena_size);
		ca->first_allocation = ((char *)ca) + headersize;
		ca->next_allocation = ca->first_allocation;
		ca->sz = arena_size - headersize;
		ca->remaining = ca->sz;

		ca->next = ma->next;
		ma->next = ca;
	}

	r = ca->next_allocation;
	ca->next_allocation += nsize; /* next_allocation stays aligned */
	ca->remaining -= nsize;

	return r;
}

