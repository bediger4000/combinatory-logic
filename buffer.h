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



struct buffer {
	char *buffer;
	int   size;
	int   offset;
};

struct buffer *new_buffer(int desired_size);
void           resize_buffer(struct buffer *b, int increment);
void           buffer_append(struct buffer *b, const char *bytes, int length);
void           delete_buffer(struct buffer *b);
