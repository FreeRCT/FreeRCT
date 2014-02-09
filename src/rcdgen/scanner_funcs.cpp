/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scanner_funcs.cpp Scanner / parser interface functions. */

#include "../stdafx.h"
#include <cassert>
#include <cstdio>
#include "scanner_funcs.h"

/**
 * Report an error message to the user and abort.
 * @param message Message to report.
 */
void yyerror(const char *message)
{
	fprintf(stderr, "Parsing failed near line %d: %s\n", line, message);
	exit(1);
}
