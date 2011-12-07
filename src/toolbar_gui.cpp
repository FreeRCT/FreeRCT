/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file toolbar_gui.cpp Main toolbar window code. */

#include "stdafx.h"
#include "window.h"
#include "widget.h"
#include "video.h"

#include "table/strings.h"

/** Main toolbar. */
class ToolbarWindow : public GuiWindow {
public:
	ToolbarWindow();
};

/** Widget numbers of the toolbar Gui. */
enum ToolbarGuiWidgets {
	TB_GUI_QUIT,  ///< Quit program button.
	TB_GUI_PATHS, ///< Build paths button.
};

/** Widget parts of the toolbar Gui. */
static const WidgetPart _toolbar_widgets[] = {
	Intermediate(1, 0),
		Widget(WT_TEXTBUTTON, TB_GUI_QUIT, 0), SetData(STR_TOOLBAR_GUI_QUIT, STR_TOOLBAR_GUI_QUIT_PROGRAM),
		Widget(WT_TEXTBUTTON, TB_GUI_PATHS, 0), SetData(STR_TOOLBAR_GUI_PATHS, STR_TOOLBAR_GUI_BUILD_PATHS),
	EndContainer(),
};


ToolbarWindow::ToolbarWindow() : GuiWindow(WC_TOOLBAR)
{
	this->SetupWidgetTree(_toolbar_widgets, lengthof(_toolbar_widgets));
	this->SetPosition(10, 0);
	this->MarkDirty();
}


/** Open the main toolbar window. */
void ShowToolbar()
{
	new ToolbarWindow();
}
