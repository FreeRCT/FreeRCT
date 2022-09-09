/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster.cpp Coaster type data. */

#include <cmath>
#include "stdafx.h"
#include "sprite_store.h"
#include "coaster.h"
#include "fileio.h"
#include "memory.h"
#include "map.h"
#include "messages.h"
#include "people.h"
#include "sprite_data.h"
#include "viewport.h"

#include "generated/coasters_strings.cpp"

CoasterPlatform _coaster_platforms[CPT_COUNT]; ///< Sprites for the coaster platforms.

static CarType _car_types[16]; ///< Car types available to the program (arbitrary limit).
static uint _used_types = 0;   ///< First free car type in #_car_types.

static const int32 TRAIN_DEPARTURE_INTERVAL_TESTING = 3000; ///< How many milliseconds a train should wait in the station in test mode.

static const uint16 ENTRANCE_OR_EXIT = INT16_MAX;  ///< Indicates that a voxel belongs to an entrance or exit.

/**
 * Get a new car type.
 * @return A free car type, or \c nullptr if no free car type available.
 */
CarType *GetNewCarType()
{
	if (_used_types == lengthof(_car_types)) return nullptr;
	uint index = _used_types;
	_used_types++;
	return &_car_types[index];
}

CarType::CarType()
{
	std::fill_n(this->cars, lengthof(this->cars), nullptr);
}

/**
 * Load the data of a CARS block from file.
 * @param rcd_file Data file being loaded.
 * @param sprites Already loaded sprites.
 */
void CarType::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(3);
	int length = rcd_file->size;
	length -= 2 + 2 + 4 + 4 + 2 + 2 + 16384 + 4 * 3;
	rcd_file->CheckMinLength(length, 0, "header");

	this->tile_width = rcd_file->GetUInt16();
	if (this->tile_width != 64) rcd_file->Error("Unsupported tile width %d", this->tile_width);  // Do not allow anything else than 64 pixels tile width.

	this->z_height = rcd_file->GetUInt16();
	if (this->z_height != this->tile_width / 4) rcd_file->Error("Wrong Z height");

	this->car_length = rcd_file->GetUInt32();
	if (this->car_length > 65535) rcd_file->Error("Car too long");  // Assumption is that a car fits in a single tile, at least some of the time.

	this->inter_car_length = rcd_file->GetUInt32();
	this->num_passengers = rcd_file->GetUInt16();
	this->num_entrances = rcd_file->GetUInt16();
	if (this->num_entrances == 0 || this->num_entrances > 4) {
		rcd_file->Error("Invalid number of entrances");  // Seems like a nice arbitrary upper limit on the number of rows of a car.
	}
	uint16 pass_per_row = this->num_passengers / this->num_entrances;
	if (this->num_passengers != pass_per_row * this->num_entrances) rcd_file->Error("Passenger counts don't match up");

	for (uint i = 0; i < 4096; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->cars[i]);
	}

	if (this->cars[0] == nullptr) rcd_file->Error("No car type");
	const int64 nr_overlays = 4096 * this->num_passengers;
	rcd_file->CheckExactLength(length, 4 * nr_overlays, "guest overlays");
	this->guest_overlays.reset(new ImageData*[nr_overlays]);
	for (int64 i = 0; i < nr_overlays; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->guest_overlays[i]);
	}

	for (int i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		this->recolours.Set(i, RecolourEntry(recolour));
	}
}

CoasterType::CoasterType() : RideType(RTK_COASTER)
{
}

CoasterType::~CoasterType()
{
}

/**
 * Load a coaster type.
 * @param rcd_file Data file being loaded.
 * @param texts Already loaded text blocks.
 * @param piece_map Already loaded track pieces.
 */
void CoasterType::Load(RcdFileReader *rcd_file, const TextMap &texts, const TrackPiecesMap &piece_map)
{
	rcd_file->CheckVersion(7);
	int length = rcd_file->size;
	length -= 2 + 1 + 1 + 1 + 4 + 2 + 6;
	rcd_file->CheckMinLength(length, 0, "header");

	this->coaster_kind = rcd_file->GetUInt16();
	this->platform_type = rcd_file->GetUInt8();
	this->max_number_trains = rcd_file->GetUInt8();
	this->max_number_cars = rcd_file->GetUInt8();
	this->reliability_max = rcd_file->GetUInt16();
	this->reliability_decrease_daily = rcd_file->GetUInt16();
	this->reliability_decrease_monthly = rcd_file->GetUInt16();
	if (this->coaster_kind == 0 || this->coaster_kind >= CST_COUNT) rcd_file->Error("Invalid coaster kind");
	if (this->platform_type == 0 || this->platform_type >= CPT_COUNT) rcd_file->Error("Invalid platform type");

	this->item_type[0] = ITP_RIDE;
	this->item_cost[0] = 100;  // Entrance fee. \todo Read this from the RCD file.
	this->item_cost[1] = 0;    // Unused.

	TextData *text_data;
	LoadTextFromFile(rcd_file, texts, &text_data);
	StringID base = _language.RegisterStrings(*text_data, _coasters_strings_table);
	this->SetupStrings(text_data, base, STR_GENERIC_COASTER_START, COASTERS_STRING_TABLE_END, COASTERS_NAME_TYPE, COASTERS_DESCRIPTION_TYPE);

	int piece_count = rcd_file->GetUInt16();
	length -= 4 * piece_count;
	rcd_file->CheckMinLength(length, 0, "pieces");

	this->pieces.resize(piece_count);
	for (auto &piece : this->pieces) {
		uint32 val = rcd_file->GetUInt32();
		if (val == 0) rcd_file->Error("Empty track piece");  // We don't expect missing track pieces (they should not be included at all).
		auto iter = piece_map.find(val);
		if (iter == piece_map.end()) rcd_file->Error("Track piece not found");
		piece = (*iter).second;
	}
	/* Setup a track voxel list for fast access in the type. */
	for (const auto &piece : this->pieces) {
		for (const auto &tv : piece->track_voxels) {
			this->voxels.push_back(tv.get());
		}
	}

	this->internal_name = rcd_file->GetText();
	rcd_file->CheckExactLength(length, this->internal_name.size() + 1, "end of block");
}

/**
 * Select the default car type for this type of coaster.
 * @return The default car type, or \c nullptr is no car type is available.
 */
const CarType *CoasterType::GetDefaultCarType() const
{
	const CarType *car_type = &_car_types[0]; /// \todo Make a proper #CarType selection.
	return car_type;
}

bool CoasterType::CanMakeInstance() const
{
	return this->GetDefaultCarType() != nullptr;
}

RideInstance *CoasterType::CreateInstance() const
{
	const CarType *car_type = this->GetDefaultCarType();
	assert(car_type != nullptr); // Ensured by CanMakeInstance pre-check.

	return new CoasterInstance(this, car_type);
}

const ImageData *CoasterType::GetView([[maybe_unused]] uint8 orientation) const
{
	return nullptr; // No preview available.
}

const StringID *CoasterType::GetInstanceNames() const
{
	static const StringID names[] = {COASTERS_NAME_INSTANCE, STR_INVALID};
	return names;
}

/**
 * Get the index of the provided track voxel for use as instance data.
 * @param tvx Track voxel to look for.
 * @return Index of the provided track voxel in this coaster type.
 * @see Voxel::SetInstanceData
 */
int CoasterType::GetTrackVoxelIndex(const TrackVoxel *tvx) const
{
	auto match = std::find_if(this->voxels.begin(), this->voxels.end(), [tvx](const TrackVoxel *tv){ return tv == tvx; });
	assert(match != this->voxels.end());
	return std::distance(this->voxels.begin(), match);
}

/** Default constructor, no sprites available yet. */
CoasterPlatform::CoasterPlatform()
:
	ne_sw_back(nullptr), ne_sw_front(nullptr), se_nw_back(nullptr), se_nw_front(nullptr),
	sw_ne_back(nullptr), sw_ne_front(nullptr), nw_se_back(nullptr), nw_se_front(nullptr)
{
}

/**
 * Load a coaster platform (CSPL) block.
 * @param rcd_file Data file being loaded.
 * @param sprites Sprites already loaded from the RCD file.
 */
void LoadCoasterPlatform(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 1 + 8 * 4, "header");

	uint16 width = rcd_file->GetUInt16();
	if (width != 64) rcd_file->Error("Wrong width");
	uint8 type = rcd_file->GetUInt8();
	if (type >= CPT_COUNT) rcd_file->Error("Unknown type");

	CoasterPlatform &platform = _coaster_platforms[type];
	platform.tile_width = width;
	platform.type = static_cast<CoasterPlatformType>(type);

	LoadSpriteFromFile(rcd_file, sprites, &platform.ne_sw_back);
	LoadSpriteFromFile(rcd_file, sprites, &platform.ne_sw_front);
	LoadSpriteFromFile(rcd_file, sprites, &platform.se_nw_back);
	LoadSpriteFromFile(rcd_file, sprites, &platform.se_nw_front);
	LoadSpriteFromFile(rcd_file, sprites, &platform.sw_ne_back);
	LoadSpriteFromFile(rcd_file, sprites, &platform.sw_ne_front);
	LoadSpriteFromFile(rcd_file, sprites, &platform.nw_se_back);
	LoadSpriteFromFile(rcd_file, sprites, &platform.nw_se_front);
}

DisplayCoasterCar::DisplayCoasterCar() : VoxelObject(), yaw(0xff) // Mark everything as invalid.
{
	this->owning_car = nullptr;
}

const ImageData *DisplayCoasterCar::GetSprite([[maybe_unused]] const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const
{
	*recolour = &this->owning_car->owning_train->coaster->recolours;
	return this->car_type->GetCar(this->pitch, this->roll, (this->yaw + orient * 4) & 0xF);
}

VoxelObject::Overlays DisplayCoasterCar::GetOverlays([[maybe_unused]] const SpriteStorage *sprites, ViewOrientation orient) const
{
	Overlays result;
	if (this->owning_car == nullptr) return result;
	for (int i = 0; i < this->car_type->num_passengers; i++) {
		if (this->owning_car->guests[i] != nullptr) {
			result.push_back(Overlay {
					this->car_type->GetGuestOverlay(this->pitch, this->roll, (this->yaw + orient * 4) & 0xF, i),
					&this->owning_car->guests[i]->recolour});
		}
	}
	return result;
}

/**
 * Set the position and orientation of the car. It requests repainting of voxels.
 * @param vox_pos %Voxel position of car.
 * @param pix_pos Position within the voxel (may be outside the \c 0..255 boundary).
 * @param pitch Pitch of the car.
 * @param roll Roll of the car.
 * @param yaw Yaw of the car.
 */
void DisplayCoasterCar::Set(const XYZPoint16 &vox_pos, const XYZPoint16 &pix_pos, uint8 pitch, uint8 roll, uint8 yaw)
{
	bool change_voxel = this->vox_pos != vox_pos;

	if (!change_voxel && this->pix_pos == pix_pos && this->pitch == pitch && this->roll == roll && this->yaw == yaw) {
		return; // Nothing changed.
	}

	if (this->yaw != 0xff && change_voxel) {
		/* Valid data, and changing voxel -> remove self from the old voxel. */
		Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
		this->RemoveSelf(v);
	}

	/* Update voxel and orientation. */
	this->vox_pos = vox_pos;
	this->pix_pos = pix_pos;
	this->pitch = pitch;
	this->roll = roll;
	this->yaw = yaw;

	if (this->yaw != 0xff) {
		if (change_voxel) { // With a really new voxel, also add self to the new voxel.
			Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
			this->AddSelf(v);
		}
	}
}

/** Displayed car is about to be removed from the train, clean up if necessary. */
void DisplayCoasterCar::PreRemove()
{
	/* Nothing to do currently. */
}

static const uint32 CURRENT_VERSION_DisplayCoasterCar = 1;   ///< Currently supported version of %DisplayCoasterCar.
static const uint32 CURRENT_VERSION_CoasterCar        = 1;   ///< Currently supported version of %CoasterCar.
static const uint32 CURRENT_VERSION_CoasterTrain      = 1;   ///< Currently supported version of %CoasterTrain.
static const uint32 CURRENT_VERSION_CoasterInstance   = 1;   ///< Currently supported version of %CoasterInstance.

void DisplayCoasterCar::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("dpcc");
	if (version != CURRENT_VERSION_DisplayCoasterCar) ldr.VersionMismatch(version, CURRENT_VERSION_DisplayCoasterCar);
	this->VoxelObject::Load(ldr);

	this->pitch = ldr.GetByte();
	this->roll = ldr.GetByte();
	this->yaw = ldr.GetByte();

	Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);

	if (v != nullptr) {
		this->AddSelf(v);
	} else {
		throw LoadingError("Invalid world coordinates for coaster car.");
	}
	ldr.ClosePattern();
}

void DisplayCoasterCar::Save(Saver &svr)
{
	svr.StartPattern("dpcc", CURRENT_VERSION_DisplayCoasterCar);
	this->VoxelObject::Save(svr);

	svr.PutByte(this->pitch);
	svr.PutByte(this->roll);
	svr.PutByte(this->yaw);
	svr.EndPattern();
}

void CoasterCar::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("cstc");
	if (version != CURRENT_VERSION_CoasterCar) ldr.VersionMismatch(version, CURRENT_VERSION_CoasterCar);

	this->front.Load(ldr);
	this->back.Load(ldr);
	const long nr_guests = ldr.GetLong();
	this->guests.resize(nr_guests);
	for (int i = 0; i < nr_guests; i++) {
		const int32 id = ldr.GetLong();
		this->guests[i] = id < 0 ? nullptr : _guests.GetCreate(id);
	}
	ldr.ClosePattern();
}

void CoasterCar::Save(Saver &svr)
{
	svr.StartPattern("cstc", CURRENT_VERSION_CoasterCar);
	this->front.Save(svr);
	this->back.Save(svr);
	svr.PutLong(this->guests.size());
	for (Guest *g : this->guests) svr.PutLong(g == nullptr ? -1l : g->id);
	svr.EndPattern();
}

/** Car is about to be removed from the train, clean up if necessary. */
void CoasterCar::PreRemove()
{
#ifndef NDEBUG
	for (Guest *g : this->guests) assert(g == nullptr);
#endif
	this->front.PreRemove();
	this->back.PreRemove();
}

CoasterTrain::CoasterTrain()
:
	coaster(nullptr), // Set later during CoasterInstance::CoasterInstance.
	back_position(0),
	speed(0),
	cur_piece(nullptr), // Set later.
	station_policy(TSP_IN_STATION_BACK),
	time_left_waiting(0)
{
}

/**
 * Change the length of the train.
 * @param length New length of the train.
 */
void CoasterTrain::SetLength(const int length)
{
	for (CoasterCar &car : this->cars) {
		car.PreRemove();
	}
	this->cars.clear();
	this->cars.resize(length);
	for (CoasterCar &car : this->cars) {
		car.owning_train = this;
		car.front.car_type = this->coaster->car_type;
		car.back.car_type = this->coaster->car_type;
		car.front.owning_car = &car;
		car.back.owning_car = &car;
		car.guests.resize(car.front.car_type->num_passengers, nullptr);
	}
}

/** Combined sin/cos table for 16 roll positions. sin table starts at 0, cos table start at 4. */
static const float _sin_cos_table[20] = {
	0.00, 0.38, 0.71, 0.92,
	1.00, 0.92, 0.71, 0.38, 0.00, -0.38, -0.71, -0.92, -1.00, -0.92, -0.71, -0.38,
	0.00, 0.38, 0.71, 0.92
};

/**
 * Revert the roll of the coaster car in the direction vector.
 * @param roll Amount of roll of the car.
 * @param dy [inout] Derivative of Y position.
 * @param dz [inout] Derivative of Z position.
 */
static void inline Unroll(uint roll, int32 *dy, int32 *dz)
{
	/* Rotates around X axis, thus dx does not change. */
	int32 new_dy = *dy * _sin_cos_table[roll + 4] - *dz * _sin_cos_table[roll];
	int32 new_dz = *dy * _sin_cos_table[roll]     + *dz * _sin_cos_table[roll + 4];
	*dy = new_dy;
	*dz = new_dz;
}

/**
 * Time has passed, update the position of the train.
 * @param delay Amount of time passed, in milliseconds.
 */
void CoasterTrain::OnAnimate(int delay)
{
	if (this->coaster->state != RIS_OPEN && this->coaster->state != RIS_TESTING) delay = 0;
	if (this->station_policy == TSP_IN_STATION_FRONT) {
		this->time_left_waiting -= delay;
		delay = 0;  // Don't move forward while in station.
	} else if (this->station_policy == TSP_IN_STATION_BACK) {
		delay = 0;
	} else if (this->station_policy != TSP_ENTERING_STATION) {
		this->time_left_waiting = 0;
	}

	if (this->speed >= 0) {
		this->back_position += this->speed * delay;
		if (this->back_position >= this->coaster->coaster_length) {
			this->back_position -= this->coaster->coaster_length;
			this->cur_piece = this->coaster->pieces.get();
		}
		while (this->cur_piece->distance_base + this->cur_piece->piece->piece_length < this->back_position) this->cur_piece++;
	} else {
		uint32 change = -this->speed * delay;
		if (change > this->back_position) {
			this->back_position = this->back_position + this->coaster->coaster_length - change;
			/* Update the track piece as well. There is no simple way to get
			 * the last piece, so moving from the front will have to do. */
			this->cur_piece = this->coaster->pieces.get();
			while (this->cur_piece->distance_base + this->cur_piece->piece->piece_length < this->back_position) this->cur_piece++;
		} else {
			this->back_position -= change;
			while (this->cur_piece->distance_base > this->back_position) this->cur_piece--;
		}
	}
	uint32 car_length = this->coaster->car_type->car_length;
	uint32 position = this->back_position; // Back position of the train / last car.
	const PositionedTrackPiece *ptp = this->cur_piece;
	for (uint i = 0; i < this->cars.size(); i++) {
		CoasterCar &car = this->cars[i];
		if (position >= this->coaster->coaster_length) {
			position -= this->coaster->coaster_length;
			ptp = this->coaster->pieces.get();
		}
		while (ptp->distance_base + ptp->piece->piece_length < position) ptp++;

		/* Get position of the back of the car. */
		int32 xpos_back = ptp->piece->car_xpos->GetValue(position - ptp->distance_base) + (ptp->base_voxel.x << 8);
		int32 ypos_back = ptp->piece->car_ypos->GetValue(position - ptp->distance_base) + (ptp->base_voxel.y << 8);
		int32 zpos_back = ptp->piece->car_zpos->GetValue(position - ptp->distance_base) * 2 + (ptp->base_voxel.z << 8);

		/* Get roll from the center of the car. */
		position += car_length / 2;
		if (position >= this->coaster->coaster_length) {
			position -= this->coaster->coaster_length;
			ptp = this->coaster->pieces.get();
		}
		while (ptp->distance_base + ptp->piece->piece_length < position) ptp++;
		uint roll = static_cast<uint>(ptp->piece->car_roll->GetValue(position - ptp->distance_base) + 0.5) & 0xf;

		/* Get position of the front of the car. */
		position += car_length / 2;
		if (position >= this->coaster->coaster_length) {
			position -= this->coaster->coaster_length;
			ptp = this->coaster->pieces.get();
		}
		while (ptp->distance_base + ptp->piece->piece_length < position) ptp++;
		int32 xpos_front = ptp->piece->car_xpos->GetValue(position - ptp->distance_base) + (ptp->base_voxel.x << 8);
		int32 ypos_front = ptp->piece->car_ypos->GetValue(position - ptp->distance_base) + (ptp->base_voxel.y << 8);
		int32 zpos_front = ptp->piece->car_zpos->GetValue(position - ptp->distance_base) * 2 + (ptp->base_voxel.z << 8);

		int32 xder = xpos_front - xpos_back;
		int32 yder = ypos_front - ypos_back;
		int32 zder = (zpos_front - zpos_back) / 2; // Tile height is half the width.

		int32 xpos_middle = xpos_back + xder / 2; // Compute center point of the car as position to render the car.
		int32 ypos_middle = ypos_back + yder / 2;
		int32 zpos_middle = zpos_back + zder;

		float total_speed  = sqrt(xder * xder + yder * yder + zder * zder);
		/* Gravity */
		this->speed -= zder / total_speed * 9.8;

		/** \todo Air and rail friction */

		/* Unroll the orientation vector. */
		Unroll(roll, &yder, &zder);
		float horizontal_speed = std::hypot(xder, yder);

		static const double TAN11_25 = 0.198912367379658;  // tan(11.25 degrees).
		static const double TAN33_75 = 0.6681786379192989; // tan(3*11.25 degrees).

		/* Compute pitch. */
		bool swap_dz = false;
		if (zder < 0) {
			swap_dz = true;
			zder = -zder;
		}

		uint pitch;
		if (horizontal_speed < zder) {
			if (horizontal_speed < zder * TAN11_25) {
				pitch = 4;
			} else if (horizontal_speed < zder * TAN33_75) {
				pitch = 3;
			} else {
				pitch = 2;
			}
		} else {
			if (zder < horizontal_speed * TAN11_25) {
				pitch = 0;
			} else if (zder < horizontal_speed * TAN33_75) {
				pitch = 1;
			} else {
				pitch = 2;
			}
		}
		if (swap_dz) pitch = (16 - pitch) & 0xf;

		/* Compute yaw. */
		bool swap_dx = false;
		if (xder > 0) {
			swap_dx = true;
			xder = -xder;
		}
		bool swap_dy = false;
		if (yder > 0) {
			swap_dy = true;
			yder = -yder;
		}
		uint yaw;
		/* There are 16 yaw directions, the 360 degrees needs to be split in 16 pieces.
		 * However the x and y axes are in the middle of a piece, so 360 degrees is split
		 * in 32 parts, and 2 parts form one piece. */
		if (xder < yder) {
			/* In the first 45 degrees. It is split in 4 parts (4*11.25 degrees)
			 * where the 1st part is for direction 0. The 2nd and 3rd part are for direction 1,
			 * and the 4th part is for direction 2. */
			if (xder * TAN11_25 < yder) {
				yaw = 0;
			} else if (xder * TAN33_75 < yder) {
				yaw = 1;
			} else {
				yaw = 2;
			}
		} else {
			/* In the second 45 degrees. It is also split in 4 parts (4*11.25 degrees)
			 * where the 1st part is for direction 2. The 2nd and 3rd part are for direction 3,
			 * and the 4th part is for direction 4.
			 *
			 * Rather than re-inventing a solution, re-use the same checks as
			 * above with swapped xder and yder. */
			if (yder * TAN11_25 < xder) {
				yaw = 4;
			} else if (yder * TAN33_75 < xder) {
				yaw = 3;
			} else {
				yaw = 2;
			}
		}
		if (swap_dx) yaw = 8 - yaw;
		if (swap_dy) yaw = (16 - yaw) & 0xf;

		xpos_back  &= 0xFFFFFF00; // Back and front positions to the north bottom corner of the voxel.
		ypos_back  &= 0xFFFFFF00;
		zpos_back  &= 0xFFFFFF00;
		xpos_front &= 0xFFFFFF00;
		ypos_front &= 0xFFFFFF00;
		zpos_front &= 0xFFFFFF00;
		XYZPoint16 back( xpos_back  >> 8, ypos_back  >> 8, zpos_back  >> 8);
		XYZPoint16 front(xpos_front >> 8, ypos_front >> 8, zpos_front >> 8);

		XYZPoint16 back_pix( xpos_middle - xpos_back,  ypos_middle - ypos_back,  zpos_middle - zpos_back);
		XYZPoint16 front_pix(xpos_middle - xpos_front, ypos_middle - ypos_front, zpos_middle - zpos_front);

		car.back.Set (back,  back_pix,  pitch, roll, yaw);
		car.front.Set(front, front_pix, pitch, roll, yaw);
		position += this->coaster->car_type->inter_car_length;

		if (i == 0) {
			/*
			 * \todo This "calculation" of horizontal and vertical G forces is extremely
			 * simplistic. For now this is good enough, but when we have coasters with loopings,
			 * banked curves and so on, this will need to be replaced with a proper physics
			 * model, as the results of this magic would make no sense for such tracks.
			 */
			this->coaster->SampleStatistics(this->back_position, this->station_policy == TSP_NO_STATION, this->speed,
					pitch > 8 ? static_cast<int>(pitch) - 16 : pitch,
					roll  > 8 ? static_cast<int>(roll)  - 16 : roll);
		}
	}

	bool has_platform = false, has_power = false;
	uint32 indexed_car_position = this->back_position;
	const PositionedTrackPiece *indexed_car_piece = this->cur_piece;
	int car_index = cars.size();
	do {
		if (car_index > 0) {
			if (indexed_car_piece->piece->HasPlatform()) {
				has_platform = true;
				if (this->station_policy == TSP_NO_STATION) {
					this->station_policy = TSP_ENTERING_STATION;
					this->time_left_waiting = this->coaster->state == RIS_TESTING ? TRAIN_DEPARTURE_INTERVAL_TESTING : this->coaster->max_idle_duration;
					this->coaster->RecalculateRatings();  // Recalculate ratings whenever a train has completed a circuit.
				}
			}
			has_power |= indexed_car_piece->piece->HasPower();
		}
		indexed_car_position += (car_length + this->coaster->car_type->inter_car_length);
		if (indexed_car_position >= this->coaster->coaster_length) {
			indexed_car_position -= this->coaster->coaster_length;
			indexed_car_piece = this->coaster->pieces.get();
		}
		while (indexed_car_piece->distance_base + indexed_car_piece->piece->piece_length < indexed_car_position) {
			indexed_car_piece++;
		}
		car_index--;
	} while (car_index > 0);
	const bool front_is_in_station = indexed_car_piece->piece->HasPlatform();
	/* Powered tiles speed the car up if it is slow; station tiles set a fixed speed. */
	if (has_platform || (has_power && this->speed < 65536 / 1000)) {
		const int32 max_speed_change = delay;  // Determines how quickly trains accelerate and brake.
		this->speed -= std::min<int32>(max_speed_change, std::max<int32>(-max_speed_change, this->speed - 65536 / 1000));
	}

	bool other_train_directly_in_front = false, other_train_in_station_front = false;
	for (CoasterTrain &train : this->coaster->trains) {
		if (&train == this || train.cars.empty() || this->back_position > train.back_position) continue;
		if (delay > 0 && indexed_car_position > train.back_position) {
			this->coaster->Crash(this, &train);
			return;
		}
		other_train_directly_in_front |= (indexed_car_position + 256 * this->coaster->GetTrainSpacing() > train.back_position);
		other_train_in_station_front |= (indexed_car_position + 2 * 256 * this->coaster->GetTrainSpacing() > train.back_position);
	}

	if (!has_platform && this->station_policy == TSP_LEAVING_STATION) this->station_policy = TSP_NO_STATION;
	if (this->station_policy == TSP_ENTERING_STATION || this->station_policy == TSP_IN_STATION_FRONT || this->station_policy == TSP_IN_STATION_BACK) {
		if (!front_is_in_station && this->time_left_waiting <= 0) {
			this->station_policy = TSP_LEAVING_STATION;
		} else if (front_is_in_station && !other_train_directly_in_front) {
			this->station_policy = TSP_ENTERING_STATION;
		} else {
			bool is_inside_station = false;
			if (this->station_policy == TSP_ENTERING_STATION) {
				int station_index = 0;
				for (const CoasterStation &s : this->coaster->stations) {
					if (this->coaster->IsInStation(this->back_position, s)) {
						is_inside_station = true;
						break;
					}
					station_index++;
				}
				if (is_inside_station) {
					for (CoasterCar &car : this->cars) {
						for (uint i = 0; i < car.guests.size(); i++) {
							if (car.guests[i] != nullptr) {
								car.guests[i]->ExitRide(this->coaster, static_cast<TileEdge>(station_index));
								car.guests[i] = nullptr;
							}
						}
					}
				}
			} else {
				is_inside_station = true;
			}
			if (is_inside_station) {
				this->station_policy = other_train_in_station_front ? TSP_IN_STATION_BACK : TSP_IN_STATION_FRONT;
			}
		}
	}
}

void CoasterTrain::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("cstt");
	if (version != CURRENT_VERSION_CoasterTrain) ldr.VersionMismatch(version, CURRENT_VERSION_CoasterTrain);

	for (std::vector<CoasterCar>::iterator it = this->cars.begin(); it != this->cars.end(); ++it) {
		it->Load(ldr);
	}

	this->back_position = ldr.GetLong();
	this->speed = (int32)ldr.GetLong();
	this->station_policy = static_cast<TrainStationPolicy>(ldr.GetByte());
	this->time_left_waiting = ldr.GetLong();
	ldr.ClosePattern();
}

void CoasterTrain::Save(Saver &svr)
{
	svr.StartPattern("cstt", CURRENT_VERSION_CoasterTrain);

	for (std::vector<CoasterCar>::iterator it = this->cars.begin(); it != this->cars.end(); ++it) {
		it->Save(svr);
	}

	svr.PutLong(this->back_position);
	svr.PutLong((uint32)this->speed);
	svr.PutByte(this->station_policy);
	svr.PutLong(this->time_left_waiting);
	svr.EndPattern();
}

/** Default constructor for a station of length 0 with no entrance or exit. */
CoasterStation::CoasterStation()
:
	direction(INVALID_EDGE),
	length(0),
	back_position(0),
	entrance(XYZPoint16::invalid()),
	exit(XYZPoint16::invalid())
{
}

/**
 * Constructor of a roller coaster instance.
 * @param ct Coaster type being built.
 * @param init_car_type Kind of cars used for the coaster.
 */
CoasterInstance::CoasterInstance(const CoasterType *ct, const CarType *init_car_type) : RideInstance(ct),
	pieces(new PositionedTrackPiece[MAX_PLACED_TRACK_PIECES]()),
	capacity(MAX_PLACED_TRACK_PIECES),
	number_of_trains(0),
	cars_per_train(0),
	car_type(init_car_type),
	temp_entrance_pos(XYZPoint16::invalid()),
	temp_exit_pos(XYZPoint16::invalid()),
	max_idle_duration(30000),
	min_idle_duration(5000)
{
	for (uint i = 0; i < lengthof(this->trains); i++) {
		CoasterTrain &train = this->trains[i];
		train.coaster = this;
		train.cur_piece = this->pieces.get();
	}
}

const Recolouring *CoasterInstance::GetRecolours(const XYZPoint16 &pos) const
{
	if (pos == this->temp_entrance_pos) return &this->entrance_recolours;
	if (pos == this->temp_exit_pos) return &this->exit_recolours;
	for (const CoasterStation &s : this->stations) {
		if (pos == s.entrance) return &this->entrance_recolours;
		if (pos == s.exit) return &this->exit_recolours;
	}
	return RideInstance::GetRecolours(pos);
}

/**
 * Can the user click in the world to re-open the coaster instance window for this coaster?
 * @return \c true if an object in the world exists, \c false otherwise.
 */
bool CoasterInstance::IsAccessible()
{
	return this->GetFirstPlacedTrackPiece() >= 0;
}

bool CoasterInstance::CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const
{
	if (!RideInstance::CanBeVisited(vox, edge)) return false;
	for (const CoasterStation &s : this->stations) {
		if (vox == s.entrance && (edge + 2) % 4 == EntranceExitRotation(vox, &s)) return true;
	}
	return false;
}

void CoasterInstance::OnAnimate(int delay)
{
	RideInstance::OnAnimate(delay);
	if (this->broken) return;

	for (uint i = 0; i < lengthof(this->trains); i++) {
		CoasterTrain &train = this->trains[i];
		if (train.cars.size() == 0) break;
		train.OnAnimate(delay);
	}
}

void CoasterInstance::TestRide()
{
	if (this->state != RIS_OPEN) {
		/*
		 * Reset trains. If we are already open we can skip this smoothly.
		 * If the user clicked the Test button again during testing, reset the trains anyway.
		 */
		this->CloseRide();
		this->ReinitializeTrains(true);
	}
	this->state = RIS_TESTING;
}

void CoasterInstance::OpenRide()
{
	if (this->state == RIS_OPEN) return;
	if (this->state != RIS_TESTING) {
		/* Reset trains. If we are currently testing we can skip this smoothly. */
		this->CloseRide();
		this->ReinitializeTrains(false);
	}
	RideInstance::OpenRide();
}

void CoasterInstance::CloseRide()
{
	this->intensity_statistics.clear();
	for (uint i = 0; i < lengthof(this->trains); i++) {
		CoasterTrain &train = this->trains[i];
		train.back_position = 0;
		train.speed = 0;
		train.station_policy = TSP_IN_STATION_BACK;
		train.cur_piece = this->pieces.get();
		train.cars.resize(0);
		train.OnAnimate(0);
	}
	RideInstance::CloseRide();
	this->RecalculateRatings();
}

void CoasterInstance::InitializeItemPricesAndStatistics()
{
	RideInstance::InitializeItemPricesAndStatistics();
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		this->item_price[i] = GetRideType()->item_cost[i];
	}
}

void CoasterInstance::GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const
{
	const CoasterType *ct = this->GetCoasterType();

	sprites[0] = nullptr;
	sprites[3] = nullptr;
	auto orientation_index = [orient](const int o) {
		return (4 + o - orient) & 3;
	};
	if (this->IsEntranceLocation(vox)) {
		const ImageData *const *array = _rides_manager.entrances[this->entrance_type]->images[orientation_index(EntranceExitRotation(vox, nullptr))];
		sprites[1] = array[0];
		sprites[2] = array[1];
		if (platform != nullptr) *platform = PATH_NE_NW_SE_SW;
		return;
	}
	if (this->IsExitLocation(vox)) {
		const ImageData *const *array = _rides_manager.exits[this->exit_type]->images[orientation_index(EntranceExitRotation(vox, nullptr))];
		sprites[1] = array[0];
		sprites[2] = array[1];
		if (platform != nullptr) *platform = PATH_NE_NW_SE_SW;
		return;
	}
	if (voxel_number == ENTRANCE_OR_EXIT) {
		sprites[1] = nullptr;
		sprites[2] = nullptr;
		return;
	}

	assert(voxel_number < ct->voxels.size());
	const TrackVoxel *tv = ct->voxels[voxel_number];

	sprites[1] = tv->back[orient];  // SO_RIDE
	sprites[2] = tv->front[orient]; // SO_RIDE_FRONT
	if ((tv->back[orient] == nullptr && tv->front[orient] == nullptr) || !tv->HasPlatform() || ct->platform_type >= CPT_COUNT) {
		sprites[0] = nullptr; // SO_PLATFORM_BACK
		sprites[3] = nullptr; // SO_PLATFORM_FRONT
	} else {
		const CoasterPlatform &platform = _coaster_platforms[ct->platform_type];
		TileEdge edge = static_cast<TileEdge>(orientation_index(tv->GetPlatformDirection()));
		switch (edge) {
			case EDGE_NE:
				sprites[0] = platform.ne_sw_back;
				sprites[3] = platform.ne_sw_front;
				break;

			case EDGE_SE:
				sprites[0] = platform.se_nw_back;
				sprites[3] = platform.se_nw_front;
				break;

			case EDGE_SW:
				sprites[0] = platform.sw_ne_back;
				sprites[3] = platform.sw_ne_front;
				break;

			case EDGE_NW:
				sprites[0] = platform.nw_se_back;
				sprites[3] = platform.nw_se_front;
				break;

			default: NOT_REACHED();
		}
	}
}

uint8 CoasterInstance::GetEntranceDirections(const XYZPoint16 &vox) const
{
	for (const CoasterStation &s : this->stations) {
		if (s.entrance == vox || s.exit == vox) return 1 << this->EntranceExitRotation(vox, &s);
	}
	return SHF_ENTRANCE_NONE;
}

RideEntryResult CoasterInstance::EnterRide(int guest_id, const XYZPoint16 &vox, [[maybe_unused]] TileEdge edge)
{
	Guest *guest = _guests.GetExisting(guest_id);
	if (guest->cash < GetSaleItemPrice(0)) return RER_REFUSED;
	Random r;
	for (const CoasterStation &s : this->stations) {
		if (s.entrance != vox) continue;

		/* Find the frontmost train in this station. */
		CoasterTrain *loading_train = nullptr;
		for (CoasterTrain &t : this->trains) {
			if (t.station_policy == TSP_IN_STATION_FRONT &&
					this->IsInStation(t.back_position, s) &&
					(loading_train == nullptr ||
							this->PositionRelativeTo(loading_train->back_position, s.back_position) <
							this->PositionRelativeTo(t.back_position, s.back_position))) {
				loading_train = &t;
			}
		}
		if (loading_train == nullptr) return RER_WAIT;

		/* Find all free seats in the train. */
		std::set<std::pair<CoasterCar*, int>> free_slots;
		for (CoasterCar &car : loading_train->cars) {
			for (uint i = 0; i < car.guests.size(); i++) {
				if (car.guests[i] == nullptr) free_slots.insert(std::make_pair(&car, i));
			}
		}
		if (free_slots.empty()) return RER_WAIT;

		auto it = free_slots.begin();
		std::advance(it, r.Uniform(free_slots.size() - 1));
		it->first->guests[it->second] = guest;
		if (free_slots.size() == 1) {
			/* Start the train as soon as the minimum idle duration has elapsed. */
			if (loading_train->time_left_waiting > this->max_idle_duration - this->min_idle_duration) {
				loading_train->time_left_waiting += this->max_idle_duration - this->min_idle_duration;
			} else {
				loading_train->time_left_waiting = 0;
			}
		}
		return RER_ENTERED;
	}
	NOT_REACHED();
}

EdgeCoordinate CoasterInstance::GetMechanicEntrance() const
{
	for (const CoasterStation &s : this->stations) {  // Pick the first exit there is.
		if (s.exit != XYZPoint16::invalid()) return EdgeCoordinate {s.exit, static_cast<TileEdge>(this->EntranceExitRotation(s.exit, &s))};
	}
	NOT_REACHED();
}

/* We here (mis-)use the TileEdge parameter as the index of the station at which the guest is getting off. */
XYZPoint32 CoasterInstance::GetExit([[maybe_unused]] int guest, TileEdge station_index)
{
	const CoasterStation &station = this->stations[static_cast<int>(station_index)];
	const int direction = this->EntranceExitRotation(station.exit, &station);
	XYZPoint32 p(station.exit.x * 256, station.exit.y * 256, station.exit.z * 256);
	Random r;  // Don't put all guests on exactly the same spot.
	const int d = 128 + r.Uniform(128) - 64;
	switch (direction) {
		case VOR_WEST:  p.x += d;       p.y -= 32;      break;
		case VOR_EAST:  p.x += d;       p.y += 256+32;  break;
		case VOR_NORTH: p.x -= 32;      p.y += d;       break;
		case VOR_SOUTH: p.x += 256+32;  p.y += d;       break;
		default: NOT_REACHED();
	}
	return p;
}

bool CoasterInstance::PathEdgeWanted(const XYZPoint16 &vox, const TileEdge edge) const
{
	for (const CoasterStation &s : this->stations) {
		if (s.entrance == vox || s.exit == vox) return edge == this->EntranceExitRotation(vox, &s);
	}
	return false;
}

void CoasterInstance::RemoveAllPeople()
{
	for (CoasterTrain &train : this->trains) {
		for (CoasterCar &car : train.cars) {
			for (uint i = 0; i < car.guests.size(); i++) {
				if (car.guests[i] != nullptr) {
					car.guests[i]->ExitRide(this, static_cast<TileEdge>(0));
					car.guests[i] = nullptr;
				}
			}
		}
	}
}

/**
 * Retrieve the first placed track piece, if available.
 * @return The index of the first placed track piece, or \c -1 if no such piece available.
 */
int CoasterInstance::GetFirstPlacedTrackPiece() const
{
	for (int i = 0; i < this->capacity; i++) {
		if (this->pieces[i].piece != nullptr) return i;
	}
	return -1;
}

void CoasterInstance::RemoveFromWorld()
{
	this->RemoveStationsFromWorld();
	const uint16 index = this->GetIndex();
	const int start_piece = GetFirstPlacedTrackPiece();
	if (start_piece < 0) return;
	for (int p = start_piece;;) {
		this->pieces[p].RemoveFromWorld(index);
		p = FindSuccessorPiece(this->pieces[p]);
		if (p < 0 || p == start_piece) break;
	}
}

/** Immediately remove all entrances and exits of this ride from all voxels they currently occupy. */
void CoasterInstance::RemoveStationsFromWorld()
{
#ifndef NDEBUG
	const SmallRideInstance index = static_cast<SmallRideInstance>(this->GetIndex());
#endif
	for (CoasterStation &s : this->stations) {
		for (const XYZPoint16 &p : {s.entrance, s.exit}) {
			if (p != XYZPoint16::invalid()) {
				const int8 height = (p == s.entrance) ? RideEntranceExitType::entrance_height : RideEntranceExitType::exit_height;
				for (int16 h = 0; h < height; ++h) {
					Voxel *voxel = _world.GetCreateVoxel(p + XYZPoint16(0, 0, h), false);
					if (voxel != nullptr && voxel->instance != SRI_FREE) {
						assert(voxel->instance == index);
						voxel->ClearInstances();
					}
				}
				AddRemovePathEdges(p, PATH_EMPTY, EDGE_ALL, PAS_UNUSED);
			}
		}
	}
}

/** Link all entrances and exits of this ride into all voxels they are meant to occupy. */
void CoasterInstance::InsertStationsIntoWorld()
{
	const SmallRideInstance index = static_cast<SmallRideInstance>(this->GetIndex());
	for (CoasterStation &s : this->stations) {
		for (const XYZPoint16 &p : {s.entrance, s.exit}) {
			if (p != XYZPoint16::invalid()) {
				const bool entrance = (p == s.entrance);
				const int8 height = entrance ? RideEntranceExitType::entrance_height : RideEntranceExitType::exit_height;
				for (int16 h = 0; h < height; ++h) {
					const XYZPoint16 pos = p + XYZPoint16(0, 0, h);
					Voxel *voxel = _world.GetCreateVoxel(pos, true);
					assert(voxel != nullptr && voxel->instance == SRI_FREE);
					voxel->SetInstance(index);
					voxel->SetInstanceData(ENTRANCE_OR_EXIT);
				}
				AddRemovePathEdges(p, PATH_EMPTY, GetEntranceDirections(p), entrance ? PAS_QUEUE_PATH : PAS_NORMAL_PATH);
			}
		}
	}
}

/**
 * Find the first placed track piece at a given position with a given entry connection.
 * @param vox Required voxel position.
 * @param entry_connect Required entry connection.
 * @param start First index to search.
 * @param end End of the search (one beyond the last positioned track piece to search).
 * @return Index of the requested positioned track piece if it exists, else \c -1.
 */
int CoasterInstance::FindSuccessorPiece(const XYZPoint16 &vox, uint8 entry_connect, int start, int end)
{
	if (start < 0) start = 0;
	if (end > MAX_PLACED_TRACK_PIECES) end = MAX_PLACED_TRACK_PIECES;
	for (; start < end; start++) {
		if (this->pieces[start].CanBeSuccessor(vox, entry_connect)) return start;
	}
	return -1;
}

/**
 * Find the first placed track piece that follows a provided placed track piece.
 * @param placed Already placed track piece.
 * @return Index of the requested positioned track piece if it exists, else \c -1.
 */
int CoasterInstance::FindSuccessorPiece(const PositionedTrackPiece &placed)
{
	return this->FindSuccessorPiece(placed.GetEndXYZ(), placed.piece->exit_connect);
}

/**
 * Find the first placed track piece that precedes a provided placed track piece.
 * @param placed Already placed track piece.
 * @return Index of the requested positioned track piece if it exists, else \c -1.
 */
int CoasterInstance::FindPredecessorPiece(const PositionedTrackPiece &placed)
{
	for (int i = 0; i < this->capacity; i++) {
		if (placed.CanBeSuccessor(this->pieces[i])) {
			return i;
		}
	}
	return -1;
}

/**
 * Try to make a loop with the current set of positioned track pieces.
 * @param modified [out] If not \c nullptr, \c false after the call denotes the pieces array was not modified
 *                       (while \c true denotes it was changed).
 * @return Whether the current set of positioned track pieces can be used as a ride.
 * @note May re-order positioned track pieces.
 */
bool CoasterInstance::MakePositionedPiecesLooping(bool *modified)
{
	this->UpdateStations();
	if (modified != nullptr) *modified = false;

	/* First step, move all non-null track pieces to the start of the array. */
	int count = 0;
	for (int i = 0; i < this->capacity; i++) {
		PositionedTrackPiece *ptp = this->pieces.get() + i;
		if (ptp->piece == nullptr) continue;
		if (i == count) {
			count++;
			continue;
		}
		std::swap(this->pieces[count], *ptp);
		if (modified != nullptr) *modified = true;
		ptp->piece = nullptr;
		count++;
	}

	/* Second step, find a loop from start to end. */
	if (count < 2) return false; // 0 or 1 positioned pieces won't ever make a loop.

	PositionedTrackPiece *ptp = this->pieces.get();
	uint32 distance = 0;
	if (ptp->distance_base != distance) {
		if (modified != nullptr) *modified = true;
		ptp->distance_base = distance;
	}
	distance += ptp->piece->piece_length;

	for (int i = 1; i < count; i++) {
		int j = this->FindSuccessorPiece(ptp->GetEndXYZ(), ptp->piece->exit_connect, i, count);
		if (j < 0) return false;
		ptp++; // Now points to pieces[i].
		if (i != j) {
			std::swap(*ptp, this->pieces[j]); // Make piece 'j' the next positioned piece.
			if (modified != nullptr) *modified = true;
		}
		if (ptp->distance_base != distance) {
			if (modified != nullptr) *modified = true;
			ptp->distance_base = distance;
		}
		distance += ptp->piece->piece_length;
	}
	this->coaster_length = distance;
	this->UpdateStations();
	return this->pieces[0].CanBeSuccessor(*ptp);
}

/**
 * Try to add a positioned track piece to the coaster instance.
 * @param placed New positioned track piece to add.
 * @return If the new piece can be added, its index is returned, else \c -1.
 */
int CoasterInstance::AddPositionedPiece(const PositionedTrackPiece &placed)
{
	if (placed.piece == nullptr || !placed.IsOnWorld()) return -1;

	for (int i = 0; i < this->capacity; i++) {
		if (this->pieces[i].piece == nullptr) {
			this->pieces[i] = placed;
			if (placed.piece->IsStartingPiece()) this->UpdateStations();
			return i;
		}
	}
	return -1;
}

/**
 * Try to remove a positioned track piece from the coaster instance.
 * @param piece Positioned track piece to remove.
 */
void CoasterInstance::RemovePositionedPiece(PositionedTrackPiece &piece)
{
	assert(piece.piece != nullptr);
	this->RemoveTrackPieceInWorld(piece);
	if (piece.piece->IsStartingPiece()) this->UpdateStations();
	piece.piece = nullptr;
}

/**
 * Get the number of this ride.
 * @return The (unique) number of this ride.
 */
SmallRideInstance CoasterInstance::GetRideNumber() const
{
	SmallRideInstance ride_number = (SmallRideInstance)this->GetIndex();
	assert(ride_number >= SRI_FULL_RIDES && ride_number <= SRI_LAST);
	return ride_number;
}

/**
 * Get the instance data of a track voxel that is to be placed in a voxel.
 * @param tv Track voxel to display in a voxel.
 * @return instance data of that track voxel.
 */
uint16 CoasterInstance::GetInstanceData(const TrackVoxel *tv) const
{
	const CoasterType *ct = this->GetCoasterType();
	return ct->GetTrackVoxelIndex(tv);
}

/**
 * Add the positioned track piece to #_world.
 * @param placed Track piece to place.
 * @pre placed->CanBePlaced() should hold.
 */
void CoasterInstance::PlaceTrackPieceInWorld(const PositionedTrackPiece &placed)
{
	assert(placed.CanBePlaced() == STR_NULL);
	SmallRideInstance ride_number = this->GetRideNumber();

	for (const auto& tvx : placed.piece->track_voxels) {
		Voxel *vx = _world.GetCreateVoxel(placed.base_voxel + tvx->dxyz, true);
		// assert(vx->CanPlaceInstance()): Checked by this->CanBePlaced().
		vx->SetInstance(ride_number);
		vx->SetInstanceData(this->GetInstanceData(tvx.get()));
	}
}

/**
 * Add 'removal' of the positioned track piece to #_world.
 * @param placed Track piece to be removed.
 */
void CoasterInstance::RemoveTrackPieceInWorld(const PositionedTrackPiece &placed)
{
	for (const auto& tvx : placed.piece->track_voxels) {
		Voxel *vx = _world.GetCreateVoxel(placed.base_voxel + tvx->dxyz, false);
		assert(vx->GetInstance() == this->GetRideNumber());
		vx->SetInstance(SRI_FREE);
		vx->SetInstanceData(0); // Not really needed.
	}
}

/**
 * Find the length of the ride's shortest station.
 * @return Shortest station length in pixels.
 */
uint32 CoasterInstance::GetShortestStation() const
{
	if (this->stations.empty()) return 0;
	uint32 l = stations[0].length;
	for (const CoasterStation &s : this->stations) l = std::min(l, s.length);
	return l;
}

/**
 * Determine the length of a train with the given number of cars.
 * @param cars Number of cars in the train.
 * @return Train length in pixels.
 */
uint32 CoasterInstance::GetTrainLength(const int cars) const
{
	return cars > 0 ? (cars * this->car_type->car_length / 256 + (cars - 1) * this->car_type->inter_car_length / 256) : 0;
}

/**
 * Decide the minimum spacing between two trains in a station.
 * @return Spacing in pixels.
 */
uint32 CoasterInstance::GetTrainSpacing() const
{
	return this->car_type->car_length / 512;  // Half a car length.
}

/**
 * Determine how many trains of the given size this ride can own at most.
 * @param cars Number of cars in each train.
 * @return Maximum number of trains allowed.
 */
int CoasterInstance::GetMaxNumberOfTrains(const int cars) const
{
	if (cars < 1) return 0;
	if (cars > GetMaxNumberOfCars()) return 0;
	return std::max(1, std::min<int>(
		this->GetShortestStation() / (this->GetTrainLength(cars) + this->GetTrainSpacing()),
		this->GetCoasterType()->max_number_trains));
}

/**
 * Determine how many cars each train in this ride can own at most.
 * @return Maximum number of cars per train allowed.
 */
int CoasterInstance::GetMaxNumberOfCars() const
{
	const uint32 shortest_station = this->GetShortestStation();
	for (int c = this->GetCoasterType()->max_number_cars; c >= 0; c--) {
		if (this->GetTrainLength(c) <= shortest_station) return c;
	}
	NOT_REACHED();
}

/**
 * Change the number of cars in this ride's trains. Does not update the positions of the trains.
 * @param number_cars Number of cars in each train.
 * @pre This should not be done while the ride is running.
 */
void CoasterInstance::SetNumberOfCars(const int number_cars)
{
	this->cars_per_train = number_cars;
	for (int i = 0; i < this->number_of_trains; i++) {
		this->trains[i].SetLength(this->cars_per_train);
	}
}

/**
 * Change the number of trains in this ride's trains, and move all trains to their initial positions inside the first station.
 * @param number_trains Number of trains.
 * @pre This should not be done while the ride is running.
 */
void CoasterInstance::SetNumberOfTrains(const int number_trains)
{
	this->number_of_trains = number_trains;
	PositionedTrackPiece *location = this->pieces.get();
	uint32 back_position = 0;
	uint32 back_position_in_piece = 0;
	const uint32 train_length = 256 * (this->GetTrainLength(this->cars_per_train) + this->GetTrainSpacing());
	for (uint i = 0; i < lengthof(this->trains); i++) {
		CoasterTrain &train = this->trains[i];
		train.cur_piece = location;
		train.back_position = back_position;
		train.speed = 0;
		train.station_policy = (static_cast<int>(i) + 1 == number_trains) ? TSP_IN_STATION_FRONT : TSP_IN_STATION_BACK;
		train.time_left_waiting = 0;
		if (static_cast<int>(i) < number_trains) {
			train.SetLength(this->cars_per_train);
			back_position += train_length;
			back_position_in_piece += train_length;
			while (back_position_in_piece >= 256 * 256) {
				back_position_in_piece -= 256 * 256;
				location++;
			}
		} else {
			train.SetLength(0);
		}
		train.OnAnimate(0);
	}
}

/**
 * Reset all trains to their initial positions in the station.
 * @param test_mode Whether the ride is in test mode.
 */
void CoasterInstance::ReinitializeTrains(const bool test_mode)
{
	this->SetNumberOfCars(this->cars_per_train);
	this->SetNumberOfTrains(this->number_of_trains);  // This takes care of actually moving the trains to the correct positions.
	for (int i = 0; i < this->number_of_trains; i++) {
		this->trains[i].station_policy = TSP_ENTERING_STATION;
		this->trains[i].time_left_waiting = test_mode ? TRAIN_DEPARTURE_INTERVAL_TESTING : this->max_idle_duration;
	}
}

/**
 * A train of this coaster crashed.
 * @param t1 The train that crashed.
 * @param t2 The train with which the first train collided. May be \c nullptr if no second train is involved.
 */
void CoasterInstance::Crash(CoasterTrain *t1, CoasterTrain *t2)
{
	int number_dead = 0;
	for (CoasterCar &car : t1->cars) {
		for (Guest *g : car.guests) {
			if (g != nullptr) {
				g->DeActivate(OAR_DEACTIVATE);
				number_dead++;
			}
		}
	}
	if (t2 != nullptr) {
		for (CoasterCar &car : t2->cars) {
			for (Guest *g : car.guests) {
				if (g != nullptr) {
					g->DeActivate(OAR_DEACTIVATE);
					number_dead++;
				}
			}
		}
	}

	if (number_dead > 0) {
		_inbox.SendMessage(new Message(GUI_MESSAGE_CRASH_WITH_DEAD, this->GetIndex(), number_dead));
	} else {
		_inbox.SendMessage(new Message(GUI_MESSAGE_CRASH_NO_DEAD, this->GetIndex()));
	}
	this->CloseRide();
	this->BreakDown();
	/* \todo Display animation of a big ball of fire. */
	/* \todo Decrease ride excitement rating and park rating. */
	ShowCoasterManagementGui(this);
}

bool CoasterInstance::CanOpenRide() const
{
	return !this->stations.empty() && !this->NeedsEntrance() && !this->NeedsExit() && RideInstance::CanOpenRide();
}

/**
 * Check whether the coaster does not have enough entrances yet.
 * @return At least one entrance is lacking.
 */
bool CoasterInstance::NeedsEntrance() const
{
	for (const CoasterStation &s : this->stations) {
		if (s.entrance == XYZPoint16::invalid()) return true;
	}
	return false;
}

/**
 * Check whether the coaster does not have enough exits yet.
 * @return At least one exit is lacking.
 */
bool CoasterInstance::NeedsExit() const
{
	for (const CoasterStation &s : this->stations) {
		if (s.exit == XYZPoint16::invalid()) return true;
	}
	return false;
}

bool CoasterInstance::IsEntranceLocation(const XYZPoint16& pos) const
{
	if (pos == this->temp_entrance_pos) return true;
	for (const CoasterStation &s : this->stations) {
		if (s.entrance == pos) return true;
	}
	return false;
}
bool CoasterInstance::IsExitLocation(const XYZPoint16& pos) const
{
	if (pos == this->temp_exit_pos) return true;
	for (const CoasterStation &s : this->stations) {
		if (s.exit == pos) return true;
	}
	return false;
}

/**
 * Get the rotation of an entrance or exit placed at the given location.
 * @param vox The absolute coordinates.
 * @param station The station to which the entrance/exit belongs (may be \c nullptr if any station will do).
 * @return The rotation.
 */
int CoasterInstance::EntranceExitRotation(const XYZPoint16& vox, const CoasterStation *station) const
{
	if (station != nullptr) {
		switch (station->direction) {
			case EDGE_NE:
			case EDGE_SW:
				return (vox.y < station->locations[0].y) ? EDGE_NW : EDGE_SE;
			case EDGE_NW:
			case EDGE_SE:
				return (vox.x < station->locations[0].x) ? EDGE_NE : EDGE_SW;
			default:
				NOT_REACHED();
		}
	}

	/* If the position belongs to a placed entrance or exit, prefer that one. */
	for (const CoasterStation &s : this->stations) {
		if (s.entrance == vox || s.exit == vox) return this->EntranceExitRotation(vox, &s);
	}
	/* It is a temporary location. Any station adjacent to the location will do. */
	for (const CoasterStation &s : this->stations) {
		if (this->CanPlaceEntranceOrExit(vox, true, nullptr)) return this->EntranceExitRotation(vox, &s);
	}
	NOT_REACHED();
}

/**
 * Check if a given station instance matches with an existing station.
 * If this is the case, the existing station's attributes are copied to the new station;
 * otherwise the new station is unchanged.
 * @param current_station Station to initialize.
 */
void CoasterInstance::InitializeStation(CoasterStation &current_station) const
{
	bool entrance_found = false;
	bool exit_found = false;
	for (const CoasterStation &old : this->stations) {
		if (!entrance_found && old.entrance != XYZPoint16::invalid()) {
			for (const XYZPoint16 &p : old.locations) {
				if (std::abs(p.x - old.entrance.x) + std::abs(p.y - old.entrance.y) != 1) continue;
				if (std::find(current_station.locations.begin(), current_station.locations.end(), p) != current_station.locations.end()) {
					entrance_found = true;
					current_station.entrance = old.entrance;
					break;
				}
			}
		}
		if (!exit_found && old.exit != XYZPoint16::invalid()) {
			for (const XYZPoint16 &p : old.locations) {
				if (std::abs(p.x - old.exit.x) + std::abs(p.y - old.exit.y) != 1) continue;
				if (std::find(current_station.locations.begin(), current_station.locations.end(), p) != current_station.locations.end()) {
					exit_found = true;
					current_station.exit = old.exit;
					break;
				}
			}
		}
		if (entrance_found && exit_found) break;
	}
}

/** Reinitialize the station information. This needs to be done after station pieces were added or deleted. */
void CoasterInstance::UpdateStations()
{
	this->RemoveStationsFromWorld();
	const int start_piece = GetFirstPlacedTrackPiece();
	if (start_piece < 0) {
		this->stations.clear();
		return;
	}

	std::vector<CoasterStation> result;
	std::unique_ptr<CoasterStation> current_station;
	for (int p = start_piece;;) {
		const PositionedTrackPiece &piece = this->pieces[p];
		if (piece.piece->IsStartingPiece()) {
			/* This code assumes that all station pieces are flat and straight tiles of size 1×1. */
			if (current_station.get() == nullptr) {
				current_station.reset(new CoasterStation);
				current_station->back_position = piece.distance_base;
			}
			current_station->direction = piece.piece->GetStartDirection();
			current_station->length += 256;
			for (const auto &track : piece.piece->track_voxels) {
				current_station->locations.emplace_back(piece.base_voxel + track->dxyz);
			}
		} else if (current_station.get() != nullptr) {
			this->InitializeStation(*current_station);
			result.emplace_back(*current_station);
			current_station.reset();
		}
		p = FindSuccessorPiece(this->pieces[p]);
		if (p < 0 || p == start_piece) break;
	}

	if (current_station != nullptr) {
		this->InitializeStation(*current_station);
		if (!result.empty() && result[0].back_position == 0) {
			/* Merge the station at the end of the track with the one at the beginning of the track. */
			assert(result[0].direction == current_station->direction);
			result[0].back_position = current_station->back_position;
			result[0].length += current_station->length;
			if (result[0].entrance == XYZPoint16::invalid()) result[0].entrance = current_station->entrance;
			if (result[0].exit     == XYZPoint16::invalid()) result[0].exit     = current_station->exit;
			result[0].locations.insert(result[0].locations.begin(), current_station->locations.begin(), current_station->locations.end());
		} else {
			result.emplace_back(*current_station);
		}
	}

	this->stations = result;
	this->InsertStationsIntoWorld();
}

/**
 * Check whether a position along the track lies within the given station.
 * @param pos Position to check (in 1/256 pixels).
 * @param s Station to check.
 * @return The point lies in the station.
 */
bool CoasterInstance::IsInStation(uint32 pos, const CoasterStation &s) const
{
	if (pos >= s.back_position && pos < s.back_position + 256 * s.length) return true;
	if (s.back_position + 256 * s.length > this->coaster_length) {
		/* The station wraps around the beginning of the track. */
		if (pos < s.back_position + 256 * s.length - this->coaster_length) return true;
	}
	return false;
}

/**
 * Calculate the forward-travelling distance between two track positions.
 * @param pos Position to reach (in 1/256 pixels).
 * @param offset Position to start from (in 1/256 pixels).
 * @return The forward-travelling distance (in 1/256 pixels).
 */
uint32 CoasterInstance::PositionRelativeTo(uint32 pos, const uint32 offset) const
{
	while (pos < offset) pos += this->coaster_length;
	return pos - offset;
}

/**
 * Check whether an entrance or exit can be placed at the given location.
 * @param pos Absolute voxel in the world.
 * @param entrance Whether we're placing an entrance or exit.
 * @param station The station for which the entrance/exit is intended (may be \c nullptr if any station will do).
 * @return The entrance or exit can be placed.
 */
bool CoasterInstance::CanPlaceEntranceOrExit(const XYZPoint16 &pos, const bool entrance, const CoasterStation *station) const
{
	if (!IsVoxelstackInsideWorld(pos.x, pos.y) || _world.GetTileOwner(pos.x, pos.y) != OWN_PARK) return false;
	if (station == nullptr) {
		for (const CoasterStation &s : this->stations) {
			if (this->CanPlaceEntranceOrExit(pos, entrance, &s)) return true;
		}
		return false;
	}

	if (station->locations.empty() || station->direction == INVALID_EDGE) return false;
	switch (station->direction) {
		case EDGE_NE:
		case EDGE_SW: {
			if (std::abs(pos.y - station->locations[0].y) != 1) return false;  // Not directly adjacent.
			short min_x = station->locations[0].x;
			short max_x = min_x;
			short min_z = station->locations[0].z;
			for (const XYZPoint16 &p : station->locations) {
				min_x = std::min(min_x, p.x);
				max_x = std::max(max_x, p.x);
				min_z = std::min(min_z, p.z);
			}
			if (pos.x < min_x || pos.x > max_x || pos.z != min_z) return false;  // Out of bounds.
			break;
		}
		case EDGE_NW:
		case EDGE_SE: {
			if (std::abs(pos.x - station->locations[0].x) != 1) return false;  // Not directly adjacent.
			short min_y = station->locations[0].y;
			short max_y = min_y;
			short min_z = station->locations[0].z;
			for (const XYZPoint16 &p : station->locations) {
				min_y = std::min(min_y, p.y);
				max_y = std::max(max_y, p.y);
				min_z = std::min(min_z, p.z);
			}
			if (pos.y < min_y || pos.y > max_y || pos.z != min_z) return false;  // Out of bounds.
			break;
		}
		default: NOT_REACHED();
	}

	/* Is there enough vertical space available? */
	const int8 height = entrance ? RideEntranceExitType::entrance_height : RideEntranceExitType::exit_height;
	for (int16 h = 0; h < height; ++h) {
		Voxel *v = _world.GetCreateVoxel(pos + XYZPoint16(0, 0, h), false);
		if (v == nullptr) continue;

		if (h > 0 && v->GetGroundType() != GTP_INVALID) return false;
		if (!v->CanPlaceInstance() || v->GetGroundSlope() != SL_FLAT) return false;
	}
	return true;
}

/**
 * Place an entrance or exit at the given location.
 * @param pos Absolute voxel in the world.
 * @param entrance Whether we're placing an entrance or exit.
 * @param station The station for which the entrance/exit is intended (may be \c nullptr if any station will do).
 * @return The entrance or exit was placed.
 */
bool CoasterInstance::PlaceEntranceOrExit(const XYZPoint16 &pos, const bool entrance, CoasterStation *station)
{
	if (station == nullptr) {
		for (CoasterStation &s : this->stations) {
			if (this->PlaceEntranceOrExit(pos, entrance, &s)) return true;
		}
		return false;
	}

	if (!this->CanPlaceEntranceOrExit(pos, entrance, station)) return false;
	this->RemoveStationsFromWorld();
	if (entrance) {
		station->entrance = pos;
	} else {
		station->exit = pos;
	}
	this->InsertStationsIntoWorld();
	return true;
}

/**
 * Update the intensity statistics with a piece of new information.
 * @param point Position along the coaster.
 * @param valid Whether to use this statistics point for calculating the ride ratings.
 * @param speed Current speed of a train.
 * @param vg Current vertical G force.
 * @param hg Current horizontal G force.
 */
void CoasterInstance::SampleStatistics(uint32 point, const bool valid, const int32 speed, const int32 vg, const int32 hg)
{
	point /= COASTER_INTENSITY_STATISTICS_SAMPLING_PRECISION;
	auto it = this->intensity_statistics.find(point);
	if (it == this->intensity_statistics.end()) {
		this->intensity_statistics.emplace(std::make_pair(point, CoasterIntensityStatistics{valid, 1, speed, vg, hg}));
	} else {
		it->second.valid &= valid;
		it->second.speed        = (it->second.precision * it->second.speed        + speed) / (it->second.precision + 1);
		it->second.vertical_g   = (it->second.precision * it->second.vertical_g   + vg   ) / (it->second.precision + 1);
		it->second.horizontal_g = (it->second.precision * it->second.horizontal_g + hg   ) / (it->second.precision + 1);
		it->second.precision++;
	}
}

void CoasterInstance::RecalculateRatings()
{
	uint64 exc = 100;  // Excitement rating in percent.
	uint64 iny = 100;  // Intensity rating in percent.
	uint64 nau = 100;  // Nausea rating in percent.
	uint32 statpoints = 0;
	for (const auto &pair : this->intensity_statistics) {
		if (!pair.second.valid) continue;
		exc += std::abs(pair.second.speed);
		iny += std::abs(pair.second.speed);
		iny += std::abs(pair.second.horizontal_g * pair.second.speed);
		iny += std::abs(pair.second.vertical_g   * pair.second.speed);
		nau += std::abs(pair.second.vertical_g   * pair.second.speed);
		statpoints++;
	}
	if (statpoints == 0) {
		this->excitement_rating = RATING_NOT_YET_CALCULATED;
		this->intensity_rating = RATING_NOT_YET_CALCULATED;
		this->nausea_rating = RATING_NOT_YET_CALCULATED;
		return;
	}

	iny /= statpoints;
	nau /= statpoints;
	exc /= statpoints;

	std::set<XYZPoint16> considered_locations;
	const uint16 index = this->GetIndex();
	const int start_piece = GetFirstPlacedTrackPiece();
	for (int p = start_piece;;) {
		for (int dx = -2; dx <= 2; dx++) {
			for (int dy = -2; dy <= 2; dy++) {
				if (!IsVoxelstackInsideWorld(this->pieces[p].base_voxel.x + dx, this->pieces[p].base_voxel.y + dy)) continue;
				for (int dh = -4; dh <= 2; dh++) {
					XYZPoint16 pos(dx, dy, dh);
					pos += this->pieces[p].base_voxel;

					if (considered_locations.count(pos)) continue;
					considered_locations.insert(pos);

					Voxel *voxel = _world.GetCreateVoxel(pos, false);
					if (voxel == nullptr) continue;

					if (IsImplodedSteepSlope(voxel->GetGroundSlope()))                 exc += 2;
					if (voxel->instance == SRI_SCENERY)                                exc += 4;
					if (voxel->instance >= SRI_FULL_RIDES && voxel->instance != index) exc += 7;
					/* \todo Also give a bonus for accurately mowed lawns and building near water. */
				}
			}
		}
		p = FindSuccessorPiece(this->pieces[p]);
		if (p < 0 || p == start_piece) break;
	}

	exc -= std::min(exc / 2, nau);
	exc -= std::min(exc / 2, iny);

	this->intensity_rating  = iny;
	this->nausea_rating     = nau;
	this->excitement_rating = exc;
}

void CoasterInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("csti");
	if (version != CURRENT_VERSION_CoasterInstance) ldr.VersionMismatch(version, CURRENT_VERSION_CoasterInstance);
	this->RideInstance::Load(ldr);

	this->capacity = (int)ldr.GetLong();
	this->coaster_length = ldr.GetLong();

	const CoasterType *ct = this->GetCoasterType();

	uint saved_pieces = ldr.GetWord();
	if (saved_pieces == 0) {
		throw LoadingError("Invalid number of track pieces (%u).", saved_pieces);
	}

	for (uint i = 0; i < saved_pieces; i++) {
		uint32 index = ldr.GetLong();
		ConstTrackPiecePtr piece = ct->pieces[index];

		if (piece != nullptr) {
			this->pieces[i].piece = piece;
			this->pieces[i].Load(ldr);
			this->PlaceTrackPieceInWorld(this->pieces[i]);
		} else {
			throw LoadingError("Invalid track piece.");
		}
	}

	this->number_of_trains = ldr.GetWord();
	this->cars_per_train = ldr.GetWord();
	this->SetNumberOfTrains(number_of_trains);
	this->SetNumberOfCars(cars_per_train);
	for (int i = 0; i < this->number_of_trains; i++) {
		this->trains[i].Load(ldr);
	}

	this->max_idle_duration = ldr.GetLong();
	this->min_idle_duration = ldr.GetLong();
	const int nr_stations = ldr.GetWord();
	this->stations.resize(nr_stations);
	for (int i = 0; i < nr_stations; ++i) {
		this->stations[i].entrance.x = ldr.GetWord();
		this->stations[i].entrance.y = ldr.GetWord();
		this->stations[i].entrance.z = ldr.GetWord();
		this->stations[i].exit.x = ldr.GetWord();
		this->stations[i].exit.y = ldr.GetWord();
		this->stations[i].exit.z = ldr.GetWord();
		this->stations[i].direction = static_cast<TileEdge>(ldr.GetByte());
		this->stations[i].length = ldr.GetLong();
		this->stations[i].back_position = ldr.GetLong();
		const int nr_locations = ldr.GetWord();
		this->stations[i].locations.resize(nr_locations);
		for (int j = 0; j < nr_locations; ++j) {
			this->stations[i].locations[j].x = ldr.GetWord();
			this->stations[i].locations[j].y = ldr.GetWord();
			this->stations[i].locations[j].z = ldr.GetWord();
		}
	}

	this->intensity_statistics.clear();
	for (long i = ldr.GetLong(); i > 0; i--) {
		const uint32 point = ldr.GetLong();
		const bool valid = ldr.GetByte() != 0;
		const int32 precision = ldr.GetLong();
		const int32 speed = ldr.GetLong();
		const int32 vg = ldr.GetLong();
		const int32 hg = ldr.GetLong();
		this->intensity_statistics.emplace(std::make_pair(point, CoasterIntensityStatistics{valid, precision, speed, vg, hg}));
	}

	this->InsertStationsIntoWorld();
	ldr.ClosePattern();
}

void CoasterInstance::Save(Saver &svr)
{
	svr.StartPattern("csti", CURRENT_VERSION_CoasterInstance);
	this->RideInstance::Save(svr);

	svr.PutLong((uint32)this->capacity);
	svr.PutLong(this->coaster_length);

	int count = 0;
	for (int i = 0; i < this->capacity; i++) {
		if (this->pieces[i].piece != nullptr) {
			count++;
		}
	}
	svr.PutWord(count);

	const CoasterType *ct = this->GetCoasterType();

	for (int i = 0; i < this->capacity; i++) {
		if (this->pieces[i].piece != nullptr) {
			for (uint j = 0; j < ct->pieces.size(); j++) {
				if (this->pieces[i].piece == ct->pieces[j]) {
					svr.PutLong(j);
					this->pieces[i].Save(svr);
					break;
				}
			}
		}
	}

	svr.PutWord(this->number_of_trains);
	svr.PutWord(this->cars_per_train);
	for (int i = 0; i < this->number_of_trains; i++) {
		this->trains[i].Save(svr);
	}

	svr.PutLong(this->max_idle_duration);
	svr.PutLong(this->min_idle_duration);
	svr.PutWord(this->stations.size());
	for (const CoasterStation &s : this->stations) {
		svr.PutWord(s.entrance.x);
		svr.PutWord(s.entrance.y);
		svr.PutWord(s.entrance.z);
		svr.PutWord(s.exit.x);
		svr.PutWord(s.exit.y);
		svr.PutWord(s.exit.z);
		svr.PutByte(s.direction);
		svr.PutLong(s.length);
		svr.PutLong(s.back_position);
		svr.PutWord(s.locations.size());
		for (const XYZPoint16 &p : s.locations) {
			svr.PutWord(p.x);
			svr.PutWord(p.y);
			svr.PutWord(p.z);
		}
	}

	svr.PutLong(this->intensity_statistics.size());
	for (const auto &pair : this->intensity_statistics) {
		svr.PutLong(pair.first);
		svr.PutByte(pair.second.valid ? 1 : 0);
		svr.PutLong(pair.second.precision);
		svr.PutLong(pair.second.speed);
		svr.PutLong(pair.second.vertical_g);
		svr.PutLong(pair.second.horizontal_g);
	}
	svr.EndPattern();
}
