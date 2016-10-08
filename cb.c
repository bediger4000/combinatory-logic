#include <stdio.h>
#include <stdlib.h>

#include <cb.h>

/*
 * These two need to constitute 2 bit patterns:
 * CBUFSIZE  100...0
 * CBUFMASK   11...1
 *
 * Actual value of CBUFSIZE mostly constitutes a guess.
 */
#define CBUFSIZE 0x020
#define CBUFMASK 0x01f

struct queue_node {
	int state;
	struct queue_node *next;
};

struct queue {
	int *cbuf;
	int  in;
	int  out;
	struct queue_node *ovfhead;
	struct queue_node *ovftail;
	int size;
};



struct queue *
queueinit()
{
	struct queue *r;

	r = malloc(sizeof(*r));

	r->cbuf = malloc(CBUFSIZE*sizeof(int));

	r->in = 0;
	r->out = 0;
	r->ovfhead = r->ovftail = NULL;
	r->size = 0;

	return r;
}

void
enqueue(struct queue *q, int state)
{

	++q->size;

	if (((q->in + 1)&CBUFMASK) != q->out)
	{
		q->cbuf[q->in] = state;
		q->in = (q->in+1)&CBUFMASK;

	} else {
		struct queue_node *newnode = malloc(sizeof *newnode);
		newnode->state = state;
		newnode->next = NULL;

		if (NULL == q->ovftail)
			q->ovfhead = q->ovftail = newnode;
		else {
			q->ovftail->next = newnode;
			q->ovftail = newnode;
		}
	}
}

int
queueempty(struct queue *q)
{
	return (q->in == q->out);
}

int
dequeue(struct queue *q)
{
	int r = q->cbuf[q->out];

	q->out = (q->out + 1)&CBUFMASK;
	--q->size;

	if (NULL != q->ovfhead)
	{
		struct queue_node *tmp;

		q->cbuf[q->in]= q->ovfhead->state;
		q->in = (q->in+1)&CBUFMASK;
		tmp = q->ovfhead;
		q->ovfhead = q->ovfhead->next;
		if (NULL == q->ovfhead) q->ovftail = NULL;
		free(tmp);
	}

	return r;
}

void
queuedestroy(struct queue *q)
{
	if (NULL != q->cbuf) free(q->cbuf);

	while (NULL != q->ovfhead)
	{
		struct queue_node *t = q->ovfhead->next;

		free(q->ovfhead);

		q->ovfhead = t;
	}

	free(q);
}
