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
	Point() = default;

	/**
	 * Parameterised constructor.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 */
	Point(CT x, CT y)
	{
		this->x = x;
		this->y = y;
	}

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
inline bool operator<(const Point<CT> &p, const Point<CT> &q)
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
inline bool operator==(const Point<CT> &p, const Point<CT> &q)
{
	return p.x == q.x && p.y == q.y;
}

/**
 * Generic 3D point template.
 * @tparam CT Type of the coordinates.
 */
template <typename CT>
struct XYZPoint {
	XYZPoint() = default;

	/**
	 * Constructor for creating a point at a given coordinate.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 */
	XYZPoint(CT x, CT y, CT z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	/**
	 * Get the 2D coordinates of the 3D point.
	 * @return X & Y coordinates.
	 */
	Point<CT> Get2D() const
	{
		return Point<CT>(this->x, this->y);
	}

	typedef CT CoordType; ///< Type of the coordinate value.

	CT x; ///< X coordinate.
	CT y; ///< Y coordinate.
	CT z; ///< Z coordinate.

	/**
	 * Add a 3D point to this one.
	 * @param q Second point to add.
	 * @return All 3 dimensions added to each other.
	 */
	inline XYZPoint<CT> operator+=(const XYZPoint<CT> &q)
	{
		this->x += q.x;
		this->y += q.y;
		this->z += q.z;
		return *this;
	}
};

typedef XYZPoint<int32> XYZPoint32; ///< 32 bit 3D point.
typedef XYZPoint<int16> XYZPoint16; ///< 16 bit 3D point.

/**
 * Test for 3D point equality.
 * @param p First point to compare.
 * @param q Second point to compare.
 * @return Points are logically equal (same x, y, and z position).
 */
template <typename CT>
inline bool operator==(const XYZPoint<CT> &p, const XYZPoint<CT> &q)
{
	return p.x == q.x && p.y == q.y && p.z == q.z;
}

/**
 * Test for 3D point inequality.
 * @param p First point to compare.
 * @param q Second point to compare.
 * @return Points are logically inequal (x, y, or z position is different).
 */
template <typename CT>
inline bool operator!=(const XYZPoint<CT> &p, const XYZPoint<CT> &q)
{
	return !(p == q);
}

/**
 * Comparsion of two 3D points.
 * @param p First point to compare.
 * @param q Second point to compare.
 * @return
 */
template <typename CT>
inline bool operator<(const XYZPoint<CT> &p, const XYZPoint<CT> &q)
{
	return p.x < q.x && p.y < q.y && p.z < q.z;
}

/**
 * Add two 3D points together.
 * @param p First point to add.
 * @param q Second point to add.
 * @return All 3 dimensions added to each other.
 * @note Takes a copy of the lhs.
 */
template <typename CT>
inline XYZPoint<CT> operator+(XYZPoint<CT> p, const XYZPoint<CT> &q)
{
	p += q;
	return p;
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
	 * @return The assigned value.
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

	/**
	 * Change the rectangle coordinates such that the given point is inside the rectangle.
	 * @param pt %Point to add.
	 */
	template <typename CT>
	void AddPoint(const Point<CT> &pt)
	{
		this->AddPoint(pt.x, pt.y);
	}

	/**
	 * Change the rectangle coordinates such that the given point is inside the rectangle.
	 * @param xpos X position of the point to add.
	 * @param ypos Y position of the point to add.
	 */
	void AddPoint(typename PT::CoordType xpos, typename PT::CoordType ypos)
	{
		if (this->width == 0) {
			this->base.x = xpos;
			this->width = 1;
		} else if (xpos < this->base.x) {
			this->width += this->base.x - xpos;
			this->base.x = xpos;
		} else if (this->base.x + (typename PT::CoordType)this->width <= xpos) {
			this->width = xpos - this->base.x + 1;
		}

		if (this->height == 0) {
			this->base.y = ypos;
			this->height = 1;
		} else if (ypos < this->base.y) {
			this->height += this->base.y - ypos;
			this->base.y = ypos;
		} else if (this->base.y + (typename PT::CoordType)this->height <= ypos) {
			this->height = ypos - this->base.y + 1;
		}
	}

	/**
	 * Restrict the rectangle to the given area (this is, ensure it is completely inside the provided area).
	 * @param rect %Rectangle to restrict to.
	 */
	void RestrictTo(const Rectangle<PT, SZ> &rect)
	{
		this->RestrictTo(rect.base.x, rect.base.y, rect.width, rect.height);
	}

	/**
	 * Restrict the rectangle to the given area (this is, ensure it is completely inside the provided area).
	 * @param startx Base X coordinate of the area to restrict to.
	 * @param starty Base Y coordinate of the area to restrict to.
	 * @param w Horizontal length of the area to restrict to.
	 * @param h Vertical length of the area to restrict to.
	 */
	void RestrictTo(typename PT::CoordType startx, typename PT::CoordType starty, SZ w, SZ h)
	{
		if (this->width == 0 || this->height == 0) return;

		typename PT::CoordType xend = this->base.x + this->width;
		if (xend > static_cast<typename PT::CoordType>(startx + w)) xend = startx + w;
		if (this->base.x < startx) this->base.x = startx;
		this->width = (this->base.x < xend) ? xend - this->base.x : 0;

		typename PT::CoordType yend = this->base.y + this->height;
		if (yend > static_cast<typename PT::CoordType>(starty + h)) yend = starty + h;
		if (this->base.y < starty) this->base.y = starty;
		this->height = (this->base.y < yend) ? yend - this->base.y : 0;
	}

	/**
	 * Merge the area of the provided rectangle with this rectangle, so both areas are included.
	 * @param rect %Rectangle to merge.
	 */
	void MergeArea(const Rectangle<PT, SZ> &rect)
	{
		this->MergeArea(rect.base.x, rect.base.y, rect.width, rect.height);
	}

	/**
	 * Merge the area of the provided rectangle coordinates with this rectangle, so both areas are included.
	 * @param startx Base X coordinate of the area to merge.
	 * @param starty Base Y coordinate of the area to merge.
	 * @param w Horizontal length of the area to merge.
	 * @param h Vertical length of the area to merge.
	 */
	void MergeArea(typename PT::CoordType startx, typename PT::CoordType starty, SZ w, SZ h)
	{
		this->AddPoint(startx, starty);
		this->AddPoint(startx + w, starty + h);
	}

	PT base;   ///< Base coordinate.
	SZ width;  ///< Width of the rectangle.
	SZ height; ///< Height of the rectangle.
};

/**
 * Test rectangles for equality.
 * @tparam PT Base point type.
 * @tparam SZ Size type.
 * @param r1 First rectangle to test.
 * @param r2 Second rectangle to test.
 * @return The rectangles express the same area.
 */
template <typename PT, typename SZ>
inline bool operator==(const Rectangle<PT, SZ> &r1, const Rectangle<PT, SZ> &r2)
{
	return r1.base == r2.base && r1.width == r2.width && r1.height == r2.height;
}

typedef Rectangle<Point16, uint16> Rectangle16; ///< %Rectangle with 16 bit values.
typedef Rectangle<Point32, uint32> Rectangle32; ///< %Rectangle with 16 bit values.

#endif
