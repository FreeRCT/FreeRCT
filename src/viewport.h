/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.h %Main display data. */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "window.h"

class Viewport;
class Person;
class RideInstance;

/**
 * Known mouse modes.
 * @ingroup viewport_group
 */
enum ViewportMouseMode {
	MM_INACTIVE,       ///< Inactive mode.
	MM_OBJECT_SELECT,  ///< Object selection from the display.
	MM_TILE_TERRAFORM, ///< Terraforming tiles.
	MM_PATH_BUILDING,  ///< Construct paths.
	MM_SHOP_PLACEMENT, ///< Placement of a shop.

	MM_COUNT,          ///< Number of mouse modes.
};

/**
 * Available cursor types.
 * @ingroup viewport_group
 */
enum CursorType {
	CUR_TYPE_NORTH,    ///< Show a N corner highlight.
	CUR_TYPE_EAST,     ///< Show a E corner highlight.
	CUR_TYPE_SOUTH,    ///< Show a S corner highlight.
	CUR_TYPE_WEST,     ///< Show a W corner highlight.
	CUR_TYPE_TILE,     ///< Show a tile highlight.
	CUR_TYPE_ARROW_NE, ///< Show a build arrow in the NE direction.
	CUR_TYPE_ARROW_SE, ///< Show a build arrow in the SE direction.
	CUR_TYPE_ARROW_SW, ///< Show a build arrow in the SW direction.
	CUR_TYPE_ARROW_NW, ///< Show a build arrow in the NW direction.

	CUR_TYPE_INVALID,  ///< Invalid/unused cursor.
};

/** Order of blitting sprites in a single voxel (earlier in the list is sooner). */
enum SpriteOrder {
	SO_NONE       = 0,      ///< No drawing.
	SO_FOUNDATION = 1 << 0, ///< Draw foundation sprites.
	SO_GROUND     = 1 << 1, ///< Draw ground sprites.
	SO_SUPPORT    = 1 << 2, ///< Draw support sprites.
	SO_PLATFORM   = 1 << 3, ///< Draw platform sprites.
	SO_PATH       = 1 << 4, ///< Draw path sprites.
	SO_RIDE       = 1 << 5, ///< Draw ride sprites.
	SO_PERSON     = 1 << 6, ///< Draw person sprites.
	SO_CURSOR     = 1 << 7, ///< Draw cursor sprites.
};
DECLARE_ENUM_AS_BIT_SET(SpriteOrder)

/** Data found by Viewport::ComputeCursorPosition. */
class FinderData {
public:
	FinderData(SpriteOrder allowed_types, bool select_corner);

	SpriteOrder allowed;  ///< Bit-set of sprite-types looking for (#SO_GROUND, #SO_PATH, #SO_RIDE, #SO_PERSON).
	bool select_corner;   ///< Select a corner of a ground tile.

	CursorType cursor;    ///< Type of cursor suggested.
	uint16 xvoxel;        ///< X position of the voxel with the closest sprite.
	uint16 yvoxel;        ///< Y position of the voxel with the closest sprite.
	uint8  zvoxel;        ///< Z position of the voxel with the closest sprite.
	const Person *person; ///< Found person, if any.
	uint16 ride;          ///< Found ride instance, if any.
};

/**
 * Base class of a viewport cursor.
 * @ingroup viewport_group
 */
class BaseCursor {
public:
	BaseCursor(Viewport *vp);
	virtual ~BaseCursor();

	Viewport *vp;    ///< Parent viewport object.
	CursorType type; ///< Type of cursor.

	/** Update the cursor at the screen. */
	virtual void MarkDirty() = 0;

	/**
	 * Get a cursor.
	 * @param xpos Expected x coordinate of the cursor.
	 * @param ypos Expected y coordinate of the cursor.
	 * @param zpos Expected z coordinate of the cursor.
	 * @return The cursor sprite if the cursor exists and the coordinates are correct, else \c NULL.
	 */
	virtual CursorType GetCursor(uint16 xpos, uint16 ypos, uint8 zpos) = 0;

	/**
	 * Get the maximum height of the \a xpos, \a ypos stack that needs to be examined.
	 * @param xpos X position of the voxel.
	 * @param ypos Y position of the voxel.
	 * @param zpos Top of the stack found so far.
	 * @return Highest voxel to draw for this cursor.
	 */
	virtual uint8 GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos) = 0;

	void SetInvalid();
};

/**
 * Single tile cursor.
 * @ingroup viewport_group
 */
class Cursor : public BaseCursor {
public:
	Cursor(Viewport *vp);

	uint16 xpos; ///< %Voxel x position of the cursor.
	uint16 ypos; ///< %Voxel y position of the cursor.
	uint8  zpos; ///< %Voxel z position of the cursor.

	bool SetCursor(uint16 xpos, uint16 ypos, uint8 zpos, CursorType type, bool always = false);

	virtual void MarkDirty();
	virtual CursorType GetCursor(uint16 xpos, uint16 ypos, uint8 zpos);
	virtual uint8 GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos);
};

/**
 * Cursor consisting of one or more tiles.
 * The cursor at every tile is a tile cursor.
 */
class MultiCursor : public BaseCursor {
public:
	MultiCursor(Viewport *vp);

	Rectangle32 rect;   ///< Rectangle with cursors.
	int16 zpos[10][10]; ///< Cached Z positions of the cursors (negative means not set).

	bool SetCursor(const Rectangle32 &rect, CursorType type, bool always = false);

	virtual void MarkDirty();
	virtual CursorType GetCursor(uint16 xpos, uint16 ypos, uint8 zpos);
	virtual uint8 GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos);

	void ClearZPositions();
	uint8 GetZpos(int xpos, int ypos);
	void ResetZPosition(const Point32 &pos);
};

/**
 * Class for displaying parts of the world.
 * @ingroup viewport_group
 */
class Viewport : public Window {
public:
	Viewport(int x, int y, uint w, uint h);
	~Viewport();

	void MarkVoxelDirty(int16 xpos, int16 ypos, int16 zpos, int16 height = 0);
	virtual void OnDraw();

	void Rotate(int direction);
	void MoveViewport(int dx, int dy);

	SpriteOrder ComputeCursorPosition(FinderData *fdata);
	CursorType GetCursorAtPos(uint16 xpos, uint16 ypos, uint8 zpos);
	uint8 GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos);

	void EnableWorldAdditions();
	void DisableWorldAdditions();
	void EnsureAdditionsAreVisible();

	Point32 ComputeHorizontalTranslation(int dx, int dy);
	int32 ComputeX(int32 xpos, int32 ypos);
	int32 ComputeY(int32 xpos, int32 ypos, int32 zpos);

	int32 xview; ///< X position of the center point of the viewport.
	int32 yview; ///< Y position of the center point of the viewport.
	int32 zview; ///< Z position of the center point of the viewport.

	uint16 tile_width;           ///< Width of a tile.
	uint16 tile_height;          ///< Height of a tile.
	ViewOrientation orientation; ///< Direction of view.
	Cursor tile_cursor;          ///< Cursor for selecting a tile (or tile corner).
	Cursor arrow_cursor;         ///< Cursor for showing the path/track build direction.
	MultiCursor area_cursor;     ///< Cursor for showing an area.
	Point16 mouse_pos;           ///< Last known position of the mouse.
	bool additions_enabled;      ///< Flashing of world additions is enabled.

private:
	bool additions_displayed;    ///< Additions in #_additions are displayed to the user.

	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);

	virtual void TimeoutCallback();
};

/** A single mouse mode. */
class MouseMode {
public:
	MouseMode(WindowTypes wtype, ViewportMouseMode mode);
	virtual ~MouseMode();

	/**
	 * Query the mode whether it can be enabled. Method may be called at any moment (or more often than once).
	 * Answering \c true does not mean the mouse mode will be activated.
	 * @return Whether the mode may be enabled.
	 */
	virtual bool MayActivateMode() = 0;

	/**
	 * Activate the mouse mode.
	 * @param pos Last known mouse position.
	 */
	virtual void ActivateMode(const Point16 &pos) = 0;

	/** Notification that the mouse mode is about to be disabled. */
	virtual void LeaveMode() = 0;

	virtual void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos);
	virtual void OnMouseButtonEvent(Viewport *vp, uint8 state);
	virtual void OnMouseWheelEvent(Viewport *vp, int direction);

	/**
	 * Control display of cursors in the mouse mode.
	 * @return \c true when cursors should be displayed, \c false otherwise.
	 */
	virtual bool EnableCursors() = 0;

	const WindowTypes wtype;      ///< Type of window associated with this mouse mode.
	const ViewportMouseMode mode; ///< Mouse mode implemented by the object.
};

/** Default mouse mode, selected when no other mouse mode is available. */
class DefaultMouseMode : public MouseMode {
public:
	DefaultMouseMode();

	virtual bool MayActivateMode();
	virtual void ActivateMode(const Point16 &pos);
	virtual void LeaveMode();
	virtual bool EnableCursors();
};

/** All mouse modes. */
class MouseModes {
public:
	MouseModes();

	void RegisterMode(MouseMode *mm);
	void SetViewportMousemode();
	ViewportMouseMode GetMouseMode();
	void SetMouseMode(ViewportMouseMode mode);

	void SwitchMode(MouseMode *new_mode);

	Viewport *main_display;     ///< Main screen, managed by #Viewport.
	MouseMode *current;         ///< Current mouse mode.
	MouseMode *modes[MM_COUNT]; ///< Registered mouse modes.

private:
	DefaultMouseMode default_mode; ///< Dummy mouse mode (#MM_INACTIVE mode).
};

extern MouseModes _mouse_modes;

void EnableWorldAdditions();
void DisableWorldAdditions();
Viewport *GetViewport();

#endif
