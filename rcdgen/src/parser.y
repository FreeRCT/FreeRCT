/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

%{
#include <cstdio>
#include "scanner_funcs.h"
#include "ast.h"

NamedValueList *_parsed_data = NULL; ///< Result of parsing the input file.
%}

%token PAR_OPEN PAR_CLOSE CURLY_OPEN CURLY_CLOSE
%token COLON PIPE SEMICOLON COMMA
%token<line> MINUS BITSET_KW IMPORT_KW
%token<chars> STRING
%token<number> NUMBER
%token<chars> IDENTIFIER
%type<expr> Factor Expression
%type<exprlist> ExpressionList
%type<iden_table> IdentifierRows IdentifierTable
%type<iden_row> IdentifierRow
%type<group> Group
%type<value> NamedValue
%type<values> NamedValueList

%start Program

%%

Program : NamedValueList {
	_parsed_data = $1;
}
        ;

ExpressionList : Expression {
	$$ = new ExpressionList;
	$$->exprs.push_back($1);
}
               ;
ExpressionList : ExpressionList COMMA Expression {
	$$ = $1;
	$$->exprs.push_back($3);
}
               ;

Expression : Factor {
	$$ = $1;
}
           ;
Expression : MINUS Factor {
	Position pos(filename, $1);
	$$ = new UnaryOperator(pos, '-', $2);
}
           ;

Factor : STRING {
	Position pos(filename, $1.line);
	$$ = new StringLiteral(pos, $1.value);
}
       ;

Factor : NUMBER {
	Position pos(filename, $1.line);
	$$ = new NumberLiteral(pos, $1.value);
}
       ;

Factor : IDENTIFIER {
	Position pos(filename, $1.line);
	$$ = new IdentifierLiteral(pos, $1.value);
}
       ;

Factor : BITSET_KW PAR_OPEN PAR_CLOSE {
	Position pos(filename, $1);
	$$ = new BitSet(pos, NULL);
}
       ;

Factor : BITSET_KW PAR_OPEN ExpressionList PAR_CLOSE {
	Position pos(filename, $1);
	$$ = new BitSet(pos, $3);
}
       ;

Factor : PAR_OPEN Expression PAR_CLOSE {
	$$ = $2;
}
       ;

NamedValueList : /* Empty */ {
	$$ = new NamedValueList;
}
               ;

NamedValueList : NamedValueList NamedValue {
	$$ = $1;
	$$->values.push_back($2);
}
               ;

NamedValue : Group {
	$$ = new NamedValue(NULL, $1);
}
           ;

NamedValue : IDENTIFIER COLON Group {
	Position pos(filename, $1.line);
	Name *name = new SingleName(pos, $1.value);
	$$ = new NamedValue(name, $3);
}
           ;

NamedValue : IDENTIFIER COLON Expression SEMICOLON {
	Position pos(filename, $1.line);
	Name *name = new SingleName(pos, $1.value);
	Group *group = new ExpressionGroup($3);
	$$ = new NamedValue(name, group);
}
           ;

NamedValue : IdentifierTable COLON Group {
	$$ = new NamedValue($1, $3);
}
           ;

NamedValue : IMPORT_KW STRING SEMICOLON {
	Position pos(filename, $1);
	$$ = new ImportValue(pos, $2.value);
}
           ;

IdentifierTable : PAR_OPEN IdentifierRows PAR_CLOSE {
	$$ = $2;
}
                ;

IdentifierRows : IdentifierRow {
	$$ = new NameTable();
	$$->rows.push_back($1);
}
               ;

IdentifierRows : IdentifierRows PIPE IdentifierRow {
	$$ = $1;
	$$->rows.push_back($3);
}
               ;

IdentifierRow : IDENTIFIER {
	$$ = new NameRow();
	Position pos(filename, $1.line);
	IdentifierLine *il = new IdentifierLine(pos, $1.value);
	$$->identifiers.push_back(il);
}
              ;

IdentifierRow : IdentifierRow COMMA IDENTIFIER {
	$$ = $1;
	Position pos(filename, $3.line);
	IdentifierLine *il = new IdentifierLine(pos, $3.value);
	$$->identifiers.push_back(il);
}
              ;

Group : IDENTIFIER CURLY_OPEN NamedValueList CURLY_CLOSE {
	Position pos(filename, $1.line);
	$$ = new NodeGroup(pos, $1.value, NULL, $3);
}
      ;

Group : IDENTIFIER PAR_OPEN ExpressionList PAR_CLOSE CURLY_OPEN NamedValueList CURLY_CLOSE {
	Position pos(filename, $1.line);
	$$ = new NodeGroup(pos, $1.value, $3, $6);
}
      ;

%%

