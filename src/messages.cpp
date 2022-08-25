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
#include "window.h"

/** Default constructor, for loading only. */
Message::Message()
: category(MSC_INFO), message(STR_NULL), data_type(MDT_NONE), data1(0), data2(0), data_for_plural(nullptr)
{
}

/**
 * Common constructor for a message.
 * @param str The actual message.
 * @param d1 Extra data as required by #str.
 * @param d2 Extra data as required by #str.
 */
Message::Message(StringID str, uint32 d1, uint32 d2)
: timestamp(_date), message(str), data1(d1), data2(d2), data_for_plural(nullptr)
{
	this->InitMessageDataTypes();
}

/** Set the message's %MessageDataType and %MessageCategory from the #message ID. */
void Message::InitMessageDataTypes()
{
	switch (this->message) {
		case GUI_MESSAGE_SCENARIO_WON:
		case GUI_MESSAGE_AWARD_WON:
			this->category = MSC_GOOD;
			this->data_type = MDT_NONE;
			return;

		case GUI_MESSAGE_CHEAP_FEE:
			this->category = MSC_GOOD;
			this->data_type = MDT_PARK;
			return;

		case GUI_MESSAGE_SCENARIO_LOST:
		case GUI_MESSAGE_COMPLAIN_HUNGRY:
		case GUI_MESSAGE_COMPLAIN_THIRSTY:
		case GUI_MESSAGE_COMPLAIN_TOILET:
		case GUI_MESSAGE_COMPLAIN_LITTER:
		case GUI_MESSAGE_COMPLAIN_VANDALISM:
		case GUI_MESSAGE_NEGATIVE_AWARD:
			this->category = MSC_BAD;
			this->data_type = MDT_NONE;
			return;

		case GUI_MESSAGE_BAD_RATING:
			this->category = MSC_BAD;
			this->data_type = MDT_PARK;
			this->data_for_plural = &this->data1;
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
			this->data_for_plural = &this->data2;
			/* FALL-THROUGH */
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
			printf("ERROR: Invalid message string %d ('%s')\n", this->message, _language.GetSgText(this->message).c_str());
			NOT_REACHED();
	}
}

/** Set the string parameters for this message. */
void Message::SetStringParameters() const
{
	if (this->data_for_plural != nullptr) _str_params.pluralize_count = *this->data_for_plural;
	switch (this->data_type) {
		case MDT_NONE:
		case MDT_GOTO:
			break;
		case MDT_PARK:
			_str_params.SetNumber(1, this->data1);
			break;
		case MDT_GUEST:
			_str_params.SetText(1, _guests.GetExisting(this->data1)->GetName());
			break;
		case MDT_RIDE_TYPE:
			_str_params.SetStrID(1, _rides_manager.GetRideType(this->data1)->GetTypeName());
			break;
		case MDT_RIDE_INSTANCE:
			_str_params.SetText(1, _rides_manager.GetRideInstance(this->data1)->name);
			_str_params.SetNumber(2, this->data2);
			break;
		default:
			NOT_REACHED();
	}
}

/** The user clicked this message's button. */
void Message::OnClick() const
{
	switch (this->data_type) {
		case MDT_NONE:
			break;
		case MDT_GUEST:
			ShowPersonInfoGui(_guests.GetExisting(this->data1));
			break;
		case MDT_RIDE_INSTANCE:
			ShowRideManagementGui(this->data1);
			break;
		case MDT_RIDE_TYPE:
			ShowRideSelectGui();  // \todo Pre-select the ride type indicated by the message.
			break;
		case MDT_PARK:
			/* \todo Implement showing park management GUI. */
			break;
		case MDT_GOTO:
			/* \todo Move the main view to the coordinates indicated by the message. */
			break;
	}
}

static const uint32 CURRENT_VERSION_INBX = 1;     ///< Currently supported version of the INBX Pattern.
static const uint32 CURRENT_VERSION_Message = 1;  ///< Currently supported version of the %Message Pattern.

void Message::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("mssg");
	if (version != CURRENT_VERSION_Message) ldr.version_mismatch(version, CURRENT_VERSION_Message);

	this->message = GUI_INBOX_TITLE + ldr.GetWord();
	this->data1 = ldr.GetLong();
	this->data2 = ldr.GetLong();
	this->timestamp = Date(CompressedDate(ldr.GetLong()));
	this->InitMessageDataTypes();
	ldr.ClosePattern();
}

void Message::Save(Saver &svr) const
{
	svr.StartPattern("mssg", CURRENT_VERSION_Message);
	svr.PutWord(this->message - GUI_INBOX_TITLE);
	svr.PutLong(this->data1);
	svr.PutLong(this->data2);
	svr.PutLong(this->timestamp.Compress());
	svr.EndPattern();
}

/** Reset the inbox to a clean state. */
void Inbox::Clear()
{
	this->display_time = 0;
	this->display_message = nullptr;
	this->messages.clear();
}

/**
 * Add a message to the inbox and notify the player.
 * @param message Message to send.
 */
void Inbox::SendMessage(Message *message)
{
	this->messages.push_back(std::unique_ptr<Message>(message));
	if (this->display_message == nullptr) {
		this->display_time = 0;
		this->display_message = message;
	}
}

/**
 * Some time has passed.
 * @param time Number of milliseconds realtime since the last call to this method.
 */
void Inbox::Tick(const uint32 time)
{
	if (this->display_message == nullptr) return;
	this->display_time += time;
	if (this->display_time > 10000) this->DismissDisplayMessage();  // Automatically dismiss messages after an arbitrary timeout of 10 seconds.
}

/** Dismiss the display message, and show the next one if applicable. */
void Inbox::DismissDisplayMessage()
{
	this->display_time = 0;
	bool found = false, changed = false;
	for (auto &msg : this->messages) {
		if (found) {
			display_message = msg.get();
			changed = true;
			break;
		}
		found = (msg.get() == display_message);
	}
	if (!changed) this->display_message = nullptr;
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}

/**
 * Notification that a ride is being removed.
 * @param ri Ride being removed.
 */
void Inbox::NotifyRideDeletion(const uint16 ride)
{
	for (auto it = this->messages.begin(); it != this->messages.end();) {
		if ((*it)->data_type == MDT_RIDE_INSTANCE && (*it)->data1 == ride) {
			if (it->get() == this->display_message) this->DismissDisplayMessage();
			it = this->messages.erase(it);
		} else {
			it++;
		}
	}
}

/**
 * Notification that a guest is being removed.
 * @param ri Guest being removed.
 */
void Inbox::NotifyGuestDeletion(const uint16 guest)
{
	for (auto it = this->messages.begin(); it != this->messages.end();) {
		if ((*it)->data_type == MDT_GUEST && (*it)->data1 == guest) {
			if (it->get() == this->display_message) this->DismissDisplayMessage();
			it = this->messages.erase(it);
		} else {
			it++;
		}
	}
}

void Inbox::Load(Loader &ldr)
{
	this->Clear();
	const uint32 version = ldr.OpenPattern("INBX");
	switch (version) {
		case 0:
			break;
		case 1:
			for (long i = ldr.GetLong(); i > 0; i--) {
				Message *m = new Message;
				m->Load(ldr);
				this->messages.push_back(std::unique_ptr<Message>(m));
			}
			break;

		default:
			ldr.version_mismatch(version, CURRENT_VERSION_INBX);
	}
	ldr.ClosePattern();
}

void Inbox::Save(Saver &svr) const
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("INBX", CURRENT_VERSION_INBX);
	svr.PutLong(this->messages.size());
	for (const auto &m : this->messages) {
		m->Save(svr);
	}
	svr.EndPattern();
}

Inbox _inbox;
