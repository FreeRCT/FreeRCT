/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster_gui.cpp Roller coaster windows. */

#include "stdafx.h"
#include "window.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "coaster.h"

#include "table/gui_sprites.h"

/** Widget numbers of the roller coaster instance window. */
enum CoasterInstanceWidgets {
	CIW_TITLEBAR, ///< Titlebar widget.
};

/** Widget parts of the #CoasterInstanceWindow. */
static const WidgetPart _coaster_instance_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, CIW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetMinimalSize(100, 100),
	EndContainer(),
};


/** Window to display and setup a roller coaster. */
class CoasterInstanceWindow : public GuiWindow {
public:
	CoasterInstanceWindow(CoasterInstance *ci);
	/* virtual */ ~CoasterInstanceWindow();

	/* virtual */ void SetWidgetStringParameters(WidgetNumber wid_num) const;
private:
	CoasterInstance *ci; ///< Roller coaster instance to display and control.
};

/**
 * Consteructor of the roller coaster instance window.
 * @param ci Roller coaster instance to display and control.
 */
CoasterInstanceWindow::CoasterInstanceWindow(CoasterInstance *ci) : GuiWindow(WC_COASTER_MANAGER, ci->GetIndex())
{
	this->ci = ci;
	this->SetupWidgetTree(_coaster_instance_gui_parts, lengthof(_coaster_instance_gui_parts));
}

CoasterInstanceWindow::~CoasterInstanceWindow()
{
	if (!GetWindowByType(WC_COASTER_BUILD, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

/* virtual */ void CoasterInstanceWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CIW_TITLEBAR:
			_str_params.SetUint8(1, (uint8 *)this->ci->name);
			break;
	}
}

/**
 * Open a roller coaster management window for the given roller coaster ride.
 * @param coaster Coaster instance to display.
 */
void ShowCoasterManagementGui(RideInstance *coaster)
{
	if (coaster->GetKind() != RTK_COASTER) return;
	CoasterInstance *ci = static_cast<CoasterInstance *>(coaster);
	assert(ci != NULL);

	RideInstanceState ris = ci->DecideRideState();
	if (ris == RIS_TESTING || ris == RIS_CLOSED || ris == RIS_OPEN) {
		if (HighlightWindowByType(WC_COASTER_MANAGER, coaster->GetIndex())) return;

		new CoasterInstanceWindow(ci);
		return;
	}
	ShowCoasterBuildGui(ci);
}

/** Widgets of the coaster construction window. */
enum CoasterConstructionWidgets {
	CCW_TITLEBAR,            ///< Titlebar widget.
	CCW_BEND_WIDE_LEFT,      ///< Button for selecting wide left turn.
	CCW_BEND_NORMAL_LEFT,    ///< Button for selecting normal left turn.
	CCW_BEND_TIGHT_LEFT,     ///< Button for selecting tight left turn.
	CCW_BEND_NONE,           ///< Button for selecting straight ahead (no turn).
	CCW_BEND_TIGHT_RIGHT,    ///< Button for selecting tight right turn.
	CCW_BEND_NORMAL_RIGHT,   ///< Button for selecting normal right turn.
	CCW_BEND_WIDE_RIGHT,     ///< Button for selecting wide right turn.
	CCW_BANK_LEFT,           ///< Button for selecting banking to the left.
	CCW_BANK_NONE,           ///< Button for selecting no banking.
	CCW_BANK_RIGHT,          ///< Button for selecting banking to the right.
	CCW_SLOPE_VERTICAL_DOWN, ///< Button for selecting vertically down slope.
	CCW_SLOPE_STEEP_DOWN,    ///< Button for selecting steep down slope.
	CCW_SLOPE_DOWN,          ///< Button for selecting gentle down slope.
	CCW_SLOPE_FLAT,          ///< Button for selecting level slope.
	CCW_SLOPE_UP,            ///< Button for selecting gentle up slope.
	CCW_SLOPE_STEEP_UP,      ///< Button for selecting steep up slope.
	CCW_SLOPE_VERTICAL_UP,   ///< Button for selecting vertically up slope.
	CCW_DISPLAY_PIECE,       ///< Display space for a track piece.
	CCW_REMOVE,              ///< Remove track piece button.
	CCW_BACKWARD,            ///< Move backward.
	CCW_FORWARD,             ///< Move forward.
	CCW_ROT_NEG,             ///< Rotate in negative direction.
	CCW_ROT_POS,             ///< Rotate in positive direction.
};

/** Widget parts of the #CoasterBuildWindow. */
static const WidgetPart _coaster_construction_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, CCW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(5, 1),
				Intermediate(1, 9), // Bend type.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_WIDE_LEFT,    COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_WIDE, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NORMAL_LEFT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_NORMAL, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_TIGHT_LEFT,   COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_TIGHT, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NONE,         COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_STRAIGHT, GUI_COASTER_BUILD_NO_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_TIGHT_RIGHT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_TIGHT, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NORMAL_RIGHT, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_NORMAL, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_WIDE_RIGHT,   COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_WIDE, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Intermediate(1, 5), // Banking.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_LEFT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BANK_START + TBG_BANK_LEFT, GUI_COASTER_BUILD_BANK_LEFT_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_NONE,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BANK_START + TBG_BANK_NONE, GUI_COASTER_BUILD_BANK_NONE_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_RIGHT, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BANK_START + TBG_BANK_RIGHT, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Intermediate(1, 9), // Slopes.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_VERTICAL_DOWN, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STRAIGHT_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_STEEP_DOWN, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STEEP_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_DOWN, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_FLAT, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_FLAT, GUI_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_UP, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_STEEP_UP, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STEEP_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_VERTICAL_UP, 0), SetPadding(0, 5, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STRAIGHT_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetPadding(5, 2, 5, 2),
					Widget(WT_TEXT_PUSHBUTTON, CCW_DISPLAY_PIECE, COL_RANGE_DARK_RED),
							SetData(STR_NULL, GUI_COASTER_BUILD_BUY_TOOLTIP), SetFill(1, 1), SetMinimalSize(200, 200),
				Intermediate(1, 5), // delete, prev/next, rotate
					Widget(WT_TEXT_PUSHBUTTON, CCW_REMOVE, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_REMOVE, GUI_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_TEXT_PUSHBUTTON, CCW_BACKWARD, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_BACKWARD, GUI_PATH_GUI_BACKWARD_TIP),
					Widget(WT_TEXT_PUSHBUTTON, CCW_FORWARD, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_FORWARD, GUI_PATH_GUI_FORWARD_TIP),
					Widget(WT_IMAGE_PUSHBUTTON, CCW_ROT_NEG, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_ROT3D_NEG, GUI_RIDE_SELECT_ROT_NEG_TOOLTIP),
					Widget(WT_IMAGE_PUSHBUTTON, CCW_ROT_POS, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_ROT3D_POS, GUI_RIDE_SELECT_ROT_POS_TOOLTIP),
	EndContainer(),
};

/** Window to build or edit a roller coaster. */
class CoasterBuildWindow : public GuiWindow {
public:
	CoasterBuildWindow(CoasterInstance *ci);
	/* virtual */ ~CoasterBuildWindow();

	/* virtual */ void SetWidgetStringParameters(WidgetNumber wid_num) const;
private:
	CoasterInstance *ci; ///< Roller coaster instance to build or edit.
};

/**
 * Constructor of the roller coaster build window. The provided instance may be completely empty.
 * @param ci Coaster instance to build or modify.
 */
CoasterBuildWindow::CoasterBuildWindow(CoasterInstance *ci) : GuiWindow(WC_COASTER_BUILD, ci->GetIndex())
{
	this->ci = ci;
	this->SetupWidgetTree(_coaster_construction_gui_parts, lengthof(_coaster_construction_gui_parts));
}

CoasterBuildWindow::~CoasterBuildWindow()
{
	if (!GetWindowByType(WC_COASTER_MANAGER, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

/* virtual */ void CoasterBuildWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CCW_TITLEBAR:
			_str_params.SetUint8(1, (uint8 *)this->ci->name);
			break;
	}
}

/**
 * Open a roller coaster build/edit window for the given roller coaster.
 * @param coaster Coaster instance to modify.
 */
void ShowCoasterBuildGui(CoasterInstance *coaster)
{
	if (coaster->GetKind() != RTK_COASTER) return;
	if (HighlightWindowByType(WC_COASTER_BUILD, coaster->GetIndex())) return;

	new CoasterBuildWindow(coaster);
}
