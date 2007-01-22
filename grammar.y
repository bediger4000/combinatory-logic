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
#include <errno.h>    /* errno */
#include <string.h>   /* strerror() */
#include <stdlib.h>   /* malloc(), free(), strtoul() */
#include <unistd.h>   /* getopt() */
#include <signal.h>   /* signal(), etc */
#include <setjmp.h>   /* setjmp(), longjmp(), jmp_buf */
#include <sys/time.h> /* gettimeofday(), struct timeval */

extern char *optarg;

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <graph.h>
#include <abbreviations.h>
#include <spine_stack.h>
#include <bracket_abstraction.h>

/* flags, binary on/off for various outputs */
int debug_reduction  = 0;
int elaborate_output = 0;
int trace_reduction  = 0;
int reduction_timer  = 0;
int single_step      = 0;
int memory_info      = 0;

int reduction_timeout = 0;  /* how long to let a graph reduction run, seconds */
int max_reduction_count = 0; /* when non-zero, how many reductions to perform */

/* Signal handling.  in_reduce_graph used to (a) handle
 * contrl-C interruptions (b) reduction-run-time timeouts,
 * (c) getting out of single-stepped graph reduction in reduce_graph()
 */
void sigint_handler(int signo);
jmp_buf in_reduce_graph;
int interpreter_interrupted = 0;
int reduction_interrupted = 0;

struct node *reduce_tree(struct node *root);
float elapsed_time(struct timeval before, struct timeval after);
void usage(char *progname);

struct filename_node {
	const char *filename;
	struct filename_node *next;
};

/* from lex.l */
extern void set_yyin_stdin(void); 
extern void set_yyin(const char *filename); 
extern void reset_yyin(void);
extern void  push_and_open(const char *filename);

extern int yylex(void);
int yyerror(char *s1);

/* Various "treat as combinator" flags.
 * For example: S_as_combinator, when set (default) causes
 * the lexer to treat "S" as an S-combinator.  When unset,
 * the lexer treats "S" as any other variable. This can interact
 * strangely with bracket abstraction, which assumes that its
 * own use of "S" (again, as example) always constitutes a combinator.
 */
int S_as_combinator = 1;
int K_as_combinator = 1;
int I_as_combinator = 1;
int B_as_combinator = 1;
int C_as_combinator = 1;
int W_as_combinator = 1;

%}

%union{
	const char *identifier;
	const char *string_constant;
	int   numerical_constant;
	struct node *node;
	enum combinatorName cn;
}


%token <node> TK_EOL
%token TK_LPAREN TK_RPAREN TK_LBRACK TK_RBRACK
%token <identifier> TK_IDENTIFIER
%token <cn> TK_PRIMITIVE
%token <string_constant> STRING_LITERAL
%token TK_DEF TK_TIME TK_LOAD TK_ELABORATE TK_TRACE TK_SINGLE_STEP TK_DEBUG
%token <node> TK_REDUCE TK_TIMEOUT
%token <numerical_constant> NUMERICAL_CONSTANT
%token TK_TURNER TK_CURRY

%type <node> expression stmnt application term constant interpreter_command
%type <node> bracket_abstraction

%%

program
	: stmnt {
			reset_node_allocation();
			reduction_interrupted = 0;
		}
	| program stmnt  { reset_node_allocation(); reduction_interrupted = 0; }
	| error  /* magic token - yacc unwinds to here on syntax error */
		{ reset_node_allocation(); reduction_interrupted = 0; }
	;

stmnt
	: expression TK_EOL
		{
			print_graph($1, 0, 0); 
			$$ = reduce_tree($1);
			if (!reduction_interrupted)
				print_graph($$->left, 0, 0);
			free_node($$);
		}
	| TK_DEF TK_IDENTIFIER expression TK_EOL
		{
			struct node *prev = abbreviation_add($2, $3);
			if (prev) free_graph(prev);
			++$3->refcnt;
			free_node($3);
		}
	| interpreter_command
	| TK_EOL  { $$ = NULL; /* blank lines */ }
	;

interpreter_command
	: TK_TIME TK_EOL { reduction_timer ^= 1; }
	| TK_ELABORATE TK_EOL { elaborate_output ^= 1; }
	| TK_DEBUG TK_EOL { debug_reduction ^= 1; }
	| TK_TRACE TK_EOL { trace_reduction ^= 1; }
	| TK_SINGLE_STEP TK_EOL { single_step ^= 1; }
	| TK_LOAD STRING_LITERAL TK_EOL { push_and_open($2); }
	| TK_TIMEOUT NUMERICAL_CONSTANT TK_EOL { reduction_timeout = $2; }
	;

expression
	: application          { $$ = $1; }
	| term                 { $$ = $1; }
	| TK_REDUCE expression
		{
			struct node *tmp;
			tmp = reduce_tree($2);
			--tmp->left->refcnt;
			$$ = tmp->left;
			tmp->left = NULL;
			free_node(tmp);
		}
	| bracket_abstraction expression
		{
			$$ = curry_bracket_abstraction($1, $2);
			++$1->refcnt;
			free_node($1);
			++$2->refcnt;
			free_node($2);
		}
	| bracket_abstraction TK_CURRY expression
		{
			$$ = curry_bracket_abstraction($1, $3);
			++$1->refcnt;
			free_node($1);
			++$3->refcnt;
			free_node($3);
		}
	| bracket_abstraction TK_TURNER expression
		{
			$$ = turner_bracket_abstraction($1, $3);
			++$1->refcnt;
			free_node($1);
			++$3->refcnt;
			free_node($3);
		}
	;

application
	: term term        { $$ = new_application($1, $2); }
	| application term { $$ = new_application($1, $2); }
	;

bracket_abstraction
	: TK_LBRACK TK_IDENTIFIER TK_RBRACK
		{ $$ = new_term($2); }
	;

term
	: constant                         { $$ = $1; }
	| TK_IDENTIFIER
		{
			$$ = abbreviation_lookup($1);
			if (!$$)
				$$ = new_term($1);
		}
	| TK_LPAREN expression TK_RPAREN  { $$ = $2; }
	;

constant
	: TK_PRIMITIVE  { $$ = new_combinator($1); }
	;

%%

int
main(int ac, char **av)
{
	int c, r;
	struct filename_node *p, *load_files = NULL, *load_tail = NULL;
	struct hashtable *h = init_hashtable(64, 10);
	extern int yyparse();

	setup_abbreviation_table(h);
	setup_atom_table(h);

	while (-1 != (c = getopt(ac, av, "deL:mstT:SKIBCWx")))
	{
		switch (c)
		{
		case 'd':
			debug_reduction = 1;
			break;
		case 'e':
			elaborate_output = 1;
			break;
		case 'L':
			p = malloc(sizeof(*p));
			p->filename = Atom_string(optarg);
			p->next = NULL;
			if (load_tail)
				load_tail->next = p;
			load_tail = p;
			if (!load_files)
				load_files = p;
			break;
		case 'm':
			memory_info = 1;
			break;
		case 's':
			single_step = 1;
			break;
		case 'T':
			reduction_timeout = strtol(optarg, NULL, 10);
			break;
		case 't':
			trace_reduction = 1;
			break;
		case 'x':
			usage(av[0]);
			exit(0);
			break;
		/* Turn *off* selected combinators: they become mere identifiers */
		case 'S':
			S_as_combinator = 0;
			break;
		case 'K':
			K_as_combinator = 0;
			break;
		case 'I':
			I_as_combinator = 0;
			break;
		case 'B':
			B_as_combinator = 0;
			break;
		case 'C':
			C_as_combinator = 0;
			break;
		case 'W':
			W_as_combinator = 0;
			break;
		}
	}

	init_node_allocation(memory_info);

	if (load_files)
	{
		struct filename_node *t, *z;
		for (z = load_files; z; z = t)
		{
			FILE *fin;

			t = z->next;

			printf("load file named \"%s\"\n",
				z->filename);

			if (!(fin = fopen(z->filename, "r")))
			{
				fprintf(stderr, "Problem reading \"%s\": %s\n",
					z->filename, strerror(errno));
				continue;
			}

			set_yyin(z->filename);

			r = yyparse();

			reset_yyin();

			if (r)
				printf("Problem with file \"%s\"\n", z->filename);

			free(z);
			fin = NULL;
		}
	}

	set_yyin_stdin();

	do {
		r =  yyparse();
	} while (r);

	if (memory_info) fprintf(stderr, "Memory usage indicators:\n");
	free_all_nodes(memory_info);
	free_hashtable(h);
	free_all_spine_stacks(memory_info);

	return r;
}

int
yyerror(char *s1)
{
    fprintf(stderr, "%s\n", s1);

    return 0;
}


void
sigint_handler(int signo)
{
	/* the "return value" of 1 or 2 comes out in the
	 * call to setjmp() in reduce_tree().
	 */
	longjmp(in_reduce_graph, signo == SIGINT? 1: 2);
}

/*
 * Function reduce_tree() exists to wrap reduce_graph()
 * at the topmost level.  It wraps with setting signal handlers,
 * taking before & after timestamps, setting jmp_buf structs, etc.
 */
struct node *
reduce_tree(struct node *real_root)
{
	void (*old_sigint_handler)(int);
	void (*old_sigalm_handler)(int);
	struct timeval before, after;
	int cc;
	struct node *new_root = new_application(real_root, NULL);

	++new_root->refcnt;

	MARK_RIGHT_BRANCH_TRAVERSED(new_root);

	old_sigint_handler = signal(SIGINT, sigint_handler);
	old_sigalm_handler = signal(SIGALRM, sigint_handler);

	if (!(cc = setjmp(in_reduce_graph)))
	{
		alarm(reduction_timeout);
		gettimeofday(&before, NULL);
		reduce_graph(new_root);
		alarm(0);
		gettimeofday(&after, NULL);
	} else {
		const char *phrase = "Unset";
		alarm(0);
		gettimeofday(&after, NULL);
		switch (cc)
		{
		case 1: phrase = "Interrupt"; break;
		case 2: phrase = "Timeout";   break;
		case 3: phrase = "Terminated";break;
		default:
			phrase = "Unknown";
			break;
		}
		printf("%s\n", phrase);
		reduction_interrupted = 1;
		++interpreter_interrupted;
	}

	signal(SIGINT, old_sigint_handler);
	signal(SIGALRM, old_sigalm_handler);

	if (reduction_timer)
		printf("elapsed time %.3f seconds\n", elapsed_time(before, after));

	return new_root;
}

/* utility function elapsed_time() */
float
elapsed_time(struct timeval before, struct timeval after)
{
	float r = 0.0;

	if (before.tv_usec > after.tv_usec)
	{
		after.tv_usec += 1000000;
		--after.tv_sec;
	}

	r = (float)(after.tv_sec - before.tv_sec)
		+ (1.0E-6)*(float)(after.tv_usec - before.tv_usec);

	return r;
}

void
usage(char *progname)
{
	fprintf(stderr, "%s: Combinatory Logic like language interpreter\n",
		progname);
	fprintf(stderr, "Flags:\n"
		"-d             debug reductions\n"
		"-e             elaborate output\n"
		"-L  filename   Load and interpret a filenamed filename\n"
		"-m             on exit, print memory usage summary\n"
		"-s             single-step reductions\n"
		"-T number      reduce a graph for number seconds, the stop\n"
		"-t             trace reductions\n"
		"-S             treat S as a non-combinator\n"
		"-K             treat K as a non-combinator\n"
		"-I             treat I as a non-combinator\n"
		"-B             treat B as a non-combinator\n"
		"-C             treat C as a non-combinator\n"
		"-W             treat W as a non-combinator\n"
		""
	);
}
