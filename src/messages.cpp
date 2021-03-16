/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file messages.cpp Implementation of the inbox messages system. */

#include "messages.h"
#include "person.h"
#include "people.h"
#include "ride_type.h"

/** Default constructor, for loading only. */
Message::Message()
{
	this->message = STR_NULL;
	this->data1 = 0;
	this->data2 = 0;
	this->category = MSC_INFO;
	this->data_type = MDT_NONE;
}

/**
 * Common constructor for a message.
 * @param str The actual message.
 * @param d1 Extra data as required by #str.
 * @param d2 Extra data as required by #str.
 */
Message::Message(StringID str, uint32 d1, uint32 d2)
{
	this->message = str;
	this->data1 = d1;
	this->data2 = d2;
	this->timestamp = _date;
	this->InitMessageDataTypes();
}

/** Set the message's %MessageDataType and %MessageCategory from the #message ID. */
void Message::InitMessageDataTypes()
{
	switch (this->message) {
		case GUI_MESSAGE_SCENARIO_WON:
		case GUI_MESSAGE_CHEAP_FEE:
		case GUI_MESSAGE_AWARD_WON:
			this->category = MSC_GOOD;
			this->data_type = MDT_NONE;
			return;

		case GUI_MESSAGE_SCENARIO_LOST:
		case GUI_MESSAGE_COMPLAIN_HUNGRY:
		case GUI_MESSAGE_COMPLAIN_THIRSTY:
		case GUI_MESSAGE_COMPLAIN_TOILET:
		case GUI_MESSAGE_COMPLAIN_LITTER:
		case GUI_MESSAGE_NEGATIVE_AWARD:
			this->category = MSC_BAD;
			this->data_type = MDT_NONE;
			return;

		case GUI_MESSAGE_BAD_RATING:
			this->category = MSC_BAD;
			this->data_type = MDT_PARK;
			return;

		case GUI_MESSAGE_GUEST_LOST:
			this->category = MSC_INFO;
			this->data_type = MDT_GUEST;
			return;

		case GUI_MESSAGE_NEW_RIDE:
			this->category = MSC_GOOD;
			this->data_type = MDT_RIDE_TYPE;
			return;

		case GUI_MESSAGE_CRASH_WITH_DEAD:
		case GUI_MESSAGE_CRASH_NO_DEAD:
			this->category = MSC_BAD;
			this->data_type = MDT_RIDE_INSTANCE;
			return;

		case GUI_MESSAGE_BROKEN_DOWN:
		case GUI_MESSAGE_REPAIRED:
		case GUI_MESSAGE_COMPLAIN_QUEUE:
			this->category = MSC_INFO;
			this->data_type = MDT_RIDE_INSTANCE;
			return;

		default:
			printf("ERROR: Invalid message string %d (%s)\n", this->message, _language.GetText(this->message));
			NOT_REACHED();
	}
}

/** Set the string parameters for this message. */
void Message::SetStringParameters() const
{
	switch (this->data_type) {
		case MDT_NONE:
		case MDT_GOTO:
			break;
		case MDT_PARK:
			_str_params.SetNumber(1, this->data1);
			break;
		case MDT_GUEST:
			_str_params.SetUint8(1, _guests.Get(this->data1)->GetName());
			break;
		case MDT_RIDE_TYPE:
			_str_params.SetStrID(1, _rides_manager.GetRideType(this->data1)->GetTypeName());
			break;
		case MDT_RIDE_INSTANCE:
			_str_params.SetUint8(1, _rides_manager.GetRideInstance(this->data1)->name.get());
			_str_params.SetNumber(2, this->data2);
			break;
		default:
			printf("ERROR: Invalid message type %d\n", this->data_type);
			NOT_REACHED();
	}
}

void Message::Load(Loader &ldr)
{
	this->message = ldr.GetWord();
	this->data1 = ldr.GetLong();
	this->data2 = ldr.GetLong();
	this->timestamp = Date(CompressedDate(ldr.GetLong()));
	this->InitMessageDataTypes();
}

void Message::Save(Saver &svr) const
{
	svr.PutWord(this->message);
	svr.PutLong(this->data1);
	svr.PutLong(this->data2);
	svr.PutLong(this->timestamp.Compress());
}

/** Reset the inbox to a clean state. */
void Inbox::Clear()
{
	this->messages.clear();
}

/**
 * Add a message to the inbox and notify the player.
 * @param message Message to send.
 */
void Inbox::SendMessage(const Message &message)
{
	this->messages.push_back(message);
	/* The GUI will update itself. */
}

/**
 * Notification that the ride is being removed.
 * @param ri Ride being removed.
 */
void Inbox::NotifyRideDeletion(const uint16 ride)
{
	for (auto it = this->messages.begin(); it != this->messages.end();) {
		if (it->data_type == MDT_RIDE_INSTANCE && it->data1 == ride) {
			it = this->messages.erase(it);
		} else {
			it++;
		}
	}
}

/**
 * Notification that the guest is being removed.
 * @param ri Guest being removed.
 */
void Inbox::NotifyGuestDeletion(const uint16 guest)
{
	for (auto it = this->messages.begin(); it != this->messages.end();) {
		if (it->data_type == MDT_GUEST && it->data1 == guest) {
			it = this->messages.erase(it);
		} else {
			it++;
		}
	}
}

void Inbox::Load(Loader &ldr)
{
	this->Clear();
	for (long i = ldr.GetLong(); i > 0; i--) {
		Message m;
		m.Load(ldr);
		this->messages.push_back(m);
	}
}

void Inbox::Save(Saver &svr) const
{
	svr.PutLong(this->messages.size());
	for (const Message &m : this->messages) {
		m.Save(svr);
	}
}

Inbox _inbox;
