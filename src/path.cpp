/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path.cpp %Path definitions. */

#include "stdafx.h"
#include "path.h"

/**
 * Expansion of path sprite number to a value using path bits (#PATHBIT_N, #PATHBIT_E, #PATHBIT_S, #PATHBIT_W,
 * #PATHBIT_NE, #PATHBIT_SE, #PATHBIT_SW, and #PATHBIT_NW).
 */
const uint8 _path_expand[] = {
	  0, // PATH_EMPTY
	 16, // PATH_NE
	 32, // PATH_SE
	 48, // PATH_NE_SE
	 50, // PATH_NE_SE_E
	 64, // PATH_SW
	 80, // PATH_NE_SW
	 96, // PATH_SE_SW
	100, // PATH_SE_SW_S
	112, // PATH_NE_SE_SW
	114, // PATH_NE_SE_SW_E
	116, // PATH_NE_SE_SW_S
	118, // PATH_NE_SE_SW_E_S
	128, // PATH_NW
	144, // PATH_NE_NW
	145, // PATH_NE_NW_N
	160, // PATH_NW_SE
	176, // PATH_NE_NW_SE
	177, // PATH_NE_NW_SE_N
	178, // PATH_NE_NW_SE_E
	179, // PATH_NE_NW_SE_N_E
	192, // PATH_NW_SW
	200, // PATH_NW_SW_W
	208, // PATH_NE_NW_SW
	209, // PATH_NE_NW_SW_N
	216, // PATH_NE_NW_SW_W
	217, // PATH_NE_NW_SW_N_W
	224, // PATH_NW_SE_SW
	228, // PATH_NW_SE_SW_S
	232, // PATH_NW_SE_SW_W
	236, // PATH_NW_SE_SW_S_W
	240, // PATH_NE_NW_SE_SW
	241, // PATH_NE_NW_SE_SW_N
	242, // PATH_NE_NW_SE_SW_E
	243, // PATH_NE_NW_SE_SW_N_E
	244, // PATH_NE_NW_SE_SW_S
	245, // PATH_NE_NW_SE_SW_N_S
	246, // PATH_NE_NW_SE_SW_E_S
	247, // PATH_NE_NW_SE_SW_N_E_S
	248, // PATH_NE_NW_SE_SW_W
	249, // PATH_NE_NW_SE_SW_N_W
	250, // PATH_NE_NW_SE_SW_E_W
	251, // PATH_NE_NW_SE_SW_N_E_W
	252, // PATH_NE_NW_SE_SW_S_W
	253, // PATH_NE_NW_SE_SW_N_S_W
	254, // PATH_NE_NW_SE_SW_E_S_W
	255, // PATH_NE_NW_SE_SW_N_E_S_W
};

/**
 * Inverse operation of #_path_expand. #PATH_INVALID means there is no sprite defined for this combination.
 * Note that even if a sprite is defined in this table, a particular path type may not have a sprite for
 * every combination (for example, queue paths have no corner bits at all).
 */
const uint8 _path_implode[256] = {
	PATH_EMPTY,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SE,               PATH_INVALID,             PATH_NE_SE_E,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SW,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE_SW_S,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SE_SW,            PATH_INVALID,             PATH_NE_SE_SW_E,          PATH_INVALID,
	PATH_NE_SE_SW_S,          PATH_INVALID,             PATH_NE_SE_SW_E_S,        PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW,               PATH_NE_NW_N,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SE,            PATH_NE_NW_SE_N,          PATH_NE_NW_SE_E,          PATH_NE_NW_SE_N_E,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SW_W,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SW,            PATH_NE_NW_SW_N,          PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SW_W,          PATH_NE_NW_SW_N_W,        PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW,            PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_S,          PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_W,          PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_S_W,        PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SE_SW,         PATH_NE_NW_SE_SW_N,       PATH_NE_NW_SE_SW_E,       PATH_NE_NW_SE_SW_N_E,
	PATH_NE_NW_SE_SW_S,       PATH_NE_NW_SE_SW_N_S,     PATH_NE_NW_SE_SW_E_S,     PATH_NE_NW_SE_SW_N_E_S,
	PATH_NE_NW_SE_SW_W,       PATH_NE_NW_SE_SW_N_W,     PATH_NE_NW_SE_SW_E_W,     PATH_NE_NW_SE_SW_N_E_W,
	PATH_NE_NW_SE_SW_S_W,     PATH_NE_NW_SE_SW_N_S_W,   PATH_NE_NW_SE_SW_E_S_W,   PATH_NE_NW_SE_SW_N_E_S_W,
};
