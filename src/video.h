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

/** Clipped rectangle. */
class ClippedRectangle {
public:
	ClippedRectangle();
	ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h);
	ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h);
	ClippedRectangle(const Rectangle &rect);

	ClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle &operator=(const ClippedRectangle &cr);

	void ValidateAddress();

	uint16 absx;    ///< Absolute X position in the screen of the top-left.
	uint16 absy;    ///< Absolute Y position in the screen of the top-left.
	uint16 width;   ///< Number of columns.
	uint16 height;  ///< Number of rows.

	uint8 *address; ///< Base address. @note Call #ValidateAddress prior to use.
	uint16 pitch;   ///< Pitch of a row in bytes. @note Call #ValidateAddress prior to use.
};


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

	/** Mark the display as being up-to-date. */
	FORCEINLINE void MarkDisplayClean()
	{
		this->dirty = false;
	}

	void SetClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle GetClippedRectangle();

	void LockSurface();
	void UnlockSurface();
	void FillSurface(uint8 colour, const Rectangle &rect);
	void BlitImage(const Point &img_base, const Sprite *spr, const Rectangle & rect);

	void FinishRepaint();

private:
	bool initialized;   ///< Video system is initialized.
	bool dirty;         ///< Video display needs being repainted.

	TTF_Font *font;             ///< Opened text font.
	SDL_Surface *video;         ///< Video surface.
	ClippedRectangle blit_rect; ///< Rectangle to blit in.
};

#endif
