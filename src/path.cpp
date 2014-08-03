/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path.cpp %Path definitions. */

#include "stdafx.h"
#include "path.h"
#include "map.h"
#include "viewport.h"

/** Imploded path tile sprite number to use for an 'up' slope from a given edge. */
const PathSprites _path_up_from_edge[EDGE_COUNT] = {
	PATH_RAMP_NE, ///< EDGE_NE
	PATH_RAMP_SE, ///< EDGE_SE
	PATH_RAMP_SW, ///< EDGE_SW
	PATH_RAMP_NW, ///< EDGE_NW
};

/** Imploded path tile sprite number to use for an 'down' slope from a given edge. */
const PathSprites _path_down_from_edge[EDGE_COUNT] = {
	PATH_RAMP_SW, ///< EDGE_NE
	PATH_RAMP_NW, ///< EDGE_SE
	PATH_RAMP_NE, ///< EDGE_SW
	PATH_RAMP_SE, ///< EDGE_NW
};

/**
 * Expansion of path sprite number to a value using path bits (#PATHBIT_N, #PATHBIT_E, #PATHBIT_S, #PATHBIT_W,
 * #PATHBIT_NE, #PATHBIT_SE, #PATHBIT_SW, and #PATHBIT_NW).
 * @ingroup map_group
 */
const uint8 _path_expand[] = {
	  0, // PATH_EMPTY
	 16, // PATH_NE
	 32, // PATH_SE
	 48, // PATH_NE_SE
	 50, // PATH_NE_SE_E
	 64, // PATH_SW
	 80, // PATH_NE_SW
	 96, // PATH_SE_SW
	100, // PATH_SE_SW_S
	112, // PATH_NE_SE_SW
	114, // PATH_NE_SE_SW_E
	116, // PATH_NE_SE_SW_S
	118, // PATH_NE_SE_SW_E_S
	128, // PATH_NW
	144, // PATH_NE_NW
	145, // PATH_NE_NW_N
	160, // PATH_NW_SE
	176, // PATH_NE_NW_SE
	177, // PATH_NE_NW_SE_N
	178, // PATH_NE_NW_SE_E
	179, // PATH_NE_NW_SE_N_E
	192, // PATH_NW_SW
	200, // PATH_NW_SW_W
	208, // PATH_NE_NW_SW
	209, // PATH_NE_NW_SW_N
	216, // PATH_NE_NW_SW_W
	217, // PATH_NE_NW_SW_N_W
	224, // PATH_NW_SE_SW
	228, // PATH_NW_SE_SW_S
	232, // PATH_NW_SE_SW_W
	236, // PATH_NW_SE_SW_S_W
	240, // PATH_NE_NW_SE_SW
	241, // PATH_NE_NW_SE_SW_N
	242, // PATH_NE_NW_SE_SW_E
	243, // PATH_NE_NW_SE_SW_N_E
	244, // PATH_NE_NW_SE_SW_S
	245, // PATH_NE_NW_SE_SW_N_S
	246, // PATH_NE_NW_SE_SW_E_S
	247, // PATH_NE_NW_SE_SW_N_E_S
	248, // PATH_NE_NW_SE_SW_W
	249, // PATH_NE_NW_SE_SW_N_W
	250, // PATH_NE_NW_SE_SW_E_W
	251, // PATH_NE_NW_SE_SW_N_E_W
	252, // PATH_NE_NW_SE_SW_S_W
	253, // PATH_NE_NW_SE_SW_N_S_W
	254, // PATH_NE_NW_SE_SW_E_S_W
	255, // PATH_NE_NW_SE_SW_N_E_S_W
};

/**
 * Inverse operation of #_path_expand. #PATH_INVALID means there is no sprite defined for this combination.
 * Note that even if a sprite is defined in this table, a particular path type may not have a sprite for
 * every combination (for example, queue paths have no corner bits at all).
 * @ingroup map_group
 */
const uint8 _path_implode[256] = {
	PATH_EMPTY,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SE,               PATH_INVALID,             PATH_NE_SE_E,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SW,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_SE_SW_S,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_SE_SW,            PATH_INVALID,             PATH_NE_SE_SW_E,          PATH_INVALID,
	PATH_NE_SE_SW_S,          PATH_INVALID,             PATH_NE_SE_SW_E_S,        PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW,                  PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW,               PATH_NE_NW_N,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SE,            PATH_NE_NW_SE_N,          PATH_NE_NW_SE_E,          PATH_NE_NW_SE_N_E,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SW,               PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SW_W,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SW,            PATH_NE_NW_SW_N,          PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SW_W,          PATH_NE_NW_SW_N_W,        PATH_INVALID,             PATH_INVALID,
	PATH_INVALID,             PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW,            PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_S,          PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_W,          PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NW_SE_SW_S_W,        PATH_INVALID,             PATH_INVALID,             PATH_INVALID,
	PATH_NE_NW_SE_SW,         PATH_NE_NW_SE_SW_N,       PATH_NE_NW_SE_SW_E,       PATH_NE_NW_SE_SW_N_E,
	PATH_NE_NW_SE_SW_S,       PATH_NE_NW_SE_SW_N_S,     PATH_NE_NW_SE_SW_E_S,     PATH_NE_NW_SE_SW_N_E_S,
	PATH_NE_NW_SE_SW_W,       PATH_NE_NW_SE_SW_N_W,     PATH_NE_NW_SE_SW_E_W,     PATH_NE_NW_SE_SW_N_E_W,
	PATH_NE_NW_SE_SW_S_W,     PATH_NE_NW_SE_SW_N_S_W,   PATH_NE_NW_SE_SW_E_S_W,   PATH_NE_NW_SE_SW_N_E_S_W,
};

/** Path sprites to use when rotating the world view. */
const uint8 _path_rotation[PATH_COUNT][4] = {
	{PATH_EMPTY,               PATH_EMPTY,               PATH_EMPTY,               PATH_EMPTY              },
	{PATH_NE,                  PATH_NW,                  PATH_SW,                  PATH_SE                 },
	{PATH_SE,                  PATH_NE,                  PATH_NW,                  PATH_SW                 },
	{PATH_NE_SE,               PATH_NE_NW,               PATH_NW_SW,               PATH_SE_SW              },
	{PATH_NE_SE_E,             PATH_NE_NW_N,             PATH_NW_SW_W,             PATH_SE_SW_S            },
	{PATH_SW,                  PATH_SE,                  PATH_NE,                  PATH_NW                 },
	{PATH_NE_SW,               PATH_NW_SE,               PATH_NE_SW,               PATH_NW_SE              },
	{PATH_SE_SW,               PATH_NE_SE,               PATH_NE_NW,               PATH_NW_SW              },
	{PATH_SE_SW_S,             PATH_NE_SE_E,             PATH_NE_NW_N,             PATH_NW_SW_W            },
	{PATH_NE_SE_SW,            PATH_NE_NW_SE,            PATH_NE_NW_SW,            PATH_NW_SE_SW           },
	{PATH_NE_SE_SW_E,          PATH_NE_NW_SE_N,          PATH_NE_NW_SW_W,          PATH_NW_SE_SW_S         },
	{PATH_NE_SE_SW_S,          PATH_NE_NW_SE_E,          PATH_NE_NW_SW_N,          PATH_NW_SE_SW_W         },
	{PATH_NE_SE_SW_E_S,        PATH_NE_NW_SE_N_E,        PATH_NE_NW_SW_N_W,        PATH_NW_SE_SW_S_W       },
	{PATH_NW,                  PATH_SW,                  PATH_SE,                  PATH_NE                 },
	{PATH_NE_NW,               PATH_NW_SW,               PATH_SE_SW,               PATH_NE_SE              },
	{PATH_NE_NW_N,             PATH_NW_SW_W,             PATH_SE_SW_S,             PATH_NE_SE_E            },
	{PATH_NW_SE,               PATH_NE_SW,               PATH_NW_SE,               PATH_NE_SW              },
	{PATH_NE_NW_SE,            PATH_NE_NW_SW,            PATH_NW_SE_SW,            PATH_NE_SE_SW           },
	{PATH_NE_NW_SE_N,          PATH_NE_NW_SW_W,          PATH_NW_SE_SW_S,          PATH_NE_SE_SW_E         },
	{PATH_NE_NW_SE_E,          PATH_NE_NW_SW_N,          PATH_NW_SE_SW_W,          PATH_NE_SE_SW_S         },
	{PATH_NE_NW_SE_N_E,        PATH_NE_NW_SW_N_W,        PATH_NW_SE_SW_S_W,        PATH_NE_SE_SW_E_S       },
	{PATH_NW_SW,               PATH_SE_SW,               PATH_NE_SE,               PATH_NE_NW              },
	{PATH_NW_SW_W,             PATH_SE_SW_S,             PATH_NE_SE_E,             PATH_NE_NW_N            },
	{PATH_NE_NW_SW,            PATH_NW_SE_SW,            PATH_NE_SE_SW,            PATH_NE_NW_SE           },
	{PATH_NE_NW_SW_N,          PATH_NW_SE_SW_W,          PATH_NE_SE_SW_S,          PATH_NE_NW_SE_E         },
	{PATH_NE_NW_SW_W,          PATH_NW_SE_SW_S,          PATH_NE_SE_SW_E,          PATH_NE_NW_SE_N         },
	{PATH_NE_NW_SW_N_W,        PATH_NW_SE_SW_S_W,        PATH_NE_SE_SW_E_S,        PATH_NE_NW_SE_N_E       },
	{PATH_NW_SE_SW,            PATH_NE_SE_SW,            PATH_NE_NW_SE,            PATH_NE_NW_SW           },
	{PATH_NW_SE_SW_S,          PATH_NE_SE_SW_E,          PATH_NE_NW_SE_N,          PATH_NE_NW_SW_W         },
	{PATH_NW_SE_SW_W,          PATH_NE_SE_SW_S,          PATH_NE_NW_SE_E,          PATH_NE_NW_SW_N         },
	{PATH_NW_SE_SW_S_W,        PATH_NE_SE_SW_E_S,        PATH_NE_NW_SE_N_E,        PATH_NE_NW_SW_N_W       },
	{PATH_NE_NW_SE_SW,         PATH_NE_NW_SE_SW,         PATH_NE_NW_SE_SW,         PATH_NE_NW_SE_SW        },
	{PATH_NE_NW_SE_SW_N,       PATH_NE_NW_SE_SW_W,       PATH_NE_NW_SE_SW_S,       PATH_NE_NW_SE_SW_E      },
	{PATH_NE_NW_SE_SW_E,       PATH_NE_NW_SE_SW_N,       PATH_NE_NW_SE_SW_W,       PATH_NE_NW_SE_SW_S      },
	{PATH_NE_NW_SE_SW_N_E,     PATH_NE_NW_SE_SW_N_W,     PATH_NE_NW_SE_SW_S_W,     PATH_NE_NW_SE_SW_E_S    },
	{PATH_NE_NW_SE_SW_S,       PATH_NE_NW_SE_SW_E,       PATH_NE_NW_SE_SW_N,       PATH_NE_NW_SE_SW_W      },
	{PATH_NE_NW_SE_SW_N_S,     PATH_NE_NW_SE_SW_E_W,     PATH_NE_NW_SE_SW_N_S,     PATH_NE_NW_SE_SW_E_W    },
	{PATH_NE_NW_SE_SW_E_S,     PATH_NE_NW_SE_SW_N_E,     PATH_NE_NW_SE_SW_N_W,     PATH_NE_NW_SE_SW_S_W    },
	{PATH_NE_NW_SE_SW_N_E_S,   PATH_NE_NW_SE_SW_N_E_W,   PATH_NE_NW_SE_SW_N_S_W,   PATH_NE_NW_SE_SW_E_S_W  },
	{PATH_NE_NW_SE_SW_W,       PATH_NE_NW_SE_SW_S,       PATH_NE_NW_SE_SW_E,       PATH_NE_NW_SE_SW_N      },
	{PATH_NE_NW_SE_SW_N_W,     PATH_NE_NW_SE_SW_S_W,     PATH_NE_NW_SE_SW_E_S,     PATH_NE_NW_SE_SW_N_E    },
	{PATH_NE_NW_SE_SW_E_W,     PATH_NE_NW_SE_SW_N_S,     PATH_NE_NW_SE_SW_E_W,     PATH_NE_NW_SE_SW_N_S    },
	{PATH_NE_NW_SE_SW_N_E_W,   PATH_NE_NW_SE_SW_N_S_W,   PATH_NE_NW_SE_SW_E_S_W,   PATH_NE_NW_SE_SW_N_E_S  },
	{PATH_NE_NW_SE_SW_S_W,     PATH_NE_NW_SE_SW_E_S,     PATH_NE_NW_SE_SW_N_E,     PATH_NE_NW_SE_SW_N_W    },
	{PATH_NE_NW_SE_SW_N_S_W,   PATH_NE_NW_SE_SW_E_S_W,   PATH_NE_NW_SE_SW_N_E_S,   PATH_NE_NW_SE_SW_N_E_W  },
	{PATH_NE_NW_SE_SW_E_S_W,   PATH_NE_NW_SE_SW_N_E_S,   PATH_NE_NW_SE_SW_N_E_W,   PATH_NE_NW_SE_SW_N_S_W  },
	{PATH_NE_NW_SE_SW_N_E_S_W, PATH_NE_NW_SE_SW_N_E_S_W, PATH_NE_NW_SE_SW_N_E_S_W, PATH_NE_NW_SE_SW_N_E_S_W},
	{PATH_RAMP_NE,             PATH_RAMP_NW,             PATH_RAMP_SW,             PATH_RAMP_SE            },
	{PATH_RAMP_NW,             PATH_RAMP_SW,             PATH_RAMP_SE,             PATH_RAMP_NE            },
	{PATH_RAMP_SE,             PATH_RAMP_NE,             PATH_RAMP_NW,             PATH_RAMP_SW            },
	{PATH_RAMP_SW,             PATH_RAMP_SE,             PATH_RAMP_NE,             PATH_RAMP_NW            },
};

/**
 * Find all edges that are an exit for a path in the given voxel. No investigation is performed whether the exits connect to anything.
 * @param v %Voxel to investigate.
 * @return Exits for a path in the queried voxel. Lower 4 bits are exits at the bottom; upper 4 bits are exits at the top.
 */
uint8 GetPathExits(const Voxel *v)
{
	SmallRideInstance instance = v->GetInstance();
	if (instance != SRI_PATH) return 0;
	uint16 inst_data = v->GetInstanceData();
	if (!HasValidPath(inst_data)) return 0;

	PathSprites slope = GetImplodedPathSlope(inst_data);
	if (slope < PATH_FLAT_COUNT) { // At a flat path tile.
		return (_path_expand[slope] >> PATHBIT_NE) & 0xF;
	}

	switch (slope) {
		case PATH_RAMP_NE: return (1 << EDGE_NE) | (0x10 << EDGE_SW);
		case PATH_RAMP_NW: return (1 << EDGE_NW) | (0x10 << EDGE_SE);
		case PATH_RAMP_SE: return (1 << EDGE_SE) | (0x10 << EDGE_NW);
		case PATH_RAMP_SW: return (1 << EDGE_SW) | (0x10 << EDGE_NE);
		default: NOT_REACHED();
	}
}

/**
 * Find all edges that are an exit for a path in the given voxel. No investigation is performed whether the exits connect to anything.
 * @param xpos X position of the voxel.
 * @param ypos Y position of the voxel.
 * @param zpos Z position of the voxel.
 * @return Exits for a path in the queried voxel. Lower 4 bits are exits at the bottom; upper 4 bits are exits at the top.
 */
uint8 GetPathExits(int xpos, int ypos, int zpos)
{
	if (!IsVoxelstackInsideWorld(xpos, ypos)) return 0;
	const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
	if (v == nullptr) return 0;
	return GetPathExits(v);
}

/**
 * Walk over a queue path from the given entry edge at the given position.
 * If it leads to a new voxel edge, the provided position and edge is update with the exit point.
 * @param xp [inout] Start voxel X position before the queue path, updated to last voxel position.
 * @param yp [inout] Start voxel Y position before the queue path, updated to last voxel position.
 * @param zp [inout] Start voxel Z position before the queue path, updated to last voxel position.
 * @param entry Direction used for entry to the path, updated to last edge exit direction.
 * @return Whether a (possibly) new last voxel could be found, \c false means the path leads to nowhere.
 * @note Parameter values may get changed during the call, do not rely on their values except when \c true is returned.
 */
bool TravelQueuePath(int *xp, int *yp, int *zp, TileEdge *entry)
{
	int orig_xp = *xp;
	int orig_yp = *yp;
	int orig_zp = *zp;

	int xpos = *xp;
	int ypos = *yp;
	int zpos = *zp;
	TileEdge edge = *entry;

	/* Check that entry voxel actually exists. */
	if (!IsVoxelstackInsideWorld(xpos, ypos)) return false;

	for (;;) {
		xpos += _tile_dxy[edge].x;
		ypos += _tile_dxy[edge].y;
		if (!IsVoxelstackInsideWorld(xpos, ypos)) return false;

		const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
		if (vx == nullptr || !HasValidPath(vx)) {
			/* No path here, check the voxel below. */
			if (zpos == 0) return true; // Path ends here.
			zpos--;
			vx = _world.GetVoxel(xpos, ypos, zpos);
			if (vx == nullptr || !HasValidPath(vx)) return true; // Path ends here.
		}

		if (xpos == orig_xp && ypos == orig_yp && zpos == orig_zp) return false; // Cycle detected.

		/* Stop if we found a non-queue path. */
		if (_sprite_manager.GetPathStatus(GetPathType(vx->GetInstanceData())) != PAS_QUEUE_PATH) return true;

		/* At this point:
		 * *xp, *yp, *zp, edge (and *entry) contain the last valid voxel edge.
		 * xpos, ypos, zpos, vx is the next queue path tile position.
		 */

		uint8 exits = GetPathExits(vx);

		/* Check that the new tile can go back to our last tile. */
		uint8 rev_edge = (edge + 2) % 4;
		if (!((exits & (0x01 << rev_edge)) != 0 && zpos == *zp) &&
				!((exits & (0x10 << rev_edge)) != 0 && zpos == *zp -1)) {
			return false;
		}

		/* Find exit to the next path tile. */
		for (edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
			if (edge == rev_edge) continue; // Skip the direction we came from.
			if ((exits & (0x01 << edge)) != 0) break;
			if ((exits & (0x10 << edge)) != 0) {
				zpos++;
				break;
			}
		}

		if (edge == EDGE_COUNT) return false; // Queue path doesn't have a second exit.

		*xp = xpos;
		*yp = ypos;
		*zp = zpos;
		*entry = edge;
	}
}

/**
 * Set the edge of a path sprite. Also updates the corner pieces of the flat path tiles.
 * @param slope Current path slope (imploded).
 * @param edge Edge to set or unset.
 * @param connect If \c true, connect to the edge, else remove the connection.
 * @return (Possibly) updated path slope.
 */
uint8 SetPathEdge(uint8 slope, TileEdge edge, bool connect)
{
	const uint8 north_edges = (1 << PATHBIT_NE) | (1 << PATHBIT_NW);
	const uint8 south_edges = (1 << PATHBIT_SE) | (1 << PATHBIT_SW);
	const uint8 east_edges  = (1 << PATHBIT_NE) | (1 << PATHBIT_SE);
	const uint8 west_edges  = (1 << PATHBIT_NW) | (1 << PATHBIT_SW);

	if (slope >= PATH_FLAT_COUNT) return slope; // Ramps do not have edge to connect.

	slope = _path_expand[slope];
	uint8 bit_value;
	switch (edge) {
		case EDGE_NE: bit_value = 1 << PATHBIT_NE; break;
		case EDGE_SE: bit_value = 1 << PATHBIT_SE; break;
		case EDGE_SW: bit_value = 1 << PATHBIT_SW; break;
		case EDGE_NW: bit_value = 1 << PATHBIT_NW; break;
		default: NOT_REACHED();
	}
	if (connect) {
		slope |= bit_value;
	} else {
		slope &= ~bit_value;
	}

	slope &= ~((1 << PATHBIT_N) | (1 << PATHBIT_E) | (1 << PATHBIT_S) | (1 << PATHBIT_W));
	if ((slope & north_edges) == north_edges) slope |= 1 << PATHBIT_N;
	if ((slope & south_edges) == south_edges) slope |= 1 << PATHBIT_S;
	if ((slope & east_edges)  == east_edges)  slope |= 1 << PATHBIT_E;
	if ((slope & west_edges)  == west_edges)  slope |= 1 << PATHBIT_W;
	return _path_implode[slope];
}

/**
 * Get number of edge connections (0, 1, many) of an imploded slope.
 * @param impl_slope Slope to check (imploded number).
 * @return Exact count of edges for \c 0 and \c 1, else \c 2 which means 'more than 1'.
 */
static int GetQuePathEdgeConnectCount(uint8 impl_slope)
{
	uint8 exp_edges = _path_expand[impl_slope] & PATHMASK_EDGES;
	if (exp_edges == 0) return 0;
	if (exp_edges == PATHMASK_NE || exp_edges == PATHMASK_SE || exp_edges == PATHMASK_SW || exp_edges == PATHMASK_NW) return 1;
	return 2;
}

/**
 * Examine, and perhaps modify a neighbouring path edge or ride connection, to make it connect (or not if not \a add_edges)
 * to the centre examined tile.
 * @param xpos X coordinate of the neighbouring voxel.
 * @param ypos Y coordinate of the neighbouring voxel.
 * @param zpos Z coordinate of the neighbouring voxel.
 * @param use_additions Use #_additions rather than #_world.
 * @param edge Edge to examine, and/or connected to.
 * @param add_edges If set, add edges (else, remove them).
 * @param at_bottom Whether a path connection is expected at the bottom (if \c false, it should be at the top).
 * @param dest_voxel [out] %Voxel containing the neighbouring path, or \c nullptr.
 * @param dest_inst_data [out] New instance of the voxel. Only valid if \a dest_voxel is not \c nullptr.
 * @param dest_status [out] Status of the neighbouring path.
 * @return Neighbouring voxel was (logically) connected to the centre tile.
 */
static bool ExamineNeighbourPathEdge(uint16 xpos, uint16 ypos, uint8 zpos, bool use_additions, TileEdge edge, bool add_edges, bool at_bottom,
		Voxel **dest_voxel, uint16 *dest_inst_data, PathStatus *dest_status)
{
	Voxel *v;

	*dest_voxel = nullptr;
	*dest_status = PAS_UNUSED;
	*dest_inst_data = PATH_INVALID;

	if (use_additions) {
		v = _additions.GetCreateVoxel(xpos, ypos, zpos, false);
	} else {
		v = _world.GetCreateVoxel(xpos, ypos, zpos, false);
	}
	if (v == nullptr) return false;

	uint16 number = v->GetInstance();
	if (number == SRI_PATH) {
		uint16 instance_data = v->GetInstanceData();
		if (!HasValidPath(instance_data)) return false;

		uint8 slope = GetImplodedPathSlope(instance_data);
		if (at_bottom) {
			if (slope >= PATH_FLAT_COUNT && slope != _path_up_from_edge[edge]) return false;
		} else {
			if (slope != _path_down_from_edge[edge]) return false;
		}

		PathStatus status = _sprite_manager.GetPathStatus(GetPathType(instance_data));
		if (add_edges && status == PAS_QUEUE_PATH) { // Only try to connect to queue paths if they are not yet connected to 2 (or more) neighbours.
			if (GetQuePathEdgeConnectCount(slope) > 1) return false;
		}

		slope = SetPathEdge(slope, edge, add_edges);
		*dest_status = status;
		*dest_voxel = v;
		*dest_inst_data = SetImplodedPathSlope(instance_data, slope);
		return true;

	} else if (number >= SRI_FULL_RIDES) { // A ride instance. Does it have an entrance here?
		if ((v->GetInstanceData() & (1 << edge)) != 0) {
			*dest_status = PAS_QUEUE_PATH;
			return true;
		}
	}
	return false;
}


/**
 * Add edges of the neighbouring path tiles.
 * @param xpos X coordinate of the central voxel with a path tile.
 * @param ypos Y coordinate of the central voxel with a path tile.
 * @param zpos Z coordinate of the central voxel with a path tile.
 * @param slope Imploded path slope of the central voxel.
 * @param dirs Edge directions to change (bitset of #TileEdge), usually #EDGE_ALL.
 * @param use_additions Use #_additions rather than #_world.
 * @param status Status of the path. #PAS_UNUSED means to remove the edges.
 * @return Updated (imploded) slope at the central voxel.
 */
uint8 AddRemovePathEdges(uint16 xpos, uint16 ypos, uint8 zpos, uint8 slope, uint8 dirs, bool use_additions, PathStatus status)
{
	PathStatus ngb_status[4];    // Neighbour path status, #PAS_UNUSED means do not connect.
	Voxel *ngb_voxel[4];         // Neighbour voxels with path, may be null if it doesn't need changing.
	uint16 ngb_instance_data[4]; // New instance data, if the voxel exists.
	uint8 ngb_zpos[4];           // Z coordinate of the neighbouring voxel.

	std::fill_n(ngb_status, lengthof(ngb_status), PAS_UNUSED); // Clear path all statuses to prevent connecting to it if an edge is skipped.
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		if ((dirs & (1 << edge)) == 0) continue; // Skip directions that should not be updated.
		int delta_z = 0;
		if (slope >= PATH_FLAT_COUNT) {
			if (_path_down_from_edge[edge] == slope) {
				delta_z = 1;
			} else if (_path_up_from_edge[edge] != slope) {
				continue;
			}
		}
		Point16 dxy = _tile_dxy[edge];
		if ((dxy.x < 0 && xpos == 0) || (dxy.x > 0 && xpos == _world.GetXSize() - 1)) continue;
		if ((dxy.y < 0 && ypos == 0) || (dxy.y > 0 && ypos == _world.GetYSize() - 1)) continue;

		TileEdge edge2 = (TileEdge)((edge + 2) % 4);
		bool modified = false;
		if (delta_z <= 0 || zpos < WORLD_Z_SIZE - 1) {
			ngb_zpos[edge] = zpos + delta_z;
			modified = ExamineNeighbourPathEdge(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, use_additions, edge2, status != PAS_UNUSED, true,
					&ngb_voxel[edge], &ngb_instance_data[edge], &ngb_status[edge]);
		}
		delta_z--;
		if (!modified && (delta_z >= 0 || zpos > 0)) {
			ngb_zpos[edge] = zpos + delta_z;
			ExamineNeighbourPathEdge(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, use_additions, edge2, status != PAS_UNUSED, false,
					&ngb_voxel[edge], &ngb_instance_data[edge], &ngb_status[edge]);
		}
	}

	switch (status) {
		case PAS_UNUSED: // All edges get removed.
		case PAS_NORMAL_PATH: // All edges get added.
			for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
				if (ngb_status[edge] != PAS_UNUSED) {
					if (slope < PATH_FLAT_COUNT) slope = SetPathEdge(slope, edge, status != PAS_UNUSED);
					if (ngb_voxel[edge] != nullptr) {
						ngb_voxel[edge]->SetInstanceData(ngb_instance_data[edge]);
						MarkVoxelDirty(xpos + _tile_dxy[edge].x, ypos + _tile_dxy[edge].y, ngb_zpos[edge]);
					}
				}
			}
			return slope;

		case PAS_QUEUE_PATH:
			/* Pass 1: Try to add other queue paths. */
			for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
				if (ngb_status[edge] == PAS_QUEUE_PATH) {
					if (GetQuePathEdgeConnectCount(slope) > 1) break;

					if (slope < PATH_FLAT_COUNT) slope = SetPathEdge(slope, edge, true);
					if (ngb_voxel[edge] != nullptr) {
						ngb_voxel[edge]->SetInstanceData(ngb_instance_data[edge]);
						MarkVoxelDirty(xpos + _tile_dxy[edge].x, ypos + _tile_dxy[edge].y, ngb_zpos[edge]);
					}
				}
			}
			/* Pass 2: Add normal paths. */
			for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
				if (ngb_status[edge] == PAS_NORMAL_PATH) {
					if (GetQuePathEdgeConnectCount(slope) > 1) break;

					if (slope < PATH_FLAT_COUNT) slope = SetPathEdge(slope, edge, true);
					if (ngb_voxel[edge] != nullptr) {
						ngb_voxel[edge]->SetInstanceData(ngb_instance_data[edge]);
						MarkVoxelDirty(xpos + _tile_dxy[edge].x, ypos + _tile_dxy[edge].y, ngb_zpos[edge]);
					}
				}
			}
			return slope;

		default: NOT_REACHED();
	}
}
