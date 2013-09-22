/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file memory.h Memory management functions. */

#ifndef MEMORY_H
#define MEMORY_H

/* Copied from OpenTTD. */

/**
 * Type wrapper around memcpy.
 * @tparam T Type of the data to copy.
 * @param dest Base address of the destination.
 * @param src  Base address of the source data.
 * @param count Number of data elements.
 */
template <class T>
inline void MemCpy(T *dest, const T *src, size_t count)
{
	memcpy(dest, src, sizeof(T) * count);
}

/**
 * Type-safe version of memset.
 *
 * @param ptr Pointer to the destination buffer
 * @param value Value to be set
 * @param num number of items to be set (!not number of bytes!)
 */
template <typename T>
inline void MemSetT(T *ptr, byte value, size_t num = 1)
{
	memset(ptr, value, num * sizeof(T));
}

#endif
