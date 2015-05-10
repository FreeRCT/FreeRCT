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
#include "mouse_mode.h"
#include "language.h"

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

static const WidgetPart _simple_ride_construction_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, RBW_TITLEBAR, COL_RANGE_DARK_RED), SetData(GUI_RIDE_BUILD_TITLEBAR, GUI_TITLEBAR_TIP),
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

private:
	RideInstance *instance; ///< Instance to build, set to \c nullptr after build to prevent deletion of the instance.
	TileEdge orientation;   ///< Orientation of the simple ride.
};

RideBuildWindow::RideBuildWindow(RideInstance *ri) : GuiWindow(WC_RIDE_BUILD, ri->GetIndex()), instance(ri), orientation(EDGE_SE)
{
	this->SetupWidgetTree(_simple_ride_construction_gui_parts, lengthof(_simple_ride_construction_gui_parts));
}

RideBuildWindow::~RideBuildWindow()
{
	if (this->instance != nullptr) _rides_manager.DeleteInstance(this->instance->GetIndex());
}

void RideBuildWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case RBW_TITLEBAR:
			_str_params.SetUint8(1, (uint8 *)this->instance->name);
			break;

		case RBW_TYPE_NAME:
			if (this->instance != nullptr) {
				const RideType *ride_type = this->instance->GetRideType();
				_str_params.SetStrID(1, ride_type->GetString(ride_type->GetTypeName()));
			} else {
				_str_params.SetUint8(1, (uint8 *)"Unknown");
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
			if (ride_type->kind != RTK_SHOP) return;

			static const Recolouring recolour; // Never modified, display 'original' image in the GUI.
			Point32 pt(this->GetWidgetScreenX(wid) + wid->pos.width / 2, this->GetWidgetScreenY(wid) + wid->pos.height - 40);
			_video.BlitImage(pt, ride_type->GetView(this->orientation), recolour, GS_NORMAL);
			break;
	}
}

void RideBuildWindow::OnClick(WidgetNumber wid_num, const Point16 &pos)
{
	switch (wid_num) {
		case RBW_POS_ROTATE:
			this->orientation = static_cast<TileEdge>((this->orientation + 3) & 3);
			this->MarkWidgetDirty(RBW_DISPLAY_RIDE);
			break;

		case RBW_NEG_ROTATE:
			this->orientation = static_cast<TileEdge>((this->orientation + 1) & 3);
			this->MarkWidgetDirty(RBW_DISPLAY_RIDE);
			break;
	}
}


