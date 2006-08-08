struct memory_arena *new_arena(void);
void free_arena_contents(struct memory_arena *ma);
void deallocate_arena(struct memory_arena *ma);

void *arena_alloc(struct memory_arena *ma, size_t size);

