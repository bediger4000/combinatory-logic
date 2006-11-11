#include <stdio.h>
#include <unistd.h>  /* getpagesize() */
#include <stdlib.h>  /* malloc(), free() */
#include <assert.h>  /* malloc(), free() */

#include <arena.h>

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
	float f;
	double d;
};

struct memory_arena {
	char *first_allocation;
	char *next_allocation;
	int sz;                     /* max free size */
	int remaining;              /* how many bytes remain in allocation */
	struct memory_arena *next;
	int usage_info;
};

static int pagesize = 0;
static int headersize = 0;
static int combosize = 0;

/* Public way to get a new struct memory_arena
 * The first struct memory_arena in a chain constitutes a "dummy",
 * an empty head element of a FIFO (stack) of structs memory_arena.
 * The first allocation demanded from a memory arena will cause
 * the malloc() of one or more pages, and a 2nd element in the stack.
 */
struct memory_arena *
new_arena(int memory_info_flag)
{
	struct memory_arena *ra = NULL;

	if (!pagesize)
	{
		combosize = sizeof(union combo);
		pagesize = getpagesize();
		headersize = roundup(sizeof(struct memory_arena), combosize);
	}

	ra = malloc(sizeof(*ra));

	ra->sz = 0;
	ra->remaining = 0;
	ra->first_allocation = ra->next_allocation = NULL;
	ra->next = NULL;
	ra->usage_info = memory_info_flag;

	return ra;
}

void
deallocate_arena(struct memory_arena *ma, int memory_info_flag)
{
	int free_arena_cnt = 0;
	int maxsz = 0;
	while (ma)
	{
		struct memory_arena *tmp = ma->next;

		if (ma->sz > maxsz) maxsz = ma->sz;

		ma->next = NULL;
		free(ma);

		ma = tmp;
		++free_arena_cnt;
	}

	if (memory_info_flag)
	{
		fprintf(stderr, "Page size %d (0x%x)\n", pagesize, pagesize);
		fprintf(stderr, "Header size %d (0x%x)\n", headersize, headersize);
		fprintf(stderr, "Allocated %d arenas\n", free_arena_cnt);
		fprintf(stderr, "Max arena size %d (0x%x)\n", maxsz, maxsz);
	}
}

void
free_arena_contents(struct memory_arena *ma)
{
	int memory_info_flag = ma->usage_info;
	ma = ma->next; /* dummy head node */
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
	nsize = roundup(size, combosize);

	ca = ma->next;

	while (ca && nsize > ca->remaining)
		ca = ca->next;

	if (NULL == ca)
	{

		/* You could do something like moving all arenas with
		 * less than combosize remaining into a 2nd list - one
		 * that never gets checked, as combosize is the minimum
		 * allocation possible. */

		/* create a new struct memory_arena of at least 1 page */
		size_t arena_size = roundup((nsize + headersize), pagesize);

		ca = malloc(arena_size);
		ca->first_allocation = ((char *)ca) + headersize;
		ca->next_allocation = ca->first_allocation;
		ca->sz = arena_size - headersize;
		ca->remaining = ca->sz;

		ca->usage_info = ma->usage_info;
		ca->next = ma->next;
		ma->next = ca;
	}

	r = ca->next_allocation;
	ca->next_allocation += nsize; /* next_allocation stays aligned */
	ca->remaining -= nsize;

	return r;
}

