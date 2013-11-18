/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster.cpp Coaster type data. */

#include "stdafx.h"
#include "sprite_store.h"
#include "coaster.h"
#include "fileio.h"
#include "memory.h"
#include "math_func.h"
#include "map.h"

#include "generated/coasters_strings.cpp"

CoasterType::CoasterType() : RideType(RTK_COASTER)
{
	this->voxel_count = 0;
	this->voxels = nullptr;
}

CoasterType::~CoasterType()
{
	delete[] this->voxels;
}

/**
 * Load a coaster type.
 * @param rcd_file Data file being loaded.
 * @param length Length of the voxel (according to the file).
 * @param texts Already loaded text blocks.
 * @param piece_map Already loaded track pieces.
 * @return Loading was successful.
 */
bool CoasterType::Load(RcdFile *rcd_file, uint32 length, const TextMap &texts, const TrackPiecesMap &piece_map)
{
	if (length < 2+1+4+2) return false;
	length -= 2+1+4+2;
	this->coaster_kind = rcd_file->GetUInt16();
	this->platform_type = rcd_file->GetUInt8();
	if (this->coaster_kind == 0 || this->coaster_kind >= CST_COUNT) return false;
	if (this->platform_type == 0 || this->platform_type >= CPT_COUNT) return false;

	TextData *text_data;
	if (!LoadTextFromFile(rcd_file, texts, &text_data)) return false;
	StringID base = _language.RegisterStrings(*text_data, _coasters_strings_table);
	this->SetupStrings(text_data, base, STR_GENERIC_COASTER_START, COASTERS_STRING_TABLE_END, COASTERS_NAME_TYPE, COASTERS_DESCRIPTION_TYPE);

	int piece_count = rcd_file->GetUInt16();
	if (length != 4u * piece_count) return false;

	this->pieces.resize(piece_count);
	for (auto &piece : this->pieces) {
		uint32 val = rcd_file->GetUInt32();
		if (val == 0) return false; // We don't expect missing track pieces (they should not be included at all).
		auto iter = piece_map.find(val);
		if (iter == piece_map.end()) return false;
		piece = (*iter).second;
	}
	/* Setup a track voxel list for fast access in the type. */
	this->voxel_count = 0;
	for (const auto &piece : this->pieces) {
		this->voxel_count += piece->voxel_count;
	}
	this->voxels = new const TrackVoxel*[this->voxel_count]();
	int vi = 0;
	for (const auto &piece : this->pieces) {
		for (int j = 0; j < piece->voxel_count; j++) {
			this->voxels[vi] = &piece->track_voxels[j];
			vi++;
		}
	}
	return true;
}

RideInstance *CoasterType::CreateInstance() const
{
	return new CoasterInstance(this);
}

const ImageData *CoasterType::GetView(uint8 orientation) const
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
	for (int i = 0; i < this->voxel_count; i++) {
		if (this->voxels[i] == tvx) return i;
	}
	NOT_REACHED();
}

/**
 * Constructor of a roller coaster instance.
 * @param ct Coaster type being built.
 */
CoasterInstance::CoasterInstance(const CoasterType *ct) : RideInstance(ct)
{
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) this->item_price[i] = ct->item_cost[i] * 2;
	this->pieces = new PositionedTrackPiece[MAX_PLACED_TRACK_PIECES]();
	this->capacity = MAX_PLACED_TRACK_PIECES;
}

CoasterInstance::~CoasterInstance()
{
	delete[] this->pieces;
}

/**
 * Can the user click in the world to re-open the coaster instance window for this coaster?
 * @return \c true if an object in the world exists, \c false otherwise.
 */
bool CoasterInstance::IsAccessible()
{
	return this->GetFirstPlacedTrackPiece() >= 0;
}

void CoasterInstance::GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
{
	const CoasterType *ct = this->GetCoasterType();
	assert(voxel_number < ct->voxel_count);
	const TrackVoxel *tv = ct->voxels[voxel_number];

	sprites[0] = nullptr; // SO_PLATFORM_BACK /// \todo If platform flag, add coaster platforms.
	sprites[1] = tv->back[orient]; // SO_RIDE
	sprites[2] = tv->front[orient]; // SO_RIDE_FRONT
	sprites[3] = nullptr; // SO_PLATFORM_FRONT /// \todo If platform flag, add coaster platforms.
}

uint8 CoasterInstance::GetEntranceDirections(uint16 xvox, uint16 yvox, uint8 zvox) const
{
	return 0; /// \todo add entrance bits for the coaster.
}

/**
 * Check the state of the coaster ride, and set the #state flag.
 * @return The new coaster instance state.
 */
RideInstanceState CoasterInstance::DecideRideState()
{
	bool modified;
	if (!MakePositionedPiecesLooping(&modified)) {
		this->state = RIS_BUILDING;
		return (RideInstanceState)this->state;
	}
	if (modified || (this->state != RIS_CLOSED && this->state != RIS_OPEN)) this->state = RIS_TESTING;
	return (RideInstanceState)this->state;
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

/**
 * Find the first placed track piece at a given position with a given entry connection.
 * @param x Required X position.
 * @param y Required Y position.
 * @param z Required Z position.
 * @param entry_connect Required entry connection.
 * @param start First index to search.
 * @param end End of the search (one beyond the last positioned track piece to search).
 * @return Index of the requested positioned track piece if it exists, else \c -1.
 */
int CoasterInstance::FindSuccessorPiece(uint16 x, uint16 y, uint8 z, uint8 entry_connect, int start, int end)
{
	if (start < 0) start = 0;
	if (end > MAX_PLACED_TRACK_PIECES) end = MAX_PLACED_TRACK_PIECES;
	for (; start < end; start++) {
		if (this->pieces[start].CanBeSuccessor(x, y, z, entry_connect)) return start;
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
	return this->FindSuccessorPiece(placed.GetEndX(), placed.GetEndY(), placed.GetEndZ(), placed.piece->exit_connect);
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
	if (modified != nullptr) *modified = false;

	/* First step, move all non-null track pieces to the start of the array. */
	int count = 0;
	for (int i = 0; i < this->capacity; i++) {
		PositionedTrackPiece *ptp = this->pieces + i;
		if (ptp->piece == nullptr) continue;
		if (i == count) {
			count++;
			continue;
		}
		Swap(this->pieces[count], *ptp);
		if (modified != nullptr) *modified = true;
		ptp->piece = nullptr;
		count++;
	}

	/* Second step, find a loop from start to end. */
	if (count < 2) return false; // 0 or 1 positioned pieces won't ever make a loop.

	PositionedTrackPiece *ptp = this->pieces;
	for (int i = 1; i < count; i++) {
		int j = this->FindSuccessorPiece(ptp->GetEndX(), ptp->GetEndY(), ptp->GetEndZ(), ptp->piece->exit_connect, i, count);
		if (j < 0) return false;
		ptp++; // Now points to pieces[i].
		if (i != j) {
			Swap(*ptp, this->pieces[j]); // Make piece 'j' the next positioned piece.
			if (modified != nullptr) *modified = true;
		}
	}
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
			return i;
		}
	}
	return -1;
}

/**
 * Add the positioned track piece to #_additions.
 * @param placed Track piece to place.
 * @pre placed->CanBePlaced() should hold.
 */
void CoasterInstance::PlaceTrackPieceInAdditions(const PositionedTrackPiece &placed)
{
	assert(placed.CanBePlaced());
	assert(this->GetIndex() >= SRI_FULL_RIDES && this->GetIndex() <= SRI_LAST);
	SmallRideInstance ride_number = (SmallRideInstance)this->GetIndex();

	const CoasterType *ct = this->GetCoasterType();
	const TrackVoxel *tvx = placed.piece->track_voxels;
	for (int i = 0; i < placed.piece->voxel_count; i++) {
		Voxel *vx = _additions.GetCreateVoxel(placed.x_base + tvx->dx, placed.y_base + tvx->dy, placed.z_base + tvx->dz, true);
		// assert(vx->CanPlaceInstance()): Checked by this->CanBePlaced().
		vx->SetInstance(ride_number);
		vx->SetInstanceData(ct->GetTrackVoxelIndex(tvx));
		tvx++;
	}
}
