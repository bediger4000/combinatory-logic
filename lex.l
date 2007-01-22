%{
/*
	Copyright (C) 2007, Bruce Ediger

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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>    /* errno */
#include <string.h>   /* strlen(), strerror() */

#include <hashtable.h>
#include <atom.h>
#include <node.h>

#include "y.tab.h"
int lineno = 0;

const char *current_input_stream;

char *unescape_string(char *s);
void  push_and_open(const char *filename);

struct stream_node {
#ifdef FLEX_SCANNER   
	YY_BUFFER_STATE stream;
#else
	FILE *stream;
#endif
	struct stream_node *next;
	const char *old_filename;
	int old_lineno;
};

struct stream_node *file_stack = NULL;

void set_yyin_stdin(void);

extern int S_as_combinator;
extern int K_as_combinator;
extern int I_as_combinator;
extern int B_as_combinator;
extern int C_as_combinator;
extern int W_as_combinator;


%}

%%

\#.*$		{ return TK_EOL; }
\n		    { return TK_EOL; }
\\\n	    { /* Just eat it. */ }
\(		    { return TK_LPAREN; }
\)		    { return TK_RPAREN; }
\[		    { return TK_LBRACK; }
]		    { return TK_RBRACK; }
"def"       { return TK_DEF; }
"define"    { return TK_DEF; }
"reduce"    { return TK_REDUCE; }
"timer"     { return TK_TIME; }
"timeout"   { return TK_TIMEOUT; }
"debug"     { return TK_DEBUG; }
"step"      { return TK_SINGLE_STEP; }
"trace"     { return TK_TRACE; }
"elaborate" { return TK_ELABORATE; }
"load"      { return TK_LOAD; }
"curry"     { return TK_CURRY; }
"turner"    { return TK_TURNER; }
[1-9][0-9]* { yylval.numerical_constant = strtol(yytext, NULL, 10);
				return NUMERICAL_CONSTANT; }
\"(\\.|[^\\"])*\" {
	char *tmp;
	tmp = unescape_string(yytext);
	yylval.string_constant = Atom_string(tmp);
	free(tmp);
	return STRING_LITERAL;
}
S { if (S_as_combinator) { yylval.cn = COMB_S; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
K { if (K_as_combinator) { yylval.cn = COMB_K; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
I { if (I_as_combinator) {yylval.cn = COMB_I; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
B { if (B_as_combinator) {yylval.cn = COMB_B; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
C { if (C_as_combinator) {yylval.cn = COMB_C; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
W { if (W_as_combinator) {yylval.cn = COMB_W; return TK_PRIMITIVE;}
	else {yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER;}
 }
[a-zA-Z][a-zA-Z_0-9]* { yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER; }
.		    { /* Just eat it. */ }

%%

char *
unescape_string(char *s)
{
	char *n = NULL;
	if (s)
	{
		int i, j, l;
		if ('"' == s[0])
			++s;
		if ('"' == s[strlen(s) - 1])
			s[strlen(s) - 1] = '\0';
		n = malloc(strlen(s) + 1);
		l = strlen(s);
		for (i = 0, j = 0; i < l; ++i)
		{
			if ('\\' == s[i])
			{
				++i;
				switch (s[i])
				{
				/* XXX - doesn't do '\0' or other numerical escapes */
				case 't': n[j++] = '\t';  break;
				case 'r': n[j++] = '\r';  break;
				case 'n': n[j++] = '\n';  break;
				case '\\': n[j++] = '\\'; break;
				default:
					n[j++] = s[i];
					break;
				}
			} else {
				n[j++] = s[i];
			}
		}
		n[j] = '\0';
	}
	return n;
}

void
push_and_open(const char *filename)
{
	FILE *fin;
	extern FILE *yyin;
	extern int lineno;
	extern const char *current_input_stream;

	if (NULL != (fin = fopen(filename, "r")))
	{
		struct stream_node *n;
		n = malloc(sizeof(*n));
#ifdef FLEX_SCANNER
		n->stream = YY_CURRENT_BUFFER;
		yyin = fin;
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
#else
		n->stream = yyin;
		yyin = fin;
#endif
		n->next = file_stack;
		n->old_filename = current_input_stream;
		n->old_lineno = lineno;
		current_input_stream = filename;
		file_stack = n;
		lineno = 0;
	} else {
		fprintf(stderr, "Could not open \"%s\" for read: %s\n",
			filename, strerror(errno));
	}
}

void
set_yyin_stdin(void)
{
		yyin = stdin;
#ifdef FLEX_SCANNER
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
#endif
}

void
set_yyin(const char *filename)
{
	FILE *fin;

	if (NULL != (fin = fopen(filename, "r")))
	{
		yyin = fin;
#ifdef FLEX_SCANNER
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
#endif
		current_input_stream = filename;
		lineno = 0;
	} else {
		fprintf(stderr, "Could not open \"%s\" for read: %s\n",
			filename, strerror(errno));
	}
}

void
reset_yyin(void)
{
	if (yyin)
		fclose(yyin);
	yyin = NULL;
#ifdef FLEX_SCANNER
	yy_delete_buffer(YY_CURRENT_BUFFER);
#endif
}

int
yywrap()
{
	extern FILE *yyin;
	extern int lineno;
	extern struct stream_node *file_stack;

	int r = 1;
	if (file_stack)
	{
		struct stream_node *tmp = file_stack->next;
		fclose(yyin);
#ifdef FLEX_SCANNER
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(file_stack->stream);
#else
		yyin = file_stack->stream;
#endif
		current_input_stream = file_stack->old_filename;
		lineno = file_stack->old_lineno;
		file_stack->stream = NULL;
		file_stack->next = NULL;
		free(file_stack);
		file_stack = tmp;
		r = 0;
	}
	return r;
}
