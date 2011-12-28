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

#include <map>

/**
 * Search the world for voxels to render.
 * @ingroup viewport_group
 */
class VoxelCollector {
public:
	VoxelCollector(int32 xview, int32 yview, int32 zview, uint16 width, uint16 height, ViewOrientation orient);
	virtual ~VoxelCollector();

	void SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height);

	void Collect();

	/**
	 * Convert 3D position to the horizontal 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @return X position in 2D.
	 */
	FORCEINLINE int32 ComputeX(int32 x, int32 y)
	{
		switch (this->orient) {
			case VOR_NORTH: return ((y - x)  * this->tile_width / 2) >> 8;
			case VOR_WEST:  return (-(x + y) * this->tile_width / 2) >> 8;
			case VOR_SOUTH: return ((x - y)  * this->tile_width / 2) >> 8;
			case VOR_EAST:  return ((x + y)  * this->tile_width / 2) >> 8;
			default: NOT_REACHED();
		}
	}

	/**
	 * Convert 3D position to the vertical 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @param z Z position in the game world.
	 * @return Y position in 2D.
	 */
	FORCEINLINE int32 ComputeY(int32 x, int32 y, int32 z)
	{
		switch (this->orient) {
			case VOR_NORTH: return ((x + y)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_WEST:  return ((y - x)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_SOUTH: return (-(x + y) * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_EAST:  return ((x - y)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			default: NOT_REACHED();
		}
	}

	int32 xview; ///< X position of the center point of the display.
	int32 yview; ///< Y position of the center point of the display.
	int32 zview; ///< Z position of the center point of the display.

	uint16 tile_width;            ///< Width of a tile.
	uint16 tile_height;           ///< Height of a tile.
	ViewOrientation orient;       ///< Direction of view.
	const SpriteStorage *sprites; ///< Sprite collection of the right size.

	Rectangle32 rect; ///< Screen area of interest.

protected:
	/**
	 * Handle a voxel that should be collected.
	 * @param vx   %Voxel to add.
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
 * Data temporary needed for drawing.
 * @ingroup viewport_group
 */
struct DrawData {
	const Sprite *cursor;     ///< Mouse cursor to draw.
	const Sprite *path;       ///< Path sprite to draw.
	const Sprite *ground;     ///< Surface tile to draw.
	const Sprite *foundation; ///< Foundations to draw.
	Point32 base;             ///< Base coordinate of the image, relative to top-left of the window.
};

/**
 * Map of distance to image.
 * Used for temporary sorting and storage of images drawn at the viewport.
 * @ingroup viewport_group
 */
typedef std::multimap<int32, DrawData> DrawImages;

/**
 * Collect sprites to draw in a viewport.
 * @ingroup viewport_group
 */
class SpriteCollector : public VoxelCollector {
public:
	SpriteCollector(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient);
	~SpriteCollector();

	void SetXYOffset(int16 xoffset, int16 yoffset);
	void SetMouseCursor(uint16 xpos, uint16 ypos, uint8 zpos, ViewOrientation cursor);

	DrawImages draw_images; ///< Sprites to draw ordered by viewing distance.
	int16 xoffset; ///< Horizontal offset of the top-left coordinate to the top-left of the display.
	int16 yoffset; ///< Vertical offset of the top-left coordinate to the top-left of the display.

	bool draw_mouse_cursor; ///< Add the mouse cursor to the right ground tile.
	uint16 mousex;          ///< X position of the voxel with the mouse cursor.
	uint16 mousey;          ///< Y position of the voxel with the mouse cursor.
	uint8  mousez;          ///< Z position of the voxel with the mouse cursor.
	ViewOrientation cursor; ///< Cursor type (one corner, or the entire tile).

protected:
	void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth);
};


/**
 * Find the sprite and pixel under the mouse cursor.
 * @ingroup viewport_group
 */
class PixelFinder : public VoxelCollector {
public:
	/**
	 * Constructor for collecting the sprite displayed at the mouse cursor.
	 * @param xview X world position of the origin.
	 * @param yview Y world position of the origin.
	 * @param zview Z world position of the origin.
	 * @param tile_width Width of a tile at the display.
	 * @param tile_height Height of a tile in the display.
	 * @param orient View orientation.
	 */
	PixelFinder(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient);
	~PixelFinder();

	bool   found;    ///< Found a solution.
	int32  distance; ///< Closest distance so far.
	uint8  pixel;    ///< Pixel colour of the closest sprite.
	uint16 xvoxel;   ///< X position of the voxel with the closest sprite.
	uint16 yvoxel;   ///< Y position of the voxel with the closest sprite.
	uint8  zvoxel;   ///< Z position of the voxel with the closest sprite.

protected:
	void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth);
};


/**
 * Constructor.
 * @param xview X world position of the origin.
 * @param yview Y world position of the origin.
 * @param zview Z world position of the origin.
 * @param tile_width Width of a tile at the display.
 * @param tile_height Height of a tile in the display.
 * @param orient View orientation.
 */
VoxelCollector::VoxelCollector(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient)
{
	this->xview = xview;
	this->yview = yview;
	this->zview = zview;
	this->tile_width = tile_width;
	this->tile_height = tile_height;
	this->orient = orient;

	this->sprites = _sprite_manager.GetSprites(this->tile_width);
	assert(this->sprites != NULL);
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
 * @todo Add referenced voxels map here.
 * @todo Do this less stupid. Walking the whole world is not going to work in general.
 */
void VoxelCollector::Collect()
{
	for (uint xpos = 0; xpos < _world.GetXSize(); xpos++) {
		int32 world_x = (xpos + ((this->orient == VOR_SOUTH || this->orient == VOR_WEST) ? 1 : 0)) * 256;
		for (uint ypos = 0; ypos < _world.GetYSize(); ypos++) {
			int32 world_y = (ypos + ((this->orient == VOR_SOUTH || this->orient == VOR_EAST) ? 1 : 0)) * 256;
			int32 north_x = ComputeX(world_x, world_y);
			if (north_x + this->tile_width / 2 <= (int32)this->rect.base.x) continue; // Right of voxel column is at left of window.
			if (north_x - this->tile_width / 2 >= (int32)(this->rect.base.x + this->rect.width)) continue; // Left of the window.

			VoxelStack *stack = _world.GetStack(xpos, ypos);
			for (int count = 0; count < stack->height; count++) {
				uint zpos = stack->base + count;
				int32 north_y = this->ComputeY(world_x, world_y, zpos * 256);
				if (north_y - this->tile_height >= (int32)(this->rect.base.y + this->rect.height)) continue; // Voxel is below the window.
				if (north_y + this->tile_width / 2 + this->tile_height <= (int32)this->rect.base.y) break; // Above the window and rising!

				this->CollectVoxel(&stack->voxels[count], xpos, ypos, zpos, north_x, north_y);
			}
		}
	}
}

/**
 * Constructor of sprites collector.
 */
SpriteCollector::SpriteCollector(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient) :
		VoxelCollector(xview, yview, zview, tile_width, tile_height, orient)
{
	this->draw_images.clear();
	this->draw_mouse_cursor = false;
	this->cursor = VOR_INVALID;
	this->xoffset = 0;
	this->yoffset = 0;
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
 * Set position of the mouse cursor.
 */
void SpriteCollector::SetMouseCursor(uint16 xpos, uint16 ypos, uint8 zpos, ViewOrientation cursor)
{
	this->draw_mouse_cursor = true;
	this->mousex = xpos;
	this->mousey = ypos;
	this->mousez = zpos;
	this->cursor = cursor;
}

/**
 * Add all sprites of the voxel to the set of sprites to draw.
 * @param voxel %Voxel to add.
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 */
void SpriteCollector::CollectVoxel(const Voxel *voxel, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth)
{
	int sx = (this->orient == VOR_NORTH || this->orient == VOR_EAST) ? 256 : -256;
	int sy = (this->orient == VOR_NORTH || this->orient == VOR_WEST) ? 256 : -256;

	switch (voxel->GetType()) {
		case VT_SURFACE: {
			const SurfaceVoxelData *svd = voxel->GetSurface();
			const Sprite *surf;
			if (svd->ground.type == GTP_INVALID) {
				surf = NULL;
			} else {
				surf = this->sprites->GetSurfaceSprite(svd->ground.type, svd->ground.slope, this->orient);
			}
			const Sprite *path;
			if (svd->path.type == PT_INVALID) {
				path = NULL;
			} else {
				path = this->sprites->GetPathSprite(svd->path.type, svd->path.slope, this->orient);
			}
			const Sprite *mspr = NULL;
			if (this->draw_mouse_cursor && xpos == this->mousex && ypos == this->mousey && zpos == this->mousez) {
				if (this->cursor == VOR_INVALID) {
					mspr = this->sprites->GetCursorSprite(svd->ground.slope, this->orient);
				} else {
					mspr = this->sprites->GetCornerSprite(svd->ground.slope, this->orient, this->cursor);
				}
			}

			if (surf != NULL || mspr != NULL || path != NULL) {
				std::pair<int32, DrawData> p;
				p.first = sx * xpos + sy * ypos + zpos * 256;
				p.second.cursor = mspr;
				p.second.path = path;
				p.second.ground = surf;
				p.second.foundation = NULL; // TODO svd->foundation.
				p.second.base.x = this->xoffset + xnorth - this->rect.base.x;
				p.second.base.y = this->yoffset + ynorth - this->rect.base.y;
				draw_images.insert(p);
			}
			break;
		}

		default:
			break;
	}
}


PixelFinder::PixelFinder(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient) :
		VoxelCollector(xview, yview, zview, tile_width, tile_height, orient)
{
	this->found = false;
	this->distance = 0;
	this->pixel = 0;
	this->xvoxel = 0;
	this->yvoxel = 0;
	this->zvoxel = 0;
}

PixelFinder::~PixelFinder()
{
}

/**
 * Find the closest sprite.
 * @param voxel %Voxel to examine.
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 */
void PixelFinder::CollectVoxel(const Voxel *voxel, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth)
{
	int sx = (this->orient == VOR_NORTH || this->orient == VOR_EAST) ? 256 : -256;
	int sy = (this->orient == VOR_NORTH || this->orient == VOR_WEST) ? 256 : -256;

	switch (voxel->GetType()) {
		case VT_SURFACE: {
			const SurfaceVoxelData *svd = voxel->GetSurface();
			if (svd->ground.type == GTP_INVALID) break;
			const Sprite *spr = this->sprites->GetSurfaceSprite(GTP_CURSOR_TEST, svd->ground.slope, this->orient);
			if (spr == NULL) break;
			int32 dist = sx * xpos + sy * ypos + zpos * 256;
			if (this->found && dist <= this->distance) break;

			// north_x + spr->xoffset, north_y + spr->yoffset, spr->img_data->width, spr->img_data->height
			int16 xoffset = this->rect.base.x - xnorth - spr->xoffset;
			int16 yoffset = this->rect.base.y - ynorth - spr->yoffset;
			if (xoffset < 0 || yoffset < 0) break;

			uint8 pixel = spr->GetPixel(xoffset, yoffset);
			if (pixel == 0) break; // Transparent pixel, thus not the right ground tile.

			this->distance = sx * xpos + sy * ypos + zpos * 256;
			this->found = true;

			this->xvoxel = xpos;
			this->yvoxel = ypos;
			this->zvoxel = zpos;
			this->pixel = pixel;
			break;
		}

		default:
			break;
	}
}


/**
 * %Viewport constructor.
 */
Viewport::Viewport(int x, int y, uint w, uint h) : Window(WC_MAINDISPLAY)
{
	this->xview = _world.GetXSize() * 256 / 2;
	this->yview = _world.GetYSize() * 256 / 2;
	this->zview = 8 * 256;

	this->tile_width  = 64;
	this->tile_height = 16;
	this->orientation = VOR_NORTH;

	this->mouse_mode = MM_INACTIVE;
	this->mouse_pos.x = 0;
	this->mouse_pos.y = 0;
	this->mouse_state = 0;

	this->xvoxel = 0;
	this->yvoxel = 0;
	this->zvoxel = 0;
	this->cursor = VOR_INVALID;

	this->SetSize(w, h);
	this->SetPosition(x, y);
}

/* virtual */ void Viewport::OnDraw()
{
	SpriteCollector collector(this->xview, this->yview, this->zview, this->tile_width, this->tile_height, this->orientation);
	collector.SetWindowSize(-(int16)this->rect.width / 2, -(int16)this->rect.height / 2, this->rect.width, this->rect.height);
	if (this->mouse_mode == MM_TILE_TERRAFORM) {
		ViewOrientation vor = (this->cursor != VOR_INVALID) ? SubtractOrientations(this->cursor, this->orientation) : VOR_INVALID;
		collector.SetMouseCursor(this->xvoxel, this->yvoxel, this->zvoxel, vor);
	}
	collector.Collect();


	_video->FillSurface(COL_BACKGROUND, this->rect); // Black background.

	ClippedRectangle cr = _video->GetClippedRectangle();
	_video->SetClippedRectangle(this->rect);

	for (DrawImages::const_iterator iter = collector.draw_images.begin(); iter != collector.draw_images.end(); iter++) {
		const DrawData &dd = (*iter).second;
		if (dd.foundation != NULL) _video->BlitImage(dd.base, dd.foundation);
		if (dd.ground != NULL) _video->BlitImage(dd.base, dd.ground);
		if (dd.path != NULL) _video->BlitImage(dd.base, dd.path);
		if (dd.cursor != NULL) _video->BlitImage(dd.base, dd.cursor);
	}

	_video->SetClippedRectangle(cr);
}

/**
 * Compute position of the mouse cursor, and update the display if necessary.
 * @todo It may be possible to give a more accurate estimate of what parts of the screen to redraw.
 */
void Viewport::ComputeCursorPosition()
{
	int16 xp = this->mouse_pos.x - this->rect.width / 2;
	int16 yp = this->mouse_pos.y - this->rect.height / 2;
	PixelFinder collector(this->xview, this->yview, this->zview, this->tile_width, this->tile_height, this->orientation);
	collector.SetWindowSize(xp, yp, 1, 1);
	collector.Collect();
	if (!collector.found) return; // Not at a tile.

	ViewOrientation orient;
	switch (collector.pixel) {
		case 181: orient = AddOrientations(VOR_NORTH, this->orientation); break;
		case 182: orient = AddOrientations(VOR_EAST,  this->orientation); break;
		case 184: orient = AddOrientations(VOR_WEST,  this->orientation); break;
		case 185: orient = AddOrientations(VOR_SOUTH, this->orientation); break;
		default:  orient = VOR_INVALID; break;
	}

	if (collector.xvoxel != this->xvoxel || collector.yvoxel != this->yvoxel || collector.zvoxel != this->zvoxel || this->cursor != orient) {
		this->xvoxel = collector.xvoxel;
		this->yvoxel = collector.yvoxel;
		this->zvoxel = collector.zvoxel;
		this->cursor = orient;
		this->MarkDirty();
	}
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
}

/**
 * Move the viewport a number of screen pixels.
 * @param dx Horizontal shift in screen pixels.
 * @param dy Vertical shift in screen pixels.
 */
void Viewport::MoveViewport(int dx, int dy)
{
	if (dx == 0 && dy == 0) return;

	int32 new_x, new_y;
	switch (this->orientation) {
		case VOR_NORTH:
			new_x = this->xview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_y = this->yview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		case VOR_EAST:
			new_x = this->xview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_y = this->yview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_SOUTH:
			new_x = this->xview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_y = this->yview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_WEST:
			new_x = this->xview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_y = this->yview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		default:
			NOT_REACHED();
	}

	new_x = Clamp<int32>(new_x, 0, _world.GetXSize() * 256);
	new_y = Clamp<int32>(new_y, 0, _world.GetYSize() * 256);
	if (new_x != this->xview || new_y != this->yview) {
		this->xview = new_x;
		this->yview = new_y;
		this->MarkDirty();
	}
}

/**
 * Modify terrain (#MM_TILE_TERRAFORM mode).
 * @param direction Direction of movement.
 * @pre #xvoxel, #yvoxel, #zvoxel denote the currently selected voxel.
 * @pre #cursor denotes the currently selected (part of the) tile.
 */
void Viewport::ChangeTerrain(int direction)
{
	Point32 p;
	p.x = 0;
	p.y = 0;
	TerrainChanges changes(p, _world.GetXSize(), _world.GetYSize());

	p.x = this->xvoxel;
	p.y = this->yvoxel;

	bool ok = false;
	switch (this->cursor) {
		case VOR_NORTH:
		case VOR_EAST:
		case VOR_SOUTH:
		case VOR_WEST:
			ok = changes.ChangeCorner(p, (TileSlope)this->cursor, direction);
			break;

		case VOR_INVALID:
			ok = changes.ChangeCorner(p, TC_NORTH, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_EAST, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_SOUTH, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_WEST, direction);
			break;

		default:
			NOT_REACHED();
	}

	if (ok) {
		changes.ChangeWorld(direction);
		/* Move voxel selection along with the terrain to allow another mousewheel event at the same place.
		 * Note that the mouse cursor position is not changed at all, it still points at the original position.
		 * The coupling is restored with the next mouse movement.
		 */
		this->zvoxel = _world.GetGroundHeight(this->xvoxel, this->yvoxel);
		this->MarkDirty();
	}
}

/**
 * Set mode and state of the mouse interaction of the viewport.
 * @param mode Possibly new mode.
 * @param state State within the mouse mode.
 */
void Viewport::SetMouseModeState(ViewportMouseMode mode, uint8 state)
{
	assert(mode < MM_COUNT);
	this->mouse_state = state;
	this->mouse_mode = mode;
}

/**
 * Get the current mode of mouse interaction of the viewport.
 * @return Current mouse mode.
 */
ViewportMouseMode Viewport::GetMouseMode()
{
	return this->mouse_mode;
}

/* virtual */ void Viewport::OnMouseMoveEvent(const Point16 &pos)
{
	Point16 old_mouse_pos = this->mouse_pos;
	this->mouse_pos = pos;

	switch (this->mouse_mode) {
		case MM_INACTIVE:
			break;

		case MM_TILE_TERRAFORM:
			if ((this->mouse_state & MB_RIGHT) != 0) {
				/* Drag the window if button is pressed down. */
				this->MoveViewport(pos.x - old_mouse_pos.x, pos.y - old_mouse_pos.y);
			} else {
				this->ComputeCursorPosition();
			}
			break;

		default: NOT_REACHED();
	}
}

/* virtual */ WmMouseEvent Viewport::OnMouseButtonEvent(uint8 state)
{
	switch (this->mouse_mode) {
		case MM_INACTIVE:
			break;

		case MM_TILE_TERRAFORM:
			this->mouse_state = state & MB_CURRENT;
			break;

		default: NOT_REACHED();
	}
	return WMME_NONE;
}

/* virtual */ void Viewport::OnMouseWheelEvent(int direction)
{
	switch (this->mouse_mode) {
		case MM_INACTIVE:
			break;

		case MM_TILE_TERRAFORM:
			ChangeTerrain(direction);
			break;

		default: NOT_REACHED();
	}
}

/* virtual */ void Viewport::OnMouseEnterEvent()
{
	this->mouse_state = 0;
}

/* virtual */ void Viewport::OnMouseLeaveEvent()
{
	this->mouse_state = 0;
}

/**
 * Open the main isometric display window.
 * @ingroup viewport_group
 */
Viewport *ShowMainDisplay()
{
	uint16 width  = _video->GetXSize();
	uint16 height = _video->GetYSize();
	assert(width >= 120 && height >= 120);
	Viewport *w = new Viewport(50, 50, width - 100, height - 100);

	w->SetMouseModeState(MM_TILE_TERRAFORM);
	return w;
}
