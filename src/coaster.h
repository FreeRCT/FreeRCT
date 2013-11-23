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
#include <vector>
#include "ride_type.h"
#include "track_piece.h"

static const int MAX_PLACED_TRACK_PIECES = 1024; ///< Maximum number of track pieces in a single roller coaster.

typedef std::map<uint32, ConstTrackPiecePtr> TrackPiecesMap; ///< Map of loaded track pieces.

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
	~CoasterType();
	RideInstance *CreateInstance() const override;
	const ImageData *GetView(uint8 orientation) const override;
	const StringID *GetInstanceNames() const override;

	bool Load(RcdFile *rcd_file, uint32 length, const TextMap &texts, const TrackPiecesMap &piece_map);

	int GetTrackVoxelIndex(const TrackVoxel *tvx) const;

	uint16 coaster_kind;       ///< Kind of coaster. @see CoasterKind
	uint8 platform_type;       ///< Type of platform. @see CoasterPlatformType
	int voxel_count;           ///< Number of track voxels in #voxels.
	const TrackVoxel **voxels; ///< All voxels of all track pieces (do not free the track voxels, #pieces owns this data).

	std::vector<ConstTrackPiecePtr> pieces; ///< Track pieces of the coaster.
};

/**
 * Platforms of the coaster.
 * @todo Add support for different sizes sprites.
 */
class CoasterPlatform {
public:
	CoasterPlatform();

	int tile_width;           ///< Width of the tile of these sprites.
	CoasterPlatformType type; ///< Type of platform.
	ImageData *ne_sw_back;    ///< Background sprite for the NE-SW direction.
	ImageData *ne_sw_front;   ///< Foreground sprite for the NE-SW direction.
	ImageData *se_nw_back;    ///< Background sprite for the SE-NW direction.
	ImageData *se_nw_front;   ///< Foreground sprite for the SE-NW direction.
	ImageData *sw_ne_back;    ///< Background sprite for the SW_NE direction.
	ImageData *sw_ne_front;   ///< Foreground sprite for the SW_NE direction.
	ImageData *nw_se_back;    ///< Background sprite for the NW_SE direction.
	ImageData *nw_se_front;   ///< Foreground sprite for the NW_SE direction.
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

	void GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const override;
	uint8 GetEntranceDirections(uint16 xvox, uint16 yvox, uint8 zvox) const override;

	RideInstanceState DecideRideState();

	bool MakePositionedPiecesLooping(bool *modified);
	int GetFirstPlacedTrackPiece() const;
	int AddPositionedPiece(const PositionedTrackPiece &placed);
	int FindSuccessorPiece(uint16 x, uint16 y, uint8 z, uint8 entry_connect, int start = 0, int end = MAX_PLACED_TRACK_PIECES);
	int FindSuccessorPiece(const PositionedTrackPiece &placed);

	void PlaceTrackPieceInAdditions(const PositionedTrackPiece &piece);

	PositionedTrackPiece *pieces; ///< Positioned track pieces.
	int capacity;                 ///< Number of entries in the #pieces.
};

bool LoadCoasterPlatform(RcdFile *rcdfile, uint32 length, const ImageMap &sprites);

#endif
