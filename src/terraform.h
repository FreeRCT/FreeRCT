/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform.h Terraform declarations. */

#ifndef TERRAFORM_H
#define TERRAFORM_H

#include <map>

/**
 * Ground data + modification storage.
 * @ingroup map_group
 */
struct GroundData {
	uint8 height;     ///< Height of the voxel with ground.
	uint8 orig_slope; ///< Original slope data.
	uint8 modified;   ///< Raised or lowered corners.

	GroundData(uint8 height, uint8 orig_slope);

	uint8 GetOrigHeight(TileSlope corner) const;
	bool GetCornerModified(TileSlope corner) const;
	void SetCornerModified(TileSlope corner);
};

/**
 * Map of voxels to ground modification data.
 * @ingroup map_group
 */
typedef std::map<Point32, GroundData> GroundModificationMap;

/**
 * Store and manage terrain changes.
 * @todo Enable pulling the screen min/max coordinates from it, so we can give a good estimate of the area to redraw.
 * @ingroup map_group
 */
class TerrainChanges {
public:
	TerrainChanges(const Point32 &base, uint16 xsize, uint16 ysize);
	~TerrainChanges();

	bool ChangeVoxel(const Point32 &pos, uint8 height, int direction);
	bool ChangeCorner(const Point32 &pos, TileSlope corner, int direction);
	bool ModifyWorld(int direction);

	GroundModificationMap changes; ///< Registered changes.

private:
	Point32 base; ///< Base position of the smooth changing world.
	uint16 xsize; ///< Horizontal size of the smooth changing world.
	uint16 ysize; ///< Vertical size of the smooth changing world.

	GroundData *GetGroundData(const Point32 &pos);
};

/** State of the terraform coordinator. */
enum TerraformerState {
	TFS_OFF,      ///< %Window closed.
	TFS_NO_MOUSE, ///< %Window opened, but no mouse mode active.
	TFS_ON,       ///< Active.
};

/**
 * Tile terraforming mouse mode.
 *
 * Terraforms an area of the world.
 * It has two modes, a non-zero size, meaning terraforming is limited to that area, or a zero-size (aka 'dot mode'),
 * which means 'unlimited' (the entire world may get changed, if money (and world contents) permits.
 *
 * The \c true 'leveling' setting means that the lowest parts are raised or the highest parts are lowered.
 * A \c false 'leveling' value means that the entire area is moved up or down.
 */
class TileTerraformMouseMode: public MouseMode {
public:
	TerraformerState state; ///< Own state.
	uint8 mouse_state; ///< Last known state of the mouse.
	uint8 xsize;       ///< Horizontal size of the terraform area. May be \c 0, which means 'dot'.
	uint8 ysize;       ///< Vertical size of the terraform area. May be \c 0, which means 'dot'.

	TileTerraformMouseMode();

	void OpenWindow();
	void CloseWindow();
	void SetSize(int xsize, int ysize);

	void SetCursors();

	virtual bool MayActivateMode();
	virtual void ActivateMode(const Point16 &pos);
	virtual void LeaveMode();
	virtual bool EnableCursors();

	virtual void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos);
	virtual void OnMouseButtonEvent(Viewport *vp, uint8 state);
	virtual void OnMouseWheelEvent(Viewport *vp, int direction);
};

extern TileTerraformMouseMode _terraformer;

#endif

