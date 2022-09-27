/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file time_func.h Functions for measuring time. */

#ifndef TIME_H
#define TIME_H

#include <chrono>

using Realtime = std::chrono::high_resolution_clock::time_point;  ///< Represents a time point in real time.
using Duration = std::chrono::duration<double, std::milli>;       ///< Difference between two time points with millisecond precision.

/**
 * Get the current real time.
 * @return The time.
 */
inline Realtime Time()
{
	return std::chrono::high_resolution_clock::now();
}

/**
 * Get the time difference between two time points in milliseconds.
 * @return The time difference.
 */
inline double Delta(const Realtime &start, const Realtime &end = Time())
{
	Duration d = end - start;
	return d.count();
}

#endif
