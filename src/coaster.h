/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster.h Coaster data. */

#ifndef COASTER_H
#define COASTER_H

#include <map>
#include "reference_count.h"
#include "ride_type.h"
#include "bitmath.h"

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
	TPB_NONE = 0,  ///< Track piece does not bank.
	TPB_LEFT = 1,  ///< Track piece banks to the left.
	TPB_RIGHT = 2, ///< Track piece banks to the right.

	TPB_END, ///< End of the banking values.
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
		assert(banking < TPB_END);
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
	 * Get the bend of the track piece (-3=wide left, -1=tight left, 0=straight, 1=tight right, 3=wide right).
	 * @return the bend of the track piece.
	 */
	inline int GetBend() const
	{
		int bend = GB(this->track_flags, 11, 3);
		if ((bend & 4) != 0) bend |= ~7;
		return bend;
	}

protected:
	~TrackPiece();
};

typedef DataReference<TrackPiece> TrackPieceRef; ///< Track piece reference class.

typedef std::map<uint32, TrackPieceRef> TrackPiecesMap; ///< Map of loaded track pieces.

/** Kinds of coasters. */
enum CoasterKind {
	CST_SIMPLE = 1, ///< 'Simple' coaster type.

	CST_COUNT,  ///< Number of coaster types.
};

/** Platform types of coasters. */
enum CoasterPlatformType {
	CPT_WOOD = 1,  ///< Wooden platform type.

	CPT_COUNT, ///< Number of platform types with coasters.
};

/** Coaster ride type. */
class CoasterType : public RideType {
public:
	CoasterType();
	/* virtual */ ~CoasterType();
	/* virtual */ RideInstance *CreateInstance() const;
	/* virtual */ const ImageData *GetView(uint8 orientation) const;
	/* virtual */ const StringID *GetInstanceNames() const;

	bool Load(RcdFile *rcd_file, uint32 length, const TextMap &texts, const TrackPiecesMap &piece_map);

	uint16 coaster_kind;       ///< Kind of coaster. @see CoasterKind
	uint8 platform_type;       ///< Type of platform. @see CoasterPlatformType
	int piece_count;           ///< Number of track pieces in #pieces.
	TrackPieceRef *pieces;     ///< Track pieces of the coaster.
	int voxel_count;           ///< Number of track voxels in #voxels.
	const TrackVoxel **voxels; ///< All voxels of all track pieces (do not free the track voxels, #pieces owns this data).
};

/**
 * A roller coaster in the world.
 * Since roller coaster rides need to be constructed by the user first, an instance can exist
 * without being able to open for public.
 */
class CoasterInstance : public RideInstance {
public:
	CoasterInstance(const CoasterType *rt);
	~CoasterInstance();

	bool IsAccessible();

	/**
	 * Get the coaster type of this ride.
	 * @return The coaster type of this ride.
	 */
	const CoasterType *GetCoasterType() const
	{
		return static_cast<const CoasterType *>(this->type);
	}

	/* virtual */ void GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const;

	RideInstanceState DecideRideState();
};

#endif
