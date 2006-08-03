%{
/*
	Copyright (C) 2006, Bruce Ediger

    This file is part of lc.

    lc is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    lc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lc; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <hashtable.h>
#include <atom.h>

#include "y.tab.h"

%}

%%

\#.*$		{ return TK_EOL; }
\n		    { return TK_EOL; }
\(		    { return TK_LPAREN; }
\)		    { return TK_RPAREN; }
[SKICB] { yylval.identifier = Atom_string(yytext); return TK_PRIMITIVE; }
[a-zA-Z][a-zA-Z_0-9]* { yylval.identifier = Atom_string(yytext); return TK_IDENTIFIER; }
.		    { /* Just eat it. */ }

%%

int
yywrap()
{
	return 1;
}
