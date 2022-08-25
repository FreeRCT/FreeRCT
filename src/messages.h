/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file messages.h Definition of messages sent to the player. */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <list>
#include <memory>

#include "stdafx.h"
#include "dates.h"
#include "language.h"
#include "loadsave.h"

/** The available categories of messages. */
enum MessageCategory {
	MSC_GOOD,  ///< Good news, e.g. the scenario is won or a new attraction is available.
	MSC_INFO,  ///< Informational message, e.g. a ride has been repaired.
	MSC_BAD,   ///< Bad news, e.g. a ride has crashed or the scenario is lost.
};

/** What kind of exra data is associated with a message. */
enum MessageDataType {
	MDT_NONE,           ///< No extra data.
	MDT_GOTO,           ///< Scroll to a location.
	MDT_PARK,           ///< Park management window.
	MDT_RIDE_INSTANCE,  ///< A ride instance's window.
	MDT_RIDE_TYPE,      ///< A ride type in the Ride Select GUI.
	MDT_GUEST,          ///< A guest's window.
};

/** One message. */
class Message {
public:
	explicit Message();
	explicit Message(StringID str, uint32 d1 = 0, uint32 d2 = 0);
	Message(const Message&) = delete;

	void SetStringParameters() const;
	void OnClick() const;

	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	Date timestamp;             ///< The game time when this message was sent.
	MessageCategory category;   ///< Type of this message.
	StringID message;           ///< Message content.
	MessageDataType data_type;  ///< Type of the extra data.
	uint32 data1, data2;        ///< Extra data the message may refer to (the meaning depends on #data_type).
	uint32 *data_for_plural;    ///< The data variable to use for message pluralization (may be \c nullptr).

private:
	void InitMessageDataTypes();
};

/** All the player's messages. */
struct Inbox {
	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	void SendMessage(Message *message);
	void Clear();
	void Tick(uint32 time);
	void DismissDisplayMessage();

	void NotifyRideDeletion(uint16 ride);
	void NotifyGuestDeletion(uint16 guest);

	std::list<std::unique_ptr<Message>> messages;  ///< All messages belonging to the player.
	Message *display_message;                      ///< Message to display in the bottom toolbar (may be \c nullptr).
	uint32 display_time;                           ///< Number of milliseconds for which the #display_message (if any) has been shown.
};
extern Inbox _inbox;

#endif
