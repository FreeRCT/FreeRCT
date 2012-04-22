/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file random.cpp Random generator support functions. */

#include "stdafx.h"
#include "random.h"
#include <time.h>


uint32 Random::seed = 0;


/**
 * See whether we are lucky.
 * @param upper Upper bound on the value (exclusive), between \c 0 and \c 1024.
 * @return Drawn value is less then the \a upper limit.
 */
bool Random::Success1024(uint upper)
{
	uint32 val = this->DrawNumber() >> 22; // Keep the upper 10 bits.
	return val < upper;
}

/**
 * Try being successful for \a perc percent.
 * @param perc Percentage of success.
 * @return Whether we were lucky.
 */
bool Random::Success(int perc)
{
	assert(perc >= 0 && perc <= 100);
	return this->Success1024((uint)perc * 1024 / 100);
}

/**
 * Draw a number from an uniform distribution. Lower bound is \c 0, upper bound is \a incl_upper (inclusive).
 * @param incl_upper Inclusive upper bound.
 * @return Value drawn from the distribution within the set limits.
 */
uint16 Random::Uniform(uint16 incl_upper)
{
	uint length = 1;
	uint32 val = 2;
	while (val <= incl_upper) {
		val <<= 1;
		length++;
	}
	for (;;) {
		val = this->DrawNumber() >> (32 - length);
		if (val <= incl_upper) return val;
	}
}

/**
 * Draw a random 32 bit number ('ranqd1' generator in Numerical Recipes).
 * @return New random number on every call.
 * @note Higher bits are more random than the low ones.
 */
uint32 Random::DrawNumber()
{
	if (seed == 0) {
		seed = time(NULL);
	}

	seed = 1664525UL * seed + 1013904223UL;
	return seed;
}
