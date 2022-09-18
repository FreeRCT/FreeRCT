/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.cpp %Path building manager code. */

#include "stdafx.h"
#include "geometry.h"
#include "viewport.h"
#include "path_build.h"
#include "map.h"
#include "finances.h"
#include "gamecontrol.h"
#include "window.h"
#include "math_func.h"

/**
 * Build a path at a tile, and claim the voxels above it as well.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type The type of path to build.
 * @param path_status Whether to build a queue or normal path.
 * @param path_spr Imploded sprite number.
 * @param pay Whether to pay money for this path.
 * @see RemovePathAtTile
 */
static void BuildPathAtTile(const XYZPoint16 &voxel_pos, PathType path_type, PathStatus path_status, uint8 path_spr, bool pay)
{
	VoxelStack *avs = _world.GetModifyStack(voxel_pos.x, voxel_pos.y);

	Voxel *av = avs->GetCreate(voxel_pos.z, true);
	const int ground_height = _world.GetBaseGroundHeight(voxel_pos.x, voxel_pos.y);
	Money cost = CONSTRUCTION_COST_PATH;
	assert(voxel_pos.z >= ground_height);  // \todo Allow building underground.
	if (av->GetGroundType() == GTP_INVALID) {
		cost += CONSTRUCTION_COST_SUPPORT * (voxel_pos.z - ground_height + 1);
	} else if (path_spr >= PATH_FLAT_COUNT) {
		cost += CONSTRUCTION_COST_SUPPORT;
	}

	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, cost)) return;
	if (_game_control.action_test_mode) {
		ShowCostOrReturnEstimate(cost);
		return;
	}
	if (pay) {
		_finances_manager.PayRideConstruct(cost);
		_window_manager.GetViewport()->AddFloatawayMoneyAmount(cost, voxel_pos);
	}

	av->SetInstance(SRI_PATH);
	uint8 slope = AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, path_status);
	av->SetInstanceData(MakePathInstanceData(slope, path_type, path_status));

	av = avs->GetCreate(voxel_pos.z + 1, true);
	av->ClearVoxel();
	av->SetInstance(SRI_PATH);
	av->SetInstanceData(PATH_INVALID);

	if (path_spr >= PATH_FLAT_COUNT) { // For non-flat sprites, add another voxel.
		av = avs->GetCreate(voxel_pos.z + 2, true);
		av->ClearVoxel();
		av->SetInstance(SRI_PATH);
		av->SetInstanceData(PATH_INVALID);
	}
}

/**
 * Remove a path from a tile, and free the voxels above it as well.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_spr Imploded sprite number.
 * @param pay Whether to receive money for this path.
 * @see BuildPathAtTile
 */
static void RemovePathAtTile(const XYZPoint16 &voxel_pos, uint8 path_spr, bool pay)
{
	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, CONSTRUCTION_COST_PATH_RETURN)) return;
	if (_game_control.action_test_mode) {
		ShowCostOrReturnEstimate(CONSTRUCTION_COST_PATH_RETURN);
		return;
	}

	VoxelStack *avs = _world.GetModifyStack(voxel_pos.x, voxel_pos.y);

	Voxel *av = avs->GetCreate(voxel_pos.z, false);
	av->SetInstance(SRI_FREE);
	av->SetInstanceData(0);
	AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, PAS_UNUSED);

	av = avs->GetCreate(voxel_pos.z + 1, false);
	av->SetInstance(SRI_FREE);
	av->SetInstanceData(0);

	if (path_spr >= PATH_FLAT_COUNT) {
		av = avs->GetCreate(voxel_pos.z + 2, false);
		av->SetInstance(SRI_FREE);
		av->SetInstanceData(0);
	}

	if (pay) {
		_finances_manager.PayRideConstruct(CONSTRUCTION_COST_PATH_RETURN);
		_window_manager.GetViewport()->AddFloatawayMoneyAmount(CONSTRUCTION_COST_PATH_RETURN, voxel_pos);
	}
}

/**
 * Change the path type of a currently existing path.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type The type of path to change to.
 * @param path_status Whether to build a queue or normal path.
 * @param path_spr Imploded sprite number.
 * @param pay Whether to pay money for this path.
 */
static void ChangePathAtTile(const XYZPoint16 &voxel_pos, PathType path_type, PathStatus path_status, uint8 path_spr, bool pay)
{
	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, CONSTRUCTION_COST_PATH_CHANGE)) return;
	if (_game_control.action_test_mode) {
		ShowCostOrReturnEstimate(CONSTRUCTION_COST_PATH_CHANGE);
		return;
	}

	VoxelStack *avs = _world.GetModifyStack(voxel_pos.x, voxel_pos.y);

	Voxel *av = avs->GetCreate(voxel_pos.z, false);
	AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, PAS_UNUSED);

	/* Reset flat path to one without edges or corners. */
	if (path_spr < PATH_FLAT_COUNT)
		path_spr = PATH_EMPTY;

	uint8 slope = AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, path_status);
	av->SetInstanceData(MakePathInstanceData(slope, path_type, path_status));

	if (pay) {
		_finances_manager.PayRideConstruct(CONSTRUCTION_COST_PATH_CHANGE);
		_window_manager.GetViewport()->AddFloatawayMoneyAmount(CONSTRUCTION_COST_PATH_CHANGE, voxel_pos);
	}
}

/**
 * Does a path run at/to the bottom the given voxel in the neighbouring voxel?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Direction to move to get the neighbouring voxel.
 * @return Whether a path exists at the bottom of the neighbouring voxel.
 * @pre voxel coordinate must be valid in the world.
 * @todo Merge with path computations in the path placement.
 */
bool PathExistsAtBottomEdge(XYZPoint16 voxel_pos, TileEdge edge)
{
	voxel_pos.x += _tile_dxy[edge].x;
	voxel_pos.y += _tile_dxy[edge].y;
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;

	const Voxel *vx = _world.GetVoxel(voxel_pos);
	if (vx == nullptr || !HasValidPath(vx)) {
		/* No path here, check the voxel below. */
		if (voxel_pos.z == 0) return false;
		voxel_pos.z--;
		vx = _world.GetVoxel(voxel_pos);
		if (vx == nullptr || !HasValidPath(vx)) return false;
		/* Path must end at the top of the voxel. */
		return GetImplodedPathSlope(vx) == _path_up_from_edge[edge];
	}
	/* Path must end at the bottom of the voxel. */
	return GetImplodedPathSlope(vx) < PATH_FLAT_COUNT || GetImplodedPathSlope(vx) == _path_down_from_edge[edge];
}

/**
 * In the given voxel, can an upward path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param path_status Whether to build a queue or normal path.
 * @param test_only Only test whether it could be created.
 * @param pay Whether to pay money for this path.
 * @return Whether the path is or could be built.
 */
bool BuildUpwardPath(const XYZPoint16 &voxel_pos, TileEdge edge, PathType path_type, PathStatus path_status, bool test_only, bool pay)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z < 0 || voxel_pos.z > WORLD_Z_SIZE - 3) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z + 1); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z + 2); // 2 voxels higher should also be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
			if ((slope & (TSB_STEEP | TSB_TOP)) == TSB_STEEP) return false;
			if ((slope & _corners_at_edge[edge]) != 0) return false; // A raised corner at 'edge'.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) BuildPathAtTile(voxel_pos, path_type, path_status, _path_up_from_edge[edge], pay);
	return true;
}

/**
 * In the given voxel, can a flat path be build in the voxel?
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param path_status Whether to build a queue or normal path.
 * @param test_only Only test whether it could be created.
 * @param pay Whether to pay money for this path.
 * @return Whether the path is or could be built.
 */
bool BuildFlatPath(const XYZPoint16 &voxel_pos, PathType path_type, PathStatus path_status, bool test_only, bool pay)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z < 0 || voxel_pos.z > WORLD_Z_SIZE - 2) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z + 1); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;
	v = vs->Get(voxel_pos.z);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			if (v->GetGroundSlope() >= PATH_FLAT_COUNT) return false; // Non-flat surface.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) BuildPathAtTile(voxel_pos, path_type, path_status, PATH_EMPTY, pay);
	return true;
}

/**
 * In the given voxel, can an downward path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param path_status Whether to build a queue or normal path.
 * @param test_only Only test whether it could be created.
 * @param pay Whether to pay money for this path.
 * @return Whether the path is or could be built.
 */
bool BuildDownwardPath(XYZPoint16 voxel_pos, TileEdge edge, PathType path_type, PathStatus path_status, bool test_only, bool pay)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z <= 0 || voxel_pos.z > WORLD_Z_SIZE - 3) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z + 1); // 1 voxel higher should also be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z - 1);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
			if ((slope & (TSB_STEEP | TSB_TOP)) == TSB_STEEP) return false;
			if ((slope & _corners_at_edge[(edge + 2) & 3]) != 0) return false; // A raised corner at the opposite 'edge'.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) {
		voxel_pos.z--;
		BuildPathAtTile(voxel_pos, path_type, path_status, _path_down_from_edge[edge], pay);
	}
	return true;
}

/**
 * (Try to) remove a path from the given voxel.
 * @param voxel_pos Coordinate of the voxel.
 * @param test_only Only test whether it could be removed.
 * @param pay Whether to pay money for this path.
 * @return Whether the path is or could be removed.
 */
bool RemovePath(const XYZPoint16 &voxel_pos, bool test_only, bool pay)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z <= 0 || voxel_pos.z > WORLD_Z_SIZE - 2) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z);
	if (v == nullptr || !HasValidPath(v)) return false;
	PathSprites ps = GetImplodedPathSlope(v);
	assert(ps < PATH_COUNT);

	v = vs->Get(voxel_pos.z + 1);
	assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	if (ps >= PATH_FLAT_COUNT) {
		v = vs->Get(voxel_pos.z + 2);
		assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	}

	if (!test_only) RemovePathAtTile(voxel_pos, ps, pay);
	return true;
}

/**
 * (Try to) change the path type of the current path at the given voxel.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type For changing (ie not \a test_only), the type of path to change to.
 * @param path_status Whether to build a queue or normal path.
 * @param test_only Only test whether it could be changed.
 * @param pay Whether to pay money for this path.
 * @return Whether the path's type could be changed.
 */
bool ChangePath(const XYZPoint16 &voxel_pos, PathType path_type, PathStatus path_status, bool test_only, bool pay)
{
	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z);
	if (v == nullptr || !HasValidPath(v)) return false;
	PathSprites ps = GetImplodedPathSlope(v);
	assert(ps < PATH_COUNT);

	v = vs->Get(voxel_pos.z + 1);
	assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	if (ps >= PATH_FLAT_COUNT) {
		v = vs->Get(voxel_pos.z + 2);
		assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	}

	if (!test_only) ChangePathAtTile(voxel_pos, path_type, path_status, ps, pay);
	return true;
}

/**
 * In the given voxel, can a path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @return Bit-set of track slopes, indicating the directions of building paths.
 */
uint8 CanBuildPathFromEdge(const XYZPoint16 &voxel_pos, TileEdge edge)
{
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return 0;
	if (voxel_pos.z < 0 || voxel_pos.z >= WORLD_Z_SIZE - 1) return 0;

	/* If other side of the edge is not on-world or not owned, don't compute path options. */
	Point16 dxy = _tile_dxy[edge];
	if (!IsVoxelstackInsideWorld(voxel_pos.x + dxy.x, voxel_pos.y + dxy.y) ||
			(_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x + dxy.x, voxel_pos.y + dxy.y) != OWN_PARK)) return 0;

	const Voxel *v = _world.GetVoxel(voxel_pos);
	if (v != nullptr && HasValidPath(v)) {
		PathSprites ps = GetImplodedPathSlope(v);
		if (ps < PATH_FLAT_COUNT) return 1 << TSL_FLAT;
		if (ps == _path_up_from_edge[edge]) return 1 << TSL_UP;
	}
	if (voxel_pos.z > 0) {
		v = _world.GetVoxel(voxel_pos + XYZPoint16(0, 0, -1));
		if (v != nullptr &&  HasValidPath(v) && GetImplodedPathSlope(v) == _path_down_from_edge[edge]) return 1 << TSL_DOWN;
	}

	uint8 slopes = 0;
	// XXX Check for already existing paths.
	if (BuildDownwardPath(voxel_pos, edge, PAT_INVALID, PAS_UNUSED, true, false)) slopes |= 1 << TSL_DOWN;
	if (BuildFlatPath(voxel_pos, PAT_INVALID, PAS_UNUSED, true, false)) slopes |= 1 << TSL_FLAT;
	if (BuildUpwardPath(voxel_pos, edge, PAT_INVALID, PAS_UNUSED, true, false)) slopes |= 1 << TSL_UP;
	return slopes;
}

/**
 * Compute the attach points of a path in a voxel.
 * @param voxel_pos Coordinate of the voxel.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
uint8 GetPathAttachPoints(const XYZPoint16 &voxel_pos)
{
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return 0;
	if (voxel_pos.z >= WORLD_Z_SIZE - 1) return 0; // The voxel containing the flat path, and one above it.

	const Voxel *v = _world.GetVoxel(voxel_pos);
	if (v == nullptr) return 0;

	uint8 edges = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		uint16 x = voxel_pos.x + _tile_dxy[edge].x;
		uint16 y = voxel_pos.y + _tile_dxy[edge].y;
		if (!IsVoxelstackInsideWorld(x, y)) continue;

		const XYZPoint16 new_pos(x, y, voxel_pos.z);
		if (HasValidPath(v)) {
			PathSprites ps = GetImplodedPathSlope(v);
			if (ps < PATH_FLAT_COUNT) {
				if (CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				const XYZPoint16 new_above(x, y, voxel_pos.z + 1);
				if (_path_up_from_edge[edge] == ps
						&& CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
				if (_path_down_from_edge[edge] == ps
						&& CanBuildPathFromEdge(new_above, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
			if ((ts & TSB_STEEP) != 0) continue;
			if ((ts & _corners_at_edge[edge]) == 0) {
				if (CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				const XYZPoint16 new_above(x, y, voxel_pos.z + 1);
				if (CanBuildPathFromEdge(new_above, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		/* No path and no ground -> Invalid, do next edge. */
	}
	return edges;
}
