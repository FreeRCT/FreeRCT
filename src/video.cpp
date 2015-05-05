/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.cpp Video handling. */

#include "stdafx.h"
#include "video.h"
#include "sprite_data.h"
#include "palette.h"
#include "math_func.h"
#include "bitmath.h"
#include "rev.h"
#include "freerct.h"
#include "gamecontrol.h"
#include "window.h"
#include "viewport.h"
#include <string>

VideoSystem _video;  ///< Video sub-system.
static bool _finish; ///< Finish execution of the main loop (and program).

/** End the program. */
void QuitProgram()
{
	_finish = true;
}

/** Default constructor of a clipped rectangle. */
ClippedRectangle::ClippedRectangle()
{
	this->absx = 0;
	this->absy = 0;
	this->width = 0;
	this->height = 0;
	this->address = nullptr; this->pitch = 0;
}

/**
 * Construct a clipped rectangle from coordinates.
 * @param x Top-left x position.
 * @param y Top-left y position.
 * @param w Width.
 * @param h Height.
 */
ClippedRectangle::ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h)
{
	this->absx = x;
	this->absy = y;
	this->width = w;
	this->height = h;
	this->address = nullptr; this->pitch = 0;
}

/**
 * Construct a clipped rectangle inside an existing one.
 * @param cr Existing rectangle.
 * @param x Top-left x position.
 * @param y Top-left y position.
 * @param w Width.
 * @param h Height.
 * @note %Rectangle is clipped to the old one.
 */
ClippedRectangle::ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h)
{
	if (x >= cr.width || y >= cr.height) {
		this->absx = 0;
		this->absy = 0;
		this->width = 0;
		this->height = 0;
		this->address = nullptr; this->pitch = 0;
		return;
	}
	if (x + w > cr.width)  w = cr.width - x;
	if (y + h > cr.height) h = cr.height - y;

	this->absx = cr.absx + x;
	this->absy = cr.absy + y;
	this->width = w;
	this->height = h;
	this->address = nullptr; this->pitch = 0;
}

/**
 * Copy constructor.
 * @param cr Existing clipped rectangle.
 */
ClippedRectangle::ClippedRectangle(const ClippedRectangle &cr)
{
	this->absx = cr.absx;
	this->absy = cr.absy;
	this->width = cr.width;
	this->height = cr.height;
	this->address = cr.address; this->pitch = cr.pitch;
}

/**
 * Assignment operator override.
 * @param cr Existing clipped rectangle.
 * @return The assigned value.
 */
ClippedRectangle &ClippedRectangle::operator=(const ClippedRectangle &cr)
{
	if (this != &cr) {
		this->absx = cr.absx;
		this->absy = cr.absy;
		this->width = cr.width;
		this->height = cr.height;
		this->address = cr.address; this->pitch = cr.pitch;
	}
	return *this;
}

/** Initialize the #address if not done already. */
void ClippedRectangle::ValidateAddress()
{
	if (this->address == nullptr) {
		this->pitch = _video.GetXSize();
		this->address = _video.mem + this->absx + this->absy * this->pitch;
	}
}

/**
 * Default constructor, does nothing, never goes wrong.
 * Call #Initialize to initialize the system.
 */
VideoSystem::VideoSystem()
{
	this->initialized = false;
}

/** Destructor. */
VideoSystem::~VideoSystem()
{
	if (this->initialized) this->Shutdown();
}

/**
 * Initialize the video system, preparing it for use.
 * @param font_name Name of the font file to load.
 * @param font_size Size of the font.
 * @return Error message if initialization failed, else an empty text.
 */
std::string VideoSystem::Initialize(const char *font_name, int font_size)
{
	if (this->initialized) return "";

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::string err = "SDL video initialization failed: ";
		err += SDL_GetError();
		return err;
	}

	std::string caption = "FreeRCT ";
	caption += _freerct_revision;
	this->window = SDL_CreateWindow(caption.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
	if (this->window == nullptr) {
		std::string err = "SDL window creation failed: ";
		err += SDL_GetError();
		SDL_Quit();
		return err;
	}

	this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);
	if (this->renderer == nullptr) {
		std::string err = "SDL renderer creation failed: ";
		err += SDL_GetError();
		SDL_DestroyWindow(this->window);
		SDL_Quit();
		return err;
	}

	this->GetResolutions();

	this->SetResolution({800, 600}); // Allocates this->mem, return value is ignored.

	/* SDL_CreateRGBSurfaceFrom() pretends to use a void* for the data,
	 * but it's really treated as endian-specific uint32*.
	 * But since the c++ compiler handles the endianness of _icon_data, it's ok. */
	const uint32 rmask = 0xff000000;
	const uint32 gmask = 0x00ff0000;
	const uint32 bmask = 0x0000ff00;
	const uint32 amask = 0x000000ff;
	SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void *)_icon_data, 32, 32, 4 * 8, 4 * 32,
	                                             rmask, gmask, bmask, amask);

	if (icon != nullptr) {
		SDL_SetWindowIcon(this->window, icon);
		SDL_FreeSurface(icon);
	} else {
		printf("Could not set window icon (%s)\n", SDL_GetError());
	}

	SDL_StartTextInput(); // Enable Unicode character input.

	if (TTF_Init() != 0) {
		SDL_Quit();
		delete[] this->mem;
		std::string err = "TTF font initialization failed: ";
		err += TTF_GetError();
		return err;
	}

	this->font = TTF_OpenFont(font_name, font_size);
	if (this->font == nullptr) {
		std::string err = "TTF Opening font \"";
		err += font_name;
		err += "\" size ";
		err += std::to_string(font_size);
		err += " failed: ";
		err += TTF_GetError();
		TTF_Quit();
		SDL_Quit();
		delete[] this->mem;
		return err;
	}

	this->font_height = TTF_FontLineSkip(this->font);
	this->initialized = true;
	this->dirty = true; // Ensure it gets painted.
	this->missing_sprites = false;

	this->digit_size.x = 0;
	this->digit_size.y = 0;

	return "";
}


/**
 * Change the resolution of the game window, including
 * reinitialising some screen-size related data structures.
 * @param res Resolution to set the screen to.
 * @return True if resolution was changed.
 */
bool VideoSystem::SetResolution(const Point32 &res)
{
	if (this->initialized && this->GetXSize() == res.x && this->GetYSize() == res.y) return true;

	/* Destroy old window, if it exists. */
	if (this->initialized) {
		delete[] mem;
		this->mem = nullptr;
		SDL_DestroyTexture(this->texture);
		this->texture = nullptr;
	}

	this->vid_width = res.x;
	this->vid_height = res.y;
	SDL_SetWindowSize(this->window, this->vid_width, this->vid_height);

	this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, this->vid_width, this->vid_height);
	if (this->texture == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Could not create texture (%s)\n", SDL_GetError());
		return false;
	}

	this->mem = new uint32[this->vid_width * this->vid_height];
	if (this->mem == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Failed to obtain window display storage.\n");
		return false;
	}

	/* Update internal screen size data structures. */
	this->blit_rect = ClippedRectangle(0, 0, this->vid_width, this->vid_height);
	Viewport *vp = GetViewport();
	if (vp != nullptr) vp->SetSize(this->vid_width, this->vid_height);
	_window_manager.RepositionAllWindows();
	this->MarkDisplayDirty();
	return true;
}

/** Gets the available default resolutions that are supported by the graphics driver. */
void VideoSystem::GetResolutions()
{
	int num_modes = SDL_GetNumDisplayModes(0); // \todo Support multiple displays?
	SDL_DisplayMode mode;
	for (int i = 0; i < num_modes; i++) {
		if (SDL_GetDisplayMode(0, i, &mode) != 0) {
			fprintf(stderr, "Could not query display mode (%s)\n", SDL_GetError());
			continue;
		}
		this->resolutions.emplace(mode.w, mode.h);
	}
}

/** Mark the entire display as being out of date (it needs the be repainted). */
void VideoSystem::MarkDisplayDirty()
{
	this->dirty = true;
}

/**
 * Mark the stated area of the screen as being out of date.
 * @param rect %Rectangle which is out of date.
 * @todo Keep an administration of the rectangle(s??) to update, and update just that part.
 */
void VideoSystem::MarkDisplayDirty(const Rectangle32 &rect)
{
	this->dirty = true;
}

/** Mark the display as being up-to-date. */
void VideoSystem::MarkDisplayClean()
{
	this->dirty = false;
}

/**
 * Set the clipped area.
 * @param cr New clipped blitting area.
 */
void VideoSystem::SetClippedRectangle(const ClippedRectangle &cr)
{
	this->blit_rect = cr;
}

/**
 * Get the current clipped blitting area.
 * @return Current clipped area.
 */
ClippedRectangle VideoSystem::GetClippedRectangle()
{
	this->blit_rect.ValidateAddress();
	return this->blit_rect;
}

/**
 * Update the mouse position in the program.
 * @param x New x position of the mouse.
 * @param y New y position of the mouse.
 */
static void UpdateMousePosition(int16 x, int16 y)
{
	_window_manager.MouseMoveEvent({x, y});
}

/**
 * Process input from the keyboard.
 * @param key_code Kind of input.
 * @param symbol Entered symbol, if \a key_code is #WMKC_SYMBOL. Utf-8 encoded.
 * @return Game-ending event has happened.
 */
static bool HandleKeyInput(WmKeyCode key_code, const uint8 *symbol)
{
	if (_window_manager.KeyEvent(key_code, symbol)) return false;

	if (key_code == WMKC_CURSOR_LEFT) {
		GetViewport()->Rotate(-1);
	} else if (key_code == WMKC_CURSOR_RIGHT) {
		GetViewport()->Rotate(1);
	} else if (key_code == WMKC_SYMBOL) {
		if (symbol[0] == '1') {
			GetViewport()->ToggleUndergroundMode();
		} else if (symbol[0] == 'q') {
			QuitProgram();
			return true;
		}
	}

	return false;
}

/**
 * Handle an input event.
 * @return Game-ending event has happened.
 */
bool VideoSystem::HandleEvent()
{
	SDL_Event event;
	if (SDL_PollEvent(&event) != 1) return true;

	switch (event.type) {
		case SDL_TEXTINPUT:
			return HandleKeyInput(WMKC_SYMBOL, (const uint8 *)event.text.text);

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
				case SDLK_UP:        return HandleKeyInput(WMKC_CURSOR_UP, nullptr);
				case SDLK_LEFT:      return HandleKeyInput(WMKC_CURSOR_LEFT, nullptr);
				case SDLK_RIGHT:     return HandleKeyInput(WMKC_CURSOR_RIGHT, nullptr);
				case SDLK_DOWN:      return HandleKeyInput(WMKC_CURSOR_DOWN, nullptr);
				case SDLK_ESCAPE:    return HandleKeyInput(WMKC_CANCEL, nullptr);
				case SDLK_DELETE:    return HandleKeyInput(WMKC_DELETE, nullptr);
				case SDLK_BACKSPACE: return HandleKeyInput(WMKC_BACKSPACE, nullptr);

				case SDLK_RETURN:
				case SDLK_RETURN2:
				case SDLK_KP_ENTER:  return HandleKeyInput(WMKC_CONFIRM, nullptr);

				default:             return false;
			}

		case SDL_MOUSEMOTION:
			UpdateMousePosition(event.button.x, event.button.y);
			return false;

		case SDL_MOUSEWHEEL:
			_window_manager.MouseWheelEvent((event.wheel.y > 0) ? 1 : -1);
			return false;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					UpdateMousePosition(event.button.x, event.button.y);
					_window_manager.MouseButtonEvent(MB_LEFT, (event.type == SDL_MOUSEBUTTONDOWN));
					break;

				case SDL_BUTTON_MIDDLE:
					UpdateMousePosition(event.button.x, event.button.y);
					_window_manager.MouseButtonEvent(MB_MIDDLE, (event.type == SDL_MOUSEBUTTONDOWN));
					break;

				case SDL_BUTTON_RIGHT:
					UpdateMousePosition(event.button.x, event.button.y);
					_window_manager.MouseButtonEvent(MB_RIGHT, (event.type == SDL_MOUSEBUTTONDOWN));
					break;

				default:
					break;
			}
			return false;

		case SDL_USEREVENT:
			return true;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				this->SetResolution({event.window.data1, event.window.data2});
			}
			if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
				_video.MarkDisplayDirty();
				_window_manager.UpdateWindows();
			}
			return false;

		case SDL_QUIT:
			QuitProgram();
			return true;

		default:
			return false; // Ignore other events.
	}
}

/** Main loop. Loops until told not to. */
void VideoSystem::MainLoop()
{
	static const uint32 FRAME_DELAY = 30; // Number of milliseconds between two frames.
	bool missing_sprites_check = false;
	_finish = false;

	while (!_finish) {
		uint32 start = SDL_GetTicks();

		OnNewFrame(FRAME_DELAY);

		/* Handle input events until time for the next frame has arrived. */
		for (;;) {
			if (HandleEvent()) break;
		}
		if (_finish) break;

		uint32 now = SDL_GetTicks();
		if (now >= start) { // No wrap around.
			now -= start;
			if (now < FRAME_DELAY) SDL_Delay(FRAME_DELAY - now); // Too early, wait until next frame.
		}

		if (!missing_sprites_check && this->missing_sprites) {
			ShowGraphicsErrorMessage();
			missing_sprites_check = true;
		}
	}
}

/** Close down the video system. */
void VideoSystem::Shutdown()
{
	if (this->initialized) {
		TTF_CloseFont(this->font);
		TTF_Quit();
		SDL_Quit();
		delete[] this->mem;
		this->initialized = false;
		this->dirty = false;
	}
}

/**
 * Finish repainting, perform the final steps.
 * @todo Implement partial window repainting.
 */
void VideoSystem::FinishRepaint()
{
	SDL_UpdateTexture(this->texture, nullptr, this->mem, this->GetXSize() * sizeof(uint32)); // Upload memory to the GPU.
	SDL_RenderClear(this->renderer);
	SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);

	MarkDisplayClean();
}

/**
 * Blit a pixel to an area of \a numx times \a numy sprites.
 * @param cr Clipped rectangle.
 * @param scr_base Base address of the screen array.
 * @param xmin Minimal x position.
 * @param ymin Minimal y position.
 * @param numx Number of horizontal count.
 * @param numy Number of vertical count.
 * @param width Width of an image.
 * @param height Height of an image.
 * @param colour Pixel value to blit.
 * @note Function does not handle alpha blending of the new pixel with the background.
 */
static void BlitPixel(const ClippedRectangle &cr, uint32 *scr_base,
		int32 xmin, int32 ymin, uint16 numx, uint16 numy, uint16 width, uint16 height, uint32 colour)
{
	const int32 xend = xmin + numx * width;
	const int32 yend = ymin + numy * height;
	while (ymin < yend) {
		if (ymin >= cr.height) return;

		if (ymin >= 0) {
			uint32 *scr = scr_base;
			int32 x = xmin;
			while (x < xend) {
				if (x >= cr.width) break;
				if (x >= 0) *scr = colour;

				x += width;
				scr += width;
			}
		}
		ymin += height;
		scr_base += height * cr.pitch;
	}
}

/**
 * Blit 8bpp images to the screen.
 * @param cr Clipped rectangle to draw to.
 * @param x_base Base X coordinate of the sprite data.
 * @param y_base Base Y coordinate of the sprite data.
 * @param spr The sprite to blit.
 * @param numx Number of sprites to draw in horizontal direction.
 * @param numy Number of sprites to draw in vertical direction.
 * @param recoloured Shifted palette to use.
 */
static void Blit8bppImages(const ClippedRectangle &cr, int32 x_base, int32 y_base, const ImageData *spr, uint16 numx, uint16 numy, const uint8 *recoloured)
{
	uint32 *line_base = cr.address + x_base + cr.pitch * y_base;
	int32 ypos = y_base;
	for (int yoff = 0; yoff < spr->height; yoff++) {
		uint32 offset = spr->table[yoff];
		if (offset != INVALID_JUMP) {
			int32 xpos = x_base;
			uint32 *src_base = line_base;
			for (;;) {
				uint8 rel_off = spr->data[offset];
				uint8 count   = spr->data[offset + 1];
				uint8 *pixels = &spr->data[offset + 2];
				offset += 2 + count;

				xpos += rel_off & 127;
				src_base += rel_off & 127;
				while (count > 0) {
					uint32 colour = _palette[recoloured[*pixels]];
					BlitPixel(cr, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
					pixels++;
					xpos++;
					src_base++;
					count--;
				}
				if ((rel_off & 128) != 0) break;
			}
		}
		line_base += cr.pitch;
		ypos++;
	}
}

/**
 * Blit 32bpp images to the screen.
 * @param cr Clipped rectangle to draw to.
 * @param x_base Base X coordinate of the sprite data.
 * @param y_base Base Y coordinate of the sprite data.
 * @param spr The sprite to blit.
 * @param numx Number of sprites to draw in horizontal direction.
 * @param numy Number of sprites to draw in vertical direction.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 */
static void Blit32bppImages(const ClippedRectangle &cr, int32 x_base, int32 y_base, const ImageData *spr, uint16 numx, uint16 numy, const Recolouring &recolour, GradientShift shift)
{
	uint32 *line_base = cr.address + x_base + cr.pitch * y_base;
	ShiftFunc sf = GetGradientShiftFunc(shift);
	int32 ypos = y_base;
	const uint8 *src = spr->data + 2; // Skip the length word.
	for (int yoff = 0; yoff < spr->height; yoff++) {
		int32 xpos = x_base;
		uint32 *src_base = line_base;
		for (;;) {
			uint8 mode = *src++;
			if (mode == 0) break;
			switch (mode >> 6) {
				case 0: // Fully opaque pixels.
					mode &= 0x3F;
					for (; mode > 0; mode--) {
						uint32 colour = MakeRGBA(sf(src[0]), sf(src[1]), sf(src[2]), OPAQUE);
						BlitPixel(cr, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
						xpos++;
						src_base++;
						src += 3;
					}
					break;

				case 1: { // Partial opaque pixels.
					uint8 opacity = *src++;
					mode &= 0x3F;
					for (; mode > 0; mode--) {
						/* Cheat transparency a bit by just recolouring the previously drawn pixel */
						uint32 old_pixel = *src_base;

						uint r = sf(src[0]) * opacity + GetR(old_pixel) * (256 - opacity);
						uint g = sf(src[1]) * opacity + GetG(old_pixel) * (256 - opacity);
						uint b = sf(src[2]) * opacity + GetB(old_pixel) * (256 - opacity);

						/* Opaque, but colour adjusted depending on the old pixel. */
						uint32 ndest = MakeRGBA(r >> 8, g >> 8, b >> 8, OPAQUE);
						BlitPixel(cr, src_base, xpos, ypos, numx, numy, spr->width, spr->height, ndest);
						xpos++;
						src_base++;
						src += 3;
					}
					break;
				}
				case 2: // Fully transparent pixels.
					xpos += mode & 0x3F;
					src_base += mode & 0x3F;
					break;

				case 3: { // Recoloured pixels.
					uint8 layer = *src++;
					const uint32 *table = recolour.GetRecolourTable(layer - 1);
					uint8 opacity = *src++;
					mode &= 0x3F;
					for (; mode > 0; mode--) {
						uint32 old_pixel = *src_base;
						uint32 recoloured = table[*src++];

						uint r = sf(GetR(recoloured)) * opacity + GetR(old_pixel) * (256 - opacity);
						uint g = sf(GetG(recoloured)) * opacity + GetG(old_pixel) * (256 - opacity);
						uint b = sf(GetB(recoloured)) * opacity + GetB(old_pixel) * (256 - opacity);

						uint32 colour = MakeRGBA(r >> 8, g >> 8, b >> 8, OPAQUE);
						BlitPixel(cr, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
						xpos++;
						src_base++;
					}
					break;
				}
			}
		}
		line_base += cr.pitch;
		ypos++;
		src += 2; // Skip the length word.
	}
}

/**
 * Blit pixels from the \a spr relative to \a img_base into the area.
 * @param pt Base coordinates of the sprite data.
 * @param spr The sprite to blit.
 * @param numx Number of sprites to draw in horizontal direction.
 * @param numy Number of sprites to draw in vertical direction.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 */
void VideoSystem::BlitImages(const Point32 &pt, const ImageData *spr, uint16 numx, uint16 numy, const Recolouring &recolour, GradientShift shift)
{
	this->blit_rect.ValidateAddress();

	int x_base = pt.x + spr->xoffset;
	int y_base = pt.y + spr->yoffset;

	/* Don't draw wildly outside the screen. */
	while (numx > 0 && x_base + spr->width < 0) {
		x_base += spr->width; numx--;
	}
	while (numx > 0 && x_base + (numx - 1) * spr->width >= this->blit_rect.width) numx--;
	if (numx == 0) return;

	while (numy > 0 && y_base + spr->height < 0) {
		y_base += spr->height; numy--;
	}
	while (numy > 0 && y_base + (numy - 1) * spr->height >= this->blit_rect.height) numy--;
	if (numy == 0) return;

	if (GB(spr->flags, IFG_IS_8BPP, 1) != 0) {
		Blit8bppImages(this->blit_rect, x_base, y_base, spr, numx, numy, recolour.GetPalette(shift));
	} else {
		Blit32bppImages(this->blit_rect, x_base, y_base, spr, numx, numy, recolour, shift);
	}
}

/**
 * Get the text-size of a string.
 * @param text Text to calculate.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void VideoSystem::GetTextSize(const uint8 *text, int *width, int *height)
{
	if (TTF_SizeUTF8(this->font, (const char *)text, width, height) != 0) {
		*width = 0;
		*height = 0;
	}
}

/**
 * Get the text-size of a number between \a smallest and \a biggest.
 * @param smallest Smallest possible number to display.
 * @param biggest Biggest possible number to display.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void VideoSystem::GetNumberRangeSize(int64 smallest, int64 biggest, int *width, int *height)
{
	assert(smallest <= biggest);
	if (this->digit_size.x == 0) { // First call, initialize the variable.
		this->digit_size.x = 0;
		this->digit_size.y = 0;

		uint8 buffer[2];
		buffer[1] = '\0';
		for (uint8 i = '0'; i <= '9'; i++) {
			buffer[0] = i;
			int w, h;
			this->GetTextSize(buffer, &w, &h);
			if (w > this->digit_size.x) this->digit_size.x = w;
			if (h > this->digit_size.y) this->digit_size.y = h;
		}
	}

	int digit_count = 1;
	if (smallest < 0) digit_count++; // Add a 'digit' for the '-' sign.

	int64 tmp = std::max(std::abs(smallest), std::abs(biggest));
	while (tmp > 9) {
		digit_count++;
		tmp /= 10;
	}

	*width = digit_count * this->digit_size.x;
	*height = this->digit_size.y;
}

/**
 * Blit text to the screen.
 * @param text Text to display.
 * @param colour Colour of the text.
 * @param xpos Absolute horizontal position at the display.
 * @param ypos Absolute vertical position at the display.
 * @param width Available width of the text (in pixels).
 * @param align Horizontal alignment of the string.
 */
void VideoSystem::BlitText(const uint8 *text, uint32 colour, int xpos, int ypos, int width, Alignment align)
{
	SDL_Color col = {0, 0, 0}; // Font colour does not matter as only the bitmap is used.
	SDL_Surface *surf = TTF_RenderUTF8_Solid(this->font, (const char *)text, col);
	if (surf == nullptr) {
		fprintf(stderr, "Rendering text failed (%s)\n", TTF_GetError());
		return;
	}

	if (surf->format->BitsPerPixel != 8 || surf->format->BytesPerPixel != 1) {
		fprintf(stderr, "Rendering text failed (Wrong surface format)\n");
		return;
	}

	int real_w = std::min(surf->w, width);
	switch (align) {
		case ALG_LEFT:
			break;

		case ALG_CENTER:
			xpos += (width - real_w) / 2;
			break;

		case ALG_RIGHT:
			xpos += width - real_w;
			break;

		default: NOT_REACHED();
	}

	this->blit_rect.ValidateAddress();

	uint8 *src = ((uint8 *)surf->pixels);
	uint32 *dest = this->blit_rect.address + xpos + ypos * this->blit_rect.pitch;
	int h = surf->h;
	if (ypos < 0) {
		h += ypos;
		src  -= ypos * surf->pitch;
		dest -= ypos * this->blit_rect.pitch;
		ypos = 0;
	}
	while (h > 0) {
		if (ypos >= this->blit_rect.height) break;
		uint8 *src2 = src;
		uint32 *dest2 = dest;
		int w = real_w;
		int x = xpos;
		if (x < 0) {
			w += x;
			if (w <= 0) break;
			dest2 -= x;
			src2 -= x;
			x = 0;
		}
		while (w > 0) {
			if (x >= this->blit_rect.width) break;
			if (*src2 != 0) *dest2 = colour;
			src2++;
			dest2++;
			x++;
			w--;
		}
		ypos++;
		src  += surf->pitch;
		dest += this->blit_rect.pitch;
		h--;
	}

	SDL_FreeSurface(surf);
}

/**
 * Draw a line from \a start to \a end using the specified \a colour.
 * @param start Starting point at the screen.
 * @param end End point at the screen.
 * @param colour Colour to use.
 */
void VideoSystem::DrawLine(const Point16 &start, const Point16 &end, uint32 colour)
{
	int16 dx, inc_x, dy, inc_y;
	if (start.x > end.x) {
		dx = start.x - end.x;
		inc_x = -1;
	} else {
		dx = end.x - start.x;
		inc_x = 1;
	}
	if (start.y > end.y) {
		dy = start.y - end.y;
		inc_y = -1;
	} else {
		dy = end.y - start.y;
		inc_y = 1;
	}

	int16 step = std::min(dx, dy);
	int16 pos_x = start.x;
	int16 pos_y = start.y;
	int16 sum_x = 0;
	int16 sum_y = 0;

	this->blit_rect.ValidateAddress();
	uint32 *dest = this->blit_rect.address + pos_x + pos_y * this->blit_rect.pitch;

	for (;;) {
		/* Blit pixel. */
		if (pos_x >= 0 && pos_x < this->blit_rect.width && pos_y >= 0 && pos_y < this->blit_rect.height) {
			*dest = colour;
		}
		if (pos_x == end.x && pos_y == end.y) break;

		sum_x += step;
		sum_y += step;
		if (sum_x >= dy) {
			pos_x += inc_x;
			dest += inc_x;
			sum_x -= dy;
		}
		if (sum_y >= dx) {
			pos_y += inc_y;
			dest += inc_y * this->blit_rect.pitch;
			sum_y -= dx;
		}
	}
}

/**
 * Draw a rectangle at the screen.
 * @param rect %Rectangle to draw.
 * @param colour Colour to use.
 */
void VideoSystem::DrawRectangle(const Rectangle32 &rect, uint32 colour)
{
	Point16 top_left    (static_cast<int16>(rect.base.x),                  static_cast<int16>(rect.base.y));
	Point16 top_right   (static_cast<int16>(rect.base.x + rect.width - 1), static_cast<int16>(rect.base.y));
	Point16 bottom_left (static_cast<int16>(rect.base.x),                  static_cast<int16>(rect.base.y + rect.height - 1));
	Point16 bottom_right(static_cast<int16>(rect.base.x + rect.width - 1), static_cast<int16>(rect.base.y + rect.height - 1));
	this->DrawLine(top_left, top_right, colour);
	this->DrawLine(top_left, bottom_left, colour);
	this->DrawLine(top_right, bottom_right, colour);
	this->DrawLine(bottom_left, bottom_right, colour);
}

/**
 * Fill the rectangle with a single colour.
 * @param rect %Rectangle to fill.
 * @param colour Colour to fill with.
 */
void VideoSystem::FillRectangle(const Rectangle32 &rect, uint32 colour)
{
	ClippedRectangle cr = this->GetClippedRectangle();

	int x = Clamp((int)rect.base.x, 0, (int)cr.width);
	int w = Clamp((int)(rect.base.x + rect.width), 0, (int)cr.width);
	int y = Clamp((int)rect.base.y, 0, (int)cr.height);
	int h = Clamp((int)(rect.base.y + rect.height), 0, (int)cr.height);

	w -= x;
	h -= y;
	if (w == 0 || h == 0) return;

	uint32 *pixels_base = cr.address + x + y * cr.pitch;
	while (h > 0) {
		uint32 *pixels = pixels_base;
		for (int i = 0; i < w; i++) *pixels++ = colour;
		pixels_base += cr.pitch;
		h--;
	}
}
