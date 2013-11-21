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
#include "utils.h"

std::shared_ptr<NamedValueList> _parsed_data = nullptr; // Documentation is in scanner_funcs.h, since this file is not being scanned.
%}

%token PAR_OPEN PAR_CLOSE CURLY_OPEN CURLY_CLOSE
%token COLON SEMICOLON COMMA
%token<line> MINUS BITSET_KW IMPORT_KW AND PIPE XOR SHL SHR ADD MUL DIV MOD NEG
%token<chars> STRING
%token<number> NUMBER
%token<chars> IDENTIFIER
%type<expr> Factor Expression Term MulTerm AddTerm ShiftTerm
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
	$$ = std::make_shared<ExpressionList>();
	$$->exprs.push_back($1);
}
               ;
ExpressionList : ExpressionList COMMA Expression {
	$$ = $1;
	$$->exprs.push_back($3);
}
               ;
/* Logical operators. */
Expression : ShiftTerm {
	$$ = $1;
}
           ;

Expression : Expression AND ShiftTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '&', $3);
}
           ;

Expression : Expression PIPE ShiftTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '|', $3);
}
           ;

Expression : Expression XOR ShiftTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '^', $3);
}
           ;

/* Shift operators. */
ShiftTerm : AddTerm {
	$$ = $1;
}
          ;

ShiftTerm : ShiftTerm SHL AddTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '<', $3);
}
          ;

ShiftTerm : ShiftTerm SHR AddTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '>', $3);
}
          ;

/* Add/subtract operators. */
AddTerm : MulTerm {
	$$ = $1;
}
        ;

AddTerm : AddTerm ADD MulTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '+', $3);
}
           ;

AddTerm : AddTerm MINUS MulTerm {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '-', $3);
}
           ;

/* Multiply operators. */
MulTerm : Term {
	$$ = $1;
}
        ;

MulTerm : MulTerm MUL Term {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '*', $3);
}
        ;

MulTerm : MulTerm DIV Term {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '/', $3);
}
        ;

MulTerm : MulTerm MOD Term {
	Position pos(filename, $2);
	$$ = std::make_shared<BinaryOperator>(pos, $1, '%', $3);
}
        ;

/* Unary operator. */
Term : Factor {
	$$ = $1;
}
     ;

Term : MINUS Factor {
	Position pos(filename, $1);
	$$ = std::make_shared<UnaryOperator>(pos, '-', $2);
}
     ;

Term : NEG Factor {
	Position pos(filename, $1);
	$$ = std::make_shared<UnaryOperator>(pos, '~', $2);
}
     ;

/* Elementary expressions. */
Factor : STRING {
	Position pos(filename, $1.line);
	$$ = std::make_shared<StringLiteral>(pos, $1.value);
}
       ;

Factor : NUMBER {
	Position pos(filename, $1.line);
	$$ = std::make_shared<NumberLiteral>(pos, $1.value);
}
       ;

Factor : IDENTIFIER {
	Position pos(filename, $1.line);
	CheckIsSingleName($1.value, pos);
	$$ = std::make_shared<IdentifierLiteral>(pos, $1.value);
}
       ;

Factor : BITSET_KW PAR_OPEN PAR_CLOSE {
	Position pos(filename, $1);
	$$ = std::make_shared<BitSet>(pos, nullptr);
}
       ;

Factor : BITSET_KW PAR_OPEN ExpressionList PAR_CLOSE {
	Position pos(filename, $1);
	$$ = std::make_shared<BitSet>(pos, $3);
}
       ;

Factor : PAR_OPEN Expression PAR_CLOSE {
	$$ = $2;
}
       ;

NamedValueList : /* Empty */ {
	$$ = std::make_shared<NamedValueList>();
}
               ;

NamedValueList : NamedValueList NamedValue {
	$$ = $1;
	$$->values.push_back($2);
}
               ;

NamedValue : Group {
	$$ = std::make_shared<NamedValue>(nullptr, $1);
}
           ;

NamedValue : IDENTIFIER COLON Group {
	Position pos(filename, $1.line);
	CheckIsSingleName($1.value, pos);
	std::shared_ptr<Name> name = std::make_shared<SingleName>(pos, $1.value);
	$$ = std::make_shared<NamedValue>(name, $3);
}
           ;

NamedValue : IDENTIFIER COLON Expression SEMICOLON {
	Position pos(filename, $1.line);
	CheckIsSingleName($1.value, pos);
	std::shared_ptr<Name> name = std::make_shared<SingleName>(pos, $1.value);
	std::shared_ptr<Group> group = std::make_shared<ExpressionGroup>($3);
	$$ = std::make_shared<NamedValue>(name, group);
}
           ;

NamedValue : IdentifierTable COLON Group {
	$$ = std::make_shared<NamedValue>($1, $3);
}
           ;

NamedValue : IMPORT_KW STRING SEMICOLON {
	Position pos(filename, $1);
	$$ = std::make_shared<ImportValue>(pos, $2.value);
}
           ;

IdentifierTable : PAR_OPEN IdentifierRows PAR_CLOSE {
	$$ = $2;
}
                ;

IdentifierRows : IdentifierRow {
	$$ = std::make_shared<NameTable>();
	$$->rows.push_back($1);
}
               ;

IdentifierRows : IdentifierRows PIPE IdentifierRow {
	$$ = $1;
	$$->rows.push_back($3);
}
               ;

IdentifierRow : IDENTIFIER {
	$$ = std::make_shared<NameRow>();
	Position pos(filename, $1.line);
	std::shared_ptr<IdentifierLine> il = std::make_shared<IdentifierLine>(pos, $1.value);
	$$->identifiers.push_back(il);
}
              ;

IdentifierRow : IdentifierRow COMMA IDENTIFIER {
	$$ = $1;
	Position pos(filename, $3.line);
	std::shared_ptr<IdentifierLine> il = std::make_shared<IdentifierLine>(pos, $3.value);
	$$->identifiers.push_back(il);
}
              ;

Group : IDENTIFIER CURLY_OPEN NamedValueList CURLY_CLOSE {
	Position pos(filename, $1.line);
	CheckIsSingleName($1.value, pos);
	$$ = std::make_shared<NodeGroup>(pos, $1.value, nullptr, $3);
}
      ;

Group : IDENTIFIER PAR_OPEN ExpressionList PAR_CLOSE CURLY_OPEN NamedValueList CURLY_CLOSE {
	Position pos(filename, $1.line);
	CheckIsSingleName($1.value, pos);
	$$ = std::make_shared<NodeGroup>(pos, $1.value, $3, $6);
}
      ;

%%

