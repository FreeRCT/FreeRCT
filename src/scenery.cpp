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
#include "person.h"
#include "people.h"
#include "random.h"
#include "viewport.h"

SceneryManager _scenery;

std::map<uint8, PathObjectType*> PathObjectType::all_types;

/* Predefined path object types. */
const PathObjectType PathObjectType::LITTER   (1, true,  true,  Money(  0));
const PathObjectType PathObjectType::VOMIT    (2, true,  true,  Money(  0));
const PathObjectType PathObjectType::LAMP     (3, false, true,  Money(400));
const PathObjectType PathObjectType::BENCH    (4, false, false, Money(500));
const PathObjectType PathObjectType::LITTERBIN(5, false, true,  Money(600));

/**
 * Private constructor.
 * @param id Unique ID of this type.
 * @param ign This item type ignores edges.
 * @param slope This item type can exist on slopes.
 * @param cost Cost to buy this item (\c 0 means the user can't buy it).
 */
PathObjectType::PathObjectType(const uint8 id, const bool ign, const bool slope, const Money &cost)
: buy_cost(cost), type_id(id), ignore_edges(ign), can_exist_on_slope(slope)
{
	assert(this->type_id != INVALID_PATH_OBJECT && all_types.count(this->type_id) == 0);
	all_types[this->type_id] = this;
}

/**
 * Retrieve a path object type by its ID.
 * @param id The type ID.
 * @return The type.
 */
const PathObjectType *PathObjectType::Get(uint8 id)
{
	return all_types.at(id);
}

/**
 * PathObjectSprite constructor.
 * @param s Sprite to draw.
 * @param off Pixel offset.
 */
PathObjectInstance::PathObjectSprite::PathObjectSprite(const ImageData *s, XYZPoint16 off)
:
	sprite(s),
	offset(off),
	semi_transparent(false)
{
}

/**
 * Construct a new path object instance.
 * @param t Type of path object.
 * @param pos Voxel coordinate of the instance.
 * @param offset Offset of the item inside the voxel.
 */
PathObjectInstance::PathObjectInstance(const PathObjectType *t, const XYZPoint16 &pos, const XYZPoint16 &offset)
:
	type(t),
	vox_pos(pos),
	pix_pos(offset),
	state(this->type->ignore_edges ? 0xFF : 0)
{
	if (this->type == &PathObjectType::BENCH) {
		std::fill_n(this->data, lengthof(this->data), PathObjectType::NO_GUEST_ON_BENCH | (PathObjectType::NO_GUEST_ON_BENCH << 16));
	} else {
		std::fill_n(this->data, lengthof(this->data), 0);
	}

	assert(_world.GetVoxel(this->vox_pos) != nullptr && HasValidPath(_world.GetVoxel(this->vox_pos)));
	this->RecomputeExistenceState();
}

PathObjectInstance::~PathObjectInstance()
{
	if (_scenery.temp_path_object == this) _scenery.temp_path_object = nullptr;
	if (this->type == &PathObjectType::BENCH) {
		for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
			this->RemoveGuestsFromBench(e);
		}
	}
}

/** Recompute at which of the path edges this item should exist. */
void PathObjectInstance::RecomputeExistenceState()
{
	const Voxel *voxel = _world.GetVoxel(this->vox_pos);
	if (voxel == nullptr || !HasValidPath(voxel)) {
		/* The path was deleted under the object. Delete it now. */
		_scenery.SetPathObjectInstance(this->vox_pos, nullptr);
		return;
	}
	const PathSprites path_slope_imploded = GetImplodedPathSlope(voxel);
	assert(path_slope_imploded < PATH_COUNT && path_slope_imploded >= PATH_EMPTY);  // Path should be either flat or a ramp.
	const bool is_ramp = (path_slope_imploded >= PATH_FLAT_COUNT);

	if (this->type->ignore_edges) {
		if (this->state == 0xFF) {
			const PathDecoration &pdec = _sprite_manager.GetSprites(64)->path_decoration;
			uint8 count;
			if (is_ramp) {
				this->data[0] = path_slope_imploded - PATH_FLAT_COUNT;
				count = (this->type == &PathObjectType::LITTER ? pdec.ramp_litter_count : pdec.ramp_vomit_count)[this->data[0]];
			} else {
				this->data[0] = INVALID_EDGE;
				count = (this->type == &PathObjectType::LITTER ? pdec.flat_litter_count : pdec.flat_vomit_count);
			}

			assert(count > 0);
			if (count > 1) {
				Random r;
				this->state = r.Uniform(count - 1);
			} else {
				this->state = 0;
			}
		}
		return;
	}

	if (is_ramp) {
		if (this->type->can_exist_on_slope) {
			switch (path_slope_imploded) {
				case PATH_RAMP_NE:
				case PATH_RAMP_SW:
					this->SetExistsOnTileEdge(EDGE_NE, false);
					this->SetExistsOnTileEdge(EDGE_SW, false);
					this->SetExistsOnTileEdge(EDGE_SE, true );
					this->SetExistsOnTileEdge(EDGE_NW, true );
					break;
				case PATH_RAMP_SE:
				case PATH_RAMP_NW:
					this->SetExistsOnTileEdge(EDGE_NE, true);
					this->SetExistsOnTileEdge(EDGE_SW, true);
					this->SetExistsOnTileEdge(EDGE_SE, false);
					this->SetExistsOnTileEdge(EDGE_NW, false);
					break;
				default: NOT_REACHED();
			}
		} else {
			for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) this->SetExistsOnTileEdge(e, false);
		}
	} else {
		const uint8 path_edges_bitset = _path_expand[path_slope_imploded] & PATHMASK_EDGES;
		this->SetExistsOnTileEdge(EDGE_NE, (path_edges_bitset & PATHMASK_NE) == 0);
		this->SetExistsOnTileEdge(EDGE_SE, (path_edges_bitset & PATHMASK_SE) == 0);
		this->SetExistsOnTileEdge(EDGE_SW, (path_edges_bitset & PATHMASK_SW) == 0);
		this->SetExistsOnTileEdge(EDGE_NW, (path_edges_bitset & PATHMASK_NW) == 0);
	}
}

/**
 * Demolish this path object.
 * @param e Edge of the tile on which the item to demolish is located.
 */
void PathObjectInstance::Demolish(const TileEdge e)
{
	assert(!this->type->ignore_edges);
	assert(this->GetExistsOnTileEdge(e));
	assert(!this->GetDemolishedOnTileEdge(e));

	this->SetDemolishedOnTileEdge(e, true);

	if (this->type == &PathObjectType::LITTERBIN) {
		XYZPoint16 offset;
		if (GetImplodedPathSlope(_world.GetVoxel(this->vox_pos)) >= PATH_FLAT_COUNT) offset.z = 128;
		Random r;

		/* Spread the bin's contents all over the path in front of the bin. */
		for (; this->data[e] > 0; this->data[e]--) {
			switch (e) {
				case EDGE_NE:
					offset.x =   0 + r.Uniform(32);
					offset.y = 128 + r.Uniform(64) - 32;
					break;
				case EDGE_SE:
					offset.y = 255 - r.Uniform(32);
					offset.x = 128 + r.Uniform(64) - 32;
					break;
				case EDGE_SW:
					offset.x = 255 - r.Uniform(32);
					offset.y = 128 + r.Uniform(64) - 32;
					break;
				case EDGE_NW:
					offset.y =   0 + r.Uniform(32);
					offset.x = 128 + r.Uniform(64) - 32;
					break;

				default: NOT_REACHED();
			}
			_scenery.AddLitter(this->vox_pos, offset);
		}
	}
}

/**
 * Remove all guests from this bench.
 * @param e Edge of the bench.
 * @pre This item is a bench.
 */
void PathObjectInstance::RemoveGuestsFromBench(const TileEdge e)
{
	if (!this->GetExistsOnTileEdge(e)) return;

	uint16 id = this->GetLeftGuest(e);
	if (id != PathObjectType::NO_GUEST_ON_BENCH) {
		_guests.GetExisting(id)->ExpelFromBench();
		this->SetLeftGuest(e, PathObjectType::NO_GUEST_ON_BENCH);
	}

	id = this->GetRightGuest(e);
	if (id != PathObjectType::NO_GUEST_ON_BENCH) {
		_guests.GetExisting(id)->ExpelFromBench();
		this->SetRightGuest(e, PathObjectType::NO_GUEST_ON_BENCH);
	}
}

/**
 * Get all sprites that should be drawn for this object.
 * @param orientation Viewport orientation.
 * @return All sprites to draw.
 */
std::vector<PathObjectInstance::PathObjectSprite> PathObjectInstance::GetSprites(const uint8 orientation) const
{
	const PathDecoration &pdec = _sprite_manager.GetSprites(64)->path_decoration;

	if (this->type->ignore_edges) {
		switch (this->data[0]) {
			case INVALID_EDGE:
				return {PathObjectInstance::PathObjectSprite((
						this->type == &PathObjectType::LITTER ? pdec.flat_litter : pdec.flat_vomit)[this->state], this->pix_pos)};

			case EDGE_NE:
			case EDGE_SE:
			case EDGE_SW:
			case EDGE_NW:
				return {PathObjectInstance::PathObjectSprite((
						this->type == &PathObjectType::LITTER ? pdec.ramp_litter : pdec.ramp_vomit)[this->data[0]][this->state], this->pix_pos)};

			default: NOT_REACHED();
		}
	} else {
		std::vector<PathObjectSprite> result;
		XYZPoint16 offset;
		if (GetImplodedPathSlope(_world.GetVoxel(this->vox_pos)) >= PATH_FLAT_COUNT) offset.z = 128;

		for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
			if (!this->GetExistsOnTileEdge(e)) continue;

			const uint8 orient = (e - orientation) & 3;
			switch (e) {
				case EDGE_NE: offset.x =   0; offset.y = 128; break;
				case EDGE_SE: offset.x = 128; offset.y = 255; break;
				case EDGE_SW: offset.x = 255; offset.y = 128; break;
				case EDGE_NW: offset.x = 128; offset.y =   0; break;
				default: NOT_REACHED();
			}

			if (this->GetDemolishedOnTileEdge(e)) {
				if (this->type == &PathObjectType::BENCH) {
					result.emplace_back(pdec.demolished_bench[orient], offset);
				} else if (this->type == &PathObjectType::LAMP) {
					result.emplace_back(pdec.demolished_lamp[orient], offset);
				} else if (this->type == &PathObjectType::LITTERBIN) {
					result.emplace_back(pdec.demolished_bin[orient], offset);
				} else {
					NOT_REACHED();
				}
			} else {
				if (this->type == &PathObjectType::BENCH) {
					result.emplace_back(pdec.bench[orient], offset);
				} else if (this->type == &PathObjectType::LAMP) {
					result.emplace_back(pdec.lamp_post[orient], offset);
				} else if (this->type == &PathObjectType::LITTERBIN) {
					if (this->data[e] < PathObjectType::BIN_FULL_CAPACITY) {
						result.emplace_back(pdec.litterbin[orient], offset);
					} else {
						result.emplace_back(pdec.overflow_bin[orient], offset);
					}
				} else {
					NOT_REACHED();
				}
			}
		}

		return result;
	}
}

/**
 * Check whether this item exists on a specific edge of its voxel.
 * @param e Edge to query,
 * @return An item exists there.
 */
bool PathObjectInstance::GetExistsOnTileEdge(TileEdge e) const
{
	return GB(this->state, e, 1);
}

/**
 * Set whether this item should exist on a specific edge of its voxel.
 * @param e Edge to query,
 * @param b The item should exist there.
 */
void PathObjectInstance::SetExistsOnTileEdge(TileEdge e, bool b)
{
	if (!b && this->type == &PathObjectType::BENCH) this->RemoveGuestsFromBench(e);
	SB(this->state, e, 1, b ? 1 : 0);
}

/**
 * Check whether this item is demolished on a specific edge of its voxel.
 * @param e Edge to query,
 * @return The item there is demolished.
 */
bool PathObjectInstance::GetDemolishedOnTileEdge(TileEdge e) const
{
	return GB(this->state, e + 4, 1);
}

/**
 * Set whether this item should be demolished on a specific edge of its voxel.
 * @param e Edge to query,
 * @param d The item is demolished there.
 */
void PathObjectInstance::SetDemolishedOnTileEdge(TileEdge e, bool d)
{
	if (d && this->type == &PathObjectType::BENCH) this->RemoveGuestsFromBench(e);
	SB(this->state, e + 4, 1, d ? 1 : 0);
}

/**
 * Get the free bin capacity of the bin on a given edge.
 * @param e Edge to query,
 * @return The free capacity (\c 0 for full bins).
 * @pre This item is a bin, and a non-demolished bin exists on this edge.
 */
uint PathObjectInstance::GetFreeBinCapacity(TileEdge e) const
{
	assert(this->data[e] <= PathObjectType::BIN_MAX_CAPACITY);
	return PathObjectType::BIN_MAX_CAPACITY - this->data[e];
}

/**
 * Check whether the bin on a given edge is so full that it should be emptied.
 * @param e Edge to query,
 * @return The bin needs emptying
 * @pre This item is a bin, and a non-demolished bin exists on this edge.
 */
bool PathObjectInstance::BinNeedsEmptying(TileEdge e) const
{
	return this->data[e] >= PathObjectType::BIN_FULL_CAPACITY;
}

/**
 * Get the guest sitting on the left half of the bench on a given edge.
 * @param e Edge to query,
 * @return The guest's ID (#PathObjectType::NO_GUEST_ON_BENCH if the seat is free).
 * @pre This item is a bench, and a non-demolished bench exists on this edge.
 */
uint16 PathObjectInstance::GetLeftGuest(TileEdge e) const
{
	return this->data[e] & 0xFFFF;
}

/**
 * Get the guest sitting on the left half of the bench on a given edge.
 * @param e Edge to query,
 * @return The guest's ID (#PathObjectType::NO_GUEST_ON_BENCH if the seat is free).
 * @pre This item is a bench, and a non-demolished bench exists on this edge.
 */
uint16 PathObjectInstance::GetRightGuest(TileEdge e) const
{
	return this->data[e] >> 16;
}

/**
 * Throw a piece of litter into the bin on a given edge.
 * @param e Edge to query,
 * @pre This item is a bin, and a non-demolished bin that still has free capacity exists on this edge.
 */
void PathObjectInstance::AddItemToBin(TileEdge e) {
	this->data[e]++;
}

/**
 * Empty the bin on a given edge.
 * @param e Edge to query,
 * @pre This item is a bin, and a non-demolished bin exists on this edge.
 */
void PathObjectInstance::EmptyBin(TileEdge e) {
	this->data[e] = 0;
}

/**
 * Set the guest sitting on the left half of the bench on a given edge.
 * @param e Edge to query,
 * @param id The guest's ID (#PathObjectType::NO_GUEST_ON_BENCH to mark the seat as free).
 * @pre This item is a bench, and a non-demolished bench exists on this edge.
 */
void PathObjectInstance::SetLeftGuest(TileEdge e, uint16 id) {
	this->data[e] &= 0xFFFF0000;
	this->data[e] |= id;
}

/**
 * Set the guest sitting on the right half of the bench on a given edge.
 * @param e Edge to query,
 * @param id The guest's ID (#PathObjectType::NO_GUEST_ON_BENCH to mark the seat as free).
 * @pre This item is a bench, and a non-demolished bench exists on this edge.
 */
void PathObjectInstance::SetRightGuest(TileEdge e, uint16 id) {
	this->data[e] &= 0x0000FFFF;
	this->data[e] |= (id << 16);
}


static const uint32 CURRENT_VERSION_PathObjectInstance = 1;   ///< Currently supported version of %PathObjectInstance.

void PathObjectInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("pobj");
	if (version != CURRENT_VERSION_PathObjectInstance) ldr.VersionMismatch(version, CURRENT_VERSION_PathObjectInstance);
	this->pix_pos.x = ldr.GetWord();
	this->pix_pos.y = ldr.GetWord();
	this->pix_pos.z = ldr.GetWord();
	this->state = ldr.GetByte();
	for (uint8 i = 0; i < 4; i++) this->data[i] = ldr.GetLong();
	ldr.ClosePattern();
}

void PathObjectInstance::Save(Saver &svr) const
{
	svr.StartPattern("pobj", CURRENT_VERSION_PathObjectInstance);
	/* #type and #vox_pos are saved by #SceneryManager::Save. */
	svr.PutWord(this->pix_pos.x);
	svr.PutWord(this->pix_pos.y);
	svr.PutWord(this->pix_pos.z);
	svr.PutByte(this->state);
	for (uint8 i = 0; i < 4; i++) svr.PutLong(this->data[i]);
	svr.EndPattern();
}

/** Default constructor. */
SceneryType::SceneryType()
:
	category(SCC_SCENARIO),
	name(STR_NULL),
	width_x(0),
	width_y(0),
	watering_interval(0),
	min_watering_interval(0),
	symmetric(true),
	main_animation(nullptr),
	dry_animation(nullptr)
{
	std::fill_n(this->previews, lengthof(this->previews), nullptr);
}

/**
 * Load a type of scenery from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 */
void SceneryType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	rcd_file->CheckVersion(3);
	int length = rcd_file->size;
	if (length <= 2) rcd_file->Error("Length too short for header");

	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	if (this->width_x < 1 || this->width_y < 1) rcd_file->Error("Width is zero");

	length -= 52 + (this->width_x * this->width_y);
	if (length < 0) rcd_file->Error("Length too short for extended header");

	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; x++) {
		for (int8 y = 0; y < this->width_y; y++) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}

	this->watering_interval = rcd_file->GetUInt32();
	this->min_watering_interval = rcd_file->GetUInt32();
	this->main_animation = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	this->dry_animation = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	for (int i = 0; i < 4; i++) {
		ImageData *view;
		LoadSpriteFromFile(rcd_file, sprites, &view);
		this->previews[i] = view;
	}

	this->buy_cost = Money(rcd_file->GetInt32());
	this->return_cost = Money(rcd_file->GetInt32());
	this->return_cost_dry = Money(rcd_file->GetInt32());
	this->symmetric = rcd_file->GetUInt8() > 0;
	this->category = static_cast<SceneryCategory>(rcd_file->GetUInt8());

	TextData *text_data;
	LoadTextFromFile(rcd_file, texts, &text_data);
	this->name = _language.RegisterStrings(*text_data, _scenery_strings_table);

	this->internal_name = rcd_file->GetText();
	if (length != static_cast<int>(this->internal_name.size() + 1)) rcd_file->Error("Trailing bytes at end of block");
}

/**
 * Construct a new scenery item instance.
 * @param t Type of scenery item.
 */
SceneryInstance::SceneryInstance(const SceneryType *t)
:
	type(t),
	vox_pos(XYZPoint16::invalid()),
	orientation(0),
	animtime(0),
	time_since_watered(0)
{
}

/** Destructor. */
SceneryInstance::~SceneryInstance()
{
	if (_scenery.temp_item == this) _scenery.temp_item = nullptr;
	this->RemoveFromWorld();
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
	this->time_since_watered = 0;
	this->animtime = 0;
	this->MarkDirty();
}

/** Remove this item from the voxels it currently occupies. */
void SceneryInstance::RemoveFromWorld()
{
	this->MarkDirty();
#ifndef NDEBUG
	const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->type);
#endif
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
void SceneryInstance::GetSprites(const XYZPoint16 &vox, const uint16 voxel_number, const uint8 orient, const ImageData *sprites[4], [[maybe_unused]] uint8 *platform) const
{
	sprites[0] = nullptr;
	sprites[1] = nullptr;
	sprites[2] = nullptr;
	sprites[3] = nullptr;
	if (voxel_number == INVALID_VOXEL_DATA) return;
	const XYZPoint16 unrotated_pos = UnorientatedOffset(this->orientation, vox.x - this->vox_pos.x, vox.y - this->vox_pos.y);
	const TimedAnimation *anim = (this->IsDry() ? this->type->dry_animation : this->type->main_animation);
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
	const bool was_dry = this->IsDry();
	const uint32 old_animtime = this->animtime;

	if (this->type->watering_interval > 0) this->time_since_watered += delay;
	const TimedAnimation *anim = (this->IsDry() ? this->type->dry_animation : this->type->main_animation);
	this->animtime += delay;
	this->animtime %= anim->GetTotalDuration();

	if (this->IsDry() != was_dry || anim->GetFrame(old_animtime, true) != anim->GetFrame(this->animtime, true)) {
		this->MarkDirty();  // Ensure the animation is updated.
	}
}

/**
 * Whether this item is dried up for lack of watering.
 * @return The item is dry.
 */
bool SceneryInstance::IsDry() const
{
	return this->type->watering_interval > 0 && this->time_since_watered > this->type->watering_interval;
}

/**
 * Whether this item should be watered by a handyman soon.
 * @return The item needs watering soon.
 */
bool SceneryInstance::ShouldBeWatered() const
{
	return this->type->watering_interval > 0 && this->time_since_watered > this->type->min_watering_interval;
}

static const uint32 CURRENT_VERSION_SceneryInstance = 1;   ///< Currently supported version of %SceneryInstance.

void SceneryInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("scni");
	if (version != CURRENT_VERSION_SceneryInstance) ldr.VersionMismatch(version, CURRENT_VERSION_SceneryInstance);

	this->vox_pos.x = ldr.GetWord();
	this->vox_pos.y = ldr.GetWord();
	this->vox_pos.z = ldr.GetWord();
	this->orientation = ldr.GetByte();
	this->animtime = ldr.GetLong();
	this->time_since_watered = ldr.GetLong();
	ldr.ClosePattern();
}

void SceneryInstance::Save(Saver &svr) const
{
	svr.StartPattern("scni", CURRENT_VERSION_SceneryInstance);
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
	svr.PutByte(this->orientation);
	svr.PutLong(this->animtime);
	svr.PutLong(this->time_since_watered);
	svr.EndPattern();
}

/** Default constructor. */
SceneryManager::SceneryManager() : temp_item(nullptr), temp_path_object(nullptr)
{
}

/**
 * Register a new scenery type.
 * @param type Scenery type to add.
 * @note On success, takes ownership of the pointer and clears the passed smart pointer.
 */
void SceneryManager::AddSceneryType(std::unique_ptr<SceneryType> &type)
{
	if (type->internal_name.empty() || this->GetType(type->internal_name) != nullptr) {
		throw LoadingError("Invalid or duplicate scenery name '%s'", type->internal_name.c_str());
	}
	this->scenery_item_types.emplace_back(std::move(type));
}

/**
 * Retrieve the index of a scenery type.
 * @param type Type to query.
 * @return The unique index.
 */
uint16 SceneryManager::GetSceneryTypeIndex(const SceneryType *type) const
{
	for (uint i = 0; i < this->scenery_item_types.size(); i++) {
		if (this->scenery_item_types[i].get() == type) {
			return i;
		}
	}
	NOT_REACHED();
}

/**
 * Retrieve the scenery type with a given index.
 * @param index Index to query.
 * @return The type (\c nullptr for invalid indices).
 */
const SceneryType *SceneryManager::GetType(const uint16 index) const
{
	return this->scenery_item_types[index].get();
}

/**
 * Retrieve the scenery type with a given internal name.
 * @param internal_name Internal name of the type.
 * @return The type (\c nullptr for invalid names).
 */
const SceneryType *SceneryManager::GetType(const std::string &internal_name) const
{
	for (const auto &t : this->scenery_item_types) if (t->internal_name == internal_name) return t.get();
	return nullptr;
}

/**
 * Returns all scenery types with the given category.
 * @param cat Category of interest.
 * @return All types in the category.
 */
std::vector<const SceneryType*> SceneryManager::GetAllTypes(SceneryCategory cat) const
{
	std::vector<const SceneryType*> result;
	for (const auto &item_type : this->scenery_item_types) {
		if (item_type->category == cat) {
			result.push_back(item_type.get());
		}
	}
	return result;
}

/** Remove all scenery items. */
void SceneryManager::Clear()
{
	this->temp_item = nullptr;
	this->temp_path_object = nullptr;
	while (!this->all_items.empty()) this->RemoveItem(this->all_items.begin()->first);
	/* Do not use std::map::clear(), it may result in a heap-use-after-free. */
	while (!this->litter_and_vomit.empty()) this->litter_and_vomit.erase(this->litter_and_vomit.begin());
	while (!this->all_path_objects.empty()) this->all_path_objects.erase(this->all_path_objects.begin());
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
	this->all_items.erase(it);  // This deletes the instance.
}

/**
 * Count the amount of litter and vomit on a path.
 * @return The amount of dirt (\c 0 if the path is clean).
 */
uint SceneryManager::CountLitterAndVomit(const XYZPoint16 &pos) const
{
	return this->litter_and_vomit.count(pos);
}

/**
 * Count the amount of vandalised items on a path.
 * @return The amount of vandalised items.
 */
uint8 SceneryManager::CountDemolishedItems(const XYZPoint16 &pos) const
{
	const auto it = this->all_path_objects.find(pos);
	if (it == this->all_path_objects.end()) return 0;

	uint8 result = 0;
	for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
		if (it->second->GetExistsOnTileEdge(e) && it->second->GetDemolishedOnTileEdge(e)) {
			result++;
		}
	}
	return result;
}

/**
 * Build a path object to a path. This replaces any other object previously present there.
 * @param pos Coordinate of the path.
 * @param type Object to place (may be \c nullptr to delete item without replacing them).
 */
void SceneryManager::SetPathObjectInstance(const XYZPoint16 &pos, const PathObjectType *type)
{
	if (type == nullptr) {
		this->all_path_objects.erase(pos);
	} else {
		this->all_path_objects[pos].reset(new PathObjectInstance(type, pos, XYZPoint16(/* Offset is ignored for user-placeable types. */)));
	}
}

/**
 * Get the path object at a given path.
 * @param pos Coordinate of the path.
 * @return The object there (may be \c nullptr).
 */
PathObjectInstance *SceneryManager::GetPathObject(const XYZPoint16 &pos)
{
	auto it = this->all_path_objects.find(pos);
	return it == this->all_path_objects.end() ? nullptr : it->second.get();
}

/**
 * Add some litter to a path.
 * @param pos Coordinate of the path.
 * @param offset Offset of the item inside the voxel.
 */
void SceneryManager::AddLitter(const XYZPoint16 &pos, const XYZPoint16 &offset)
{
	this->litter_and_vomit.emplace(pos, std::unique_ptr<PathObjectInstance>(new PathObjectInstance(&PathObjectType::LITTER, pos, offset)));
}

/**
 * Add some vomit to a path.
 * @param pos Coordinate of the path.
 * @param offset Offset of the item inside the voxel.
 */
void SceneryManager::AddVomit(const XYZPoint16 &pos, const XYZPoint16 &offset)
{
	this->litter_and_vomit.emplace(pos, std::unique_ptr<PathObjectInstance>(new PathObjectInstance(&PathObjectType::VOMIT, pos, offset)));
}

/**
 * Remove all litter and vomit from a path.
 * @param pos Coordinate of the path.
 */
void SceneryManager::RemoveLitterAndVomit(const XYZPoint16 &pos)
{
	this->litter_and_vomit.erase(pos);
}

/**
 * Get all path objects that should be drawn at a given path.
 * @param pos Coordinate of the path.
 * @param orientation Viewport orientation.
 * @return All path objects to draw.
 */
std::vector<PathObjectInstance::PathObjectSprite> SceneryManager::DrawPathObjects(const XYZPoint16 &pos, const uint8 orientation) const
{
	std::vector<PathObjectInstance::PathObjectSprite> result;

	for (const auto &pair : this->litter_and_vomit) {
		if (pair.first == pos) {
			for (const PathObjectInstance::PathObjectSprite &image : pair.second->GetSprites(orientation)) {
				result.push_back(image);
			}
		}
	}

	auto it = this->all_path_objects.find(pos);
	if (it != this->all_path_objects.end()) {
		for (const PathObjectInstance::PathObjectSprite &image : it->second->GetSprites(orientation)) {
			result.push_back(image);
		}
	}

	if (this->temp_path_object != nullptr && this->temp_path_object->vox_pos == pos) {
		for (PathObjectInstance::PathObjectSprite image : this->temp_path_object->GetSprites(orientation)) {
			image.semi_transparent = true;
			result.push_back(image);
		}
	}

	return result;
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

static const uint32 CURRENT_VERSION_SceneryInstance_SCNY = 3;   ///< Currently supported version of the SCNY Pattern.

void SceneryManager::Load(Loader &ldr)
{
	this->Clear();
	const uint32 version = ldr.OpenPattern("SCNY");
	switch (version) {
		case 0:
			break;
		case 1:
		case 2:
		case 3:
			for (long l = ldr.GetLong(); l > 0; l--) {
				SceneryInstance *i = new SceneryInstance(version >= 3 ? this->GetType(ldr.GetText()) : this->scenery_item_types[ldr.GetWord()].get());
				i->Load(ldr);
				this->all_items[i->vox_pos] = std::unique_ptr<SceneryInstance>(i);
			}
			if (version > 1) {
				for (long l = ldr.GetLong(); l > 0; l--) {
					XYZPoint16 pos;
					pos.x = ldr.GetWord();
					pos.y = ldr.GetWord();
					pos.z = ldr.GetWord();
					PathObjectInstance *i = new PathObjectInstance(PathObjectType::Get(ldr.GetByte()), pos, pos /* will be overwritten by #Load() */);
					i->Load(ldr);
					this->all_path_objects[pos] = std::unique_ptr<PathObjectInstance>(i);
				}
				for (long l = ldr.GetLong(); l > 0; l--) {
					XYZPoint16 pos;
					pos.x = ldr.GetWord();
					pos.y = ldr.GetWord();
					pos.z = ldr.GetWord();
					PathObjectInstance *i = new PathObjectInstance(PathObjectType::Get(ldr.GetByte()), pos, pos);
					i->Load(ldr);
					this->litter_and_vomit.emplace(pos, std::unique_ptr<PathObjectInstance>(i));
				}
			}
			break;

		default:
			ldr.VersionMismatch(version, CURRENT_VERSION_SceneryInstance_SCNY);
	}
	ldr.ClosePattern();
}

void SceneryManager::Save(Saver &svr) const
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("SCNY", CURRENT_VERSION_SceneryInstance_SCNY);

	svr.PutLong(this->all_items.size());
	for (const auto &pair : this->all_items) {
		svr.PutText(pair.second->type->internal_name);
		pair.second->Save(svr);
	}

	svr.PutLong(this->all_path_objects.size());
	for (const auto &pair : this->all_path_objects) {
		svr.PutWord(pair.first.x);
		svr.PutWord(pair.first.y);
		svr.PutWord(pair.first.z);
		svr.PutByte(pair.second->type->type_id);
		pair.second->Save(svr);
	}

	svr.PutLong(this->litter_and_vomit.size());
	for (const auto &pair : this->litter_and_vomit) {
		svr.PutWord(pair.first.x);
		svr.PutWord(pair.first.y);
		svr.PutWord(pair.first.z);
		svr.PutByte(pair.second->type->type_id);
		pair.second->Save(svr);
	}

	svr.EndPattern();
}
