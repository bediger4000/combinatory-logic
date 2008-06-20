/*
	Copyright (C) 2008, Bruce Ediger

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
#ifndef _TCC_
#ident "$Id$"
#endif

#include <stdio.h>    /* stderr */
#include <stdlib.h>   /* malloc(), free(), realloc() */
#include <assert.h>   /* assert() macro */
#include <ctype.h>    /* isprint() */
#include <string.h>   /* memmove() */

#include <buffer.h>

struct buffer *
new_buffer(int desired_size)
{
	struct buffer *r = malloc(sizeof *r);
	assert(desired_size > 0);
	if (r)
	{
		r->buffer = malloc(desired_size);
		if (r->buffer)
			r->size = desired_size;
		else
			r->size = 0;
		r->offset = 0;

		assert(0 == r->offset);
		assert((r->buffer && 0 < r->size) || (NULL == r->buffer));
	}
	return r;
}

void
delete_buffer(struct buffer *b)
{
	if (b)
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
}

void
resize_buffer(struct buffer *b, int increment)
{
	if (b)
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
}

void
buffer_append(struct buffer *b, const char *bytes, int length)
{
	if (NULL != b && NULL != bytes && 0 < length)
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
}

void
print_buffer(int lineno, struct buffer *b)
{
	print_buffer_size(lineno, b);
	if (b)
	{
		if (0 < b->offset)
			print_buffer_contents(b->buffer, b->offset);
	} else {
		fprintf(stderr, "NULL buffer\n");
	}
}

void
print_buffer_size(int lineno, struct buffer *b)
{
	if (b)
	{
		fprintf(stderr, "Line %d, buffer of size %d at 0x%p has %d bytes in it\n",
			lineno, b->size, b->buffer, b->offset);
	} else {
		fprintf(stderr, "Line %d, NULL buffer\n", lineno);
	}
}

void
print_buffer_contents(char *b, int length)
{
	int i, cnt;
	char *x, *p = malloc(4*length);
	for (x = p, i = 0, cnt = 0; i < length; ++i)
	{
		switch (b[i])
		{
		case '\0': *x++ = '\\'; *x++ = '0';  ++cnt; break;
		case ' ':   *x++ = '#';              ++cnt; break;
		case '\t':  *x++ = '\\'; *x++ = 't'; ++cnt; break;
		case '\n':  *x++ = '\\'; *x++ = 'n'; ++cnt; break;
		case '\f':  *x++ = '\\'; *x++ = 'f'; ++cnt; break;
		case '\r':  *x++ = '\\'; *x++ = 'r'; ++cnt; break;
		case '\v':  *x++ = '\\'; *x++ = 'v'; ++cnt; break;
		default:
			if (isprint(b[1]))
			{
				*x++ = b[i];
				++cnt;
			} else {
				*x++ = '\\';
				sprintf(x, "%02x", b[i]);
				x += 3;
				++cnt;
			}
			break;
		}
	}
	*x = '\0';
	fprintf(stderr, "Contents (%d): \"%s\"\n", cnt, p);
	free(p);
}

/* Shuffles a struct buffer's raw buffer from index 0 to
 * the index of an end-of-message (eom) mark byte's index.
 * If it finds the end-of-message byte value, it moves all
 * the remaining bytes in the raw buffer to the start of
 * the raw buffer, and sets offset value appropriately.
 * This effectivel "consumes" the message in the raw buffer.
 */
void
consume_message(struct buffer *b, int eom)
{
	if (b && b->buffer)
	{
		int i;

		/* find carriage return */
		for (i = 0; i < b->offset; ++i)
		{
			if (eom == b->buffer[i])
			{
				/* move bytes from index of <eom> + 1 to end-of-buffer
		 		* back to the start of the buffer. i contains index of <eom> */
				memmove(b->buffer, &b->buffer[i+1], b->offset - i - 1);
				b->offset -= (i + 1);
				break;
			}
		}
	}
}
