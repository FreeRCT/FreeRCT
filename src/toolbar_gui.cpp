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

/** Main toolbar. */
class ToolbarWindow : public Window {
public:
	ToolbarWindow();
	virtual ~ToolbarWindow();

	virtual void OnDraw(VideoSystem *vid);
};



ToolbarWindow::ToolbarWindow() : Window(WC_TOOLBAR)
{
	this->SetSize(200, 50);
	this->SetPosition(0, 0);
}

ToolbarWindow::~ToolbarWindow()
{
}

void ToolbarWindow::OnDraw(VideoSystem *vid)
{
	DrawPanel(vid, this->rect);
}

/** Open the main toolbar window. */
void ShowToolbar()
{
	new ToolbarWindow();
}
