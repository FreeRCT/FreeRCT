/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file user.h Declaration of a user object. */

#ifndef USER_H
#define USER_H

#include "money.h"

/** Data of user. */
class User {
private:
	Money money; ///< Amount of money the user has.

public:

	/** Default constructor. */
	User()
	{
		this->money = 100000;
	}
};

extern User _user;

#endif
