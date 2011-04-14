#include <stdio.h>
#include <stdlib.h> /* malloc(), realloc() */
#include <string.h> /* strcpy() */

#include <node.h>
#include <buffer.h>
#include <pattern_paths.h>

void calculate_strings(struct node *node, struct buffer *buf);
static char **paths = NULL;
static int path_cnt = 0;
static int paths_used = 0;

char **
get_pat_paths(void)
{
	return paths;
}

int
set_pattern_paths(struct node *pattern)
{
	struct buffer *buf = new_buffer(512);
	calculate_strings(pattern, buf);
	delete_buffer(buf);
	return paths_used;
}

void
calculate_strings(struct node *node, struct buffer *b)
{
	int curr_offset, orig_offset = b->offset;
	char *buf;
	char *pattern_string;

	switch (node->typ)
	{
	case APPLICATION:
		buffer_append(b, node->name, strlen(node->name));
		curr_offset = b->offset;

		buffer_append(b, "1", 1);
		calculate_strings(node->left, b);

		b->offset = curr_offset;
		buffer_append(b, "2", 1);

		calculate_strings(node->right, b);

		b->offset = orig_offset;
		break;
	case ATOM:
		buf = b->buffer;
		buf[b->offset] = '\0';
		pattern_string = malloc(b->offset + 1 + 1);

		if ('*' != node->name[0])
			sprintf(pattern_string, "%s%s", buf, node->name);
		else
			sprintf(pattern_string, "%s", buf);

		if (paths_used >= path_cnt)
		{
			char **tmp;
			int alloc_bytes = (sizeof(char *))*(path_cnt + 4);

			if (paths)
				tmp = realloc(paths, alloc_bytes);
			else
				tmp = malloc(alloc_bytes);

			if (tmp)
			{
				paths = tmp;
				path_cnt += 4;
			} else
				fprintf(stderr, "Failed to alloc/realloc pattern paths array, size %d\n", path_cnt + 4);
		}

		/* XXX - If a realloc() fails, this could overwrite paths[] */
		paths[paths_used++] = pattern_string;

		break;
	}
}
