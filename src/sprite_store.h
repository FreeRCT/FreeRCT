/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h Sprite storage data structure. */

#ifndef SPRITE_STORE_H
#define SPRITE_STORE_H

#include "orientation.h"

class Sprite;

/** Storage of all sprites. */
class SpriteStore {
public:
	SpriteStore();
	~SpriteStore();

	void Load();

	const Sprite *GetSurfaceSprite(uint type, uint8 slope, uint8 size, ViewOrientation orient);
};

extern SpriteStore _sprite_store;

#endif
