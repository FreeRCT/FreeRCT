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
RideType::RideType(RideTypeKind rtk) : kind(rtk)
{
	this->monthly_cost      = 12345; // Arbitrary non-zero cost.
	this->monthly_open_cost = 12345; // Arbitrary non-zero cost.
	std::fill_n(this->item_type, NUMBER_ITEM_TYPES_SOLD, ITP_NOTHING);
	std::fill_n(this->item_cost, NUMBER_ITEM_TYPES_SOLD, 12345); // Artbitary non-zero cost.
	this->SetupStrings(nullptr, 0, 0, 0, 0, 0);
}

RideType::~RideType()
{
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
 * Constructor of the base ride instance class.
 * @param rt Type of the ride instance.
 */
RideInstance::RideInstance(const RideType *rt)
{
	this->name = nullptr;
	this->type = rt;
	this->state = RIS_ALLOCATED;
	this->flags = 0;
	this->recolours = rt->recolours;
	this->SetEntranceType(0);
	this->SetExitType(0);
	this->recolours.AssignRandomColours();
	this->entrance_recolours.AssignRandomColours();
	this->exit_recolours.AssignRandomColours();
	std::fill_n(this->item_price, NUMBER_ITEM_TYPES_SOLD, 12345); // Arbitrary non-zero amount.
	std::fill_n(this->item_count, NUMBER_ITEM_TYPES_SOLD, 0);

	this->reliability = 365 / 2; // \todo Make different reliabilities for different rides; read from RCDs
	this->breakdown_ctr = -1;
	this->breakdown_state = BDS_UNOPENED;
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
 * \n bool RideInstance::CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const
 * Can the ride be visited, assuming it is approached from direction \a edge?
 * @param vox Position of the voxel with the ride.
 * @param edge Direction of movement (exit direction of the neighbouring voxel).
 * @return Whether the ride can be visited.
 */

/**
 * The recolouring map to apply to this ride at the given position.
 * @param pos Position in the world.
 * @return Recoluring to use.
 */
const Recolouring *RideInstance::GetRecolours(const XYZPoint16 &pos) const
{
	return &this->recolours;
}

/**
 * Whether the ride's entrance should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An entrance is located at the given location.
 */
bool RideInstance::IsEntranceLocation(const XYZPoint16& pos) const
{
	return false;
}

/**
 * Whether the ride's exit should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An exit is located at the given location.
 */
bool RideInstance::IsExitLocation(const XYZPoint16& pos) const
{
	return false;
}

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
 * Some time has passed, update the state of the ride. Default implementation does nothing.
 * @param delay Number of milliseconds that passed.
 */
void RideInstance::OnAnimate(int delay)
{
}

/** Monthly update of the shop administration. */
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

	NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/** Daily update of breakages and maintenance. */
void RideInstance::OnNewDay()
{
	this->HandleBreakdown();
}

/** Handle breakdowns of rides. */
void RideInstance::HandleBreakdown()
{
	if (this->state != RIS_OPEN) return;

	switch (this->breakdown_state) {
		case BDS_UNOPENED:
		case BDS_BROKEN:
			break;

		case BDS_NEEDS_NEW_CTR:
			this->breakdown_ctr = this->rnd.Exponential(this->reliability);
			this->breakdown_state = BDS_WILL_BREAK;
			break;

		case BDS_WILL_BREAK:
			this->breakdown_ctr--;
			break;

		default:
			NOT_REACHED();
	}

	if (this->breakdown_ctr <= 0) {
		this->breakdown_state = BDS_BROKEN;
		/* \todo Call a mechanic. */
	}
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
	if (this->breakdown_state == BDS_UNOPENED) {
		this->breakdown_ctr = this->rnd.Exponential(this->reliability) + BREAKDOWN_GRACE_PERIOD;
		this->breakdown_state = BDS_WILL_BREAK;
	}

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

void RideInstance::Load(Loader &ldr)
{
	this->name.reset(ldr.GetText());

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
	this->breakdown_ctr = (int16)ldr.GetWord();
	this->reliability = ldr.GetWord();
	this->breakdown_state = static_cast<BreakdownState>(ldr.GetByte());
}

void RideInstance::Save(Saver &svr)
{
	svr.PutByte(static_cast<uint8>(this->GetKind()));
	svr.PutText(_language.GetText(this->type->GetString(this->type->GetTypeName())));
	svr.PutText(this->name.get());
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
	svr.PutWord((uint16)this->breakdown_ctr);
	svr.PutWord(this->reliability);
	svr.PutByte(static_cast<uint8>(this->breakdown_state));
}

/** Default constructor of the rides manager. */
RidesManager::RidesManager()
{
	std::fill_n(this->ride_types, lengthof(this->ride_types), nullptr);
	std::fill_n(this->instances, lengthof(this->instances), nullptr);
	std::fill_n(this->entrances, lengthof(this->entrances), nullptr);
	std::fill_n(this->exits, lengthof(this->exits), nullptr);
}

RidesManager::~RidesManager()
{
	for (uint i = 0; i < lengthof(this->entrances); i++) delete this->entrances[i];
	for (uint i = 0; i < lengthof(this->exits); i++) delete this->exits[i];
	for (uint i = 0; i < lengthof(this->ride_types); i++) delete this->ride_types[i];
	for (uint i = 0; i < lengthof(this->instances); i++) delete this->instances[i];
}

/**
 * Some time has passed, update the state of the rides.
 * @param delay Number of milliseconds that passed.
 */
void RidesManager::OnAnimate(int delay)
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		this->instances[i]->OnAnimate(delay);
	}
}

/** A new month has started; perform monthly payments. */
void RidesManager::OnNewMonth()
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		this->instances[i]->OnNewMonth();
	}
}

/** A new day has started; break rides randomly. */
void RidesManager::OnNewDay()
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		this->instances[i]->OnNewDay();
	}
}

void RidesManager::Load(Loader &ldr)
{
	uint32 version = ldr.OpenBlock("RIDS");
	if (version == 1) {
		uint16 allocated_ride_count = ldr.GetWord();
		for (uint16 i = 0; i < allocated_ride_count; i++) {
			const RideType *ride_type = nullptr;
			RideTypeKind ride_kind = static_cast<RideTypeKind>(ldr.GetByte());
			const uint8 *ride_type_name = ldr.GetText();
			bool found = false;

			if (ride_type_name != nullptr) {
				for (uint j = 0; j < lengthof(this->ride_types); j++) {
					ride_type = this->ride_types[j];
					const uint8 *tmp_name = _language.GetText(ride_type->GetString(ride_type->GetTypeName()));

					if (ride_kind == ride_type->kind && StrEqual(tmp_name, ride_type_name)) {
						found = true;
						break;
					}
				}
				delete[] ride_type_name;
			} else {
				ldr.SetFailMessage("Invalid ride type name.");
			}

			if (ride_type == nullptr || !found) {
				ldr.SetFailMessage("Unknown/invalid ride type.");
				break;
			}

			uint16 instance = this->GetFreeInstance(ride_type);
			if (instance == INVALID_RIDE_INSTANCE) {
				ldr.SetFailMessage("Invalid ride instance.");
				break;
			}

			this->instances[i] = this->CreateInstance(ride_type, instance);
			this->instances[i]->Load(ldr);
		}
	} else if (version != 0) {
		ldr.SetFailMessage("Incorrect version of rides block.");
	}
	ldr.CloseBlock();
}

void RidesManager::Save(Saver &svr)
{
	svr.StartBlock("RIDS", 1);
	int count = 0;
	uint16 allocated_ride_indexes[MAX_NUMBER_OF_RIDE_INSTANCES];
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		allocated_ride_indexes[count] = i;
		count++;
	}
	svr.PutWord(count);
	for (uint16 i = 0; i < count; i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		this->instances[allocated_ride_indexes[i]]->Save(svr);
	}
	svr.EndBlock();
}

/**
 * Get the requested ride instance.
 * @param num Ride number to retrieve.
 * @return The requested ride, or \c nullptr if not available.
 */
RideInstance *RidesManager::GetRideInstance(uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	if (num >= lengthof(this->instances)) return nullptr;
	return this->instances[num];
}

/**
 * Get the requested ride instance (read-only).
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c nullptr if not available.
 */
const RideInstance *RidesManager::GetRideInstance(uint16 num) const
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	if (num >= lengthof(this->instances)) return nullptr;
	return this->instances[num];
}

/**
 * Get the ride instance index number.
 * @return Ride instance index.
 */
uint16 RideInstance::GetIndex() const
{
	for (uint i = 0; i < lengthof(_rides_manager.instances); i++) {
		if (_rides_manager.instances[i] == this) return i + SRI_FULL_RIDES;
	}
	NOT_REACHED();
}

/**
 * Add a new ride type to the manager.
 * @param type New ride type to add.
 * @return \c true if the addition was successful, else \c false.
 */
bool RidesManager::AddRideType(RideType *type)
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] == nullptr) {
			this->ride_types[i] = type;
			return true;
		}
	}
	return false;
}

/**
 * Add a new ride entrance or exit type to the manager.
 * @param type New ride entrance/exit type to add.
 * @return \c true if the addition was successful, else \c false.
 */
bool RidesManager::AddRideEntranceExitType(RideEntranceExitType *type)
{
	auto array = type->is_entrance ? this->entrances : this->exits;
	for (uint i = 0; i < lengthof(array); i++) {
		if (array[i] == nullptr) {
			array[i] = type;
			return true;
		}
	}
	return false;
}

/**
 * Check whether the ride type can actually be created.
 * @param type Ride type that should be created.
 * @return Index of a free instance if it exists (claim it immediately using #CreateInstance), or #INVALID_RIDE_INSTANCE if all instances are used.
 */
uint16 RidesManager::GetFreeInstance(const RideType *type)
{
	uint16 idx;
	for (idx = 0; idx < lengthof(this->instances); idx++) {
		if (this->instances[idx] == nullptr) break;
	}
	return (idx < lengthof(this->instances) && type->CanMakeInstance()) ? idx + SRI_FULL_RIDES : INVALID_RIDE_INSTANCE;
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
	assert(num < lengthof(this->instances));
	assert(this->instances[num] == nullptr);
	this->instances[num] = type->CreateInstance();
	return this->instances[num];
}

/**
 * Check whether a ride exists with the given name.
 * @param name Name of the ride to find.
 * @return The ride with the given name, or \c nullptr if no such ride exists.
 */
RideInstance *RidesManager::FindRideByName(const uint8 *name)
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == nullptr || this->instances[i]->state == RIS_ALLOCATED) continue;
		if (StrEqual(name, this->instances[i]->name.get())) return this->instances[i];
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

		ri->name.reset(new uint8[MAX_RIDE_INSTANCE_NAME_LENGTH]);
		/* Construct a new name. */
		if (shop_num == 1) {
			DrawText(rt->GetString(names[idx]), ri->name.get(), MAX_RIDE_INSTANCE_NAME_LENGTH);
		} else {
			_str_params.SetStrID(1, rt->GetString(names[idx]));
			_str_params.SetNumber(2, shop_num);
			DrawText(GUI_NUMBERED_INSTANCE_NAME, ri->name.get(), MAX_RIDE_INSTANCE_NAME_LENGTH);
		}

		if (this->FindRideByName(ri->name.get()) == nullptr) break;

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
	assert(num < lengthof(this->instances));
	this->instances[num]->RemoveAllPeople();
	_guests.NotifyRideDeletion(this->instances[num]);
	this->instances[num]->RemoveFromWorld();
	delete this->instances[num];
	this->instances[num] = nullptr;
}

void RidesManager::DeleteAllRideInstances()
{
	for (uint i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] != nullptr) this->DeleteInstance(this->instances[i]->GetIndex());
	}
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
