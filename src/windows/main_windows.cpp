/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file freerct.cpp Main program for windows. */


#include "../stdafx.h"
#include "../freerct.h"
#include <windows.h>
#include <windef.h>

/**
 * Parse command line
 * @param line Command line string
 * @param argv Buffer where parsed arguments end up in
 * @param max_argc Maximum number of arguments that the buffer can hold
 * @return number of arguments parsed.
 */
static int ParseCommandLine(char *line, char **argv, int max_argc)
{
	int n = 0;

	do {
		/* skip whitespace */
		while (*line == ' ' || *line == '\t') line++;

		/* end? */
		if (*line == '\0') break;

		/* special handling when quoted */
		if (*line == '"') {
			argv[n++] = ++line;
			while (*line != '"') {
				if (*line == '\0') return n;
				line++;
			}
		}
		else {
			argv[n++] = line;
			while (*line != ' ' && *line != '\t') {
				if (*line == '\0') return n;
				line++;
			}
		}
		*line++ = '\0';
	} while (n != max_argc);

	return n;
}

/**
 * Main entry point.
 * @return The exit code of the program.
 */
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc;
	char *argv[64]; // max 64 command line arguments

	/** \todo port FS2OTTD from OpenTTD and use here to convert command line string to UTF8 */
	char *cmdline = strdup(GetCommandLine());

	argc = ParseCommandLine(cmdline, argv, lengthof(argv));

	int result = freerct_main(argc, argv);

	free(cmdline);
	return result;
}
