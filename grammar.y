%{
/*
	Copyright (C) 2007-2009, Bruce Ediger

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
#include <buffer.h>
#include <graph.h>
#include <abbreviations.h>
#include <spine_stack.h>
#include <bracket_abstraction.h>
#include <cycle_detector.h>
#include <parser.h>

#ifdef YYBISON
#define YYERROR_VERBOSE
#endif

/* flags, binary on/off for various outputs */
int cycle_detection  = 0;
int multiple_reduction_detection  = 0;
int debug_reduction  = 0;
int elaborate_output = 0;
int trace_reduction  = 0;
int reduction_timer  = 0;
int single_step      = 0;
int memory_info      = 0;
int count_reductions = 0;    /* produce a count of reductions */

int found_binary_command = 0;  /* lex and yacc coordinate on this */
int look_for_algorithm = 0;
int looking_for_filename = 0;

int reduction_timeout = 0;   /* how long to let a graph reduction run, seconds */
int max_reduction_count = 0; /* when non-zero, how many reductions to perform */

int prompting = 1;

/* Signal handling.  in_reduce_graph used to (a) handle
 * contrl-C interruptions (b) reduction-run-time timeouts,
 * (c) getting out of single-stepped graph reduction in reduce_graph()
 * (d) quitting when enough reductions have occurred.
 */
void sigint_handler(int signo);
sigjmp_buf in_reduce_graph;
int interpreter_interrupted = 0;  /* communicates with spine_stack.c code */
int reduction_interrupted = 0;    /* communicates with reset_node_allocation() */

void top_level_cleanup(int syntax_error_processing);

/* related to "output_command" non-terminal */
void set_output_command(enum OutputModifierCommands cmd, const char *setting);
void show_output_command(enum OutputModifierCommands cmd);
int *find_cmd_variable(enum OutputModifierCommands cmd);


void print_commands(void);

struct node *reduce_tree(struct node *root, enum graphReductionResult *r);
struct node *execute_bracket_abstraction(
	struct node *(*bafunc)(struct node *, struct node *),
	struct node *abstracted_var,
	struct node *root
);
float elapsed_time(struct timeval before, struct timeval after);
void usage(char *progname);

struct id_list {
	struct identifier_element *head;
	struct identifier_element *tail;
};

struct identifier_element {
	const char *identifier;
	struct identifier_element *next; 
};

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
 * For example: as_combinator[COMB_S], when set (default) causes
 * the lexer to treat "S" as an S-combinator.  When unset,
 * the lexer treats "S" as any other variable. This can interact
 * strangely with bracket abstraction, which assumes that its
 * own use of "S" (again, as example) always constitutes a combinator.
 */
int as_combinator[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

%}

%union{
	const char *identifier;
	const char *string_constant;
	int   numerical_constant;
	enum OutputModifierCommands command;
	struct node *node;
	enum primitiveName cn;
	struct node *(*bafunc)(struct node *, struct node *);
	struct id_list *idlist;
}


%token TK_EOL TK_COUNT_REDUCTIONS TK_SIZE TK_LENGTH
%token TK_LPAREN TK_RPAREN TK_LBRACK TK_RBRACK TK_COMMA
%token <identifier> TK_IDENTIFIER
%token <cn> TK_PRIMITIVE
%token <string_constant> FILE_NAME
%token <node> TK_REDUCE TK_TIMEOUT
%token <numerical_constant> NUMERICAL_CONSTANT
%token <identifier> TK_ALGORITHM_NAME
%token TK_DEF TK_LOAD TK_HELP TK_GRAPH
%token <command> TK_COMMAND
%token TK_MAX_COUNT TK_SET_BRACKET_ABSTRACTION  TK_EQUALS TK_PRINT TK_CANONICALIZE
%token <string_constant> BINARY_MODIFIER

%type <node> expression stmnt application term constant interpreter_command
%type <idlist> bracket_abstraction identifier_list
%type <bafunc> abstraction_algorithm
%type <command> output_command

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
			enum graphReductionResult grr;
			print_graph($1, 0, 0); 
			$$ = reduce_tree($1, &grr);
			if (INTERRUPT != grr)
			{
				int ignore;
				struct buffer *b = new_buffer(256);
				int redex_count = reduction_count($$->left, 0, &ignore, b);

				b->buffer[b->offset] = '\0';

				if (REDUCTION_LIMIT == grr)
					printf("Reduction limit\n");

				if (multiple_reduction_detection)
					printf("[%d] ", redex_count);
				printf("%s\n", b->buffer);

				delete_buffer(b);

				if (CYCLE_DETECTED != grr && REDUCTION_LIMIT != grr)
				{
					/* more built-in testing: if a redex occurs in the
				 	* term, it didn't get to normal form. */
					if (redex_count > 0) printf("Problem: %d reductions remaining, normal form not reached.\n", redex_count);
				}
			}
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
	: output_command BINARY_MODIFIER TK_EOL { found_binary_command = 0; set_output_command($1, $2); }
	| output_command TK_EOL { found_binary_command = 0; show_output_command($1); }
	| TK_HELP TK_EOL { print_commands(); }
	| TK_LOAD {looking_for_filename = 1; } FILE_NAME TK_EOL { looking_for_filename = 0; push_and_open($3); }
	| TK_TIMEOUT NUMERICAL_CONSTANT TK_EOL { reduction_timeout = $2; }
	| TK_TIMEOUT TK_EOL { printf("reduction runs for %d seconds\n", reduction_timeout); }
	| TK_MAX_COUNT NUMERICAL_CONSTANT TK_EOL { max_reduction_count = $2; }
	| TK_MAX_COUNT TK_EOL { printf("perform %d reductions at maximum\n", max_reduction_count); }
	| TK_SET_BRACKET_ABSTRACTION { look_for_algorithm = 1;} TK_ALGORITHM_NAME TK_EOL
		{
			default_bracket_abstraction = determine_bracket_abstraction($3);
			look_for_algorithm = 0;
		}
	| expression TK_EQUALS expression TK_EOL
		{
			if (equivalent_graphs($1, $3))
				printf("Equivalent\n");
			else
				printf("Not equivalent\n");

			++$1->refcnt;
			++$3->refcnt;
			free_node($1);
			free_node($3);
			$1 = $3 = NULL;
		}
	| TK_PRINT expression TK_EOL {
			printf("Literal: ");
			if (multiple_reduction_detection)
			{
				int ignore;
				struct buffer *b = new_buffer(256);
				int n = reduction_count($2, 0, &ignore, b);
				b->buffer[b->offset] = '\0';
				printf("[%d] %s\n", n, b->buffer);
				delete_buffer(b);
			} else
				print_graph($2, 0, 0); 
			++$2->refcnt;
			free_node($2);
		}
	| TK_CANONICALIZE expression TK_EOL {
			char *buf = NULL;
			printf("Canonically: ");
			buf = canonicalize_graph($2); 
			printf("%s\n", buf);
			++$2->refcnt;
			free_node($2);
			free(buf);
		}
	| TK_COUNT_REDUCTIONS expression TK_EOL {
			int ignore;
			int cnt = reduction_count($2, 0, &ignore, NULL);
			printf("Found %d possible reductions\n", cnt);
			++$2->refcnt;
			free_node($2);
		}
	| TK_LENGTH expression TK_EOL {
			int cnt = node_count($2, 0);  /* only count atoms. */
			printf("%d atoms\n", cnt);
			++$2->refcnt;
			free_node($2);
		}
	| TK_SIZE expression TK_EOL {
			int cnt = node_count($2, 1);  /* count interior nodes, too. */
			printf("%d nodes\n", cnt);
			++$2->refcnt;
			free_node($2);
		}
	;

/* Interpreter commands like "timer", "trace", "debug",
 * that take "on" or "off" as arguments, or, when called
 * without an argument, print their current status. */
output_command
	: TK_COMMAND { found_binary_command = 1; $$ = $1; }
	;

expression
	: application          { $$ = $1; }
	| term                 { $$ = $1; }
	| TK_REDUCE expression
		{
			struct node *tmp;
			enum graphReductionResult r;
			tmp = reduce_tree($2, &r);  /* XXX - need to check r */
			--tmp->left->refcnt;
			$$ = tmp->left;
			tmp->left = NULL;
			free_node(tmp);
		}
	| bracket_abstraction abstraction_algorithm expression
		{
			struct node *abstracted_expression = NULL, *tmp;
			struct identifier_element *curr, *head;

			look_for_algorithm = 0;

			curr = $1->tail;
			head = $1->head;

			tmp = $3;

			do {
				struct identifier_element *e = NULL;
				struct node *n = new_term(curr->identifier);

				abstracted_expression = execute_bracket_abstraction($2, n, tmp);

				++n->refcnt;
				free_node(n);

				++tmp->refcnt;
				free_node(tmp);

				tmp = abstracted_expression;

				for (e = head; e && e->next != curr; e = e->next)
					;

				free(curr);
				curr = e;
				if (curr) curr->next = NULL;

			} while (curr);

			free($1);

			$$ = abstracted_expression;
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
	: TK_LBRACK identifier_list TK_RBRACK
		{ $$ = $2; look_for_algorithm = 1; }
	;

identifier_list
	: TK_IDENTIFIER
		{
			struct identifier_element *ide;
			$$ = malloc(sizeof(struct id_list));
			ide = malloc(sizeof(struct identifier_element));
			ide->identifier = $1;
			ide->next = NULL;
			$$->head = ide;
			$$->tail = ide;
		}
	| identifier_list TK_COMMA TK_IDENTIFIER
		{
			struct identifier_element *ide;
			ide = malloc(sizeof(struct identifier_element));
			ide->identifier = $3;
			ide->next = NULL;
			$1->tail->next = ide;
			$1->tail = ide;
			$$ = $1;
		}
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
	int c, r, x;
	struct filename_node *p, *load_files = NULL, *load_tail = NULL;
	struct hashtable *h = init_hashtable(64, 10);
	struct node *(*dba)(struct node *, struct node *);

	setup_abbreviation_table(h);
	setup_atom_table(h);

	while (-1 != (c = getopt(ac, av, "deL:mN:pstT:cC:B:x")))
	{
		switch (c)
		{
		case 'c':
			cycle_detection = 1;
			break;
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
			x = COMB_NONE;
			switch(optarg[0])
			{
			case 'S': x = COMB_S; break;
			case 'K': x = COMB_K; break;
			case 'I': x = COMB_I; break;
			case 'B': x = COMB_B; break;
			case 'C': x = COMB_C; break;
			case 'W': x = COMB_W; break;
			case 'M': x = COMB_M; break;
			case 'T': x = COMB_T; break;
			case 'J': x = COMB_J; break;
			default:
				fprintf(stderr, "Unknown primitive combinator \"%s\"\n", optarg);
				usage(av[0]);
				break;
			}
			as_combinator[x] = 0;
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
	if (cycle_detection) free_detection();
	reset_yyin();

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
reduce_tree(struct node *real_root, enum graphReductionResult *grr)
{
	void (*old_sigint_handler)(int);
	void (*old_sigalm_handler)(int);
	struct timeval before, after;
	int cc;
	struct node *new_root = new_application(real_root, new_application(NULL, NULL));

	/* new_root - points to a "dummy" node, necessary for I and
	 * K reductions, if the expression is something like "I x" or
	 * K a b. It has a dummy right-child so as to avoid continually
	 * testing for a missing right-hand-child node.
	 */
	++new_root->refcnt;
	MARK_RIGHT_BRANCH_TRAVERSED(new_root);

	old_sigint_handler = signal(SIGINT, sigint_handler);
	old_sigalm_handler = signal(SIGALRM, sigint_handler);

	if (!(cc = sigsetjmp(in_reduce_graph, 1)))
	{
		alarm(reduction_timeout);
		gettimeofday(&before, NULL);
		*grr = reduce_graph(new_root);
		alarm(0);
		gettimeofday(&after, NULL);
	} else {
		const char *phrase = "Unset";
		alarm(0);
		gettimeofday(&after, NULL);
		*grr = INTERRUPT;
		switch (cc)
		{
		case 1:
			phrase = "Interrupt";
			if (cycle_detection) reset_detection();
			break;
		case 2:
			phrase = "Timeout";
			if (cycle_detection) reset_detection();
			break;
		case 3:
			phrase = "Terminated";
			if (cycle_detection) reset_detection();
			break;
		default:
			phrase = "Unknown";
			break;
		}
		printf("%s\n", phrase);
		++interpreter_interrupted;
		reduction_interrupted = 1;
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
		"-c             Enable reduction cycle detection\n"
		"-d             Debug reductions\n"
		"-e             Elaborate output\n"
		"-L  filename   Load and interpret a filenamed filename\n"
		"-m             on exit, print memory usage summary\n"
		"-N number      Perform up to number reductions\n"
		"-p             Don't print prompts\n"
		"-s             Single-step reductions\n"
		"-T number      Evaluate an expression for up to number seconds\n"
		"-t             trace reductions\n"
		"-C combinator  Treat combinator as a non-primitive. Combinator one of S, K, I, B, C, W, M, T\n"
		"-B algorithm   Use algorithm as default for bracket abstraction.  One of curry, tromp, grz, btmk, or turner\n"
		""
	);
}

static int *command_variables[] = {
	&debug_reduction,
	&elaborate_output,
	&trace_reduction,
	&reduction_timer,
	&single_step,
	&cycle_detection,
	&multiple_reduction_detection
};

int *
find_cmd_variable(enum OutputModifierCommands cmd)
{
	return command_variables[cmd];
}

void
set_output_command(enum OutputModifierCommands cmd, const char *setting)
{
	*(find_cmd_variable(cmd)) = strcmp(setting, "on")? 0: 1;
}

const static char *command_phrases[] = {
	"debugging output",
	"elaborate debugging output",
	"tracing",
	"reduction timer",
	"single-stepping",
	"reduction cycle detection",
	"non-head reduction detection"
};

void
show_output_command(enum OutputModifierCommands cmd)
{
	printf("%s %s\n", command_phrases[cmd], *(find_cmd_variable(cmd))? "on": "off");
}

void
print_commands(void)
{
	printf("Help Section\n");
}

