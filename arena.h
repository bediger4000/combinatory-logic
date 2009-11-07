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

struct memory_arena *new_arena(int memory_info_flag);
void free_arena_contents(struct memory_arena *ma);
void deallocate_arena(struct memory_arena *ma, int memory_info_flag);

void *arena_alloc(struct memory_arena *ma, size_t size);

