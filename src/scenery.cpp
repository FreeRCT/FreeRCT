/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scenery.cpp Scenery items implementation. */

#include "scenery.h"

#include "fileio.h"
#include "generated/scenery_strings.h"
#include "generated/scenery_strings.cpp"

SceneryManager _scenery;

/** Default constructor. */
SceneryType::SceneryType()
{
	this->name = STR_NULL;
	this->width_x = 0;
	this->width_y = 0;
	this->buy_cost = Money(-1);
	this->return_cost = Money(-1);
	this->symmetric = true;
	this->animation = nullptr;
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
	if (rcd_file->size != 35 + (this->width_x * this->width_y)) return false;

	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; ++x) {
		for (int8 y = 0; y < this->width_y; ++y) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}

	this->animation = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	for (int i = 0; i < 4; i++) {
		ImageData *view;
		if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
		this->previews[i] = view;
	}

	this->buy_cost = Money(rcd_file->GetUInt32());
	this->return_cost = Money(rcd_file->GetUInt32());
	this->symmetric = rcd_file->GetUInt8() > 0;

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
}

/** Link this item into the voxels it occupies. */
void SceneryInstance::InsertIntoWorld()
{
	const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->type);
	const int8 wx = this->type->width_x;
	const int8 wy = this->type->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const int8 height = this->type->GetHeight(x, y);
			const XYZPoint16 location = OrientatedOffset(this->orientation, x, y);
			for (int16 h = 0; h < height; ++h) {
				const XYZPoint16 p = this->vox_pos + XYZPoint16(location.x, location.y, h);
				Voxel *voxel = _world.GetCreateVoxel(p, true);
				assert(voxel && voxel->GetInstance() == SRI_FREE);
				voxel->SetInstance(SRI_SCENERY);
				voxel->SetInstanceData(h == 0 ? voxel_data : INVALID_VOXEL_DATA);
			}
		}
	}
}

/** Remove this item from the voxels it currently occupies. */
void SceneryInstance::RemoveFromWorld()
{
	const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->type);
	const int8 wx = this->type->width_x;
	const int8 wy = this->type->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const XYZPoint16 unrotated_pos = OrientatedOffset(this->orientation, x, y);
			const int8 height = this->type->GetHeight(x, y);
			if (!IsVoxelstackInsideWorld(this->vox_pos.x + unrotated_pos.x, this->vox_pos.y + unrotated_pos.y)) continue;
			for (int16 h = 0; h < height; ++h) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(unrotated_pos.x, unrotated_pos.y, h), false);
				if (voxel != nullptr && voxel->instance != SRI_FREE) {
					assert(voxel->instance == SRI_SCENERY);
					assert(voxel->instance_data == voxel_data);
					voxel->ClearInstances();
				}
			}
		}
	}
}

void SceneryInstance::Load(Loader &ldr)
{
	this->vox_pos.x = ldr.GetWord();
	this->vox_pos.y = ldr.GetWord();
	this->vox_pos.z = ldr.GetWord();
	this->orientation = ldr.GetByte();
	_scenery.AddItem(this);
}

void SceneryInstance::Save(Saver &svr) const
{
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
	svr.PutByte(orientation);
}

/** Default constructor. */
SceneryManager::SceneryManager()
{
	/* Nothing to do currently. */
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

/** Remove all scenery items. */
void SceneryManager::Clear()
{
	while (!this->all_items.empty()) this->RemoveItem(this->all_items.begin()->first);
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

void SceneryManager::Load(Loader &ldr)
{
	this->Clear();
	const uint32 version = ldr.OpenBlock("SCNY");
	if (version == 1) {
		for (long l = ldr.GetLong(); l > 0; l--) {
			SceneryInstance *i = new SceneryInstance(this->scenery_item_types[ldr.GetWord()].get());
			i->Load(ldr);
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
