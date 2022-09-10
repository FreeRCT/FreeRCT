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
#include "person.h"
#include "track_piece.h"

static const int MAX_PLACED_TRACK_PIECES = 1024; ///< Maximum number of track pieces in a single roller coaster.

typedef std::map<uint32, ConstTrackPiecePtr> TrackPiecesMap; ///< Map of loaded track pieces.

/** Kinds of coasters. */
enum CoasterKind {
	CST_SIMPLE = 1, ///< 'Simple' coaster type.

	CST_COUNT,      ///< Number of coaster types.
};

/** Platform types of coasters. */
enum CoasterPlatformType {
	CPT_WOOD = 1,  ///< Wooden platform type.

	CPT_COUNT,     ///< Number of platform types with coasters.
};

/** A type of car riding at a track. */
class CarType {
public:
	CarType();

	void Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	/**
	 * Retrieve an image of the car at a given \a pitch, \a roll, and \a yaw orientation.
	 * @param pitch Required pitch of the car.
	 * @param roll Required roll of the car.
	 * @param yaw Required yaw of the car.
	 * @return Image of the car in the requested orientation, if available.
	 */
	const ImageData *GetCar(uint pitch, uint roll, uint yaw) const
	{
		return this->cars[(pitch & 0xf) | ((roll & 0xf) << 4) | ((yaw & 0xf) << 8)];
	}

	/**
	 * Retrieve a guest overlay sprite at a given \a pitch, \a roll, and \a yaw orientation.
	 * @param pitch Required pitch of the car.
	 * @param roll Required roll of the car.
	 * @param yaw Required yaw of the car.
	 * @param slot Seat number of the guest to overlay.
	 * @return The overlay in the requested orientation, if available.
	 */
	const ImageData *GetGuestOverlay(uint pitch, uint roll, uint yaw, uint slot) const
	{
		return this->guest_overlays[slot * 16u*16u*16u + ((pitch & 0xf) | ((roll & 0xf) << 4) | ((yaw & 0xf) << 8))];
	}

	ImageData *cars[4096];                        ///< Images of the car, in all possible orientations.
	std::unique_ptr<ImageData*[]> guest_overlays; ///< Images for the guest overlays, in all possible orientations.
	uint16 tile_width;       ///< Tile width of the images.
	uint16 z_height;         ///< Change in z-height of the images.
	uint32 car_length;       ///< Length of a car in 1/256 pixel.
	uint32 inter_car_length; ///< Length between two car in 1/256 pixel.
	uint16 num_passengers;   ///< Number of passenger of a car.
	uint16 num_entrances;    ///< Number of entrance rows of a car.
	Recolouring recolours;   ///< Sprite recolour map.
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

	void Load(RcdFileReader *rcd_file, const TextMap &texts, const TrackPiecesMap &piece_map);

	int GetTrackVoxelIndex(const TrackVoxel *tvx) const;

	uint16 coaster_kind;       ///< Kind of coaster. @see CoasterKind
	uint8 platform_type;       ///< Type of platform. @see CoasterPlatformType
	uint8 max_number_trains;   ///< Maximum number of trains at a roller coaster.
	uint8 max_number_cars;     ///< Maximum number of cars in a train.
	std::vector<const TrackVoxel *> voxels; ///< All voxels of all track pieces (do not free the track voxels, #pieces owns this data).

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

extern CoasterPlatform _coaster_platforms[];

class CoasterCar;
class CoasterTrain;

/**
 * Displayed car in a train.
 * Note that #yaw decides validness of the data.
 */
class DisplayCoasterCar : public VoxelObject {
public:
	DisplayCoasterCar();

	const ImageData *GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const override;
	VoxelObject::Overlays GetOverlays(const SpriteStorage *sprites, ViewOrientation orient) const override;

	void Set(const XYZPoint16 &vox_pos, const XYZPoint16 &pix_pos, uint8 pitch, uint8 roll, uint8 yaw);
	void PreRemove();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	const CarType *car_type; ///< Car type data.
	uint8 pitch;             ///< Pitch of the car.
	uint8 roll;              ///< Roll of the car.
	uint8 yaw;               ///< Yaw of the car (\c 0xff means all data is invalid).

	const CoasterCar *owning_car;  ///< Pointer to the owning car.
};

/** Coaster car drawn at the front and the back position. */
class CoasterCar {
public:
	DisplayCoasterCar front;     ///< %Voxel image displayed at the front of the car.
	DisplayCoasterCar back;      ///< %Voxel image displayed at the end of the car.
	std::vector<Guest*> guests;  ///< Guests currently in the car.
	CoasterTrain *owning_train;  ///< Pointer to the owning train.

	void PreRemove();

	void Load(Loader &ldr);
	void Save(Saver &svr);
};

/** How a coaster train is behaving in regard to a station. */
enum TrainStationPolicy {
	TSP_LEAVING_STATION,   ///< The train is leaving a station.
	TSP_ENTERING_STATION,  ///< The train is entering a station.
	TSP_IN_STATION_FRONT,  ///< The train is waiting motionlessly at the front of a station.
	TSP_IN_STATION_BACK,   ///< The train is waiting motionlessly in a station behind another train.
	TSP_NO_STATION,        ///< The train is nowhere near a station.
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

	void Load(Loader &ldr);
	void Save(Saver &svr);

	CoasterInstance *coaster;              ///< Roller coaster owning the train.
	std::vector<CoasterCar> cars;          ///< Cars in the train. \c 0 means the train is not used.
	uint32 back_position;                  ///< Position of the back-end of the train (in 1/256 pixels).
	int32 speed;                           ///< Amount of forward motion / millisecond, in 1/256 pixels.
	const PositionedTrackPiece *cur_piece; ///< Track piece that has the back-end position of the train.
	TrainStationPolicy station_policy;     ///< The train's behaviour regarding stations.
	int32 time_left_waiting;               ///< The number of milliseconds left this train should wait in a station before departing.
};

/** A station belonging to a coaster. */
struct CoasterStation {
	CoasterStation();

	std::vector<XYZPoint16> locations;  ///< Voxels occupied by the station.
	TileEdge direction;                 ///< The start direction of the station's first tile.
	uint32 length;                      ///< The station's length in pixels.
	uint32 back_position;               ///< Position of the begin of the station in 1/256 pixels.
	XYZPoint16 entrance;                ///< Position of the station's entrance (may be \c invalid()).
	XYZPoint16 exit;                    ///< Position of the station's exit (may be \c invalid()).
};

/** Holds data about the intensity of a coaster at a specific point. */
struct CoasterIntensityStatistics {
	bool valid;          ///< Whether this data point should be considered for the calculation.
	int32 precision;     ///< Over how many individual test results this entry is sampled.
	int32 speed;         ///< Average speed of trains passing through here.
	int32 vertical_g;    ///< Average vertical G force of trains passing through here.
	int32 horizontal_g;  ///< Average horizontal G force of trains passing through here.
};
static const int COASTER_INTENSITY_STATISTICS_SAMPLING_PRECISION = 0x8000;  ///< Minimum distance of two points in a coaster's intensity statistics map.

/**
 * A roller coaster in the world.
 * Since roller coaster rides need to be constructed by the user first, an instance can exist
 * without being able to open for public.
 */
class CoasterInstance : public RideInstance {
public:
	CoasterInstance(const CoasterType *rt, const CarType *car_type);

	bool IsAccessible();

	void OnAnimate(int delay) override;
	void CloseRide() override;

	/**
	 * Get the coaster type of this ride.
	 * @return The coaster type of this ride.
	 */
	const CoasterType *GetCoasterType() const
	{
		return static_cast<const CoasterType *>(this->type);
	}

	void TestRide();
	void OpenRide() override;
	void RecalculateRatings() override;

	void GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const override;
	uint8 GetEntranceDirections(const XYZPoint16 &vox) const override;
	RideEntryResult EnterRide(int guest, const XYZPoint16 &vox, TileEdge entry) override;
	XYZPoint32 GetExit(int guest, TileEdge entry_edge) override;
	void RemoveAllPeople() override;
	bool CanOpenRide() const override;
	bool CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const override;
	bool PathEdgeWanted(const XYZPoint16 &vox, TileEdge edge) const override;
	const Recolouring *GetRecolours(const XYZPoint16 &pos) const override;
	void InitializeItemPricesAndStatistics() override;
	void OnNewMonth() override;
	XYZPoint16 RepresentativeLocation() const override;
	Money ComputeReturnCost() const override;

	bool MakePositionedPiecesLooping(bool *modified);
	int GetFirstPlacedTrackPiece() const;
	int AddPositionedPiece(const PositionedTrackPiece &placed);
	void RemovePositionedPiece(PositionedTrackPiece &piece);

	int FindSuccessorPiece(const XYZPoint16 &vox, uint8 entry_connect, int start = 0, int end = MAX_PLACED_TRACK_PIECES);
	int FindSuccessorPiece(const PositionedTrackPiece &placed);
	int FindPredecessorPiece(const PositionedTrackPiece &placed);
	void UpdateStations();
	void InitializeStation(CoasterStation&) const;
	bool IsInStation(uint32 pos, const CoasterStation&) const;
	uint32 PositionRelativeTo(uint32 pos, uint32 offset) const;
	bool CanPlaceEntranceOrExit(const XYZPoint16 &pos, bool entrance, const CoasterStation *station) const;
	bool PlaceEntranceOrExit(const XYZPoint16 &pos, bool entrance, CoasterStation *station);
	bool NeedsEntrance() const;
	bool NeedsExit() const;
	bool IsEntranceLocation(const XYZPoint16& pos) const override;
	bool IsExitLocation(const XYZPoint16& pos) const override;
	int EntranceExitRotation(const XYZPoint16& vox, const CoasterStation *station) const;
	EdgeCoordinate GetMechanicEntrance() const override;

	SmallRideInstance GetRideNumber() const;
	uint16 GetInstanceData(const TrackVoxel *tv) const;
	void PlaceTrackPieceInWorld(const PositionedTrackPiece &piece);
	void RemoveTrackPieceInWorld(const PositionedTrackPiece &piece);

	uint32 GetShortestStation() const;
	uint32 GetTrainLength(int cars_per_train) const;
	uint32 GetTrainSpacing() const;
	int GetMaxNumberOfTrains(int cars_per_train) const;
	int GetMaxNumberOfCars() const;
	void SetNumberOfTrains(int number_trains);
	void SetNumberOfCars(int number_cars);
	void ReinitializeTrains(bool test_mode);
	void Crash(CoasterTrain *t1, CoasterTrain *t2);
	void SampleStatistics(uint32 point, bool valid, int32 speed, int32 vg, int32 hg);

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	void RemoveStationsFromWorld();
	void InsertStationsIntoWorld();
	void RemoveFromWorld() override;
	void InsertIntoWorld() override {
		/* This was already done during construction – nothing left to do here. */
	}

	std::unique_ptr<PositionedTrackPiece[]> pieces; ///< Positioned track pieces.
	int capacity;                 ///< Number of entries in the #pieces.
	uint32 coaster_length;        ///< Total length of the roller coaster track (in 1/256 pixels).
	int number_of_trains;         ///< Current number of trains.
	int cars_per_train;           ///< Current number of cars in each train.
	CoasterTrain trains[4];       ///< Trains at the roller coaster (with an arbitrary max size). A train without cars means the train is not used.
	const CarType *car_type;      ///< Type of cars running at the coaster.
	std::vector<CoasterStation> stations;  ///< All stations of this coaster.
	XYZPoint16 temp_entrance_pos;          ///< Temporary location of one of the ride's entrance while the user is moving the entrance.
	XYZPoint16 temp_exit_pos;              ///< Temporary location of one of the ride's exit while the user is moving the exit.
	int max_idle_duration;                 ///< Maximum duration how long a train may wait in a station in milliseconds.
	int min_idle_duration;                 ///< Minimum duration how long a train may wait in a station in milliseconds.
	std::map<uint32, CoasterIntensityStatistics> intensity_statistics;  ///< Intensity along the track.
};

void LoadCoasterPlatform(RcdFileReader *rcd_file, const ImageMap &sprites);

#endif
