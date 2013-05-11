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

TrackVoxel::TrackVoxel()
{
	for (uint i = 0; i < lengthof(this->back); i++) this->back[i] = NULL;
	for (uint i = 0; i < lengthof(this->front); i++) this->front[i] = NULL;
}

TrackVoxel::~TrackVoxel()
{
	/* Images are deleted by the Rcd manager. */
}

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

TrackPiece::TrackPiece() : RefCounter()
{
	this->voxel_count = 0;
	this->track_voxels = NULL;
}

TrackPiece::~TrackPiece()
{
	delete[] this->track_voxels;
}

bool TrackPiece::Load(RcdFile *rcd_file, uint32 length, const ImageMap &sprites)
{
	if (length < 2+3+1+1+4+2) return false;
	length -= 2+3+1+1+4+2;
	this->entry_connect = rcd_file->GetUInt8();
	this->exit_connect = rcd_file->GetUInt8();
	this->exit_dx = rcd_file->GetInt8();
	this->exit_dy = rcd_file->GetInt8();
	this->exit_dz = rcd_file->GetInt8();
	this->speed = rcd_file->GetInt8();
	this->track_flags = rcd_file->GetUInt8();
	this->cost = rcd_file->GetUInt32();
	this->voxel_count = rcd_file->GetUInt16();
	if (length != 36u * this->voxel_count) return false;
	this->track_voxels = new TrackVoxel[this->voxel_count];
	for (int i = 0; i < this->voxel_count; i++) {
		if (!this->track_voxels[i].Load(rcd_file, 36, sprites)) return false;
	}
	return true;
}
