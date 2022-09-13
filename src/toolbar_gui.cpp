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
#include "sprite_data.h"
#include "sprite_store.h"
#include "gui_sprites.h"
#include "messages.h"
#include "person.h"
#include "people.h"
#include "viewport.h"
#include "gamecontrol.h"
#include "weather.h"
#include "gameobserver.h"

/**
 * Top toolbar.
 * @ingroup gui_group
 */
class ToolbarWindow : public GuiWindow {
public:
	ToolbarWindow();

	Point32 OnInitialPosition() override;
	void OnDraw(MouseModeSelector *selector) override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
};

/**
 * Widget numbers of the toolbar GUI.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	TB_DROPDOWN_MAIN,     ///< Main menu dropdown.
	TB_DROPDOWN_VIEW,     ///< View options dropdown.
	TB_SPEED_0,           ///< Pause game button.
	TB_SPEED_1,           ///< 1× game speed button.
	TB_SPEED_2,           ///< 2× game speed button.
	TB_SPEED_4,           ///< 4× game speed button.
	TB_SPEED_8,           ///< 8× game speed button.
	TB_GUI_PATHS,         ///< Build paths button.
	TB_GUI_RIDE_SELECT,   ///< Select ride button.
	TB_GUI_FENCE,         ///< Select fence button.
	TB_GUI_SCENERY,       ///< Select scenery button.
	TB_GUI_PATH_OBJECTS,  ///< Select path objects button.
	TB_GUI_TERRAFORM,     ///< Terraform button.
	TB_GUI_FINANCES,      ///< Finances button.
	TB_GUI_STAFF,         ///< Staff button.
	TB_GUI_INBOX,         ///< Inbox button.
	TB_GUI_PARK,          ///< Park management button.
};

/**
 * Entries in the main menu dropdown.
 * @ingroup gui_group
 */
enum DropdownMain {
	DDM_SAVE,      ///< Save game.
	DDM_GAME_MODE, ///< Switch game mode.
	DDM_SETTINGS,  ///< General settings.
	DDM_MENU,      ///< Back to main menu.
	DDM_QUIT,      ///< Quit the game.
	DDM_COUNT      ///< Number of entries.
};

/**
 * Entries in the view options dropdown.
 * @ingroup gui_group
 */
enum DropdownView {
	DDV_MINIMAP,      ///< Open the minimap.
	DDV_GRID,         ///< Toggle terrain grid.
	DDV_UNDERGROUND,  ///< Toggle underground view.
	DDV_UNDERWATER,   ///< Toggle underwater view.
	DDV_WIRE_RIDES,        ///< Toggle wireframe view for rides.
	DDV_WIRE_SCENERY,      ///< Toggle wireframe view for scenery items.
	DDV_HIDE_PEOPLE,       ///< Toggle visibility of people.
	DDV_HIDE_SUPPORTS,     ///< Toggle visibility of supports.
	DDV_HIDE_SURFACES,     ///< Toggle visibility of surfaces.
	DDV_HIDE_FOUNDATIONS,  ///< Toggle visibility of foundations.
	DDV_HEIGHT_RIDES,      ///< Toggle height markers on rides.
	DDV_HEIGHT_PATHS,      ///< Toggle height markers on paths.
	DDV_HEIGHT_TERRAIN,    ///< Toggle height markers on terrain.
};

/**
 * Widget parts of the toolbar GUI.
 * @ingroup gui_group
 */
static const WidgetPart _toolbar_widgets[] = {
	Intermediate(1, 0),
		Widget(WT_IMAGE_DROPDOWN_BUTTON, TB_DROPDOWN_MAIN,   COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_MAIN,  GUI_TOOLBAR_GUI_DROPDOWN_MAIN),
		Widget(WT_IMAGE_DROPDOWN_BUTTON, TB_DROPDOWN_VIEW,   COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_VIEW,  GUI_TOOLBAR_GUI_DROPDOWN_VIEW_TOOLTIP),
		Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetMinimalSize(16, 16),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
			Intermediate(1, 0),
				Widget(WT_IMAGE_BUTTON, TB_SPEED_0, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_SPEED_0, GUI_TOOLBAR_GUI_DROPDOWN_SPEED_PAUSE), SetPadding(8, 0, 8, 8),
				Widget(WT_IMAGE_BUTTON, TB_SPEED_1, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_SPEED_1, GUI_TOOLBAR_GUI_DROPDOWN_SPEED_1), SetPadding(8, 0, 8, 0),
				Widget(WT_IMAGE_BUTTON, TB_SPEED_2, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_SPEED_2, GUI_TOOLBAR_GUI_DROPDOWN_SPEED_2), SetPadding(8, 0, 8, 0),
				Widget(WT_IMAGE_BUTTON, TB_SPEED_4, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_SPEED_4, GUI_TOOLBAR_GUI_DROPDOWN_SPEED_4), SetPadding(8, 0, 8, 0),
				Widget(WT_IMAGE_BUTTON, TB_SPEED_8, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_SPEED_8, GUI_TOOLBAR_GUI_DROPDOWN_SPEED_8), SetPadding(8, 8, 8, 0),
			EndContainer(),
		Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetMinimalSize(16, 16),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_TERRAFORM,    COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_TERRAIN, GUI_TOOLBAR_GUI_TOOLTIP_TERRAFORM),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_PATHS,        COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_PATH,    GUI_TOOLBAR_GUI_TOOLTIP_BUILD_PATHS),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_FENCE,        COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_FENCE,   GUI_TOOLBAR_GUI_TOOLTIP_FENCE),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_SCENERY,      COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_SCENERY, GUI_TOOLBAR_GUI_TOOLTIP_SCENERY),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_PATH_OBJECTS, COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_OBJECTS, GUI_TOOLBAR_GUI_TOOLTIP_PATH_OBJECTS),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_RIDE_SELECT,  COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_RIDE,    GUI_TOOLBAR_GUI_TOOLTIP_RIDE_SELECT),
		Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetMinimalSize(16, 16),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_PARK,        COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_PARK,     GUI_TOOLBAR_GUI_TOOLTIP_PARK),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_STAFF,       COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_STAFF,    GUI_TOOLBAR_GUI_TOOLTIP_STAFF),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_INBOX,       COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_INBOX,    GUI_TOOLBAR_GUI_TOOLTIP_INBOX),
		Widget(WT_IMAGE_PUSHBUTTON, TB_GUI_FINANCES,    COL_RANGE_ORANGE_BROWN), SetData(SPR_GUI_TOOLBAR_FINANCES, GUI_TOOLBAR_GUI_TOOLTIP_FINANCES),
	EndContainer(),
};

ToolbarWindow::ToolbarWindow() : GuiWindow(WC_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->closeable = false;
	this->SetupWidgetTree(_toolbar_widgets, lengthof(_toolbar_widgets));
}

Point32 ToolbarWindow::OnInitialPosition()
{
	static const Point32 pt(10, 0);
	return pt;
}

void ToolbarWindow::OnDraw(MouseModeSelector *selector)
{
	this->SetWidgetPressed(TB_SPEED_0, _game_control.speed == GSP_PAUSE);
	this->SetWidgetPressed(TB_SPEED_1, _game_control.speed == GSP_1);
	this->SetWidgetPressed(TB_SPEED_2, _game_control.speed == GSP_2);
	this->SetWidgetPressed(TB_SPEED_4, _game_control.speed == GSP_4);
	this->SetWidgetPressed(TB_SPEED_8, _game_control.speed == GSP_8);

	GuiWindow::OnDraw(selector);
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

void ToolbarWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	switch (number) {
		case TB_DROPDOWN_MAIN: {
			DropdownList itemlist;
			for (int i = 0; i < DDM_COUNT; i++) {
				_str_params.SetStrID(1, i == DDM_GAME_MODE ? GetSwitchGameModeString() : GUI_MAIN_MENU_SAVE + i);
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(number, itemlist, -1);
			break;
		}
		case TB_DROPDOWN_VIEW: {
			DropdownList itemlist;
			/* Keep the order consistent with the DropdownView ordering! */
			/* DDV_MINIMAP */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_MINIMAP));
			/* DDV_GRID */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_GRID, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->grid ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_UNDERGROUND */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_UNDERGROUND, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->underground_mode ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_UNDERWATER */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_UNDERWATER, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->underwater_mode ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_WIRE_RIDES */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_WIRE_RIDES, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->wireframe_rides ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_WIRE_SCENERY */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_WIRE_SCENERY, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->wireframe_scenery ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HIDE_PEOPLE */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HIDE_PEOPLE, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->hide_people ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HIDE_SUPPORTS */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HIDE_SUPPORTS, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->hide_supports ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HIDE_SURFACES */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HIDE_SURFACES, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->hide_surfaces ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HIDE_FOUNDATIONS */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HIDE_FOUNDATIONS, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->hide_foundations ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HEIGHT_RIDES */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HEIGHT_RIDES, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->height_markers_rides ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HEIGHT_PATHS */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HEIGHT_PATHS, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->height_markers_paths ? DDIF_SELECTED : DDIF_NONE)));
			/* DDV_HEIGHT_TERRAIN */
			itemlist.push_back(DropdownItem(GUI_TOOLBAR_GUI_DROPDOWN_VIEW_HEIGHT_TERRAIN, DDIF_SELECTABLE |
					(_window_manager.GetViewport()->height_markers_terrain ? DDIF_SELECTED : DDIF_NONE)));

			this->ShowDropdownMenu(number, itemlist, -1);
			break;
		}

		case TB_SPEED_0:
			_game_control.speed = GSP_PAUSE;
			break;
		case TB_SPEED_1:
			_game_control.speed = GSP_1;
			break;
		case TB_SPEED_2:
			_game_control.speed = GSP_2;
			break;
		case TB_SPEED_4:
			_game_control.speed = GSP_4;
			break;
		case TB_SPEED_8:
			_game_control.speed = GSP_8;
			break;

		case TB_GUI_PATHS:
			ShowPathBuildGui();
			break;

		case TB_GUI_RIDE_SELECT:
			ShowRideSelectGui();
			break;

		case TB_GUI_FENCE:
			ShowFenceGui();
			break;

		case TB_GUI_SCENERY:
			ShowSceneryGui();
			break;

		case TB_GUI_PATH_OBJECTS:
			ShowPathObjectsGui();
			break;

		case TB_GUI_TERRAFORM:
			ShowTerraformGui();
			break;

		case TB_GUI_FINANCES:
			ShowFinancesGui();
			break;

		case TB_GUI_PARK:
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_GENERAL);
			break;

		case TB_GUI_STAFF:
			ShowStaffManagementGui();
			break;

		case TB_GUI_INBOX:
			ShowInboxGui();
			break;
	}
}

void ToolbarWindow::OnChange(ChangeCode code, uint32 parameter)
{
	switch (code) {
		case CHG_DROPDOWN_RESULT: {
			const int entry = parameter & 0xFF;
			switch ((parameter >> 16) & 0xFF) {
				case TB_DROPDOWN_MAIN:
					switch (entry) {
						case DDM_QUIT:
							ShowQuitProgram(false);
							break;
						case DDM_SETTINGS:
							ShowSettingGui();
							break;
						case DDM_GAME_MODE:
							_game_mode_mgr.SetGameMode(_game_mode_mgr.InEditorMode() ? GM_PLAY : GM_EDITOR);
							break;
						case DDM_SAVE:
							ShowSaveGameGui();
							break;
						case DDM_MENU:
							ShowQuitProgram(true);
							break;
						default: NOT_REACHED();
					}
					break;
				case TB_DROPDOWN_VIEW:
					switch (entry) {
						case DDV_MINIMAP:
							ShowMinimap();
							break;
						case DDV_UNDERGROUND:
							_window_manager.GetViewport()->ToggleUndergroundMode();
							break;

#define TOGGLE(what) _window_manager.GetViewport()->what = !_window_manager.GetViewport()->what
						case DDV_UNDERWATER:       TOGGLE(underwater_mode); break;
						case DDV_GRID:             TOGGLE(grid); break;
						case DDV_WIRE_RIDES:       TOGGLE(wireframe_rides); break;
						case DDV_WIRE_SCENERY:     TOGGLE(wireframe_scenery); break;
						case DDV_HIDE_PEOPLE:      TOGGLE(hide_people); break;
						case DDV_HIDE_SUPPORTS:    TOGGLE(hide_supports); break;
						case DDV_HIDE_SURFACES:    TOGGLE(hide_surfaces); break;
						case DDV_HIDE_FOUNDATIONS: TOGGLE(hide_foundations); break;
						case DDV_HEIGHT_RIDES:     TOGGLE(height_markers_rides); break;
						case DDV_HEIGHT_PATHS:     TOGGLE(height_markers_paths); break;
						case DDV_HEIGHT_TERRAIN:   TOGGLE(height_markers_terrain); break;
#undef TOGGLE

						default: NOT_REACHED();
					}
					break;
			}
			break;
		}

		default:
			break;
	}
}

void ToolbarWindow::UpdateWidgetSize([[maybe_unused]] WidgetNumber wid_num, [[maybe_unused]] BaseWidget *wid)
{
}

void ToolbarWindow::SetWidgetStringParameters([[maybe_unused]] WidgetNumber wid_num) const
{
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
	void SetTooltipStringParameters(BaseWidget *tooltip_widget) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(const WidgetNumber wid_num, const Point16 &pos) override;
};

/**
 * Widget numbers of the bottom toolbar GUI.
 * @ingroup gui_group
 */
enum BottomToolbarGuiWidgets {
	BTB_EMPTY,          ///< Empty widget defining the width of the status bar.
	BTB_CASH,           ///< Status panel containing the park's current cash.
	BTB_WEATHER,        ///< Weather sprite.
	BTB_TEMPERATURE,    ///< Temperature in the park.
	BTB_MESSAGE,        ///< A preview of the last message.
	BTB_VIEW_DIRECTION, ///< Status panel containing viewing direction.
	BTB_GUESTCOUNT,     ///< Display of number of guests in the park.
	BTB_PARK_RATING,    ///< Display of the park rating.
	BTB_DATE,           ///< Status panel containing date.
};

static const uint32 BOTTOM_BAR_HEIGHT = 55;     ///< Minimum Y-coord size of the bottom toolbar (BTB) panel.
static const uint32 BOTTOM_BAR_POSITION_X = 75; ///< Separation of the toolbar from the edge of the window.

/**
 * Widget parts of the bottom toolbar GUI.
 * @ingroup gui_group
 * @todo Left/Right Padding get ignored when drawing text widgets
 * @todo Implement non-minimal default window size to prevent the need to compute remaining space manually.
 */
static const WidgetPart _bottom_toolbar_widgets[] = {
	Intermediate(0, 1),
		Widget(WT_EMPTY, BTB_EMPTY, COL_RANGE_INVALID),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
			Intermediate(1, 0),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
					Intermediate(3, 1),
						Widget(WT_CENTERED_TEXT, BTB_CASH, COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1, STR_NULL),
						Widget(WT_CENTERED_TEXT, BTB_GUESTCOUNT, COL_RANGE_ORANGE_BROWN), SetData(GUI_BOTTOMBAR_GUESTCOUNT, STR_NULL),
						Widget(WT_EMPTY, BTB_PARK_RATING, COL_RANGE_ORANGE_BROWN), SetData(STR_NULL, GUI_PARK_MANAGEMENT_RATING), SetFill(1, 1),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
					Widget(WT_EMPTY, BTB_MESSAGE, COL_RANGE_ORANGE_BROWN), SetFill(1, 0), SetMinimalSize(300, BOTTOM_BAR_HEIGHT),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
					Intermediate(1, 2),
						Widget(WT_EMPTY, BTB_VIEW_DIRECTION, COL_RANGE_ORANGE_BROWN), SetMinimalSize(1, BOTTOM_BAR_HEIGHT), SetFill(1, 1),
						Intermediate(2, 1),
							Widget(WT_RIGHT_TEXT, BTB_DATE, COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1, STR_NULL), SetFill(0, 0),
							Intermediate(1, 2),
								Widget(WT_RIGHT_TEXT, BTB_TEMPERATURE, COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1, STR_NULL), SetFill(1, 0),
								Widget(WT_EMPTY, BTB_WEATHER, COL_RANGE_ORANGE_BROWN), SetFill(1, 1),
			EndContainer(),
	EndContainer(),
};

BottomToolbarWindow::BottomToolbarWindow() : GuiWindow(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->closeable = false;
	this->SetupWidgetTree(_bottom_toolbar_widgets, lengthof(_bottom_toolbar_widgets));
}

Point32 BottomToolbarWindow::OnInitialPosition()
{
	static Point32 pt;
	pt.x = BOTTOM_BAR_POSITION_X;
	pt.y = _video.Height() - BOTTOM_BAR_HEIGHT;
	return pt;
}

void BottomToolbarWindow::SetTooltipStringParameters(BaseWidget *tooltip_widget) const
{
	GuiWindow::SetTooltipStringParameters(tooltip_widget);
	if (tooltip_widget == this->GetWidget<BaseWidget>(BTB_PARK_RATING)) {
		_str_params.SetNumber(1, _game_observer.current_park_rating);
	}
}

void BottomToolbarWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case BTB_CASH:
			_finances_manager.CashToStrParams();
			break;

		case BTB_DATE:
			_str_params.SetDate(1, _date);
			break;

		case BTB_TEMPERATURE:
			_str_params.SetTemperature(1, _weather.temperature);
			break;

		case BTB_GUESTCOUNT:
			_str_params.SetNumberAndPlural(1, _game_observer.current_guest_count);
			break;
	}
}

void BottomToolbarWindow::OnClick(const WidgetNumber wid_num, const Point16 &pos)
{
	switch (wid_num) {
		case BTB_MESSAGE:
			if (_inbox.display_message == nullptr) {
				ShowInboxGui();
			} else {
				BaseWidget *w = this->GetWidget<BaseWidget>(wid_num);
				if (pos.x < w->pos.width - w->pos.height) {
					_inbox.DismissDisplayMessage();
				} else {
					_inbox.display_message->OnClick();
				}
			}
			break;

		case BTB_CASH:
			ShowFinancesGui();
			break;

		case BTB_PARK_RATING:
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_RATING);
			break;

		case BTB_GUESTCOUNT:
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_GUESTS);
			break;

		default:
			GuiWindow::OnClick(wid_num, pos);
			break;
	}
}

void BottomToolbarWindow::OnChange(ChangeCode code, [[maybe_unused]] uint32 parameter)
{
	switch (code) {
		case CHG_RESOLUTION_CHANGED:
			this->ResetSize();
			break;

		default:
			break; // Ignore other messages.
	}
}

void BottomToolbarWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	/* -99,999,999.99 = Maximum amount of money that is worth handling for now. */
	static const int64 LARGE_MONEY_AMOUNT = -9999999999;
	static const int LARGE_TEMPERATURE = 999; // Large enough to format all temperatures.
	static const int MANY_GUESTS = 10000;
	Point32 p(0, 0);

	switch (wid_num) {
		case BTB_CASH:
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

		case BTB_GUESTCOUNT:
			_str_params.SetNumber(1, MANY_GUESTS);
			GetTextSize(GUI_BOTTOMBAR_GUESTCOUNT, &p.x, &p.y);
			break;

		case BTB_EMPTY:
			p.x = _video.Width() - (2 * BOTTOM_BAR_POSITION_X);
			break;

		case BTB_DATE:
			p = GetMaxDateSize();
			break;
	}

	wid->min_x = std::max(wid->min_x, (uint16)p.x);
	wid->min_y = std::max(wid->min_y, (uint16)p.y);
}

void BottomToolbarWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case BTB_VIEW_DIRECTION: {
			Viewport *vp = _window_manager.GetViewport();
			int dir = (vp == nullptr) ? 0 : vp->orientation;
			const ImageData *img = _sprite_manager.GetTableSprite(SPR_GUI_COMPASS_START + dir);
			if (img != nullptr) {
				_video.BlitImage({GetWidgetScreenX(wid) + (wid->pos.width - img->width) / 2,
						GetWidgetScreenY(wid) + (wid->pos.height - img->height) / 2}, img);
			}
			break;
		}

		case BTB_WEATHER: {
			int spr = SPR_GUI_WEATHER_START + _weather.GetWeatherType();
			const ImageData *img = _sprite_manager.GetTableSprite(spr);
			if (img != nullptr) _video.BlitImage({GetWidgetScreenX(wid), GetWidgetScreenY(wid)}, img);
			break;
		}

		case BTB_PARK_RATING: {
			const int x = this->GetWidgetScreenX(wid) + 3;
			const int y = this->GetWidgetScreenY(wid) + 3;
			const int w = wid->pos.width  - 7;
			const int h = wid->pos.height - 7;
			const int col = 255 * _game_observer.current_park_rating / MAX_PARK_RATING;
			assert(col >= 0 && col <= 255);
			_video.FillRectangle(Rectangle32(x, y, w * _game_observer.current_park_rating / MAX_PARK_RATING, h),
					0xff | (col << 16) | ((255 - col) << 24));
			_video.DrawRectangle(Rectangle32(x, y, w, h), 0xff);
			break;
		}

		case BTB_MESSAGE:
			if (_inbox.display_message != nullptr) {
				DrawMessage(_inbox.display_message, Rectangle32(this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid), wid->pos.width, wid->pos.height), true);
			}
			break;
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
	explicit QuitProgramWindow(bool back_to_main_menu);

	Point32 OnInitialPosition() override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;

private:
	bool back_to_main_menu;  ///< Just return to the main menu instead of quitting FreeRCT.
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
#define QUIT_PROGRAM_WIDGET_TREE(caption, message) \
const WidgetPart _quit_program_widgets[] = { \
	Intermediate(0, 1), \
		Intermediate(1, 0), \
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(caption, GUI_TITLEBAR_TIP), \
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED), \
		EndContainer(), \
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED), \
			Intermediate(2,0), \
				Widget(WT_CENTERED_TEXT, QP_MESSAGE, COL_RANGE_RED), \
						SetData(message, STR_NULL), SetPadding(5, 5, 5, 5), \
			EndContainer(), \
			Intermediate(1, 5), SetPadding(0, 0, 3, 0), \
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0), \
				Widget(WT_TEXT_PUSHBUTTON, QP_NO, COL_RANGE_YELLOW), SetData(GUI_QUIT_NO, STR_NULL), \
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0), \
				Widget(WT_TEXT_PUSHBUTTON, QP_YES, COL_RANGE_YELLOW), SetData(GUI_QUIT_YES, STR_NULL), \
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0), \
			EndContainer(), \
}

Point32 QuitProgramWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video.Width() - this->rect.width ) / 2;
	pt.y = (_video.Height() - this->rect.height) / 2;
	return pt;
}

/**
 * Constructor.
 * @param b Just return to the main menu instead of quitting FreeRCT.
 */
QuitProgramWindow::QuitProgramWindow(bool b) : GuiWindow(WC_QUIT, ALL_WINDOWS_OF_TYPE)
{
	this->back_to_main_menu = b;
	QUIT_PROGRAM_WIDGET_TREE(this->back_to_main_menu ? GUI_RETURN_CAPTION : GUI_QUIT_CAPTION, this->back_to_main_menu ? GUI_RETURN_MESSAGE : GUI_QUIT_MESSAGE);
	this->SetupWidgetTree(_quit_program_widgets, lengthof(_quit_program_widgets));
}

bool QuitProgramWindow::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (key_code == WMKC_CONFIRM) {
		this->OnClick(QP_YES, Point16());
		return true;
	}
	return GuiWindow::OnKeyEvent(key_code, mod, symbol);
}

void QuitProgramWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	if (number == QP_YES) {
		if (this->back_to_main_menu) {
			_game_control.MainMenu();
		} else {
			_game_control.QuitGame();
		}
	} else if (number != QP_NO) {
		return;
	}
	delete this;
}

/**
 * Ask the user whether the game should be stopped.
 * @param back_to_main_menu Just return to the main menu instead of quitting FreeRCT.
 */
void ShowQuitProgram(const bool back_to_main_menu)
{
	Window *w = GetWindowByType(WC_QUIT, ALL_WINDOWS_OF_TYPE);
	delete w;

	new QuitProgramWindow(back_to_main_menu);
}
