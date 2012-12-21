/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

 /**
  * @file money.h
  * Implementation of money.
  * @note Basic format is taken from OpenTTD.
  */

#ifndef MONEY_H
#define MONEY_H

/**
 * Overflow safe integer
 * you multiply the maximum value with 2, or add 2, or substract something from
 * the minimum value, etc.
 */
class Money {
private:
	int64 m_value; ///< Non-overflow safe backend to store the value in.

public:
	Money() : m_value(0) { }

	/**
	 * Constructor.
	 * @param other Another Money to create a new Money with.
	 */
	Money(const Money& other)
	{
		this->m_value = other.m_value;
	}

	/**
	 * Constructor.
	 * @param num The value to create a Money class with.
	 */
	Money(int64 num)
	{
		this->m_value = num;
	}

	/**
	 * Assignment operator.
	 * @param other The new value to assign to this.
	 * @return The new value.
	 */
	inline Money& operator=(const Money& other)
	{
		this->m_value = other.m_value;
		return *this;
	}

	/**
	 * Minus Operator.
	 * @return The negative of this.
	 */
	inline Money operator-() const
	{
		return Money(-this->m_value);
	}

	/**
	 * Safe implementation of addition.
	 * @param other The amount to add.
	 * @note When the addition would yield more than INT64_MAX (or less than INT64_MIN),
	 *       it will be INT64_MAX (respectively INT64_MIN).
	 * @return The result.
	 */
	inline Money& operator+= (const Money& other)
	{
		if ((INT64_MAX - abs(other.m_value)) < abs(this->m_value) &&
		    (this->m_value < 0) == (other.m_value < 0)) {
			this->m_value = (this->m_value < 0) ? INT64_MIN : INT64_MAX ;
		} else {
			this->m_value += other.m_value;
		}
		return *this;
	}

	/* ### Operators for addition and substraction ### */

	/**
	 * Addition operator.
	 * @param other The money to add to this.
	 * @return The result.
	 */
	inline Money operator+(const Money& other) const
	{
		Money result = *this;
		result += other;
		return result;
	}

	/**
	 * Addition operator.
	 * @param other The int to add to this.
	 * @return The result.
	 */
	inline Money operator+(const int other) const
	{
		Money result = *this;
		result += (int64)other;
		return result;
	}

	/**
	 * Addition operator.
	 * @param other The uint to add to this.
	 * @return The result.
	 */
	inline Money operator+(const uint other) const
	{
		Money result = *this;
		result += (int64)other;
		return result;
	}

	/**
	 * Subtraction operator.
	 * @param other The money to take away from this.
	 * @return The result.
	 */
	inline Money& operator-=(const Money& other)
	{
		return *this += (-other);
	}

	/**
	 * Subtraction operator.
	 * @param other The money to take away from this.
	 * @return The result.
	 */
	inline Money operator-(const Money& other) const
	{
		Money result = *this;
		result -= other;
		return result;
	}

	/**
	 * Subtraction operator.
	 * @param other The int to take away from this.
	 * @return The result.
	 */
	inline Money operator-(const int other) const
	{
		Money result = *this;
		result -= (int64)other;
		return result;
	}

	/**
	 * Subtraction operator.
	 * @param other The uint to take away from this.
	 * @return The result.
	 */
	inline Money operator-(const uint other) const
	{
		Money result = *this;
		result -= (int64)other;
		return result;
	}

	/**
	 * Prefix increment operator.
	 * @return The result.
	 */
	inline Money& operator++()
	{
		return *this += 1;
	}

	/**
	 * Prefix decrement operator.
	 * @return The result.
	 */
	inline Money& operator--()
	{
		return *this += -1;
	}

	/**
	 * Postfix increment operator.
	 * @return The result.
	 */
	inline Money operator++(int)
	{
		Money org = *this;
		*this += 1;
		return org;
	}

	/**
	 * Postfix decrement operator.
	 * @return The result.
	 */
	inline Money operator--(int)
	{
		Money org = *this;
		*this += -1;
		return org;
	}

	/**
	 * Safe implementation of multiplication.
	 * @param factor the factor to multiply this with.
	 * @note when the multiplication would yield more than INT64_MAX (or less than INT64_MIN),
	 *       it will be INT64_MAX (respectively INT64_MIN).
	 * @return The result.
	 */
	inline Money& operator*= (const int factor)
	{
		if (factor != 0 && (INT64_MAX / abs(factor)) < abs(this->m_value)) {
			 this->m_value = ((this->m_value < 0) == (factor < 0)) ? INT64_MAX : INT64_MIN ;
		} else {
			this->m_value *= factor ;
		}
		return *this;
	}

	/* ### Operators for multiplication ### */

	/**
	 * Multiplication operator.
	 * @param factor The int64 to multiply this by.
	 * @return The result.
	 */
	inline Money operator*(const int64 factor) const
	{
		Money result = *this;
		result *= factor;
		return result;
	}

	/**
	 * Multiplication operator.
	 * @param factor The int to multiply this by.
	 * @return The result.
	 */
	inline Money operator*(const int factor) const
	{
		Money result = *this;
		result *= (int64)factor;
		return result;
	}

	/**
	 * Multiplication operator.
	 * @param factor The uint to multiply this by.
	 * @return The result.
	 */
	inline Money operator*(const uint factor) const
	{
		Money result = *this;
		result *= (int64)factor;
		return result;
	}

	/**
	 * Multiplication operator.
	 * @param factor The uint16 to multiply this by.
	 * @return The result.
	 */
	inline Money operator*(const uint16 factor) const
	{
		Money result = *this;
		result *= (int64)factor;
		return result;
	}

	/**
	 * Multiplication operator.
	 * @param factor The byte to multiply this by.
	 * @return The result.
	 */
	inline Money operator*(const byte factor) const
	{
		Money result = *this;
		result *= (int64)factor;
		return result;
	}

	/* ### Operators for division ### */

	/**
	 * Division operator.
	 * @param divisor The int64 to divide this by.
	 * @return The result.
	 */
	inline Money& operator/=(const int64 divisor)
	{
		this->m_value /= divisor;
		return *this;
	}

	/**
	 * Division operator.
	 * @param divisor The Money to divide this by.
	 * @return The result.
	 */
	inline Money operator/(const Money& divisor) const
	{
		Money result = *this;
		result /= divisor.m_value;
		return result;
	}

	/**
	 * Division operator.
	 * @param divisor The int to divide this by.
	 * @return The result.
	 */
	inline Money operator/(const int divisor) const
	{
		Money result = *this;
		result /= divisor;
		return result;
	}

	/**
	 * Division operator.
	 * @param divisor The uint to divide this by.
	 * @return The result.
	 */
	inline Money operator/(const uint divisor) const
	{
		Money result = *this;
		result /= (int)divisor;
		return result;
	}

	/* ### Operators for modulo ### */

	/**
	 * Modulus operator.
	 * @param divisor The int divisor.
	 * @return The result.
	 */
	inline Money& operator%=(const int divisor)
	{
		this->m_value %= divisor;
		return *this;
	}

	/**
	 * Modulus operator.
	 * @param divisor The int divisor.
	 * @return The result.
	 */
	inline Money operator%(const int divisor) const
	{
		Money result = *this;
		result %= divisor;
		return result;
	}

	/* ### Operators for shifting ### */

	/**
	 * Shift operator.
	 * @param shift The amount to shift this to the left.
	 * @return The result.
	 */
	inline Money& operator<<= (const int shift)
	{
		this->m_value <<= shift;
		return *this;
	}

	/**
	 * Shift operator.
	 * @param shift The amount to shift this to the left.
	 * @return The result.
	 */
	inline Money operator<< (const int shift) const
	{
		Money result = *this;
		result <<= shift;
		return result;
	}

	/**
	 * Shift operator.
	 * @param shift The amount to shift this to the right.
	 * @return The result.
	 */
	inline Money& operator>>=(const int shift)
	{
		this->m_value >>= shift;
		return *this;
	}

	/**
	 * Shift operator.
	 * @param shift The amount to shift this to the right.
	 * @return The result.
	 */
	inline Money operator>>(const int shift) const
	{
		Money result = *this;
		result >>= shift;
		return result;
	}

	/* ### Operators for (in)equality when comparing overflow safe ints ### */

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator==(const Money& other) const
	{
		return this->m_value == other.m_value;
	}

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator!=(const Money& other) const
	{
		return !(*this == other);
	}

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator>(const Money& other) const
	{
		return this->m_value > other.m_value;
	}

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator>=(const Money& other) const
	{
		return this->m_value >= other.m_value;
	}

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator<(const Money& other) const
	{
		return !(*this >= other);
	}

	/**
	 * Equality operator.
	 * @param other The Money to check.
	 * @return The result.
	 */
	inline bool operator<=(const Money& other) const
	{
		return !(*this > other);
	}

	/* ### Operators for (in)equality when comparing non-overflow safe ints ### */

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator==(const int other) const
	{
		return this->m_value == other;
	}

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator!=(const int other) const
	{
		return !(*this == other);
	}

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator>(const int other) const
	{
		return this->m_value > other;
	}

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator>=(const int other) const
	{
		return this->m_value >= other;
	}

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator<(const int other) const
	{
		return !(*this >= other);
	}

	/**
	 * Equality operator.
	 * @param other The int to check.
	 * @return The result.
	 */
	inline bool operator<=(const int other) const
	{
		return !(*this > other);
	}

	/**
	 * Cast operator.
	 * @return This, as an int64.
	 */
	inline operator int64() const
	{
		return this->m_value;
	}
};

#endif
