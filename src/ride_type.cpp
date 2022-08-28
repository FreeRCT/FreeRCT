/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_type.cpp Ride type storage and retrieval. */

#include "stdafx.h"
#include "string_func.h"
#include "sprite_store.h"
#include "language.h"
#include "sprite_store.h"
#include "map.h"
#include "fileio.h"
#include "ride_type.h"
#include "bitmath.h"
#include "gamelevel.h"
#include "window.h"
#include "finances.h"
#include "messages.h"
#include "person.h"
#include "people.h"
#include "random.h"
#include "generated/entrance_exit_strings.h"
#include "generated/entrance_exit_strings.cpp"

/**
 * \page Rides
 *
 * Rides are the central concept in what guests 'do' to have fun.
 * The main classes are #RideType and #RideInstance.
 *
 * - The #RideType represents the type of a ride, e.g. "the kiosk" or a "basic steel roller coaster".
 *   - Shop types are implement in #ShopType.
 *   - Gentle ride types and thrill ride types are implemented in #GentleThrillRideType.
 *   - Coaster types are implemented in #CoasterType.
 *
 * - The #RideInstance represents actual rides in the park.
 *   - Shops instances are implemented in #ShopInstance.
 *   - Gentle ride instances and thrill ride instances are implemented in #GentleThrillRideInstance.
 *   - Coasters instances are implemented in #CoasterInstance.
 *
 * The #RidesManager (#_rides_manager) manages both ride types and ride instances.
 */

RidesManager _rides_manager; ///< Storage and retrieval of ride types and rides in the park.

/* \todo Move these constants to the RCD files. */
uint8 RideEntranceExitType::entrance_height = 4;
uint8 RideEntranceExitType::exit_height = 3;

RideEntranceExitType::RideEntranceExitType()
{
	/* Nothing to do currently. */
}

/**
 * Load a type of ride entrance or exit from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 */
bool RideEntranceExitType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	if (rcd_file->version != 1 || rcd_file->size != 51) return false;
	this->is_entrance = rcd_file->GetUInt8() > 0;

	TextData *text_data;
	if (!LoadTextFromFile(rcd_file, texts, &text_data)) return false;
	StringID base = _language.RegisterStrings(*text_data, _entrance_exit_strings_table);
	this->name = base + (ENTRANCE_EXIT_NAME - STR_GENERIC_ENTRANCE_EXIT_START);
	this->recolour_description_1 = base + (ENTRANCE_EXIT_DESCRIPTION_RECOLOUR1 - STR_GENERIC_ENTRANCE_EXIT_START);
	this->recolour_description_2 = base + (ENTRANCE_EXIT_DESCRIPTION_RECOLOUR2 - STR_GENERIC_ENTRANCE_EXIT_START);
	this->recolour_description_3 = base + (ENTRANCE_EXIT_DESCRIPTION_RECOLOUR3 - STR_GENERIC_ENTRANCE_EXIT_START);

	const int width = rcd_file->GetUInt16();
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			ImageData *view;
			if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
			if (width != 64) continue; /// \todo Widths other than 64.
			this->images[i][j] = view;
		}
	}
	for (int i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		this->recolours.Set(i, RecolourEntry(recolour));
	}
	return true;
}

/**
 * Ride type base class constructor.
 * @param rtk Kind of ride.
 */
RideType::RideType(RideTypeKind rtk) : kind(rtk),
	monthly_cost(12345),       // Arbitrary non-zero cost.
	monthly_open_cost(12345),  // Arbitrary non-zero cost.
	reliability_max(RELIABILITY_RANGE),
	reliability_decrease_daily(0),
	reliability_decrease_monthly(0)
{
	std::fill_n(this->item_type, NUMBER_ITEM_TYPES_SOLD, ITP_NOTHING);
	std::fill_n(this->item_cost, NUMBER_ITEM_TYPES_SOLD, 12345); // Artbitary non-zero cost.
	this->SetupStrings(nullptr, 0, 0, 0, 0, 0);
}

/**
 * Are all resources available for building an instance of this type?
 * For example, are all sprites available? Default implementation always allows building an instance.
 * @return Whether an instance of the ride type can be created.
 */
bool RideType::CanMakeInstance() const
{
	return true;
}

/**
 * \fn RideInstance *RideType::CreateInstance()
 * Construct a ride instance of the ride type.
 * @return An ride of its own type.
 */

/**
 * \fn const ImageData *RideType::GetView(uint8 orientation) const
 * Get a display of the ride type for the purchase screen.
 * @param orientation Orientation of the ride type. Many ride types have \c 4 view orientations,
 *                    but some types may have only a view for orientation \c 0.
 * @return An image to display in the purchase window, or \c nullptr if the queried orientation has no view.
 */

/**
 * \fn const StringID *RideType::GetInstanceNames() const
 * Get the instance base names of rides.
 * @return Array of proposed names, terminated with #STR_INVALID.
 */

/**
 * Setup the the strings of the ride type.
 * @param text Strings from the RCD file.
 * @param base Assigned base number for strings of this type.
 * @param start First generic string number of this type.
 * @param end One beyond the last generic string number of this type.
 * @param name String containing the name of this ride type.
 * @param desc String containing the description of this ride type.
 */
void RideType::SetupStrings(TextData *text, StringID base, StringID start, StringID end, StringID name, StringID desc)
{
	this->text = text;
	this->str_base = base;
	this->str_start = start;
	this->str_end = end;
	this->str_name = name;
	this->str_description = desc;
}

/**
 * Retrieve the string with the name of this type of ride.
 * @return The name of this ride type.
 */
StringID RideType::GetTypeName() const
{
	return this->str_name;
}

/**
 * Retrieve the string with the description of this type of ride.
 * @return The description of this ride type.
 */
StringID RideType::GetTypeDescription() const
{
	return this->str_description;
}

/**
 * Get the string instance for the generic ride string of \a number.
 * @param number Generic ride string number to retrieve.
 * @return The instantiated string for this ride type.
 */
StringID RideType::GetString(uint16 number) const
{
	assert(number >= this->str_start && number < this->str_end);
	return this->str_base + (number - this->str_start);
}

/**
 * Sets the string parameters for a ride rating attribute (excitement, intensity, or nausea).
 * @param rating Value of the ride's rating attribute.
 */
void SetRideRatingStringParam(const uint32 rating)
{
	if (rating == RATING_NOT_YET_CALCULATED) {
		_str_params.SetStrID(1, GUI_RIDE_MANAGER_RATING_NOT_YET_CALCULATED);
		return;
	}

	std::string str;

	if (rating < 100) {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_VERY_LOW);
	} else if (rating < 250) {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_LOW);
	} else if (rating < 500) {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_MEDIUM);
	} else if (rating < 750) {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_HIGH);
	} else if (rating < 1050) {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_VERY_HIGH);
	} else {
		str = _language.GetSgText(GUI_RIDE_MANAGER_RATING_EXTREME);
	}

	_str_params.SetText(1, Format(str, rating / 100.0));
}

/**
 * Constructor of the base ride instance class.
 * @param rt Type of the ride instance.
 */
RideInstance::RideInstance(const RideType *rt)
:
	state(RIS_ALLOCATED),
	flags(0),
	recolours(rt->recolours),
	max_reliability(rt->reliability_max),
	reliability(max_reliability),
	maintenance_interval(30 * 60 * 1000),  // Half an hour by default.
	time_since_last_maintenance(0),
	broken(false),
	time_since_last_long_queue_message(0),
	excitement_rating(RATING_NOT_YET_CALCULATED),
	intensity_rating(RATING_NOT_YET_CALCULATED),
	nausea_rating(RATING_NOT_YET_CALCULATED),
	type(rt),
	mechanic_pending(false)
{
	this->SetEntranceType(0);
	this->SetExitType(0);

	this->recolours.AssignRandomColours();
	this->entrance_recolours.AssignRandomColours();
	this->exit_recolours.AssignRandomColours();

	std::fill_n(this->item_price, NUMBER_ITEM_TYPES_SOLD, 12345); // Arbitrary non-zero amount.
	std::fill_n(this->item_count, NUMBER_ITEM_TYPES_SOLD, 0);
}

/**
 * Get the kind of the ride.
 * @return the kind of the ride.
 */
RideTypeKind RideInstance::GetKind() const
{
	return this->type->kind;
}

/**
 * Get the ride type of the instance.
 * @return The ride type.
 */
const RideType *RideInstance::GetRideType() const
{
	return this->type;
}

/**
 * \fn void RideInstance::GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const
 * Get the sprites to display for the provided voxel number.
 * @param vox The voxel's absolute coordinates.
 * @param voxel_number Number of the voxel to draw (copied from the world voxel data).
 * @param orient View orientation.
 * @param sprites [out] Sprites to draw, from back to front, #SO_PLATFORM_BACK, #SO_RIDE, #SO_RIDE_FRONT, and #SO_PLATFORM_FRONT.
 * @param platform [out] Shape of the support platform, if needed. @see PathSprites
 */

/**
 * \fn uint8 RideInstance::GetEntranceDirections(const XYZPoint16 &vox) const
 * Get the set edges with an entrance to the ride.
 * @param vox Position of the voxel with the ride.
 * @return Bit set of #TileEdge that allows entering the ride (seen from the ride).
 */

/**
 * \fn RideEntryResult RideInstance::EnterRide(int guest, const XYZPoint16 &vox, TileEdge entry_edge)
 * The given guest tries to enter the ride. Meaning and further handling of the ride visit depends on the return value.
 * - If the call returns #RER_REFUSED, the guest is not given entry (ride is full), it should be tried again at another time.
 * - If the call returns #RER_ENTERED, the guest is given access, and is now staying in the ride. The ride calls Guest::ExitRide when it is done.
 * - If the call returns #RER_DONE, the guest is given access, and the ride is also immediately visited (Guest::ExitRide is called before returning this answer).
 * @param guest Number of the guest entering the ride.
 * @param vox Position of the voxel with the ride.
 * @param entry_edge Edge of entering the ride. Value is passed back through Guest::ExitRide, to use as parameter in RideInstance::GetExit.
 * @return Whether the guest is accepted, and if so, if the guest is staying inside.
 */

/**
 * \fn XYZPoint32 RideInstance::GetExit(int guest, TileEdge entry_edge)
 * Get the exit coordinates of the ride, is near the middle of a tile edge.
 * @param guest Number of the guest querying the exit coordinates.
 * @param entry_edge %Edge used for entering the ride.
 * @return World position of the exit.
 */

/**
 * \fn void RideInstance::RemoveAllPeople()
 * Immediately remove all guests and staff which is inside the ride.
 */

/**
 * \fn void RideInstance::RemoveFromWorld()
 * Immediately remove this ride from all voxels it currently occupies.
 */

/**
 * \fn void RideInstance::InsertIntoWorld()
 * Ensure that this shop is linked into the voxels it is meant to occupy.
 */

/**
 * Can the ride be visited, assuming it is approached from direction \a edge?
 * @param vox Position of the voxel with the ride.
 * @param edge Direction of movement (exit direction of the neighbouring voxel).
 * @return Whether the ride can be visited.
 * @note Derived classes need to override this function and perform additional checks regarding the location's suitability.
 */
bool RideInstance::CanBeVisited([[maybe_unused]] const XYZPoint16 &vox, [[maybe_unused]] TileEdge edge) const
{
	return this->state == RIS_OPEN && !this->broken;
}

/**
 * The recolouring map to apply to this ride at the given position.
 * @param pos Position in the world.
 * @return Recoluring to use.
 */
const Recolouring *RideInstance::GetRecolours([[maybe_unused]] const XYZPoint16 &pos) const
{
	return &this->recolours;
}

/**
 * Whether the ride's entrance should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An entrance is located at the given location.
 */
bool RideInstance::IsEntranceLocation([[maybe_unused]] const XYZPoint16 &pos) const
{
	return false;
}

/**
 * Whether the ride's exit should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An exit is located at the given location.
 */
bool RideInstance::IsExitLocation([[maybe_unused]] const XYZPoint16 &pos) const
{
	return false;
}

/**
 * \fn EdgeCoordinate RideInstance::GetMechanicEntrance() const
 * The voxel and edge at which a mechanic interacts with the ride for maintenance and repairs.
 */

/**
 * \fn void RideInstance::RecalculateRatings()
 * Update the excitement, intensity, and nausea rating stats.
 */

/**
 * Sell an item to a customer.
 * @param item_index Index of the item being sold.
 */
void RideInstance::SellItem(int item_index)
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);

	const Money cost = this->GetSaleItemCost(item_index);
	const Money price = this->GetSaleItemPrice(item_index);
	this->item_count[item_index]++;
	const Money profit = price - cost;
	this->total_sell_profit += profit;
	this->total_profit += profit;

	if (this->GetKind() == RTK_SHOP) {
		_finances_manager.PayShopStock(cost);
		_finances_manager.EarnShopSales(price);
		NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
	} else {
		_finances_manager.EarnRideTickets(price);
		NotifyChange(this->GetKind() == RTK_COASTER ? WC_COASTER_MANAGER : WC_GENTLE_THRILL_RIDE_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
	}
}

/**
 * Get the type of items sold by a ride.
 * @param item_index Index in the items being sold (\c 0 to #NUMBER_ITEM_TYPES_SOLD).
 * @return Type of item sold.
 */
ItemType RideInstance::GetSaleItemType(int item_index) const
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);
	return this->type->item_type[item_index];
}

/**
 * Get the price of an item sold by a ride.
 * @param item_index Index in the items being sold (\c 0 to #NUMBER_ITEM_TYPES_SOLD).
 * @return Price to pay for the item.
 */
Money RideInstance::GetSaleItemPrice(int item_index) const
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);
	return this->item_price[item_index];
}

/**
 * Get the cost of an item sold by a ride.
 * @param item_index Index in the items being sold (\c 0 to #NUMBER_ITEM_TYPES_SOLD).
 * @return Cost of selling the item.
 */
Money RideInstance::GetSaleItemCost(int item_index) const
{
	return this->type->item_cost[item_index];
}

/** Initialize the prices of all sold items with default values, and reset the profit statistics. */
void RideInstance::InitializeItemPricesAndStatistics()
{
	this->total_profit = 0;
	this->total_sell_profit = 0;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		this->item_price[i] = GetRideType()->item_cost[i] * 12 / 10; // Make 20% profit.
		this->item_count[i] = 0;
	}
}

/**
 * Change the ride's entrance type.
 * @param type Index of the new entrance.
 */
void RideInstance::SetEntranceType(const int type)
{
	this->entrance_type = type;
	this->entrance_recolours = _rides_manager.entrances[type]->recolours;
}

/**
 * Change the ride's exit type.
 * @param type Index of the new exit.
 */
void RideInstance::SetExitType(const int type)
{
	this->exit_type = type;
	this->exit_recolours = _rides_manager.exits[type]->recolours;
}

/**
 * Whether a path edge to/from this ride should be drawn at the given location.
 * @param vox Coordinates in the world.
 * @param edge Tile edge to check.
 * @return A path edge should be added.
 */
bool RideInstance::PathEdgeWanted(const XYZPoint16 &vox, const TileEdge edge) const
{
	return (_world.GetVoxel(vox)->GetInstanceData() & (1 << edge)) != 0;
}

/**
 * Some time has passed, update the state of the ride.
 * @param delay Number of milliseconds that passed.
 */
void RideInstance::OnAnimate(const int delay)
{
	if (this->state != RIS_OPEN) return;
	this->time_since_last_maintenance += delay;
	this->time_since_last_long_queue_message += delay;
	if (this->maintenance_interval > 0 && this->time_since_last_maintenance > this->maintenance_interval) this->CallMechanic();
}

/** Monthly update of the shop administration and ride reliability. */
void RideInstance::OnNewMonth()
{
	this->total_profit -= this->type->monthly_cost;
	_finances_manager.PayStaffWages(this->type->monthly_cost);
	SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
	if (this->state == RIS_OPEN) {
		this->total_profit -= this->type->monthly_open_cost;
		_finances_manager.PayStaffWages(this->type->monthly_open_cost);
		SB(this->flags, RIF_OPENED_PAID, 1, 1);
	} else {
		SB(this->flags, RIF_OPENED_PAID, 1, 0);
	}

	this->max_reliability = (RELIABILITY_RANGE - type->reliability_decrease_monthly) * this->max_reliability / RELIABILITY_RANGE;
	this->reliability = std::min(this->reliability, this->max_reliability);

	NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/** Daily update of reliability and breakages. */
void RideInstance::OnNewDay()
{
	if (this->state == RIS_OPEN) {
		this->reliability = (RELIABILITY_RANGE - type->reliability_decrease_daily) * this->reliability / RELIABILITY_RANGE;
		if (!this->broken) {
			int breakdown_chance = 0;
			for (int i = 0; i < 5; i++) {
				if (this->rnd.Uniform(RELIABILITY_RANGE) > this->reliability) breakdown_chance++;
			}
			if (breakdown_chance >= 3) this->BreakDown();
		}
	}
}

/** Cause the ride to break down now. */
void RideInstance::BreakDown()
{
	if (this->broken) return;
	this->broken = true;
	_inbox.SendMessage(new Message(GUI_MESSAGE_BROKEN_DOWN, this->GetIndex()));
	this->CallMechanic();
}

/** Request a mechanic to inspect or repair this ride. */
void RideInstance::CallMechanic()
{
	if (this->mechanic_pending) return;
	_staff.RequestMechanic(this);
	this->mechanic_pending = true;
}

/** Callback when a mechanic arrived to repair or inspect this ride. */
void RideInstance::MechanicArrived()
{
	assert(this->mechanic_pending);
	if (this->broken) _inbox.SendMessage(new Message(GUI_MESSAGE_REPAIRED, this->GetIndex()));
	this->broken = false;
	this->time_since_last_maintenance = 0;
	this->reliability = this->max_reliability;
	this->mechanic_pending = false;
}

/**
 * Check whether the ride can be opened.
 * @return The ride can be opened.
 */
bool RideInstance::CanOpenRide() const
{
	return this->state == RIS_CLOSED || this->state == RIS_TESTING;
}

/**
 * Open the ride for the public.
 * @pre The ride is open.
 */
void RideInstance::OpenRide()
{
	assert(this->CanOpenRide());
	this->state = RIS_OPEN;
	this->time_since_last_long_queue_message = 0;

	/* Perform payments if they have not been done this month. */
	bool money_paid = false;
	if (GB(this->flags, RIF_MONTHLY_PAID, 1) == 0) {
		this->total_profit -= this->type->monthly_cost;
		_finances_manager.PayStaffWages(this->type->monthly_cost);
		SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
		money_paid = true;
	}
	if (GB(this->flags, RIF_OPENED_PAID, 1) == 0) {
		this->total_profit -= this->type->monthly_open_cost;
		_finances_manager.PayStaffWages(this->type->monthly_open_cost);
		SB(this->flags, RIF_OPENED_PAID, 1, 1);
		money_paid = true;
	}
	if (money_paid) NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/**
 * Close the ride for the public.
 * @todo Currently closing is instantly, we may want to have a transition phase here.
 * @pre The ride is open.
 */
void RideInstance::CloseRide()
{
	this->state = RIS_CLOSED;
	this->RemoveAllPeople();
}

/**
 * Switch the ride to being constructed.
 * @pre The ride should not be open.
 */
void RideInstance::BuildRide()
{
	assert(this->state != RIS_OPEN);
	this->state = RIS_BUILDING;
}

/** Inform this ride that the queue is very long. This might send a message to the player. */
void RideInstance::NotifyLongQueue()
{
	if (this->state != RIS_OPEN || this->broken) return;
	if (this->time_since_last_long_queue_message > 10 * 60 * 1000) {  // Arbitrary threshold of 10 minutes to ensure that this notification is not repeated too often.
		this->time_since_last_long_queue_message = 0;
		_inbox.SendMessage(new Message(GUI_MESSAGE_COMPLAIN_QUEUE, this->GetIndex()));
	}
}

static const uint32 CURRENT_VERSION_RideInstance = 1;   ///< Currently supported version of %RideInstance.

void RideInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("ride");
	if (version != CURRENT_VERSION_RideInstance) ldr.version_mismatch(version, CURRENT_VERSION_RideInstance);

	this->name = ldr.GetText();

	uint16 state_and_flags = ldr.GetWord();
	this->state = static_cast<RideInstanceState>(state_and_flags >> 8);
	this->flags = state_and_flags & 0xff;
	this->SetEntranceType(ldr.GetWord());
	this->SetExitType(ldr.GetWord());
	this->recolours.Load(ldr);
	this->entrance_recolours.Load(ldr);
	this->exit_recolours.Load(ldr);
	for (Money& m : this->item_price) m = static_cast<Money>(ldr.GetLongLong());
	for (int64& m : this->item_count) m = ldr.GetLongLong();
	this->total_profit = static_cast<Money>(ldr.GetLongLong());
	this->total_sell_profit = static_cast<Money>(ldr.GetLongLong());
	this->reliability = ldr.GetWord();
	this->max_reliability = ldr.GetWord();
	this->maintenance_interval = ldr.GetLong();
	this->time_since_last_maintenance = ldr.GetLong();
	this->broken = ldr.GetByte() > 0;
	this->mechanic_pending = ldr.GetByte() > 0;
	this->time_since_last_long_queue_message = ldr.GetLong();
	this->excitement_rating = ldr.GetLong();
	this->intensity_rating = ldr.GetLong();
	this->nausea_rating = ldr.GetLong();
	ldr.ClosePattern();
}

void RideInstance::Save(Saver &svr)
{
	svr.StartPattern("ride", CURRENT_VERSION_RideInstance);

	svr.PutText(this->name);
	svr.PutWord((static_cast<uint16>(this->state) << 8) | this->flags);
	svr.PutWord(this->entrance_type);
	svr.PutWord(this->exit_type);
	this->recolours.Save(svr);
	this->entrance_recolours.Save(svr);
	this->exit_recolours.Save(svr);
	for (const Money& m : this->item_price) svr.PutLongLong(static_cast<uint64>(m));
	for (const int64& m : this->item_count) svr.PutLongLong(m);
	svr.PutLongLong(static_cast<uint64>(this->total_profit));
	svr.PutLongLong(static_cast<uint64>(this->total_sell_profit));
	svr.PutWord(this->reliability);
	svr.PutWord(this->max_reliability);
	svr.PutLong(this->maintenance_interval);
	svr.PutLong(this->time_since_last_maintenance);
	svr.PutByte(this->broken ? 1 : 0);
	svr.PutByte(this->mechanic_pending ? 1 : 0);
	svr.PutLong(this->time_since_last_long_queue_message);
	svr.PutLong(this->excitement_rating);
	svr.PutLong(this->intensity_rating);
	svr.PutLong(this->nausea_rating);
	svr.EndPattern();
}

/**
 * Some time has passed, update the state of the rides.
 * @param delay Number of milliseconds that passed.
 */
void RidesManager::OnAnimate(int delay)
{
	for (auto &pair : this->instances) {
		if (pair.second->state == RIS_ALLOCATED) continue;
		pair.second->OnAnimate(delay);
	}
}

/** A new month has started; perform monthly payments. */
void RidesManager::OnNewMonth()
{
	for (auto &pair : this->instances) {
		if (pair.second->state == RIS_ALLOCATED) continue;
		pair.second->OnNewMonth();
	}
}

/** A new day has started; break rides randomly. */
void RidesManager::OnNewDay()
{
	for (auto &pair : this->instances) {
		if (pair.second->state == RIS_ALLOCATED) continue;
		pair.second->OnNewDay();
	}
}

static const uint32 CURRENT_VERSION_RIDS = 3;   ///< Currently supported version of the RIDS Pattern.

void RidesManager::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("RIDS");

	if (version >= 1 && version <= CURRENT_VERSION_RIDS) {
		uint16 allocated_ride_count = ldr.GetWord();
		for (uint16 i = 0; i < allocated_ride_count; i++) {
			const RideType *ride_type = nullptr;
			const uint16 index = (version >= 3) ? (ldr.GetWord() + SRI_FULL_RIDES) : INVALID_RIDE_INSTANCE;

			RideTypeKind ride_kind = static_cast<RideTypeKind>(ldr.GetByte());

			if (version >= 2) {
				ride_type = this->GetRideType(ldr.GetWord());
			} else {
				const std::string ride_type_name = ldr.GetText();
				if (ride_type_name.empty()) throw LoadingError("Invalid ride type name.");

				for (const auto &rt : _rides_manager.ride_types) {
					const std::string tmp_name = _language.GetSgText(rt->GetString(rt->GetTypeName()));
					if (ride_kind == rt->kind && tmp_name == ride_type_name) {
						ride_type = rt.get();
						break;
					}
				}
			}

			if (ride_type == nullptr || ride_type->kind != ride_kind) {
				throw LoadingError("Unknown/invalid ride type.");
			}

			RideInstance *r = this->CreateInstance(ride_type, index != INVALID_RIDE_INSTANCE ? index : this->GetFreeInstance(ride_type));
			r->Load(ldr);
		}
	} else if (version != 0) {
		ldr.version_mismatch(version, CURRENT_VERSION_RIDS);
	}
	ldr.ClosePattern();
}

void RidesManager::Save(Saver &svr)
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("RIDS", CURRENT_VERSION_RIDS);

	std::set<uint16> allocated_ride_indexes;
	for (const auto &pair : this->instances) {
		if (pair.second->state != RIS_ALLOCATED) {
			allocated_ride_indexes.insert(pair.first);
		}
	}
	svr.PutWord(allocated_ride_indexes.size());

	for (size_t index : allocated_ride_indexes) {
		svr.PutWord(index);
		RideInstance *r = this->instances.at(index).get();
		svr.PutByte(static_cast<uint8>(r->GetKind()));
		svr.PutWord(this->FindRideType(r->GetRideType()));
		r->Save(svr);
	}

	svr.EndPattern();
}

/**
 * Get the requested ride instance.
 * @param num Ride number to retrieve.
 * @return The requested ride, or \c nullptr if not available.
 */
RideInstance *RidesManager::GetRideInstance(uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	const auto it = this->instances.find(num - SRI_FULL_RIDES);
	return (it == this->instances.end()) ? nullptr : it->second.get();
}

/**
 * Get the requested ride instance (read-only).
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c nullptr if not available.
 */
const RideInstance *RidesManager::GetRideInstance(uint16 num) const
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	const auto it = this->instances.find(num - SRI_FULL_RIDES);
	return (it == this->instances.end()) ? nullptr : it->second.get();
}

/**
 * Get the ride instance index number.
 * @return Ride instance index.
 */
uint16 RideInstance::GetIndex() const
{
	for (const auto &pair : _rides_manager.instances) {
		if (pair.second.get() == this) {
			return pair.first + SRI_FULL_RIDES;
		}
	}
	NOT_REACHED();
}

/**
 * Add a new ride type to the manager.
 * @param type New ride type to add.
 * @note Takes ownership of the pointer and clears the passed smart pointer.
 */
void RidesManager::AddRideType(std::unique_ptr<RideType> type)
{
	this->ride_types.emplace_back(std::move(type));
}

/**
 * Add a new ride entrance or exit type to the manager.
 * @param type New ride entrance/exit type to add.
 * @note Takes ownership of the pointer and clears the passed smart pointer.
 */
void RidesManager::AddRideEntranceExitType(std::unique_ptr<RideEntranceExitType> &type)
{
	(type->is_entrance ? this->entrances : this->exits).emplace_back(std::move(type));
}

/**
 * Check whether the ride type can actually be created.
 * @param type Ride type that should be created.
 * @return Index of a free instance if it exists (claim it immediately using #CreateInstance), or #INVALID_RIDE_INSTANCE if all instances are used.
 */
uint16 RidesManager::GetFreeInstance(const RideType *type)
{
	if (!type->CanMakeInstance()) return INVALID_RIDE_INSTANCE;
	if (this->instances.empty()) return SRI_FULL_RIDES;
	return this->instances.rbegin()->first + 1 + SRI_FULL_RIDES;  // In a non-empty map, rbegin() always holds the highest key.
}

/**
 * Create a new ride instance.
 * @param type Type of ride to construct.
 * @param num Ride number.
 * @return The created ride.
 */
RideInstance *RidesManager::CreateInstance(const RideType *type, uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	assert(this->instances.count(num) == 0);
	this->instances.emplace(num, std::unique_ptr<RideInstance>(type->CreateInstance()));
	return this->instances.at(num).get();
}

/**
 * Get the requested ride type's ID.
 * @param r Ride to look up.
 * @return The ride's index.
 */
uint16 RidesManager::FindRideType(const RideType *r) const
{
	for (uint i = 0; i < this->ride_types.size(); i++) {
		if (this->ride_types[i].get() == r) {
			return i;
		}
	}
	NOT_REACHED();
}

/**
 * Check whether a ride exists with the given name.
 * @param name Name of the ride to find.
 * @return The ride with the given name, or \c nullptr if no such ride exists.
 */
RideInstance *RidesManager::FindRideByName(const std::string &name)
{
	for (auto &pair : this->instances) {
		if (pair.second->state == RIS_ALLOCATED) continue;
		if (name == pair.second->name) return pair.second.get();
	}
	return nullptr;
}

/**
 * A new ride instance was added. Initialize it further.
 * @param num Ride number of the new ride.
 */
void RidesManager::NewInstanceAdded(uint16 num)
{
	RideInstance *ri = this->GetRideInstance(num);
	const RideType *rt = ri->GetRideType();
	assert(ri->state == RIS_ALLOCATED);
	ri->InsertIntoWorld();

	/* Find a new name for the instance. */
	const StringID *names = rt->GetInstanceNames();
	assert(names != nullptr && names[0] != STR_INVALID); // Empty array of names loops forever below.
	int idx = 0;
	int shop_num = 1;
	for (;;) {
		if (names[idx] == STR_INVALID) {
			shop_num++;
			idx = 0;
		}

		ri->name.clear();
		/* Construct a new name. */
		if (shop_num == 1) {
			ri->name = DrawText(rt->GetString(names[idx]));
		} else {
			_str_params.SetStrID(1, rt->GetString(names[idx]));
			_str_params.SetNumber(2, shop_num);
			ri->name = DrawText(GUI_NUMBERED_INSTANCE_NAME);
		}

		if (this->FindRideByName(ri->name) == nullptr) break;

		idx++;
	}

	/* Initialize money and counters. */
	ri->InitializeItemPricesAndStatistics();

	switch (ri->GetKind()) {
		case RTK_SHOP:
		case RTK_GENTLE:
		case RTK_THRILL:
			ri->CloseRide();
			break;

		case RTK_COASTER:
			ri->BuildRide();
			break;

		default:
			NOT_REACHED(); /// \todo Add other ride types.
	}
}

/**
 * Destroy the indicated instance.
 * @param num The ride number to destroy.
 * @pre Instance must be closed.
 */
void RidesManager::DeleteInstance(uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	auto it = this->instances.find(num);
	assert(it != this->instances.end());
	it->second->RemoveAllPeople();
	_inbox.NotifyRideDeletion(num + SRI_FULL_RIDES);
	_guests.NotifyRideDeletion(it->second.get());
	_staff.NotifyRideDeletion(it->second.get());
	it->second->RemoveFromWorld();
	this->instances.erase(it);  // Deletes the instance.
}

void RidesManager::DeleteAllRideInstances()
{
	while (!this->instances.empty()) this->DeleteInstance(this->instances.begin()->first + SRI_FULL_RIDES);
}

/**
 * Does a ride entrance exists at/to the bottom the given voxel in the neighbouring voxel?
 * @param pos Coordinate of the voxel.
 * @param edge Direction to move to get the neighbouring voxel.
 * @pre voxel coordinate must be valid in the world.
 * @return The ride at the neighbouring voxel, if available (else \c nullptr is returned).
 */
RideInstance *RideExistsAtBottom(XYZPoint16 pos, TileEdge edge)
{
	pos.x += _tile_dxy[edge].x;
	pos.y += _tile_dxy[edge].y;
	if (!IsVoxelstackInsideWorld(pos.x, pos.y)) return nullptr;

	const Voxel *vx = _world.GetVoxel(pos);
	if (vx == nullptr || vx->GetInstance() < SRI_FULL_RIDES) {
		/* No ride here, check the voxel below. */
		if (pos.z == 0) return nullptr;
		vx = _world.GetVoxel(pos + XYZPoint16(0, 0, -1));
	}
	if (vx == nullptr || vx->GetInstance() < SRI_FULL_RIDES) return nullptr;
	return _rides_manager.GetRideInstance(vx->GetInstance());
}
