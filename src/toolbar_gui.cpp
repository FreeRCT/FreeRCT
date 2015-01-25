/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file toolbar_gui.cpp Main toolbar window code. */

#include "stdafx.h"
#include "freerct.h"
#include "window.h"
#include "video.h"
#include "language.h"
#include "finances.h"
#include "dates.h"
#include "math_func.h"
#include "sprite_store.h"
#include "gui_sprites.h"
#include "viewport.h"
#include "gamemode.h"
#include "weather.h"

void ShowQuitProgram();

/**
 * Top toolbar.
 * @ingroup gui_group
 */
class ToolbarWindow : public GuiWindow {
public:
	ToolbarWindow();

	Point32 OnInitialPosition() override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);
};

/**
 * Widget numbers of the toolbar GUI.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	TB_GUI_QUIT,        ///< Quit program button.
	TB_GUI_SETTINGS,    ///< Settings button.
	TB_GUI_GAME_MODE,   ///< Switch game mode button.
	TB_GUI_PATHS,       ///< Build paths button.
	TB_GUI_SAVE,        ///< Save game button.
	TB_GUI_LOAD,        ///< Load game button.
	TB_GUI_RIDE_SELECT, ///< Select ride button.
	TB_GUI_FENCE,       ///< Select fence button.
	TB_GUI_TERRAFORM,   ///< Terraform button.
	TB_GUI_FINANCES,    ///< Finances button.
};

/**
 * Widget parts of the toolbar GUI.
 * @ingroup gui_group
 */
static const WidgetPart _toolbar_widgets[] = {
	Intermediate(1, 0),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_QUIT,        COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_QUIT,        GUI_TOOLBAR_GUI_TOOLTIP_QUIT_PROGRAM),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_SETTINGS,    COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_SETTINGS,    GUI_TOOLBAR_GUI_TOOLTIP_SETTINGS),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_GAME_MODE,   COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1,                    GUI_TOOLBAR_GUI_TOOLTIP_GAME_MODE),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_PATHS,       COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_PATHS,       GUI_TOOLBAR_GUI_TOOLTIP_BUILD_PATHS),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_SAVE,        COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_SAVE,        GUI_TOOLBAR_GUI_TOOLTIP_SAVE_GAME),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_LOAD,        COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_LOAD,        GUI_TOOLBAR_GUI_TOOLTIP_LOAD_GAME),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_RIDE_SELECT, COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_RIDE_SELECT, GUI_TOOLBAR_GUI_TOOLTIP_RIDE_SELECT),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_FENCE,       COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_FENCE,       GUI_TOOLBAR_GUI_TOOLTIP_FENCE),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_TERRAFORM,   COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_TERRAFORM,   GUI_TOOLBAR_GUI_TOOLTIP_TERRAFORM),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_FINANCES,    COL_RANGE_ORANGE_BROWN), SetData(GUI_TOOLBAR_GUI_FINANCES,    GUI_TOOLBAR_GUI_TOOLTIP_FINANCES),
	EndContainer(),
};

ToolbarWindow::ToolbarWindow() : GuiWindow(WC_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_toolbar_widgets, lengthof(_toolbar_widgets));
}

Point32 ToolbarWindow::OnInitialPosition()
{
	static const Point32 pt(10, 0);
	return pt;
}

/**
 * Determines the string ID of the string to display
 * for the switch game mode button.
 * @return String id of the string to display.
 */
StringID GetSwitchGameModeString()
{
	switch (_game_mode_mgr.GetGameMode()) {
		case GM_PLAY:
			return GUI_TOOLBAR_GUI_GAME_MODE_EDITOR;

		case GM_EDITOR:
			return GUI_TOOLBAR_GUI_GAME_MODE_PLAY;

		case GM_NONE:
			/* The toolbar is not visible in none-mode. */
			return STR_NULL;

		default:
			NOT_REACHED();
	}
}

void ToolbarWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case TB_GUI_QUIT:
			ShowQuitProgram();
			break;

		case TB_GUI_SETTINGS:
			ShowSettingGui();
			break;

		case TB_GUI_GAME_MODE:
			_game_mode_mgr.SetGameMode(_game_mode_mgr.InEditorMode() ? GM_PLAY : GM_EDITOR);
			break;

		case TB_GUI_PATHS:
			ShowPathBuildGui();
			break;

		case TB_GUI_SAVE: {
			SaveGame("saved.fct");
			/// \todo Provide option to enter the filename for saving.
			/// \todo Provide feedback on the save.
			break;
		}

		case TB_GUI_LOAD: {
			LoadGame("saved.fct");
			/// \todo Provide option to select the file to load.
			break;
		}

		case TB_GUI_RIDE_SELECT:
			ShowRideSelectGui();
			break;

		case TB_GUI_FENCE:
			ShowFenceGui();
			break;

		case TB_GUI_TERRAFORM:
			ShowTerraformGui();
			break;

		case TB_GUI_FINANCES:
			ShowFinancesGui();
			break;
	}
}

void ToolbarWindow::OnChange(ChangeCode code, uint32 parameter)
{
	switch (code) {
		case CHG_UPDATE_BUTTONS:
			/* Esure the right string parameters are used. */
			this->MarkWidgetDirty(TB_GUI_GAME_MODE);
			break;

		default:
			break;
	}
}

void ToolbarWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case TB_GUI_GAME_MODE: {
			/* Use max width of the two strings to display on this button. */
			int width, height;

			_str_params.SetStrID(1, GUI_TOOLBAR_GUI_GAME_MODE_PLAY);
			GetTextSize(STR_ARG1, &width, &height);
			wid->min_x = std::max(wid->min_x, (uint16)width);
			wid->min_y = std::max(wid->min_y, (uint16)height);

			_str_params.SetStrID(1, GUI_TOOLBAR_GUI_GAME_MODE_EDITOR);
			GetTextSize(STR_ARG1, &width, &height);
			wid->min_x = std::max(wid->min_x, (uint16)width);
			wid->min_y = std::max(wid->min_y, (uint16)height);
			break;
		}

		default:
			break;
	}
}

void ToolbarWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case TB_GUI_GAME_MODE:
			_str_params.SetStrID(1, GetSwitchGameModeString());
			break;

		default:
			break;
	}
}

/**
 * Open the main toolbar window.
 * @ingroup gui_group
 */
void ShowToolbar()
{
	new ToolbarWindow();
}

/**
 * Bottom toolbar.
 * @ingroup gui_group
 */
class BottomToolbarWindow : public GuiWindow {
public:
	BottomToolbarWindow();

	Point32 OnInitialPosition() override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
};

/**
 * Widget numbers of the bottom toolbar GUI.
 * @ingroup gui_group
 */
enum BottomToolbarGuiWidgets {
	BTB_STATUS,         ///< Status panel containing cash and rating readout.
	BTB_WEATHER,        ///< Weather sprite.
	BTB_TEMPERATURE,    ///< Temperature in the park.
	BTB_SPACING,        ///< Status panel containing nothing (yet).
	BTB_VIEW_DIRECTION, ///< Status panel containing viewing direction.
	BTB_DATE,           ///< Status panel containing date.
};

static const uint32 BOTTOM_BAR_HEIGHT = 35;     ///< Minimum Y-coord size of the bottom toolbar (BTB) panel.
static const uint32 BOTTOM_BAR_POSITION_X = 75; ///< Separation of the toolbar from the edge of the window.

/**
 * Widget parts of the bottom toolbar GUI.
 * @ingroup gui_group
 * @todo Left/Right Padding get ignored when drawing text widgets
 * @todo Implement non-minimal default window size to prevent the need to compute remaining space manually.
 */
static const WidgetPart _bottom_toolbar_widgets[] = {
	Intermediate(0, 1),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
			Intermediate(1, 0), SetPadding(0, 3, 0, 3),
				Widget(WT_LEFT_TEXT, BTB_STATUS, COL_RANGE_ORANGE_BROWN), SetPadding(3, 5, 30, 0), SetData(STR_ARG1, STR_NULL),
						SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
				Widget(WT_EMPTY, BTB_WEATHER, COL_RANGE_ORANGE_BROWN), SetPadding(3, 3, 3, 3), SetFill(0, 1),
				Widget(WT_RIGHT_TEXT, BTB_TEMPERATURE, COL_RANGE_ORANGE_BROWN), SetFill(1, 0), SetData(STR_ARG1, STR_NULL),
				Widget(WT_EMPTY, BTB_SPACING, COL_RANGE_ORANGE_BROWN), SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
				Widget(WT_EMPTY, BTB_VIEW_DIRECTION, COL_RANGE_ORANGE_BROWN), SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
				Widget(WT_RIGHT_TEXT, BTB_DATE, COL_RANGE_ORANGE_BROWN), SetPadding(3, 0, 30, 0), SetData(STR_ARG1, STR_NULL),
						SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
			EndContainer(),
	EndContainer(),
};

BottomToolbarWindow::BottomToolbarWindow() : GuiWindow(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_bottom_toolbar_widgets, lengthof(_bottom_toolbar_widgets));
}

Point32 BottomToolbarWindow::OnInitialPosition()
{
	static Point32 pt;
	pt.x = BOTTOM_BAR_POSITION_X;
	pt.y = _video.GetYSize() - BOTTOM_BAR_HEIGHT;
	return pt;
}

void BottomToolbarWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case BTB_STATUS:
			_finances_manager.CashToStrParams();
			break;

		case BTB_DATE:
			_str_params.SetDate(1, _date);
			break;

		case BTB_TEMPERATURE:
			_str_params.SetTemperature(1, _weather.temperature);
			break;
	}
}

void BottomToolbarWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

void BottomToolbarWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	/* -99,999,999.99 = Maximum amount of money that is worth handling for now. */
	static const int64 LARGE_MONEY_AMOUNT = -9999999999;
	static const int LARGE_TEMPERATURE = 999; // Large enough to format all temperatures.
	Point32 p(0, 0);

	switch (wid_num) {
		case BTB_STATUS:
			p = GetMoneyStringSize(LARGE_MONEY_AMOUNT);
			break;

		case BTB_VIEW_DIRECTION: {
			Rectangle16 rect = _sprite_manager.GetTableSpriteSize(SPR_GUI_COMPASS_START); // It's the same size for all compass sprites.
			p = {rect.width, rect.height};
			break;
		}

		case BTB_WEATHER: {
			Rectangle16 rect = _sprite_manager.GetTableSpriteSize(SPR_GUI_WEATHER_START);
			p = {rect.width, rect.height};
			break;
		}

		case BTB_TEMPERATURE:
			_str_params.SetTemperature(1, LARGE_TEMPERATURE);
			GetTextSize(STR_ARG1, &p.x, &p.y);
			break;

		case BTB_SPACING: {
			_str_params.SetNumber(1, LARGE_TEMPERATURE);
			Point32 temp_size;
			GetTextSize(STR_ARG1, &temp_size.x, &temp_size.y);

			int32 remaining = _video.GetXSize() - (2 * BOTTOM_BAR_POSITION_X);
			remaining -= temp_size.x;
			remaining -= _sprite_manager.GetTableSpriteSize(SPR_GUI_WEATHER_START).width;
			remaining -= GetMoneyStringSize(LARGE_MONEY_AMOUNT).x;
			remaining -= GetMaxDateSize().x;
			remaining -= _sprite_manager.GetTableSpriteSize(SPR_GUI_COMPASS_START).base.x; // It's the same size for all compass sprites.
			p = {remaining, (int32)BOTTOM_BAR_HEIGHT};
			break;
		}

		case BTB_DATE:
			p = GetMaxDateSize();
			break;
	}

	wid->min_x = std::max(wid->min_x, (uint16)p.x);
	wid->min_y = std::max(wid->min_y, (uint16)p.y);
}

void BottomToolbarWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	static Recolouring recolour; // Never changed.

	switch (wid_num) {
		case BTB_VIEW_DIRECTION: {
			Viewport *vp = GetViewport();
			int dir = (vp == nullptr) ? 0 : vp->orientation;
			const ImageData *img = _sprite_manager.GetTableSprite(SPR_GUI_COMPASS_START + dir);
			if (img != nullptr) _video.BlitImage({GetWidgetScreenX(wid), GetWidgetScreenY(wid)}, img, recolour, GS_NORMAL);
			break;
		}

		case BTB_WEATHER: {
			int spr = SPR_GUI_WEATHER_START + _weather.GetWeatherType();
			const ImageData *img = _sprite_manager.GetTableSprite(spr);
			if (img != nullptr) _video.BlitImage({GetWidgetScreenX(wid), GetWidgetScreenY(wid)}, img, recolour, GS_NORMAL);
			break;
		}
	}
}

/**
 * Open the bottom toolbar window.
 * @ingroup gui_group
 */
void ShowBottomToolbar()
{
	new BottomToolbarWindow();
}

/** %Window to ask confirmation for ending the program. */
class QuitProgramWindow : public GuiWindow {
public:
	QuitProgramWindow();

	Point32 OnInitialPosition() override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
};

/**
 * Widget numbers of the quit-program window.
 * @ingroup gui_group
 */
enum QuitProgramWidgets {
	QP_MESSAGE, ///< Displayed message.
	QP_YES,     ///< 'yes' button.
	QP_NO,      ///< 'no' button.
};

/** Window definition of the quit program GUI. */
static const WidgetPart _quit_program_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(GUI_QUIT_CAPTION, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Intermediate(2,0),
				Widget(WT_CENTERED_TEXT, QP_MESSAGE, COL_RANGE_RED),
						SetData(GUI_QUIT_MESSAGE, STR_NULL), SetPadding(5, 5, 5, 5),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_NO, COL_RANGE_YELLOW), SetData(GUI_QUIT_NO, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_YES, COL_RANGE_YELLOW), SetData(GUI_QUIT_YES, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
			EndContainer(),
};

Point32 QuitProgramWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video.GetXSize() - this->rect.width ) / 2;
	pt.y = (_video.GetYSize() - this->rect.height) / 2;
	return pt;
}

/** Default constructor. */
QuitProgramWindow::QuitProgramWindow() : GuiWindow(WC_QUIT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_quit_program_widgets, lengthof(_quit_program_widgets));
}

void QuitProgramWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	if (number == QP_YES) QuitProgram();
	_window_manager.DeleteWindow(this);
}

/** Ask the user whether the program should be stopped. */
void ShowQuitProgram()
{
	Window *w = GetWindowByType(WC_QUIT, ALL_WINDOWS_OF_TYPE);
	if (w != nullptr) _window_manager.DeleteWindow(w);

	new QuitProgramWindow();
}
