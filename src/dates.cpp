/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dates.cpp Days and years in the program. */

#include "stdafx.h"
#include "dates.h"
#include "language.h"

static const int _days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; ///< Numbers of days in each month (in a non-leap year).

Date _date; /// %Date in the program.

/**
 * Constructor for a specific date.
 * @param pday Day of the month (1-based).
 * @param pmonth Month (1-based).
 * @param pyear Year (1-based).
 * @param pfrac Day fraction (0-based).
 */
Date::Date(int pday, int pmonth, int pyear, int pfrac) : day(pday), month(pmonth), year(pyear), frac(pfrac)
{
	assert(pyear > 0);
	assert(pmonth > 0 && pmonth < 13);
	assert(pday > 0 && pday <= _days_per_month[pmonth - 1]);
	assert(pfrac >= 0 && pfrac < TICK_COUNT_PER_DAY);
}

/** Default constructor. */
Date::Date() : day(1), month(1), year(1), frac(0)
{
}

/**
 * Copy constructor.
 * @param d Existing date.
 */
Date::Date(const Date &d) : day(d.day), month(d.month), year(d.year), frac(d.frac)
{
}

/**
 * Assignment operator.
 * @param d Existing date.
 */
Date &Date::operator=(const Date &d)
{
	if (this != &d) {
		this->day = d.day;
		this->month = d.month;
		this->year = d.year;
		this->frac = d.frac;
	}
	return *this;
}

/**
 * Get the name of a month or the current month.
 * @param month Month number (1-based). Use \c 0 to get the name of the current month.
 * @return String number containing the month name.
 */
StringID Date::GetMonthName(int month) const
{
	static const uint16 month_names[] = {
		GUI_MONTH_JANUARY,
		GUI_MONTH_FEBRUARY,
		GUI_MONTH_MARCH,
		GUI_MONTH_APRIL,
		GUI_MONTH_MAY,
		GUI_MONTH_JUNE,
		GUI_MONTH_JULY,
		GUI_MONTH_AUGUST,
		GUI_MONTH_SEPTEMBER,
		GUI_MONTH_OCTOBER,
		GUI_MONTH_NOVEMBER,
		GUI_MONTH_DECEMBER,
	};

	if (month == 0) month = this->month;
	assert(month >= 1 && month < 13);
	return month_names[month - 1];
}

/**
 * Update the day.
 * @return Set of bits what parts of the date have changed.
 * @note It does not care about leap years.
 * @see DateTickChanges
 */
uint8 DateOnTick()
{
	uint8 result = 0;

	_date.frac++;
	if (_date.frac >= TICK_COUNT_PER_DAY) {
		_date.frac = 0;
		_date.day++;
		result |= DTC_DAY;
		if (_date.day > _days_per_month[_date.month - 1]) {
			_date.day = 1;
			_date.month++;
			result |= DTC_MONTH;
			if (_date.month > 12) {
				_date.month = 1;
				_date.year++;
				result |= DTC_YEAR;
			}
		}
	}
	return result;
}
