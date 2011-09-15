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

SpriteStore::SpriteStore()
{
}

SpriteStore::~SpriteStore()
{
}

void SpriteStore::Load()
{
}

const Sprite *SpriteStore::GetSurfaceSprite(uint type, uint slope)
{
	return NULL;
}

