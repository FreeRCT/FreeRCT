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
 * Compute smallest value of both arguments.
 * @param a First value.
 * @param b Second value.
 * @return Smallest of \a a and \a b.
 */
template <typename T>
static inline T min(const T a, const T b)
{
	return (a < b) ? a : b;
}

/**
 * Compute biggest value of both arguments.
 * @param a First value.
 * @param b Second value.
 * @return Biggest of \a a and \a b.
 */
template <typename T>
static inline T max(const T a, const T b)
{
	return (a < b) ? b : a;
}

int GreatestCommonDivisor(int a, int b);
int LeastCommonMultiple(int a, int b);

#endif
