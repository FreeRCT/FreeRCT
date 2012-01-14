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

#include <map>

/**
 * Proposed additions to the game world. Not part of the game itself, but they are displayed by calling
 * #EnableWorldAdditions (and stopped being displayed with #DisableWorldAdditions).
 * The additions will flash on and off to show they are not decided yet.
 */
WorldAdditions _additions;

static const int ADDITIONS_TIMEOUT_LENGTH = 35; ///< Length of the time interval of displaying or not displaying world additions.


/** Enable flashing display of showing proposed game world additions to the player. */
void EnableWorldAdditions()
{
	Viewport *vp = GetViewport();
	vp->EnableWorldAdditions();
}

/** Disable flashing display of showing proposed game world additions to the player. */
void DisableWorldAdditions()
{
	Viewport *vp = GetViewport();
	vp->DisableWorldAdditions();
}

/**
 * Convert 3D position to the horizontal 2D position.
 * @param x X position in the game world.
 * @param y Y position in the game world.
 * @param orient Orientation.
 * @param width Tile width in pixels.
 * @return X position in 2D.
 */
static FORCEINLINE int32 ComputeXFunction(int32 x, int32 y, ViewOrientation orient, uint16 width)
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
static FORCEINLINE int32 ComputeYFunction(int32 x, int32 y, int32 z, ViewOrientation orient, uint16 width, uint16 height)
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
	VoxelCollector(int32 xview, int32 yview, int32 zview, uint16 width, uint16 height, ViewOrientation orient);
	virtual ~VoxelCollector();

	void SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height);

	void Collect(bool use_additions);

	/**
	 * Convert 3D position to the horizontal 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @return X position in 2D.
	 */
	FORCEINLINE int32 ComputeX(int32 x, int32 y)
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
	FORCEINLINE int32 ComputeY(int32 x, int32 y, int32 z)
	{
		return ComputeYFunction(x, y, z, this->orient, this->tile_width, this->tile_height);
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
	void SetMouseCursors(Viewport *vp);

	DrawImages draw_images; ///< Sprites to draw ordered by viewing distance.
	int16 xoffset; ///< Horizontal offset of the top-left coordinate to the top-left of the display.
	int16 yoffset; ///< Vertical offset of the top-left coordinate to the top-left of the display.

	Viewport *vp;  ///< Parent viewport holding the cursors if not \c NULL.

protected:
	void CollectVoxel(const Voxel *vx, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth);
	const Sprite *GetCursorSpriteAtPos(uint16 xpos, uint16 ypos, uint8 zpos, uint8 tslope);
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
 * @param use_additions Use the #_additions voxels for drawing.
 * @todo Add referenced voxels map here.
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
	if (this-additions_displayed) {
		this->additions_displayed = false;
		_additions.MarkDirty(this);
		this->arrow_cursor.MarkDirty();
	}
	this->timeout = 0;
}

/** Toggle display of world additions (in #_additions) if enabled. */
/* virtual */ void Viewport::TimeoutCallback()
{
	if (!this->additions_enabled) return;

	this->additions_displayed = !this->additions_displayed;
	_additions.MarkDirty(this);
	this->arrow_cursor.MarkDirty();
	this->timeout = ADDITIONS_TIMEOUT_LENGTH;
}

/**
 * Constructor of sprites collector.
 */
SpriteCollector::SpriteCollector(int32 xview, int32 yview, int32 zview, uint16 tile_width, uint16 tile_height, ViewOrientation orient) :
		VoxelCollector(xview, yview, zview, tile_width, tile_height, orient)
{
	this->draw_images.clear();
	this->vp = NULL;
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
void SpriteCollector::SetMouseCursors(Viewport *vp)
{
	this->vp = vp;
}

/**
 * Get the cursor type at a given position.
 * @param xpos X position of the voxel being drawn.
 * @param ypos Y position of the voxel being drawn.
 * @param zpos Z position of the voxel being drawn.
 * @return Cursor type at the position (\c CUR_TYPE_INVALID means no cursor available).
 */
CursorType Viewport::GetCursorAtPos(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->additions_enabled && !this->additions_displayed) {
		CursorType ct = this->arrow_cursor.GetCursor(xpos, ypos, zpos);
		if (ct != CUR_TYPE_INVALID) return ct;
	}
	return this->tile_cursor.GetCursor(xpos, ypos, zpos);
}

/**
 * Get the cursor sprite at a given voxel.
 * @param xpos X position of the voxel being drawn.
 * @param ypos Y position of the voxel being drawn.
 * @param zpos Z position of the voxel being drawn.
 * @param tslope Slope of the tile.
 * @return Pointer to the cursor sprite, or \c NULL if no cursor available.
 */
const Sprite *SpriteCollector::GetCursorSpriteAtPos(uint16 xpos, uint16 ypos, uint8 zpos, uint8 tslope)
{
	if (this->vp == NULL) return NULL;

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

		default:
			return NULL;
	}
}

/**
 * Add all sprites of the voxel to the set of sprites to draw.
 * @param voxel %Voxel to add.
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 * @todo Can we gain time by checking for cursors once at every voxel stack, and only test every \a zpos when there is one in a stack?
 */
void SpriteCollector::CollectVoxel(const Voxel *voxel, int xpos, int ypos, int zpos, int32 xnorth, int32 ynorth)
{
	int sx = (this->orient == VOR_NORTH || this->orient == VOR_EAST) ? 256 : -256;
	int sy = (this->orient == VOR_NORTH || this->orient == VOR_WEST) ? 256 : -256;

	if (voxel == NULL) {
		const Sprite *mspr = this->GetCursorSpriteAtPos(xpos, ypos, zpos, SL_FLAT);
		if (mspr != NULL) {
			std::pair<int32, DrawData> p;
			p.first = sx * xpos + sy * ypos + zpos * 256;
			p.second.cursor = mspr;
			p.second.path = NULL;
			p.second.ground = NULL;
			p.second.foundation = NULL;
			p.second.base.x = this->xoffset + xnorth - this->rect.base.x;
			p.second.base.y = this->yoffset + ynorth - this->rect.base.y;
			draw_images.insert(p);
		}
		return;
	}

	switch (voxel->GetType()) {
		case VT_SURFACE: {
			const SurfaceVoxelData *svd = voxel->GetSurface();
			const Sprite *path;
			if (svd->path.type == PT_INVALID) {
				path = NULL;
			} else {
				path = this->sprites->GetPathSprite(svd->path.type, svd->path.slope, this->orient);
			}
			const Sprite *surf;
			uint8 gslope;
			if (svd->ground.type == GTP_INVALID) {
				surf = NULL;
				gslope = SL_FLAT;
			} else {
				surf = this->sprites->GetSurfaceSprite(svd->ground.type, svd->ground.slope, this->orient);
				gslope = svd->ground.slope;
			}
			const Sprite *mspr = this->GetCursorSpriteAtPos(xpos, ypos, zpos, gslope);

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

		default: {
			const Sprite *mspr = this->GetCursorSpriteAtPos(xpos, ypos, zpos, SL_FLAT);
			if (mspr != NULL) {
				std::pair<int32, DrawData> p;
				p.first = sx * xpos + sy * ypos + zpos * 256;
				p.second.cursor = mspr;
				p.second.path = NULL;
				p.second.ground = NULL;
				p.second.foundation = NULL;
				p.second.base.x = this->xoffset + xnorth - this->rect.base.x;
				p.second.base.y = this->yoffset + ynorth - this->rect.base.y;
				draw_images.insert(p);
			}
			break;
		}
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
Viewport::Viewport(int x, int y, uint w, uint h) : Window(WC_MAINDISPLAY), tile_cursor(this), arrow_cursor(this)
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

	this->SetSize(w, h);
	this->SetPosition(x, y);
}

/**
 * Get relative X position of a point in the world (used for marking window pats dirty).
 * @param xpos X world position.
 * @param ypos Y world position.
 * @return Relative X position.
 */
int32 Viewport::ComputeX(int32 xpos, int32 ypos)
{
	return ComputeXFunction(xpos, ypos, this->orientation, this->tile_width);
}

/**
 * Get relative Y position of a point in the world (used for marking window pats dirty).
 * @param xpos X world position.
 * @param ypos Y world position.
 * @param zpos Z world position.
 * @return Relative Y position.
 */
int32 Viewport::ComputeY(int32 xpos, int32 ypos, int32 zpos)
{
	return ComputeYFunction(xpos, ypos, zpos, this->orientation, this->tile_width, this->tile_height);
}

/* virtual */ void Viewport::OnDraw()
{
	SpriteCollector collector(this->xview, this->yview, this->zview, this->tile_width, this->tile_height, this->orientation);
	collector.SetWindowSize(-(int16)this->rect.width / 2, -(int16)this->rect.height / 2, this->rect.width, this->rect.height);
	if (this->mouse_mode == MM_TILE_TERRAFORM) {
		collector.SetMouseCursors(this);
	}
	collector.Collect(this->additions_enabled && this->additions_displayed);


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
		if (v == NULL) {
			height = 1;
		} else {
			VoxelType vt = v->GetType();
			if (vt == VT_REFERENCE) {
				const ReferenceVoxelData *rvd = v->GetReference();
				xpos = rvd->xpos;
				ypos = rvd->ypos;
				zpos = rvd->zpos;
				v = _world.GetVoxel(xpos, ypos, zpos);
				vt = v->GetType();
			}
			switch (v->GetType()) {
				case VT_EMPTY:
					height = 1;
					break;

				case VT_SURFACE: {
					const SurfaceVoxelData *svd = v->GetSurface();
					if (svd->ground.type == GTP_INVALID) {
						height = 1;
					} else {
						TileSlope tslope = ExpandTileSlope(svd->ground.slope);
						height = ((tslope & TCB_STEEP) != 0) ? 2 : 1;
					}
					break;
				}

				case VT_COASTER: // XXX Not implemented yet.
				default: NOT_REACHED();
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

	_video->MarkDisplayDirty(rect);
}

/**
 * Compute position of the mouse cursor, and update the display if necessary.
 * @param select_corner Not only select the voxel but also the corner.
 */
void Viewport::ComputeCursorPosition(bool select_corner)
{
	int16 xp = this->mouse_pos.x - this->rect.width / 2;
	int16 yp = this->mouse_pos.y - this->rect.height / 2;
	PixelFinder collector(this->xview, this->yview, this->zview, this->tile_width, this->tile_height, this->orientation);
	collector.SetWindowSize(xp, yp, 1, 1);
	collector.Collect(false);
	if (!collector.found) return; // Not at a tile.

	CursorType cur_type = CUR_TYPE_TILE;
	if (select_corner) {
		switch (collector.pixel) {
			case 181: cur_type = (CursorType)AddOrientations(VOR_NORTH, this->orientation); break;
			case 182: cur_type = (CursorType)AddOrientations(VOR_EAST,  this->orientation); break;
			case 184: cur_type = (CursorType)AddOrientations(VOR_WEST,  this->orientation); break;
			case 185: cur_type = (CursorType)AddOrientations(VOR_SOUTH, this->orientation); break;
			default: break;
		}
	}
	this->tile_cursor.SetCursor(collector.xvoxel, collector.yvoxel, collector.zvoxel, cur_type);
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
 * @pre #tile_cursor contains the currently selected (corner of the) surface.
 */
void Viewport::ChangeTerrain(int direction)
{
	assert(this->mouse_mode == MM_TILE_TERRAFORM);

	Point32 p;
	p.x = 0;
	p.y = 0;
	TerrainChanges changes(p, _world.GetXSize(), _world.GetYSize());

	p.x = this->tile_cursor.xpos;
	p.y = this->tile_cursor.ypos;

	bool ok = false;
	switch (this->tile_cursor.type) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST:
			ok = changes.ChangeCorner(p, (TileSlope)(this->tile_cursor.type - CUR_TYPE_NORTH), direction);
			break;

		case CUR_TYPE_TILE:
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
		this->tile_cursor.zpos = _world.GetGroundHeight(this->tile_cursor.xpos, this->tile_cursor.ypos);
		GroundModificationMap::const_iterator iter;
		for (iter = changes.changes.begin(); iter != changes.changes.end(); iter++) {
			const Point32 &pt = (*iter).first;
			this->MarkVoxelDirty(pt.x, pt.y, (*iter).second.height);
		}
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
				this->ComputeCursorPosition(true);
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

/**
 * Get the address of the viewport window.
 * @pre %Viewport is assumed to be available.
 * @return Address of the viewport.
 */
Viewport *GetViewport()
{
	Viewport *v = dynamic_cast<Viewport *>(GetWindowByType(WC_MAINDISPLAY));
	assert(v != NULL);
	return v;
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
