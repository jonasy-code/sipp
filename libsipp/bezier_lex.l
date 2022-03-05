%{
/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Equivalent Software HB  1992
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

/**
 ** bezier_lex.l - Lex source for reading bezier descriptions.
 **/

#include "sipp.h"
#include "bezier.h"

extern int     atoi();
extern double  atof();

extern Tokenval  tokenval;
extern FILE     *bezier_file;
%}
%%

"bezier_patches:"       {return PATCHES;}
"bezier_curves:"        {return CURVES;}
"vertices:"             {return NVERTICES;}
"patches:"              {return NPATCHES;}
"curves:"               {return NCURVES;}
"vertex_list:"          {return VERTEX_LIST;}
"patch_list:"           {return PATCH_LIST;}
"curve_list:"           {return CURVE_LIST;}

-?[0-9]+                {tokenval.intval = atoi(yytext);
                         return INTEGER;
                        }
-?[0-9]+"."[0-9]*([eE][-+]?[0-9]+)?  {tokenval.floatval = atof(yytext);
                                      return FLOAT;
                                     }

[ \n\t]                 {}
#.*$                    { /* This is a comment */ }
.                       { return yytext[0]; /* We found something wrong */ }

%%

#ifndef FLEX_SCANNER
int
yywrap()
{
    return 1;
}
#endif


