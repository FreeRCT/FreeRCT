/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file track_piece.h Declarations for track pieces. */

#ifndef TRACK_PIECE_H
#define TRACK_PIECE_H

#include <map>
#include "reference_count.h"
#include "bitmath.h"
#include "money.h"

class ImageData;
typedef class std::map<unsigned int, ImageData*> ImageMap;

/** Data of a voxel in a track piece. */
struct TrackVoxel {
	TrackVoxel();
	~TrackVoxel();

	bool Load(RcdFile *rcd_file, size_t length, const ImageMap &sprites);

	ImageData *back[4];  ///< Reference to the background tracks (N/E/S/W view).
	ImageData *front[4]; ///< Reference to the front tracks (N/E/S/W view).
	int8 dx;     ///< Relative X position of the voxel.
	int8 dy;     ///< Relative Y position of the voxel.
	int8 dz;     ///< Relative Z position of the voxel.
	uint8 space; ///< Space requirements of the voxel.
};

/** Banking of the track piece. */
enum TrackPieceBanking {
	TPB_NONE,  ///< Track piece does not bank.
	TPB_LEFT,  ///< Track piece banks to the left.
	TPB_RIGHT, ///< Track piece banks to the right.

	TPB_COUNT, ///< End of the banking values.

	TPB_INVALID = 0xff, ///< Invalid banking value.
};

/** Available bends in the tracks. */
enum TrackBend {
	TBN_LEFT_WIDE,    ///< Wide bend to the left.
	TBN_LEFT_NORMAL,  ///< Normal bend to the left.
	TBN_LEFT_TIGHT,   ///< Tight bend to the left.
	TBN_STRAIGHT,     ///< No bend either way.
	TBN_RIGHT_TIGHT,  ///< Tight bend to the right.
	TBN_RIGHT_NORMAL, ///< Normal bend to the right.
	TBN_RIGHT_WIDE,   ///< Wide bend to the right.

	TBN_COUNT,        ///< Number of bend types.

	TBN_INVALID = 0xff, ///< Invalid bend value.
};

/** One track piece (type) of a roller coaster track. */
class TrackPiece : public RefCounter {
public:
	TrackPiece();

	bool Load(RcdFile *rcd_file, uint32 length, const ImageMap &sprites);

	uint8 entry_connect;      ///< Entry connection code
	uint8 exit_connect;       ///< Exit connection code
	int8 exit_dx;             ///< Relative X position of the exit voxel.
	int8 exit_dy;             ///< Relative Y position of the exit voxel.
	int8 exit_dz;             ///< Relative Z position of the exit voxel.
	int8 speed;               ///< If non-zero, the minimal speed of cars at the track.
	uint16 track_flags;       ///< Flags of the track piece.
	Money cost;               ///< Cost of this track piece.
	int voxel_count;          ///< Number of voxels in #track_voxels.
	TrackVoxel *track_voxels; ///< Track voxels of this piece.

	/**
	 * Does the track piece have a platform?
	 * @return Whether the track piece has a platform.
	 */
	inline bool HasPlatform() const
	{
		return GB(this->track_flags, 0, 1) != 0;
	}

	/**
	 * Get the 'direction' of the platform.
	 * @return Direction of the tracks (and thus the platform).
	 * @pre #HasPlatform() holds.
	 */
	inline TileEdge GetPlatformDirection() const
	{
		return (TileEdge)GB(this->track_flags, 1, 2);
	}

	/**
	 * Does the track piece have a platform?
	 * @return Whether the track piece has a platform.
	 */
	inline bool IsStartingPiece() const
	{
		return GB(this->track_flags, 3, 1) != 0;
	}

	/**
	 * Get the direction of the initial track piece. Should be used to match with the build arrow direction.
	 * @return Direction of the initial track piece.
	 * @pre #IsStartingPiece() holds.
	 */
	inline TileEdge GetStartDirection() const
	{
		return (TileEdge)GB(this->track_flags, 4, 2);
	}

	/**
	 * Get banking of the track piece.
	 * @return Banking of the track piece.
	 */
	inline TrackPieceBanking GetBanking() const
	{
		uint8 banking = GB(this->track_flags, 6, 2);
		assert(banking < TPB_COUNT);
		return (TrackPieceBanking)banking;
	}

	/**
	 * Get the slope of the track piece.
	 * @return the bend of the track piece.
	 */
	inline TrackSlope GetSlope() const
	{
		int slope = GB(this->track_flags, 8, 3);
		if ((slope & 4) != 0) slope |= ~7;
		switch (slope + 3) {
			case -3 + 3: return TSL_STRAIGHT_DOWN;
			case -2 + 3: return TSL_STEEP_DOWN;
			case -1 + 3: return TSL_DOWN;
			case 0 + 3:  return TSL_FLAT;
			case 1 + 3:  return TSL_UP;
			case 2 + 3:  return TSL_STEEP_UP;
			case 3 + 3:  return TSL_STRAIGHT_UP;
			default: return TSL_FLAT;
		}
	}

	/**
	 * Get the bend of the track piece.
	 * @return the bend of the track piece.
	 */
	inline TrackBend GetBend() const
	{
		int bend = GB(this->track_flags, 11, 3);
		if ((bend & 4) != 0) bend |= ~7;
		return (TrackBend)(bend + 3);
	}

protected:
	~TrackPiece();
};

typedef DataReference<TrackPiece> TrackPieceRef; ///< Track piece reference class.


/**
 * Track piece with a position. Used in roller coasters to define their path in the world.
 * @note The #piece value is owned by the coaster type, do not free it.
 */
class PositionedTrackPiece {
public:
	PositionedTrackPiece();
	PositionedTrackPiece(uint16 xpos, uint16 ypos, uint8 zpos, const TrackPiece *piece);

	bool IsOnWorld() const;
	bool CanBePlaced() const;
	bool CanBeSuccessor(uint16 x, uint16 y, uint8 z, uint8 connect) const;
	bool CanBeSuccessor(const PositionedTrackPiece &pred) const;

	/**
	 * Get the X position of the exit voxel.
	 * @return The X position of the exit voxel.
	 */
	inline uint16 GetEndX() const
	{
		return this->x_base + this->piece->exit_dx;
	}

	/**
	 * Get the Y position of the exit voxel.
	 * @return The Y position of the exit voxel.
	 */
	inline uint16 GetEndY() const
	{
		return this->y_base + this->piece->exit_dy;
	}

	/**
	 * Get the Z position of the exit voxel.
	 * @return The Z position of the exit voxel.
	 */
	inline uint8 GetEndZ() const
	{
		return this->z_base + this->piece->exit_dz;
	}

	uint16 x_base; ///< X position of the entry point of the track piece.
	uint16 y_base; ///< Y position of the entry point of the track piece.
	uint8 z_base;  ///< Z position of the entry point of the track piece.

	const TrackPiece *piece; ///< Track piece placed at the given position, may be \c NULL.
};

#endif
