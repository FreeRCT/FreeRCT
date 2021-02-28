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
#include "viewport.h"
#include "map.h"
#include "mouse_mode.h"
#include "video.h"
#include "palette.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "shop_type.h"
#include "person.h"
#include "weather.h"
#include "fence.h"

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
	VoxelCollector(Viewport *vp);
	virtual ~VoxelCollector();

	void SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height);

	void Collect();
	void SetSelector(MouseModeSelector *selector);

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

	XYZPoint32 view_pos;          ///< Position of the centre point of the display.
	uint16 tile_width;            ///< Width of a tile.
	uint16 tile_height;           ///< Height of a tile.
	ViewOrientation orient;       ///< Direction of view.
	const SpriteStorage *sprites; ///< Sprite collection of the right size.
	Viewport *vp;                 ///< Parent viewport for accessing the cursors if not \c nullptr.
	MouseModeSelector *selector;  ///< Mouse mode selector.
	bool underground_mode;        ///< Whether to draw underground mode sprites (else draw normal surface sprites).

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
	 * @param vx %Voxel to add, \c nullptr means 'cursor above stack'.
	 * @param view_pos World position.
	 * @param xnorth X coordinate of the north corner at the display.
	 * @param ynorth y coordinate of the north corner at the display.
	 * @note Implement in a derived class.
	 */
	virtual void CollectVoxel(const Voxel *vx, const XYZPoint16 &view_pos, int32 xnorth, int32 ynorth) = 0;
};

/**
 * Data temporary needed for ordering sprites and blitting them to the screen.
 * @ingroup viewport_group
 */
struct DrawData {
	/**
	 * Setter method to initialize the other fields.
	 * @param level Slice of this sprite (vertical row).
	 * @param z_height Height of the voxel being drawn.
	 * @param order Selection when to draw this sprite (sorts sprites within a voxel). @see SpriteOrder
	 * @param sprite Mouse cursor to draw.
	 * @param base Base coordinate of the image, relative to top-left of the window.
	 * @param recolour Recolouring of the sprite.
	 * @param highlight Highlight the sprite.
	 */
	inline void Set(int32 level, uint16 z_height, SpriteOrder order, const ImageData *sprite, const Point32 &base, const Recolouring *recolour = nullptr, bool highlight = false)
	{
		this->level = level;
		this->z_height = z_height;
		this->order = order;
		this->sprite = sprite;
		this->base = base;
		this->recolour = recolour;
		this->highlight = highlight;
	}

	int32 level;                 ///< Slice of this sprite (vertical row).
	uint16 z_height;             ///< Height of the voxel being drawn.
	SpriteOrder order;           ///< Selection when to draw this sprite (sorts sprites within a voxel). @see SpriteOrder
	const ImageData *sprite;     ///< Mouse cursor to draw.
	Point32 base;                ///< Base coordinate of the image, relative to top-left of the window.
	const Recolouring *recolour; ///< Recolouring of the sprite.
	bool highlight;              ///< Highlight the sprite (semi-transparent white).
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
	SpriteCollector(Viewport *vp);
	~SpriteCollector();

	void SetXYOffset(int16 xoffset, int16 yoffset);

	DrawImages draw_images; ///< Sprites to draw ordered by viewing distance.
	int16 xoffset; ///< Horizontal offset of the top-left coordinate to the top-left of the display.
	int16 yoffset; ///< Vertical offset of the top-left coordinate to the top-left of the display.

protected:
	void CollectVoxel(const Voxel *vx, const XYZPoint16 &voxel_pos, int32 xnorth, int32 ynorth) override;
	void SetupSupports(const VoxelStack *stack, uint xpos, uint ypos) override;
	const ImageData *GetCursorSpriteAtPos(CursorType ctype, const XYZPoint16 &voxel_pos, uint8 tslope);

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
	void CollectVoxel(const Voxel *vx, const XYZPoint16 &voxel_pos, int32 xnorth, int32 ynorth) override;
};

/**
 * Base class constructor.
 * @param vp %Viewport querying the voxel information.
 * @todo Can we remove some variables, as \a vp is stored now as well?
 */
VoxelCollector::VoxelCollector(Viewport *vp)
{
	this->vp = vp;
	this->selector = nullptr;
	this->view_pos = vp->view_pos;
	this->tile_width = vp->tile_width;
	this->tile_height = vp->tile_height;
	this->orient = vp->orientation;
	this->underground_mode = vp->underground_mode;

	this->sprites = _sprite_manager.GetSprites(this->tile_width);
	assert(this->sprites != nullptr);
}

/* Destructor. */
VoxelCollector::~VoxelCollector()
{
}

/**
 * Set screen area of interest (relative to the (#Viewport::view_pos position).
 * @param xpos Horizontal position of the top-left corner.
 * @param ypos Vertical position of the top-left corner.
 * @param width Width of the area.
 * @param height Height of the area.
 */
void VoxelCollector::SetWindowSize(int16 xpos, int16 ypos, uint16 width, uint16 height)
{
	this->rect.base.x = this->ComputeX(this->view_pos.x, this->view_pos.y) + xpos;
	this->rect.base.y = this->ComputeY(this->view_pos.x, this->view_pos.y, this->view_pos.z) + ypos;
	this->rect.width = width;
	this->rect.height = height;
}

/**
 * Set the mouse mode selector.
 * @param selector Selector to use while rendering.
 */
void VoxelCollector::SetSelector(MouseModeSelector *selector)
{
	this->selector = selector;
}

/**
 * Perform the collecting cycle.
 * This part walks over the voxels, and call #CollectVoxel for each useful voxel.
 * A derived class may then inspect the voxel in more detail.
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

			const VoxelStack *stack = _world.GetStack(xpos, ypos);

			/* Compute lowest and highest voxel to render. */
			uint zpos = stack->base;
			uint top = stack->base + stack->height - 1;

			if (this->selector != nullptr) {
				uint32 range = this->selector->GetZRange(xpos, ypos);
				if (range != 0) {
					zpos = std::min(zpos, (range & 0xFFFF));
					top = std::max(top, (range >> 16));
				}
			}
			this->SetupSupports(stack, xpos, ypos);

			for (; zpos <= top; zpos++) {
				int32 north_y = this->ComputeY(world_x, world_y, zpos * 256);
				if (north_y - this->tile_height >= (int32)(this->rect.base.y + this->rect.height)) continue; // Voxel is below the window.
				if (north_y + this->tile_width / 2 + this->tile_height <= (int32)this->rect.base.y) break; // Above the window and rising!

				int count = zpos - stack->base;
				const Voxel *voxel = (count >= 0 && count < stack->height) ? &stack->voxels[count] : nullptr;
				this->CollectVoxel(voxel, XYZPoint16(xpos, ypos, zpos), north_x, north_y);
			}
		}
	}
}

/**
 * Constructor of sprites collector.
 * @param vp %Viewport that needs the sprites.
 */
SpriteCollector::SpriteCollector(Viewport *vp) : VoxelCollector(vp)
{
	this->draw_images.clear();
	this->xoffset = 0;
	this->yoffset = 0;

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
 * Get the cursor sprite at a given voxel.
 * @param ctype Cursor type to get.
 * @param voxel_pos Position of the voxel being drawn.
 * @param tslope Slope of the tile.
 * @return Pointer to the cursor sprite, or \c nullptr if no cursor available.
 */
const ImageData *SpriteCollector::GetCursorSpriteAtPos(CursorType ctype, const XYZPoint16 &voxel_pos, uint8 tslope)
{
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
 * @param pos Position of the voxel being drawn.
 * @param base_pos Base position of the sprite in the screen.
 * @param orient View orientation.
 * @param number Ride instance number.
 * @param voxel_number Number of the voxel.
 * @param dd [out] Data to draw (4 entries).
 * @param platform [out] Shape of the support platform, if needed. @see PathSprites
 * @return The number of \a dd entries filled.
 */
static int DrawRide(int32 slice, const XYZPoint16 & pos, const Point32 base_pos,
		ViewOrientation orient, uint16 number, uint16 voxel_number, DrawData *dd, uint8 *platform)
{
	const RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr) return 0;

	if (platform != nullptr) *platform = PATH_INVALID;
	const ImageData *sprites[4];
	ri->GetSprites(pos, voxel_number, orient, sprites, platform);

	int idx = 0;
	static const SpriteOrder sprite_numbers[4] = {SO_PLATFORM_BACK, SO_RIDE, SO_RIDE_FRONT, SO_PLATFORM_FRONT};
	for (int i = 0; i < 4; i++) {
		if (sprites[i] == nullptr) continue;

		dd[idx].Set(slice, pos.z, sprite_numbers[i], sprites[i], base_pos, ri->GetRecolours(pos));
		idx++;
	}
	return idx;
}

/**
 * Add all sprites of the voxel to the set of sprites to draw.
 * @param voxel %Voxel to add, \c nullptr means 'cursor above stack'.
 * @param voxel_pos World position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 * @todo Can we gain time by checking for cursors once at every voxel stack, and only test every \a zpos when there is one in a stack?
 */
void SpriteCollector::CollectVoxel(const Voxel *voxel, const XYZPoint16 &voxel_pos, int32 xnorth, int32 ynorth)
{
	int32 slice;
	switch (this->orient) {
		case 0: slice =  voxel_pos.x + voxel_pos.y; break;
		case 1: slice =  voxel_pos.x - voxel_pos.y; break;
		case 2: slice = -voxel_pos.x - voxel_pos.y; break;
		case 3: slice = -voxel_pos.x + voxel_pos.y; break;
		default: NOT_REACHED();
	}

	Point32 north_point(this->xoffset + xnorth - this->rect.base.x, this->yoffset + ynorth - this->rect.base.y);

	uint8 platform_shape = PATH_INVALID;
	SmallRideInstance sri = (voxel == nullptr) ? SRI_FREE : voxel->GetInstance();
	uint16 instance_data = (voxel == nullptr) ? 0 : voxel->GetInstanceData();
	bool highlight = false;
	if (this->selector != nullptr) highlight = this->selector->GetRide(voxel, voxel_pos, &sri, &instance_data);
	if (sri == SRI_PATH && HasValidPath(instance_data)) { // A path (and not something reserved above it).
		platform_shape = _path_rotation[GetImplodedPathSlope(instance_data)][this->orient];
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_PATH, this->sprites->GetPathSprite(GetPathType(instance_data), GetImplodedPathSlope(instance_data), this->orient),
				north_point, nullptr, highlight);
		this->draw_images.insert(dd);
	} else if (sri >= SRI_FULL_RIDES) { // A normal ride.
		DrawData dd[4];
		int count = DrawRide(slice, voxel_pos, north_point, this->orient, sri, instance_data, dd, &platform_shape);
		for (int i = 0; i < count; i++) {
			dd[i].highlight = highlight;
			this->draw_images.insert(dd[i]);
		}
	}

	/* Foundations. */
	if (voxel != nullptr && voxel->GetFoundationType() != FDT_INVALID) {
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
				dd.Set(slice, voxel_pos.z, SO_FOUNDATION, img, north_point);
				this->draw_images.insert(dd);
			}
		}
		if (se != 0) {
			const ImageData *img = fnd->sprites[se - 1];
			if (img != nullptr) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, SO_FOUNDATION, img, north_point);
				this->draw_images.insert(dd);
			}
		}
	}

	/* Ground surface. */
	uint8 gslope = SL_FLAT;
	if (voxel != nullptr && voxel->GetGroundType() != GTP_INVALID) {
		uint8 slope = voxel->GetGroundSlope();
		uint8 type = (this->underground_mode) ? GTP_UNDERGROUND : voxel->GetGroundType();
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND, this->sprites->GetSurfaceSprite(type, slope, this->orient), north_point);
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
	if (voxel != nullptr) {
		uint32 fences = voxel->GetFences();
		if (this->selector != nullptr) fences = this->selector->GetFences(voxel, voxel_pos, fences);
		for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
			FenceType fence_type = GetFenceType(fences, edge);
			if (fence_type != FENCE_TYPE_INVALID) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, IsBackEdge(this->orient, edge) ? SO_FENCE_BACK : SO_FENCE_FRONT,
						this->sprites->GetFenceSprite(fence_type, edge, gslope, this->orient), north_point);
				if (IsImplodedSteepSlope(gslope) && !IsImplodedSteepSlopeTop(gslope)) dd.z_height++;
				if (GB(fences, 16 + edge, 1) != 0) dd.highlight = true;
				this->draw_images.insert(dd);
			}
		}
	}

	/* Sprite cursor. */
	if (this->selector != nullptr) {
		CursorType ctype = this->selector->GetCursor(voxel_pos);
		if (ctype != CUR_TYPE_INVALID) {
			const ImageData *mspr = this->GetCursorSpriteAtPos(ctype, voxel_pos, gslope);
			if (mspr != nullptr) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, SO_CURSOR, mspr, north_point);
				if (ctype >= CUR_TYPE_EDGE_NE && ctype <= CUR_TYPE_EDGE_NW && IsImplodedSteepSlope(gslope) && !IsImplodedSteepSlopeTop(gslope)) dd.z_height++;
				this->draw_images.insert(dd);
			}
		}
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
			dd.Set(slice, voxel_pos.z, SO_PLATFORM, pl_spr, north_point);
			this->draw_images.insert(dd);
		}

		/* XXX Use the shape to draw handle bars. */

		/* Add supports. */
		uint16 height = this->ground_height;
		this->ground_height = -1;
		uint8 slope = this->ground_slope;
		while (height < voxel_pos.z) {
			int yoffset = (voxel_pos.z - height) * this->vp->tile_height; // Compensate y position of support.
			uint sprnum;
			if (slope == SL_FLAT) {
				if (height + 1 < voxel_pos.z) {
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
				dd.Set(slice, height, SO_SUPPORT, img, Point32(north_point.x, north_point.y + yoffset));
				this->draw_images.insert(dd);
			}
		}
	}

	/* Add voxel objects (persons, ride cars, etc). */
	const VoxelObject *vo = (voxel == nullptr) ? nullptr : voxel->voxel_objects;
	while (vo != nullptr) {
		const Recolouring *recolour;
		const ImageData *anim_spr = vo->GetSprite(this->sprites, this->orient, &recolour);
		if (anim_spr != nullptr) {
			int x_off = ComputeX(vo->pix_pos.x, vo->pix_pos.y);
			int y_off = ComputeY(vo->pix_pos.x, vo->pix_pos.y, vo->pix_pos.z);
			Point32 pos(north_point.x + this->north_offsets[this->orient].x + x_off,
			            north_point.y + this->north_offsets[this->orient].y + y_off);
			DrawData dd;
			dd.Set(slice, voxel_pos.z, SO_PERSON, anim_spr, pos, recolour);
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
PixelFinder::PixelFinder(Viewport *vp, FinderData *fdata) : VoxelCollector(vp)
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
 * @param voxel_pos World position.
 * @param xnorth X coordinate of the north corner at the display.
 * @param ynorth y coordinate of the north corner at the display.
 */
void PixelFinder::CollectVoxel(const Voxel *voxel, const XYZPoint16 &voxel_pos, int32 xnorth, int32 ynorth)
{
	int32 slice;
	switch (this->orient) {
		case 0: slice =  voxel_pos.x + voxel_pos.y; break;
		case 1: slice =  voxel_pos.x - voxel_pos.y; break;
		case 2: slice = -voxel_pos.x - voxel_pos.y; break;
		case 3: slice = -voxel_pos.x + voxel_pos.y; break;
		default: NOT_REACHED();
	}
	/* Looking for surface edge? */
	if ((this->allowed & SO_GROUND_EDGE) != 0 && voxel->GetGroundType() != GTP_INVALID) {
		const ImageData *spr = this->sprites->GetSurfaceSprite(GTP_CURSOR_EDGE_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND_EDGE, nullptr, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
		if (spr != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = spr->GetPixel(dd.base.x - spr->xoffset, dd.base.y - spr->yoffset);
			if (pixel != 0) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos = voxel_pos;
				this->pixel = pixel;
			}
		}
	}

	if (voxel == nullptr) return; // Ignore cursors, they are not clickable.

	SmallRideInstance number = voxel->GetInstance();
	if ((this->allowed & CS_RIDE) != 0 && number >= SRI_FULL_RIDES) {
		/* Looking for a ride? */
		DrawData dd[4];
		int count = DrawRide(slice, voxel_pos, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth),
				this->orient, number, voxel->GetInstanceData(), dd, nullptr);
		for (int i = 0; i < count; i++) {
			if (!this->found || this->data < dd[i]) {
				const ImageData *img = dd[i].sprite;
				uint32 pixel = img->GetPixel(dd[i].base.x - img->xoffset, dd[i].base.y - img->yoffset);
				if (GetA(pixel) != TRANSPARENT) {
					this->found = true;
					this->data = dd[i];
					this->fdata->voxel_pos = voxel_pos;
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
		dd.Set(slice, voxel_pos.z, SO_PATH, nullptr, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
		if (img != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = img->GetPixel(dd.base.x - img->xoffset, dd.base.y - img->yoffset);
			if (GetA(pixel) != TRANSPARENT) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos = voxel_pos;
				this->pixel = pixel;
			}
		}
	} else if ((this->allowed & CS_GROUND) != 0 && voxel->GetGroundType() != GTP_INVALID) {
		/* Looking for surface? */
		const ImageData *spr = this->sprites->GetSurfaceSprite(GTP_CURSOR_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND, nullptr, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
		if (spr != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = spr->GetPixel(dd.base.x - spr->xoffset, dd.base.y - spr->yoffset);
			if (GetA(pixel) != TRANSPARENT) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos = voxel_pos;
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
			int x_off = ComputeX(pers->pix_pos.x, pers->pix_pos.y);
			int y_off = ComputeY(pers->pix_pos.x, pers->pix_pos.y, pers->pix_pos.z);
			DrawData dd;
			dd.Set(slice, voxel_pos.z, SO_PERSON, nullptr, Point32(this->rect.base.x - xnorth - x_off, this->rect.base.y - ynorth - y_off));
			if (anim_spr && (!this->found || this->data < dd)) {
				uint32 pixel = anim_spr->GetPixel(dd.base.x - anim_spr->xoffset, dd.base.y - anim_spr->yoffset);
				if (GetA(pixel) != TRANSPARENT) {
					this->found = true;
					this->data = dd;
					this->fdata->voxel_pos = voxel_pos;
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
 * @param view_pos Pixel position of the center viewpoint of the main display.
 */
Viewport::Viewport(const XYZPoint32 &view_pos) : Window(WC_MAINDISPLAY, ALL_WINDOWS_OF_TYPE)
{
	this->view_pos = view_pos;
	this->tile_width  = 64;
	this->tile_height = 16;
	this->orientation = VOR_NORTH;

	this->mouse_pos.x = 0;
	this->mouse_pos.y = 0;
	this->underground_mode = false;

	uint16 width  = _video.GetXSize();
	uint16 height = _video.GetYSize();
	assert(width >= 120 && height >= 120); // Arbitrary lower limit as sanity check.

	this->SetSize(width, height);
	this->SetPosition(0, 0);
}

Viewport::~Viewport()
{
}

/**
 * Can the viewport switch to underground mode view?
 * @return Whether underground mode view is available.
 */
bool Viewport::IsUndergroundModeAvailable() const
{
	const SpriteStorage *storage = _sprite_manager.GetSprites(this->tile_width);
	return storage->surface[GTP_UNDERGROUND].HasAllSprites();
}

/** Enable the underground mode view, if possible. */
void Viewport::SetUndergroundMode()
{
	if (!IsUndergroundModeAvailable()) return;
	this->underground_mode = true;
	this->MarkDirty();
}

/** Toggle the underground mode view on/off. */
void Viewport::ToggleUndergroundMode()
{
	if (this->underground_mode) {
		this->underground_mode = false;
		this->MarkDirty();
	} else {
		this->SetUndergroundMode();
	}
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

void Viewport::OnDraw(MouseModeSelector *selector)
{
	SpriteCollector collector(this);
	collector.SetWindowSize(-(int16)this->rect.width / 2, -(int16)this->rect.height / 2, this->rect.width, this->rect.height);
	collector.SetSelector(selector);
	collector.Collect();
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
		_video.BlitImage(dd.base, dd.sprite, rec, dd.highlight ? GS_SEMI_TRANSPARENT : gs);
	}

	_video.SetClippedRectangle(cr);
}

/**
 * Mark a voxel as in need of getting painted.
 * @param voxel_pos Position of the voxel.
 * @param height Number of voxels to mark above the specified coordinate (\c 0 means inspect the voxel itself).
 */
void Viewport::MarkVoxelDirty(const XYZPoint16 &voxel_pos, int16 height)
{
	if (height <= 0) {
		const Voxel *v = _world.GetVoxel(voxel_pos);
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
						case RTK_SHOP:
						case RTK_GENTLE:
						case RTK_THRILL: {
							const FixedRideInstance *si = static_cast<const FixedRideInstance *>(ri);
							const XYZPoint16 pos = FixedRideType::UnorientatedOffset(si->orientation, voxel_pos.x - si->vox_pos.x, voxel_pos.y - si->vox_pos.y);
							height =
								si->IsEntranceLocation(voxel_pos) ? RideEntranceExitType::entrance_height :
								si->IsExitLocation(voxel_pos) ? RideEntranceExitType::exit_height :
								si->GetFixedRideType()->GetHeight(pos.x, pos.y);
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

	int32 center_x = this->ComputeX(this->view_pos.x, this->view_pos.y) - this->rect.base.x - this->rect.width / 2;
	int32 center_y = this->ComputeY(this->view_pos.x, this->view_pos.y, this->view_pos.z) - this->rect.base.y - this->rect.height / 2;

	pt = &_corner_dxy[this->orientation];
	rect.base.y = this->ComputeY((voxel_pos.x + pt->x) * 256, (voxel_pos.y + pt->y) * 256, (voxel_pos.z + height) * 256) - center_y;

	pt = &_corner_dxy[RotateCounterClockwise(this->orientation)];
	rect.base.x = this->ComputeX((voxel_pos.x + pt->x) * 256, (voxel_pos.y + pt->y) * 256) - center_x;

	pt = &_corner_dxy[RotateClockwise(this->orientation)];
	int32 d = this->ComputeX((voxel_pos.x + pt->x) * 256, (voxel_pos.y + pt->y) * 256) - center_x;
	assert(d >= rect.base.x);
	rect.width = d - rect.base.x + 1;

	pt = &_corner_dxy[RotateClockwise(RotateClockwise(this->orientation))];
	d = this->ComputeY((voxel_pos.x + pt->x) * 256, (voxel_pos.y + pt->y) * 256, voxel_pos.z * 256) - center_y;
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
	collector.Collect();
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
			new_xy.x = this->view_pos.x + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_xy.y = this->view_pos.y - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		case VOR_EAST:
			new_xy.x = this->view_pos.x - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_xy.y = this->view_pos.y - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_SOUTH:
			new_xy.x = this->view_pos.x - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_xy.y = this->view_pos.y + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_WEST:
			new_xy.x = this->view_pos.x + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_xy.y = this->view_pos.y + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
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
	if (new_x != this->view_pos.x || new_y != this->view_pos.y) {
		this->view_pos.x = new_x;
		this->view_pos.y = new_y;
		this->MarkDirty();
	}
}

void Viewport::OnMouseMoveEvent(const Point16 &pos)
{
	Point16 old_mouse_pos = this->mouse_pos;
	this->mouse_pos = pos;

	if (_window_manager.SelectorMouseMoveEvent(this, pos)) return;

	/* Intercept RMB drag for moving the viewport. */
	if ((_window_manager.GetMouseState() & MB_RIGHT) != 0) {
		this->MoveViewport(pos.x - old_mouse_pos.x, pos.y - old_mouse_pos.y);
	}
}

WmMouseEvent Viewport::OnMouseButtonEvent(uint8 state)
{
	if (_window_manager.SelectorMouseButtonEvent(state)) return WMME_NONE;

	/* Did the user click on something that has a window? */
	if ((state & MB_CURRENT) != 0) {
		FinderData fdata(CS_RIDE | CS_PERSON, FW_TILE);
		switch (this->ComputeCursorPosition(&fdata)) {
			case CS_RIDE: {
				RideInstance *ri = _rides_manager.GetRideInstance(fdata.ride);
				if (ri == nullptr) break;
				switch (ri->GetKind()) {
					case RTK_SHOP:
						ShowShopManagementGui(fdata.ride);
						return WMME_NONE;

					case RTK_GENTLE:
					case RTK_THRILL:
						ShowGentleThrillRideManagementGui(fdata.ride);
						return WMME_NONE;

					case RTK_COASTER:
						ShowCoasterManagementGui(ri);
						return WMME_NONE;

					default: break; // Other types are not implemented yet.
				}
				break;
			}

			case CS_PERSON:
				ShowGuestInfoGui(fdata.person);
				return WMME_NONE;

			default: break;
		}

	}
	return WMME_NONE;
}

void Viewport::OnMouseWheelEvent(int direction)
{
	_window_manager.SelectorMouseWheelEvent(direction);
}

/**
 * Mark a voxel as in need of getting painted.
 * @param voxel_pos Position of the voxel.
 * @param height Number of voxels to mark above the specified coordinate (\c 0 means inspect the voxel itself).
 */
void MarkVoxelDirty(const XYZPoint16 &voxel_pos, int16 height)
{
	Viewport *vp = _window_manager.GetViewport();
	if (vp != nullptr) vp->MarkVoxelDirty(voxel_pos, height);
}

/**
 * Open the main isometric display window.
 * @param view_pos Pixel position of the center viewpoint of the main display.
 * @ingroup viewport_group
 */
void ShowMainDisplay(const XYZPoint32 &view_pos)
{
	new Viewport(view_pos);
}
