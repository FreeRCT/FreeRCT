/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rcdgen.cpp Main program of rcdgen. */

#include "../stdafx.h"
#include "../getoptdata.h"
#include "../string_func.h"
#include "scanner_funcs.h"
#include "ast.h"
#include "nodes.h"
#include "string_storage.h"
#include "file_writing.h"
#include <cstdarg>

/**
 * Error handling for fatal non-user errors.
 * @param str the string to print.
 * @note \b Never returns.
 */
void error(const char *str, ...)
{
	va_list va;

	va_start(va, str);
	vfprintf(stderr, str, va);
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
	printf("Usage: rcdgen options | FILE ...\n");
	printf("This program has three modes of operation, depending on the command line.\n");
	printf("1. Print online help:\n");
	printf("\n");
	printf("\trcdgen -h | --help\n");
	printf("\n");
	printf("2. Generate RCD data files from input files or stdin:\n");
	printf("\n");
	printf("\trcdgen [FILE ...]\n");
	printf("\n");
	printf("3. Generate .h and/or .cpp files for strings of the program:\n");
	printf("\n");
	printf("\t rcdgen --prefix PREFIX [--base BASE] [--header HEADER] [--code CODE]\n");
	printf("\n");
	printf("   PREFIX is the kind of strings you want to generate.\n");
	printf("          Accepted values are \"GUI\", \"SHOPS\", \"GENTLE_THRILL_RIDES\", and \"COASTERS\".\n");
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
	const char *header = nullptr;
	const char *code = nullptr;
	const char *prefix = nullptr;
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

	if (prefix != nullptr) {
		/* Prefix specified, generate strings. */
		if (header != nullptr) GenerateStringsHeaderFile(prefix, base, header);
		if (code != nullptr) GenerateStringsCodeFile(prefix, code);
		exit(0);
	}

	/* No --prefix, generate an RCD file. */
	if (header != nullptr) printf("Warning: --header option is not used.\n");
	if (code != nullptr) printf("Warning: --code option is not used.\n");

	int num_files = std::max(1, opt_data.numleft);
	for (int i = 0; i < num_files; i++) {
		assert(i < opt_data.numleft);
		if (StrEndsWith(opt_data.argv[i], ".yml", false) || StrEndsWith(opt_data.argv[i], ".yaml", false)) {
			/* Translations. Update the strings storage and proceed with the next file. */
			_strings_storage.ReadFromYAML(opt_data.argv[i]);
			continue;
		} else if (!StrEndsWith(opt_data.argv[i], ".fpp", false)) {
			if (StrEndsWith(opt_data.argv[i], ".fml", false)) {
				fprintf(stderr, "FML files must be preprocessed into FPP files before they can be used to generate RCD files.\n");
			} else if (StrEndsWith(opt_data.argv[i], ".fms", false)) {
				fprintf(stderr, "Do not compile FMS files directly. They are meant to be #included in an FML file.\n");
			} else {
				fprintf(stderr, "Unrecognized file extension for file (supported are .fpp and .yml): '%s'\n", opt_data.argv[i]);
			}
			exit(1);
		}

		/* Phase 1: Parse the input file. */
		std::shared_ptr<NamedValueList> nvs = LoadFile(opt_data.argv[i]);

		/* Phase 2: Check and simplify the loaded input. */
		FileNodeList *file_nodes = CheckTree(nvs);
		nvs = nullptr;

		/* Phase 3: Construct output files. */
		for (auto iter : file_nodes->files) {
			FileWriter fw;
			iter->Write(&fw);
			fw.WriteFile(iter->file_name);
		}

		delete file_nodes;
	}
	exit(0);
}
