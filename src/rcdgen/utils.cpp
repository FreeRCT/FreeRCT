/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file utils.h Support code for rcdgen. */

#include "../stdafx.h"
#include <cassert>
#include "ast.h"
#include "utils.h"


ParameterizedNameRange::ParameterizedNameRange()
{
	this->used = false;
	this->min_value = -1;
	this->max_value = -1;
}

/**
 * Get the number of elements in the range.
 * @return Number of elements in the range.
 */
int ParameterizedNameRange::size() const
{
	assert(this->used);
	return this->max_value - this->min_value + 1;
}

ParameterizedName::ParameterizedName()
{
	this->result = HV_NONE;
	this->name = NULL;
	this->variant = NULL;
}

ParameterizedName::~ParameterizedName()
{
	delete[] this->name;
	delete[] this->variant;
}

/**
 * Quick check to verify that a give name does not have parameters (that is, it is a normal single name).
 * @param name Name to inspect.
 * @return Returns \c true if no parameters are found, else \c false.
 */
bool ParameterizedName::HasNoParameters(const char *name)
{
	while (*name) {
		if (*name == '{') return false;
		name++;
	}
	return true;
}

/**
 * Add 'horizontal seen' to the set of seen parameters.
 * @param hv Parameters seen up to now.
 * @return The updated parameter set.
 */
static HorVert AddHor(HorVert hv)
{
	if (hv == HV_NONE) return HV_HOR;
	if (hv == HV_VERT) return HV_BOTH;
	return hv;
}

/**
 * Add 'vertical seen' to the set of seen parameters.
 * @param hv Parameters seen up to now.
 * @return The updated parameter set.
 */
static HorVert AddVert(HorVert hv)
{
	if (hv == HV_NONE) return HV_VERT;
	if (hv == HV_HOR)  return HV_BOTH;
	return hv;
}

/**
 * Parse the number at \a *p.
 * @param p [inout] Pointer to number text to parse. Gets updated to point after the last read digit after the number.
 * @return Value of the parsed number.
 * @pre Text should point to the first digit of the number to read.
 */
static int ReadNumber(const char **p)
{
	int num = 0;
	bool seen_first = false;

	while (**p >= '0' && **p <= '9') {
		num = num * 10 + **p - '0';
		(*p)++;
		seen_first = true;
	}
	assert(seen_first);
	return num;
}

/**
 * Inspect \a name, and return the information about the parameters.
 * @param name Name to inspect.
 * @param pos Position to report in case of a found error.
 * @return The found parameter ranges.
 */
HorVert ParameterizedName::DecodeName(const char *name, const Position &pos)
{
	const char *p = name;

	delete[] this->name;
	this->name = NULL;
	delete[] this->variant;
	this->variant = NULL;

	this->result = HV_NONE;
	while (*p) {
		if (*p != '{') {
			p++;
			continue;
		}

		/* Check which type of parameter is used in the name. */
		ParameterizedNameRange *range;
		const char *range_type;
		if (strncmp(p, "{hor(", 5) == 0) {
			this->result = AddHor(this->result);
			range = &this->hor_range;
			range->offset = p - name;
			p += 5;
			range_type = "hor";
		} else {
			assert(strncmp(p, "{vert(", 6) == 0);
			this->result = AddVert(this->result);
			range = &this->vert_range;
			range->offset = p - name;
			p += 6;
			range_type = "vert";
		}

		/* Process range. */
		if (range->used) {
			fprintf(stderr, "Error at line %d: A \"%s\" range is used more than one time in name \"%s\".", pos.line, range_type, name);
			exit(1);
		}

		range->min_value = ReadNumber(&p);
		assert(p[0] == '.' && p[1] == '.');
		p += 2;
		range->max_value = ReadNumber(&p);
		assert(p[0] == ')' && p[1] == '}');
		p += 2;
		if (range->min_value > range->max_value) {
			fprintf(stderr, "Error at line %d: A \"%s\" range runs from %d to %d in name \"%s\", which is an empty range.", pos.line, range_type, range->min_value, range->max_value, name);
			exit(1);
		}
		range->used = true;
		range->length = (p - name) - range->offset;
		continue;
	}
	/* Copy name. */
	this->name = new char[(p - name) + 1];
	this->variant = new char[(p - name) + 1];
	memcpy(this->name, name, (p - name) + 1);

	return this->result;
}

/**
 * Expand the parameterized name for a given row and column.
 * @param row Row number.
 * @param col Column number.
 * @return Reference to the expanded name for the given parameter values. Do not free!
 */
const char *ParameterizedName::GetParmName(int row, int col)
{
	assert(this->variant != NULL);
	char *dest = this->variant;
	int i = 0;
	while (this->name[i] != '\0') {
		if (this->hor_range.used && this->hor_range.offset == i) {
			dest += sprintf(dest, "%d", this->hor_range.min_value + col);
			i = this->hor_range.offset + this->hor_range.length;
			continue;
		}
		if (this->vert_range.used && this->vert_range.offset == i) {
			dest += sprintf(dest, "%d", this->vert_range.min_value + row);
			i = this->vert_range.offset + this->vert_range.length;
			continue;
		}
		*dest++ = this->name[i++];
	}
	*dest = '\0';
	assert(dest - this->variant <= i); // No buffer overflow.
	return this->variant;
}

/**
 * Check that the given \a name does not contain \c {hor(a..b)} or \c {vert(a..b)} strings.
 * @param name Name to check.
 * @param pos Position to report an error.
 */
void CheckIsSingleName(const char *name, const Position &pos)
{
	if (!ParameterizedName::HasNoParameters(name)) {
		fprintf(stderr, "Line %d: Name \"%s\" may not contain horizontal or vertical parameters.\n", pos.line, name);
		exit(1);
	}
}

