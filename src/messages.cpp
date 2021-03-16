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
	this->category = MSG_INFO;
	this->message = STR_NULL;
	this->data1 = 0;
	this->data2 = 0;
	this->data_type = MDT_NONE;
}

/**
 * Common constructor for a message.
 * @param cat Type of this message.
 * @param str The actual message.
 * @param d1 Extra data as required by #str.
 * @param d2 Extra data as required by #str.
 */
Message::Message(MessageCategory cat, StringID str, uint32 d1, uint32 d2)
{
	this->category = cat;
	this->message = str;
	this->data1 = d1;
	this->data2 = d2;
	this->timestamp = _date;
	this->InitMessageDataType();
}

/** Set the message's %MessageDataType from the #message ID. */
void Message::InitMessageDataType()
{
	switch (this->message) {
		case GUI_MESSAGE_SCENARIO_WON:
		case GUI_MESSAGE_SCENARIO_LOST:
			this->data_type = MDT_NONE;
			return;
		case GUI_MESSAGE_BAD_RATING:
			this->data_type = MDT_PARK;
			return;
		case GUI_MESSAGE_GUEST_LOST:
			this->data_type = MDT_GUEST;
			return;
		case GUI_MESSAGE_NEW_RIDE:
			this->data_type = MDT_RIDE_TYPE;
			return;
		case GUI_MESSAGE_CRASH_WITH_DEAD:
		case GUI_MESSAGE_BROKEN_DOWN:
		case GUI_MESSAGE_REPAIRED:
		case GUI_MESSAGE_CRASH_NO_DEAD:
			this->data_type = MDT_RIDE_INSTANCE;
			return;
		default:
			printf("ERROR: Invalid message string %d (%s)\n", this->message, _language.GetText(this->message));
			NOT_REACHED();
	}
}

/**
 * Set the string parameters for this message.
 * @param colour [out] Filled in with the colour index in which this message should be drawn.
 */
void Message::SetStringParametersAndColour(uint8 *colour) const
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

	switch (this->category) {
		case MSG_GOOD: *colour = COL_RANGE_BLUE;   break;
		case MSG_INFO: *colour = COL_RANGE_YELLOW; break;
		case MSG_BAD: *colour = COL_RANGE_RED;     break;

		default:
			printf("ERROR: Invalid message category %d\n", this->category);
			NOT_REACHED();
	}
	/* Conversion from a colour range to a palette index. */
	*colour *= COL_SERIES_LENGTH;
	*colour += COL_SERIES_START + COL_SERIES_LENGTH / 2;
}

void Message::Load(Loader &ldr)
{
	this->category = static_cast<MessageCategory>(ldr.GetByte());
	this->message = ldr.GetWord();
	this->data1 = ldr.GetLong();
	this->data2 = ldr.GetLong();
	this->timestamp = Date(CompressedDate(ldr.GetLong()));
	this->InitMessageDataType();
}

void Message::Save(Saver &svr) const
{
	svr.PutByte(this->category);
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
	// NOCOM update GUI
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
