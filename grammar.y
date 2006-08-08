%{
#include <stdio.h>

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <graph.h>

int debug_reduction = 0;
int elaborate_output = 0;
%}

%union{
	const char *identifier;
	struct node *node;
}


%token TK_EOL
%token TK_LPAREN TK_RPAREN 
%token TK_IDENTIFIER
%token TK_PRIMITIVE

%%

program
	: stmnt 
	| program stmnt 
	| error  /* magic token - yacc unwinds to here on syntax error */
	;

stmnt
	: expression TK_EOL
		{
			print_graph($1.node); 
			reduce_graph($1.node, NULL, NULL, NULL);
			if ($1.node && $1.node->typ == APPLICATION)
				reduce_graph($1.node, NULL, NULL, NULL);
			print_graph($1.node);
			free_node($1.node);
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
	| TK_IDENTIFIER                    { $$.node = new_combinator($1.identifier); }
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

	while (-1 != (c = getopt(ac, av, "de")))
	{
		switch (c)
		{
		case 'd':
			printf("Turning on debug_reduction\n");
			debug_reduction = 1;
			break;
		case 'e':
			printf("Turning on elaborate_output\n");
			elaborate_output = 1;
			break;
		}
	}

	setup_atom_table(h);

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

