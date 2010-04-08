/*
	Copyright (C) 2008-2009, Bruce Ediger

    This file is part of cl.

    cl is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    cl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cl; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/* $Id$ */

#include <stdio.h>    /* NULL manifest constant */
#include <stdlib.h>   /* malloc(), free(), realloc() */
#include <string.h>   /* memcpy() */

#include <buffer.h>

struct buffer *
new_buffer(int desired_size)
{
	struct buffer *r = malloc(sizeof *r);

	r->buffer = malloc(desired_size);
	r->size = r->buffer? desired_size: 0;
	r->offset = 0;

	return r;
}

void
delete_buffer(struct buffer *b)
{
	if (b->buffer)
	{
		free(b->buffer);
		b->buffer = NULL;
	}
	b->offset = b->size = 0;
	free(b);
	b = NULL;
}

void
resize_buffer(struct buffer *b, int increment)
{
	char *reallocated_buffer;
	int new_size = b->size + increment;

	reallocated_buffer = realloc(b->buffer, new_size);

	if (reallocated_buffer)
	{
		b->buffer = reallocated_buffer;
		b->size   = new_size;
	}
}

void
buffer_append(struct buffer *b, const char *bytes, int length)
{
	if (length >= (b->size - b->offset))
		resize_buffer(b, length);

	if (length < b->size - b->offset)
	{
		/* resize_buffer() might fail */
		memcpy(&b->buffer[b->offset], bytes, length);
		b->offset += length;
	}
}
