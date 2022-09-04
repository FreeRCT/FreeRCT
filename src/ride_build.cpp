/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_build.cpp Implementation of a builder for simple rides. */

#include "stdafx.h"
#include "viewport.h"
#include "sprite_store.h"
#include "shop_type.h"
#include "gentle_thrill_ride_type.h"
#include "mouse_mode.h"
#include "gamecontrol.h"

#include "gui_sprites.h"

/** Widgets of the simple ride build window. */
enum RideBuildWidgets {
	RBW_TITLEBAR,     ///< Titlebar text.
	RBW_TYPE_NAME,    ///< Label for the name of the simple ride.
	RBW_COST,         ///< Label for the cost of the simple ride.
	RBW_DISPLAY_RIDE, ///< Display the ride.
	RBW_POS_ROTATE,   ///< Positive rotate button.
	RBW_NEG_ROTATE,   ///< Negative rotate button.
};

/** Widget parts of the #RideBuildWindow GUI. */
static const WidgetPart _simple_ride_construction_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, RBW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(0, 1),
				Widget(WT_LEFT_TEXT, RBW_TYPE_NAME, COL_RANGE_DARK_RED), SetFill(1, 0),
						SetData(GUI_RIDE_BUILD_NAME_TEXT, STR_NULL), SetPadding(2, 2, 0, 2),
				Widget(WT_LEFT_TEXT, RBW_COST, COL_RANGE_DARK_RED), SetFill(1, 0),
						SetData(GUI_RIDE_BUILD_COST_TEXT, STR_NULL), SetPadding(2, 2, 0, 2),
				Widget(WT_PANEL, RBW_DISPLAY_RIDE, COL_RANGE_DARK_RED), SetPadding(0, 2, 2, 2),
						SetData(STR_NULL, GUI_RIDE_BUILD_DISPLAY_TOOLTIP), SetFill(1, 1), SetMinimalSize(150, 100),
				EndContainer(),
				Intermediate(1, 4),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_PUSHBUTTON, RBW_POS_ROTATE, COL_RANGE_DARK_RED), SetPadding(0, 1, 2, 2),
							SetData(SPR_GUI_ROT3D_POS, GUI_RIDE_BUILD_ROTATE_TOOLTIP),
					Widget(WT_IMAGE_PUSHBUTTON, RBW_NEG_ROTATE, COL_RANGE_DARK_RED), SetPadding(0, 2, 2, 1),
							SetData(SPR_GUI_ROT3D_NEG, GUI_RIDE_BUILD_ROTATE_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
			EndContainer(),
	EndContainer(),
};

/** Result codes in trying to place a shop in the world. */
enum RidePlacementResult {
	RPR_FAIL,    ///< Ride could not be placed in the world.
	RPR_SAMEPOS, ///< Ride got placed at the same spot as previously.
	RPR_CHANGED, ///< Ride got placed at a different spot in the world.
};

/**
 * Window for building simple 'plop down' rides. If the window is closed without building the ride, it is deleted.
 * @todo Add a 'Cancel build' button for people that fail to understand closing the window has that function.
 */
class RideBuildWindow : public GuiWindow {
public:
	RideBuildWindow(RideInstance *instance);
	~RideBuildWindow();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;

	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

	RideMouseMode selector; ///< Mouse mode displaying the new ride.
private:
	StringID str_titlebar;  ///< String to use for the titlebar of the window.
	RideInstance *instance; ///< Instance to build, set to \c nullptr after build to prevent deletion of the instance.
	TileEdge orientation;   ///< Orientation of the simple ride.
	BestErrorMessageReason build_forbidden_reason;  ///< Reason why we may not place the instance at the given location, if any.

	bool CanPlaceFixedRide(const FixedRideType *selected_ride, const XYZPoint16 &pos, uint8 ride_orient, ViewOrientation vp_orient);
	RidePlacementResult ComputeFixedRideVoxel(XYZPoint32 world_pos, ViewOrientation vp_orient);
};

/**
 * Constructor of the #RideBuildWindow, for 'plopping down' a ride.
 * @param ri Ride to 'plop down'.
 */
RideBuildWindow::RideBuildWindow(RideInstance *ri)
: GuiWindow(WC_RIDE_BUILD,
  ri->GetIndex()),
  instance(ri),
  orientation(EDGE_SE),
  build_forbidden_reason(BestErrorMessageReason::ACT_BUILD)
{
	switch (ri->GetKind()) {
		case RTK_SHOP:
			str_titlebar = GUI_RIDE_BUILD_TITLEBAR_SHOP;
			break;
		case RTK_GENTLE:
			str_titlebar = GUI_RIDE_BUILD_TITLEBAR_GENTLE;
			break;
		case RTK_THRILL:
			str_titlebar = GUI_RIDE_BUILD_TITLEBAR_THRILL;
			break;

		default:
			NOT_REACHED(); // Make sure other ride kinds are caught when adding.
	}

	this->SetupWidgetTree(_simple_ride_construction_gui_parts, lengthof(_simple_ride_construction_gui_parts));
	this->selector.cur_cursor = CUR_TYPE_INVALID;
	this->selector.SetSize(0, 0); // Disable the selector.
	this->SetSelector(&this->selector);
}

RideBuildWindow::~RideBuildWindow()
{
	this->SetSelector(nullptr);
	if (this->instance != nullptr) _rides_manager.DeleteInstance(this->instance->GetIndex());
}

void RideBuildWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case RBW_TITLEBAR:
			_str_params.SetStrID(1, str_titlebar);
			break;

		case RBW_TYPE_NAME:
			if (this->instance != nullptr) {
				const RideType *ride_type = this->instance->GetRideType();
				_str_params.SetStrID(1, ride_type->GetString(ride_type->GetTypeName()));
			} else {
				_str_params.SetText(1, "Unknown");
			}
			break;

		case RBW_COST:
			_str_params.SetMoney(1, 0);
			break;
	}
}

void RideBuildWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case RBW_DISPLAY_RIDE:
			if (this->instance == nullptr) return;

			const RideType *ride_type = this->instance->GetRideType();
			if (ride_type->kind != RTK_SHOP && ride_type->kind != RTK_GENTLE && ride_type->kind != RTK_THRILL) return;

			static const Recolouring recolour; // Never modified, display 'original' image in the GUI.
			Point32 pt(this->GetWidgetScreenX(wid) + wid->pos.width / 2, this->GetWidgetScreenY(wid) + wid->pos.height - 40);
			_video.BlitImage(pt, ride_type->GetView(this->orientation), recolour, GS_NORMAL);
			break;
	}
}

void RideBuildWindow::OnClick(WidgetNumber wid_num, [[maybe_unused]] const Point16 &pos)
{
	switch (wid_num) {
		case RBW_POS_ROTATE:
			this->orientation = static_cast<TileEdge>((this->orientation + 3) & 3);
			break;

		case RBW_NEG_ROTATE:
			this->orientation = static_cast<TileEdge>((this->orientation + 1) & 3);
			break;
	}
}

/**
 * Checks whether the air space above the ground at the given location is suited to place a fixed ride of the given height.
 * @param position Coordinate of the base voxel.
 * @param height Height of the object.
 * @return The space is suited to build the ride.
 */
static bool CheckSufficientVerticalSpace(const XYZPoint16& position, const int8 height)
{
	for (int8 h = 0; h < height; ++h) {
		const Voxel *v = _world.GetVoxel(position + XYZPoint16(0, 0, h));
		if (v == nullptr) continue;

		if (h > 0 && v->GetGroundType() != GTP_INVALID) return false;
		if (!v->CanPlaceInstance() || v->GetGroundSlope() != SL_FLAT) return false;
	}
	return true;
}

/**
 * Checks whether the given location is suited to place a fixed ride on flat ground.
 * @param position Coordinate of the base voxel.
 * @return The space is flat and suited to build the ride.
 */
static bool CanPlaceFixedRideOnFlatGround(const XYZPoint16& position)
{
	const Voxel *vx = _world.GetVoxel(position);
	return vx != nullptr && vx->GetGroundType() != GTP_INVALID && vx->GetGroundSlope() == SL_FLAT;
}

/**
 * Checks whether the given location is suited to place a fixed ride on a slope.
 * @param position Coordinate of the base voxel.
 * @return The space is suited to build the ride.
 */
static bool CanPlaceFixedRideOnSlope(const XYZPoint16& position)
{
	const Voxel *vx = _world.GetVoxel(position + XYZPoint16(0, 0, -1));
	if (vx == nullptr || vx->GetGroundType() == GTP_INVALID || vx->GetGroundSlope() == SL_FLAT) return false;
	const Voxel *top_voxel = _world.GetVoxel(position);
	return top_voxel == nullptr || !IsImplodedSteepSlope(top_voxel->GetGroundSlope());
}

/**
 * Can a fixed ride be placed at the given voxel?
 * @param selected_ride Ride to place.
 * @param pos Coordinate of the voxel.
 * @param ride_orient Orientation of the ride.
 * @param vp_orient Orientation of the viewport.
 * @pre voxel coordinate must be valid in the world.
 * @pre \a selected_ride may not be \c nullptr.
 * @return Ride can be placed at the given position.
 * @note Sets the value of #build_forbidden_reason if appropriate.
 */
bool RideBuildWindow::CanPlaceFixedRide(const FixedRideType *selected_ride, const XYZPoint16 &pos, uint8 ride_orient, ViewOrientation vp_orient)
{
	/* 1. Can the position itself be used to build a ride? */
	for (int x = 0; x < selected_ride->width_x; ++x) {
		for (int y = 0; y < selected_ride->width_y; ++y) {
			const XYZPoint16 location = OrientatedOffset(ride_orient, x, y) + pos;
			if (!IsVoxelstackInsideWorld(location.x, location.y)) {
				this->build_forbidden_reason.UpdateReason(GUI_ERROR_MESSAGE_BAD_LOCATION);
				return false;
			}
			if (_world.GetTileOwner(location.x, location.y) != OWN_PARK) {
				this->build_forbidden_reason.UpdateReason(GUI_ERROR_MESSAGE_UNOWNED_LAND);
				return false;
			}
		}
	}
	bool can_place_base = false;
	bool can_place_air = true;
	for (int x = 0; x < selected_ride->width_x; ++x) {
		for (int y = 0; y < selected_ride->width_y; ++y) {
			const XYZPoint16 location = pos + OrientatedOffset(ride_orient, x, y);
			can_place_base |= CanPlaceFixedRideOnFlatGround(location);
			can_place_air &= CheckSufficientVerticalSpace(location, selected_ride->GetHeight(x, y));
		}
	}
	if (!can_place_air) {
		this->build_forbidden_reason.UpdateReason(can_place_base ? GUI_ERROR_MESSAGE_OCCUPIED : GUI_ERROR_MESSAGE_BAD_LOCATION);
		return false;
	}
	if (can_place_base) return true;

	/* 2. Is the ride just above non-flat ground? */
	if (pos.z > 0) {
		for (int x = 0; x < selected_ride->width_x; ++x) {
			for (int y = 0; y < selected_ride->width_y; ++y) {
				const XYZPoint16 location = pos + OrientatedOffset(ride_orient, x, y);
				if (CanPlaceFixedRideOnSlope(location)) return true;
			}
		}
	}

	/* 3. For shops only: Is there a path at the right place? */
	if (selected_ride->kind != RTK_SHOP) {
		this->build_forbidden_reason.UpdateReason(GUI_ERROR_MESSAGE_BAD_LOCATION);
		return false;
	}
	const ShopType *selected_shop = static_cast<const ShopType*>(selected_ride);
	for (TileEdge entrance = EDGE_BEGIN; entrance < EDGE_COUNT; entrance++) { // Loop over the 4 unrotated directions.
		if ((selected_shop->flags & (1 << entrance)) == 0) continue; // No entrance here.
		TileEdge entr = static_cast<TileEdge>((entrance + vp_orient + this->orientation) & 3); // Perform rotation specified by the user in the GUI.
		if (PathExistsAtBottomEdge(pos, entr)) return true;
	}
	this->build_forbidden_reason.UpdateReason(GUI_ERROR_MESSAGE_BAD_LOCATION);
	return false;
}

/**
 * Decide at which voxel to place a fixed ride. It should be placed at a voxel intersecting with the view line through the given point in the world.
 * @param world_pos Coordinate of the point.
 * @param vp_orient Orientation of the viewport.
 * @return Result of the placement process.
 */
RidePlacementResult RideBuildWindow::ComputeFixedRideVoxel(XYZPoint32 world_pos, ViewOrientation vp_orient)
{
	this->build_forbidden_reason.Reset();
	this->build_forbidden_reason.UpdateReason(GUI_ERROR_MESSAGE_BAD_LOCATION);
	FixedRideInstance *si = static_cast<FixedRideInstance *>(this->instance);
	assert(si != nullptr);
	const FixedRideType *st = si->GetFixedRideType();
	assert(st != nullptr);

	/* Change of xworld and yworld for every (zworld / 2) change. */
	const int8 dx = _orientation_signum_dx[vp_orient];
	const int8 dy = _orientation_signum_dy[vp_orient];

	XYZPoint16 vox_pos;
	/* Move to the top voxel of the world. */
	vox_pos.z = WORLD_Z_SIZE - 1;
	int dz = vox_pos.z * 256 - world_pos.z;
	world_pos.x += dx * dz / 2;
	world_pos.y += dy * dz / 2;

	while (vox_pos.z >= 0) {
		vox_pos.x = world_pos.x / 256;
		vox_pos.y = world_pos.y / 256;
		if (IsVoxelstackInsideWorld(vox_pos.x, vox_pos.y) && this->CanPlaceFixedRide(st, vox_pos, this->orientation + vp_orient, vp_orient)) {
			/* Position of the ride the same as previously? */
			if (si->vox_pos != vox_pos || si->orientation != this->orientation) {
				si->SetRide((this->orientation + vp_orient) & 3, vox_pos);
				return RPR_CHANGED;
			}
			return RPR_SAMEPOS;
		} else {
			/* Since z gets smaller, we subtract dx and dy, thus the checks reverse. */
			if (vox_pos.x < 0 && dx > 0) break;
			if (vox_pos.x >= _world.GetXSize() && dx < 0) break;
			if (vox_pos.y < 0 && dy > 0) break;
			if (vox_pos.y >= _world.GetYSize() && dy < 0) break;
		}
		world_pos.x -= 128 * dx;
		world_pos.y -= 128 * dy;
		vox_pos.z--;
	}
	return RPR_FAIL;
}

void RideBuildWindow::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	Point32 wxy = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);

	/* Clean current display if needed. */
	switch (this->ComputeFixedRideVoxel(XYZPoint32(wxy.x, wxy.y, vp->view_pos.z), vp->orientation)) {
		case RPR_FAIL:
			this->selector.SetSize(0, 0);
			return;

		case RPR_SAMEPOS:
		case RPR_CHANGED: {
			/// \todo Let the ride do this.
			FixedRideInstance *si = static_cast<FixedRideInstance *>(this->instance);
			assert(si != nullptr);

			const FixedRideType* type = si->GetFixedRideType();
			{
				XYZPoint16 location = si->vox_pos;
				XYZPoint16 extent = OrientatedOffset(si->orientation, type->width_x, type->width_y);
				if (extent.x < 0) {
					location.x += extent.x + 1;
					extent.x *= -1;
				}
				if (extent.y < 0) {
					location.y += extent.y + 1;
					extent.y *= -1;
				}
				this->selector.SetPosition(location.x, location.y);
				this->selector.SetSize(extent.x, extent.y);
			}
			for (int8 x = 0; x < type->width_x; ++x) {
				for (int8 y = 0; y < type->width_y; ++y) {
					this->selector.AddVoxel(si->vox_pos + OrientatedOffset(si->orientation, x, y));
				}
			}
			this->selector.SetupRideInfoSpace();

			SmallRideInstance inst_number = static_cast<SmallRideInstance>(this->instance->GetIndex());
			for (int8 x = 0; x < type->width_x; ++x) {
				for (int8 y = 0; y < type->width_y; ++y) {
					const XYZPoint16 pos = si->vox_pos + OrientatedOffset(si->orientation, x, y);
					this->selector.SetRideData(pos, inst_number, si->GetEntranceDirections(pos));
				}
			}
			return;
		}

		default: NOT_REACHED();
	}
}

void RideBuildWindow::SelectorMouseButtonEvent(uint8 state)
{
	if (!IsLeftClick(state)) return;

	if (this->selector.area.width < 1 || this->selector.area.height < 1) {
		this->build_forbidden_reason.ShowErrorMessage();
		return;
	}
	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, Money(0))) return;  // \todo Check if we have enough money.

	FixedRideInstance *si = static_cast<FixedRideInstance *>(this->instance);
	const SmallRideInstance inst_number = static_cast<SmallRideInstance>(this->instance->GetIndex());
	const RideTypeKind kind = si->GetKind();

	_rides_manager.NewInstanceAdded(inst_number);
	AddRemovePathEdges(si->vox_pos, PATH_EMPTY, si->GetEntranceDirections(si->vox_pos), PAS_QUEUE_PATH);

	this->instance = nullptr; // Delete this window, and
	si = nullptr; // (Also clean the copy of the pointer.)
	delete this;

	/* Open GUI for the new ride or shop. */
	switch (kind) {
		case RTK_SHOP:
			ShowShopManagementGui(inst_number);
			break;
		case RTK_GENTLE:
		case RTK_THRILL:
			ShowGentleThrillRideManagementGui(inst_number);
			break;
		default: NOT_REACHED(); // \todo open GUI for other ride types
	}
}

/**
 * Open a builder for simple (plop down) rides.
 * @param ri Instance to place.
 */
void ShowRideBuildGui(RideInstance *ri)
{
	if (HighlightWindowByType(WC_RIDE_BUILD, ri->GetIndex()) != nullptr) return;

	new RideBuildWindow(ri);
}
