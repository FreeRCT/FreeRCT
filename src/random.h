/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file random.h %Random number generator. */

#ifndef RANDOM_H
#define RANDOM_H

/** A random generator class. */
class Random {
public:
	bool Success1024(uint upper);
	bool Success(int perc);
	uint16 Uniform(uint16 incl_upper);
	uint16 Exponential(uint16 mean);

	static void Load(Loader &ldr);
	static void Save(Saver &svr);

private:
	static uint32 seed; ///< Seed of the generators.

	uint32 DrawNumber();
};

#endif
