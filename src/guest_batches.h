/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file guest_batches.h Batches of guests on a ride.
 *
 * Guests can enter a ride in groups, a batch of guests. A ride can have several such batches active
 * at the same time (several rooms, or several cars).
 */

#ifndef GUEST_BATCHES_H
#define GUEST_BATCHES_H

#include "tile.h"
#include <vector>

/** Data of a guest in a ride. */
struct GuestData {
	GuestData();

	int guest;      ///< Number of the guest.
	TileEdge entry; ///< Direction of entry of the guest at the ride, #INVALID_EDGE for a non-used entry.

	/**
	 * Is the entry in use?
	 * @return Whether the entry is currently empty.
	 */
	bool IsEmpty() const
	{
		return this->entry == INVALID_EDGE;
	}

	void Clear();
	void Set(int guest, TileEdge entry);
};

/** State of the batch. */
enum BatchState {
	BST_EMPTY,     ///< Batch is free.
	BST_LOADING,   ///< Batch is loading.
	BST_RUNNING,   ///< Batch is running the ride.
	BST_FINISHED,  ///< Batch has finished running, guests are waiting for unloading.
	BST_UNLOADING, ///< Batch is unloading.
};


/** A batch (a group) of guests riding together. */
struct GuestBatch {
	std::vector<GuestData> guests; ///< Guests in the batch.
	BatchState state; ///< State of the batch.
	int remaining;    ///< Amount of time until the end of the ride (in milli-seconds). Positive means time is running,
	                  ///< \c 0 means the batch has reached the end.
	int gate;         ///< Gate used by the guests to enter the ride (or for any other purpose as the ride sees fit).

	bool IsEmpty() const;
	void Configure(int batch_size);

	bool AddGuest(int guest, TileEdge entry);
	void Start(int ride_time);

	void OnAnimate(int delay);
};

class OnRideGuests {
public:
	OnRideGuests(int batch_size = 0, int num_batches = 0);

	void Configure(int batch_size, int num_batches);

	/**
	 * Get a batch of guests.
	 * @param index Index number of the batch.
	 * @return The requested batch.
	 */
	inline GuestBatch &GetBatch(int index)
	{
		return this->batches[index];
	}

	/**
	 * Get an empty batch.
	 * @param start Index where the last search ended, or \c -1 to search from the start.
	 * @return Index of a free batch, or \c -1 if no free batch exists.
	 */
	inline int GetFreeBatch(int start = -1)
	{
		return this->GetNextBatch(BST_EMPTY, start);
	}

	/**
	 * Get a batch that is being loaded.
	 * @param start Index where the last search ended, or \c -1 to search from the start.
	 * @return Index of a loading batch, or \c -1 if no loading batch exists.
	 */
	inline int GetLoadingBatch(int start = -1)
	{
		return this->GetNextBatch(BST_LOADING, start);
	}

	/**
	 * Get a batch that has finished the ride.
	 * @param start Index where the last search ended, or \c -1 to search from the start.
	 * @return Index of a finished batch, or \c -1 if no finished batch exists.
	 */
	inline int GetFinishedBatch(int start = -1)
	{
		return this->GetNextBatch(BST_FINISHED, start);
	}

	/**
	 * Get a batch that is being unloaded.
	 * @param start Index where the last search ended, or \c -1 to search from the start.
	 * @return Index of an unloading batch, or \c -1 if no unloading batch exists.
	 */
	inline int GetUnloadingBatch(int start = -1)
	{
		return this->GetNextBatch(BST_UNLOADING, start);
	}

	void OnAnimate(int delay);

	std::vector<GuestBatch> batches; ///< Batches of guests.
	int batch_size;  ///< Size of a batch of guests.
	int num_batches; ///< Number of batches in the ride.

private:
	int GetNextBatch(BatchState state, int start);
};

#endif

