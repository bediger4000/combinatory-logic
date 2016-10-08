/*
	Copyright (C) 2011, Bruce Ediger

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
struct output_extent {
	int   *out;   /* array of int, lengths of matched paths */
	int    len;   /* next array element to fill in */
	int    max;   /* number of elements in array */
};

struct gto {
	int **ary;                     /* transition table */
	int   ary_len;                 /* max state currently in table */
	int  *failure;                 /* failure states */
	int **delta;
	struct output_extent *output;  /* output for output states */
	int   output_len;              /* max state for output states */
	int   pattern_path_cnt;        /* count of paths through the pattern tree */
};

#define FAIL -1

void add_state(struct gto *p, int state, char input, int new_state);
void set_output(struct gto *p, int state, char *keyword);
void construct_goto(char *keywords[], int k, struct gto *g);
void construct_failure(struct gto *g);
void construct_delta(struct gto *g);
struct gto *init_goto(void);
void        destroy_goto(struct gto *);
