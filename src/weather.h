/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file weather.h Game weather declarations. */

#ifndef WEATHER_H
#define WEATHER_H

/** Types of weather. */
enum WeatherType {
	WTP_SUNNY,        ///< Sunny weather.
	WTP_LIGHT_CLOUDS, ///< Light clouds.
	WTP_THICK_CLOUDS, ///< Thick clouds.
	WTP_RAINING,      ///< Rain.
	WTP_THUNDERSTORM, ///< Heavy rain with thunder.

	WTP_COUNT,        ///< Number of weather types.
};

/** The weather in the game. */
class Weather {
public:
	Weather();

	int temperature; ///< Current temperature, in 1/10 degrees Celcius.

	int current; ///< Current weather.
	int next;    ///< Next weather type.
	int change;  ///< speed of change in the weather.

	void Initialize();
	void OnNewDay();

	WeatherType GetWeatherType() const;

private:
	void SetTemperature();
};

extern Weather _weather;

#endif

