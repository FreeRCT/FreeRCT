/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gui_sprites.h Gui sprites table. */

#ifndef GUI_SPRITES_H
#define GUI_SPRITES_H

/** Gui sprite table. */
enum GuiSpritesTable {
	SPR_GUI_BULLDOZER, ///< Bulldozer sprite.
	SPR_GUI_ROT2D_POS, ///< 2d vertical rotation in positive direction (counter clockwise).
	SPR_GUI_ROT2D_NEG, ///< 2d vertical rotation in negative direction (clockwise).
	SPR_GUI_ROT3D_POS, ///< 3d (xy-plane) rotation in positive direction (counter clockwise).
	SPR_GUI_ROT3D_NEG, ///< 3d (xy-plane) rotation in negative direction (clockwise).

	SPR_GUI_MESSAGE_GOTO,      ///< Inbox button go to location sprite.
	SPR_GUI_MESSAGE_PARK,      ///< Inbox button park management sprite.
	SPR_GUI_MESSAGE_GUEST,     ///< Inbox button guest window sprite.
	SPR_GUI_MESSAGE_RIDE,      ///< Inbox button ride instance window sprite.
	SPR_GUI_MESSAGE_RIDE_TYPE, ///< Inbox button ride select gui sprite.

	SPR_GUI_BENCH,  ///< Bench icon.
	SPR_GUI_BIN,    ///< Bin icon.
	SPR_GUI_LAMP,   ///< Lamp icon.

	SPR_GUI_LOADSAVE_ERR,   ///< Loadsave icon for files with errors.
	SPR_GUI_LOADSAVE_WARN,  ///< Loadsave icon for files with warnings.
	SPR_GUI_LOADSAVE_OK,    ///< Loadsave icon for files without warnings or errors.

	SPR_GUI_SPEED_0,  ///< 0× speed icon.
	SPR_GUI_SPEED_1,  ///< 1× speed icon.
	SPR_GUI_SPEED_2,  ///< 2× speed icon.
	SPR_GUI_SPEED_4,  ///< 4× speed icon.
	SPR_GUI_SPEED_8,  ///< 8× speed icon.

	SPR_GUI_TOOLBAR_BEGIN,                         ///< Begin of toolbar sprites.
	SPR_GUI_TOOLBAR_MAIN = SPR_GUI_TOOLBAR_BEGIN,  ///< Toolbar button main menu gui sprite.
	SPR_GUI_TOOLBAR_SPEED,                         ///< Toolbar button speed menu gui sprite.
	SPR_GUI_TOOLBAR_PATH,                          ///< Toolbar button path build gui sprite.
	SPR_GUI_TOOLBAR_RIDE,                          ///< Toolbar button ride select gui sprite.
	SPR_GUI_TOOLBAR_FENCE,                         ///< Toolbar button fence build gui sprite.
	SPR_GUI_TOOLBAR_SCENERY,                       ///< Toolbar button scenery build gui sprite.
	SPR_GUI_TOOLBAR_TERRAIN,                       ///< Toolbar button terraform gui sprite.
	SPR_GUI_TOOLBAR_STAFF,                         ///< Toolbar button staff gui sprite.
	SPR_GUI_TOOLBAR_INBOX,                         ///< Toolbar button inbox gui sprite.
	SPR_GUI_TOOLBAR_FINANCES,                      ///< Toolbar button finances gui sprite.
	SPR_GUI_TOOLBAR_OBJECTS,                       ///< Toolbar button path objects gui sprite.
	SPR_GUI_TOOLBAR_VIEW,                          ///< Toolbar button view menu gui sprite.
	SPR_GUI_TOOLBAR_PARK,                          ///< Toolbar button park management gui sprite.
	SPR_GUI_TOOLBAR_END,                           ///< End of toolbar sprites.

	SPR_GUI_TRIANGLE_LEFT = SPR_GUI_TOOLBAR_END,   ///< Triangular arrow to the left.
	SPR_GUI_TRIANGLE_RIGHT,                        ///< Triangular arrow to the right.
	SPR_GUI_TRIANGLE_UP,                           ///< Upward triangular arrow.
	SPR_GUI_TRIANGLE_DOWN,                         ///< Downward triangular arrow.

	SPR_GUI_HAS_PLATFORM, ///< Selection of track pieces with a platform.
	SPR_GUI_NO_PLATFORM,  ///< Selection of track pieces without a platform.
	SPR_GUI_HAS_POWER,    ///< Selection of powered track pieces.
	SPR_GUI_NO_POWER,     ///< Selection of unpowered track pieces.

	SPR_GUI_COMPASS_START, ///< Start of viewing direction sprites. @see TileCorner
	SPR_GUI_COMPASS_END = SPR_GUI_COMPASS_START + TC_END, ///< End of viewing direction sprites.

	SPR_GUI_WEATHER_START = SPR_GUI_COMPASS_END, ///< Start of weather sprites. @see WeatherSprites
	SPR_GUI_WEATHER_END = SPR_GUI_WEATHER_START + WTP_COUNT, ///< End of weather sprites.

	SPR_GUI_ROG_LIGHTS_START = SPR_GUI_WEATHER_END, ///< Start of red/orange/green lights.
	SPR_GUI_ROG_LIGHTS_END = SPR_GUI_ROG_LIGHTS_START + 4, ///< End of red/orange/green lights.

	SPR_GUI_RG_LIGHTS_START = SPR_GUI_ROG_LIGHTS_END, ///< Start of red/green lights.
	SPR_GUI_RG_LIGHTS_END = SPR_GUI_RG_LIGHTS_START + 3, ///< End of red/green lights.

	SPR_GUI_SLOPES_START = SPR_GUI_RG_LIGHTS_END, ///< Start of track slopes. @see TrackSlope
	SPR_GUI_SLOPES_END = SPR_GUI_SLOPES_START + TSL_COUNT_VERTICAL, ///< End of track slopes.

	SPR_GUI_BUILDARROW_START = SPR_GUI_SLOPES_END, ///< Start of build arrows. @see TileEdge
	SPR_GUI_BUILDARROW_END   = SPR_GUI_BUILDARROW_START + EDGE_COUNT, ///< End of build arrows.

	SPR_GUI_BEND_START = SPR_GUI_BUILDARROW_END, ///< Start of track bends. @see TrackBend
	SPR_GUI_BEND_END = SPR_GUI_BEND_START + TBN_COUNT, ///< End of the track bends.

	SPR_GUI_BANK_START = SPR_GUI_BEND_END, ///< Start of track banking. @see TrackBend
	SPR_GUI_BANK_END = SPR_GUI_BANK_START + TPB_COUNT, ///< End of the banking sprites.
};

#endif
