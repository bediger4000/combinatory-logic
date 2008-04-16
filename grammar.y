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

#ifdef YYBISON
#define YYERROR_VERBOSE
#endif

/* flags, binary on/off for various outputs */
int debug_reduction  = 0;
int elaborate_output = 0;
int trace_reduction  = 0;
int reduction_timer  = 0;
int single_step      = 0;
int memory_info      = 0;
int count_reductions = 0;    /* produce a count of reductions */

int reduction_timeout = 0;   /* how long to let a graph reduction run, seconds */
int max_reduction_count = 0; /* when non-zero, how many reductions to perform */

int prompting = 1;

/* Signal handling.  in_reduce_graph used to (a) handle
 * contrl-C interruptions (b) reduction-run-time timeouts,
 * (c) getting out of single-stepped graph reduction in reduce_graph()
 */
void sigint_handler(int signo);
sigjmp_buf in_reduce_graph;
int interpreter_interrupted = 0;
int reduction_interrupted = 0;

void top_level_cleanup(int syntax_error_processing);

struct node *reduce_tree(struct node *root);
struct node *execute_bracket_abstraction(
	struct node *(*bafunc)(struct node *, struct node *),
	struct node *abstracted_var,
	struct node *root
);
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
int yyerror(const char *s1);

extern int yyparse(void);

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
int T_as_combinator = 1;
int M_as_combinator = 1;

%}

%union{
	const char *identifier;
	const char *string_constant;
	int   numerical_constant;
	struct node *node;
	enum combinatorName cn;
	struct node *(*bafunc)(struct node *, struct node *);
}


%token <node> TK_EOL
%token TK_LPAREN TK_RPAREN TK_LBRACK TK_RBRACK
%token <identifier> TK_IDENTIFIER
%token <cn> TK_PRIMITIVE
%token <string_constant> STRING_LITERAL
%token <node> TK_REDUCE TK_TIMEOUT
%token <numerical_constant> NUMERICAL_CONSTANT
%token <identifier> TK_ALGORITHM_NAME
%token TK_DEF TK_TIME TK_LOAD TK_ELABORATE TK_TRACE TK_SINGLE_STEP TK_DEBUG
%token TK_MAX_COUNT TK_SET_BRACKET_ABSTRACTION 

%type <node> expression stmnt application term constant interpreter_command
%type <node> bracket_abstraction 
%type <bafunc> abstraction_algorithm

%%

program
	: stmnt { top_level_cleanup(0); }
	| program stmnt { top_level_cleanup(0); }
	| error  /* magic token - yacc unwinds to here on syntax error */
		{ top_level_cleanup(1); }
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
			abbreviation_add($2, $3);
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
	| TK_MAX_COUNT NUMERICAL_CONSTANT TK_EOL { max_reduction_count = $2; }
	| TK_SET_BRACKET_ABSTRACTION TK_ALGORITHM_NAME TK_EOL { default_bracket_abstraction = determine_bracket_abstraction($2); }
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
	| bracket_abstraction abstraction_algorithm expression
		{
			$$ = execute_bracket_abstraction($2, $1, $3);
			++$1->refcnt;
			free_node($1);
			++$3->refcnt;
			free_node($3);
		}
	;

abstraction_algorithm
	: TK_ALGORITHM_NAME  { $$ = determine_bracket_abstraction($1); }
	| { $$ = default_bracket_abstraction; }
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
	struct node *(*dba)(struct node *, struct node *);

	setup_abbreviation_table(h);
	setup_atom_table(h);

	while (-1 != (c = getopt(ac, av, "deL:mN:pstT:C:B:x")))
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
		case 'p':
			prompting = 0;
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
		case 'B':
			dba = determine_bracket_abstraction(optarg);
			if (dba) default_bracket_abstraction = dba;
			else {
				fprintf(stderr, "Unknown bracket abstraction algoritm \"%s\"\n", optarg);
				usage(av[0]);
			}
			break;
		case 'C':
			/* Turn *off* selected combinators: they become mere identifiers */
			switch(optarg[0])
			{
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
			case 'M':
				M_as_combinator = 0;
				break;
			case 'T':
				T_as_combinator = 0;
				break;
			default:
				fprintf(stderr, "Unknown primitive combinator \"%s\"\n", optarg);
				usage(av[0]);
				break;
			}
			break;
		case 'N':
			max_reduction_count = strtol(optarg, NULL, 10);
			if (max_reduction_count < 0) max_reduction_count = 0;
			break;
		}
	}

	init_node_allocation(memory_info);

	if (load_files)
	{
		struct filename_node *t, *z;
		int old_prompt = prompting;
		prompting = 0;
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
		prompting = old_prompt;
	}

	set_yyin_stdin();

	do {
		if (prompting) printf("CL> ");
		r =  yyparse();
	} while (r);
	if (prompting) printf("\n");

	if (memory_info) fprintf(stderr, "Memory usage indicators:\n");
	free_all_nodes(memory_info);
	free_hashtable(h);
	free_all_spine_stacks(memory_info);

	return r;
}

void top_level_cleanup(int syntax_error_occurred)
{
	reset_node_allocation();
	reduction_interrupted = 0;
	if (prompting && !syntax_error_occurred) printf("CL> ");
}

int
yyerror(const char *s1)
{
    fprintf(stderr, "%s\n", s1);

    return 0;
}


void
sigint_handler(int signo)
{
	/* the "return value" of 1 or 2 comes out in the
	 * call to sigsetjmp() in reduce_tree().
	 */
	siglongjmp(in_reduce_graph, signo == SIGINT? 1: 2);
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

	/* new_root - points to a "dummy" node, necessary for I and
	 * K reductions, if the expression is something like "I x" or
	 * K a b. */
	++new_root->refcnt;
	MARK_RIGHT_BRANCH_TRAVERSED(new_root);

	old_sigint_handler = signal(SIGINT, sigint_handler);
	old_sigalm_handler = signal(SIGALRM, sigint_handler);

	if (!(cc = sigsetjmp(in_reduce_graph, 1)))
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
		case 1:
			phrase = "Interrupt";
			reduction_interrupted = 1;
			break;
		case 2:
			reduction_interrupted = 1;
			phrase = "Timeout";
			break;
		case 3:
			phrase = "Terminated";
			reduction_interrupted = 1;
			break;
		case 4:
			phrase = "Reduction limit";
			reduction_interrupted = 0;
			break;
		default:
			phrase = "Unknown";
			break;
		}
		printf("%s\n", phrase);
		++interpreter_interrupted;
	}

	signal(SIGINT, old_sigint_handler);
	signal(SIGALRM, old_sigalm_handler);

	if (reduction_timer)
		printf("elapsed time %.3f seconds\n", elapsed_time(before, after));

	return new_root;
}

/*
 * Function execute_bracket_abstraction() exists to wrap various bracket
 * abstraction functions.  It wraps with setting signal handlers,
 * taking before & after timestamps, setting jmp_buf structs, etc.
 */
struct node *
execute_bracket_abstraction(
	struct node *(*bafunc)(struct node *, struct node *),
	struct node *abstracted_var,
	struct node *root
)
{
	struct node *r = NULL;
	void (*old_sigint_handler)(int);
	void (*old_sigalm_handler)(int);
	struct timeval before, after;
	int cc;

	old_sigint_handler = signal(SIGINT, sigint_handler);
	old_sigalm_handler = signal(SIGALRM, sigint_handler);

	if (!(cc = sigsetjmp(in_reduce_graph, 1)))
	{
		alarm(reduction_timeout);
		gettimeofday(&before, NULL);
		r = (bafunc)(abstracted_var, root);
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
	}

	signal(SIGINT, old_sigint_handler);
	signal(SIGALRM, old_sigalm_handler);

	if (reduction_timer)
		printf("elapsed time %.3f seconds\n", elapsed_time(before, after));

	return r;
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
		"-N number      perform up to number reductions\n"
		"-s             single-step reductions\n"
		"-T number      evaluate an expression for up to number seconds\n"
		"-t             trace reductions\n"
		"-C combinator  treat combinator as a non-primitive. Combinator one of S, K, I, B, C, W, M, T\n"
		"-B algoritm    Use algorithm as default for bracket abstraction.  One of curry, tromp, grz, btmk\n"
		""
	);
}
