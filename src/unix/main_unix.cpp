/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file freerct.cpp Main program for unix. */


#include "../stdafx.h"
#include "../freerct.h"

/**
 * Main entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return The exit code of the program.
 */
int main(int argc, char **argv)
{
	exit(freerct_main(argc, argv));
}
