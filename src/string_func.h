/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_func.h Safe string functions. */

#ifndef STRING_FUNC_H
#define STRING_FUNC_H

char *SafeStrncpy(char *dest, const char *src, int size);
char *StrDup(const char *src);

bool StrEndsWith(const char *str, const char *end, bool case_sensitive);

#endif
