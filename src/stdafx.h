/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file stdafx.h Common type and macro definitions. */

#ifndef STDAFX_H
#define STDAFX_H

/* Gratefully borrowed from OpenTTD. */

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cassert>

#if defined(UNIX) || defined(__MINGW32__)
	#include <sys/types.h>
#endif


/* Stuff for GCC */
#if defined(__GNUC__)
	#define NORETURN __attribute__ ((noreturn))
	#define FORCEINLINE inline
	#define CDECL
	#define __int64 long long
	#define GCC_PACK __attribute__((packed))
	/* Warn about functions using 'printf' format syntax. First argument determines which parameter
	 * is the format string, second argument is start of values passed to printf. */
	#define WARN_FORMAT(string, args) __attribute__ ((format (printf, string, args)))
#endif /* __GNUC__ */


typedef unsigned char byte;

/* This is already defined in unix, but not in QNX Neutrino (6.x)*/
#if (!defined(UNIX) && !defined(__CYGWIN__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__MORPHOS__)) || defined(__QNXNTO__)
	typedef unsigned int uint;
#endif

#if defined(TROUBLED_INTS)
	/* NDS'/BeOS'/Haiku's types for uint32/int32 are based on longs, which causes
	 * trouble all over the place in OpenTTD. */
	#define uint32 uint32_ugly_hack
	#define int32 int32_ugly_hack
	typedef unsigned int uint32_ugly_hack;
	typedef signed int int32_ugly_hack;
#else
	typedef unsigned char    uint8;
	typedef   signed char     int8;
	typedef unsigned short   uint16;
	typedef   signed short    int16;
	typedef unsigned int     uint32;
	typedef   signed int      int32;
	typedef unsigned __int64 uint64;
	typedef   signed __int64  int64;
#endif /* !TROUBLED_INTS */

/* Compile time assertions. Prefer c++0x static_assert().
 * Older compilers cannot evaluate some expressions at compile time,
 * typically when templates are involved, try assert_tcompile() in those cases. */
#if defined(__STDCXX_VERSION__) || defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(__GXX_EXPERIMENTAL_CPP0X__) || defined(static_assert)
	/* __STDCXX_VERSION__ is c++0x feature macro, __GXX_EXPERIMENTAL_CXX0X__ is used by gcc, __GXX_EXPERIMENTAL_CPP0X__ by icc */
	#define assert_compile(expr) static_assert(expr, #expr )
	#define assert_tcompile(expr) assert_compile(expr)
#elif defined(__OS2__)
	/* Disabled for OS/2 */
	#define assert_compile(expr)
	#define assert_tcompile(expr) assert_compile(expr)
#else
	#define assert_compile(expr) typedef int __ct_assert__[1 - 2 * !(expr)]
	#define assert_tcompile(expr) assert(expr)
#endif

/* Check that the types have the byte sizes like we are using them. */
assert_compile(sizeof(uint64) == 8);
assert_compile(sizeof(uint32) == 4);
assert_compile(sizeof(uint16) == 2);
assert_compile(sizeof(uint8)  == 1);

/**
 * Return the length of an fixed size array.
 * Unlike sizeof this function returns the number of elements
 * of the given type.
 *
 * @param x The pointer to the first element of the array
 * @return The number of elements
 */
#define lengthof(x) (sizeof(x) / sizeof(x[0]))

/**
 * Get the end element of an fixed size array.
 *
 * @param x The pointer to the first element of the array
 * @return The pointer past to the last element of the array
 */
#define endof(x) (&x[lengthof(x)])

/**
 * Get the last element of an fixed size array.
 *
 * @param x The pointer to the first element of the array
 * @return The pointer to the last element of the array
 */
#define lastof(x) (&x[lengthof(x) - 1])

/**
 * Report a fatal error.
 * @param str Format of the error.
 */
void NORETURN CDECL error(const char *str, ...) WARN_FORMAT(1, 2);

/** Macro for reporting reaching an 'impossible' position in the code. */
#define NOT_REACHED() error("NOT_REACHED triggered at line %i of %s", __LINE__, __FILE__)


#endif
