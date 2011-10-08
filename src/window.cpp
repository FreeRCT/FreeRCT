/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling functions. */

#include "stdafx.h"
#include "math_func.h"
#include "window.h"
#include "map.h"
#include "video.h"
#include "palette.h"
#include "sprite_store.h"

WindowManager _manager; ///< %Window manager.

/**
 * %Window constructor. It automatically adds the window to the window stack.
 * @param xp X position top-left corner.
 * @param yp Y position top-left corner.
 * @param wp Width of the window.
 * @param hp Height of the window.
 * @param wtype %Window type (for finding a window in the stack).
 */
Window::Window(int xp, int yp, uint wp, uint hp, WindowTypes wtype) :
		x(xp), y(yp), width(wp), height(hp), wtype(wtype)
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

/** Paint the window to the screen. */
/* virtual */ void Window::OnDraw() { }

/**
 * Mouse moved to new position.
 * @param pos New position.
 */
/* virtual */ void Window::OnMouseMoveEvent(const Point16 &pos) { }

/**
 * Mouse buttons changed state.
 * @param state Updated state. @see MouseButtons
 */
/* virtual */ void Window::OnMouseButtonEvent(uint8 state) { }

/**
 * Mousewheel rotated.
 * @param direction Direction of change (\c +1 or \c -1).
 */
/* virtual */ void Window::OnMouseWheelEvent(int direction) { }

/** Mouse entered window. */
/* virtual */ void Window::OnMouseEnterEvent() { }

/** Mouse left window. */
/* virtual */ void Window::OnMouseLeaveEvent() { }

/**
 * %Viewport constructor.
 */
Viewport::Viewport(int x, int y, uint w, uint h) : Window(x, y, w, h, WC_MAINDISPLAY)
{
	this->xview = _world.GetXSize() * 256 / 2;
	this->yview = _world.GetYSize() * 256 / 2;
	this->zview = 0 * 256;

	this->tile_width  = 64;
	this->tile_height = 16;
	this->orientation = VOR_NORTH;

	this->mouse_mode = MM_INACTIVE;
	this->mouse_pos.x = 0;
	this->mouse_pos.y = 0;
	this->mouse_state = 0;
}

/** Data temporary needed for drawing. */
struct DrawData {
	const Sprite *spr; ///< Sprite to draw.
	Point base;        ///< Base coordinate of the image, relative to top-left of the window.
};

/**
 * Add sprites of the voxel to the set of sprites to draw.
 * @param vport %Viewport being drawn.
 * @param rect 2D rectangle of the visible area relative to the center point of \a vport.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param north_x X coordinate of the voxel base 2D position.
 * @param north_y Y coordinate of the voxel base 2D position.
 * @param draw_images [inout] Collected sprites to draw so far.
 */
void Voxel::AddSprites(const Viewport *vport, const Rectangle &rect, int xpos, int ypos, int zpos, int32 north_x, int32 north_y, DrawImages &draw_images)
{
	int sx, sy; // Direction of x and y in sorting.
	switch (vport->orientation) {
		case VOR_NORTH: sx =  1; sy =  1; break;
		case VOR_EAST:  sx =  1; sy = -1; break;
		case VOR_SOUTH: sx = -1; sy = -1; break;
		case VOR_WEST:  sx = -1; sy =  1; break;
		default:        NOT_REACHED();
	}

	if (this->GetType() != VT_SURFACE) return;

	const Sprite *spr = _sprite_store.GetSurfaceSprite(0, this->GetSurfaceSprite(), vport->tile_width, vport->orientation);
	if (spr == NULL) return;

	Rectangle spr_rect = Rectangle(north_x + spr->xoffset, north_y + spr->yoffset, spr->img_data->width, spr->img_data->height);
	if (rect.Intersects(spr_rect)) {
		std::pair<int32, DrawData> p;
		p.first = sx * xpos * 256 + sy * ypos * 256 + zpos * 256;
		p.second.spr = spr;
		p.second.base.x = spr_rect.base.x - rect.base.x + vport->x;
		p.second.base.y = spr_rect.base.y - rect.base.y + vport->y;
		draw_images.insert(p);
	}
}


/** @todo Do this less stupid. Drawing the whole world is not going to work in general. */
/* virtual */ void Viewport::OnDraw()
{
	Rectangle rect;
	rect.base.x = this->ComputeX(this->xview, this->yview) - this->width / 2;
	rect.base.y = this->ComputeY(this->xview, this->yview, this->zview) - this->height / 2;
	rect.width = this->width;
	rect.height = this->height;

	DrawImages draw_images;

	int dx, dy; // Offset of the 'north' corner of a view relative to the real north corner.
	switch (this->orientation) {
		case VOR_NORTH: dx = 0;   dy = 0;   break;
		case VOR_EAST:  dx = 0;   dy = 256; break;
		case VOR_SOUTH: dx = 256; dy = 256; break;
		case VOR_WEST:  dx = 256; dy = 0;   break;
		default:        NOT_REACHED();
	}

	for (int xpos = 0; xpos < _world.GetXSize(); xpos++) {
		for (int ypos = 0; ypos < _world.GetYSize(); ypos++) {
			int32 north_x = ComputeX(xpos * 256 + dx, ypos * 256 + dy);
			if (north_x + this->tile_width / 2 <= (int32)rect.base.x) continue; // Right of voxel column is at left of window.
			if (north_x - this->tile_width / 2 >= (int32)(rect.base.x + this->width)) continue; // Left of the window.

			VoxelStack *stack = _world.GetStack(xpos, ypos);
			for (int16 count = 0; count < stack->height; count++) {
				int32 north_y = this->ComputeY(xpos * 256 + dx, ypos * 256 + dy, (stack->base + count) * 256);
				if (north_y >= (int32)(rect.base.y + this->height)) continue; // Voxel is below the window.
				if (north_y + this->tile_width / 2 + this->tile_height <= (int32)rect.base.y) break; // Above the window and rising!

				stack->voxels[count].AddSprites(this, rect, xpos, ypos, stack->base + count, north_x, north_y, draw_images);
			}
		}
	}

	Rectangle wind_rect = Rectangle(this->x, this->y, this->width, this->height); // XXX Why not use this in the window itself?

	VideoSystem *vid = GetVideo();
	vid->LockSurface();

	vid->FillSurface(COL_BACKGROUND); // Black background.

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
 * Move the viewport a number of screen pixels.
 * @param dx Horizontal shift in screen pixels.
 * @param dy Vertical shift in screen pixels.
 */
void Viewport::MoveViewport(int dx, int dy)
{
	int32 new_x, new_y;
	switch (this->orientation) {
		case VOR_NORTH:
			new_x = this->xview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_y = this->yview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		case VOR_EAST:
			new_x = this->xview - dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			new_y = this->yview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_SOUTH:
			new_x = this->xview - dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_y = this->yview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			break;

		case VOR_WEST:
			new_x = this->xview + dx * 256 / this->tile_width + dy * 512 / this->tile_width;
			new_y = this->yview + dx * 256 / this->tile_width - dy * 512 / this->tile_width;
			break;

		default:
			NOT_REACHED();
	}

	new_x = Clamp<int32>(new_x, 0, _world.GetXSize() * 256);
	new_y = Clamp<int32>(new_y, 0, _world.GetYSize() * 256);
	if (new_x != this->xview || new_y != this->yview) {
		this->xview = new_x;
		this->yview = new_y;
		this->MarkDirty();
	}
}

void Viewport::SetMouseMode(MouseMode mode)
{
	assert(mode < MM_COUNT);
	this->mouse_state = 0;
	this->mouse_mode = mode;
}

/* virtual */ void Viewport::OnMouseMoveEvent(const Point16 &pos)
{
	switch (this->mouse_mode) {
		case MM_INACTIVE:
			break;

		case MM_TILE_TERRAFORM:
			if (pos == this->mouse_pos) break;
			if ((this->mouse_state & MB_RIGHT) != 0) {
				/* Drag the window if button is pressed down. */
				this->MoveViewport(pos.x - this->mouse_pos.x, pos.y - this->mouse_pos.y);
			}
			this->mouse_pos = pos;
			break;

		default: NOT_REACHED();
	}
}

/* virtual */ void Viewport::OnMouseButtonEvent(uint8 state)
{
	switch (this->mouse_mode) {
		case MM_INACTIVE:
			break;

		case MM_TILE_TERRAFORM:
			this->mouse_state = state & MB_CURRENT;
			break;

		default: NOT_REACHED();
	}
}

/* virtual */ void Viewport::OnMouseEnterEvent()
{
	this->mouse_state = 0;
}

/* virtual */ void Viewport::OnMouseLeaveEvent()
{
	this->mouse_state = 0;
}

/**
 * %Window manager default constructor.
 * @todo Move this to a separate InitWindowSystem, together with #SetVideo?
 */
WindowManager::WindowManager()
{
	this->top = NULL;
	this->bottom = NULL;

	/* Mouse event handling. */
	this->mouse_pos.x = -10000; // A very unlikely position for a window.
	this->mouse_pos.y = -10000;
	this->current_window = NULL;
	this->mouse_state = 0;

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

Window *WindowManager::FindWindowByPosition(const Point16 &pos) const
{
	Window *w = this->top;
	while (w != NULL) {
		if (pos.x > w->x && pos.y > w->y && pos.x < w->x + (int)w->width && pos.y < w->y + (int)w->height) break;
		w = w->lower;
	}
	return w;
}

void WindowManager::MouseMoveEvent(const Point16 &pos)
{
	if (pos == this->mouse_pos) return;
	this->mouse_pos = pos;

	this->UpdateCurrentWindow();
	if (this->current_window != NULL) {
		/* Compute position relative to window origin. */
		Point16 pos2;
		pos2.x = pos.x - this->current_window->x;
		pos2.y = pos.y - this->current_window->y;
		this->current_window->OnMouseMoveEvent(pos2);
	}
}

/**
 * Update the #current_window variable.
 * This may happen when the mouse has moved, but also because of a change in the window stack.
 * @return Window has changed.
 * @todo Hook a call to this function into the window stack changing code.
 */
bool WindowManager::UpdateCurrentWindow()
{
	Window *w = this->FindWindowByPosition(this->mouse_pos);
	if (w == this->current_window) return false;

	/* Windows are different, send mouse leave/enter events. */
	if (this->current_window != NULL && this->HasWindow(this->current_window)) this->current_window->OnMouseLeaveEvent();

	this->current_window = w;
	if (this->current_window != NULL) this->current_window->OnMouseEnterEvent();
	return true;
}

void WindowManager::MouseButtonEvent(MouseButtons button, bool pressed)
{
	assert(button == MB_LEFT || button == MB_MIDDLE || button == MB_RIGHT);
	uint8 newstate = this->mouse_state;
	if (pressed) {
		newstate |= button;
	} else {
		newstate &= ~button;
	}

	this->UpdateCurrentWindow();
	if (newstate != this->mouse_state && this->current_window != NULL) {
		this->current_window->OnMouseButtonEvent((this->mouse_state << 4) | newstate);
	}
	this->mouse_state = newstate;
}

void WindowManager::MouseWheelEvent(int direction)
{
	this->UpdateCurrentWindow();
	if (this->current_window != NULL) this->current_window->OnMouseWheelEvent(direction);
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

/**
 * Find an opened window by window type.
 * @param wtype %Window type to look for.
 * @return The window with the requested type if it is opened, else \c NULL.
 */
Window *GetWindowByType(WindowTypes wtype)
{
	Window *w = _manager.top;
	while (w != NULL) {
		if (w->wtype == wtype) return w;
		w = w->lower;
	}
	return NULL;
}
