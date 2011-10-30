/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.h Video handling. */

#ifndef VIDEO_H
#define VIDEO_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "geometry.h"

class Sprite;
class PaletteData;

/**
 * Class representing the video system.
 */
class VideoSystem {
public:
	VideoSystem();
	~VideoSystem();

	bool Initialize(const char *font_name, int font_size);
	void SetPalette();
	void Shutdown();

	uint16 GetXSize() const;
	uint16 GetYSize() const;

	/**
	 * Query whether the display needs to be repainted.
	 * @return Display needs an update.
	 */
	FORCEINLINE bool DisplayNeedsRepaint()
	{
		return this->dirty;
	}

	/** Mark the display as being out of date (it needs the be repainted). */
	FORCEINLINE void MarkDisplayDirty()
	{
		this->dirty = true;
	}

	void LockSurface();
	void UnlockSurface();
	void FillSurface(uint8 colour);
	void BlitImage(const Point &img_base, const Sprite *spr, const Rectangle & rect);

private:
	bool initialized;   ///< Video system is initialized.
	bool dirty;         ///< Video display needs being repainted.

	TTF_Font *font;     ///< Opened text font.
	SDL_Surface *video; ///< Video surface.
};

#endif
