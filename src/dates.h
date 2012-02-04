/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.h File IO declarations. */

#ifndef DATES_H
#define DATES_H

static const int TICK_COUNT_PER_DAY = 100; ///< Number of ticks in a day (stored in #Date::frac).

class Date {
public:
	Date();
	Date(int pday, int pmonth, int pyear, int pfrac = 0);
	Date(const Date &d);
	Date &operator=(const Date &d);
	~Date() { }

	uint16 GetMonthName(int month = 0) const;

	int day;   ///< Day of the month, 1-based.
	int month; ///< Month of the year, 1-based.
	int year;  ///< The current year, 1-based.
	int frac;  ///< Day fraction, 0-based.
};

void DateOnTick();

extern Date _date;

#endif

