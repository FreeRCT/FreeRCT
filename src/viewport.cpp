/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.cpp %Viewport window code. */

/**
 * @defgroup viewport_group %Viewport (main display) code and data
 * @ingroup window_group
 */

#include "stdafx.h"
#include "math_func.h"
#include "viewport.h"
#include "map.h"
#include "video.h"
#include "palette.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "path_build.h"
#include "shop_placement.h"
#include "coaster_build.h"
#include "shop_type.h"
#include "terraform.h"
#include "select_mode.h"
#include "person.h"
#include "weather.h"
#include "fence.h"
#include "fence_build.h"

#include <set>

/**
 * \page the_world_page World
 *
 * The world is stored in #_world, implemented in the #VoxelWorld. It is a 2D grid of #VoxelStack, one for each x/y position.
 * A #VoxelStack is quite literally a stack of #Voxel structures, the elementary grid element.
 * Each voxel has
 * - Optional ground (surface). #GroundType defines the ground type (query with #Voxel::GetGroundType). If it is valid (i.e. different
 *   than #GTP_INVALID), #Voxel::GetGroundSlope contains the imploded slope.
 * - Optional foundations (vertical wall). #FoundationType (query with #Voxel::GetFoundationType) defines which type of
 *   foundation is built. If it is not #FDT_INVALID, the actual foundations can be obtained with #Voxel::GetFoundationSlope.
 * - Ride instance data (#SmallRideInstance). It points to the ride that uses the voxel.
 * - Voxel information for the ride (e.g. sprite numbers, but it can mean anything).
 *
 * The ride instance is mostly one of
 * - #SRI_FREE, denoting this voxel is free for use.
 * - #SRI_RIDES_START is the first value used to denote a ride in the voxel.
 * - #SRI_FULL_RIDES is the first value used to denote a 'normal' ride.
 *
 * Almost everything is a ride. Even a path or a decorative object is a ride. These rides are however so simple that no
 * other storage other than in the voxel is needed. These rides start at #SRI_RIDES_START, up to #SRI_FULL_RIDES.
 * The normal rides can be obtained from the #_rides_manager (of type #RidesManager).
 *
 * In the future, a voxel will allow up to 4 rides (one in each corner). Rides that use more than one corner store their
 * ride instance number in the north, east, or south corner, and use #SRI_SAME_AS_NORTH, #SRI_SAME_AS_EAST, or
 * #SRI_SAME_AS_SOUTH ride instance numbers for the other corners.
 */

/**
 * \page world_additions_page World additions
 *
 * The #_world (described in \ref the_world_page) contains the 'current' world. The game allows changing of the world
 * (building rides, terraforming, removing shops, etc.) while the game runs. To keep both separate, and make it easy to
 * revert changes when the user changes his mind, changes are not stored in the world itself, but in #_additions
 * (implemented in #WorldAdditions). It is a layer on top of #_world that can be toggled on and off
 * (#EnableWorldAdditions and #DisableWorldAdditions do that).
 * There are also functions to clear the additions, and to copy changes to the world itself (thus making the change a part of
 * the game world).
 *
 * See also \ref mouse_modes_page.
 */

/**
 * \page mouse_modes_page Mouse modes
 *
 * Mouse modes are the means to introduce new ways of handling the mouse.
 * #MouseModes is the overall mouse mode manager (through #_mouse_modes) that contains all mouse modes, and manages switching
 * between them.
 *
 * A single mouse mode is a class derived from #MouseMode. It should have an entry in #ViewportMouseMode, and implement the
 * interface defined in #MouseMode. An object should statically be available, and get registered in #InitMouseModes.
 */

/**
 * Proposed additions to the game world. Not part of the game itself, but they are displayed by calling
 * #EnableWorldAdditions (and stopped being displayed with #DisableWorldAdditions).
 * The additions will flash on and off to show they are not decided yet.
 */
WorldAdditions _additions;
MouseModes _mouse_modes; ///< Mouse modes in the game.

static const int ADDITIONS_TIMEOUT_LENGTH = 15; ///< Length of the time interval of displaying or not displaying world additions.

/** Enable flashing display of showing proposed game world additions to the player. */
void EnableWorldAdditions()
{
	Viewport *vp = GetViewport();
	if (vp != nullptr) vp->EnableWorldAdditions();
}

/** Disable flashing display of showing proposed game world additions to the player. */
void DisableWorldAdditions()
{
	Viewport *vp = GetViewport();
	if (vp != nullptr) vp->DisableWorldAdditions();
}

/**
 * Convert 3D position to the horizontal 2D position.
 * @param x X position in the game world.
 * @param y Y position in the game world.
 * @param orient Orientation.
 * @param width Tile width in pixels.
 * @return X position in 2D.
 */
static inline int32 ComputeXFunction(int32 x, int32 y, ViewOrientation orient, uint16 width)
{
	switch (orient) {
		case VOR_NORTH: return ((y - x)  * width / 2) >> 8;
		case VOR_WEST:  return (-(x + y) * width / 2) >> 8;
		case VOR_SOUTH: return ((x - y)  * width / 2) >> 8;
		case VOR_EAST:  return ((x + y)  * width / 2) >> 8;
		default: NOT_REACHED();
	}
}

/**
 * Convert 3D position to the vertical 2D position.
 * @param x X position in the game world.
 * @param y Y position in the game world.
 * @param z Z position in the game world.
 * @param orient Orientation.
 * @param width Tile width in pixels.
 * @param height Tile height in pixels.
 * @return Y position in 2D.
 */
static inline int32 ComputeYFunction(int32 x, int32 y, int32 z, ViewOrientation orient, uint16 width, uint16 height)
{
	switch (orient) {
		case VOR_NORTH: return ((x + y)  * width / 4 - z * height) >> 8;
		case VOR_WEST:  return ((y - x)  * width / 4 - z * height) >> 8;
		case VOR_SOUTH: return (-(x + y) * width / 4 - z * height) >> 8;
		case VOR_EAST:  return ((x - y)  * width / 4 - z * height) >> 8;
		default: NOT_REACHED();
	}
}

/**
 * Search the world for voxels to render.
 * @ingroup viewport_group
 */
class VoxelCollector {
public:
	VoxelCollector(Viewport *vp, bool draw_above_stack);
	virtual ~VoxelCollector();

	void SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height);

	void Collect(bool use_additions);

	/**
	 * Convert 3D position to the horizontal 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @return X position in 2D.
	 */
	inline int32 ComputeX(int32 x, int32 y)
	{
		return ComputeXFunction(x, y, this->orient, this->tile_width);
	}

	/**
	 * Convert 3D position to the vertical 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @param z Z position in the game world.
	 * @return Y position in 2D.
	 */
	inline int32 ComputeY(int32 x, int32 y, int32 z)
	{
		return ComputeYFunction(x, y, z, this->orient, this->tile_width, this->tile_height);
	}

	int32 xview; ///< X position of the centre point of the display.
	int32 yview; ///< Y position of the centre point of the display.
	int32 zview; ///< Z position of the centre point of the display.

	uint16 tile_width;            ///< Width of a tile.
	uint16 tile_height;           ///< Height of a tile.
	ViewOrientation orient;       ///< Direction of view.
	const SpriteStorage *sprites; ///< Sprite collection of the right size.
	Viewport *vp;                 ///< Parent viewport for accessing the cursors if not \c nullptr.
	bool draw_above_stack;        ///< Also draw voxels above the voxel stack (for cursors).

	Rectangle32 rect; ///< Screen area of interest.

protected:
	/**
	 * Decide where supports should be raised.
	 * @param stack %Voxel stack to examine.
	 * @param xpos X position of the voxel stack.
	 * @param ypos Y position of the voxel stack.
	 */
	virtual void SetupSupports(const VoxelStack *stack, uint xpos, uint ypos)
	{
	}

	/**
	 * Handle a voxel that should be collected.
	 * @param vx   %Voxel to add, \c nullptr means 'cursor above stack'.
	 * @param xpos X world position.
	 * @param ypos Y world position.
	 * @param zpos Z world position.
	 * @param xnorth X coordinate of the north corner at the display.
	 * @param ynorth y coordinate of the north corner at the display.
	 * @note Implement in a derived class.
	 */
	virtual void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth) = 0;
};

/**
 * Data temporary needed for ordering sprites and blitting them to the screen.
 * @ingroup viewport_group
 */
struct DrawData {
	int32 level;                 ///< Slice of this sprite (vertical row).
	uint16 z_height;             ///< Height of the voxel being drawn.
	SpriteOrder order;           ///< Selection when to draw this sprite (sorts sprites within a voxel). @see SpriteOrder
	const ImageData *sprite;     ///< Mouse cursor to draw.
	Point32 base;                ///< Base coordinate of the image, relative to top-left of the window.
	const Recolouring *recolour; ///< Recolouring of the sprite.
};

/**
 * Sort predicate of the draw data.
 * @param dd1 First value to compare.
 * @param dd2 Second value to compare.
 * @return \c true if \a dd1 should be drawn before \a dd2.
 */
inline bool operator<(const DrawData &dd1, const DrawData &dd2)
{
	if (dd1.level != dd2.level) return dd1.level < dd2.level; // Order on slice first.
	if (dd1.z_height != dd2.z_height) return dd1.z_height < dd2.z_height; // Lower in the same slice first.
	if (dd1.order != dd2.order) return dd1.order < dd2.order; // Type of sprite.
	return dd1.base.y < dd2.base.y;
}

/**
 * Collection of sprites to render to the screen.
 * @ingroup viewport_group
 */
typedef std::multiset<DrawData> DrawImages;

/**
 * Collect sprites to draw in a viewport.
 * @ingroup viewport_group
 */
class SpriteCollector : public VoxelCollector {
public:
	SpriteCollector(Viewport *vp, bool enable_cursors);
	~SpriteCollector();

	void SetXYOffset(int16 xoffset, int16 yoffset);

	DrawImages draw_images; ///< Sprites to draw ordered by viewing distance.
	int16 xoffset; ///< Horizontal offset of the top-left coordinate to the top-left of the display.
	int16 yoffset; ///< Vertical offset of the top-left coordinate to the top-left of the display.
	bool enable_cursors; ///< Enable cursor drawing.

protected:
	void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth) override;
	void SetupSupports(const VoxelStack *stack, uint xpos, uint ypos) override;
	const ImageData *GetCursorSpriteAtPos(uint16 xpos, uint16 ypos, uint8 zpos, uint8 tslope, uint8 &yoffset);

	/** For each orientation the location of the real northern corner of a tile relative to the northern displayed corner. */
	Point16 north_offsets[4];

	uint16 ground_height; ///< The height of the ground in the current voxel stack. \c -1 means no valid ground found.
	uint8 ground_slope;   ///< Imploded ground slope if #ground_height is valid.
};

/**
 * Find the sprite and pixel under the mouse cursor.
 * @ingroup viewport_group
 */
class PixelFinder : public VoxelCollector {
public:
	PixelFinder(Viewport *vp, FinderData *fdata);
	~PixelFinder();

	ClickableSprite allowed; ///< Sprite types looking for.
	bool found;              ///< Found a match.
	DrawData data;           ///< Drawing data of the match found so far.
	uint32 pixel;            ///< Pixel colour of the closest sprite.
	FinderData *fdata;       ///< Finder data to return.

protected:
	void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth) override;
};

/**
 * Base class constructor.
 * @param vp %Viewport querying the voxel information.
 * @param draw_above_stack Also visit the cursors above a voxel stack.
 * @todo Can we remove some variables, as \a vp is stored now as well?
 */
VoxelCollector::VoxelCollector(Viewport *vp, bool draw_above_stack)
{
	this->vp = vp;
	this->xview = vp->xview;
	this->yview = vp->yview;
	this->zview = vp->zview;
	this->tile_width = vp->tile_width;
	this->tile_height = vp->tile_height;
	this->orient = vp->orientation;
	this->draw_above_stack = draw_above_stack;

	this->sprites = _sprite_manager.GetSprites(this->tile_width);
	assert(this->sprites != nullptr);
}

/* Destructor. */
VoxelCollector::~VoxelCollector()
{
}

/**
 * Set screen area of interest (relative to the (#xview, #yview, and #zview position).
 * @param xpos Horizontal position of the top-left corner.
 * @param ypos Vertical position of the top-left corner.
 * @param width Width of the area.
 * @param height Height of the area.
 */
void VoxelCollector::SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height)
{
	this->rect.base.x = this->ComputeX(this->xview, this->yview) + xpos;
	this->rect.base.y = this->ComputeY(this->xview, this->yview, this->zview) + ypos;
	this->rect.width = width;
	this->rect.height = height;
}

/**
 * Perform the collecting cycle.
 * This part walks over the voxels, and call #CollectVoxel for each useful voxel.
 * A derived class may then inspect the voxel in more detail.
 * @param use_additions Use the #_additions voxels for drawing.
 * @todo Do this less stupid. Walking the whole world is not going to work in general.
 */
void VoxelCollector::Collect(bool use_additions)
{
	for (uint xpos = 0; xpos < _world.GetXSize(); xpos++) {
		int32 world_x = (xpos + ((this->orient == VOR_SOUTH || this->orient == VOR_WEST) ? 1 : 0)) * 256;
		for (uint ypos = 0; ypos < _world.GetYSize(); ypos++) {
			int32 world_y = (ypos + ((this->orient == VOR_SOUTH || this->orient == VOR_EAST) ? 1 : 0)) * 256;
			int32 north_x = ComputeX(world_x, world_y);
			if (north_x + this->tile_width / 2 <= (int32)this->rect.base.x) continue; // Right of voxel column is at left of window.
			if (north_x - this->tile_width / 2 >= (int32)(this->rect.base.x + this->rect.width)) continue; // Left of the window.

			const VoxelStack *stack = use_additions ? _additions.GetStack(xpos, ypos) : _world.GetStack(xpos, ypos);
			this->SetupSupports(stack, xpos, ypos);
			uint zpos = stack->base;
			for (int count = 0; count < stack->height; zpos++, count++) {
				int32 north_y = this->ComputeY(world_x, world_y, zpos * 256);
				if (north_y - this->tile_height >= (int32)(this->rect.base.y + this->rect.height)) continue; // Voxel is below the window.
				if (north_y + this->tile_width / 2 + this->tile_height <= (int32)this->rect.base.y) break; // Above the window and rising!

				this->CollectVoxel(&stack->voxels[count], xpos, ypos, zpos, north_x, north_y);
			}
			/* Possibly cursors should be drawn above this. */
			if (this->draw_above_stack) {
				uint8 zmax = this->vp->GetMaxCursorHeight(xpos, ypos, (zpos == 0) ? zpos : zpos - 1);
				for (; zpos <= zmax; zpos++) {
					int32 north_y = this->ComputeY(world_x, world_y, zpos * 256);
					if (north_y - this->tile_height >= (int32)(this->rect.base.y + this->rect.height)) continue; // Voxel is below the window.
					if (north_y + this->tile_width / 2 + this->tile_height <= (int32)this->rect.base.y) break; // Above the window and rising!

					this->CollectVoxel(nullptr, xpos, ypos, zpos, north_x, north_y);
				}
			}
		}
	}
}

/** Enable flashing display of showing proposed game world additions to the player. */
void Viewport::EnableWorldAdditions()
{
	if (this->additions_enabled) return;

	this->additions_enabled = true;
	this->additions_displayed = true;
	_additions.MarkDirty(this);
	this->arrow_cursor.MarkDirty();
	this->timeout = ADDITIONS_TIMEOUT_LENGTH;
}

/** Disable flashing display of showing proposed game world additions to the player. */
void Viewport::DisableWorldAdditions()
{
	this->additions_enabled = false;
	if (this->additions_displayed) {
		this->additions_displayed = false;
		_additions.MarkDirty(this);
		this->arrow_cursor.MarkDirty();
	}
	this->timeout = 0;
}

/**
 * Reset the display state of the world additions, such that they become visible directly.
 * @pre World additions have to be enabled.
 */
void Viewport::EnsureAdditionsAreVisible()
{
	assert(this->additions_enabled);
	if (!this->additions_displayed) {
		this->additions_displayed = true;
		_additions.MarkDirty(this);
		this->arrow_cursor.MarkDirty();
	}
	this->timeout = ADDITIONS_TIMEOUT_LENGTH;
}

/** Toggle display of world additions (in #_additions) if enabled. */
void Viewport::TimeoutCallback()
{
	if (!this->additions_enabled) return;

	this->additions_displayed = !this->additions_displayed;
	_additions.MarkDirty(this);
	this->arrow_cursor.MarkDirty();
	this->timeout = ADDITIONS_TIMEOUT_LENGTH;
}

/**
 * Constructor of sprites collector.
 * @param vp %Viewport that needs the sprites.
 * @param enable_cursors Also collect cursors.
 */
SpriteCollector::SpriteCollector(Viewport *vp, bool enable_cursors) : VoxelCollector(vp, true)
{
	this->draw_images.clear();
	this->xoffset = 0;
	this->yoffset = 0;
	this->enable_cursors = enable_cursors;

	this->north_offsets[VOR_NORTH].x = 0;                     this->north_offsets[VOR_NORTH].y = 0;
	this->north_offsets[VOR_EAST].x  = -this->tile_width / 2; this->north_offsets[VOR_EAST].y  = this->tile_width / 4;
	this->north_offsets[VOR_SOUTH].x = 0;                     this->north_offsets[VOR_SOUTH].y = this->tile_width / 2;
	this->north_offsets[VOR_WEST].x  = this->tile_width / 2;  this->north_offsets[VOR_WEST].y  = this->tile_width / 4;
}

SpriteCollector::~SpriteCollector()
{
}

/**
 * Set the offset of the top-left coordinate of the collect window to the top-left of the display.
 * @param xoffset Horizontal offset.
 * @param yoffset Vertical offset.
 */
void SpriteCollector::SetXYOffset(int16 xoffset, int16 yoffset)
{
	this->xoffset = xoffset;
	this->yoffset = yoffset;
}

/**
 * Constructor of a cursor.
 * @param vp %Viewport displaying the cursor.
 */
BaseCursor::BaseCursor(Viewport *vp)
{
	this->vp = vp;
	this->type = CUR_TYPE_INVALID;
}

BaseCursor::~BaseCursor()
{
}

/** Mark the cursor as being invalid, and update the viewport if necessary. */
void BaseCursor::SetInvalid()
{
	this->MarkDirty();
	this->type = CUR_TYPE_INVALID;
}

/**
 * Constructor of a cursor.
 * @param vp %Viewport displaying the cursor.
 */
Cursor::Cursor(Viewport *vp) : BaseCursor(vp)
{
	this->cursor_pos = XYZPoint16(0, 0, 0);
}

void Cursor::MarkDirty()
{
	if (this->type != CUR_TYPE_INVALID) this->vp->MarkVoxelDirty(this->cursor_pos.x, this->cursor_pos.y, this->cursor_pos.z);
}

/**
 * Get a cursor.
 * @param cursor_pos Expected coordinate of the cursor.
 * @return The cursor sprite if the cursor exists and the coordinates are correct, else \c nullptr.
 */
CursorType Cursor::GetCursor(const XYZPoint16 &cursor_pos)
{
	if (this->cursor_pos != cursor_pos) return CUR_TYPE_INVALID;
	return this->type;
}

uint8 Cursor::GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->type == CUR_TYPE_INVALID) return zpos;
	if (this->cursor_pos.x != xpos || this->cursor_pos.y != ypos || zpos >= this->cursor_pos.z) return zpos;
	return this->cursor_pos.z;
}

/**
 * Set a cursor.
 * @param cursor_pos Position of the voxel containing the cursor.
 * @param type Type of cursor to set.
 * @param always Always set the cursor (else, only set it if it changed).
 * @return %Cursor has been set/changed.
 */
bool Cursor::SetCursor(const XYZPoint16 &cursor_pos, CursorType type, bool always)
{
	if (!always && this->cursor_pos == cursor_pos && this->type == type) return false;
	this->MarkDirty();
	this->cursor_pos = cursor_pos;
	this->type = type;
	this->MarkDirty();
	return true;
}

/**
 * Constructor of a cursor.
 * @param vp %Viewport displaying the cursor.
 */
MultiCursor::MultiCursor(Viewport *vp) : BaseCursor(vp)
{
	this->ClearZPositions();
}

/** Clear the z-positions cache. */
void MultiCursor::ClearZPositions()
{
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 10; y++) {
			this->zpos[x][y] = -1;
		}
	}
}

/**
 * Reset the cached height of a voxel in the area.
 * @param pos Position to reset (may be outside the cursor).
 */
void MultiCursor::ResetZPosition(const Point32 &pos)
{
	if (this->rect.IsPointInside(pos)) {
		this->zpos[pos.x - this->rect.base.x][pos.y - this->rect.base.y] = -1;
	}
}

/**
 * Get the Z position of a tile within the cursor.
 * @param xpos Horizontal world position.
 * @param ypos Vertical world position.
 * @return Height of the ground (which is the z-position of the cursor at the position).
 * @pre World position is inside the cursor rectangle.
 * @pre World position is inside the world boundaries.
 */
uint8 MultiCursor::GetZpos(int xpos, int ypos)
{
	int xoff = xpos - this->rect.base.x;
	int yoff = ypos - this->rect.base.y;
	assert(xoff >= 0 && xoff < (int)this->rect.width);
	assert(yoff >= 0 && yoff < (int)this->rect.height);

	if (this->zpos[xoff][yoff] < 0) this->zpos[xoff][yoff] = _world.GetGroundHeight(xpos, ypos);
	return this->zpos[xoff][yoff];
}

void MultiCursor::MarkDirty()
{
	if (this->type == CUR_TYPE_INVALID) return;

	for (uint x = 0; x < this->rect.width; x++) {
		for (uint y = 0; y < this->rect.height; y++) {
			this->vp->MarkVoxelDirty(this->rect.base.x + x, this->rect.base.y + y,
					this->GetZpos(this->rect.base.x + x, this->rect.base.y + y));
		}
	}
}

CursorType MultiCursor::GetCursor(const XYZPoint16 &cursor_pos)
{
	if (this->type == CUR_TYPE_INVALID) return CUR_TYPE_INVALID;

	Point32 pt(cursor_pos.x, cursor_pos.y);
	if (!this->rect.IsPointInside(pt)) return CUR_TYPE_INVALID;
	if (cursor_pos.z != this->GetZpos(cursor_pos.x, cursor_pos.y)) return CUR_TYPE_INVALID;
	return CUR_TYPE_TILE;
}

uint8 MultiCursor::GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->type == CUR_TYPE_INVALID) return zpos;

	Point32 pt(xpos, ypos);
	if (!this->rect.IsPointInside(pt)) return zpos;
	return std::max(zpos, this->GetZpos(xpos, ypos));
}

/**
 * Set the area covered by the area cursor.
 * @param rect Rectangle denoting the size and position of the area cursor.
 * @param type #CUR_TYPE_TILE for stating a new tile cursor, #CUR_TYPE_INVALID for disabling the cursor.
 * @param always Always set the cursor (else, only set it if it changed).
 * @return Whether the function has set the cursor.
 * @note The \a rect should be non-empty, and less than 10x10 tiles.
 */
bool MultiCursor::SetCursor(const Rectangle32 &rect, CursorType type, bool always)
{
	if (type == CUR_TYPE_INVALID) {
		if (!always && this->type == CUR_TYPE_INVALID) return false;
		this->MarkDirty();
		this->type = type;
		return true;
	}
	assert(type == CUR_TYPE_TILE);

	/* Copy and sanitize cursor. */
	Rectangle32 r(rect);
	r.RestrictTo(0, 0, static_cast<int>(_world.GetXSize()), static_cast<int>(_world.GetYSize()));
	if (r.width > 10) r.width = 10;
	if (r.height > 10) r.height = 10;
	if (r.width == 0 || r.height == 0) {
		// Empty tile cursor is invalid.
		if (!always && this->type == CUR_TYPE_INVALID) return false;
		this->MarkDirty();
		this->type = CUR_TYPE_INVALID;
		return true;
	}

	if (!always && this->rect == r) return false;

	this->MarkDirty();
	this->rect = r;
	this->ClearZPositions();
	this->type = type;
	this->MarkDirty();
	return true;
}

/**
 * Constructor of an edge cursor.
 * @param vp %Viewport displaying the cursor.
 */
EdgeCursor::EdgeCursor(Viewport *vp) : BaseCursor(vp)
{
	this->cursor_pos = XYZPoint16(0, 0, 0);
}

void EdgeCursor::MarkDirty()
{
	if (this->type != CUR_TYPE_INVALID) this->vp->MarkVoxelDirty(this->cursor_pos.x, this->cursor_pos.y, this->cursor_pos.z);
}

/**
 * Get a cursor.
 * @param cursor_pos Expected coordinate of the cursor.
 * @return The cursor sprite if the cursor exists and the coordinates are correct, else \c nullptr.
 */
CursorType EdgeCursor::GetCursor(const XYZPoint16 &cursor_pos)
{
	if (this->cursor_pos != cursor_pos) return CUR_TYPE_INVALID;
	return this->type;
}

uint8 EdgeCursor::GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->type == CUR_TYPE_INVALID) return zpos;
	if (this->cursor_pos.x != xpos || this->cursor_pos.y != ypos || zpos >= this->cursor_pos.z) return zpos;
	return this->cursor_pos.z;
}

/**
 * Set a cursor.
 * @param cursor_pos Position of the voxel containing the cursor.
 * @param type Type of cursor to set.
 * @param sprite Sprite to display.
 * @param yoffset Offset in screen y coordinates for where to render the sprite related to given voxel.
 * @param always Always set the cursor (else, only set it if it changed).
 * @return %Cursor has been set/changed.
 */
bool EdgeCursor::SetCursor(const XYZPoint16 &cursor_pos, CursorType type, const ImageData *sprite, uint8 yoffset, bool always)
{
	if (!always && this->cursor_pos == cursor_pos && this->type == type) return false;
	this->MarkDirty();
	this->cursor_pos = cursor_pos;
	this->type = type;
	this->sprite = sprite;
	this->yoffset = yoffset;
	this->MarkDirty();
	return true;
}


/**
 * Get the cursor type at a given position.
 * @param xpos X position of the voxel being drawn.
 * @param ypos Y position of the voxel being drawn.
 * @param zpos Z position of the voxel being drawn.
 * @return %Cursor type at the position (\c CUR_TYPE_INVALID means no cursor available).
 */
CursorType Viewport::GetCursorAtPos(uint16 xpos, uint16 ypos, uint8 zpos)
{
	CursorType ct = CUR_TYPE_INVALID;

	XYZPoint16 cursor_pos(xpos, ypos, zpos);
	if (this->additions_enabled && !this->additions_displayed) {
		ct = this->arrow_cursor.GetCursor(cursor_pos);
		if (ct != CUR_TYPE_INVALID) return ct;
	}
	ct = this->tile_cursor.GetCursor(cursor_pos);
	if (ct != CUR_TYPE_INVALID) return ct;
	ct = this->area_cursor.GetCursor(cursor_pos);
	if (ct != CUR_TYPE_INVALID) return ct;
	return this->edge_cursor.GetCursor(cursor_pos);
}

/**
 * Get the highest voxel that should be examined in a voxel stack (since cursors may be placed above all voxels).
 * @param xpos X position of the voxel stack.
 * @param ypos Y position of the voxel stack.
 * @param zpos Z position of the top voxel.
 * @return Highest voxel to draw.
 */
uint8 Viewport::GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->additions_enabled && !this->additions_displayed) {
		zpos = this->arrow_cursor.GetMaxCursorHeight(xpos, ypos, zpos);
	}
	zpos = this->tile_cursor.GetMaxCursorHeight(xpos, ypos, zpos);
	zpos = this->area_cursor.GetMaxCursorHeight(xpos, ypos, zpos);
	zpos = this->edge_cursor.GetMaxCursorHeight(xpos, ypos, zpos);
	assert(zpos != 255);
	return zpos;
}

/**
 * Get the cursor sprite at a given voxel.
 * @param xpos X position of the voxel being drawn.
 * @param ypos Y position of the voxel being drawn.
 * @param zpos Z position of the voxel being drawn.
 * @param tslope Slope of the tile.
 * @param yoffset Offset in screen y coordinates for where to render the sprite related to given voxel.
 * @return Pointer to the cursor sprite, or \c nullptr if no cursor available.
 */
const ImageData *SpriteCollector::GetCursorSpriteAtPos(uint16 xpos, uint16 ypos, uint8 zpos, uint8 tslope, uint8 &yoffset)
{
	yoffset = 0;
	if (!this->enable_cursors) return nullptr;

	CursorType ctype = this->vp->GetCursorAtPos(xpos, ypos, zpos);
	switch (ctype) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST: {
			ViewOrientation vor = SubtractOrientations((ViewOrientation)(ctype - CUR_TYPE_NORTH), this->orient);
			return this->sprites->GetCornerSprite(tslope, this->orient, vor);
		}

		case CUR_TYPE_TILE:
			return this->sprites->GetCursorSprite(tslope, this->orient);

		case CUR_TYPE_ARROW_NE:
		case CUR_TYPE_ARROW_SE:
		case CUR_TYPE_ARROW_SW:
		case CUR_TYPE_ARROW_NW:
			return this->sprites->GetArrowSprite(ctype - CUR_TYPE_ARROW_NE, this->orient);

		case CUR_TYPE_EDGE_NE:
		case CUR_TYPE_EDGE_SE:
		case CUR_TYPE_EDGE_SW:
		case CUR_TYPE_EDGE_NW: {
			Viewport *vp = GetViewport();
			if (vp != nullptr) {
				yoffset = vp->edge_cursor.yoffset;
				return vp->edge_cursor.sprite;
			}
			return nullptr;
		}

		default:
			return nullptr;
	}
}

void SpriteCollector::SetupSupports(const VoxelStack *stack, uint xpos, uint ypos)
{
	for (uint i = 0; i < stack->height; i++) {
		const Voxel *v = &stack->voxels[i];
		if (v->GetGroundType() == GTP_INVALID) continue;
		if (v->GetInstance() == SRI_FREE) {
			this->ground_height = stack->base + i;
			this->ground_slope = _slope_rotation[v->GetGroundSlope()][this->orient];
			return;
		}
		break;
	}
	this->ground_height = -1;
	return;
}

/**
 * Prepare to add the sprite for a ride.
 * @param slice Depth of this sprite.
 * @param zpos Z position of the voxel being drawn.
 * @param basex X position of the sprite in the screen.
 * @param basey Y position of the sprite in the screen.
 * @param orient View orientation.
 * @param number Ride instance number.
 * @param voxel_number Number of the voxel.
 * @param dd [out] Data to draw (4 entries).
 * @param platform [out] Shape of the support platform, if needed. @see PathSprites
 * @return The number of \a dd entries filled.
 */
static int DrawRide(int32 slice, int zpos, int32 basex, int32 basey, ViewOrientation orient, uint16 number, uint16 voxel_number, DrawData *dd, uint8 *platform)
{
	const RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr) return 0;
	/* Shops are connected in every direction. */
	if (platform != nullptr) *platform = (ri->GetKind() == RTK_SHOP) ? PATH_NE_NW_SE_SW : PATH_INVALID;

	const ImageData *sprites[4];
	ri->GetSprites(voxel_number, orient, sprites);

	int idx = 0;
	static const SpriteOrder sprite_numbers[4] = {SO_PLATFORM_BACK, SO_RIDE, SO_RIDE_FRONT, SO_PLATFORM_FRONT};
	for (int i = 0; i < 4; i++) {
		if (sprites[i] == nullptr) continue;

		dd[idx].level = slice;
		dd[idx].z_height = zpos;
		dd[idx].order = sprite_numbers[i];
		dd[idx].sprite = sprites[i];
		dd[idx].base.x = basex;
		dd[idx].base.y = basey;
		dd[idx].recolour = &ri->recolours;
		idx++;
	}
	return idx;
}

/**
 * Add all sprites of the voxel to the set of sprites to draw.
 * @param voxel %Voxel to add, \c nullptr means 'cursor above stack'.
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 * @todo Can we gain time by checking for cursors once at every voxel stack, and only test every \a zpos when there is one in a stack?
 */
void SpriteCollector::CollectVoxel(const Voxel *voxel, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth)
{
	int32 slice;
	switch (this->orient) {
		case 0: slice =  xpos + ypos; break;
		case 1: slice =  xpos - ypos; break;
		case 2: slice = -xpos - ypos; break;
		case 3: slice = -xpos + ypos; break;
		default: NOT_REACHED();
	}

	if (voxel == nullptr) { // Draw cursor above stack.
		uint8 yoffset = 0;
		const ImageData *mspr = this->GetCursorSpriteAtPos(xpos, ypos, zpos, SL_FLAT, yoffset);
		if (mspr != nullptr) {
			DrawData dd;
			dd.level = slice;
			dd.z_height = zpos;
			dd.order = SO_CURSOR;
			dd.sprite = mspr;
			dd.base.x = this->xoffset + xnorth - this->rect.base.x;
			dd.base.y = this->yoffset + ynorth - this->rect.base.y + yoffset;
			dd.recolour = nullptr;
			this->draw_images.insert(dd);
		}
		return;
	}

	uint8 platform_shape = PATH_INVALID;
	SmallRideInstance sri = voxel->GetInstance();
	uint16 instance_data = voxel->GetInstanceData();
	if (sri == SRI_PATH && HasValidPath(instance_data)) { // A path (and not something reserved above it).
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_PATH;
		platform_shape = _path_rotation[GetImplodedPathSlope(instance_data)][this->orient];
		dd.sprite = this->sprites->GetPathSprite(GetPathType(instance_data), GetImplodedPathSlope(instance_data), this->orient);
		dd.base.x = this->xoffset + xnorth - this->rect.base.x;
		dd.base.y = this->yoffset + ynorth - this->rect.base.y;
		dd.recolour = nullptr;
		this->draw_images.insert(dd);
	} else if (sri >= SRI_FULL_RIDES) { // A normal ride.
		DrawData dd[4];
		int count = DrawRide(slice, zpos,
				this->xoffset + xnorth - this->rect.base.x, this->yoffset + ynorth - this->rect.base.y,
				this->orient, sri, instance_data, dd, &platform_shape);
		for (int i = 0; i < count; i++) this->draw_images.insert(dd[i]);
	}

	/* Foundations. */
	if (voxel->GetFoundationType() != FDT_INVALID) {
		uint8 fslope = voxel->GetFoundationSlope();
		uint8 sw, se; // SW foundations, SE foundations.
		switch (this->orient) {
			case VOR_NORTH: sw = GB(fslope, 4, 2); se = GB(fslope, 2, 2); break;
			case VOR_EAST:  sw = GB(fslope, 6, 2); se = GB(fslope, 4, 2); break;
			case VOR_SOUTH: sw = GB(fslope, 0, 2); se = GB(fslope, 6, 2); break;
			case VOR_WEST:  sw = GB(fslope, 2, 2); se = GB(fslope, 0, 2); break;
			default: NOT_REACHED();
		}
		const Foundation *fnd = &this->sprites->foundation[FDT_GROUND];
		if (sw != 0) {
			const ImageData *img = fnd->sprites[3 + sw - 1];
			if (img != nullptr) {
				DrawData dd;
				dd.level = slice;
				dd.z_height = zpos;
				dd.order = SO_FOUNDATION;
				dd.sprite = img;
				dd.base.x = this->xoffset + xnorth - this->rect.base.x;
				dd.base.y = this->yoffset + ynorth - this->rect.base.y;
				dd.recolour = nullptr;
				this->draw_images.insert(dd);
			}
		}
		if (se != 0) {
			const ImageData *img = fnd->sprites[se - 1];
			if (img != nullptr) {
				DrawData dd;
				dd.level = slice;
				dd.z_height = zpos;
				dd.order = SO_FOUNDATION;
				dd.sprite = img;
				dd.base.x = this->xoffset + xnorth - this->rect.base.x;
				dd.base.y = this->yoffset + ynorth - this->rect.base.y;
				dd.recolour = nullptr;
				this->draw_images.insert(dd);
			}
		}
	}

	/* Ground surface. */
	uint8 gslope = SL_FLAT;
	if (voxel->GetGroundType() != GTP_INVALID) {
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_GROUND;
		uint8 slope = voxel->GetGroundSlope();
		dd.sprite = this->sprites->GetSurfaceSprite(voxel->GetGroundType(), slope, this->orient);
		dd.base.x = this->xoffset + xnorth - this->rect.base.x;
		dd.base.y = this->yoffset + ynorth - this->rect.base.y;
		dd.recolour = nullptr;
		this->draw_images.insert(dd);
		switch (slope) {
			// XXX There are no sprites for partial support of a platform.
			case SL_FLAT:
				if (platform_shape < PATH_FLAT_COUNT) platform_shape = PATH_INVALID;
				break;

			case TSB_SOUTHWEST:
				if (platform_shape == PATH_RAMP_NE) platform_shape = PATH_INVALID;
				break;

			case TSB_SOUTHEAST:
				if (platform_shape == PATH_RAMP_NW) platform_shape = PATH_INVALID;
				break;

			case TSB_NORTHWEST:
				if (platform_shape == PATH_RAMP_SE) platform_shape = PATH_INVALID;
				break;

			case TSB_NORTHEAST:
				if (platform_shape == PATH_RAMP_SW) platform_shape = PATH_INVALID;
				break;

			default:
				platform_shape = PATH_INVALID;
				break;
		}

		gslope = voxel->GetGroundSlope();
	}

	/* Fences */
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		FenceType fence_type = voxel->GetFenceType(edge);
		if (fence_type != FENCE_TYPE_INVALID) {
			DrawData dd;
			dd.level = slice;
			dd.z_height = zpos;
			dd.order = (edge + 4 * this->orient + 1) % 4 < EDGE_SW ? SO_FENCE_BACK : SO_FENCE_FRONT;
			TileSlope slope = ExpandTileSlope(voxel->GetGroundSlope());
			dd.sprite = this->sprites->GetFenceSprite(fence_type, edge, slope, this->orient);
			int32 extra_y = 0;
			if ((slope & TSB_STEEP) != 0) {
				/* Is lower edge of steep slope? */
				uint8 corner_height[4];
				ComputeCornerHeight(slope, (slope & TSB_TOP) != 0 ? 1 : 0, corner_height);
				if (corner_height[edge] == 0 || corner_height[(edge+1) % 4] == 0) {

					extra_y = this->tile_height;
				}
 			}
			dd.base.x = this->xoffset + xnorth - this->rect.base.x;
			dd.base.y = this->yoffset + ynorth - this->rect.base.y + extra_y;
			dd.recolour = nullptr;
			this->draw_images.insert(dd);
		}
	}

	/* Sprite cursor (arrow) */
	uint8 cursor_yoffset = 0;
	const ImageData *mspr = this->GetCursorSpriteAtPos(xpos, ypos, zpos, gslope, cursor_yoffset);
	if (mspr != nullptr) {
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_CURSOR;
		dd.sprite = mspr;
		dd.base.x = this->xoffset + xnorth - this->rect.base.x;
		dd.base.y = this->yoffset + ynorth - this->rect.base.y + cursor_yoffset;
		dd.recolour = nullptr;
		this->draw_images.insert(dd);
	}

	/* Add platforms. */
	if (platform_shape != PATH_INVALID) {
		/* Platform gets automatically added when drawing a path or ride, without drawing ground. */
		ImageData *pl_spr;
		switch (platform_shape) {
			case PATH_RAMP_NE: pl_spr = this->sprites->platform.ramp[2]; break;
			case PATH_RAMP_NW: pl_spr = this->sprites->platform.ramp[1]; break;
			case PATH_RAMP_SE: pl_spr = this->sprites->platform.ramp[3]; break;
			case PATH_RAMP_SW: pl_spr = this->sprites->platform.ramp[0]; break;
			default: pl_spr = this->sprites->platform.flat[this->orient & 1]; break;
		}
		if (pl_spr != nullptr) {
			DrawData dd;
			dd.level = slice;
			dd.z_height = zpos;
			dd.order = SO_PLATFORM;
			dd.sprite = pl_spr;
			dd.base.x = this->xoffset + xnorth - this->rect.base.x;
			dd.base.y = this->yoffset + ynorth - this->rect.base.y;
			dd.recolour = nullptr;
			this->draw_images.insert(dd);
		}

		/* XXX Use the shape to draw handle bars. */

		/* Add supports. */
		uint16 height = this->ground_height;
		this->ground_height = -1;
		uint8 slope = this->ground_slope;
		while (height < zpos) {
			int yoffset = (zpos - height) * this->vp->tile_height; // Compensate y position of support.
			uint sprnum;
			if (slope == SL_FLAT) {
				if (height + 1 < zpos) {
					sprnum = SSP_FLAT_DOUBLE_NS + (this->orient & 1);
					height += 2;
				} else {
					sprnum = SSP_FLAT_SINGLE_NS + (this->orient & 1);
					height += 1;
				}
			} else {
				if (slope >= 15) { // Imploded steep slope.
					sprnum = SSP_STEEP_N + ((slope - 15) + 2) % 4;
					height += 2;
				} else {
					sprnum = SSP_NONFLAT_BASE + 15 - slope;
					height++;
				}
				slope = SL_FLAT;
			}
			ImageData *img = this->sprites->support.sprites[sprnum];
			if (img != nullptr) {
				DrawData dd;
				dd.level = slice;
				dd.z_height = height;
				dd.order = SO_SUPPORT;
				dd.sprite = img;
				dd.base.x = this->xoffset + xnorth - this->rect.base.x;
				dd.base.y = this->yoffset + ynorth - this->rect.base.y + yoffset;
				dd.recolour = nullptr;
				this->draw_images.insert(dd);
			}
		}
	}

	const VoxelObject *vo = voxel->voxel_objects;
	while (vo != nullptr) {
		DrawData dd;
		const ImageData *anim_spr = vo->GetSprite(this->sprites, this->orient, &dd.recolour);
		if (anim_spr != nullptr) {
			int x_off = ComputeX(vo->x_pos, vo->y_pos);
			int y_off = ComputeY(vo->x_pos, vo->y_pos, vo->z_pos);
			dd.level = slice;
			dd.z_height = zpos;
			dd.order = SO_PERSON;
			dd.sprite = anim_spr;
			dd.base.x = this->xoffset + this->north_offsets[this->orient].x + xnorth - this->rect.base.x + x_off;
			dd.base.y = this->yoffset + this->north_offsets[this->orient].y + ynorth - this->rect.base.y + y_off;
			this->draw_images.insert(dd);
		}
		vo = vo->next_object;
	}
}

/**
 * Constructor of the finder data class.
 * @param allowed Bit-set of sprite types to look for. @see #ClickableSprite
 * @param select What to find of a ground tile.
 */
FinderData::FinderData(ClickableSprite allowed, GroundTilePart select)
{
	this->allowed = allowed;
	this->select = select;

	/*
	 * CS_GROUND must not be allowed when looking for edge,
	 * or the other way around.
	 */
	assert(select != FW_EDGE || (allowed & SO_GROUND) == 0);
	assert(select == FW_EDGE || (allowed & SO_GROUND_EDGE) == 0);

	// Other data is initialized in PixelFinder::PixelFinder, or Viewport::ComputeCursorPosition.
}

/**
 * Constructor of the tile position finder.
 * @param vp %Viewport that needs the tile position.
 * @param fdata Finder data.
 */
PixelFinder::PixelFinder(Viewport *vp, FinderData *fdata) : VoxelCollector(vp, false)
{
	this->allowed = fdata->allowed;
	this->found = false;
	this->pixel = _palette[0]; // 0 is transparent, and is not used in sprites.
	this->fdata = fdata;

	fdata->voxel_pos = XYZPoint16(0, 0, 0);
	fdata->person = nullptr;
	fdata->ride   = INVALID_RIDE_INSTANCE;
}

PixelFinder::~PixelFinder()
{
}

/**
 * Find the closest sprite.
 * @param voxel %Voxel to examine, \c nullptr means 'cursor above stack'.
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 */
void PixelFinder::CollectVoxel(const Voxel *voxel, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth)
{
	int32 slice;
	switch (this->orient) {
		case 0: slice =  xpos + ypos; break;
		case 1: slice =  xpos - ypos; break;
		case 2: slice = -xpos - ypos; break;
		case 3: slice = -xpos + ypos; break;
		default: NOT_REACHED();
	}
	/* Looking for surface edge? */
	if ((this->allowed & SO_GROUND_EDGE) != 0 && voxel->GetGroundType() != GTP_INVALID) {
		const ImageData *spr = this->sprites->GetSurfaceSprite(GTP_CURSOR_EDGE_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_GROUND_EDGE;
		dd.sprite = nullptr;
		dd.base.x = this->rect.base.x - xnorth;
		dd.base.y = this->rect.base.y - ynorth;
		dd.recolour = nullptr;
		if (spr != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = spr->GetPixel(dd.base.x - spr->xoffset, dd.base.y - spr->yoffset);
			if (pixel != 0) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos.x = xpos;
				this->fdata->voxel_pos.y = ypos;
				this->fdata->voxel_pos.z = zpos;
				this->pixel = pixel;
			}
		}
	}

	if (voxel == nullptr) return; // Ignore cursors, they are not clickable.

	SmallRideInstance number = voxel->GetInstance();
	if ((this->allowed & CS_RIDE) != 0 && number >= SRI_FULL_RIDES) {
		/* Looking for a ride? */
		DrawData dd[4];
		int count = DrawRide(slice, zpos, this->rect.base.x - xnorth, this->rect.base.y - ynorth,
				this->orient, number, voxel->GetInstanceData(), dd, nullptr);
		for (int i = 0; i < count; i++) {
			if (!this->found || this->data < dd[i]) {
				const ImageData *img = dd[i].sprite;
				uint32 pixel = img->GetPixel(dd[i].base.x - img->xoffset, dd[i].base.y - img->yoffset);
				if (GetA(pixel) != TRANSPARENT) {
					this->found = true;
					this->data = dd[i];
					this->fdata->voxel_pos.x = xpos;
					this->fdata->voxel_pos.y = ypos;
					this->fdata->voxel_pos.z = zpos;
					this->pixel = pixel;
					this->fdata->ride = number;
				}
			}
		}
	} else if ((this->allowed & CS_PATH) != 0 && HasValidPath(voxel)) {
		/* Looking for a path? */
		uint16 instance_data = voxel->GetInstanceData();
		const ImageData *img = this->sprites->GetPathSprite(GetPathType(instance_data), GetImplodedPathSlope(instance_data), this->orient);
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_PATH;
		dd.sprite = nullptr;
		dd.base.x = this->rect.base.x - xnorth;
		dd.base.y = this->rect.base.y - ynorth;
		dd.recolour = nullptr;
		if (img != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = img->GetPixel(dd.base.x - img->xoffset, dd.base.y - img->yoffset);
			if (GetA(pixel) != TRANSPARENT) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos.x = xpos;
				this->fdata->voxel_pos.y = ypos;
				this->fdata->voxel_pos.z = zpos;
				this->pixel = pixel;
			}
		}
	} else if ((this->allowed & CS_GROUND) != 0 && voxel->GetGroundType() != GTP_INVALID) {
		/* Looking for surface? */
		const ImageData *spr = this->sprites->GetSurfaceSprite(GTP_CURSOR_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.level = slice;
		dd.z_height = zpos;
		dd.order = SO_GROUND;
		dd.sprite = nullptr;
		dd.base.x = this->rect.base.x - xnorth;
		dd.base.y = this->rect.base.y - ynorth;
		dd.recolour = nullptr;
		if (spr != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = spr->GetPixel(dd.base.x - spr->xoffset, dd.base.y - spr->yoffset);
			if (GetA(pixel) != TRANSPARENT) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos.x = xpos;
				this->fdata->voxel_pos.y = ypos;
				this->fdata->voxel_pos.z = zpos;
				this->pixel = pixel;
			}
		}
	} else if ((this->allowed & CS_PERSON) != 0) {
		/* Looking for persons? */
		const VoxelObject *vo = voxel->voxel_objects;
		while (vo != nullptr) {
			const Person *pers = static_cast<const Person *>(vo);
			assert(pers != nullptr && pers->walk != nullptr);
			AnimationType anim_type = pers->walk->anim_type;
			const ImageData *anim_spr = this->sprites->GetAnimationSprite(anim_type, pers->frame_index, pers->type, this->orient);
			int x_off = ComputeX(pers->x_pos, pers->y_pos);
			int y_off = ComputeY(pers->x_pos, pers->y_pos, pers->z_pos);
			DrawData dd;
			dd.level = slice;
			dd.z_height = zpos;
			dd.order = SO_PERSON;
			dd.sprite = nullptr;
			dd.base.x = this->rect.base.x - xnorth - x_off;
			dd.base.y = this->rect.base.y - ynorth - y_off;
			dd.recolour = nullptr;
			if (anim_spr && (!this->found || this->data < dd)) {
				uint32 pixel = anim_spr->GetPixel(dd.base.x - anim_spr->xoffset, dd.base.y - anim_spr->yoffset);
				if (GetA(pixel) != TRANSPARENT) {
					this->found = true;
					this->data = dd;
					this->fdata->voxel_pos.x = xpos;
					this->fdata->voxel_pos.y = ypos;
					this->fdata->voxel_pos.z = zpos;
					this->pixel = pixel;
					this->fdata->person = pers;
				}
			}
			vo = vo->next_object;
		}
	}
}

/**
 * %Viewport constructor.
 * @param xview X pixel position of the center viewpoint of the main display.
 * @param yview Y pixel position of the center viewpoint of the main display.
 * @param zview Z pixel position of the center viewpoint of the main display.
 */
Viewport::Viewport(uint32 xview, uint32 yview, uint32 zview) : Window(WC_MAINDISPLAY, ALL_WINDOWS_OF_TYPE), tile_cursor(this), arrow_cursor(this), area_cursor(this), edge_cursor(this)
{
	this->xview = xview;
	this->yview = yview;
	this->zview = zview;

	this->tile_width  = 64;
	this->tile_height = 16;
	this->orientation = VOR_NORTH;

	_mouse_modes.main_display = this;

	this->mouse_pos.x = 0;
	this->mouse_pos.y = 0;
	this->additions_enabled = false;
	this->additions_displayed = false;

	uint16 width  = _video.GetXSize();
	uint16 height = _video.GetYSize();
	assert(width >= 120 && height >= 120); // Arbitrary lower limit as sanity check.

	this->SetSize(width, height);
	this->SetPosition(0, 0);
}

Viewport::~Viewport()
{
	_mouse_modes.main_display = nullptr;
}

/**
 * Get relative X position of a point in the world (used for marking window parts dirty).
 * @param xpos X world position.
 * @param ypos Y world position.
 * @return Relative X position.
 */
int32 Viewport::ComputeX(int32 xpos, int32 ypos)
{
	return ComputeXFunction(xpos, ypos, this->orientation, this->tile_width);
}

/**
 * Get relative Y position of a point in the world (used for marking window parts dirty).
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @return Relative Y position.
 */
int32 Viewport::ComputeY(int32 xpos, int32 ypos, int32 zpos)
{
	return ComputeYFunction(xpos, ypos, zpos, this->orientation, this->tile_width, this->tile_height);
}

void Viewport::OnDraw()
{
	SpriteCollector collector(this, _mouse_modes.current->EnableCursors());
	collector.SetWindowSize(-(int16)this->rect.width / 2, -(int16)this->rect.height / 2, this->rect.width, this->rect.height);
	collector.Collect(this->additions_enabled && this->additions_displayed);
	static const Recolouring recolour;

	_video.FillRectangle(this->rect, MakeRGBA(0, 0, 0, OPAQUE)); // Black background.

	ClippedRectangle cr = _video.GetClippedRectangle();
	assert(this->rect.base.x >= 0 && this->rect.base.y >= 0);
	ClippedRectangle draw_rect(cr, this->rect.base.x, this->rect.base.y, this->rect.width, this->rect.height);
	_video.SetClippedRectangle(draw_rect);

	GradientShift gs = static_cast<GradientShift>(GS_LIGHT - _weather.GetWeatherType());
	for (const auto &iter : collector.draw_images) {
		const DrawData &dd = iter;
		const Recolouring &rec = (dd.recolour == nullptr) ? recolour : *dd.recolour;
		_video.BlitImage(dd.base, dd.sprite, rec, gs);
	}

	_video.SetClippedRectangle(cr);
}

/**
 * Mark a voxel as in need of getting painted.
 * @param xpos X position of the voxel.
 * @param ypos Y position of the voxel.
 * @param zpos Z position of the voxel.
 * @param height Number of voxels to mark above the specified coordinate (\c 0 means inspect the voxel itself).
 */
void Viewport::MarkVoxelDirty(int16 xpos, int16 ypos, int16 zpos, int16 height)
{
	if (height <= 0) {
		const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
		if (v == nullptr) {
			height = 1;
		} else {
			height = 1; // There are no steep slopes, so 1 covers paths already.
			if (v->GetGroundType() != GTP_INVALID) {
				if (IsImplodedSteepSlope(v->GetGroundSlope())) height = 2;
			}
			SmallRideInstance number = v->GetInstance();
			if (number >= SRI_FULL_RIDES) { // A ride.
				const RideInstance *ri = _rides_manager.GetRideInstance(number);
				if (ri != nullptr) {
					switch (ri->GetKind()) {
						case RTK_SHOP: {
							const ShopInstance *si = static_cast<const ShopInstance *>(ri);
							height = si->GetShopType()->height;
							break;
						}

						default:
							break;
					}
				}
			}
		}
	}

	Rectangle32 rect;
	const Point16 *pt;

	int32 center_x = this->ComputeX(this->xview, this->yview) - this->rect.base.x - this->rect.width / 2;
	int32 center_y = this->ComputeY(this->xview, this->yview, this->zview) - this->rect.base.y - this->rect.height / 2;

	pt = &_corner_dxy[this->orientation];
	rect.base.y = this->ComputeY((xpos + pt->x) * 256, (ypos + pt->y) * 256, (zpos + height) * 256) - center_y;

	pt = &_corner_dxy[RotateCounterClockwise(this->orientation)];
	rect.base.x = this->ComputeX((xpos + pt->x) * 256, (ypos + pt->y) * 256) - center_x;

	pt = &_corner_dxy[RotateClockwise(this->orientation)];
	int32 d = this->ComputeX((xpos + pt->x) * 256, (ypos + pt->y) * 256) - center_x;
	assert(d >= rect.base.x);
	rect.width = d - rect.base.x + 1;

	pt = &_corner_dxy[RotateClockwise(RotateClockwise(this->orientation))];
	d = this->ComputeY((xpos + pt->x) * 256, (ypos + pt->y) * 256, zpos * 256) - center_y;
	assert(d >= rect.base.y);
	rect.height = d - rect.base.y + 1;

	_video.MarkDisplayDirty(rect);
}

/**
 * Compute position of the mouse cursor, and return the result.
 * @param fdata [inout] Parameters and results of the finding process.
 * @return Found type of sprite.
 */
ClickableSprite Viewport::ComputeCursorPosition(FinderData *fdata)
{
	int16 xp = this->mouse_pos.x - this->rect.width / 2;
	int16 yp = this->mouse_pos.y - this->rect.height / 2;
	PixelFinder collector(this, fdata);
	collector.SetWindowSize(xp, yp, 1, 1);
	collector.Collect(false);
	if (!collector.found) return CS_NONE;

	fdata->cursor = fdata->select == FW_EDGE ? CUR_TYPE_EDGE_NE : CUR_TYPE_TILE;
	if (fdata->select == FW_CORNER && (collector.data.order & CS_MASK) == CS_GROUND) {
		if (collector.pixel == _palette[181]) {
			fdata->cursor = (CursorType)AddOrientations(VOR_NORTH, this->orientation);
		} else if (collector.pixel == _palette[182]) {
			fdata->cursor = (CursorType)AddOrientations(VOR_EAST,  this->orientation);
		} else if (collector.pixel == _palette[184]) {
			fdata->cursor = (CursorType)AddOrientations(VOR_WEST,  this->orientation);
		} else if (collector.pixel == _palette[185]) {
			fdata->cursor = (CursorType)AddOrientations(VOR_SOUTH, this->orientation);
		}
	}
	else if (fdata->select == FW_EDGE && (collector.data.order & CS_MASK) == CS_GROUND_EDGE) {
		uint8 base_edge = EDGE_COUNT;
		if (collector.pixel == _palette[181]) {
			base_edge = (uint8)EDGE_NE;
		} else if (collector.pixel == _palette[182]) {
			base_edge = (uint8)EDGE_SE;
		} else if (collector.pixel == _palette[184]) {
			base_edge = (uint8)EDGE_NW;
		} else if (collector.pixel == _palette[185]) {
			base_edge = (uint8)EDGE_SW;
		}
		if (base_edge < EDGE_COUNT) {
			fdata->cursor = (CursorType)((base_edge + (uint8)this->orientation) % 4 + (uint8)CUR_TYPE_EDGE_NE);
		}
	}
	return (ClickableSprite)(collector.data.order & CS_MASK);
}

/**
 * Rotate 90 degrees clockwise or anti-clockwise.
 * @param direction Direction of turn (positive means clockwise).
 */
void Viewport::Rotate(int direction)
{
	this->orientation = (ViewOrientation)((this->orientation + VOR_NUM_ORIENT + ((direction > 0) ? 1 : -1)) % VOR_NUM_ORIENT);
	Point16 pt = this->mouse_pos;
	this->OnMouseMoveEvent(pt);
	this->MarkDirty();

	NotifyChange(WC_PATH_BUILDER, ALL_WINDOWS_OF_TYPE, CHG_VIEWPORT_ROTATED, direction);
}

/**
 * Compute the horizontal translation in world coordinates of the viewing centre to move it \a dx / \a dy pixels.
 * @param dx Horizontal shift in screen pixels.
 * @param dy Vertical shift in screen pixels.
 * @return New X and Y coordinates of the centre point.
 */
Point32 Viewport::ComputeHorizontalTranslation(int dx, int dy)
{
	Point32 new_xy;
	switch (this->orientation) {
		case VOR_NORTH:
			new_xy.x = this->xview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_xy.y = this->yview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		case VOR_EAST:
			new_xy.x = this->xview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_xy.y = this->yview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_SOUTH:
			new_xy.x = this->xview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_xy.y = this->yview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_WEST:
			new_xy.x = this->xview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_xy.y = this->yview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		default:
			NOT_REACHED();
	}
	return new_xy;
}

/**
 * Move the viewport a number of screen pixels.
 * @param dx Horizontal shift in screen pixels.
 * @param dy Vertical shift in screen pixels.
 */
void Viewport::MoveViewport(int dx, int dy)
{
	if (dx == 0 && dy == 0) return;

	Point32 new_xy = this->ComputeHorizontalTranslation(dx, dy);
	int32 new_x = Clamp<int32>(new_xy.x, 0, _world.GetXSize() * 256 - 1);
	int32 new_y = Clamp<int32>(new_xy.y, 0, _world.GetYSize() * 256 - 1);
	if (new_x != this->xview || new_y != this->yview) {
		this->xview = new_x;
		this->yview = new_y;
		this->MarkDirty();
	}
}

void Viewport::OnMouseMoveEvent(const Point16 &pos)
{
	Point16 old_mouse_pos = this->mouse_pos;
	this->mouse_pos = pos;

	_mouse_modes.current->OnMouseMoveEvent(this, old_mouse_pos, pos);
}

WmMouseEvent Viewport::OnMouseButtonEvent(uint8 state)
{
	_mouse_modes.current->OnMouseButtonEvent(this, state);
	return WMME_NONE;
}

void Viewport::OnMouseWheelEvent(int direction)
{
	_mouse_modes.current->OnMouseWheelEvent(this, direction);
}

/**
 * Constructor of a mouse mode.
 * @param p_wtype Window associated with the mouse mode (use #WC_NONE if no window).
 * @param p_mode Mouse mode implemented by the object.
 */
MouseMode::MouseMode(WindowTypes p_wtype, ViewportMouseMode p_mode) : wtype(p_wtype), mode(p_mode)
{
}

MouseMode::~MouseMode()
{
}

/**
 * The mouse moved while in this mouse mode.
 * @param vp %Viewport object.
 * @param old_pos Previous position.
 * @param pos Current position.
 */
void MouseMode::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
}

/**
 * A mouse click was detected.
 * @param vp %Viewport object.
 * @param state State of the mouse buttons. @see MouseButtons
 */
void MouseMode::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
}

/**
 * A mouse wheelie event has been detected.
 * @param vp %Viewport object.
 * @param direction Direction of movement.
 */
void MouseMode::OnMouseWheelEvent(Viewport *vp, int direction)
{
}

DefaultMouseMode::DefaultMouseMode() : MouseMode(WC_NONE, MM_INACTIVE)
{
}

bool DefaultMouseMode::MayActivateMode()
{
	return true;
}

void DefaultMouseMode::ActivateMode(const Point16 &pos)
{
}

void DefaultMouseMode::LeaveMode()
{
}

bool DefaultMouseMode::EnableCursors()
{
	return false;
}

MouseModes::MouseModes()
{
	this->main_display = nullptr;
	this->current = &default_mode;
	for (int i = 0; i < MM_COUNT; i++) this->modes[i] = nullptr;
}

/**
 * Register a mouse mode.
 * @param mm Mouse mode to register.
 */
void MouseModes::RegisterMode(MouseMode *mm)
{
	assert(mm->mode < MM_COUNT);
	assert(this->modes[mm->mode] == nullptr);
	this->modes[mm->mode] = mm;
}

/**
 * Get the address of the viewport window.
 * @return Address of the viewport.
 * @note Function may return \c nullptr.
 */
Viewport *GetViewport()
{
	return _mouse_modes.main_display;
}

/**
 * Mark a voxel as in need of getting painted.
 * @param xpos X position of the voxel.
 * @param ypos Y position of the voxel.
 * @param zpos Z position of the voxel.
 * @param height Number of voxels to mark above the specified coordinate (\c 0 means inspect the voxel itself).
 */
void MarkVoxelDirty(int16 xpos, int16 ypos, int16 zpos, int16 height)
{
	Viewport *vp = GetViewport();
	if (vp != nullptr) vp->MarkVoxelDirty(xpos, ypos, zpos, height);
}

/**
 * Decide the most appropriate mouse mode of the viewport, depending on available windows.
 * @todo Perhaps force a redraw/recompute in some way to ensure the right state is displayed?
 * @todo Perhaps switch mode when a window associated with a mode is raised?
 */
void MouseModes::SetViewportMousemode()
{
	if (this->main_display == nullptr) return;

	/* First try all windows from top to bottom. */
	Window *w = _manager.top;
	while (w != nullptr) {
		for (uint i = 0; i < lengthof(this->modes); i++) {
			MouseMode *mm = this->modes[i];
			if (mm != nullptr && mm->wtype == w->wtype && mm->MayActivateMode()) {
				this->SwitchMode(mm);
				return;
			}
		}
		w = w->lower;
	}
	/* Try all mouse modes without a window. */
	for (uint i = 0; i < lengthof(this->modes); i++) {
		MouseMode *mm = this->modes[i];
		if (mm != nullptr && mm->wtype == WC_NONE && mm->MayActivateMode()) {
			this->SwitchMode(mm);
			return;
		}
	}
	/* Switch to the default mouse mode unconditionally. */
	this->SwitchMode(&default_mode);
}

/**
 * Try to switch to a given mouse mode.
 * @param mode Mode to switch to.
 */
void MouseModes::SetMouseMode(ViewportMouseMode mode)
{
	for (uint i = 0; i < lengthof(this->modes); i++) {
		MouseMode *mm = this->modes[i];
		if (mm != nullptr && mm->mode == mode) {
			if (mm->MayActivateMode()) this->SwitchMode(mm);
			break;
		}
	}
}

/**
 * Switch to a new mouse mode.
 * @param new_mode Mode to switch to.
 * @pre New mode must allow activation of its mode.
 */
void MouseModes::SwitchMode(MouseMode *new_mode)
{
	assert(new_mode->MayActivateMode());
	if (this->main_display == nullptr || new_mode == this->current) return;

	this->current->LeaveMode();
	this->current = new_mode;
	this->current->ActivateMode(this->main_display->mouse_pos);
}

/**
 * Get the current mouse mode.
 * @return The current mouse mode.
 */
ViewportMouseMode MouseModes::GetMouseMode()
{
	return this->current->mode;
}

/**
 * Open the main isometric display window.
 * @param xview X pixel position of the center viewpoint of the main display.
 * @param yview Y pixel position of the center viewpoint of the main display.
 * @param zview Z pixel position of the center viewpoint of the main display.
 * @ingroup viewport_group
 */
void ShowMainDisplay(uint32 xview, uint32 yview, uint32 zview)
{
	new Viewport(xview, yview, zview);
	_mouse_modes.SetViewportMousemode();
}

/** Initialize the mouse modes. */
void InitMouseModes()
{
	_mouse_modes.RegisterMode(&_terraformer);
	_mouse_modes.RegisterMode(&_path_builder);
	_mouse_modes.RegisterMode(&_shop_placer);
	_mouse_modes.RegisterMode(&_select_mousemode);
	_mouse_modes.RegisterMode(&_coaster_builder);
	_mouse_modes.RegisterMode(&_fence_builder);
}
