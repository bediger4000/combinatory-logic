%{
#include <stdio.h>
#include <unistd.h>  /* getopt() */

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <graph.h>
#include <abbreviations.h>

int debug_reduction = 0;
int elaborate_output = 0;
int trace_reduction = 0;
%}

%union{
	const char *identifier;
	struct node *node;
}


%token TK_EOL
%token TK_LPAREN TK_RPAREN 
%token TK_IDENTIFIER
%token TK_PRIMITIVE
%token TK_DEF

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
			int affected, looping = 1;
			print_graph($1.node); 
			while (looping)
			{
				affected = reduce_graph($1.node, NULL, NULL, NULL, 0);
				if (0 != affected)
					looping = 0;
			}
			print_graph($1.node);
		}
	| TK_DEF TK_IDENTIFIER expression TK_EOL
		{
			struct node *prev = abbreviation_add($2.identifier, $3.node);
			if (prev) free_graph(prev);
		}
	| TK_EOL  /* blank lines */
	;

expression
	: application
	| term
	;

application
	: term term        { $$.node = new_application($1.node, $2.node); }
	| application term { $$.node = new_application($1.node, $2.node); }
	;

term
	: constant                         { $$ = $1; }
	| TK_IDENTIFIER
		{
			$$.node = abbreviation_lookup($1.identifier);
			if (!$$.node)
				$$.node = new_combinator($1.identifier);
		}
	| TK_LPAREN application TK_RPAREN  { $$ = $2; }
	;

constant
	: TK_PRIMITIVE  { $$.node = new_combinator($1.identifier); }
	;

%%

int
main(int ac, char **av)
{
	int c, r;
	struct hashtable *h = init_hashtable(64, 10);
	extern int yyparse();

	while (-1 != (c = getopt(ac, av, "det")))
	{
		switch (c)
		{
		case 'd':
			debug_reduction = 1;
			break;
		case 'e':
			elaborate_output = 1;
			break;
		case 't':
			trace_reduction = 1;
			break;
		}
	}

	setup_abbreviation_table(h);
	setup_atom_table(h);
	init_node_allocation();

	r =  yyparse();

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

