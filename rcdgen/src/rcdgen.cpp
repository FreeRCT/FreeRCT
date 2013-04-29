/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rcdgen.cpp Main program of rcdgen. */

#include "../../src/stdafx.h"
#include "../../src/getoptdata.h"
#include "scanner_funcs.h"
#include "ast.h"
#include "nodes.h"
#include "file_writing.h"
#include <cstdarg>

/**
 * Error handling for fatal non-user errors.
 * @param s the string to print.
 * @note \b Never returns.
 */
void CDECL error(const char *s, ...)
{
	va_list va;

	va_start(va, s);
	vfprintf(stderr, s, va);
	va_end(va);

	exit(1);
}

/** Command-line options of the program. */
static const OptionData _options[] = {
	GETOPT_NOVAL('h', "--help"),
	GETOPT_VALUE('d', "--header"),
	GETOPT_VALUE('c', "--code"),
	GETOPT_VALUE('b', "--base"),
	GETOPT_VALUE('p', "--prefix"),
	GETOPT_END()
};

/** Output online help. */
static void PrintUsage()
{
	printf("Usage: rcdgen options | FILE\n");
	printf("This program has three modes of operation, depending on the command line.\n");
	printf("1. Print online help:\n");
	printf("\n");
	printf("\trcdgen -h | --help\n");
	printf("\n");
	printf("2. Generate an RCD data file from an input file or stdin:\n");
	printf("\n");
	printf("\trcdgen [FILE]\n");
	printf("\n");
	printf("3. Generate .h and/or .cpp files for strings of the program:\n");
	printf("\n");
	printf("\t rcdgen --prefix PREFIX [--base BASE] [--header HEADER] [--code CODE]\n");
	printf("\n");
	printf("   PREFIX is the kind of strings you want to generate.\n");
	printf("          Accepted values are \"GUI\" and \"SHOPS\".\n");
	printf("   BASE   is the first value. If omitted, it is \"0\".\n");
	printf("   HEADER is the name of the generated .h file (if specified).\n");
	printf("   CODE   is the name of the generated .cpp file (if specified).\n");
	printf("\n");
}

/**
 * The main program of rcdgen.
 * @param argc Number of argument given to the program.
 * @param argv Argument texts.
 * @return Exit code.
 */
int main(int argc, char *argv[])
{
	GetOptData opt_data(argc - 1, argv + 1, _options);
	const char *header = NULL;
	const char *code = NULL;
	const char *prefix = NULL;
	const char *base = "0";

	int opt_id;
	do {
		opt_id = opt_data.GetOpt();
		switch (opt_id) {
			case 'h':
				PrintUsage();
				exit(0);

			case 'd':
				header = opt_data.opt;
				break;

			case 'c':
				code = opt_data.opt;
				break;

			case 'b':
				base = opt_data.opt;
				break;

			case 'p':
				prefix = opt_data.opt;
				break;

			case -1:
				break;

			default:
				/* -2 or some other weird thing happened. */
				fprintf(stderr, "ERROR while processing the command-line\n");
				exit(1);
		}
	} while (opt_id != -1);

	if (prefix != NULL) {
		/* Prefix specified, generate strings. */
		if (header != NULL) GenerateStringsHeaderFile(prefix, base, header);
		if (code != NULL) GenerateStringsCodeFile(prefix, base, code);
		exit(0);
	}

	/* No --prefix, generate an RCD file. */
	if (header != NULL) printf("Warning: --header option is not used.\n");
	if (code != NULL) printf("Warning: --code option is not used.\n");

	if (opt_data.numleft > 1) {
		fprintf(stderr, "Error: Too many arguments (use -h or --help for online help)\n");
		exit(1);
	}

	/* Phase 1: Parse the input file. */
	NamedValueList *nvs = LoadFile((opt_data.numleft == 1) ? opt_data.argv[0] : NULL);

	/* Phase 2: Check and simplify the loaded input. */
	FileNodeList *file_nodes = CheckTree(nvs);
	delete nvs;

	/* Phase 3: Construct output files. */
	for (std::list<FileNode *>::iterator iter = file_nodes->files.begin(); iter != file_nodes->files.end(); iter++) {
		FileWriter fw;
		(*iter)->Write(&fw);
		fw.WriteFile((*iter)->file_name);
	}

	delete file_nodes;
	exit(0);
}
