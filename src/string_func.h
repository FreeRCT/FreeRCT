/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_func.h Safe string functions. */

#ifndef STRING_FUNC_H
#define STRING_FUNC_H

#include <memory>
#include <string>

char *SafeStrncpy(char *dest, const char *src, int size);
uint8 *SafeStrncpy(uint8 *dest, const uint8 *src, int size);
char *StrDup(const char *src);
uint8 *StrECpy(uint8 *dest, uint8 *end, const uint8 *src);

size_t StrBytesLength(const uint8 *txt);

int DecodeUtf8Char(const char *data, size_t length, uint32 *codepoint);
int EncodeUtf8Char(uint32 codepoint, char *dest = nullptr);
size_t GetPrevChar(const std::string &data, size_t pos);
size_t GetNextChar(const std::string &data, size_t pos);

bool StrEqual(const uint8 *s1, const uint8 *s2);
bool StrEndsWith(const char *str, const char *end, bool case_sensitive);

/**
 * Convenience wrapper around the snprintf function.
 * @param format Format string with printf-style placeholders.
 * @param args Formatting arguments.
 * @return Formatted text.
 * @note When possible, use #DrawText instead.
 */
DIAG_OFF("-Wformat-security")
template<typename... Args>
std::string Format(const std::string &format, Args... args)
{
	static char buffer[2048];
	snprintf(buffer, lengthof(buffer), format.c_str(), args...);
	return buffer;
}
DIAG_ON("-Wformat-security")

#endif
