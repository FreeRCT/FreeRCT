/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_gui.cpp %Path building and editing. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "path_build.h"
#include "table/gui_sprites.h"

/**
 * Path build Gui.
 * @ingroup gui_group
 */
class PathBuildGui : public GuiWindow {
public:
	PathBuildGui();
	~PathBuildGui();

	virtual void OnClick(int16 number);
	virtual void OnChange(ChangeCode code, uint32 parameter);

	void SetButtons();
};

/**
 * Widget numbers of the path build Gui.
 * @ingroup gui_group
 */
enum PathBuildWidgets {
	PATH_GUI_SLOPE_DOWN,         ///< Button 'go down'.
	PATH_GUI_SLOPE_FLAT,         ///< Button 'flat'.
	PATH_GUI_SLOPE_UP,           ///< Button 'go up'.
	PATH_GUI_NE_DIRECTION,       ///< Build arrow in NE direction.
	PATH_GUI_SE_DIRECTION,       ///< Build arrow in SE direction.
	PATH_GUI_SW_DIRECTION,       ///< Build arrow in SW direction.
	PATH_GUI_NW_DIRECTION,       ///< Build arrow in NW direction.
	PATH_GUI_FORWARD,            ///< Move the arrow a path tile forward.
	PATH_GUI_BACKWARD,           ///< Move the arrow a path tile backward.
	PATH_GUI_LONG,               ///< Build a long path.
	PATH_GUI_BUY,                ///< Buy a path tile.
	PATH_GUI_REMOVE,             ///< Remove a path tile.
};

static const int SPR_NE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NE; ///< Sprite for building in NE direction.
static const int SPR_SE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SE; ///< Sprite for building in SE direction.
static const int SPR_SW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SW; ///< Sprite for building in SW direction.
static const int SPR_NW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NW; ///< Sprite for building in NW direction.

/**
 * Widget parts of the path build Gui.
 * @ingroup gui_group
 */
static const WidgetPart _path_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, 0), SetData(GUI_PATH_GUI_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, 0),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, 0),
			Intermediate(0, 1),
				Intermediate(1, 5), SetPadding(5, 5, 0, 5),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Slope down/level/up. */
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_DOWN, 0),
							SetData(SPR_GUI_SLOPES_START + TSL_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_FLAT, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_FLAT, GUI_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_UP, 0), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Intermediate(1, 3), SetPadding(5, 5, 0, 5),
					/* Four arrows direction. */
					Intermediate(2, 2), SetHorPIP(0, 2, 5), SetVertPIP(0, 2, 0),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NW_DIRECTION, 0),
								SetData(SPR_NW_DIRECTION, GUI_PATH_GUI_NW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NE_DIRECTION, 0),
								SetData(SPR_NE_DIRECTION, GUI_PATH_GUI_NE_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SW_DIRECTION, 0),
								SetData(SPR_SW_DIRECTION, GUI_PATH_GUI_SW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SE_DIRECTION, 0),
								SetData(SPR_SE_DIRECTION, GUI_PATH_GUI_SE_DIRECTION_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Forward/backward. */
					Intermediate(2, 1),
						Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_FORWARD, 0),
								SetData(GUI_PATH_GUI_FORWARD, GUI_PATH_GUI_FORWARD_TIP),
						Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_BACKWARD, 0),
								SetData(GUI_PATH_GUI_BACKWARD, GUI_PATH_GUI_BACKWARD_TIP),
				Intermediate(1, 7), SetPadding(5, 5, 5, 5), SetHorPIP(0, 2, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_LONG, 0), SetData(GUI_PATH_GUI_LONG, GUI_PATH_GUI_LONG_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_BUY, 0), SetData(GUI_PATH_GUI_BUY, GUI_PATH_GUI_BUY_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_REMOVE, 0), SetData(GUI_PATH_GUI_REMOVE, GUI_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
			EndContainer(),
	EndContainer(),
};

PathBuildGui::PathBuildGui() : GuiWindow(WC_PATH_BUILDER)
{
	this->SetupWidgetTree(_path_build_gui_parts, lengthof(_path_build_gui_parts));
	_path_builder.SetPathGuiState(true);
}

PathBuildGui::~PathBuildGui()
{
	_path_builder.SetPathGuiState(false);
}

/** Array with slope selection widget numbers. */
static const WidgetNumber _slope_widgets[] = {
	PATH_GUI_SLOPE_DOWN, PATH_GUI_SLOPE_FLAT, PATH_GUI_SLOPE_UP, INVALID_WIDGET_INDEX
};

/** Array with direction selection widget numbers. */
static const WidgetNumber _direction_widgets[] = {
	PATH_GUI_NE_DIRECTION, PATH_GUI_SE_DIRECTION, PATH_GUI_SW_DIRECTION, PATH_GUI_NW_DIRECTION, INVALID_WIDGET_INDEX
};

/* virtual */ void PathBuildGui::OnClick(WidgetNumber number)
{
	switch (number) {
		case PATH_GUI_SLOPE_DOWN:
		case PATH_GUI_SLOPE_FLAT:
		case PATH_GUI_SLOPE_UP:
			_path_builder.SelectSlope((TrackSlope)(number - PATH_GUI_SLOPE_DOWN));
			break;

		case PATH_GUI_NE_DIRECTION:
		case PATH_GUI_SE_DIRECTION:
		case PATH_GUI_SW_DIRECTION:
		case PATH_GUI_NW_DIRECTION: {
			Viewport *vp = GetViewport();
			if (vp != NULL) {
				TileEdge edge = (TileEdge)AddOrientations((ViewOrientation)(number - PATH_GUI_NE_DIRECTION), vp->orientation);
				_path_builder.SelectArrow(edge);
			}
			break;
		}

		case PATH_GUI_FORWARD:
		case PATH_GUI_BACKWARD:
			_path_builder.SelectMovement(number == PATH_GUI_FORWARD);
			break;

		case PATH_GUI_LONG:
			_path_builder.SelectLong();
			break;

		case PATH_GUI_REMOVE:
		case PATH_GUI_BUY:
			_path_builder.SelectBuyRemove(number == PATH_GUI_BUY);
			break;

		default:
			break;
	}
}

/** Set the buttons at the path builder gui. */
void PathBuildGui::SetButtons()
{
	Viewport *vp = GetViewport();
	if (vp == NULL) return;

	/* Update arrow buttons. */
	uint8 directions = _path_builder.GetAllowedArrows();
	TileEdge sel_dir = _path_builder.GetSelectedArrow();
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		TileEdge rot_edge = (TileEdge)SubtractOrientations((ViewOrientation)edge, vp->orientation);
		if (((0x11 << edge) & directions) != 0) {
			this->SetWidgetShaded(PATH_GUI_NE_DIRECTION + rot_edge, false);
			this->SetWidgetPressed(PATH_GUI_NE_DIRECTION + rot_edge, edge == sel_dir);
		} else {
			this->SetWidgetShaded(PATH_GUI_NE_DIRECTION + rot_edge, true);
		}
	}
	/* Update the slope buttons. */
	uint8 allowed_slopes = _path_builder.GetAllowedSlopes();
	TrackSlope sel_slope = _path_builder.GetSelectedSlope();
	for (TrackSlope ts = TSL_BEGIN; ts < TSL_COUNT_GENTLE; ts++) {
		if (((1 << ts) & allowed_slopes) != 0) {
			this->SetWidgetShaded(PATH_GUI_SLOPE_DOWN + ts, false);
			this->SetWidgetPressed(PATH_GUI_SLOPE_DOWN + ts, ts == sel_slope);
		} else {
			this->SetWidgetShaded(PATH_GUI_SLOPE_DOWN + ts, true);
		}
	}
	this->SetWidgetShaded(PATH_GUI_BUY,      !_path_builder.GetBuyIsEnabled());
	this->SetWidgetShaded(PATH_GUI_REMOVE,   !_path_builder.GetRemoveIsEnabled());
	this->SetWidgetShaded(PATH_GUI_FORWARD,  !_path_builder.GetForwardIsEnabled());
	this->SetWidgetShaded(PATH_GUI_BACKWARD, !_path_builder.GetBackwardIsEnabled());
	this->SetWidgetShaded(PATH_GUI_LONG,     !_path_builder.GetLongButtonIsEnabled());
	this->SetWidgetPressed(PATH_GUI_LONG,     _path_builder.GetLongButtonIsPressed());
}

/* virtual */ void PathBuildGui::OnChange(ChangeCode code, uint32 parameter)
{
	switch (code) {
		case CHG_UPDATE_BUTTONS:
		case CHG_VIEWPORT_ROTATED:
			this->SetButtons();
			break;

		default:
			break;
	}
}

/**
 * Open the path build gui.
 * @ingroup gui_group
 */
void ShowPathBuildGui()
{
	if (HighlightWindowByType(WC_PATH_BUILDER)) return;
	new PathBuildGui;
}

