/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file geometry.h Functions and data structures for 2D and 3D positions, etc. */

#ifndef GEOMETRY_H
#define GEOMETRY_H

/** A 2D point in space. */
struct Point {
	int32 x; ///< X coordinate.
	int32 y; ///< Y coordinate.
};

/** An area in 2D. */
struct Rectangle {
	/** Default constructor. */
	Rectangle()
	{
		this->base.x = 0;
		this->base.y = 0;
		this->width  = 0;
		this->height = 0;
	}

	/**
	 * Construct a rectangle with the given data.
	 * @param x X position of the top-left edge.
	 * @param y Y position of the top-left edge.
	 * @param w Width of the rectangle.
	 * @param h Height of the rectangle.
	 */
	Rectangle(int32 x, int32 y, int32 w, int32 h)
	{
		this->base.x = x;
		this->base.y = y;
		this->width  = w;
		this->height = h;
	}

	/**
	 * Do the rectangles intersect with each other?
	 * @param rect Other rectangle.
	 * @return Rectangles intersect with each other.
	 */
	bool Intersects(const Rectangle &rect) const
	{
		if (rect.base.x >= this->base.x + this->width) return false;
		if (rect.base.x + rect.width <= this->base.x) return false;
		if (rect.base.y >= this->base.y + this->height) return false;
		if (rect.base.y + rect.height <= this->base.y) return false;
		return true;
	}

	Point base;    ///< Base coordinate.
	int32 width;  ///< Width of the rectangle.
	int32 height; ///< Height of the rectangle.
};

#endif
