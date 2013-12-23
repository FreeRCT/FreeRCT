/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATH_FUNC_H
#define MATH_FUNC_H

/** @file math_func.h Generic computation functions. */

/* Copied from OpenTTD. */

/**
 * Clamp a value between an interval.
 *
 * This function returns a value which is between the given interval of
 * \a lower and \a upper. If the given value is in this interval the value itself
 * is returned otherwise the border of the interval is returned, according
 * which side of the interval was 'left'.
 *
 * @note The \a lower value must be less or equal of \a upper or you get some
 *       unexpected results.
 * @param a The value to clamp/truncate.
 * @param lower The minimum of the interval.
 * @param upper the maximum of the interval.
 * @returns A value between \a lower and \a upper which is closest to \a a.
 */
template <typename T>
static inline T Clamp(const T a, const T lower, const T upper)
{
	assert(lower <= upper);
	if (a <= lower) return lower;
	if (a >= upper) return upper;
	return a;
}

/**
 * Compute the sign of the argument \a val.
 * @param val Provided value.
 * @return The sign of the argument (\c -1, \c 0, \c +1).
 */
static inline int sign(int val)
{
	if (val > 0) return 1;
	if (val < 0) return -1;
	return 0;
}

int GreatestCommonDivisor(int a, int b);
int LeastCommonMultiple(int a, int b);
int CountBits(uint num);

#endif
