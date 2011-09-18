/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling functions. */

#include "stdafx.h"
#include "window.h"
#include "map.h"
#include "video.h"
#include "sprite_store.h"

#include <map>

/**
 * %Window manager class, manages the window stack.
 */
class WindowManager {
public:
	WindowManager();
	~WindowManager();

	bool HasWindow(Window *w);
	void AddTostack(Window *w);
	Window *RemoveFromStack(Window *w);

	Window *top;        ///< Top-most window in the window stack.
	Window *bottom;     ///< Lowest window in the window stack.
	VideoSystem *video; ///< Video output device.
};

static WindowManager _manager; ///< %Window manager.

/**
 * %Window constructor. It automatically adds the window to the window stack.
 * @param xp X position top-left corner.
 * @param yp Y position top-left corner.
 * @param wp Width of the window.
 * @param hp Height of the window.
 */
Window::Window(int xp, int yp, uint wp, uint hp) : x(xp), y(yp), width(wp), height(hp)
{
	this->higher = NULL;
	this->lower  = NULL;

	_manager.AddTostack(this); // Add to window stack.
}

/**
 * Destructor.
 * Do not call directly, delete the return value of #WindowManager::RemoveFromStack instead.
 */
Window::~Window()
{
	assert(!_manager.HasWindow(this));
}

/**
 * Mark windows as being dirty (needing a repaint).
 * @todo Marking the whole display as needing a repaint is too crude.
 */
void Window::MarkDirty()
{
	_manager.video->MarkDisplayDirty();
}

/**
 * Paint the window to the screen.
 */
/* virtual */ void Window::OnDraw()
{
}


/**
 * Default constructor.
 * @todo Make #Viewport a derived class of #Window.
 */
Viewport::Viewport(int x, int y, uint w, uint h) : Window(x, y, w, h)
{
	this->xview = _world.GetXSize() * 256 / 2;
	this->yview = _world.GetYSize() * 256 / 2;
	this->zview = 0 * 256;

	this->tile_width  = 64;
	this->tile_height = 16;
	this->orientation = VOR_NORTH;
}

/** Data temporary needed for drawing. */
struct DrawData {
	const Sprite *spr; ///< Sprite to draw.
	Point base;        ///< Base coordinate of the image, relative to top-left of the window.
};

/**
 * Map of distance to image.
 * Used for temporary sorting and storage of images drawn at the viewport.
 */
typedef std::multimap<int32, DrawData> DrawImages;

/** @todo Do this less stupid. Drawing the whole world is not going to work in general. */
/* virtual */ void Viewport::OnDraw()
{
	Point center = this->ComputeXY(this->xview, this->yview, this->zview);
	Rectangle rect;
	rect.base.x = center.x - this->width / 2;
	rect.base.y = center.y - this->height / 2;
	rect.width = this->width;
	rect.height = this->height;

	DrawImages draw_images;

	int dx, dy; // Offset of the 'north' corner of a view relative to the real north corner.
	int sx, sy; // Direction of x and y in sorting.
	switch (this->orientation) {
		case VOR_NORTH: dx = 0;   dy = 0;   sx =  1; sy =  1; break;
		case VOR_EAST:  dx = 0;   dy = 256; sx =  1; sy = -1; break;
		case VOR_SOUTH: dx = 256; dy = 256; sx = -1; sy = -1; break;
		case VOR_WEST:  dx = 256; dy = 0;   sx = -1; sy =  1; break;
		default:        NOT_REACHED();
	}

	for (int xpos = 0; xpos < _world.GetXSize(); xpos++) {
		for (int ypos = 0; ypos < _world.GetYSize(); ypos++) {
			VoxelStack *stack = _world.GetStack(xpos, ypos);
			for (int16 count = 0; count < stack->height; count++) {
				Voxel *v = &stack->voxels[count];
				if (v->GetType() != VT_SURFACE) continue;
				Slope sl = v->GetSlope();
				const Sprite *spr = _sprite_store.GetSurfaceSprite(0, sl, this->tile_width, this->orientation);
				if (spr == NULL) continue;
				Point north_corner = this->ComputeXY(xpos * 256 + dx, ypos * 256 + dy, (stack->base + count) * 256);
				Rectangle spr_rect = Rectangle(north_corner.x + spr->xoffset, north_corner.y + spr->yoffset,
						spr->img_data->width, spr->img_data->height);
				if (rect.Intersects(spr_rect)) {
					std::pair<int32, DrawData> p;
					p.first = sx * xpos * 256 + sy * ypos * 256 + (stack->base + count) * 256;
					p.second.spr = spr;
					p.second.base.x = spr_rect.base.x - rect.base.x + this->x;
					p.second.base.y = spr_rect.base.y - rect.base.y + this->y;
					draw_images.insert(p);
				}
			}
		}
	}

	Rectangle wind_rect = Rectangle(this->x, this->y, this->width, this->height); // XXX Why not use this in the window itself?

	VideoSystem *vid = GetVideo();
	vid->LockSurface();

	vid->FillSurface(0); // Black background.

	for (DrawImages::const_iterator iter = draw_images.begin(); iter != draw_images.end(); iter++) {
		vid->BlitImage((*iter).second.base, (*iter).second.spr, wind_rect);
	}

	vid->UnlockSurface();
}

/**
 * Rotate 90 degrees clockwise or anti-clockwise.
 * @param direction Direction of turn (positive means clockwise).
 */
void Viewport::Rotate(int direction)
{
	this->orientation = (ViewOrientation)((this->orientation + VOR_NUM_ORIENT + ((direction > 0) ? 1 : -1)) % VOR_NUM_ORIENT);
	this->MarkDirty();
}

/**
 * %Window manager default constructor.
 * @todo Move this to a separate InitWindowSystem, together with #SetVideo?
 */
WindowManager::WindowManager()
{
	this->top = NULL;
	this->bottom = NULL;

	this->video = NULL;
};

/** %Window manager destructor. */
WindowManager::~WindowManager()
{
	while (this->top != NULL) {
		delete this->RemoveFromStack(this->top);
	}
}

/**
 * Set the video output device.
 * @param vid Video output to use for drawing windows.
 */
void SetVideo(VideoSystem *vid)
{
	_manager.video = vid;
}

/**
 * Get the video output device.
 * @return Video device.
 */
VideoSystem *GetVideo()
{
	return _manager.video;
}

/**
 * Add a window to the window stack.
 * @param w Window to add.
 * @todo Implement Z priorities.
 */
void WindowManager::AddTostack(Window *w)
{
	assert(w->lower == NULL && w->higher == NULL);
	assert(!this->HasWindow(w));

	w->lower = this->top;
	w->higher = NULL;
	this->top = w;
	if (this->bottom == NULL) this->bottom = w;
}

/**
 * Remove a window from the stack.
 * @param w Window to remove.
 * @return Removed window.
 */
Window *WindowManager::RemoveFromStack(Window *w)
{
	assert(this->HasWindow(w));

	if (w->higher == NULL) {
		this->top = w->lower;
	} else {
		w->higher->lower = w->lower;
	}

	if (w->lower == NULL) {
		this->bottom = w->higher;
	} else {
		w->lower->higher = w->higher;
	}

	w->higher = NULL;
	w->lower  = NULL;
	return w;
}

/**
 * Test whether a particular window exists in the window stack.
 * @param w Window to look for.
 * @return Window exists in the window stack.
 * @note Mainly used for paranoia checking.
 */
bool WindowManager::HasWindow(Window *w)
{
	Window *v = this->top;
	while (v != NULL) {
		if (v == w) return true;
		v = v->lower;
	}
	return false;
}

/**
 * Redraw (parts of) the windows.
 * @todo Do this much less stupid.
 */
void UpdateWindows()
{
	if (_manager.video == NULL || !_manager.video->DisplayNeedsRepaint()) return;

	Window *w = _manager.bottom;
	while (w != NULL) {
		w->OnDraw();
		w = w->higher;
	}
}

