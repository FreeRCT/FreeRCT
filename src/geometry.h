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

/**
 * An area in 2D.
 * @tparam PT Base point type.
 * @tparam SZ Size type.
 */
template <typename PT, typename SZ>
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
	Rectangle(typename PT::CoordType x, typename PT::CoordType y, SZ w, SZ h)
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
	Rectangle(const Rectangle<PT, SZ> &rect)
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
	Rectangle &operator=(const Rectangle<PT, SZ> &rect)
	{
		if (this != &rect) {
			this->base.x = rect.base.x;
			this->base.y = rect.base.y;
			this->width  = rect.width;
			this->height = rect.height;
		}
		return *this;
	}

	/**
	 * Do the rectangles intersect with each other?
	 * @param rect Other rectangle.
	 * @return Rectangles intersect with each other.
	 */
	bool Intersects(const Rectangle<PT, SZ> &rect) const
	{
		if (rect.base.x >= this->base.x + (typename PT::CoordType)this->width) return false;
		if (rect.base.x + (typename PT::CoordType)rect.width <= (typename PT::CoordType)this->base.x) return false;
		if (rect.base.y >= this->base.y + (typename PT::CoordType)this->height) return false;
		if (rect.base.y + (typename PT::CoordType)rect.height <= this->base.y) return false;
		return true;
	}

	/**
	 * Is a given coordinate inside the rectangle?
	 * @param pt %Point to test.
	 * @tparam CT Coordinate type of the point (implicit).
	 * @return The given coordinate is inside the rectangle.
	 */
	template <typename CT>
	bool IsPointInside(const Point<CT> &pt)
	{
		if (this->base.x > pt.x || this->base.x + (typename PT::CoordType)this->width  <= pt.x) return false;
		if (this->base.y > pt.y || this->base.y + (typename PT::CoordType)this->height <= pt.y) return false;
		return true;
	}

	PT base;   ///< Base coordinate.
	SZ width;  ///< Width of the rectangle.
	SZ height; ///< Height of the rectangle.
};

typedef Rectangle<Point16, uint16> Rectangle16; ///< %Rectangle with 16 bit values.
typedef Rectangle<Point32, uint32> Rectangle32; ///< %Rectangle with 16 bit values.

#endif
