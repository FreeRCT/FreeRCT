/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dates.cpp Days and years in the program. */

#include "stdafx.h"
#include "dates.h"
#include "gamecontrol.h"

assert_compile(TICK_COUNT_PER_DAY < (1 << CDB_FRAC_LENGTH)); ///< Day length should stay within the fraction limit.

const int _days_per_month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; ///< Numbers of days in each 1-based month (in a non-leap year).

Date _date; ///< %Date in the program.

/**
 * How many days in the year the park is open.
 * @return Days per year.
 */
int DaysInParkYear()
{
	int d = 0;
	for (int m = FIRST_MONTH; m <= LAST_MONTH; ++m) d += _days_per_month[m];
	return d;
}

/**
 * Constructor for a specific date.
 * @param pday Day of the month (1-based).
 * @param pmonth Month (1-based).
 * @param pyear Year (1-based, although a hypothetical year 0 is also valid).
 * @param pfrac Day fraction (0-based).
 */
Date::Date(int pday, int pmonth, int pyear, int pfrac) : day(pday), month(pmonth), year(pyear), frac(pfrac)
{
	assert(pyear >= 0 && pyear < (1 << CDB_YEAR_LENGTH));
	assert(pmonth > 0 && pmonth < 13);
	assert(pday > 0 && pday <= _days_per_month[pmonth]);
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
 * Constructor od a date from a compressed date.
 * @param cd Compressed date source.
 */
Date::Date(CompressedDate cd)
{
	int pyear  = (cd >> CDB_YEAR_START)  & ((1 << CDB_YEAR_LENGTH)  - 1);
	int pmonth = (cd >> CDB_MONTH_START) & ((1 << CDB_MONTH_LENGTH) - 1);
	int pday   = (cd >> CDB_DAY_START)   & ((1 << CDB_DAY_LENGTH)   - 1);
	int pfrac  = (cd >> CDB_FRAC_START)  & ((1 << CDB_FRAC_LENGTH)  - 1);

	assert(pyear >= 0 && pyear < (1 << CDB_YEAR_LENGTH));
	assert(pmonth > 0 && pmonth < 13);
	assert(pday > 0 && pday <= _days_per_month[pmonth]);
	assert(pfrac >= 0 && pfrac < TICK_COUNT_PER_DAY);

	this->year  = pyear;
	this->month = pmonth;
	this->day   = pday;
	this->frac  = pfrac;
}

/**
 * Assignment operator.
 * @param d Existing date.
 * @return Assigned value.
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

/** Initialize the date for the start of a game. */
void Date::Initialize()
{
	this->day = 1;
	this->month = FIRST_MONTH;
	this->year = 1;
	this->frac = 0;
}

/**
 * Compress the date to an integer number.
 * @return The compressed date.
 */
CompressedDate Date::Compress() const
{
	return (this->year << CDB_YEAR_START) | (this->month << CDB_MONTH_START) | (this->day << CDB_DAY_START) | (this->frac << CDB_FRAC_START);
}

/**
 * Get the number of the previous month that the park was open.
 * @return Month number (1-based) for the previous month.
 */
int Date::GetPreviousMonth() const
{
	if (this->month == FIRST_MONTH) return LAST_MONTH;
	return this->month - 1;
}

/**
 * Get the number of the next month that the park will be open.
 * @return Month number (1-based) for the next month.
 */
int Date::GetNextMonth() const
{
	if (this->month < LAST_MONTH) return this->month + 1;
	return FIRST_MONTH;
}

/**
 * Update the day.
 * @todo Care about leap years.
 */
void DateOnTick()
{
	bool newmonth = false;
	bool newyear  = false;

	/* New tick. */
	_date.frac++;
	if (_date.frac < TICK_COUNT_PER_DAY) return;

	/* New day. */
	_date.frac = 0;
	_date.day++;

	/* New month. */
	if (_date.day > _days_per_month[_date.month]) {
		bool is_last_month = (_date.month == LAST_MONTH);
		_date.day = 1;
		_date.month++;
		newmonth = true;

		/* New year. */
		if (is_last_month || _date.month > 12) {
			_date.month = FIRST_MONTH;
			_date.year++;
			newyear = true;
		}
	}

	OnNewDay();
	if (newmonth) OnNewMonth();
	if (newyear)  OnNewYear();
}

/**
 * Compare two dates.
 * @param d Date to compare.
 * @return This date is earlier than the argument.
 */
bool Date::operator<(const Date &d) const
{
	if (this->year != d.year) return this->year < d.year;
	if (this->month != d.month) return this->month < d.month;
	if (this->day != d.day) return this->day < d.day;
	return this->frac < d.frac;
}

static const uint32 CURRENT_VERSION_DATE = 1;   ///< Currently supported version of the DATE Pattern.

/**
 * Load the current date from the save game.
 * @param ldr Input stream to load from.
 */
void LoadDate(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("DATE");
	switch (version) {
		case 0:
			_date = Date();
			break;
		case 1:
			_date = Date(ldr.GetLong());
			break;
		default:
			ldr.VersionMismatch(version, CURRENT_VERSION_DATE);
	}
	ldr.ClosePattern();
}

/**
 * Save the current date to the save game.
 * @param svr Output stream to save to.
 */
void SaveDate(Saver &svr)
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("DATE", CURRENT_VERSION_DATE);
	svr.PutLong(_date.Compress());
	svr.EndPattern();
}
