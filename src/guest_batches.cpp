/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file guest_batches.cpp Code for batches of guests. */

#include "stdafx.h"
#include "guest_batches.h"

GuestData::GuestData()
{
	this->Clear();
}

/** Clear the guest information, making the entry free for next use. */
void GuestData::Clear()
{
	this->guest = -1;
	this->entry = INVALID_EDGE;
}

/**
 * Store the guest information into the entry
 * @param guest %Guest number.
 * @param entry Entry direction of the guest into the ride.
 */
void GuestData::Set(int guest, TileEdge entry)
{
	assert(entry != INVALID_EDGE);
	assert(this->IsEmpty());

	this->guest = guest;
	this->entry = entry;
}

/**
 * Return whether the batch is entirely empty.
 * @return Whether the batch is entirely empty.
 */
bool GuestBatch::IsEmpty() const
{
	for (const GuestData &val : this->guests) if (!val.IsEmpty()) return false;
	return true;
}

/**
 * Configure a batch of guests.
 * @param batch_size Number of guests in a batch.
 */
void GuestBatch::Configure(int batch_size)
{
	assert(this->IsEmpty());

	this->guests.clear();
	this->guests.resize(batch_size);

	this->state = BST_EMPTY;
	this->remaining = 0;
	this->gate = 0;
}

/**
 * Try to add a guest into the batch.
 * @param guest %Guest number.
 * @param entry Entry direction of the guest into the ride.
 * @return Whether the guest could be added.
 */
bool GuestBatch::AddGuest(int guest, TileEdge entry)
{
	for (GuestData &val : this->guests) {
		if (!val.IsEmpty()) continue;

		val.Set(guest, entry);
		return true;
	}
	return false;
}

/**
 * Start the ride (make it #BST_RUNNING).
 * @param ride_time Length of the ride in milli-seconds.
 */
void GuestBatch::Start(int ride_time)
{
	this->state = BST_RUNNING;
	this->remaining = ride_time;
}

/**
 * Update the state of the ride due to passage of time.
 * @param delay Amount of time that has passed (milli-seconds).
 */
void GuestBatch::OnAnimate(int delay)
{
	if (this->state != BST_RUNNING) return;

	if (this->remaining > delay) {
		this->remaining -= delay;
	} else {
		this->remaining = 0;
		this->state = BST_FINISHED;
	}
}

/**
 * Constructor for storage of on-ride guests.
 * @param batch_size Size of one group of guests.
 * @param num_batches Number of groups that can be on the ride at the same time.
 */
OnRideGuests::OnRideGuests(int batch_size, int num_batches)
{
	this->Configure(batch_size, num_batches);
}

/**
 * (Re)configure the ride for the given number of batches and batch size.
 * @param batch_size Size of one group of guests.
 * @param num_batches Number of groups that can be on the ride at the same time.
 * @note The ride should be empty, as all guest information is destroyed.
 */
void OnRideGuests::Configure(int batch_size, int num_batches)
{
	this->batches.clear();
	this->batches.resize(num_batches);
	for (GuestBatch &gb : this->batches) gb.Configure(batch_size);

	this->batch_size = batch_size;
	this->num_batches = num_batches;
}

/**
 * Get the index of the next batch with a given state.
 * @param state State of the batch to look for.
 * @param start Last returned index number, or \c -1 to look from the first batch.
 * @return Index of the first batch with the given state after \a start, or \c -1 if no such batch exists.
 */
int OnRideGuests::GetNextBatch(BatchState state, int start)
{
	start++;
	while (start < static_cast<int>(this->batches.size())) {
		const GuestBatch &gb = this->batches.at(start);
		if (gb.state == state) return start;

		start++;
	}
	return -1;
}

/**
 * Time has passed, update the remaining times of the running batches.
 * @param delay Amount of time that has passed (milli-seconds).
 */
void OnRideGuests::OnAnimate(int delay)
{
	int start = -1;
	for (;;) {
		start = this->GetNextBatch(BST_RUNNING, start);
		if (start < 0) break;

		this->batches.at(start).OnAnimate(delay);
	}
}
