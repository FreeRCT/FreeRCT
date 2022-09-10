/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.h %Path building manager declarations. */

#ifndef PATH_BUILD_H
#define PATH_BUILD_H

bool PathExistsAtBottomEdge(XYZPoint16 voxel_pos, TileEdge edge);
bool BuildUpwardPath(const XYZPoint16 &voxel_pos, TileEdge edge, PathType path_type, bool test_only, bool pay);
bool BuildFlatPath(const XYZPoint16 &voxel_pos, PathType path_type, bool test_only, bool pay);
bool BuildDownwardPath(XYZPoint16 voxel_pos, TileEdge edge, PathType path_type, bool test_only, bool pay);
bool RemovePath(const XYZPoint16 &voxel_pos, bool test_only, bool pay);
bool ChangePath(const XYZPoint16 &voxel_pos, PathType path_type, bool test_only, bool pay);
uint8 CanBuildPathFromEdge(const XYZPoint16 &voxel_pos, TileEdge edge);
uint8 GetPathAttachPoints(const XYZPoint16 &voxel_pos);

#endif
