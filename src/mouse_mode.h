/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file mouse_mode.h Mouse mode handling. */

#ifndef MOUSE_MODE_H
#define MOUSE_MODE_H

#include "geometry.h"
#include "math_func.h"
#include "map.h"

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
	CUR_TYPE_EDGE_NE,  ///< Show a NE edge sprite highlight.
	CUR_TYPE_EDGE_SE,  ///< Show a SE edge sprite highlight.
	CUR_TYPE_EDGE_SW,  ///< Show a SW edge sprite highlight.
	CUR_TYPE_EDGE_NW,  ///< Show a NW edge sprite highlight.

	CUR_TYPE_INVALID = 0xFF, ///< Invalid/unused cursor.
};

static const uint32 INVALID_TILE_INDEX = 0xFFFFFFFFu; ///< Invalid selector tile index.

/** Base class for displaying and handling mouse modes from a window. */
class MouseModeSelector {
public:
	MouseModeSelector();
	MouseModeSelector(CursorType cur_cursor);
	virtual ~MouseModeSelector();

	virtual CursorType GetCursor(const XYZPoint16 &voxel_pos) = 0;
	virtual uint32 GetZRange(uint xpos, uint ypos) = 0;

	/**
	 * Get the ride data of a voxel for rendering.
	 * @param voxel %Voxel being rendered (may be null).
	 * @param voxel_pos Position of the voxel in the world.
	 * @param [inout] sri Ride instance that should be rendered.
	 * @param [inout] instance_data Instance data that should be rendered.
	 * @param [inout] sprite Background sprite that should be rendered, if any.
	 * @return Whether to highlight returned ride.
	 */
	virtual bool GetRide([[maybe_unused]] const Voxel *voxel, [[maybe_unused]] const XYZPoint16 &voxel_pos,
			[[maybe_unused]] SmallRideInstance *sri, [[maybe_unused]] uint16 *instance_data, [[maybe_unused]] const ImageData **sprite)
	{
		return false;
	}

	/**
	 * Get the fences of the voxel for rendering.
	 * @param voxel %Voxel being rendered (may be null).
	 * @param voxel_pos Position of the voxel in the world.
	 * @param fences Fence data in the world, bottom 16 bit are fences themselves,
	 * 	bit 16..19 denote whether to highlight the fence at the edge (bit 16 for #EDGE_NE, bit 17 for #EDGE_SE, and so on).
	 * 	Highlighting is always off.
	 * @return Fence data to draw, including highlighting.
	 * @see GetFenceType, SetFenceType
	 */
	virtual uint32 GetFences([[maybe_unused]] const Voxel *voxel, [[maybe_unused]] const XYZPoint16 &voxel_pos, uint32 fences)
	{
		return fences;
	}


	/**
	 * Get the offset of the tile position in the area. Parameters are unchecked.
	 * @param rel_x Horizontal relative offset in the area.
	 * @param rel_y Vertical relative offset in the area.
	 * @return Index in the tile data.
	 */
	inline uint32 GetTileOffset(int rel_x, int rel_y) const
	{
		return rel_x * this->area.height + rel_y;
	}

	/**
	 * Get the index of the tile position in the area.
	 * @param x Horizontal coordinate of the absolute world position.
	 * @param y Vertical coordinate of the absolute world position.
	 * @return Index in the tile data, or #INVALID_TILE_INDEX if out of bounds.
	 */
	inline uint32 GetTileIndex(int x, int y) const
	{
		x -= this->area.base.x;
		if (x < 0 || x >= this->area.width) return INVALID_TILE_INDEX;
		y -= this->area.base.y;
		if (y < 0 || y >= this->area.height) return INVALID_TILE_INDEX;
		return GetTileOffset(x, y);
	}

	/**
	 * Get the index of the tile position in the area.
	 * @param pos Absolute world position.
	 * @return Index in the tile data, or #INVALID_TILE_INDEX if out of bounds.
	 */
	inline uint32 GetTileIndex(const Point16 &pos) const
	{
		return this->GetTileIndex(pos.x, pos.y);
	}

	/**
	 * Rough estimate whether the selector wants to render something in the voxel stack at the given coordinate.
	 * @param x X position of the stack.
	 * @param y Y position of the stack.
	 * @return Whether the selector wants to contribute to the graphics in the given stack.
	 */
	inline bool IsInsideArea(int x, int y) const
	{
		return this->GetTileIndex(x, y) != INVALID_TILE_INDEX;
	}

	Rectangle16 area;      ///< Position and size of the selected area (over-approximation of voxel stacks).
	CursorType cur_cursor; ///< %Cursor to return at the #GetCursor call.
};

/** %Cursor data of a tile. */
struct CursorTileData {
	virtual ~CursorTileData() = default;

	int8 cursor_height;  ///< Height of the cursor (equal to ground height, except at steep slopes). Negative value means 'unknown'.
	bool cursor_enabled; ///< Whether the tile should have a cursor displayed.

	/** Initialize #CursorTileData data members. */
	virtual void Init()
	{
		this->cursor_height = -1;
		this->cursor_enabled = false;
	}

	/**
	 * Get the height of the ground at the tile (top-voxel in case of steep slope).
	 * @param abs_x Absolute world X position of the tile.
	 * @param abs_y Absolute world Y position of the tile.
	 * @return Height of the ground at the tile (top-voxel in case of steep slope).
	 */
	int8 GetGroundHeight(int abs_x, int abs_y)
	{
		if (this->cursor_height >= 0) return this->cursor_height;
		this->cursor_height = _world.GetTopGroundHeight(abs_x, abs_y);
		return this->cursor_height;
	}
};

/** Template for a mouse mode selector with an area of data. */
template <typename TileData>
class TileDataMouseMode : public MouseModeSelector {
public:
	TileDataMouseMode() : MouseModeSelector(CUR_TYPE_TILE), default_enable_cursors(false)
	{
	}

	~TileDataMouseMode()
	{
	}

	CursorType GetCursor(const XYZPoint16 &voxel_pos) override
	{
		uint32 index = this->GetTileIndex(voxel_pos.x, voxel_pos.y);
		if (index == INVALID_TILE_INDEX) return CUR_TYPE_INVALID;
		TileData &td = this->tile_data[index];
		if (td.cursor_enabled && td.GetGroundHeight(voxel_pos.x, voxel_pos.y) == voxel_pos.z) return this->cur_cursor;
		return CUR_TYPE_INVALID;
	}

	uint32 GetZRange([[maybe_unused]] uint xpos, [[maybe_unused]] uint ypos) override
	{
		return 0;
	}

	/**
	 * Set the size of the cursor area.
	 * @param xsize Horizontal size of the area.
	 * @param ysize Vertical size of the area.
	 * @pre World must have been marked dirty before moving the area.
	 * @post World must be marked dirty after moving the area.
	 */
	void SetSize(int xsize, int ysize)
	{
		xsize = Clamp(xsize, 0, 128); // Arbitrary upper limit.
		ysize = Clamp(ysize, 0, 128);
		this->area.width = xsize;
		this->area.height = ysize;
		this->InitTileData();
	}

	/**
	 * Set the position of the cursor area. Clears the cursor and range data.
	 * @param xbase X coordinate of the base position.
	 * @param ybase Y coordinate of the base position.
	 * @pre World must have been marked dirty before moving the area.
	 * @post World must be marked dirty after moving the area.
	 */
	void SetPosition(int xbase, int ybase)
	{
		this->area.base.x = xbase;
		this->area.base.y = ybase;
		this->InitTileData();
	}

	/** Initialize the tile data of the cursor area. */
	void InitTileData()
	{
		if (this->area.width == 0 || this->area.height == 0) return;

		/* Setup the cursor area for the current position and size. */
		int size = this->area.width * this->area.height;
		this->tile_data.resize(size);
		for (int x = 0; x < this->area.width; x++) {
			int xpos = this->area.base.x + x;
			for (int y = 0; y < this->area.height; y++) {
				int ypos = this->area.base.y + y;
				TileData &td = this->tile_data[this->GetTileOffset(x, y)];
				td.Init();
				td.cursor_enabled = (this->default_enable_cursors && IsVoxelstackInsideWorld(xpos, ypos) && _world.GetTileOwner(xpos, ypos) == OWN_PARK);
			}
		}
	}

	std::vector<TileData> tile_data; ///< Tile data of the area.
	bool default_enable_cursors;     ///< Draw cursor sprites in every voxel covered by the selector by default.
};

/** Ride information of a voxel. */
struct VoxelRideData {
	SmallRideInstance sri;    ///< Instance using the voxel.
	uint16 instance_data;     ///< Data of the instance.
	const ImageData *sprite;  ///< Background sprite to render.

	/** Initialization of the voxel ride data. */
	inline void Setup() {
		this->sri = SRI_FREE;
		this->sprite = nullptr;
	}
};

/** Fence information of a voxel. */
struct VoxelFenceData {
	FenceType fence_type; ///< Type of the fence to show. Only valid if #fence_edge is valid.
	TileEdge fence_edge;  ///< Edge of the fence, or #INVALID_EDGE if not valid.

	/** Initialization. */
	inline void Setup() {
		this->fence_edge = INVALID_EDGE;
	}
};

/**
 * %Tile data with voxel information.
 * @tparam VoxelContentData Data to keep for each voxel in the selector.
 */
template <typename VoxelContentData>
struct VoxelTileData : public CursorTileData {
	virtual ~VoxelTileData() = default;

	uint8 lowest;  ///< Lowest voxel in the stack that should be rendered.
	uint8 highest; ///< Highest voxel in the stack that should be rendered.
	std::vector<VoxelContentData> ride_info; ///< Information of voxels VoxelTileData::lowest to VoxelTileData::highest (inclusive).

	/** Initialize #VoxelTileData and #CursorTileData data members. */
	void Init() override
	{
		this->CursorTileData::Init();
		this->lowest = 1;
		this->highest = 0;
	}

	/**
	 * After initializing the  voxels VoxelTileData::lowest and VoxelTileData::highest data members, initialize
	 * the ride data for all voxels in-between.
	 */
	void SetupRideInfoSpace()
	{
		int size = std::max(0, this->highest - this->lowest + 1);
		this->ride_info.resize(size);
		for (int z = 0; z < size; z++) this->ride_info[z].Setup();
	}

	/**
	 * Add a (z-position) of a voxel to the vertical voxel range to render.
	 * @param zpos Vertical voxel position to add.
	 * @see TileDataMouseMode::GetZRange
	 */
	void AddVoxel(uint8 zpos)
	{
		if (this->lowest > this->highest) {
			this->lowest = zpos;
			this->highest = zpos;
		} else {
			this->lowest = std::min(this->lowest,   zpos);
			this->highest = std::max(this->highest, zpos);
		}
	}

	/**
	 * Get the range of interesting voxels in the stack.
	 * @return The range of interesting voxels (highest z in upper 16 bit, lowest z in lower 16 bit), or \c 0.
	 */
	uint32 GetZRange()
	{
		if (this->lowest > this->highest) return 0;
		uint32 value = this->highest;
		return (value << 16) | this->lowest;
	}
};

/**
 * Template for a mouse mode selector with tile ride-data.
 * @tparam TileData Tile data for each voxel stack covered by the mouse mode.
 */
template <typename TileData>
class VoxelTileDataMouseMode : public TileDataMouseMode<TileData> {
public:
	VoxelTileDataMouseMode() : TileDataMouseMode<TileData>()
	{
	}

	~VoxelTileDataMouseMode()
	{
	}

	/**
	 * Denote that the given voxel will contain part of a ride.
	 * @param pos Absolute world position.
	 */
	void AddVoxel(const XYZPoint16 &pos)
	{
		uint32 index = this->GetTileIndex(pos.x, pos.y);
		assert(index != INVALID_TILE_INDEX);
		TileData &td = this->tile_data[index];
		td.cursor_enabled = true;
		td.AddVoxel(pos.z);
	}

	/**
	 * Setup space for the ride information.
	 * @pre VoxelTileDataMouseMode::AddVoxel must have been done.
	 */
	void SetupRideInfoSpace()
	{
		for (int x = 0; x < this->area.width; x++) {
			for (int y = 0; y < this->area.height; y++) this->tile_data[this->GetTileOffset(x, y)].SetupRideInfoSpace();
		}
	}

	/**
	 * Get the tile data at the given position.
	 * @param pos Position to get (only \c x and \c y are used).
	 * @return Reference of the tile data.
	 */
	TileData &GetTileData(const XYZPoint16 &pos)
	{
		return GetTileData(pos.x, pos.y);
	}

	/**
	 * Get the tile data at the given position.
	 * @param xpos Horizontal coordinate of the position to get.
	 * @param ypos Vertical coordinate of the position to get.
	 * @return Reference of the tile data.
	 */
	TileData &GetTileData(int16 xpos, int16 ypos)
	{
		uint32 index = this->GetTileIndex(xpos, ypos);
		assert(index != INVALID_TILE_INDEX);
		return this->tile_data[index];
	}

	uint32 GetZRange(uint xpos, uint ypos) override
	{
		uint32 index = this->GetTileIndex(xpos, ypos);
		if (index == INVALID_TILE_INDEX) return 0;
		TileData &td = this->tile_data[index];
		return td.GetZRange();
	}
};

typedef TileDataMouseMode<CursorTileData> CursorMouseMode; ///< Mouse mode displaying a cursor of some size at the ground.

/** Mouse mode displaying a cursor and (part of) a ride. */
class RideMouseMode : public VoxelTileDataMouseMode<VoxelTileData<VoxelRideData>> {
public:
	RideMouseMode() : VoxelTileDataMouseMode<VoxelTileData<VoxelRideData>>()
	{
	}

	~RideMouseMode()
	{
	}

	bool GetRide([[maybe_unused]] const Voxel *voxel, const XYZPoint16 &voxel_pos, SmallRideInstance *sri, uint16 *instance_data, const ImageData **sprite) override
	{
		uint32 index = this->GetTileIndex(voxel_pos.x, voxel_pos.y);
		if (index == INVALID_TILE_INDEX) return false;

		const VoxelTileData<VoxelRideData> &td = this->tile_data[index];
		if (!td.cursor_enabled || voxel_pos.z < td.lowest || voxel_pos.z > td.highest) return false;

		const VoxelRideData &vrd = td.ride_info[voxel_pos.z - td.lowest];
		*sri = vrd.sri;
		*instance_data = vrd.instance_data;
		*sprite = vrd.sprite;
		return true;
	}

	/**
	 * Set ride data at the given position in the area. Tiles with disabled cursor are silently skipped.
	 * @param pos World position.
	 * @param sri Ride instance number.
	 * @param instance_data Instance data.
	 * @param sprite Backgroud sprite, if any.
	 */
	void SetRideData(const XYZPoint16 &pos, SmallRideInstance sri, uint16 instance_data, const ImageData *sprite)
	{
		VoxelTileData<VoxelRideData> &td = this->GetTileData(pos);
		if (!td.cursor_enabled) return;
		VoxelRideData &vrd = td.ride_info[pos.z - td.lowest];
		vrd.sri = sri;
		vrd.instance_data = instance_data;
		vrd.sprite = sprite;
	}
};

/** Mouse mode displaying a cursor and fences. */
class FencesMouseMode : public VoxelTileDataMouseMode<VoxelTileData<VoxelFenceData>> {
public:
	FencesMouseMode() : VoxelTileDataMouseMode<VoxelTileData<VoxelFenceData>>()
	{
	}

	~FencesMouseMode()
	{
	}

	uint32 GetFences([[maybe_unused]] const Voxel *voxel, const XYZPoint16 &voxel_pos, uint32 fences) override
	{
		uint32 index = this->GetTileIndex(voxel_pos.x, voxel_pos.y);
		if (index == INVALID_TILE_INDEX) return fences;

		const VoxelTileData<VoxelFenceData> &td = this->tile_data[index];
		if (!td.cursor_enabled || voxel_pos.z < td.lowest || voxel_pos.z > td.highest) return fences;

		const VoxelFenceData &vfd = td.ride_info[voxel_pos.z - td.lowest];
		if (vfd.fence_edge != INVALID_EDGE) {
			fences = SetFenceType(fences, vfd.fence_edge, vfd.fence_type); // Kills the top 16 bit, but they are all 0.
			fences |= 0x10000 << vfd.fence_edge; // Set highlight bit for the new fence.
		}
		return fences;
	}

	/**
	 * Set fence data at the given position in the area. Tiles with disabled cursor are silently skipped.
	 * @param pos World position.
	 * @param fence_type Type of the new fence.
	 * @param edge Edge of the new fence.
	 */
	void SetFenceData(const XYZPoint16 &pos, FenceType fence_type, TileEdge edge)
	{
		VoxelTileData<VoxelFenceData> &td = this->GetTileData(pos);
		if (!td.cursor_enabled) return;
		VoxelFenceData &vfd = td.ride_info[pos.z - td.lowest];
		vfd.fence_type = fence_type;
		vfd.fence_edge = edge;
	}
};

#endif
