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
#include "widget.h"
#include "language.h"
#include "table/strings.h"
#include "table/gui_sprites.h"

/** Path build Gui. */
class PathBuildGui : public GuiWindow {
public:
	PathBuildGui();

	virtual void OnClick(int16 number);
};

/** Widget numbers of the path build Gui. */
enum PathBuildWidgets {
	PATH_GUI_FOLLOW_GROUND,      ///< Radio button 'follow ground'.
	PATH_GUI_FOLLOW_GROUND_TEXT, ///< Text label with 'follow ground'.
	PATH_GUI_FREE_BUILD,         ///< Radio button 'build freely'.
	PATH_GUI_FREE_BUILD_TEXT,    ///< Text label with 'build freely'.
	PATH_GUI_SLOPE_DOWN,         ///< Button 'go down'.
	PATH_GUI_SLOPE_FLAT,         ///< Button 'flat'.
	PATH_GUI_SLOPE_UP,           ///< Button 'go up'.
	PATH_GUI_PREVIEW,            ///< Preview of the path canvas.
	PATH_GUI_COST,               ///< Cost canvas.
	PATH_GUI_NW_DIRECTION,       ///< Build arrow in NW direction.
	PATH_GUI_SW_DIRECTION,       ///< Build arrow in SW direction.
	PATH_GUI_NE_DIRECTION,       ///< Build arrow in NE direction.
	PATH_GUI_SE_DIRECTION,       ///< Build arrow in SE direction.
	PATH_GUI_BULLDOZE,           ///< Bulldoze sprite button.
	PATH_GUI_PATHS,              ///< Available paths canvas.
	PATH_GUI_SCROLL,             ///< Scrollbar of #PATH_GUI_PATHS.
};

static const int SPR_NE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NE; ///< Sprite for building in NE direction.
static const int SPR_SE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SE; ///< Sprite for building in SE direction.
static const int SPR_SW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SW; ///< Sprite for building in SW direction.
static const int SPR_NW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NW; ///< Sprite for building in NW direction.

/** Widget parts of the path build Gui. */
static const WidgetPart _path_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, 0), SetData(STR_PATH_GUI_TITLE, STR_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, 0),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, 0),
			Intermediate(0, 1),
				/* 'Follow ground' radio button, and slope up/plane/down buttons. */
				Intermediate(2, 2), // 2 rows, 2 columns
					/* Follow the ground while building. */
					Widget(WT_RADIOBUTTON, PATH_GUI_FOLLOW_GROUND, 0),
					Widget(WT_CENTERED_TEXT, PATH_GUI_FOLLOW_GROUND_TEXT, 0), SetData(STR_PATH_GUI_FOLLOW_GROUND, STR_NULL),
					/* Build in the air. */
					Widget(WT_RADIOBUTTON, PATH_GUI_FREE_BUILD, 0),
					Widget(WT_CENTERED_TEXT, PATH_GUI_FREE_BUILD_TEXT, 0), SetData(STR_PATH_GUI_FREE_BUILD, STR_NULL),
				Intermediate(1, 3), SetPadding(5, 0, 0, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Direction. */
					Intermediate(2, 2), SetHorPIP(5, 2, 5), SetVertPIP(5, 2, 5),
						Widget(WT_IMAGEBUTTON, PATH_GUI_NW_DIRECTION, 0), SetData(SPR_NW_DIRECTION, STR_PATH_GUI_NW_DIRECTION),
						Widget(WT_IMAGEBUTTON, PATH_GUI_NE_DIRECTION, 0), SetData(SPR_NE_DIRECTION, STR_PATH_GUI_NE_DIRECTION),
						Widget(WT_IMAGEBUTTON, PATH_GUI_SW_DIRECTION, 0), SetData(SPR_SW_DIRECTION, STR_PATH_GUI_SW_DIRECTION),
						Widget(WT_IMAGEBUTTON, PATH_GUI_SE_DIRECTION, 0), SetData(SPR_SE_DIRECTION, STR_PATH_GUI_SE_DIRECTION),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
			EndContainer(),
	EndContainer(),
};

PathBuildGui::PathBuildGui() : GuiWindow(WC_PATH_BUILDER)
{
	this->SetupWidgetTree(_path_build_gui_parts, lengthof(_path_build_gui_parts));
	this->SetPosition(200, 20);
	this->MarkDirty();
}

/* virtual */ void PathBuildGui::OnClick(int16 number)
{
}


void ShowPathBuildGui()
{
	new PathBuildGui();
}

