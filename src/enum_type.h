/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file enum_type.h Some enum templates. */

#ifndef ENUM_TYPE_H
#define ENUM_TYPE_H

/* Copied from OpenTTD. */

/**
 * Operators to allow to work with enum as with type safe bit set in C++.
 * @tparam mask_t Enum type.
 */
#define DECLARE_ENUM_AS_BIT_SET(mask_t) \
	inline mask_t  operator |  (mask_t  m1, mask_t m2) { return (mask_t)((int)m1 | m2); } \
	inline mask_t  operator &  (mask_t  m1, mask_t m2) { return (mask_t)((int)m1 & m2); } \
	inline mask_t  operator ^  (mask_t  m1, mask_t m2) { return (mask_t)((int)m1 ^ m2); } \
	inline mask_t &operator |= (mask_t &m1, mask_t m2) { m1 = m1 | m2; return m1; } \
	inline mask_t &operator &= (mask_t &m1, mask_t m2) { m1 = m1 & m2; return m1; } \
	inline mask_t &operator ^= (mask_t &m1, mask_t m2) { m1 = m1 ^ m2; return m1; } \
	inline mask_t  operator ~  (mask_t  m) { return (mask_t)(~(int)m); }

/** Add post-increment and post-decrement operators to an enum. */
#define DECLARE_POSTFIX_INCREMENT(type) \
	inline type operator ++(type& e, int) \
	{ \
		type e_org = e; \
		e = (type)((int)e + 1); \
		return e_org; \
	} \
	inline type operator --(type& e, int) \
	{ \
		type e_org = e; \
		e = (type)((int)e - 1); \
		return e_org; \
	}

#endif

