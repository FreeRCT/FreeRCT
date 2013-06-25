/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file reference_count.h Reference counting code. */

#ifndef REFERENCE_COUNT_H
#define REFERENCE_COUNT_H

#include <cassert>

/**
 * Reference counting class.
 * Derive from this class to add reference counting to your data.
 * For added protection, make the destructor protected.
 *
 * Use the #DataReference template class for automatic reference count handling.
 */
class RefCounter {
public:
	/** Default constructor, creates an owner of the object. */
	RefCounter()
	{
		this->count = 1;
	}

	/** Remove one owner of the object, and clean up when it reaches \c 0. */
	void Delete()
	{
		assert(this->count > 0);
		this->count--;
		if (this->count == 0) delete this;
	}

	/** Perform a virtual copy by adding an owner of this object. */
	void Copy() const
	{
		assert(this->count > 0);
		this->count++;
	}

	mutable int count; ///< Number of references.

protected:
	/** Protected destructor, use #Delete instead. */
	virtual ~RefCounter()
	{
	}
};

/**
 * Template class for keeping a reference associated with ownership.
 * @tparam D Data class being stored. Should derive from the #RefCounter class.
 */
template <typename D> class DataReference {
public:
	/** Default constructor, reference is empty. */
	DataReference()
	{
		this->data = NULL;
	}

	/**
	 * Constructor, taking ownership of the data.
	 * @param data Data to take ownership.
	 */
	DataReference(D *data)
	{
		this->data = data;
	}

	/**
	 * Copy constructor, copying the data.
	 * @param dr Value to copy from.
	 */
	DataReference(const DataReference<D> &dr)
	{
		D *dr_data = (dr.Access() == NULL) ? NULL : dr.Copy();
		this->data = dr_data;
	}

	/**
	 * Assignment operator, copying the data.
	 * @param dr Value to copy from.
	 * @return The reference object.
	 */
	DataReference<D> &operator=(const DataReference<D> &dr)
	{
		if (&dr != this) {
			D *dr_data = (dr.Access() == NULL) ? NULL : dr.Copy();
			this->Give(dr_data);
		}
		return *this;
	}

	/** Destructor. */
	~DataReference()
	{
		if (this->data != NULL) this->data->Delete();
	}

	/**
	 * Receive a reference from the environment.
	 * @param data Reference to store.
	 */
	void Give(D *data)
	{
		if (this->data != NULL) this->data->Delete();
		this->data = data;
	}

	/**
	 * Steal the reference.
	 * @return The stolen reference.
	 */
	D *Steal()
	{
		assert(this->data != NULL); // You cannot steal things that don't exist.
		D *ref = this->data;
		this->data = NULL;
		return ref;
	}

	/**
	 * Give a virtual copy of the referenced object.
	 * @return A virtually copied reference.
	 */
	D *Copy() const
	{
		assert(this->data != NULL); // You cannot copy things that don't exist.
		this->data->Copy();
		return this->data;
	}

	/**
	 * Give access to the data, without transfer of ownership.
	 * @return A reference to the owned data.
	 */
	D *Access()
	{
		return this->data;
	}

	/**
	 * Give access to the data, without transfer of ownership.
	 * @return A reference to the owned data.
	 */
	const D *Access() const
	{
		return this->data;
	}

private:
	D *data; ///< Pointer to the shared object.
};

#endif
