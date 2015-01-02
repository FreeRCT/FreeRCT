/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_finding.h Path finder code. */

#include "stdafx.h"
#include "path_finding.h"
#include "map.h"

/**
 * Constructor of a walked position.
 * @param cur_vox Current voxel position.
 * @param traveled Length of travel from the starting point so far.
 * @param estimate Estimated length of remaining travel to the destination (should be less or equal to the real value for optimal solutions).
 * @param prev_pos Previous walked position used to get at the new position, \c nullptr for the first position.
 */
WalkedPosition::WalkedPosition(const XYZPoint16 &cur_vox, uint32 traveled, uint32 estimate, const WalkedPosition *prev_pos)
		: cur_vox(cur_vox), traveled(traveled), estimate(estimate), prev_pos(prev_pos)
{
}

/**
* Compare two walked positions, and order on position.
* @param wp1 First position to compare.
* @param wp2 Second position to compare.
* @return Whether \a wp1 should come before \a wp2.
*/
bool operator<(const WalkedPosition &wp1, const WalkedPosition &wp2)
{
	return wp1.cur_vox < wp2.cur_vox;
}

/**
 * Walked distance constructor.
 * @param traveled Length of travel from the starting point to \a pos.
 * @param estimate Estimated length of remaining travel from \a pos to the destination.
 * @param pos Current position.
 */
WalkedDistance::WalkedDistance(uint32 traveled, uint32 estimate, const WalkedPosition *pos) : traveled(traveled), estimate(estimate), pos(pos)
{
}

/**
 * Compare two walked distances, and order on minimal total distance.
 * @param wd1 First distance to compare.
 * @param wd2 Second distance to compare.
 * @return Whether \a wd1 should come before \a wd2.
 */
bool operator<(const WalkedDistance &wd1, const WalkedDistance &wd2)
{
	uint32 total1 = wd1.traveled + wd1.estimate;
	uint32 total2 = wd2.traveled + wd2.estimate;
	if (total1 != total2) return total1 < total2;
	return wd1.traveled < wd2.traveled;
}

/**
 * Constructor, find a path to (\a dest_x, \a dest_y, \a dest_z). Give starting points through PathSearcher::AddStart.
 * @param dest_vox Coordinate of the destination voxel.
 */
PathSearcher::PathSearcher(const XYZPoint16 &dest_vox)
{
	this->dest_vox = dest_vox;
	this->dest_pos = nullptr;
}

/**
 * Add a starting point to the searcher.
 * @param start_vox Coordinate of the start voxel.
 */
void PathSearcher::AddStart(const XYZPoint16 &start_vox)
{
	this->AddOpen(start_vox, 0, nullptr);
}

/**
 * Get an (optimistic) estimate of the path length to go to the destination voxel.
 * @param vox Current position in voxels.
 * @return Estimate of the length of path still to go.
 */
inline uint32 PathSearcher::GetEstimate(const XYZPoint16 &vox)
{
	int32 val = abs(vox.x - this->dest_vox.x) + abs(vox.y - this->dest_vox.y);
	if (val < abs(vox.z - this->dest_vox.z)) return abs(vox.z - this->dest_vox.z);
	return val;
}

/**
 * Add a new open position to the set of open points, if it is better than already available.
 * @param vox Position of the current position.
 * @param traveled Distance traveled to get to the current position.
 * @param prev_pos Previous position (\c nullptr for the start position).
 */
void PathSearcher::AddOpen(const XYZPoint16 &vox, uint32 traveled, const WalkedPosition *prev_pos)
{
	uint32 estimate = this->GetEstimate(vox);
	WalkedPosition wp(vox, traveled, estimate, prev_pos);

	/* Find the position. */
	PositionSet::iterator pos_iter = this->positions.find(wp);
	if (pos_iter == positions.end()) { // New position.
		pos_iter = this->positions.insert(wp).first;
		this->open_points.emplace(traveled, estimate, &(*pos_iter));
		return;
	}
	/* Existing position, update if needed. */
	const WalkedPosition *pwp = &(*pos_iter);
	if (pwp->traveled + pwp->estimate <= traveled + estimate) return;

	/* New one is better, update. */
	pwp->traveled = traveled;
	pwp->estimate = estimate; // The sum is changed, making any old open points invalid.
	pwp->prev_pos = prev_pos;
	this->open_points.emplace(traveled, estimate, pwp);
}

/**
 * Search for a path to the destination.
 * @return Whether a path has been found.
 */
bool PathSearcher::Search()
{
	this->dest_pos = nullptr;
	while (!open_points.empty()) {
		WalkedDistance wd = *open_points.begin();
		open_points.erase(open_points.begin());

		if (wd.traveled != wd.pos->traveled || wd.estimate != wd.pos->estimate) continue; // Invalid open point.

		/* Reached the destination? */
		const WalkedPosition *wp = wd.pos;
		if (wp->cur_vox == this->dest_vox) {
			this->dest_pos = wp;
			return true;
		}

		/* Add new open points. */
		const Voxel *v = _world.GetVoxel(wp->cur_vox);
		if (v == nullptr) continue; // No voxel at the expected point, don't bother.

		uint8 exits = GetPathExits(v);
		for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
			if ((exits & (0x11 << edge)) == 0) continue;

			/* There is an outgoing connection, is it also on the world? */
			Point16 dxy = _tile_dxy[edge];
			if (dxy.x < 0 && wp->cur_vox.x == 0) continue;
			if (dxy.x > 0 && wp->cur_vox.x + 1 == _world.GetXSize()) continue;
			if (dxy.y < 0 && wp->cur_vox.y == 0) continue;
			if (dxy.y > 0 && wp->cur_vox.y + 1 == _world.GetYSize()) continue;

			int extra_z = ((exits & (0x10 << edge)) != 0);
			if (wp->cur_vox.z + extra_z < 0 || wp->cur_vox.z + extra_z >= WORLD_Z_SIZE) continue;

			/* Now check the other side, new_z is the voxel where the path should be at the bottom. */
			const Voxel *v2 = _world.GetVoxel(wp->cur_vox + XYZPoint16(dxy.x, dxy.y, extra_z));
			if (v2 == nullptr) continue;

			uint8 other_exits = GetPathExits(v2);
			if ((other_exits & (1 << ((edge + 2) % 4))) == 0) { // No path here, try one voxel below
				extra_z--;
				if (wp->cur_vox.z + extra_z < 0) continue;
				v2 = _world.GetVoxel(wp->cur_vox + XYZPoint16(dxy.x, dxy.y, extra_z));
				if (v2 == nullptr) continue;
				other_exits = GetPathExits(v2);
				if ((other_exits & (0x10 << ((edge + 2) % 4))) == 0) continue;
			}
			/* Add new open point to the path finder. */
			this->AddOpen(wp->cur_vox + XYZPoint16(dxy.x, dxy.y, extra_z), wp->traveled + 1, wp);
		}
	}
	return false;
}

/** Clear the used data structures of the path searcher. */
void PathSearcher::Clear()
{
	this->open_points.clear();
	this->positions.clear();
	this->dest_pos = nullptr;
}

