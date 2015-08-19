/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_func.cpp Safe string functions. */

#include "stdafx.h"
#include "string_func.h"

/**
 * Safe string copy (as in, the destination is always terminated).
 * @param dest Destination of the string.
 * @param src  Source of the string.
 * @param size Number of bytes (including terminating 0) from \a src.
 * @return Copied string.
 * @note The ::SafeStrncpy(uint8 *dest, const uint8 *src, int size) function is preferred.
 */
char *SafeStrncpy(char *dest, const char *src, int size)
{
	assert(size >= 1);

	strncpy(dest, src, size);
	dest[size - 1] = '\0';
	return dest;
}

/**
 * Safe string copy (as in, the destination is always terminated).
 * @param dest Destination of the string.
 * @param src  Source of the string.
 * @param size Number of bytes (including terminating 0) from \a src.
 * @return Copied string.
 */
uint8 *SafeStrncpy(uint8 *dest, const uint8 *src, int size)
{
	assert(size >= 1);

	strncpy((char *)dest, (const char *)src, size);
	dest[size - 1] = '\0';
	return dest;
}

/**
 * Duplicate a string.
 * @param src Source string.
 * @return Copy of the string in its own memory.
 * @note String must be deleted later!
 */
char *StrDup(const char *src)
{
	size_t n = strlen(src);
	char *mem = new char[n + 1];
	assert(mem != nullptr);

	return SafeStrncpy(mem, src, n + 1);
}

/**
 * String copy up to an end-point of the destination.
 * @param dest Start of destination buffer.
 * @param end End of destination buffer (\c *end is never written).
 * @param src Source string.
 * @return \a dest.
 */
uint8 *StrECpy(uint8 *dest, uint8 *end, const uint8 *src)
{
	uint8 *dst = dest;
	end--;
	while (*src && dest < end) {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
	return dst;
}

/**
 * Get the length of an utf-8 string in bytes.
 * @param str String to inspect.
 * @return Length of the given string in bytes.
 */
size_t StrBytesLength(const uint8 *str)
{
	return strlen((const char *)str);
}

/**
 * Decode an UTF-8 character.
 * @param data Pointer to the start of the data.
 * @param length Length of the \a data buffer.
 * @param[out] codepoint If decoding was successful, the value of the decoded character.
 * @return Number of bytes read to decode the character, or \c 0 if reading failed.
 */
int DecodeUtf8Char(const uint8 *data, size_t length, uint32 *codepoint)
{
	if (length < 1) return 0;
	uint32 value = *data;
	data++;
	if ((value & 0x80) == 0) {
		*codepoint = value;
		return 1;
	}
	int size;
	uint32 min_value;
	if ((value & 0xE0) == 0xC0) {
		size = 2;
		min_value = 0x80;
		value &= 0x1F;
	} else if ((value & 0xF0) == 0xE0) {
		size = 3;
		min_value = 0x800;
		value &= 0x0F;
	} else if ((value & 0xF8) == 0xF0) {
		size = 4;
		min_value = 0x10000;
		value &= 0x07;
	} else {
		return 0;
	}

	if (length < static_cast<size_t>(size)) return 0;
	for (int n = 1; n < size; n++) {
		uint8 val = *data;
		data++;
		if ((val & 0xC0) != 0x80) return 0;
		value = (value << 6) | (val & 0x3F);
	}
	if (value < min_value || (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) return 0;
	*codepoint = value;
	return size;
}

/**
 * Are the two strings equal?
 * @param s1 First string to compare.
 * @param s2 Second string to compare.
 * @return strings are exactly equal.
 */
bool StrEqual(const uint8 *s1, const uint8 *s2)
{
	assert(s1 != nullptr && s2 != nullptr);
	while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 == '\0' && *s2 == '\0';
}

/**
 * Test whether \a str ends with \a end.
 * @param str String to check.
 * @param end Expected end text.
 * @param case_sensitive Compare case sensitively.
 * @return \a str ends with the given \a end possibly after changing case.
 */
bool StrEndsWith(const char *str, const char *end, bool case_sensitive)
{
	size_t str_length = strlen(str);
	size_t end_length = strlen(end);
	if (end_length > str_length) return false;
	if (end_length == 0) return true;

	str += str_length - end_length;
	while (*str != '\0' && *end != '\0') {
		char s = *str;
		char e = *end;
		if (!case_sensitive && s >= 'A' && s <= 'Z') s += 'a' - 'A';
		if (!case_sensitive && e >= 'A' && e <= 'Z') e += 'a' - 'A';
		if (s != e) return false;
		str++; end++;
	}
	return true;
}
