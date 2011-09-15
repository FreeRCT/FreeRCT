/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h Sprite storage functions. */

#include "stdafx.h"
#include "sprite_store.h"

SpriteStore _sprite_store; ///< Sprite storage.

/** %Sprite storage constructor. */
SpriteStore::SpriteStore()
{
}

/** %Sprite storage destructor. */
SpriteStore::~SpriteStore()
{
}

/**
 * Load sprites from the disk.
 * @todo XXX Implement me!
 */
void SpriteStore::Load()
{
}

/**
 * Get a surface sprite.
 * @param type Type of surface.
 * @param slope Slope definition.
 * @param size Sprite size.
 * @param orient Orientation.
 * @return Requested sprite if available.
 * @todo XXX Implement me.
 */
const Sprite *SpriteStore::GetSurfaceSprite(uint type, uint8 slope, uint8 size, ViewOrientation orient)
{
	return NULL;
}

