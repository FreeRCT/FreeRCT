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
#include "gamecontrol.h"
#include "scenery.h"

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
		return ComputeXFunction(x, y, this->orient, TileWidth(this->zoom));
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
		return ComputeYFunction(x, y, z, this->orient, TileWidth(this->zoom), TileHeight(this->zoom));
	}

	XYZPoint32 view_pos;          ///< Position of the centre point of the display.
	int zoom;                    ///< The current zoom scale (an index in #_zoom_scales).
	ViewOrientation orient;       ///< Direction of view.
	Viewport *vp;                 ///< Parent viewport for accessing the cursors if not \c nullptr.
	MouseModeSelector *selector;  ///< Mouse mode selector.

	Rectangle32 rect; ///< Screen area of interest.

protected:
	/**
	 * Decide where supports should be raised.
	 * @param stack %Voxel stack to examine.
	 * @param xpos X position of the voxel stack.
	 * @param ypos Y position of the voxel stack.
	 */
	virtual void SetupSupports([[maybe_unused]] const VoxelStack *stack, [[maybe_unused]] uint xpos, [[maybe_unused]] uint ypos)
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
	 * @param gs Gradient shift of the sprite.
	 */
	inline void Set(int32 level, uint16 z_height, SpriteOrder order, const ImageData *sprite, const Point32 &base,
			const Recolouring *recolour = nullptr, GradientShift gs = GS_INVALID)
	{
		this->level = level;
		this->z_height = z_height;
		this->order = order;
		this->sprite = sprite;
		this->base = base;
		this->recolour = recolour;
		this->gs = gs;
		assert(this->sprite != nullptr);
	}

	const ImageData *sprite;     ///< Mouse cursor to draw.
	const Recolouring *recolour; ///< Recolouring of the sprite.
	int32 level;                 ///< Slice of this sprite (vertical row).
	SpriteOrder order;           ///< Selection when to draw this sprite (sorts sprites within a voxel). @see SpriteOrder
	Point32 base;                ///< Base coordinate of the image, relative to top-left of the window.
	uint16 z_height;             ///< Height of the voxel being drawn.
	GradientShift gs;            ///< Gradient shift of the sprite.
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
	this->zoom = vp->zoom;
	this->orient = vp->orientation;
}

/* Destructor. */
VoxelCollector::~VoxelCollector()
= default;

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
			if (north_x + TileWidth(this->zoom) / 2 <= static_cast<int32>(this->rect.base.x)) continue;  // Right of voxel column is at left of window.
			if (north_x - TileWidth(this->zoom) / 2 >= static_cast<int32>(this->rect.base.x + this->rect.width)) continue;  // Left of the window.

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
				if (north_y - TileHeight(this->zoom) >= static_cast<int32>(this->rect.base.y + this->rect.height)) continue;  // Voxel is below the window.
				if (north_y + TileWidth(this->zoom) / 2 + TileHeight(this->zoom) <= static_cast<int32>(this->rect.base.y)) break;  // Above the window and rising!

				int count = zpos - stack->base;
				const Voxel *voxel = (count >= 0 && count < stack->height) ? stack->voxels[count].get() : nullptr;
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
	this->north_offsets[VOR_EAST].x  = -TileWidth(this->zoom) / 2; this->north_offsets[VOR_EAST].y  = TileWidth(this->zoom) / 4;
	this->north_offsets[VOR_SOUTH].x = 0;                     this->north_offsets[VOR_SOUTH].y = TileWidth(this->zoom) / 2;
	this->north_offsets[VOR_WEST].x  = TileWidth(this->zoom) / 2;  this->north_offsets[VOR_WEST].y  = TileWidth(this->zoom) / 4;
}

SpriteCollector::~SpriteCollector()
= default;

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
const ImageData *SpriteCollector::GetCursorSpriteAtPos(CursorType ctype, [[maybe_unused]] const XYZPoint16 &voxel_pos, uint8 tslope)
{
	switch (ctype) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST: {
			ViewOrientation vor = SubtractOrientations((ViewOrientation)(ctype - CUR_TYPE_NORTH), this->orient);
			return _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetCornerSprite, tslope, this->orient, vor);
		}

		case CUR_TYPE_TILE:
			return _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetCursorSprite, tslope, this->orient);

		case CUR_TYPE_ARROW_NE:
		case CUR_TYPE_ARROW_SE:
		case CUR_TYPE_ARROW_SW:
		case CUR_TYPE_ARROW_NW:
			return _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetArrowSprite, ctype - CUR_TYPE_ARROW_NE, this->orient);

		default:
			return nullptr;
	}
}

void SpriteCollector::SetupSupports(const VoxelStack *stack, [[maybe_unused]] uint xpos, [[maybe_unused]] uint ypos)
{
	for (uint i = 0; i < stack->height; i++) {
		const Voxel *v = stack->voxels[i].get();
		if (v->GetGroundType() == GTP_INVALID) continue;
		if (v->GetInstance() == SRI_FREE) {
			this->ground_height = stack->base + i;
			this->ground_slope = _slope_rotation[v->GetGroundSlope()][this->orient];
			return;
		}
		break;
	}
	this->ground_height = -1;
}

/**
 * Prepare to add the sprite for a ride or a scenery item.
 * @param slice Depth of this sprite.
 * @param pos Position of the voxel being drawn.
 * @param base_pos Base position of the sprite in the screen.
 * @param orient View orientation.
 * @param zoom Viewport zoom index.
 * @param number Ride instance number (\c SRI_SCENERY for scenery items).
 * @param voxel_number Instance data of the voxel.
 * @param dd [out] Data to draw (4 entries).
 * @param platform [out] Shape of the support platform, if needed. @see PathSprites
 * @return The number of \a dd entries filled.
 */
static int DrawRideOrScenery(int32 slice, const XYZPoint16 & pos, const Point32 base_pos,
		ViewOrientation orient, int zoom, uint16 number, uint16 voxel_number, DrawData *dd, uint8 *platform)
{
	if (platform != nullptr) *platform = PATH_INVALID;
	const ImageData *sprites[4];
	const Recolouring *recolour = nullptr;

	if (number == SRI_SCENERY) {
		const SceneryInstance *si = _scenery.GetItem(pos);
		if (si == nullptr) return 0;
		si->GetSprites(pos, voxel_number, orient, zoom, sprites, platform);
	} else {
		const RideInstance *ri = _rides_manager.GetRideInstance(number);
		if (ri == nullptr) return 0;
		ri->GetSprites(pos, voxel_number, orient, zoom, sprites, platform);
		recolour = ri->GetRecolours(pos);
	}

	int idx = 0;
	static const SpriteOrder sprite_numbers[4] = {SO_PLATFORM_BACK, SO_RIDE, SO_RIDE_FRONT, SO_PLATFORM_FRONT};
	for (int i = 0; i < 4; i++) {
		if (sprites[i] == nullptr) continue;

		dd[idx].Set(slice, pos.z, sprite_numbers[i], sprites[i], base_pos, recolour);
		if (number == SRI_SCENERY) dd[idx].order = static_cast<SpriteOrder>(dd[idx].order & ~CS_RIDE);  // Don't treat scenery like rides.
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
	const ImageData *background_sprite = nullptr;
	if (this->selector != nullptr) highlight = this->selector->GetRide(voxel, voxel_pos, &sri, &instance_data, &background_sprite);
	if (sri == SRI_PATH && HasValidPath(instance_data)) { // A path (and not something reserved above it).
		platform_shape = _path_rotation[GetImplodedPathSlope(instance_data)][this->orient];

		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_PATH, _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetPathSprite,
				GetPathType(instance_data), GetPathStatus(instance_data), GetImplodedPathSlope(instance_data), this->orient),
				north_point, nullptr, highlight ? GS_SEMI_TRANSPARENT : GS_INVALID);
		this->draw_images.insert(dd);

		for (const PathObjectInstance::PathObjectSprite &image : _scenery.DrawPathObjects(voxel_pos, this->orient, this->zoom)) {
			const int x_off = ComputeX(image.offset.x, image.offset.y);
			const int y_off = ComputeY(image.offset.x, image.offset.y, image.offset.z);
			Point32 pos(north_point.x + this->north_offsets[this->orient].x + x_off,
			            north_point.y + this->north_offsets[this->orient].y + y_off);

			dd.Set(slice, voxel_pos.z, SO_PATH_OBJECTS, image.sprite, pos, nullptr,
					image.semi_transparent ? GS_SEMI_TRANSPARENT : this->vp->GetDisplayFlag(DF_WIREFRAME_SCENERY) ? GS_WIREFRAME : GS_INVALID);
			this->draw_images.insert(dd);
		}
	} else if (sri >= SRI_FULL_RIDES || sri == SRI_SCENERY) { // A normal ride, or a scenery item.
		DrawData dd[4];
		int count = DrawRideOrScenery(slice, voxel_pos, north_point, this->orient, this->zoom, sri, instance_data, dd, &platform_shape);
		for (int i = 0; i < count; i++) {
			if (highlight) {
				dd[i].gs = GS_SEMI_TRANSPARENT;
			} else if ((this->vp->GetDisplayFlag(DF_WIREFRAME_RIDES) && sri >= SRI_FULL_RIDES) ||
					(this->vp->GetDisplayFlag(DF_WIREFRAME_SCENERY) && sri == SRI_SCENERY)) {
				dd[i].gs = GS_WIREFRAME;
			}
			this->draw_images.insert(dd[i]);
		}
	}
	if (background_sprite != nullptr) {
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_CURSOR, background_sprite, north_point);
		this->draw_images.insert(dd);
	}

	/* Foundations. */
	if (voxel != nullptr && voxel->GetFoundationType() != FDT_INVALID && !this->vp->GetDisplayFlag(DF_HIDE_FOUNDATIONS)) {
		uint8 fslope = voxel->GetFoundationSlope();
		uint8 sw, se; // SW foundations, SE foundations.
		switch (this->orient) {
			case VOR_NORTH: sw = GB(fslope, 4, 2); se = GB(fslope, 2, 2); break;
			case VOR_EAST:  sw = GB(fslope, 6, 2); se = GB(fslope, 4, 2); break;
			case VOR_SOUTH: sw = GB(fslope, 0, 2); se = GB(fslope, 6, 2); break;
			case VOR_WEST:  sw = GB(fslope, 2, 2); se = GB(fslope, 0, 2); break;
			default: NOT_REACHED();
		}
		if (sw != 0) {
			const ImageData *img = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetFoundationSprite, FDT_GROUND, 3 + sw - 1);
			if (img != nullptr) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, SO_FOUNDATION, img, north_point);
				this->draw_images.insert(dd);
			}
		}
		if (se != 0) {
			const ImageData *img = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetFoundationSprite, FDT_GROUND, se - 1);
			if (img != nullptr) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, SO_FOUNDATION, img, north_point);
				this->draw_images.insert(dd);
			}
		}
	}

	/* Ground surface. */
	uint8 gslope = SL_FLAT;
	if (voxel != nullptr && voxel->GetGroundType() != GTP_INVALID && !this->vp->GetDisplayFlag(DF_HIDE_SURFACES)) {
		uint8 slope = voxel->GetGroundSlope();
		uint8 type = (this->vp->GetDisplayFlag(DF_UNDERGROUND_MODE)) ? GTP_UNDERGROUND : voxel->GetGroundType();
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND, _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetSurfaceSprite, type, slope, this->orient), north_point);
		this->draw_images.insert(dd);

		if (this->vp->GetDisplayFlag(DF_GRID)) {
			dd.Set(slice, voxel_pos.z, SO_GROUND, _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetCursorSprite, slope, this->orient),
					north_point, nullptr, GS_SEMI_TRANSPARENT);
			this->draw_images.insert(dd);
		}

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
						_sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetFenceSprite, fence_type, edge, gslope, this->orient), north_point);
				if (IsImplodedSteepSlope(gslope) && !IsImplodedSteepSlopeTop(gslope)) dd.z_height++;
				if (GB(fences, 16 + edge, 1) != 0) dd.gs = GS_SEMI_TRANSPARENT;
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
	if (platform_shape != PATH_INVALID && !this->vp->GetDisplayFlag(DF_HIDE_SUPPORTS)) {
		/* Platform gets automatically added when drawing a path or ride, without drawing ground. */
		const ImageData *pl_spr;
		switch (platform_shape) {
			case PATH_RAMP_NE: pl_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetRampPlatformSprite, 2); break;
			case PATH_RAMP_NW: pl_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetRampPlatformSprite, 1); break;
			case PATH_RAMP_SE: pl_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetRampPlatformSprite, 3); break;
			case PATH_RAMP_SW: pl_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetRampPlatformSprite, 0); break;
			default: pl_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetFlatPlatformSprite, this->orient & 1); break;
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
			int yoffset = (voxel_pos.z - height) * TileHeight(this->vp->zoom);  // Compensate y position of support.
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
			const ImageData *img = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetSupportSprite, sprnum);
			if (img != nullptr) {
				DrawData dd;
				dd.Set(slice, height, SO_SUPPORT, img, Point32(north_point.x, north_point.y + yoffset));
				this->draw_images.insert(dd);
			}
		}
	}

	/* Add voxel objects (persons, ride cars, etc). */
	/* Sprites on the bottom part of a steep slope need to be drawn at a higher Z layer to prevent the top slope part obscuring them. */
	const VoxelObject *vo = (voxel == nullptr) ? nullptr : voxel->voxel_objects;
	const uint32 people_z_pos = (voxel == nullptr || voxel->GetGroundType() == GTP_INVALID ||
			!IsImplodedSteepSlope(voxel->GetGroundSlope()) || IsImplodedSteepSlopeTop(voxel->GetGroundSlope())) ? voxel_pos.z : (voxel_pos.z + 1);
	while (vo != nullptr) {
		const Recolouring *recolour;
		const ImageData *anim_spr = vo->GetSprite(this->orient, this->zoom, &recolour);
		if (anim_spr != nullptr && (!this->vp->GetDisplayFlag(DF_HIDE_PEOPLE) || dynamic_cast<const Person*>(vo) == nullptr)) {
			int x_off = ComputeX(vo->pix_pos.x, vo->pix_pos.y);
			int y_off = ComputeY(vo->pix_pos.x, vo->pix_pos.y, vo->pix_pos.z);
			Point32 pos(north_point.x + this->north_offsets[this->orient].x + x_off,
			            north_point.y + this->north_offsets[this->orient].y + y_off);

			DrawData dd;
			dd.Set(slice, people_z_pos, SO_PERSON, anim_spr, pos, recolour);
			this->draw_images.insert(dd);

			if (!this->vp->GetDisplayFlag(DF_HIDE_PEOPLE)) {
				for (const VoxelObject::Overlay &overlay : vo->GetOverlays(this->orient, this->zoom)) {
					if (overlay.sprite != nullptr) {
						dd.Set(slice, people_z_pos, SO_PERSON_OVERLAY, overlay.sprite, pos, overlay.recolour);
						this->draw_images.insert(dd);
					}
				}
			}
		}
		vo = vo->next_object;
	}
}

/**
 * Constructor of the finder data class.
 * @param init_allowed Bit-set of sprite types to look for. @see #ClickableSprite
 * @param init_select What to find of a ground tile.
 */
FinderData::FinderData(ClickableSprite init_allowed, GroundTilePart init_select)
: allowed(init_allowed), select(init_select)
{
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
 * @param init_fdata Finder data.
 */
PixelFinder::PixelFinder(Viewport *vp, FinderData *init_fdata) : VoxelCollector(vp),
	allowed(init_fdata->allowed),
	found(false),
	pixel(_palette[0]), // 0 is transparent, and is not used in sprites.
	fdata(init_fdata)
{
	this->fdata->voxel_pos = XYZPoint16(0, 0, 0);
	this->fdata->person = nullptr;
	this->fdata->ride   = INVALID_RIDE_INSTANCE;
}

PixelFinder::~PixelFinder()
= default;

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
		const ImageData *spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetSurfaceSprite, GTP_CURSOR_EDGE_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND_EDGE, spr, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
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
	if (((this->allowed & CS_RIDE) != 0 && number >= SRI_FULL_RIDES) ||
			((this->allowed & CS_PARK_BORDER) != 0 && number == SRI_SCENERY && voxel->instance_data != INVALID_VOXEL_DATA &&
			_scenery.GetType(voxel->instance_data)->category == SCC_SCENARIO)) {
		/* Looking for a ride? */
		DrawData dd[4];
		int count = DrawRideOrScenery(slice, voxel_pos, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth),
				this->orient, this->zoom, number, voxel->GetInstanceData(), dd, nullptr);
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
		if (this->found && number == SRI_SCENERY) this->data.order = SO_FENCE_FRONT;  // Treat as park entrance rather than ride.
	} else if ((this->allowed & CS_PATH) != 0 && HasValidPath(voxel)) {
		/* Looking for a path? */
		uint16 instance_data = voxel->GetInstanceData();
		const ImageData *img = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetPathSprite,
				GetPathType(instance_data), GetPathStatus(instance_data), GetImplodedPathSlope(instance_data), this->orient);
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_PATH, img, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
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
		const ImageData *spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetSurfaceSprite,
				GTP_CURSOR_TEST, voxel->GetGroundSlope(), this->orient);
		DrawData dd;
		dd.Set(slice, voxel_pos.z, SO_GROUND, spr, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
		if (spr != nullptr && (!this->found || this->data < dd)) {
			uint32 pixel = spr->GetPixel(dd.base.x - spr->xoffset, dd.base.y - spr->yoffset);
			if (GetA(pixel) != TRANSPARENT) {
				this->found = true;
				this->data = dd;
				this->fdata->voxel_pos = voxel_pos;
				this->pixel = pixel;
			}
		}
	} else if ((this->allowed & CS_PARK_BORDER) != 0) {
		for (TileEdge t = EDGE_BEGIN; t < EDGE_COUNT; t++) {
			if (GetFenceType(voxel->GetFences(), t) != FENCE_TYPE_LAND_BORDER) continue;

			const ImageData *img = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetFenceSprite,
					FENCE_TYPE_LAND_BORDER, t, voxel->GetGroundSlope(), this->orient);
			if (img != nullptr) {
				DrawData dd;
				dd.Set(slice, voxel_pos.z, SO_FENCE_BACK, img, Point32(this->rect.base.x - xnorth, this->rect.base.y - ynorth));
				uint32 pixel = img->GetPixel(dd.base.x - img->xoffset, dd.base.y - img->yoffset);
				if (GetA(pixel) != TRANSPARENT && (!this->found || this->data < dd)) {
					this->found = true;
					this->data = dd;
					this->pixel = pixel;
					break;
				}
			}
		}
	}
	if ((this->allowed & CS_PERSON) != 0 && !this->vp->GetDisplayFlag(DF_HIDE_PEOPLE)) {
		/* Looking for persons? */
		for (const VoxelObject *vo = voxel->voxel_objects; vo != nullptr; vo = vo->next_object) {
			const Person *pers = dynamic_cast<const Person *>(vo);
			if (pers == nullptr) continue;
			assert(pers->walk != nullptr);
			AnimationType anim_type = pers->walk->anim_type;
			const ImageData *anim_spr = _sprite_manager.GetSprite(this->zoom, &SpriteStorage::GetAnimationSprite,
					anim_type, pers->frame_index, pers->type, this->orient);
			int x_off = ComputeX(pers->pix_pos.x, pers->pix_pos.y);
			int y_off = ComputeY(pers->pix_pos.x, pers->pix_pos.y, pers->pix_pos.z);
			DrawData dd;
			dd.Set(slice, voxel_pos.z, SO_PERSON, anim_spr, Point32(this->rect.base.x - xnorth - x_off, this->rect.base.y - ynorth - y_off));
			if (anim_spr != nullptr && (!this->found || this->data < dd)) {
				uint32 pixel = anim_spr->GetPixel(dd.base.x - anim_spr->xoffset, dd.base.y - anim_spr->yoffset);
				if (GetA(pixel) != TRANSPARENT) {
					this->found = true;
					this->data = dd;
					this->fdata->voxel_pos = voxel_pos;
					this->pixel = pixel;
					this->fdata->person = pers;
				}
			}
		}
	}
}

/**
 * Constructor with default speed and fade settings.
 * @param text Text to show.
 * @param pos Pixel position in the world.
 * @param col RGB colour of the text.
 */
FloatawayText::FloatawayText(std::string text, XYZPoint32 pos, uint32 col)
: FloatawayText(text, pos, XYZPoint32(3, -6, 0), col | 0xff, 3)
{
}

/**
 * %Viewport constructor.
 * @param init_view_pos Pixel position of the center viewpoint of the main display.
 */
Viewport::Viewport(const XYZPoint32 &init_view_pos) : Window(WC_MAINDISPLAY, ALL_WINDOWS_OF_TYPE),
	view_pos(init_view_pos),
	zoom(DEFAULT_ZOOM),
	orientation(VOR_NORTH),
	mouse_pos(0, 0),
#ifndef NDEBUG
	display_flags(DF_FPS)
#else
	display_flags(DF_NONE)
#endif
{
	uint16 width  = _video.Width();
	uint16 height = _video.Height();

	assert(width >= 120 && height >= 120); // Arbitrary lower limit as sanity check.

	this->SetSize(width, height);
	this->SetPosition(0, 0);
}

Viewport::~Viewport()
= default;

/**
 * Check if the zoom scale can be further decreased.
 * @return We can zoom out.
 */
bool Viewport::CanZoomOut() const
{
	return this->zoom > 0;
}

/**
 * Check if the zoom scale can be further increased.
 * @return We can zoom in.
 */
bool Viewport::CanZoomIn() const
{
	return this->zoom + 1 < static_cast<int>(lengthof(_zoom_scales));
}

/** Decrease the zoom scale by one unit. */
void Viewport::ZoomOut()
{
	assert(this->CanZoomOut());
	--this->zoom;
}

/** Increase the zoom scale by one unit. */
void Viewport::ZoomIn()
{
	assert(this->CanZoomIn());
	++this->zoom;
}

/**
 * Draw a floataway amount of money.
 * @param money Amount of money paid.
 * @param voxel Voxel coordinate where to draw the text.
 */
void Viewport::AddFloatawayMoneyAmount(const Money &money, const XYZPoint16 &voxel)
{
	if (money == 0) return;
	_str_params.SetMoney(1, -money);
	this->floataway_texts.emplace_back(DrawText(STR_ARG1),
			XYZPoint32(voxel.x * 256 + 128, voxel.y * 256 + 128, voxel.z * 256 + 128),
			money < 0 ? 0x00ff0000 : 0xff000000);
}

/**
 * Get relative X position of a point in the world (used for marking window parts dirty).
 * @param xpos X world position.
 * @param ypos Y world position.
 * @return Relative X position.
 */
int32 Viewport::ComputeX(int32 xpos, int32 ypos)
{
	return ComputeXFunction(xpos, ypos, this->orientation, TileWidth(this->zoom));
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
	return ComputeYFunction(xpos, ypos, zpos, this->orientation, TileWidth(this->zoom), TileHeight(this->zoom));
}

/**
 * Compute the absolute screen coordinate of a given world coordinate.
 * @param pixel Pixel world position.
 * @return Screen coordinate after orientation and offset.
 */
Point32 Viewport::ComputeScreenCoordinate(const XYZPoint32 &pixel) const
{
	int32 dx = pixel.x - this->view_pos.x;
	int32 dy = pixel.y - this->view_pos.y;
	int32 dz = pixel.z - this->view_pos.z;

	Point32 pos(this->rect.base.x + this->rect.width / 2, this->rect.base.y + this->rect.height / 2);
	pos.x -= _orientation_signum_dy[this->orientation] * dx * TileWidth(this->zoom) / 512;
	pos.x += _orientation_signum_dx[this->orientation] * dy * TileWidth(this->zoom) / 512;
	pos.y += _orientation_signum_dy[this->orientation] * dx * TileHeight(this->zoom) / 256 * (this->orientation % 2 == 0 ? 1 : -1);
	pos.y += _orientation_signum_dx[this->orientation] * dy * TileHeight(this->zoom) / 256 * (this->orientation % 2 == 0 ? 1 : -1);
	pos.y -= TileHeight(this->zoom) * dz / 256;
	return pos;
}

void Viewport::OnDraw(MouseModeSelector *selector)
{
	SpriteCollector collector(this);
	collector.SetWindowSize(-static_cast<int>(this->rect.width / 2), -static_cast<int>(this->rect.height / 2), this->rect.width, this->rect.height);
	collector.SetSelector(selector);
	collector.Collect();

	_video.FillRectangle(this->rect, MakeRGBA(0, 0, 0, OPAQUE)); // Black background.

	assert(this->rect.base.x >= 0 && this->rect.base.y >= 0);
	_video.PushClip(this->rect);

	GradientShift gs = static_cast<GradientShift>(GS_LIGHT - _weather.GetWeatherType());
	for (const DrawData &dd : collector.draw_images) {
		const Recolouring &rec = (dd.recolour == nullptr) ? _no_recolour : *dd.recolour;
		_video.BlitImage(dd.base, dd.sprite, rec, dd.gs != GS_INVALID ? dd.gs : gs);

		/* Draw height markers if applicable. */
		GuiTextColours marker_colour;
		if (this->GetDisplayFlag(DF_HEIGHT_MARKERS_RIDES) && dd.order == SO_RIDE) {
			marker_colour = HEIGHT_MARKER_RIDES;
		} else if (this->GetDisplayFlag(DF_HEIGHT_MARKERS_PATHS) && dd.order == SO_PATH) {
			marker_colour = HEIGHT_MARKER_PATHS;
		} else if (this->GetDisplayFlag(DF_HEIGHT_MARKERS_TERRAIN) && dd.order == SO_GROUND) {
			marker_colour = HEIGHT_MARKER_TERRAIN;
		} else {
			continue;
		}

		std::string text = std::to_string(dd.z_height);
		int w, h;
		_video.GetTextSize(text, &w, &h);
		Rectangle32 r(dd.base.x + dd.sprite->xoffset + (dd.sprite->width - w) / 2, dd.base.y + dd.sprite->yoffset + (dd.sprite->height - h) / 2, w, h);
		_video.FillRectangle(r, SetA(_palette[marker_colour], OPACITY_SEMI_TRANSPARENT));
		_video.BlitText(text, _palette[TEXT_BLACK], r.base.x, r.base.y, r.width, ALG_CENTER);
	}

	for (uint i = 0; i < this->floataway_texts.size();) {
		FloatawayText &f = this->floataway_texts.at(i);
		int a = GetA(f.colour);
		a -= f.fade;
		if (a > 0) {
			f.colour = SetA(f.colour, a);
			f.pos += f.speed;
			Point32 pos = ComputeScreenCoordinate(f.pos);
			_video.BlitText(f.text, f.colour, pos.x, pos.y);
			++i;
		} else {
			this->floataway_texts.at(i) = this->floataway_texts.back();
			this->floataway_texts.pop_back();
		}
	}

	if (this->GetDisplayFlag(DF_FPS)) {
		constexpr const int SPACING = 4;
		/* FPS is only interesting for developers, no need to make this translatable. */
		_video.BlitText(Format("FPS: %2.1f (avg. %2.1f)", _video.FPS(), _video.AvgFPS()),
				_palette[TEXT_WHITE], SPACING, SPACING, _video.Width() - 2 * SPACING, ALG_RIGHT);
	}

	_video.PopClip();
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

	NotifyChange(CHG_VIEWPORT_ROTATED, direction);
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
			new_xy.x = this->view_pos.x + dx * 256 / TileWidth(this->zoom) - dy * 512 / TileWidth(this->zoom);
			new_xy.y = this->view_pos.y - dx * 256 / TileWidth(this->zoom) - dy * 512 / TileWidth(this->zoom);
			break;

		case VOR_EAST:
			new_xy.x = this->view_pos.x - dx * 256 / TileWidth(this->zoom) - dy * 512 / TileWidth(this->zoom);
			new_xy.y = this->view_pos.y - dx * 256 / TileWidth(this->zoom) + dy * 512 / TileWidth(this->zoom);
			break;

		case VOR_SOUTH:
			new_xy.x = this->view_pos.x - dx * 256 / TileWidth(this->zoom) + dy * 512 / TileWidth(this->zoom);
			new_xy.y = this->view_pos.y + dx * 256 / TileWidth(this->zoom) + dy * 512 / TileWidth(this->zoom);
			break;

		case VOR_WEST:
			new_xy.x = this->view_pos.x + dx * 256 / TileWidth(this->zoom) + dy * 512 / TileWidth(this->zoom);
			new_xy.y = this->view_pos.y + dx * 256 / TileWidth(this->zoom) - dy * 512 / TileWidth(this->zoom);
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
	}
}

static const int VIEWPORT_SHIFT_ON_ARROW_KEY = 64;  ///< By how many pixels to move the viewport when the user presses an arrow key.

bool Viewport::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	std::optional<KeyboardShortcut> shortcut = _shortcuts.GetShortcut(
			key_code == WMKC_SYMBOL ? Shortcuts::Keybinding(symbol, mod) : Shortcuts::Keybinding(key_code, mod),
			_game_control.main_menu ? Shortcuts::Scope::MAIN_MENU : Shortcuts::Scope::INGAME);
	if (!shortcut.has_value()) return false;

	switch (*shortcut) {
		case KS_MAINMENU_NEW:
			ShowScenarioSelectGui();
			return true;
		case KS_MAINMENU_LOAD:
			ShowLoadGameGui();
			return true;
		case KS_MAINMENU_LAUNCH_EDITOR:
			_game_control.LaunchEditor();
			return true;
		case KS_MAINMENU_QUIT:
			_game_control.QuitGame();
			return true;

		case KS_MAINMENU_SETTINGS:
		case KS_INGAME_SETTINGS:
			ShowSettingGui();
			return true;

		case KS_INGAME_SAVE:
			ShowSaveGameGui();
			return true;
		case KS_INGAME_LOAD:
			ShowLoadGameGui();
			return true;
		case KS_INGAME_MAINMENU:
			ShowConfirmationPrompt(GUI_RETURN_CAPTION, GUI_RETURN_MESSAGE, []() { _game_control.MainMenu(); });
			return true;
		case KS_INGAME_QUIT:
			ShowConfirmationPrompt(GUI_QUIT_CAPTION, GUI_QUIT_MESSAGE, []() { _game_control.QuitGame(); });
			return true;

		case KS_INGAME_SPEED_PAUSE:
			_game_control.speed = GSP_PAUSE;
			return true;
		case KS_INGAME_SPEED_1:
			_game_control.speed = GSP_1;
			return true;
		case KS_INGAME_SPEED_2:
			_game_control.speed = GSP_2;
			return true;
		case KS_INGAME_SPEED_4:
			_game_control.speed = GSP_4;
			return true;
		case KS_INGAME_SPEED_8:
			_game_control.speed = GSP_8;
			return true;
		case KS_INGAME_SPEED_UP:
			if (_game_control.speed + 1 < GSP_COUNT) {
				_game_control.speed++;
				return true;
			}
			return false;
		case KS_INGAME_SPEED_DOWN:
			if (_game_control.speed > GSP_PAUSE) {
				_game_control.speed--;
				return true;
			}
			return false;

		case KS_INGAME_MOVE_LEFT:
			this->MoveViewport(VIEWPORT_SHIFT_ON_ARROW_KEY, 0);
			return true;
		case KS_INGAME_MOVE_RIGHT:
			this->MoveViewport(-VIEWPORT_SHIFT_ON_ARROW_KEY, 0);
			return true;
		case KS_INGAME_MOVE_UP:
			this->MoveViewport(0, VIEWPORT_SHIFT_ON_ARROW_KEY);
			return true;
		case KS_INGAME_MOVE_DOWN:
			this->MoveViewport(0, -VIEWPORT_SHIFT_ON_ARROW_KEY);
			return true;

		case KS_INGAME_ROTATE_CW:
			this->Rotate(-1);
			return true;
		case KS_INGAME_ROTATE_CCW:
			this->Rotate(1);
			return true;

		case KS_INGAME_ZOOM_OUT:
			if (this->CanZoomOut()) {
				this->ZoomOut();
				return true;
			}
			return false;
		case KS_INGAME_ZOOM_IN:
			if (this->CanZoomIn()) {
				this->ZoomIn();
				return true;
			}
			return false;

		case KS_INGAME_TERRAFORM:
			ShowTerraformGui();
			return true;
		case KS_INGAME_PATHS:
			ShowPathBuildGui();
			return true;
		case KS_INGAME_FENCES:
			ShowFenceGui();
			return true;
		case KS_INGAME_SCENERY:
			ShowSceneryGui();
			return true;
		case KS_INGAME_PATH_OBJECTS:
			ShowPathObjectsGui();
			return true;
		case KS_INGAME_RIDES:
			ShowRideSelectGui();
			return true;
		case KS_INGAME_PARK_MANAGEMENT:
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_GENERAL);
			return true;
		case KS_INGAME_STAFF:
			ShowStaffManagementGui();
			return true;
		case KS_INGAME_INBOX:
			ShowInboxGui();
			return true;
		case KS_INGAME_FINANCES:
			ShowFinancesGui();
			return true;

		case KS_INGAME_MINIMAP:
			ShowMinimap();
			return true;
		case KS_FPS:
			this->ToggleDisplayFlag(DF_FPS);
			return true;
		case KS_INGAME_GRID:
			this->ToggleDisplayFlag(DF_GRID);
			return true;
		case KS_INGAME_UNDERGROUND:
			this->ToggleDisplayFlag(DF_UNDERGROUND_MODE);
			return true;
		case KS_INGAME_UNDERWATER:
			this->ToggleDisplayFlag(DF_UNDERWATER_MODE);
			return true;
		case KS_INGAME_WIRE_RIDES:
			this->ToggleDisplayFlag(DF_WIREFRAME_RIDES);
			return true;
		case KS_INGAME_WIRE_SCENERY:
			this->ToggleDisplayFlag(DF_WIREFRAME_SCENERY);
			return true;
		case KS_INGAME_HIDE_PEOPLE:
			this->ToggleDisplayFlag(DF_HIDE_PEOPLE);
			return true;
		case KS_INGAME_HIDE_SUPPORTS:
			this->ToggleDisplayFlag(DF_HIDE_SUPPORTS);
			return true;
		case KS_INGAME_HIDE_SURFACES:
			this->ToggleDisplayFlag(DF_HIDE_SURFACES);
			return true;
		case KS_INGAME_HIDE_FOUNDATIONS:
			this->ToggleDisplayFlag(DF_HIDE_FOUNDATIONS);
			return true;
		case KS_INGAME_HEIGHT_RIDES:
			this->ToggleDisplayFlag(DF_HEIGHT_MARKERS_RIDES);
			break;
		case KS_INGAME_HEIGHT_PATHS:
			this->ToggleDisplayFlag(DF_HEIGHT_MARKERS_PATHS);
			return true;
		case KS_INGAME_HEIGHT_TERRAIN:
			this->ToggleDisplayFlag(DF_HEIGHT_MARKERS_TERRAIN);
			return true;

		default:
			break;
	}
	NOT_REACHED();
}

void Viewport::OnMouseMoveEvent(const Point16 &pos)
{
	if (_game_control.main_menu) return;

	Point16 old_mouse_pos = this->mouse_pos;
	this->mouse_pos = pos;

	if (_window_manager.SelectorMouseMoveEvent(this, pos)) return;

	/* Intercept RMB drag for moving the viewport. */
	if ((_video.GetMouseDragging() & MB_RIGHT) != 0) {
		this->MoveViewport(pos.x - old_mouse_pos.x, pos.y - old_mouse_pos.y);
	}
}

WmMouseEvent Viewport::OnMouseButtonEvent(MouseButtons state, WmMouseEventMode mode)
{
	if (mode != WMEM_PRESS) return WMME_NONE;  // Avoid processing auto-repeat events for the same click for all actions below.
	if (_game_control.main_menu) return WMME_NONE;
	if (_window_manager.SelectorMouseButtonEvent(state)) return WMME_NONE;

	if (state == MB_RIGHT) {
		_video.SetMouseDragging(MB_RIGHT, true, true);
		return WMME_NONE;
	}

	/* Did the user click on something that has a window? */
	FinderData fdata(CS_RIDE | CS_PERSON | CS_PARK_BORDER, FW_TILE);
	switch (this->ComputeCursorPosition(&fdata)) {
		case CS_RIDE:
			if (ShowRideManagementGui(fdata.ride)) return WMME_NONE;
			break;

		case CS_PERSON:
			ShowPersonInfoGui(fdata.person);
			return WMME_NONE;

		case CS_PARK_BORDER:
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_GENERAL);
			return WMME_NONE;

		default: break;
	}

	return WMME_NONE;
}

void Viewport::OnMouseWheelEvent(int direction)
{
	if (_game_control.main_menu) return;
	_window_manager.SelectorMouseWheelEvent(direction);
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
