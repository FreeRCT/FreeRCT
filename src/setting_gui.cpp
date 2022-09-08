/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file setting_gui.cpp Main setting window code. */

#include "stdafx.h"
#include "window.h"
#include "map.h"
#include "gui_sprites.h"

/**
 * Setting window.
 * @ingroup gui_group
 */
class SettingWindow : public GuiWindow {
public:
	SettingWindow();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
};

/**
 * Widget numbers of the setting window.
 * @ingroup gui_group
 */
enum SettingGuiWidgets {
	SW_TITLEBAR,   ///< Titlebar widget.
	SW_LANGUAGE,   ///< Change language dropdown widget.
	SW_RESOLUTION, ///< Change resolution widget.
};

/**
 * Widget parts of the setting window.
 * @ingroup gui_group
 */
static const WidgetPart _setting_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, SW_TITLEBAR, COL_RANGE_BLUE), SetData(GUI_SETTING_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			Intermediate(0, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
						SetData(GUI_SETTING_LANGUAGE, GUI_SETTING_LANGUAGE_TOOLTIP), SetPadding(3, 3, 3, 3),
				Widget(WT_DROPDOWN_BUTTON, SW_LANGUAGE, COL_RANGE_BLUE),
						SetData(STR_ARG1, STR_NULL), SetMinimalSize(100, 10), SetPadding(3, 3, 3, 3),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
						SetData(GUI_SETTING_RESOLUTION, GUI_SETTING_RESOLUTION_TOOLTIP), SetPadding(3, 3, 3, 3),
				Widget(WT_DROPDOWN_BUTTON, SW_RESOLUTION, COL_RANGE_BLUE),
						SetData(GUI_RESOLUTION, STR_NULL), SetMinimalSize(100, 10), SetPadding(3, 3, 3, 3),
			EndContainer(),
	EndContainer(),
};


SettingWindow::SettingWindow() : GuiWindow(WC_SETTING, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_setting_widgets, lengthof(_setting_widgets));
}

void SettingWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case SW_LANGUAGE:
			_str_params.SetStrID(1, GUI_LANGUAGE_NAME); /// \todo Language name should be part of the language's properties, rather than a string.
			break;

		case SW_RESOLUTION:
			_str_params.SetNumber(1, _video.Width());
			_str_params.SetNumber(2, _video.Height());
			break;
	}
}

void SettingWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	switch (number) {
		case SW_LANGUAGE: {
			DropdownList itemlist;
			for (int i = 0; i < LANGUAGE_COUNT; i++) {
				_str_params.SetText(1, _language.GetLanguageName(i));
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(number, itemlist, _current_language);
			break;
		}

		case SW_RESOLUTION: {
			/* Get current resolution index. */
			int index = 0;
			auto iter = _video.Resolutions().find(Point32(_video.Width(), _video.Height()));
			if (iter != _video.Resolutions().end()) {
				index = std::distance(_video.Resolutions().begin(), iter);
			}

			DropdownList itemlist;
			for (const auto &res : _video.Resolutions()) {
				_str_params.SetNumber(1, res.x);
				_str_params.SetNumber(2, res.y);
				itemlist.push_back(DropdownItem(GUI_RESOLUTION));
			}
			this->ShowDropdownMenu(number, itemlist, index);
		}
	}
}

void SettingWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code != CHG_DROPDOWN_RESULT) return;

	switch ((parameter >> 16) & 0xFF) {
		case SW_LANGUAGE:
			_current_language = parameter & 0xFF;
			_window_manager.ResetAllWindows();
			break;

		case SW_RESOLUTION: {
			int index = 0;
			for (const auto &it : _video.Resolutions()) {
				if (index == (int)(parameter & 0xFF)) {
					_video.SetResolution(it);
					break;
				}
				index++;
			}
			break;
		}

		default: NOT_REACHED();
	}
}

/**
 * Open the settings window.
 * @ingroup gui_group
 */
void ShowSettingGui()
{
	if (HighlightWindowByType(WC_SETTING, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new SettingWindow();
}
