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

	uint8 GetOrigHeight(TileCorner corner) const;
	bool GetCornerModified(TileCorner corner) const;
	void SetCornerModified(TileCorner corner);
};

/**
 * Map of voxels to ground modification data.
 * @ingroup map_group
 */
typedef std::map<Point16, GroundData> GroundModificationMap;

/**
 * Store and manage terrain changes.
 * @todo Enable pulling the screen min/max coordinates from it, so we can give a good estimate of the area to redraw.
 * @ingroup map_group
 */
class TerrainChanges {
public:
	TerrainChanges(const Point16 &base, uint16 xsize, uint16 ysize);
	~TerrainChanges();

	void UpdatelevellingHeight(const Point16 &pos, int direction, uint8 *height);
	bool ChangeVoxel(const Point16 &pos, uint8 height, int direction);
	bool ChangeCorner(const Point16 &pos, TileCorner corner, int direction);
	bool ModifyWorld(int direction);

	GroundModificationMap changes; ///< Registered changes.

private:
	Point16 base; ///< Base position of the smooth changing world.
	uint16 xsize; ///< Horizontal size of the smooth changing world.
	uint16 ysize; ///< Vertical size of the smooth changing world.

	GroundData *GetGroundData(const Point16 &pos);
};

void ChangeTileCursorMode(const Point16 &voxel_pos, CursorType ctype, Viewport *vp, bool levelling, int direction, bool dot_mode);
void ChangeAreaCursorMode(const Rectangle16 &area, Viewport *vp, bool levelling, int direction);

#endif
