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
#include "palette.h"

class ImageData;

/** Clipped rectangle. */
class ClippedRectangle {
public:
	ClippedRectangle();
	ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h);
	ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h);

	ClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle &operator=(const ClippedRectangle &cr);

	void ValidateAddress();

	uint16 absx;     ///< Absolute X position in the screen of the top-left.
	uint16 absy;     ///< Absolute Y position in the screen of the top-left.
	uint16 width;    ///< Number of columns.
	uint16 height;   ///< Number of rows.

	uint32 *address; ///< Base address. @note Call #ValidateAddress prior to use.
	int32 pitch;     ///< Pitch of a row in bytes. @note Call #ValidateAddress prior to use.
};

/** How to align text during drawing. */
enum Alignment {
	ALG_LEFT,   ///< Align to the left edge.
	ALG_CENTER, ///< Centre the text.
	ALG_RIGHT,  ///< Align to the right edge.
};

/**
 * Class representing the video system.
 */
class VideoSystem {
	friend class ClippedRectangle;

public:
	VideoSystem();
	~VideoSystem();

	bool Initialize(const char *font_name, int font_size);
	void Shutdown();

	/**
	 * Get horizontal size of the screen.
	 * @return Number of pixels of the screen horizontally.
	 */
	uint16 GetXSize() const
	{
		assert(this->initialized);
		return this->vid_width;
	}

	/**
	 * Get vertical size of the screen.
	 * @return Number of pixels of the screen vertically.
	 */
	uint16 GetYSize() const
	{
		assert(this->initialized);
		return this->vid_height;
	}

	/**
	 * Query whether the display needs to be repainted.
	 * @return Display needs an update.
	 */
	inline bool DisplayNeedsRepaint()
	{
		return this->dirty;
	}

	void MarkDisplayDirty();
	void MarkDisplayDirty(const Rectangle32 &rect);

	void SetClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle GetClippedRectangle();

	void FillSurface(uint32 colour, const Rectangle32 &rect);
	void BlitImage(const Point32 &img_base, const ImageData *spr, const Recolouring &recolour, GradientShift shift);
	void BlitImage(int x, int y, const ImageData *img, const Recolouring &recolour, GradientShift shift);

	/**
	 * Blit a row of sprites.
	 * @param xmin Start X position.
	 * @param numx Number of sprites to draw.
	 * @param y Y position.
	 * @param spr Sprite to draw.
	 * @param recolour Sprite recolouring definition.
	 */
	inline void BlitHorizontal(int32 xmin, uint16 numx, int32 y, const ImageData *spr, const Recolouring &recolour)
	{
		this->BlitImages(xmin, y, spr, numx, 1, recolour);
	}

	/**
	 * Blit a column of sprites.
	 * @param ymin Start Y position.
	 * @param numy Number of sprites to draw.
	 * @param x X position.
	 * @param spr Sprite to draw.
	 * @param recolour Sprite recolouring definition.
	 */
	inline void BlitVertical(int32 ymin, uint16 numy, int32 x, const ImageData *spr, const Recolouring &recolour)
	{
		this->BlitImages(x, ymin, spr, 1, numy, recolour);
	}

	void BlitImages(int32 x_base, int32 y_base, const ImageData *spr, uint16 numx, uint16 numy, const Recolouring &recolour);

	void FinishRepaint();

	/**
	 * Get the height of a line of text.
	 * @return Height of the font.
	 */
	int GetTextHeight() const
	{
		return this->font_height;
	}

	void GetTextSize(const uint8 *text, int *width, int *height);
	void GetNumberRangeSize(int64 smallest, int64 biggest, int *width, int *height);
	void BlitText(const uint8 *text, uint32 colour, int xpos, int ypos, int width = 0x7FFF, Alignment align = ALG_LEFT);
	void DrawLine(const Point16 &start, const Point16 &end, uint32 colour);
	void DrawRectangle(const Rectangle32 &rect, uint32 colour);

	bool missing_sprites; ///< Indicates that some sprites cannot be drawn.

private:
	int vid_width;    ///< Width of the application window.
	int vid_height;   ///< Height of the application window.
	int font_height;  ///< Height of a line of text in pixels.
	bool initialized; ///< Video system is initialized.
	bool dirty;       ///< Video display needs being repainted.

	TTF_Font *font;             ///< Opened text font.
	SDL_Window *window;         ///< %Window of the application.
	SDL_Renderer *renderer;     ///< GPU renderer to the application window.
	SDL_Texture *texture;       ///< GPU Texture storage of the application window.
	uint32 *mem;                ///< Memory used for blitting the application display.
	ClippedRectangle blit_rect; ///< %Rectangle to blit in.
	Point16 digit_size;         ///< Size of largest digit (initially a zero-size).

	void MarkDisplayClean();
};

extern VideoSystem _video;
extern const uint32 _icon_data[32][32];

#endif
