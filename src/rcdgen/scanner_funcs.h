/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scanner_funcs.h Declarations for the interface between the scanner and the parser. */

#ifndef SCANNER_FUNCS_H
#define SCANNER_FUNCS_H

#include <string>
#include "ast.h"
#include "../stdafx.h"

extern int line; ///< Line number of the input file being scanned.
extern std::string filename; ///< Name of the file being parsed.
extern std::string text; ///< Temporary storage for a string.

int yylex();   ///< Generated scanner function.
int yyparse(); ///< Generated parser function.

extern FILE *yyin; ///< Input stream of the scanner. See also flex(1).
void yyerror(const char *message);

/** Structure to communicate values from the scanner to the parser. */
struct YyStruct {
	int line; ///< Line number of the token.
	struct {
		int line;        ///< Line number of the token.
		long long value; ///< Number stored.
	} number; ///< Data while communicating a NUMBER token.
	struct {
		int line;    ///< Line number of the token.
		std::string value; ///< Characters stored.
	} chars; ///< Data while communicating an IDENTIFIER or STRING token.
	std::shared_ptr<Expression> expr;         ///< %Expression to pass on.
	std::shared_ptr<ExpressionList> exprlist; ///< Expression list to pass on.
	std::shared_ptr<NameTable> iden_table;    ///< 2D table with identifiers to pass on.
	std::shared_ptr<NameRow> iden_row;        ///< Row of identifiers to pass on.
	std::shared_ptr<Group> group;             ///< %Group to pass on.
	std::shared_ptr<BaseNamedValue> value;    ///< A named value to pass on.
	std::shared_ptr<NamedValueList> values;   ///< Sequence of named values to pass on.
};

/** Macro defining the interface type between the scanner and parser. See also flex(1). */
#define YYSTYPE YyStruct

extern std::shared_ptr<NamedValueList> _parsed_data; ///< Result of parsing the input file.

/**
 * Setup the scanner for the new file.
 * @param fname Name of the file being parsed.
 * @param new_file New input stream to switch to.
 */
void SetupScanner(const char *fname, FILE *new_file);

#endif
