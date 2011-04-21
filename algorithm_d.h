int set_pattern_paths(struct node *pattern);
char **get_pat_paths(void);
void free_paths(void);
int algorithm_d(struct gto *g, struct node *subject, int pat_path_cnt);
