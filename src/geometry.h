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

/**
 * Generic point template.
 * @tparam CT Type of the coordinates.
 */
template <typename CT>
struct Point {
	typedef CT CoordType; ///< Type of the coordinate value.

	CT x; ///< X coordinate.
	CT y; ///< Y coordinate.
};

typedef Point<int32> Point32; ///< 32 bit 2D point.
typedef Point<int16> Point16; ///< 16 bit 2D point.

/**
 * Test for point order.
 * @param p First point to compare.
 * @param q Second point to compare.
 * @return Points \a p is less than point \a q.
 */
template <typename CT>
FORCEINLINE bool operator<(const Point<CT> &p, const Point<CT> &q)
{
	if (p.x != q.x) return p.x < q.x;
	return p.y < q.y;
}

/**
 * Test for point equality.
 * @param p First point to compare.
 * @param q Second point to compare.
 * @return Points are logically equal (same x and y position).
 */
template <typename CT>
FORCEINLINE bool operator==(const Point<CT> &p, const Point<CT> &q)
{
	return p.x == q.x && p.y == q.y;
}

/** An area in 2D using 32 bit coordinates. */
struct Rectangle32 {
	/** Default constructor. */
	Rectangle32()
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
	Rectangle32(int32 x, int32 y, int32 w, int32 h)
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
	bool Intersects(const Rectangle32 &rect) const
	{
		if (rect.base.x >= this->base.x + this->width) return false;
		if (rect.base.x + rect.width <= this->base.x) return false;
		if (rect.base.y >= this->base.y + this->height) return false;
		if (rect.base.y + rect.height <= this->base.y) return false;
		return true;
	}

	Point32 base; ///< Base coordinate.
	int32 width;  ///< Width of the rectangle.
	int32 height; ///< Height of the rectangle.
};

/** An area in 2D using 16 bit coordinates. */
struct Rectangle16 {
	/** Default constructor. */
	Rectangle16()
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
	Rectangle16(int16 x, int16 y, uint16 w, uint16 h)
	{
		this->base.x = x;
		this->base.y = y;
		this->width  = w;
		this->height = h;
	}

	/**
	 * Copy constructor.
	 * @param rect %Rectangle to copy.
	 */
	Rectangle16(const Rectangle16 &rect)
	{
		this->base.x = rect.base.x;
		this->base.y = rect.base.y;
		this->width  = rect.width;
		this->height = rect.height;
	}

	/**
	 * Assignment operator
	 * @param rect %Rectangle to copy.
	 */
	Rectangle16 &operator=(const Rectangle16 &rect)
	{
		if (this != &rect) {
			this->base.x = rect.base.x;
			this->base.y = rect.base.y;
			this->width  = rect.width;
			this->height = rect.height;
		}
		return *this;
	}

	Point16 base;  ///< Base coordinate.
	uint16 width;  ///< Width of the rectangle.
	uint16 height; ///< Height of the rectangle.
};



#endif
