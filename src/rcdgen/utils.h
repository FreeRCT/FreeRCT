/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file utils.h Support code declarations for rcdgen. */

#ifndef UTILS_H
#define UTILS_H

class Position;

/** Data about one range in a parameterized name. */
class ParameterizedNameRange {
public:
	ParameterizedNameRange();

	int size() const;

	bool used;     ///< Whether the range contains useful data.
	int offset;    ///< Offset in the name of this parameter.
	int length;    ///< Length of this parameter in the original name.
	int min_value; ///< First value of the range.
	int max_value; ///< Last value of the range.
};

/** Seen parameters of an identifier. */
enum HorVert {
	HV_NONE, ///< Neither seen horizontal nor vertical.
	HV_HOR,  ///< Seen horizontal (of the form \c {hor(a..b)} ).
	HV_VERT, ///< Seen vertical (of the form \c {vert(a..b)} ).
	HV_BOTH, ///< Seen both horizontal and vertical.
};

/**
 * Data about parameterized names (identifiers with \c {hor(min..max)} and \c {vert(min..max)} in them.
 * The format supports at most one horizontal and one vertical parameter.
 */
class ParameterizedName {
public:
	ParameterizedName();
	~ParameterizedName();

	HorVert DecodeName(const char *name, const Position &pos);
	const char *GetParmName(int row, int col);

	char *name;     ///< The copied name;
	char *variant;  ///< The name with filled-in values for it parameters;
	HorVert result; ///< Decoding result.
	ParameterizedNameRange hor_range;  ///< Information about the horizontal range.
	ParameterizedNameRange vert_range; ///< Information about the vertical range.

	static bool HasNoParameters(const std::string &name);
};

void CheckIsSingleName(const std::string &name, const Position &pos);

#endif

