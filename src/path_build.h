/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.h %Path building manager declarations. */

#ifndef PATH_BUILD_H
#define PATH_BUILD_H

static const Money PATH_CONSTRUCT_COST_FLAT         (1200);  ///< How much it costs to build a single section of flat path.
static const Money PATH_CONSTRUCT_COST_RAMP         (1400);  ///< How much it costs to build a single section of ramped path.
static const Money PATH_CONSTRUCT_COST_ELEVATED_FLAT(3200);  ///< How much it costs to build a single section of elevated flat path.
static const Money PATH_CONSTRUCT_COST_ELEVATED_RAMP(3800);  ///< How much it costs to build a single section of elevated ramped path.
static const Money PATH_CONSTRUCT_COST_CHANGE       ( 400);  ///< How much it costs to change the type of a single existing path segment.
static const Money PATH_CONSTRUCT_COST_RETURN       (-800);  ///< How much money deletion of a single path segment returns.

bool PathExistsAtBottomEdge(XYZPoint16 voxel_pos, TileEdge edge);
bool BuildUpwardPath(const XYZPoint16 &voxel_pos, TileEdge edge, PathType path_type, bool test_only);
bool BuildFlatPath(const XYZPoint16 &voxel_pos, PathType path_type, bool test_only);
bool BuildDownwardPath(XYZPoint16 voxel_pos, TileEdge edge, PathType path_type, bool test_only);
bool RemovePath(const XYZPoint16 &voxel_pos, bool test_only);
bool ChangePath(const XYZPoint16 &voxel_pos, PathType path_type, bool test_only);
uint8 CanBuildPathFromEdge(const XYZPoint16 &voxel_pos, TileEdge edge);
uint8 GetPathAttachPoints(const XYZPoint16 &voxel_pos);

#endif
