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
 * \a min and \a max. If the given value is in this interval the value itself
 * is returned otherwise the border of the interval is returned, according
 * which side of the interval was 'left'.
 *
 * @note The \a min value must be less or equal of \a max or you get some
 *       unexpected results.
 * @param a The value to clamp/truncate.
 * @param min The minimum of the interval.
 * @param max the maximum of the interval.
 * @returns A value between \a min and \a max which is closest to \a a.
 */
template <typename T>
static FORCEINLINE T Clamp(const T a, const T min, const T max)
{
	assert(min <= max);
	if (a <= min) return min;
	if (a >= max) return max;
	return a;
}

#endif
