%{
#include <stdio.h>
#include <errno.h>    /* errno */
#include <string.h>   /* strerror() */
#include <stdlib.h>   /* malloc(), free() */
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

/* flags, binary on/off for various outputs */
int debug_reduction  = 0;
int elaborate_output = 0;
int trace_reduction  = 0;
int reduction_timer  = 0;

int reduction_timeout = 0;  /* how long to let a series of reductions run, seconds */

void sigint_handler(int signo);
jmp_buf in_reduce_graph;

void reduce_tree(struct node *root);
float elapsed_time(struct timeval before, struct timeval after);

struct filename_node {
	const char *filename;
	struct filename_node *next;
};

/* from lex.l */
extern void set_yyin_stdin(void); 
extern void set_yyin(const char *filename); 
extern void reset_yyin(void);
extern void  push_and_open(const char *filename);

%}

%union{
	const char *identifier;
	const char *string_constant;
	struct node *node;
	enum combinatorName cn;
}


%token TK_EOL
%token TK_LPAREN TK_RPAREN 
%token <identifier> TK_IDENTIFIER
%token <cn> TK_PRIMITIVE
%token <string_constant> STRING_CONSTANT
%token TK_DEF TK_TIME TK_REDUCE TK_LOAD

%type <node> expression stmnt application term constant

%%

program
	: stmnt { reset_node_allocation(); }
	| program stmnt  { reset_node_allocation(); }
	| error  /* magic token - yacc unwinds to here on syntax error */
		{ reset_node_allocation(); }
	;

stmnt
	: expression TK_EOL
		{
			print_graph($1); 
			reduce_tree($1);
			print_graph($1);
		}
	| TK_DEF TK_IDENTIFIER expression TK_EOL
		{
			struct node *prev = abbreviation_add($2, $3);
			if (prev) free_graph(prev);
		}
	| TK_TIME TK_EOL { reduction_timer = 1; }
	| TK_LOAD STRING_CONSTANT TK_EOL { push_and_open($2); }
	| TK_EOL  /* blank lines */
	;

expression
	: application          { $$ = $1; }
	| term                 { $$ = $1; }
	| TK_REDUCE expression { reduce_tree($2); $$ = $2; }
	;

application
	: term term        { $$ = new_application($1, $2); }
	| application term { $$ = new_application($1, $2); }
	;

term
	: constant                         { $$ = $1; }
	| TK_IDENTIFIER
		{
			$$ = abbreviation_lookup($1);
			if (!$$)
				$$ = new_term($1);
		}
	| TK_LPAREN application TK_RPAREN  { $$ = $2; }
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
	init_node_allocation();

	while (-1 != (c = getopt(ac, av, "deL:t")))
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
		case 't':
			trace_reduction = 1;
			break;
		}
	}

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

	free_all_nodes();
	free_hashtable(h);

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
void
reduce_tree(struct node *root)
{
	void (*old_sigint_handler)(int);
	void (*old_sigalm_handler)(int);
	struct timeval before, after;
	int cc;

	old_sigint_handler = signal(SIGINT, sigint_handler);
	old_sigalm_handler = signal(SIGALRM, sigint_handler);

	if (!(cc = setjmp(in_reduce_graph)))
	{
		int affected, looping = 1;

		alarm(reduction_timeout);
		gettimeofday(&before, NULL);
		while (looping)
		{
			affected = reduce_graph(root, NULL, NULL, NULL, 0);
			if (0 != affected)
				looping = 0;
		}
		alarm(0);
		gettimeofday(&after, NULL);
	} else {
		alarm(0);
		gettimeofday(&after, NULL);
		printf("%s\n", cc == 1? "Interrupt": "Timeout");
	}

	signal(SIGINT, old_sigint_handler);
	signal(SIGALRM, old_sigalm_handler);

	if (reduction_timer)
		printf("elapsed time %.3f seconds\n", elapsed_time(before, after));
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
