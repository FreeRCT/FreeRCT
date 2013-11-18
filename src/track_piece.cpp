/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file track_piece.cpp Functions of the track pieces. */

#include "stdafx.h"
#include "sprite_store.h"
#include "fileio.h"
#include "track_piece.h"
#include "map.h"

TrackVoxel::TrackVoxel()
{
	for (uint i = 0; i < lengthof(this->back); i++) this->back[i] = nullptr;
	for (uint i = 0; i < lengthof(this->front); i++) this->front[i] = nullptr;
}

TrackVoxel::~TrackVoxel()
{
	/* Images are deleted by the RCD manager. */
}

/**
 * Load a track voxel.
 * @param rcd_file Data file being loaded.
 * @param length Length of the voxel (according to the file).
 * @param sprites Already loaded sprite blocks.
 * @return Loading was successful.
 */
bool TrackVoxel::Load(RcdFile *rcd_file, size_t length, const ImageMap &sprites)
{
	if (length != 4*4 + 4*4 + 3 + 1) return false;
	for (uint i = 0; i < 4; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->back[i])) return false;
	}
	for (uint i = 0; i < 4; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->front[i])) return false;
	}
	this->dx = rcd_file->GetInt8();
	this->dy = rcd_file->GetInt8();
	this->dz = rcd_file->GetInt8();
	this->space = rcd_file->GetUInt8();
	return true;
}

TrackPiece::TrackPiece()
{
	this->voxel_count = 0;
	this->track_voxels = nullptr;
}

TrackPiece::~TrackPiece()
{
	delete[] this->track_voxels;
}

/**
 * Load a track piece.
 * @param rcd_file Data file being loaded.
 * @param length Length of the voxel (according to the file).
 * @param sprites Already loaded sprite blocks.
 * @return Loading was successful.
 */
bool TrackPiece::Load(RcdFile *rcd_file, uint32 length, const ImageMap &sprites)
{
	if (length < 2+3+1+2+4+2) return false;
	length -= 2+3+1+2+4+2;
	this->entry_connect = rcd_file->GetUInt8();
	this->exit_connect = rcd_file->GetUInt8();
	this->exit_dx = rcd_file->GetInt8();
	this->exit_dy = rcd_file->GetInt8();
	this->exit_dz = rcd_file->GetInt8();
	this->speed = rcd_file->GetInt8();
	this->track_flags = rcd_file->GetUInt16();
	this->cost = rcd_file->GetUInt32();
	this->voxel_count = rcd_file->GetUInt16();
	if (length != 36u * this->voxel_count) return false;
	this->track_voxels = new TrackVoxel[this->voxel_count];
	for (int i = 0; i < this->voxel_count; i++) {
		if (!this->track_voxels[i].Load(rcd_file, 36, sprites)) return false;
	}
	return true;
}

/** Default constructor. */
PositionedTrackPiece::PositionedTrackPiece()
{
	this->piece = nullptr;
}

/**
 * Constructor taking values for all its fields.
 * @param xpos X position of the positioned track piece.
 * @param ypos Y position of the positioned track piece.
 * @param zpos Z position of the positioned track piece.
 * @param piece Track piece to use.
 */
PositionedTrackPiece::PositionedTrackPiece(uint16 xpos, uint16 ypos, uint8 zpos, ConstTrackPiecePtr piece)
{
	this->x_base = xpos;
	this->y_base = ypos;
	this->z_base = zpos;
	this->piece = piece;
}

/**
 * Verify that all voxels of this track piece are within world boundaries.
 * @return The positioned track piece is entirely within the world boundaries.
 * @pre Positioned track piece must have a piece.
 */
bool PositionedTrackPiece::IsOnWorld() const
{
	assert(this->piece != nullptr);
	if (!IsVoxelInsideWorld(this->x_base, this->y_base, this->z_base)) return false;
	if (!IsVoxelInsideWorld(this->x_base + this->piece->exit_dx, this->y_base + this->piece->exit_dy,
			this->z_base + this->piece->exit_dz)) return false;
	const TrackVoxel *tvx = this->piece->track_voxels;
	for (int i = 0; i < this->piece->voxel_count; i++) {
		if (!IsVoxelInsideWorld(this->x_base + tvx->dx, this->y_base + tvx->dy, this->z_base + tvx->dz)) return false;
		tvx++;
	}
	return true;
}

/**
 * Can this positioned track piece be placed in the world?
 * @return Positioned track piece can be placed.
 * @pre Positioned track piece must have a piece.
 */
bool PositionedTrackPiece::CanBePlaced() const
{
	if (!this->IsOnWorld()) return false;
	const TrackVoxel *tvx = this->piece->track_voxels;
	for (int i = 0; i < this->piece->voxel_count; i++) {
		/* Is the voxel above ground level? */
		if (_world.GetGroundHeight(this->x_base + tvx->dx, this->y_base + tvx->dy) > this->z_base + tvx->dz) return false;
		const Voxel *vx = _world.GetVoxel(this->x_base + tvx->dx, this->y_base + tvx->dy, this->z_base + tvx->dz);
		if (vx != nullptr && !vx->CanPlaceInstance()) return false;
	}
	return true;
}

/**
 * Can this positioned track piece function as a successor for the given exit conditions?
 * @param x Required X position.
 * @param y Required Y position.
 * @param z Required Z position.
 * @param connect Required entry connection.
 * @return This positioned track piece can be used as successor.
 */
bool PositionedTrackPiece::CanBeSuccessor(uint16 x, uint16 y, uint8 z, uint8 connect) const
{
	return this->piece != nullptr && this->x_base == x && this->y_base == y && this->z_base == z && this->piece->entry_connect == connect;
}

/**
 * Can this positioned track piece function as a successor of piece \a pred?
 * @param pred Previous positioned track piece.
 * @return This positioned track piece can be used as successor.
 */
bool PositionedTrackPiece::CanBeSuccessor(const PositionedTrackPiece &pred) const
{
	if (pred.piece == nullptr) return false;
	return this->CanBeSuccessor(pred.GetEndX(), pred.GetEndY(), pred.GetEndZ(), pred.piece->exit_connect);
}
