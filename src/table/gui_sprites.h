/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gui_sprites.h Gui sprites table. */

#ifndef TABLE_GUI_SPRITES_H
#define TABLE_GUI_SPRITES_H

/** Gui sprite table. */
enum GuiSpritesTable {
	SPR_GUI_BULLDOZER,  ///< Bulldozer sprite.
	SPR_GUI_ROT2D_POS,  ///< 2d vertical rotation in positive direction (counter clockwise).
	SPR_GUI_ROT2D_NEG,  ///< 2d vertical rotation in negative direction (clockwise).
	SPR_GUI_ROT3D_POS,  ///< 3d (xy-plane) rotation in positive direction (counter clockwise).
	SPR_GUI_ROT3D_NEG,  ///< 3d (xy-plane) rotation in negative direction (clockwise).

	SPR_GUI_SLOPES_START, ///< Start of track slopes.
	SPR_GUI_SLOPES_END = SPR_GUI_SLOPES_START + TSL_COUNT_VERTICAL, ///< End of track slopes.

	SPR_GUI_BUILDARROW_START = SPR_GUI_SLOPES_END, ///< Start of build arrows.
	SPR_GUI_BUILDARROW_END   = SPR_GUI_BUILDARROW_START + EDGE_COUNT, ///< End of build arrows.
};

#endif
