/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file entity_gui.h Entity windows. */

#ifndef ENTITY_GUI_H
#define ENTITY_GUI_H

#include "stdafx.h"
#include "window.h"

/**
 * Widget numbers of the entity remove window.
 * @ingroup gui_group
 */
enum EntityRemoveWidgets {
	ERW_MESSAGE, ///< Displayed message.
	ERW_YES,     ///< 'yes' button.
	ERW_NO,      ///< 'no' button.
};

/** %Window to ask confirmation for deleting an entity. */
class EntityRemoveWindow : public GuiWindow {
public:
	EntityRemoveWindow(WindowTypes wtype, WindowNumber wnum);

	Point32 OnInitialPosition() override;
};

#endif
