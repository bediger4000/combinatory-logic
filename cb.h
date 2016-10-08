struct queue *queueinit(void);
void queuedestroy(struct queue *);
void enqueue(struct queue *, int);
int  dequeue(struct queue *);
int  queueempty(struct queue *);
