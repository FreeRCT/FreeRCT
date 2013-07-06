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
	~CoasterInstanceWindow();

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
	if (!this->ci->IsAccessible()) _rides_manager.DeleteInstance(this->ci->GetIndex());
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
	if (HighlightWindowByType(WC_COASTER_MANAGER, coaster->GetIndex())) return;

	CoasterInstance *ci = static_cast<CoasterInstance *>(coaster);
	assert(ci != NULL);
	new CoasterInstanceWindow(ci);
}
