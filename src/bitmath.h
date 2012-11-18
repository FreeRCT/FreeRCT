/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bitmath.h Bit math functions. */

/* Copied from OpenTTD */

#ifndef BITMATH_H
#define BITMATH_H

/**
 * Fetch \a n bits from \a x, started at bit \a s.
 *
 * This function can be used to fetch \a n bits from the value \a x. The
 * \a s value set the start position to read. The start position is
 * count from the LSB and starts at 0. The result starts at a
 * LSB, as this isn't just an and-bitmask but also some
 * bit-shifting operations. GB(0xFF, 2, 1) will so
 * return 0x01 (0000 0001) instead of
 * 0x04 (0000 0100).
 *
 * @param x The value to read some bits.
 * @param s The start position to read some bits.
 * @param n The number of bits to read.
 * @return The selected bits, aligned to a LSB.
 */
template <typename T>
static inline uint GB(const T x, const uint8 s, const uint8 n)
{
	return (x >> s) & (((T)1U << n) - 1);
}

/**
 * Set \a n bits in \a x starting at bit \a s to \a d
 *
 * This function sets \a n bits from \a x which started as bit \a s to the value of
 * \a d. The parameters \a x, \a s and \a n works the same as the parameters of
 * #GB. The result is saved in \a x again. Unused bits in the window
 * provided by n are set to 0 if the value of \a d isn't "big" enough.
 * This is not a bug, its a feature.
 *
 * @note Parameter \a x must be a variable as the result is saved there.
 * @note To avoid unexpected results the value of \a d should not use more
 *       space as the provided space of \a n bits (log2)
 * @param x The variable to change some bits
 * @param s The start position for the new bits
 * @param n The size/window for the new bits
 * @param d The actually new bits to save in the defined position.
 * @return The new value of \a x
 */
template <typename T, typename U>
static inline T SB(T &x, const uint8 s, const uint8 n, const U d)
{
	x &= (T)(~((((T)1U << n) - 1) << s));
	x |= (T)(d << s);
	return x;
}

/**
 * ROtate \a x Left by \a n bits.
 *
 * @note Assumes a byte has 8 bits.
 * @tparam T Type of \a x (deduced from the first argument).
 * @param x The value which we want to rotate.
 * @param n The number how many we want to rotate.
 * @return The rotated number.
 */
template <typename T>
static inline T ROL(const T x, const uint8 n)
{
	return (T)(x << n | x >> (sizeof(x) * 8 - n));
}

/**
 * ROtate \a x Right by \a n bits.
 *
 * @note Assumes a byte has 8 bits.
 * @tparam T Type of \a x (deduced from the first argument).
 * @param x The value which we want to rotate.
 * @param n The number how many we want to rotate.
 * @return The rotated number.
 */
template <typename T>
static inline T ROR(const T x, const uint8 n)
{
	return (T)(x >> n | x << (sizeof(x) * 8 - n));
}

#endif
