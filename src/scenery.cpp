/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scenery.cpp Scenery items implementation. */

#include "scenery.h"

#include "fileio.h"
#include "gamecontrol.h"
#include "generated/scenery_strings.h"
#include "generated/scenery_strings.cpp"
#include "viewport.h"

SceneryManager _scenery;

/** Default constructor. */
SceneryType::SceneryType()
{
	this->category = SCC_SCENARIO;
	this->name = STR_NULL;
	this->width_x = 0;
	this->width_y = 0;
	this->buy_cost = Money();
	this->return_cost = Money();
	this->return_cost_dry = Money();
	this->watering_interval = 0;
	this->symmetric = true;
	this->main_animation = nullptr;
	this->dry_animation = nullptr;
	std::fill_n(this->previews, lengthof(this->previews), nullptr);
}

/**
 * Load a type of scenery from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 */
bool SceneryType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	if (rcd_file->version != 1 || rcd_file->size < 2) return false;

	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	if (this->width_x < 1 || this->width_y < 1) return false;
	if (rcd_file->size != 48 + (this->width_x * this->width_y)) return false;

	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; x++) {
		for (int8 y = 0; y < this->width_y; y++) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}

	this->watering_interval = rcd_file->GetUInt32();
	this->main_animation = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	this->dry_animation = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	for (int i = 0; i < 4; i++) {
		ImageData *view;
		if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
		this->previews[i] = view;
	}

	this->buy_cost = Money(rcd_file->GetInt32());
	this->return_cost = Money(rcd_file->GetInt32());
	this->return_cost_dry = Money(rcd_file->GetInt32());
	this->symmetric = rcd_file->GetUInt8() > 0;
	this->category = static_cast<SceneryCategory>(rcd_file->GetUInt8());

	TextData *text_data;
	if (!LoadTextFromFile(rcd_file, texts, &text_data)) return false;
	this->name = _language.RegisterStrings(*text_data, _scenery_strings_table);
	return true;
}

/**
 * Construct a new scenery item instance.
 * @param t Type of scenery item.
 */
SceneryInstance::SceneryInstance(const SceneryType *t)
{
	this->type = t;
	this->vox_pos = XYZPoint16::invalid();
	this->orientation = 0;
	this->animtime = 0;
	this->last_watered = 0;
}

/**
 * Checks whether this item can be placed at the current position.
 * @return \c STR_NULL if the item can be placed here; otherwise the reason why it can't.
 */
StringID SceneryInstance::CanPlace() const
{
	if (this->vox_pos == XYZPoint16::invalid()) return GUI_ERROR_MESSAGE_BAD_LOCATION;

	const int8 wx = this->type->width_x;
	const int8 wy = this->type->width_y;
	for (int8 x = 0; x < wx; x++) {
		for (int8 y = 0; y < wy; y++) {
			XYZPoint16 location = this->vox_pos + OrientatedOffset(this->orientation, x, y);
			if (!IsVoxelstackInsideWorld(location.x, location.y)) return GUI_ERROR_MESSAGE_BAD_LOCATION;
			if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(location.x, location.y) != OWN_PARK) return GUI_ERROR_MESSAGE_UNOWNED_LAND;
			const int8 height = this->type->GetHeight(x, y);
			for (int16 h = 0; h < height; h++) {
				location.z = this->vox_pos.z + h;
				const Voxel *voxel = _world.GetVoxel(location);
				if (voxel == nullptr) {
					if (h == 0) {
						/* If this is the upper or lower part of a steep slope, adjust the error message accordingly. */
						location.z--;
						voxel = _world.GetVoxel(location);
						if (voxel != nullptr && voxel->GetGroundSlope() != SL_FLAT) return GUI_ERROR_MESSAGE_SLOPE;

						location.z += 2;
						voxel = _world.GetVoxel(location);
						if (voxel != nullptr && voxel->GetGroundSlope() != SL_FLAT) return GUI_ERROR_MESSAGE_UNDERGROUND;

						return GUI_ERROR_MESSAGE_BAD_LOCATION;
					}
					continue;
				}

				if (!voxel->CanPlaceInstance()) return GUI_ERROR_MESSAGE_OCCUPIED;
				if (h == 0) {
					if (voxel->GetGroundSlope() != SL_FLAT) return GUI_ERROR_MESSAGE_SLOPE;
					if (voxel->GetGroundType() == GTP_INVALID) return GUI_ERROR_MESSAGE_BAD_LOCATION;
				} else {
					if (voxel->GetGroundType() != GTP_INVALID) return GUI_ERROR_MESSAGE_UNDERGROUND;
				}
			}
		}
	}

	return STR_NULL;
}

/** Link this item into the voxels it occupies. */
void SceneryInstance::InsertIntoWorld()
{
	const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->type);
	const int8 wx = this->type->width_x;
	const int8 wy = this->type->width_y;
	for (int8 x = 0; x < wx; x++) {
		for (int8 y = 0; y < wy; y++) {
			const int8 height = this->type->GetHeight(x, y);
			const XYZPoint16 location = OrientatedOffset(this->orientation, x, y);
			for (int16 h = 0; h < height; h++) {
				const XYZPoint16 p = this->vox_pos + XYZPoint16(location.x, location.y, h);
				Voxel *voxel = _world.GetCreateVoxel(p, true);
				assert(voxel != nullptr && voxel->GetInstance() == SRI_FREE);
				voxel->SetInstance(SRI_SCENERY);
				/*
				 * On a large map, there may be more than 65535 individual scenery instances
				 * in existance. Therefore we do not assign items an individual index number
				 * like for rides, but store only the index of our %SceneryType in the voxel
				 * and lookup the instance dynamically whenever necessary. Since most
				 * scenery items occupy only very few voxels, this lookup is fast.
				 */
				voxel->SetInstanceData(h == 0 ? voxel_data : INVALID_VOXEL_DATA);
			}
		}
	}
	this->last_watered = 0;
	this->animtime = 0;
	this->MarkDirty();
}

/** Remove this item from the voxels it currently occupies. */
void SceneryInstance::RemoveFromWorld()
{
	this->MarkDirty();
	const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->type);
	const int8 wx = this->type->width_x;
	const int8 wy = this->type->width_y;
	for (int8 x = 0; x < wx; x++) {
		for (int8 y = 0; y < wy; y++) {
			const XYZPoint16 unrotated_pos = OrientatedOffset(this->orientation, x, y);
			const int8 height = this->type->GetHeight(x, y);
			if (!IsVoxelstackInsideWorld(this->vox_pos.x + unrotated_pos.x, this->vox_pos.y + unrotated_pos.y)) continue;
			for (int16 h = 0; h < height; h++) {
				const XYZPoint16 p = this->vox_pos + XYZPoint16(unrotated_pos.x, unrotated_pos.y, h);
				Voxel *voxel = _world.GetCreateVoxel(p, false);
				MarkVoxelDirty(p, 1);
				if (voxel != nullptr && voxel->instance != SRI_FREE) {
					assert(voxel->instance == SRI_SCENERY);
					assert(voxel->instance_data == (h == 0 ? voxel_data : INVALID_VOXEL_DATA));
					voxel->ClearInstances();
				}
			}
		}
	}
}

/** Mark the voxels occupied by this item as in need of repainting. */
void SceneryInstance::MarkDirty()
{
	for (int8 x = 0; x < this->type->width_x; ++x) {
		for (int8 y = 0; y < this->type->width_y; ++y) {
			MarkVoxelDirty(this->vox_pos + OrientatedOffset(this->orientation, x, y), this->type->GetHeight(x, y));
		}
	}
}

/**
 * Get the sprites to display for the provided voxel number.
 * @param vox The voxel's absolute coordinates.
 * @param voxel_number Number of the voxel to draw (copied from the world voxel data).
 * @param orient View orientation.
 * @param sprites [out] Sprites to draw, from back to front, #SO_PLATFORM_BACK, #SO_RIDE, #SO_RIDE_FRONT, and #SO_PLATFORM_FRONT.
 * @param platform [out] Shape of the support platform, if needed. @see PathSprites
 */
void SceneryInstance::GetSprites(const XYZPoint16 &vox, const uint16 voxel_number, const uint8 orient, const ImageData *sprites[4], uint8 *platform) const
{
	sprites[0] = nullptr;
	sprites[1] = nullptr;
	sprites[2] = nullptr;
	sprites[3] = nullptr;
	if (voxel_number == INVALID_VOXEL_DATA) return;
	const XYZPoint16 unrotated_pos = UnorientatedOffset(this->orientation, vox.x - this->vox_pos.x, vox.y - this->vox_pos.y);
	const TimedAnimation *anim = (this->NeedsWatering() ? this->type->dry_animation : this->type->main_animation);
	sprites[1] = anim->views[anim->GetFrame(this->animtime, true)]
			->sprites[(this->orientation + 4 - orient) & 3]
					[unrotated_pos.x * this->type->width_y + unrotated_pos.y];
}

/**
 * Some time has passed, update this item's animation.
 * @param delay Number of milliseconds that passed.
 */
void SceneryInstance::OnAnimate(const int delay)
{
	if (this->type->watering_interval > 0) this->last_watered += delay;
	this->animtime += delay;
	this->animtime %= (this->NeedsWatering() ? this->type->dry_animation : this->type->main_animation)->GetTotalDuration();
	this->MarkDirty();  // Ensure the animation is updated.
}

/**
 * Whether this item is dried up for lack of watering.
 * @return The item is dry.
 */
bool SceneryInstance::NeedsWatering() const
{
	return this->type->watering_interval > 0 && this->last_watered > this->type->watering_interval;
}

void SceneryInstance::Load(Loader &ldr)
{
	this->vox_pos.x = ldr.GetWord();
	this->vox_pos.y = ldr.GetWord();
	this->vox_pos.z = ldr.GetWord();
	this->orientation = ldr.GetByte();
	this->animtime = ldr.GetLong();
	this->last_watered = ldr.GetLong();
}

void SceneryInstance::Save(Saver &svr) const
{
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
	svr.PutByte(this->orientation);
	svr.PutLong(this->animtime);
	svr.PutLong(this->last_watered);
}

/** Default constructor. */
SceneryManager::SceneryManager()
{
	this->temp_item = nullptr;
}

/**
 * Register a new scenery type.
 * @param type Scenery type to add.
 * @note Takes ownership of the pointer.
 */
bool SceneryManager::AddSceneryType(SceneryType *type)
{
	for (uint i = 0; i < MAX_NUMBER_OF_SCENERY_TYPES; i++) {
		if (this->scenery_item_types[i].get() == nullptr) {
			this->scenery_item_types[i].reset(type);
			return true;
		}
	}
	return false;
}

/**
 * Retrieve the index of a scenery type.
 * @param type Type to query.
 */
uint16 SceneryManager::GetSceneryTypeIndex(const SceneryType *type) const
{
	for (uint i = 0; i < MAX_NUMBER_OF_SCENERY_TYPES; i++) {
		if (this->scenery_item_types[i].get() == type) {
			return i;
		}
	}
	NOT_REACHED();
}

/**
 * Returns all scenery types with the given category.
 * @param cat Category of interest.
 * @return All types in the category.
 */
std::vector<const SceneryType*> SceneryManager::GetAllTypes(SceneryCategory cat) const
{
	std::vector<const SceneryType*> result;
	for (uint i = 0; i < MAX_NUMBER_OF_SCENERY_TYPES; i++) {
		if (this->scenery_item_types[i].get() != nullptr && this->scenery_item_types[i]->category == cat) {
			result.push_back(this->scenery_item_types[i].get());
		}
	}
	return result;
}

/** Remove all scenery items. */
void SceneryManager::Clear()
{
	this->temp_item = nullptr;
	while (!this->all_items.empty()) this->RemoveItem(this->all_items.begin()->first);
}

/**
 * Some time has passed, update the state of the scenery items.
 * @param delay Number of milliseconds that passed.
 */
void SceneryManager::OnAnimate(const int delay)
{
	for (auto &pair : this->all_items) pair.second->OnAnimate(delay);
}

/**
 * Insert a new scenery item into the world.
 * @param item Item to insert.
 * @note Takes ownership of the pointer.
 * @pre The item's type, position, and orientation have been set previously.
 */
void SceneryManager::AddItem(SceneryInstance *item)
{
	assert(this->all_items.count(item->vox_pos) == 0);
	this->all_items[item->vox_pos] = std::unique_ptr<SceneryInstance>(item);
	item->InsertIntoWorld();
}

/**
 * Remove an item from the world.
 * @param pos Base position of the item to delete.
 */
void SceneryManager::RemoveItem(const XYZPoint16 &pos)
{
	auto it = this->all_items.find(pos);
	assert(it != this->all_items.end());
	it->second->RemoveFromWorld();
	this->all_items.erase(it);  // This deletes the instance.
}

/**
 * Get the item at the specified position.
 * @param pos Any of the positions occupied by the item.
 * @return The item, or \c nullptr if there isn't one here.
 */
SceneryInstance *SceneryManager::GetItem(const XYZPoint16 &pos)
{
	auto it = this->all_items.find(pos);
	if (it != this->all_items.end()) return it->second.get();
	if (this->temp_item != nullptr && this->temp_item->vox_pos == pos) return this->temp_item;

	const Voxel* voxel = _world.GetVoxel(pos);
	if (voxel == nullptr || voxel->instance != SRI_SCENERY) return nullptr;
	const uint16 instance_data = voxel->instance_data;
	if (instance_data == INVALID_VOXEL_DATA) return nullptr;

	const SceneryType *type = this->scenery_item_types[instance_data].get();
	const int search_radius = std::max(type->width_x - 1, type->width_y - 1);
	for (int x = -search_radius; x <= search_radius; x++) {
		for (int y = -search_radius; y <= search_radius; y++) {
			const XYZPoint16 p(pos.x + x, pos.y + y, pos.z);
			it = this->all_items.find(p);
			SceneryInstance *candidate;
			if (it == this->all_items.end()) {
				if (this->temp_item != nullptr && this->temp_item->vox_pos == p) {
					candidate = this->temp_item;
				} else {
					continue;
				}
			} else {
				candidate = it->second.get();
			}
			if (candidate->type != type) continue;

			const XYZPoint16 corner = p + OrientatedOffset(candidate->orientation, type->width_x - 1, type->width_y - 1);
			if (
					pos.x >= std::min(p.x, corner.x) &&
					pos.x <= std::max(p.x, corner.x) &&
					pos.y >= std::min(p.y, corner.y) &&
					pos.y <= std::max(p.y, corner.y)) {
				return candidate;
			}
		}
	}
	return nullptr;
}

void SceneryManager::Load(Loader &ldr)
{
	this->Clear();
	const uint32 version = ldr.OpenBlock("SCNY");
	if (version == 1) {
		for (long l = ldr.GetLong(); l > 0; l--) {
			SceneryInstance *i = new SceneryInstance(this->scenery_item_types[ldr.GetWord()].get());
			i->Load(ldr);
			this->all_items[i->vox_pos] = std::unique_ptr<SceneryInstance>(i);
		}
	} else if (version != 0) {
		ldr.SetFailMessage("Incorrect version of scenery block.");
	}
	ldr.CloseBlock();
}

void SceneryManager::Save(Saver &svr) const
{
	svr.StartBlock("SCNY", 1);
	svr.PutLong(this->all_items.size());
	for (const auto &pair : this->all_items) {
		svr.PutWord(this->GetSceneryTypeIndex(pair.second->type));
		pair.second->Save(svr);
	}
	svr.EndBlock();
}
