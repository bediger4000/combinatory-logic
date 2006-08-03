%{
#include <stdio.h>

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <graph.h>
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
			struct node *d1, *d2, *d3;
			d1 = d2 = d3 = NULL;
			print_graph($1.node); 
			reduce_graph(&($1.node), &d1, &d2, &d3);
			if ($1.node && $1.node->typ == APPLICATION)
			{
				d1 = d2 = d3 = NULL;
				reduce_graph(&($1.node), &d1, &d2, &d3);
			}
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
	int r;
	struct hashtable *h = init_hashtable(64, 10);
	extern int yyparse();

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

