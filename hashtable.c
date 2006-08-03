/*
	Copyright (C) 2006, Bruce Ediger

    This file is part of lc.

    lc is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    lc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lc; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hashtable.h>

unsigned int hash_djb2(const char *s);
void rehash_hashtable(struct hashtable *h);
void reallocate_buckets(struct hashtable *h);
int  sparsebit(long i);
int  msbbit(long i);

/* chain is the dummy node at head of hash chain */
void
insert_node(struct hashnode *chain, struct hashnode *n)
{
	n->next = chain->next;
	chain->next = n;
	n->next->prev = n;
	n->prev = chain;
}

struct hashtable *
init_hashtable(int buckets, int maxload)
{
	int i;
	int f, z;

	struct hashtable *h;

	h = malloc(sizeof(*h));

	h->flags = 0;

	/* Insure that bucket array size is always a multiple of 2.
	 * This amounted to about 10% run-time savings for small files
	 * by avoiding the '%' operator. */

	f = msbbit(buckets);
	z = 1 << f;

	if (z < buckets)
		z <<= 1;

	buckets = z;

	/* buckets and sentinels */
	h->buckets =
		(struct hashnode **)malloc(buckets*sizeof(*h->buckets));
	memset((char *)h->buckets, '\0', buckets*sizeof(*h->buckets));

	h->sentinels =
		(struct hashnode **)malloc(buckets*sizeof(*h->sentinels));
	memset((char *)h->sentinels, '\0', buckets*sizeof(*h->sentinels));

	for (i = 0; i < buckets; ++i)
	{
		/* first node is a dummy - for convenience and to hold chain stats */

		h->buckets[i] = (struct hashnode *)malloc(sizeof(*h->buckets[i]));
		h->buckets[i]->data = NULL;
		h->buckets[i]->string = NULL;
		h->buckets[i]->string_length = 0;

		h->sentinels[i] = (struct hashnode *)malloc(sizeof(*h->sentinels[i]));
		h->sentinels[i]->data = NULL;

		h->buckets[i]->next = h->sentinels[i];
		h->buckets[i]->prev = h->buckets[i];
		h->sentinels[i]->next = h->sentinels[i];
		h->sentinels[i]->prev = h->buckets[i];

		h->buckets[i]->string = NULL;
		h->buckets[i]->value = 0;    /* split count of chain */
		h->buckets[i]->nodes_in_chain = 0;    /* split count of chain */
	}

	/* bucket allocation */
	h->currentsize = buckets;
	h->allocated = buckets;

	h->maxload = maxload;

	/* bucket splitting */
	h->p = 0;
	h->maxp = buckets;

	h->node_cnt = 0;
	h->rehash_cnt = 0;

	return h;
}

void
rehash_hashtable(struct hashtable *h)
{
	struct hashnode *l;
	struct hashnode *oldbucket, *oldtail;
	int newindex;

	++h->rehash_cnt;

	if (h->currentsize >= h->allocated)
		reallocate_buckets(h);

	oldbucket = h->buckets[h->p];
	oldtail = h->sentinels[h->p];
	l = oldbucket->next;

	oldbucket->next = oldtail;
	oldtail->prev = oldbucket;

	newindex = h->p + h->maxp;

	if (h->flags > 1)
	{
		printf("# %d rehashes, %d total nodes, old bucket %d, new bucket %d\n",
			h->rehash_cnt, h->node_cnt, h->p, newindex);
		printf("# %d nodes in old bucket\n", oldbucket->nodes_in_chain);
	}

	oldbucket->nodes_in_chain = 0;  /* may not be any lines in chain after rehash */
	++oldbucket->value;      /* number of splits */

	++h->p;

	if (h->p == h->maxp)
	{
		h->maxp *= 2;
		h->p = 0;
	}

	++h->currentsize;

	while (oldtail != l)
	{
		struct hashnode *t = l->next;
		int index = MOD(l->value, h->maxp);

		if (index < h->p)
			index = MOD(l->value, (2*h->maxp));

		if (index == newindex)
		{
			insert_node(h->buckets[newindex], l);
			++h->buckets[newindex]->nodes_in_chain;
		} else {
			insert_node(oldbucket, l);
			++oldbucket->nodes_in_chain;
		}

		l = t;
	}

	if (h->flags > 1)
	{
		printf("# split: %d in old chain, %d in new chain\n",
			oldbucket->nodes_in_chain, h->buckets[newindex]->nodes_in_chain);
	}
}

void
reallocate_buckets(struct hashtable *h)
{
	int i;
	int newallocated;

	newallocated = h->allocated * 2;

	h->buckets = (struct hashnode **)realloc(
		h->buckets,
		sizeof(struct hashnode *)*newallocated
	);

	h->sentinels = (struct hashnode **)realloc(
		h->sentinels,
		sizeof(struct hashnode *)*newallocated
	);

	for (i = h->allocated; i < newallocated; ++i)
	{
		h->buckets[i] = (struct hashnode *)malloc(sizeof(*h->buckets[i]));
		h->sentinels[i] = (struct hashnode *)malloc(sizeof(*h->sentinels[i]));

		h->buckets[i]->next = h->sentinels[i];
		h->sentinels[i]->prev = h->buckets[i];
		h->sentinels[i]->next = h->sentinels[i];
		h->buckets[i]->prev = h->buckets[i];

		h->buckets[i]->string = NULL;
		h->buckets[i]->data = NULL;
		h->sentinels[i]->string = NULL;
		h->sentinels[i]->data = NULL;

		h->buckets[i]->nodes_in_chain = 0;
		h->buckets[i]->value = 0;    /* number of times chain splits, too */
	}

	h->allocated = newallocated;
}

const char *
add_string(struct hashtable *h, const char *string)
{
	return (const char *)add_data(h, string, NULL);
}

void *
add_data(struct hashtable *h, const char *string, void *data)
{
	struct hashnode *n, *head;
	int bucket_index;
	unsigned int hv;
	int hash_mod;

	if((n = node_lookup(h, string, &hv)))
	{
		void *r = NULL;
		if (data)
		{
			r = (void *)n->data;
			n->data = data;
		} else {
			r = (void *)n->string;
		}
		return r;
	}

	bucket_index = MOD(hv, h->maxp);
	hash_mod = h->maxp;
	if (bucket_index < h->p)
	{
		bucket_index = MOD(hv, (2*h->maxp));
		hash_mod = 2*h->maxp;
	}

	if (h->flags > 1)
		printf("# \"%s\": hash value %u, base %d, bucket %d\n", string, hv, hash_mod, bucket_index);

	/* allocate new node */
	n = (struct hashnode *)malloc(sizeof(*n));

	/* fill in newly allocated node */
	n->value = hv;
	n->string_length = strlen(string);
	n->string = malloc(n->string_length+1);
	memcpy(n->string, string, n->string_length+1);
	n->data = data;

	/* add newly allocated node to appropriate chain */
	head = h->buckets[bucket_index];

	/* just push it on the chain */
	n->next = head->next;
	head->next = n;
	n->next->prev = n;
	n->prev = head;

	++h->buckets[bucket_index]->nodes_in_chain;

	++h->node_cnt;

	/* "load" on hashtable too high? */
	if (h->node_cnt/h->currentsize > h->maxload)
		rehash_hashtable(h);

	return (NULL == data)? (void *)n->string: NULL;
}

void free_hashtable(struct hashtable *h)
{
	unsigned int i;

	for (i = 0; i < h->currentsize; ++i)
	{
		struct hashnode *chain = h->buckets[i];
		while (chain != h->sentinels[i])
		{
			struct hashnode *tmp = chain->next;
			if (chain->string)
			{
				free(chain->string);
				chain->string = NULL;
			}
			if (chain->data)
			{
				/* XXX - loose the data pointer */
				chain->data = NULL;
			}
			free(chain);
			chain = tmp;
		}
		free(h->sentinels[i]);
		h->sentinels[i] = NULL;
	}

	/* free the dummy heads and sentinels that the table
	 * doesn't currently use - it doubles bucket count
	 * every time the "load" gets too high */
	for (i = h->currentsize; i < h->allocated; ++i)
	{
		free(h->buckets[i]);
		h->buckets[i] = NULL;
		free(h->sentinels[i]);
		h->sentinels[i] = NULL;
	}

	free(h->buckets);
	free(h->sentinels);
	h->buckets = NULL;
	h->sentinels = NULL;

	free(h);
	h = NULL;
}

const char *
string_lookup(struct hashtable *h, const char *string_to_lookup)
{
	struct hashnode *hn = NULL;
	char *r = NULL;
	unsigned int hv;  /* dummy - not used in this function */

	if (NULL != (hn = node_lookup(h, string_to_lookup, &hv)))
		r = hn->string;

	return r;
}

void *
data_lookup(struct hashtable *h, const char *string_to_lookup)
{
	struct hashnode *hn = NULL;
	void *r = NULL;
	unsigned int hv;  /* dummy - not used in this function */

	if (NULL != (hn = node_lookup(h, string_to_lookup, &hv)))
		r = hn->data;

	return r;
}

/* note that 3rd argument must evaluate to non-NULL - returns hash value
 * of string_to_lookup to add_data() to allow it to skip doing a hash
 */
struct hashnode *
node_lookup(struct hashtable *h, const char *string_to_lookup, unsigned int *rhv)
{
	struct hashnode *r = NULL;
	unsigned int hashval = hash_djb2(string_to_lookup);
	int index = MOD(hashval, h->maxp);
	struct hashnode *chain;

	*rhv = hashval;

	if (index < h->p)
		index = MOD(hashval, (2*h->maxp));

	chain = h->buckets[index]->next;

	while (chain != h->sentinels[index])
	{
		/* checking the equivalence of hash value first prevents
		 * expensive byte-by-byte strcmp().  Seems to improve performance. */
		if (chain->value == hashval &&
			!strcmp(chain->string, string_to_lookup))
		{
			r = chain;
			break;
		}

		chain = chain->next;
	}

	return r;
}

unsigned int
hash_djb2(const char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

int
sparsebit(long i)
{
	int p = 0;

	if (i == 0) return(-1);     /* no bits set */

	if ((i & (-i)) != i) return(-1);    /* two or more bits set */

	if (i & 0xAAAAAAAA) p++;
	if (i & 0xCCCCCCCC) p += 2;
	if (i & 0xF0F0F0F0) p += 4;
	if (i & 0xFF00FF00) p += 8;
	if (i & 0xFFFF0000) p += 16;

	return p;
}

int
msbbit(long i)
{
	long i2;

	if (i<0) return(31); /* speed-up assumes a 'long's msb is at pos 31 */

	while ((i2 = (i & (i-1)))) i = i2;

	return(sparsebit(i));
}