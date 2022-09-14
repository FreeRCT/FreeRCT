/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform.cpp Terraforming code. */

#include "stdafx.h"
#include "map.h"
#include "viewport.h"
#include "terraform.h"
#include "finances.h"
#include "gamecontrol.h"
#include "math_func.h"
#include "memory.h"

static const Money TERRAFORM_UNIT_COST(40);  ///< The cost for applying one elemental unit of terraforming modifications.

/**
 * How much it costs to apply one elemental unit of terraforming modifications.
 * @return The cost.
 */
const Money &GetTerraformUnitCost()
{
	return TERRAFORM_UNIT_COST;
}

/**
 * Structure describing a corner at a voxel stack.
 * @ingroup map_group
 */
struct VoxelCorner {
	Point16 rel_xy;    ///< Relative voxel stack position.
	TileCorner corner; ///< Corner of the voxel.
};

/**
 * Description of neighbouring corners of a corner at a ground tile.
 * #left_neighbour and #right_neighbour are neighbours at the same tile, while
 * #neighbour_tiles list neighbouring corners at the three tiles around the
 * corner.
 * @ingroup map_group
 */
struct CornerNeighbours {
	TileCorner left_neighbour;      ///< Left neighbouring corner.
	TileCorner right_neighbour;     ///< Right neighbouring corner.
	VoxelCorner neighbour_tiles[3]; ///< Neighbouring corners at other tiles.
};

/**
 * Neighbouring corners of each corner.
 * @ingroup map_group
 */
static const CornerNeighbours neighbours[4] = {
	{TC_EAST,  TC_WEST,  { {{-1, -1}, TC_SOUTH}, {{-1,  0}, TC_WEST }, {{ 0, -1}, TC_EAST }} }, // TC_NORTH
	{TC_NORTH, TC_SOUTH, { {{-1,  0}, TC_SOUTH}, {{-1,  1}, TC_WEST }, {{ 0,  1}, TC_NORTH}} }, // TC_EAST
	{TC_EAST,  TC_WEST,  { {{ 0,  1}, TC_WEST }, {{ 1,  1}, TC_NORTH}, {{ 1,  0}, TC_EAST }} }, // TC_SOUTH
	{TC_SOUTH, TC_NORTH, { {{ 0, -1}, TC_SOUTH}, {{ 1, -1}, TC_EAST }, {{ 1,  0}, TC_NORTH}} }, // TC_WEST
};

/**
 * Construct a #GroundData structure.
 * @param height Height of the voxel containing the surface (for steep slopes, the base height).
 * @param orig_slope Original slope (that is, before the raise or lower).
 */
GroundData::GroundData(uint8 height, uint8 orig_slope)
{
	this->height = height;
	this->orig_slope = orig_slope;
	this->modified = 0;
}

/**
 * Get original height (before changing).
 * @param corner Corner to get height.
 * @return Original height of the indicated corner.
 */
uint8 GroundData::GetOrigHeight(TileCorner corner) const
{
	assert(corner >= TC_NORTH && corner < TC_END);
	if ((this->orig_slope & TSB_STEEP) == 0) { // Normal slope.
		if ((this->orig_slope & (1 << corner)) == 0) return this->height;
		return this->height + 1;
	}
	// Steep slope (note that the constructor made sure this->height is at the base of the steep slope).
	if ((this->orig_slope & (1 << corner)) != 0) return this->height + 2;
	corner = (TileCorner)((corner + 2) % 4);
	if ((this->orig_slope & (1 << corner)) != 0) return this->height;
	return this->height + 1;
}

/**
 * Set the given corner as modified.
 * @param corner Corner to set.
 * @return corner is modified.
 */
void GroundData::SetCornerModified(TileCorner corner)
{
	assert(corner >= TC_NORTH && corner < TC_END);
	this->modified |= 1 << corner;
}

/**
 * Return whether the given corner is modified or not.
 * @param corner Corner to test.
 * @return corner is modified.
 */
bool GroundData::GetCornerModified(TileCorner corner) const
{
	assert(corner >= TC_NORTH && corner < TC_END);
	return (this->modified & (1 << corner)) != 0;
}

/**
 * Terrain changes storage constructor.
 * @param init_base Base coordinate of the part of the world which is smoothly updated.
 * @param init_xsize Horizontal size of the world part.
 * @param init_ysize Vertical size of the world part.
 */
TerrainChanges::TerrainChanges(const Point16 &init_base, uint16 init_xsize, uint16 init_ysize)
: base(init_base), xsize(init_xsize), ysize(init_ysize)
{
	assert(this->base.x >= 0 && this->base.y >= 0 && this->xsize > 0 && this->ysize > 0
			&& this->base.x + this->xsize <= _world.GetXSize() && this->base.y + this->ysize <= _world.GetYSize());
}

/** Destructor. */
TerrainChanges::~TerrainChanges()
= default;

/**
 * Get ground data of a voxel stack.
 * @param pos Voxel stack position.
 * @return Pointer to the ground data, or \c nullptr if outside the world.
 */
GroundData *TerrainChanges::GetGroundData(const Point16 &pos)
{
	if (pos.x < this->base.x || pos.x >= this->base.x + this->xsize) return nullptr;
	if (pos.y < this->base.y || pos.y >= this->base.y + this->ysize) return nullptr;

	auto iter = this->changes.find(pos);
	if (iter == this->changes.end()) {
		uint8 height = _world.GetBaseGroundHeight(pos.x, pos.y);
		const Voxel *v = _world.GetVoxel(XYZPoint16(pos.x, pos.y, height));
		assert(v != nullptr && v->GetGroundType() != GTP_INVALID);
		std::pair<Point16, GroundData> p(pos, GroundData(height, ExpandTileSlope(v->GetGroundSlope())));
		iter = this->changes.insert(p).first;
	}
	return &(*iter).second;
}

/**
 * Test every corner of the given voxel for its original height, and find the extreme value.
 * @param pos %Voxel position.
 * @param direction Levelling direction (decides whether to find the lowest or highest corner).
 * @param height [inout] Extremest height found so far.
 */
void TerrainChanges::UpdatelevellingHeight(const Point16 &pos, int direction, uint8 *height)
{
	const GroundData *gd = this->GetGroundData(pos);

	if (direction > 0) { // Raising terrain, find the lowest corner.
		*height = std::min(*height, gd->GetOrigHeight(TC_NORTH));
		*height = std::min(*height, gd->GetOrigHeight(TC_EAST));
		*height = std::min(*height, gd->GetOrigHeight(TC_SOUTH));
		*height = std::min(*height, gd->GetOrigHeight(TC_WEST));
	} else { // Lowering terrain, find the highest corner.
		*height = std::max(*height, gd->GetOrigHeight(TC_NORTH));
		*height = std::max(*height, gd->GetOrigHeight(TC_EAST));
		*height = std::max(*height, gd->GetOrigHeight(TC_SOUTH));
		*height = std::max(*height, gd->GetOrigHeight(TC_WEST));
	}
}

/**
 * Change corners of a voxel if they are within the height constraint.
 * @param pos %Voxel position.
 * @param height Minimum or maximum height of the corners to modify.
 * @param direction Levelling direction (decides what constraint to use).
 * @return Change is OK for the map.
 */
bool TerrainChanges::ChangeVoxel(const Point16 &pos, uint8 height, int direction)
{
	GroundData *gd = this->GetGroundData(pos);
	bool ok = true;
	if (direction > 0) { // Raising terrain, raise everything at or below 'height'.
		if (gd->GetOrigHeight(TC_NORTH) <= height) ok &= this->ChangeCorner(pos, TC_NORTH, direction);
		if (gd->GetOrigHeight(TC_EAST)  <= height) ok &= this->ChangeCorner(pos, TC_EAST,  direction);
		if (gd->GetOrigHeight(TC_SOUTH) <= height) ok &= this->ChangeCorner(pos, TC_SOUTH, direction);
		if (gd->GetOrigHeight(TC_WEST)  <= height) ok &= this->ChangeCorner(pos, TC_WEST,  direction);
	} else { // Lowering terrain, lower everything above or at 'height'.
		if (gd->GetOrigHeight(TC_NORTH) >= height) ok &= this->ChangeCorner(pos, TC_NORTH, direction);
		if (gd->GetOrigHeight(TC_EAST)  >= height) ok &= this->ChangeCorner(pos, TC_EAST,  direction);
		if (gd->GetOrigHeight(TC_SOUTH) >= height) ok &= this->ChangeCorner(pos, TC_SOUTH, direction);
		if (gd->GetOrigHeight(TC_WEST)  >= height) ok &= this->ChangeCorner(pos, TC_WEST,  direction);
	}
	return ok;
}

/**
 * Change the height of a corner. Call this function for every corner you want to change.
 * @param pos Position of the voxel stack.
 * @param corner Corner to change.
 * @param direction Direction of change.
 * @return Change is OK for the map.
 */
bool TerrainChanges::ChangeCorner(const Point16 &pos, TileCorner corner, int direction)
{
	assert(corner >= TC_NORTH && corner < TC_END);
	assert(direction == 1 || direction == -1);

	GroundData *gd = this->GetGroundData(pos);
	if (gd == nullptr) return true; // Out of the bounds in the world, silently ignore.
	if (gd->GetCornerModified(corner)) return true; // Corner already changed.

	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(pos.x, pos.y) != OWN_PARK) return false;

	uint8 old_height = gd->GetOrigHeight(corner);
	if (direction > 0 && old_height == WORLD_Z_SIZE) return false; // Cannot change above top.
	if (direction < 0 && old_height == 0) return false; // Cannot change below bottom.

	gd->SetCornerModified(corner); // Mark corner as modified.

	/* Change neighbouring corners at the same tile. */
	if (direction > 0) {
		uint8 h = gd->GetOrigHeight(neighbours[corner].left_neighbour);
		if (h < old_height && !this->ChangeCorner(pos, neighbours[corner].left_neighbour, direction)) return false;
		h = gd->GetOrigHeight(neighbours[corner].right_neighbour);
		if (h < old_height && !this->ChangeCorner(pos, neighbours[corner].right_neighbour, direction)) return false;
	} else {
		uint8 h = gd->GetOrigHeight(neighbours[corner].left_neighbour);
		if (h > old_height && !this->ChangeCorner(pos, neighbours[corner].left_neighbour, direction)) return false;
		h = gd->GetOrigHeight(neighbours[corner].right_neighbour);
		if (h > old_height && !this->ChangeCorner(pos, neighbours[corner].right_neighbour, direction)) return false;
	}

	for (uint i = 0; i < 3; i++) {
		const VoxelCorner &vc = neighbours[corner].neighbour_tiles[i];
		Point16 pos2(pos.x + vc.rel_xy.x, pos.y + vc.rel_xy.y);
		gd = this->GetGroundData(pos2);
		if (gd == nullptr) continue;
		if (_world.GetTileOwner(pos2.x, pos2.y) != OWN_PARK) continue;
		uint height = gd->GetOrigHeight(vc.corner);
		if (old_height == height && !this->ChangeCorner(pos2, vc.corner, direction)) return false;
	}
	return true;
}

/**
 * Set an upper boundary of the height of each tile corner based on the contents of a voxel.
 * @param v %Voxel to examine.
 * @param height Height of the voxel.
 * @param bounds [inout] Updated constraints.
 * @note Length of the \a bounds array should be 4.
 */
static void SetUpperBoundary(const Voxel *v, uint8 height, uint8 *bounds)
{
	if (v == nullptr || v->IsEmpty()) return;

	uint16 instance = v->GetInstance();
	if (instance >= SRI_FULL_RIDES || instance == SRI_SCENERY) { // Rides and scenery items need the entire voxel.
		std::fill_n(bounds, 4, height);
		return;
	}

	if (instance < SRI_RIDES_START) return;

	/* Small rides, that is, a path. */
	if (!HasValidPath(v)) return;
	PathSprites ps = GetImplodedPathSlope(v);
	switch (ps) {
		case PATH_RAMP_NE:
			bounds[TC_NORTH] = height;     bounds[TC_EAST] = height;
			bounds[TC_SOUTH] = height + 1; bounds[TC_WEST] = height + 1;
			break;

		case PATH_RAMP_NW:
			bounds[TC_NORTH] = height;     bounds[TC_WEST] = height;
			bounds[TC_SOUTH] = height + 1; bounds[TC_EAST] = height + 1;
			break;

		case PATH_RAMP_SE:
			bounds[TC_SOUTH] = height;     bounds[TC_EAST] = height;
			bounds[TC_NORTH] = height + 1; bounds[TC_WEST] = height + 1;
			break;

		case PATH_RAMP_SW:
			bounds[TC_SOUTH] = height;     bounds[TC_WEST] = height;
			bounds[TC_NORTH] = height + 1; bounds[TC_EAST] = height + 1;
			break;

		default:
			assert(ps < PATH_FLAT_COUNT);
			std::fill_n(bounds, 4, height);
			break;
	}
}

/**
 * Set a lower boundary of the height of each tile corner based on the contents of a voxel.
 * @param v %Voxel to examine.
 * @param height Height of the voxel.
 * @param bounds [inout] Updated constraints.
 * @note Length of the \a bounds array should be 4.
 */
static void SetLowerBoundary(const Voxel *v, uint8 height, uint8 *bounds)
{
	if (v == nullptr || v->IsEmpty()) return;
	if (v->GetInstance() == SRI_SCENERY) {  // Scenery items need the entire voxel.
		std::fill_n(bounds, 4, height);
		return;
	}
	/* \todo Implement underground paths and rides. */
}

/**
 * Set foundations in a stack.
 * @param stack %Voxel stack to change (may be \c nullptr).
 * @param my_first Height of my first corner.
 * @param my_second Height of my second corner.
 * @param other_first Height of corner opposite to \a my_first.
 * @param other_second Height of corner opposite to \a my_second.
 * @param first_bit Bit value to use for a raised foundation at the first corner.
 * @param second_bit Bit value to use for a raised foundation at the second corner.
 */
static void SetFoundations(VoxelStack *stack, uint8 my_first, uint8 my_second, uint8 other_first, uint8 other_second, uint8 first_bit, uint8 second_bit)
{
	if (stack == nullptr) return;

	uint8 and_bits = ~(first_bit | second_bit);
	bool is_higher = my_first > other_first || my_second > other_second; // At least one of my corners must be higher to ever add foundations.

	uint8 h = stack->base;
	uint8 highest = stack->base + stack->height;

	if (other_first  < h) h = other_first;
	if (other_second < h) h = other_second;

	while (h < highest) {
		uint8 bits = 0;
		if (is_higher && (h >= other_first || h >= other_second)) {
			if (h < my_first)  bits |= first_bit;
			if (h < my_second) bits |= second_bit;
		}

		Voxel *v = stack->GetCreate(h, true);
		h++;
		if (bits == 0) { // Delete foundations.
			if (v->GetFoundationType() == FDT_INVALID) continue;
			bits = v->GetFoundationSlope() & and_bits;
			v->SetFoundationSlope(bits);
			if (bits == 0) v->SetFoundationType(FDT_INVALID);
			continue;
		} else { // Add foundations.
			if (v->GetFoundationType() == FDT_INVALID) {
				v->SetFoundationType(FDT_GROUND); // XXX We have nice foundation types, but no way to select them here.
			} else {
				bits |= (v->GetFoundationSlope() & and_bits);
			}
			v->SetFoundationSlope(bits);
			continue;
		}
	}
}

/**
 * Update the foundations in two voxel stacks along the SW edge of the first stack and the NE edge of the second stack.
 * @param xpos X position of the first voxel stack.
 * @param ypos Y position of the first voxel stack.
 * @note The first or the second voxel stack may be off-world.
 */
static void SetXFoundations(int xpos, int ypos)
{
	VoxelStack *first = (xpos < 0) ? nullptr : _world.GetModifyStack(xpos, ypos);
	VoxelStack *second = (xpos + 1 == _world.GetXSize()) ? nullptr : _world.GetModifyStack(xpos + 1, ypos);
	assert(first != nullptr || second != nullptr);

	/* Get ground height at all corners. */
	uint8 first_south = 0;
	uint8 first_west  = 0;
	if (first != nullptr) {
		for (uint i = 0; i < first->height; i++) {
			const Voxel *v = first->voxels[i].get();
			if (v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), first->base + i, heights);
			first_south = heights[TC_SOUTH];
			first_west  = heights[TC_WEST];
			break;
		}
	}

	uint8 second_north = 0;
	uint8 second_east  = 0;
	if (second != nullptr) {
		for (uint i = 0; i < second->height; i++) {
			const Voxel *v = second->voxels[i].get();
			if (v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), second->base + i, heights);
			second_north = heights[TC_NORTH];
			second_east  = heights[TC_EAST];
			break;
		}
	}

	SetFoundations(first,  first_south, first_west, second_east, second_north, 0x10, 0x20);
	SetFoundations(second, second_north, second_east, first_west, first_south, 0x01, 0x02);
}

/**
 * Update the foundations in two voxel stacks along the SE edge of the first stack and the NW edge of the second stack.
 * @param xpos X position of the first voxel stack.
 * @param ypos Y position of the first voxel stack.
 * @note The first or the second voxel stack may be off-world.
 */
static void SetYFoundations(int xpos, int ypos)
{
	VoxelStack *first = (ypos < 0) ? nullptr : _world.GetModifyStack(xpos, ypos);
	VoxelStack *second = (ypos + 1 == _world.GetYSize()) ? nullptr : _world.GetModifyStack(xpos, ypos + 1);
	assert(first != nullptr || second != nullptr);

	/* Get ground height at all corners. */
	uint8 first_south = 0;
	uint8 first_east  = 0;
	if (first != nullptr) {
		for (uint i = 0; i < first->height; i++) {
			const Voxel *v = first->voxels[i].get();
			if (v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), first->base + i, heights);
			first_south = heights[TC_SOUTH];
			first_east  = heights[TC_EAST];
			break;
		}
	}

	uint8 second_north = 0;
	uint8 second_west  = 0;
	if (second != nullptr) {
		for (uint i = 0; i < second->height; i++) {
			const Voxel *v = second->voxels[i].get();
			if (v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), second->base + i, heights);
			second_north = heights[TC_NORTH];
			second_west  = heights[TC_WEST];
			break;
		}
	}

	SetFoundations(first,  first_south, first_east, second_west, second_north, 0x08, 0x04);
	SetFoundations(second, second_north, second_west, first_east, first_south, 0x80, 0x40);
}

/**
 * Perform the proposed changes.
 * @param direction Direction of change.
 * @return Whether the change could actually be performed (else nothing is changed).
 */
bool TerrainChanges::ModifyWorld(const int direction)
{
	/* First iteration: Check that the world can be safely changed (no collisions with other game elements.) */
	Money total_cost(0);
	for (auto &iter : this->changes) {
		const Point16 &pos = iter.first;
		const GroundData &gd = iter.second;
		if (gd.modified == 0) continue;

		uint8 current[4]; // Height of each corner after applying modification.
		ComputeCornerHeight(static_cast<TileSlope>(gd.orig_slope), gd.height, current);

		/* Apply modification. */
		for (uint8 i = TC_NORTH; i < TC_END; i++) {
			if ((gd.modified & (1 << i)) == 0) continue; // Corner was not changed.
			current[i] += direction;
			total_cost += GetTerraformUnitCost();
		}

		if (direction > 0) {
			/* Moving upwards, compute upper bound on corner heights. */
			uint8 max_above[4];
			std::fill_n(max_above, lengthof(max_above), std::min(gd.height + 3, WORLD_Z_SIZE - 1));

			const VoxelStack *vs = _world.GetStack(pos.x, pos.y);
			for (int i = 2; i >= 0; i--) {
				SetUpperBoundary(vs->Get(gd.height + i), gd.height + i, max_above);
			}

			/* Check boundaries. */
			for (uint i = 0; i < 4; i++) {
				if (current[i] > max_above[i]) return false;
			}
		} else if (direction < 0) {
			/* Moving downwards, compute lower bound on corner heights. */
			uint8 max_below[4];
			std::fill_n(max_below, lengthof(max_below), gd.height >= 3 ? gd.height - 3 : 0);

			const VoxelStack *vs = _world.GetStack(pos.x, pos.y);
			for (int i = std::min<int>(2, gd.height); i >= 0; i--) {
				SetLowerBoundary(vs->Get(gd.height - i), gd.height - i, max_below);
			}

			/* Check boundaries. */
			for (uint i = 0; i < 4; i++) {
				if (current[i] < max_below[i]) return false;
			}
		}
	}

	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, total_cost)) return false;
	if (_game_control.action_test_mode) {
		ShowCostOrReturnEstimate(total_cost);
		return true;
	}
	_finances_manager.PayLandscaping(total_cost);
	_window_manager.GetViewport()->AddFloatawayMoneyAmount(total_cost, XYZPoint16(
			this->changes.begin()->first.x, this->changes.begin()->first.y, this->changes.begin()->second.height));

	/* Second iteration: Change the ground of the tiles. */
	for (auto &iter : this->changes) {
		const Point16 &pos = iter.first;
		const GroundData &gd = iter.second;
		if (gd.modified == 0) continue;

		uint8 current[4]; // Height of each corner after applying modification.
		ComputeCornerHeight(static_cast<TileSlope>(gd.orig_slope), gd.height, current);

		/* Apply modification. */
		for (uint8 i = TC_NORTH; i < TC_END; i++) {
			if ((gd.modified & (1 << i)) == 0) continue; // Corner was not changed.
			current[i] += direction;
		}

		/* Clear the current ground from the stack. */
		VoxelStack *vs = _world.GetModifyStack(pos.x, pos.y);
		Voxel *v = vs->GetCreate(gd.height, false); // Should always exist.
		GroundType gt = v->GetGroundType();
		assert(gt != GTP_INVALID);
		FoundationType ft = v->GetFoundationType();
		uint16 fences = GetGroundFencesFromMap(vs, gd.height);

		uint8 slope = v->GetGroundSlope();
		assert(!IsImplodedSteepSlopeTop(slope));
		AddGroundFencesToMap(ALL_INVALID_FENCES, vs, gd.height);
		v->SetGroundType(GTP_INVALID);
		v->SetFoundationType(FDT_INVALID);
		v->SetGroundSlope(0);
		v->SetFoundationSlope(0);
		if (IsImplodedSteepSlope(slope)) {
			Voxel *w = vs->GetCreate(gd.height + 1, false);
			assert(w->GetGroundType() == gt); // Should be the same type of ground as the base voxel.
			w->SetFoundationType(FDT_INVALID);
			w->SetGroundType(GTP_INVALID);
			w->SetGroundSlope(0);
			w->SetFoundationSlope(0);
		}

		/* Add new ground to the stack. */
		TileSlope new_slope;
		uint8 height;
		ComputeSlopeAndHeight(current, &new_slope, &height);
		assert(height < WORLD_Z_SIZE);

		v = vs->GetCreate(height, true);
		v->SetGroundSlope(new_slope);
		v->SetGroundType(gt);
		v->SetFoundationType(ft);
		v->SetFoundationSlope(0);
		if (IsImplodedSteepSlope(new_slope)) {
			v = vs->GetCreate(height + 1, true);
			/* Only for steep slopes, the upper voxel will have actual ground. */
			v->SetGroundType(gt);
			v->SetGroundSlope(new_slope + TS_TOP_OFFSET); // Set top-part as well for steep slopes.
			v->SetFoundationType(ft);
			v->SetFoundationSlope(0);
		}
		AddGroundFencesToMap(fences, vs, height); // Add fences last, as it assumes ground has been fully set.
	}

	/* Third iteration: Add foundations to every changed tile edge.
	 * The general idea is that each modified voxel handles adding
	 * of foundation to its SE and SW edge. If the NE or NW voxel is not
	 * modified, the voxel will have to perform adding of foundations
	 * there as well. */
	for (auto &iter : this->changes) {
		const Point16 &pos = iter.first;
		const GroundData &gd = iter.second;
		if (gd.modified == 0) continue;

		SetXFoundations(pos.x, pos.y);
		SetYFoundations(pos.x, pos.y);

		Point16 pt(pos.x - 1, pos.y);
		auto iter2 = this->changes.find(pt);
		if (iter2 == this->changes.end()) {
			SetXFoundations(pt.x, pt.y);
		} else {
			const GroundData &gd = iter2->second;
			if (gd.modified == 0) SetXFoundations(pt.x, pt.y);
		}

		pt.x = pos.x;
		pt.y = pos.y - 1;
		iter2 = this->changes.find(pt);
		if (iter2 == this->changes.end()) {
			SetYFoundations(pt.x, pt.y);
		} else {
			const GroundData &gd = (*iter2).second;
			if (gd.modified == 0) SetYFoundations(pt.x, pt.y);
		}
	}

	return true;
}

/**
 * Change the terrain while in 'dot' mode (i.e. a single corner or a single tile changing the entire world).
 * @param voxel_pos Position of the center voxel.
 * @param ctype Cursor type.
 * @param levelling If \c true, use levelling mode (only change the lowest/highest corners of a tile), else move every corner.
 * @param direction Direction of change.
 * @param dot_mode Using dot-mode (infinite world changes).
 */
void ChangeTileCursorMode(const Point16 &voxel_pos, CursorType ctype, bool levelling, int direction, bool dot_mode)
{
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return;

	Point16 p;
	uint16 w, h;

	if (dot_mode) { // Change entire world.
		p = {0, 0};
		w = _world.GetXSize();
		h = _world.GetYSize();
	} else { // Single tile mode.
		p = voxel_pos;
		w = 1;
		h = 1;
	}
	TerrainChanges changes(p, w, h);

	p = voxel_pos;

	bool ok;
	switch (ctype) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST:
			ok = changes.ChangeCorner(p, static_cast<TileCorner>(ctype - CUR_TYPE_NORTH), direction);
			break;

		case CUR_TYPE_TILE: {
			uint8 height = (direction > 0) ? WORLD_Z_SIZE : 0;
			if (levelling) changes.UpdatelevellingHeight(p, direction, &height);
			ok = changes.ChangeVoxel(p, height, direction);
			break;
		}

		default:
			NOT_REACHED();
	}

	if (ok) changes.ModifyWorld(direction);
}

/**
 * Change the terrain while in 'area' mode (i.e. a rectangle of tiles that changes).
 * @param orig_area Affected area (maybe partly off-world).
 * @param levelling If \c true, use levelling mode (only change the lowest/highest corners of a tile), else move every corner.
 * @param direction Direction of change.
 */
void ChangeAreaCursorMode(const Rectangle16 &orig_area, bool levelling, int direction)
{
	Point16 p;

	Rectangle16 area(orig_area); // Restrict area to on-world.
	area.RestrictTo(0, 0, _world.GetXSize(), _world.GetYSize());
	if (area.width == 0 || area.height == 0) return;

	TerrainChanges changes(area.base, area.width, area.height);

	uint8 height = (direction > 0) ? WORLD_Z_SIZE : 0;
	if (levelling) {
		for (uint dx = 0; dx < area.width; dx++) {
			p.x = area.base.x + dx;
			for (uint dy = 0; dy < area.height; dy++) {
				p.y = area.base.y + dy;
				if (_game_mode_mgr.InEditorMode() || _world.GetTileOwner(p.x, p.y) == OWN_PARK) {
					changes.UpdatelevellingHeight(p, direction, &height);
				}
			}
		}
	}

	/* Make the change in 'changes'. */
	for (uint dx = 0; dx < area.width; dx++) {
		p.x = area.base.x + dx;
		for (uint dy = 0; dy < area.height; dy++) {
			p.y = area.base.y + dy;
			if (_game_mode_mgr.InEditorMode() || _world.GetTileOwner(p.x, p.y) == OWN_PARK) {
				if (!changes.ChangeVoxel(p, height, direction)) return;
			}
		}
	}

	changes.ModifyWorld(direction);
}
