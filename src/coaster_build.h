/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster_build.h Roller coaster construction mouse mode. */

#ifndef COASTER_BUILD_H
#define COASTER_BUILD_H

#include "viewport.h"

class TrackPiece;

/** States of the #CoasterBuildMode mouse mode. */
enum BuilderState {
	BS_OFF,      ///< Turned off.
	BS_STARTING, ///< Turned on, but no permission to activate the mode.
	BS_ON,       ///< Turned on, displaying nothing.
	BS_MOUSE,    ///< Turned on, follow mouse with a piece.
	BS_FIXED,    ///< Turned on, display a fixed piece.
	BS_DOWN,     ///< Turned on, but cannot leave the mode while wanting to go to #BS_OFF.

	BS_COUNT,    ///< Number of states in #BuilderState.
};

/**
 * Mouse mode for building/editing a roller coaster. Co-operates with #CoasterBuildWindow.
 *
 * There may exist several partially build coasters. The #instance number refers to the 'current' coaster.
 *
 * The mouse mode has 6 states:
 * - #BS_OFF Nobody needs the mouse mode.
 * - #BS_STARTING The mouse mode is needed, but another mode is active at the moment.
 * - #BS_ON Mouse mode is on, but there is nothing to display at the moment.
 * - #BS_MOUSE Mouse mode is on, and a track piece is displayed at the mouse position (and moved along with the mouse).
 * - #BS_FIXED Mouse mode is on, and a track piece is displayed at a fixed position.
 * - #BS_DOWN Mouse mode in on, there is nothing to display, and it wants to go off.
 */
class CoasterBuildMode : public MouseMode {
public:
	CoasterBuildMode();

	void OpenWindow(uint16 instance);
	void CloseWindow(uint16 instance);
	void ShowNoPiece(uint16 instance);
	void SelectPosition(uint16 instance, const TrackPiece *piece, TileEdge direction);
	void DisplayPiece(uint16 instance, const TrackPiece *piece, int x, int y, int z, TileEdge direction);

	/* virtual */ bool MayActivateMode();
	/* virtual */ void ActivateMode(const Point16 &pos);
	/* virtual */ void LeaveMode();
	/* virtual */ void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos);
	/* virtual */ void OnMouseButtonEvent(Viewport *vp, uint8 state);
	/* virtual */ bool EnableCursors();

	uint16 instance;             ///< Instance number of the current coaster.
	BuilderState state;          ///< State of the #CoasterBuildMode mouse mode.
	const TrackPiece *cur_piece; ///< Current selected track piece. \c NULL if no piece is selected currently.
	TileEdge direction;          ///< Orientation of the build cursor.
	Point16 mouse_pos;           ///< Stored mouse position.
	uint16 track_xpos;           ///< Entry X position of the selected track piece.
	uint16 track_ypos;           ///< Entry Y position of the selected track piece.
	uint8  track_zpos;           ///< Entry Z position of the selected track piece.
	bool suppress_display;       ///< Suppress display of a track piece.
	bool use_mousepos;           ///< Use mouse position to derive the position of the track piece.

	/**
	 * Select a new state in the builder mouse mode.
	 * @param state New state to select.
	 */
	void SetState(BuilderState state)
	{
		assert(state >= BS_OFF && state < BS_COUNT);
		this->state = state;
	}

	/** Do not display a track piece. */
	void SetNoPiece()
	{
		this->cur_piece = NULL;
	}

	/**
	 * Update the mouse position of the builder mouse mode.
	 * @param pos New mouse position.
	 * @todo Is this data actually used?
	 */
	void SetMousePosition(const Point16 &pos)
	{
		this->mouse_pos = pos;
	}

	/**
	 * Denote to the mouse mode handler to attach a track piece to the mouse cursor.
	 * @param piece Selected track piece to attach.
	 * @param direction Direction of building (to use with a cursor).
	 */
	void SetSelectPosition(const TrackPiece *piece, TileEdge direction)
	{
		this->cur_piece = piece;
		this->direction = direction;
		this->use_mousepos = true;
	}

	/**
	 * Denote to the mouse mode handler to display a track piece at the given position.
	 * @param piece Selected track piece to attach.
	 * @param x X position of the piece.
	 * @param y Y position of the piece.
	 * @param z Z position of the piece.
	 * @param direction Direction of building (to use with a cursor).
	 */
	void SetFixedPiece(const TrackPiece *piece, int x, int y, int z, TileEdge direction)
	{
		this->cur_piece = piece;
		this->track_xpos = x;
		this->track_ypos = y;
		this->track_zpos = z;
		this->direction = direction;
		this->use_mousepos = false;
	}

	/** Enable display of a track piece (that is, show it if there is one to show). */
	void EnableDisplay()
	{
		this->suppress_display = false;
	}

	/** Suppress display of a track piece (that is, hide it if there is one to show). */
	void DisableDisplay()
	{
		this->suppress_display = true;
	}

	void UpdateDisplay(bool mousepos_changed);
};

extern CoasterBuildMode _coaster_builder;

#endif
