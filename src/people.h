/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.h Declarations for people in the world. */

#ifndef PEOPLE_H
#define PEOPLE_H

#include "person_type.h"
#include "random.h"

static const int GUEST_BLOCK_SIZE = 512; ///< Number of guests in a block.

/** Data of a guest. */
struct GuestData {
	int16 happiness; ///< Happiness of the guest (values are 0-100).
};

/**
 * Common base class of a person in the world.
 *
 * Persons are stored in contiguous blocks of memory, which makes the constructor and destructor useless.
 * Instead, \c Activate and \c DeActivate methods are used for this purpose. The #type variable controls the state of the entry.
 */
class Person {
public:
	Person();
	virtual ~Person();

	bool OnAnimate(int delay);
	bool DailyUpdate();

	void Activate(const Point16 &start, uint8 person_type);
	void DeActivate();

	void SetName(const char *name);
	const char *GetName() const;

	uint16 id;  ///< Unique id (also depends on derived class).
	/**
	 * Type of person.
	 * #PERSON_INVALID means the entry is not used.
	 * @see PersonType
	 */
	uint8 type;

	Person *next; ///< Next person in the linked list.
	Person *prev; ///< Previous person in the linked list.

protected:
	Random rnd; ///< Random number generator for deciding how the person reacts.
	char *name; ///< Name of the person. \c NULL means it has a default name (like "Guest XYZ").

	union {
		GuestData guest; ///< Guest data (only valid if #PersonIsAGuest holds for #type).
	} u; ///< Person-type specific data.
};

/** Manager of a doubly linked list of persons. */
class PersonList {
public:
	PersonList();
	~PersonList();

	bool IsEmpty();

	void AddFirst(Person *p);
	void Remove(Person *p);
	Person *Get();

protected:
	Person *first; ///< First person in the list.
	Person *last;  ///< Last person in the list.
};

/* Forward declarations. */
template <typename PersonType, int SIZE> class Block;
template <typename PersonType, int SIZE> class BlockIterator;
template <typename PersonType, int SIZE> bool operator !=(const BlockIterator<PersonType, SIZE> &iter1, const BlockIterator<PersonType, SIZE> &iter2);

/**
 * Template class for iterating over blocks.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
class BlockIterator {
	friend bool operator!=<>(const BlockIterator<PersonType, SIZE> &iter1, const BlockIterator<PersonType, SIZE> &iter2);
public:
	BlockIterator();
	BlockIterator(Block<PersonType, SIZE> *block, int index);
	BlockIterator(const BlockIterator<PersonType, SIZE> &iter);
	BlockIterator &operator=(const BlockIterator<PersonType, SIZE> &iter);
	BlockIterator operator++(int);
	PersonType *operator*();

private:
	Block<PersonType, SIZE> *block; ///< Block to iterate over.
	uint index;                     ///< Current index in the block.
};

/**
 * Default constructor.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE>::BlockIterator() : block(NULL), index(0) { }

/**
 * Constructor with arguments for a specific #Block and index.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @param blk %Block to iterate over.
 * @param idx Index of the iterator.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE>::BlockIterator(Block<PersonType, SIZE> *blk, int idx) : block(blk), index(idx) { }

/**
 * Copy constructor.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE>::BlockIterator(const BlockIterator<PersonType, SIZE> &iter) : block(iter.block), index(iter.index) { }

/**
 * Assignment operator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE> &BlockIterator<PersonType, SIZE>::operator=(const BlockIterator<PersonType, SIZE> &iter)
{
	if (this != &iter) {
		this->block = iter.block;
		this->index = iter.index;
	}
	return *this;
}

/**
 * Postfix increment operator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE> BlockIterator<PersonType, SIZE>::operator++(int)
{
	if (this->index < SIZE) {
		for (;;) {
			this->index++;
			if (this->index >= SIZE) break;
			uint32 bit = ((uint32)1) << (this->index % 32);
			if ((block->actives[this->index / 32] & bit) != 0) break;
		}
	}
	return *this;
}

/**
 * Unary '*' operator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
PersonType *BlockIterator<PersonType, SIZE>::operator*()
{
	assert(this->index < (uint)SIZE);
	return &block->element[this->index];
}

/**
 * Un-equality operator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
bool operator !=(const BlockIterator<PersonType, SIZE> &iter1, const BlockIterator<PersonType, SIZE> &iter2)
{
	return iter1.block != iter2.block || iter1.index != iter2.index;
}

/**
 * Template class holding a block of persons.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
class Block {
	friend class BlockIterator<PersonType, SIZE>;
public:
	typedef BlockIterator<PersonType, SIZE> iterator;

	Block(uint16 base_id);
	~Block();

	PersonType *GetNew();
	void DeActivate(PersonType *p);

	/**
	 * Is entry \a index active?
	 * @param index Queried entry number.
	 * @return %Block element with the given index is active.
	 */
	inline bool IsActive(int index)
	{
		if (index < 0 || index >= SIZE) return false;
		uint32 bit = ((uint32)1) << ((uint)index % 32);
		return (this->actives[(uint)index / 32] & bit) != 0;
	}

	/**
	 * Get the data of a person.
	 * @param index Queried entry number.
	 * @return Reference to the data of the queried person.
	 */
	inline PersonType &Get(int index)
	{
		return this->element[index];
	}

	iterator begin();
	iterator end();

	uint16 base_id; ///< Base number of this block.
protected:
	PersonType element[SIZE];
	uint32 actives[SIZE + 31 / 32];
};

/**
 * Block initializer.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @param base_id Base number of the IDs.
 */
template <typename PersonType, int SIZE>
Block<PersonType, SIZE>::Block(uint16 base_id)
{
	for (int i = 0; i < SIZE + 31 / 32; i++) this->actives[i] = 0;

	this->base_id = base_id;
	for (int i = 0; i < SIZE; i++) {
		this->element[i].id = base_id;
		base_id++;
	}
}

/**
 * Destructor.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 */
template <typename PersonType, int SIZE>
Block<PersonType, SIZE>::~Block() { }

/**
 * Initializer of the block iterator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @return STL-style iterator initialized for iterating over the block.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE> Block<PersonType, SIZE>::begin()
{
	BlockIterator<PersonType, SIZE> iter(this, 0);
	if ((this->actives[0] & 1) == 0) iter++;
	return iter;
}

/**
 * Final value of the block iterator.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @return STL-style iterator representing the end of the iteration.
 */
template <typename PersonType, int SIZE>
BlockIterator<PersonType, SIZE> Block<PersonType, SIZE>::end()
{
	BlockIterator<PersonType, SIZE> iter(this, SIZE);
	return iter;
}

/**
 * Try to construct a new guest.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @return A non-initialized #Guest if it could be created, else \c NULL.
 */
template <typename PersonType, int SIZE>
PersonType *Block<PersonType, SIZE>::GetNew()
{
	for (uint idx = 0; idx < (uint)SIZE; idx++) {
		uint32 bit = ((uint32)1) << (idx % 32);
		if ((this->actives[idx / 32] & bit) == 0) {
			this->actives[idx / 32] |= bit;
			return &this->element[idx];
		}
	}
	return NULL;
}

/**
 * De-activate a guest.
 * @tparam PersonType Type of person.
 * @tparam SIZE Size of the block.
 * @param pt The guest to de-activate.
 */
template <typename PersonType, int SIZE>
void Block<PersonType, SIZE>::DeActivate(PersonType *pt)
{
	assert(pt->id >= this->base_id && pt->id < this->base_id + SIZE);
	uint16 idx = pt->id - this->base_id;
	assert(pt == &this->element[idx]);
	uint32 bit = ((uint32)1) << (idx % 32);
	this->actives[idx / 32] &= ~bit;
}


/** A block of guests. */
class GuestBlock: public Block<Person, GUEST_BLOCK_SIZE> {
public:
	GuestBlock(uint16 base_id);
};

/**
 * All our guests.
 * @todo Allow to have several blocks of guests.
 */
class Guests {
public:
	Guests();
	~Guests();

	void OnAnimate(int delay);
	void DoTick();
	void OnNewDay();

private:
	GuestBlock block;     ///< The data of all actual guests.
	Random rnd;           ///< Random number generator for creating new guests.
	Point16 start_voxel;  ///< Entry x/y coordinate of the voxel stack at the edge.
	int daily_frac;       ///< Frame counter.
	int next_daily_index; ///< Index of the next guest to give daily service.
};

extern Guests _guests;

#endif

