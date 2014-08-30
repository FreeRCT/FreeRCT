/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_finding.h Declarations for path finders. */

#ifndef PATH_FINDING_H
#define PATH_FINDING_H

#include <set>

/** Intermediate position of a walk. */
class WalkedPosition {
public:
	WalkedPosition(int x, int y, int z, uint32 traveled, uint32 estimate, const WalkedPosition *prev_pos);

	int x; ///< X coordinate of the current position.
	int y; ///< Y coordinate of the current position.
	int z; ///< Z coordinate of the current position.
	mutable uint32 traveled; ///< Length of the traveled path so far.
	mutable uint32 estimate; ///< Estimated distance to the destination.
	mutable const WalkedPosition *prev_pos; ///< Position coming from (\c nullptr for initial position).
};

/** Guessed path length at a (partially) explored position. */
class WalkedDistance {
public:
	WalkedDistance(uint32 traveled, uint32 estimate, const WalkedPosition *pos);

	uint32 traveled; ///< Length of the traveled path so far.
	uint32 estimate; ///< Estimated distance to the destination.
	const WalkedPosition *pos; ///< Current position.
};

typedef std::set<WalkedPosition> PositionSet;	  ///< Visited positions (best solution so far).
typedef std::multiset<WalkedDistance> OpenPoints; ///< Points for further exploration.

/** Class for searching (and hopefully finding) a path between tiles. */
class PathSearcher {
public:
	PathSearcher(int dest_x, int dest_y, int dest_z);

	void AddStart(int start_x, int start_y, int start_z);
	bool Search();
	void Clear();

	int dest_x; ///< X coordinate of the desired destination voxel.
	int dest_y; ///< Y coordinate of the desired destination voxel.
	int dest_z; ///< Z coordinate of the desired destination voxel.
	const WalkedPosition *dest_pos; ///< If path was found, this points to the end-point of the walk.

protected:
	PositionSet positions;   ///< Examined positions.
	OpenPoints  open_points; ///< Open points to examine further.

	inline uint32 GetEstimate(int x, int y, int z);
	void AddOpen(int x, int y, int z, uint32 traveled, const WalkedPosition *prev_pos);
};

#endif

