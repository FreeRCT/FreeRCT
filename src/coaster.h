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
#include "map.h"
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

/** A type of car riding at a track. */
class CarType {
public:
	CarType();

	bool Load(RcdFile *rcdfile, uint32 length, const ImageMap &sprites);

	/**
	 * Retrieve an image of the car at a given \a pitch, \a roll, and \a yaw orientation.
	 * @param pitch Required pitch of the car.
	 * @param roll Required roll of the car.
	 * @param yaw Required yaw of the car.
	 * @return Image of the car in the requested orientation, if available.
	 */
	const ImageData *GetCar(int pitch, int roll, int yaw) const
	{
		return this->cars[(uint)(pitch & 0xf) | ((uint)(roll & 0xf) << 4) | ((uint)(yaw & 0xf) << 8)];
	}

	ImageData *cars[4096]; ///< Images of the car, in all possible orientation (if available).
	uint16 tile_width;     ///< Tile width of the images.
	uint16 z_height;       ///< Change in z-height of the images.
	uint32 car_length;     ///< Length of a car in 1/256 pixel.
	uint16 num_passengers; ///< Number of passenger of a car.
	uint16 num_entrances;  ///< Number of entrance rows of a car.
};

CarType *GetNewCarType();

/** Coaster ride type. */
class CoasterType : public RideType {
public:
	CoasterType();
	~CoasterType();
	bool CanMakeInstance() const override;
	RideInstance *CreateInstance() const override;
	const ImageData *GetView(uint8 orientation) const override;
	const StringID *GetInstanceNames() const override;

	bool Load(RcdFile *rcd_file, uint32 length, const TextMap &texts, const TrackPiecesMap &piece_map);

	int GetTrackVoxelIndex(const TrackVoxel *tvx) const;

	uint16 coaster_kind;       ///< Kind of coaster. @see CoasterKind
	uint8 platform_type;       ///< Type of platform. @see CoasterPlatformType
	uint8 max_number_trains;   ///< Maximum number of trains at a roller coaster.
	uint8 max_number_cars;     ///< Maximum number of cars in a train.
	int voxel_count;           ///< Number of track voxels in #voxels.
	const TrackVoxel **voxels; ///< All voxels of all track pieces (do not free the track voxels, #pieces owns this data).

	std::vector<ConstTrackPiecePtr> pieces; ///< Track pieces of the coaster.

private:
	const CarType *GetDefaultCarType() const;
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
 * Position and orientation of a car in a train.
 * Note that #yaw decides validness of the data.
 * @todo Add this to the persons.
 */
class CoasterCar : public VoxelObject {
public:
	CoasterCar();

	virtual const ImageData *GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const override;

	void Set(int16 xvoxel, int16 yvoxel, int8  zvoxel, int16 xpos, int16 ypos, int16 zpos, uint8 pitch, uint8 roll, uint8 yaw);
	void PreRemove();

	const CarType *car_type; ///< Car type data.
	uint8 pitch; ///< Pitch of the car.
	uint8 roll;  ///< Roll of the car.
	uint8 yaw;   ///< Yaw of the car (\c 0xff means all data is invalid).
};

class CoasterInstance;

/**
 * A 'train' of coaster cars at a roller coaster.
 * It consists of one or more cars, each of length \c car_type->length. Trains drive in increasing distance,
 * until they hit the upper bound of the coaster length (CoasterInstance::coaster_length).
 */
class CoasterTrain {
public:
	CoasterTrain();

	void SetLength(int length);

	void OnAnimate(int delay);

	const CoasterInstance *coaster; ///< Roller coaster owning the train.
	std::vector<CoasterCar> cars; ///< Cars in the train. \c 0 means the train is not used.
	uint32 back_position; ///< Position of the back-end of the train (in 1/256 pixels).
	int32 speed;          ///< Amount of forward motion / millisecond, in 1/256 pixels.
};

/**
 * A roller coaster in the world.
 * Since roller coaster rides need to be constructed by the user first, an instance can exist
 * without being able to open for public.
 */
class CoasterInstance : public RideInstance {
public:
	CoasterInstance(const CoasterType *rt, const CarType *car_type);
	~CoasterInstance();

	bool IsAccessible();

	void OnAnimate(int delay) override;

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

	int GetMaxNumberOfTrains() const;
	void SetNumberOfTrains(int number_trains);
	int GetNumberOfTrains() const;

	int GetMaxNumberOfCars() const;
	void SetNumberOfCars(int number_cars);
	int GetNumberOfCars() const;

	PositionedTrackPiece *pieces; ///< Positioned track pieces.
	int capacity;                 ///< Number of entries in the #pieces.
	uint32 coaster_length;        ///< Total length of the roller coaster track (in 1/256 pixels).
	CoasterTrain trains[4];       ///< Trains at the roller coaster (with an arbitrary max size). A train without cars means the train is not used.
	const CarType *car_type;      ///< Type of cars running at the coaster.
};

bool LoadCoasterPlatform(RcdFile *rcdfile, uint32 length, const ImageMap &sprites);

#endif
