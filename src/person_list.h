/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person_list.h A list of persons. */

#ifndef PERSON_LIST_H
#define PERSON_LIST_H

class Person;

/** Manager of a doubly linked list of persons. */
class PersonList {
	friend void CopyPersonList(PersonList &dest, const PersonList &src);
public:
	PersonList();
	~PersonList();

	bool IsEmpty() const;

	void AddFirst(Person *p);
	void Remove(Person *p);
	Person *RemoveHead();

	Person *first; ///< First person in the list.
	Person *last;  ///< Last person in the list.
};

void CopyPersonList(PersonList &dest, const PersonList &src);

#endif
