%{
%}

%union{
}


%token TK_EOL
%token TK_LPAREN TK_RPAREN 
%token S K I

%%

program
	: stmnt 
	| program stmnt 
	| error  /* magic token - yacc unwinds to here on syntax error */
	;

stmnt
	: term TK_EOL
	| TK_EOL
	;

term
	: constant
	| TK_LPAREN term term TK_RPAREN
	;

constant
	: S
	| K
	| I
	;

%%

int
main(int ac, char **av)
{
	extern int yyparse();
	int r =  yyparse();

	return r;
}

int
yyerror(char *s1)
{
    fprintf(stderr, "%s\n", s1);

    return 0;
}

