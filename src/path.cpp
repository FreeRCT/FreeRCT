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
