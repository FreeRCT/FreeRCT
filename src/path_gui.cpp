/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_gui.cpp Path building and editing. */

#include "stdafx.h"
#include "window.h"
#include "language.h"
#include "table/strings.h"
#include "table/gui_sprites.h"

/**
 * Path build Gui.
 * @ingroup gui_group
 */
class PathBuildGui : public GuiWindow {
public:
	PathBuildGui();

	virtual void OnClick(int16 number);
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
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, 0), SetData(STR_PATH_GUI_TITLE, STR_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, 0),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, 0),
			Intermediate(0, 1),
				Intermediate(1, 5), SetPadding(5, 5, 0, 5),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Slope down/level/up. */
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_DOWN, 0),
							SetData(STR_PATH_GUI_SLOPE_DOWN, STR_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_FLAT, 0), SetPadding(0, 0, 0, 5),
							SetData(STR_PATH_GUI_SLOPE_FLAT, STR_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_UP, 0), SetPadding(0, 0, 0, 5),
							SetData(STR_PATH_GUI_SLOPE_UP, STR_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Intermediate(1, 3), SetPadding(5, 5, 0, 5),
					/* Four arrows direction. */
					Intermediate(2, 2), SetHorPIP(0, 2, 5), SetVertPIP(0, 2, 0),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NW_DIRECTION, 0),
								SetData(SPR_NW_DIRECTION, STR_PATH_GUI_NW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NE_DIRECTION, 0),
								SetData(SPR_NE_DIRECTION, STR_PATH_GUI_NE_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SW_DIRECTION, 0),
								SetData(SPR_SW_DIRECTION, STR_PATH_GUI_SW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SE_DIRECTION, 0),
								SetData(SPR_SE_DIRECTION, STR_PATH_GUI_SE_DIRECTION_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Forward/backward. */
					Intermediate(2, 1),
						Widget(WT_TEXT_BUTTON, PATH_GUI_FORWARD, 0),
								SetData(STR_PATH_GUI_FORWARD, STR_PATH_GUI_FORWARD_TIP),
						Widget(WT_TEXT_BUTTON, PATH_GUI_BACKWARD, 0),
								SetData(STR_PATH_GUI_BACKWARD, STR_PATH_GUI_BACKWARD_TIP),
				Intermediate(1, 7), SetPadding(5, 5, 5, 5), SetHorPIP(0, 2, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_LONG, 0), SetData(STR_PATH_GUI_LONG, STR_PATH_GUI_LONG_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_BUY, 0), SetData(STR_PATH_GUI_BUY, STR_PATH_GUI_BUY_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_REMOVE, 0), SetData(STR_PATH_GUI_REMOVE, STR_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
			EndContainer(),
	EndContainer(),
};

PathBuildGui::PathBuildGui() : GuiWindow(WC_PATH_BUILDER)
{
	this->SetupWidgetTree(_path_build_gui_parts, lengthof(_path_build_gui_parts));
}

/* virtual */ void PathBuildGui::OnClick(WidgetNumber number)
{
}

/**
 * Open the path build gui.
 * @ingroup gui_group
 */
void ShowPathBuildGui()
{
	if (HighlightWindowByType(WC_PATH_BUILDER)) return;
	new PathBuildGui();
}

