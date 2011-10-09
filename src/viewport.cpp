/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.cpp %Window handling functions. */

#include "stdafx.h"
#include "math_func.h"
#include "viewport.h"
#include "map.h"
#include "video.h"
#include "palette.h"
#include "sprite_store.h"

/**
 * %Viewport constructor.
 */
Viewport::Viewport(int x, int y, uint w, uint h) : Window(x, y, w, h, WC_MAINDISPLAY)
{
	this->xview = _world.GetXSize() * 256 / 2;
	this->yview = _world.GetYSize() * 256 / 2;
	this->zview = 8 * 256;

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

/**
 * Set mode of the mouse interaction of the viewport.
 * @param mode New mode.
 */
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

