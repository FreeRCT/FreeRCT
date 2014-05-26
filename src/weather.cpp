/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file weather.cpp Game weather code. */

#include "stdafx.h"
#include "weather.h"
#include "dates.h"
#include "random.h"

Weather _weather; ///< Weather in the park.

/**
 * Average weather of a month.
 * Sum of all amounts should be equal for all months.
 */
class AverageWeather {
public:
	AverageWeather(int temp, int sun, int light_cloud, int thick_cloud, int rain, int thunder);

	int temp;        ///< Average temperature this month in 1/10 degrees Celcius.
	int sun;         ///< Average amount of sunny weather this month.
	int light_cloud; ///< Average amount of sun + clouds weather this month.
	int thick_cloud; ///< Average amount of of only clouds weather this month.
	int rain;        ///< Average amount of rain this month.
	int thunder;     ///< Average amount of thunder storm this month.

	int TotalAmount() const;
	WeatherType GetWeatherType(int amount) const;
	int Draw() const;
};

/**
 * Construct an average weather object.
 * @param temp Average temperature this month in 1/10 degrees Celcius.
 * @param sun Average amount of sunny weather this month.
 * @param light_cloud Average amount of sun + clouds weather this month.
 * @param thick_cloud Average amount of of only clouds weather this month.
 * @param rain Average amount of rain this month.
 * @param thunder Average amount of thunder storm this month.
 * @note \a sun + \a light_cloud + \a thick_cloud + \a rain + \a thunder should the same for every object.
 */
AverageWeather::AverageWeather(int temp, int sun, int light_cloud, int thick_cloud, int rain, int thunder)
		: temp(temp), sun(sun), light_cloud(light_cloud), thick_cloud(thick_cloud), rain(rain), thunder(thunder)
{
}

/**
 * Get the sum of all amounts of weather.
 * @return The sum of all amounts of weathers.
 */
int AverageWeather::TotalAmount() const
{
	return this->sun + this->light_cloud + this->thick_cloud + this->rain + this->thunder;
}

/**
 * Get the type of weather from a given sum of amounts.
 * @param amount (Partial) sum to convert.
 * @return Type of weather.
 */
WeatherType AverageWeather::GetWeatherType(int amount) const
{
	if (amount < this->sun) return WTP_SUNNY;
	amount -= this->sun;
	if (amount < this->light_cloud) return WTP_LIGHT_CLOUDS;
	amount -= this->light_cloud;
	if (amount < this->thick_cloud) return WTP_THICK_CLOUDS;
	amount -= this->thick_cloud;
	if (amount < this->rain) return WTP_RAINING;
	return WTP_THUNDERSTORM;
}

/**
 * Draw a random weather.
 * @return Amount representing the weather.
 */
int AverageWeather::Draw() const
{
	Random rnd;

	return rnd.Uniform(this->TotalAmount());
}


/**
 * Yearly weather pattern. Loosely based on data from the UK MetOffice, in particular
 * Sheffield 1981–2010 averages
 * http://www.metoffice.gov.uk/climate/uk/averages/19812010/sites/sheffield.html
 */
static const AverageWeather _yearly_weather[12] = {
	AverageWeather( 68,  45, 60, 90, 73, 1),
	AverageWeather( 71,  68, 70, 80, 50, 1),
	AverageWeather( 98, 112, 50, 53, 53, 1),
	AverageWeather(125, 144, 48, 40, 45, 2),

	AverageWeather(161, 191, 40, 20, 14, 4),
	AverageWeather(188, 179, 50, 28,  5, 7),
	AverageWeather(211, 199, 30, 20, 16, 4),
	AverageWeather(206, 185, 45, 19, 15, 5),

	AverageWeather(177, 136, 48, 40, 43, 2),
	AverageWeather(135,  91, 45, 61, 71, 1),
	AverageWeather( 95,  54, 70, 72, 69, 1),
	AverageWeather( 69,  40, 70, 82, 76, 1),
};

Weather::Weather()
{
	/* Verify that each month has the same amount of weather in total. */
	int sum0 = _yearly_weather[0].TotalAmount();
	for (int month = 1; month < 12; month++) {
		assert(sum0 == _yearly_weather[month].TotalAmount());
	}
}

/** Initialize the weather for a new game. */
void Weather::Initialize()
{
	this->current = _yearly_weather[_date.month - 1].Draw();
	this->next = this->current;
	this->change = 0;

	this->OnNewDay(); // Set weather + temperature for the 0th day.
}

/** Daily update of the weather. */
void Weather::OnNewDay()
{
	this->SetTemperature();

	if (this->change != 0) {
		this->current += this->change;
		if ((this->change > 0 && this->next <= this->current) || (this->change < 0 && this->next >= this->current)) {
			this->current = this->next;
			this->change = 0;
		}
	}

	if (_date.day != 12 && _date.day != 27) return;
	int month = (_date.day == 12) ? _date.month : _date.GetNextMonth();
	this->next = _yearly_weather[month - 1].Draw();
	if (this->current == this->next) return;
	this->change = (this->next - this->current) / 5;
	if (this->change == 0) this->change = (this->next - this->current > 0) ? 1 : -1;
}

/**
 * Get the current type of weather.
 * @return The type of weather of today.
 */
WeatherType Weather::GetWeatherType() const
{
	return _yearly_weather[_date.month - 1].GetWeatherType(this->current);
}

/** Compute todays temperature in the park. */
void Weather::SetTemperature()
{
	if (_date.day <= 15) {
		int prev_month = _date.GetPreviousMonth();
		int prev_length = _days_per_month[prev_month] - 15; // 16, 17, ...
		prev_length += _date.day - 1;
		int this_length = 15 - _date.day;
		this->temperature = (prev_length * _yearly_weather[_date.month - 1].temp + this_length * _yearly_weather[prev_month - 1].temp) / (prev_length + this_length);
	} else {
		int next_month = _date.GetNextMonth();
		int next_length = 15 + (_days_per_month[_date.month] - _date.day);
		int this_length = _date.day - 15;
		this->temperature = (next_length * _yearly_weather[_date.month - 1].temp + this_length * _yearly_weather[next_month - 1].temp) / (next_length + this_length);
	}
}

