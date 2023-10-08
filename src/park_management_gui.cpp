/* $Id: park_management_gui.cpp */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file park_management_gui.cpp General park management. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "finances.h"
#include "people.h"
#include "gamecontrol.h"
#include "gui_sprites.h"
#include "sprite_data.h"
#include "gameobserver.h"

static const uint MIN_MAX_GUESTS       = 100;  ///< Smallest allowed value for the guests limit.
static const uint MAX_GUESTS_STEP_SIZE = 100;  ///< Change when clicking the max guests buttons once.
static const uint OBJECTIVE_GUESTS_STEP_SIZE      =   10;    ///< Change when clicking the guests objective buttons once.
static const uint OBJECTIVE_RATING_STEP_SIZE      =   25;    ///< Change when clicking the park rating objective buttons once.
static const Money OBJECTIVE_PARK_VALUE_STEP_SIZE = 100000;  ///< Change when clicking the park value objective buttons once.
static const uint OBJECTIVE_DROP_POLICY_STEP_SIZE =    1;    ///< Change when clicking the objective drop policy buttons once.

/**
 * Park management GUI.
 * @ingroup gui_group
 */
class ParkManagementGui : public GuiWindow {
public:
	explicit ParkManagementGui(ParkManagementGuiTabs tab);

	void SelectTab(WidgetNumber widget);
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void UpdateButtons();

private:
	std::pair<uint32, bool> obj_editing_guests;       ///< Objective editing: Number of guests to achieve.
	std::pair<uint16, bool> obj_editing_rating;       ///< Objective editing: Park rating to achieve.
	std::pair<Money , bool> obj_editing_park_value;   ///< Objective editing: Park value to achieve.
	std::pair<uint32, bool> obj_editing_drop_policy;  ///< Objective editing: Park rating objective drop policy.
	std::pair<std::pair<Date, bool /* not-exact */>, bool> obj_editing_timeout;  ///< Objective editing: Objective timeout.
};

/**
 * Widget numbers of the park management GUI.
 * @ingroup gui_group
 * @note The tabbutton constants must be in sync with the #ParkManagementGuiTabs.
 */
enum ParkManagementWidgets {
	PM_TABBUTTON_GENERAL = 0,  ///< General settings tab button.
	PM_TABBUTTON_GUESTS,       ///< Guests graph tab button.
	PM_TABBUTTON_RATING,       ///< Park rating graph tab button.
	PM_TABBUTTON_OBJECTIVE,    ///< Objective tab button.
	PM_TABBUTTON_AWARDS,       ///< Awards tab button.

	PM_TABPANEL_GENERAL,       ///< General settings tab panel.
	PM_TABPANEL_GUESTS,        ///< Guests graph tab panel.
	PM_TABPANEL_RATING,        ///< Park rating graph tab panel.
	PM_TABPANEL_OBJECTIVE,     ///< Objective tab panel.
	PM_TABPANEL_AWARDS,        ///< Awards tab panel.

	PM_TITLEBAR,               ///< Window title bar.

	PM_GUESTS_TEXT,            ///< Number of guests text.
	PM_GUESTS_GRAPH,           ///< Number of guests graph.
	PM_MAX_GUESTS_PANEL,       ///< Maximum number of guests panel.
	PM_MAX_GUESTS_TEXT,        ///< Maximum number of guests text.
	PM_MAX_GUESTS_INCREASE,    ///< Maximum number of guests increase button.
	PM_MAX_GUESTS_DECREASE,    ///< Maximum number of guests decrease button.

	PM_RATING_TEXT,            ///< Park rating text.
	PM_RATING_GRAPH,           ///< Park rating graph.

	PM_OBJECTIVE_TEXT,                 ///< Scenario objective text.
	PM_OBJECTIVE_EDIT_PANEL,           ///< Objective editing panel.
	PM_OBJECTIVE_GUESTS_ENABLE,        ///< Objective editing: Enable guests objective.
	PM_OBJECTIVE_GUESTS_INCREASE,      ///< Objective editing: Increase guests objective target.
	PM_OBJECTIVE_GUESTS_DECREASE,      ///< Objective editing: Decrease guests objective target.
	PM_OBJECTIVE_GUESTS_TEXT,          ///< Objective editing: Guests objective text.
	PM_OBJECTIVE_RATING_ENABLE,        ///< Objective editing: Enable park rating objective.
	PM_OBJECTIVE_RATING_INCREASE,      ///< Objective editing: Increase park rating objective target.
	PM_OBJECTIVE_RATING_DECREASE,      ///< Objective editing: Decrease park rating objective target.
	PM_OBJECTIVE_RATING_TEXT,          ///< Objective editing: Park rating objective text.
	PM_OBJECTIVE_DROP_POLICY_ENABLE,   ///< Objective editing: Enable permanent park rating requirement.
	PM_OBJECTIVE_DROP_POLICY_INCREASE, ///< Objective editing: Increase permanent park rating requirement timeout.
	PM_OBJECTIVE_DROP_POLICY_DECREASE, ///< Objective editing: Decrease permanent park rating requirement timeout.
	PM_OBJECTIVE_DROP_POLICY_TEXT,     ///< Objective editing: Permanent park rating requirement timeout text.
	PM_OBJECTIVE_PARK_VALUE_ENABLE,    ///< Objective editing: Enable park value objective.
	PM_OBJECTIVE_PARK_VALUE_INCREASE,  ///< Objective editing: Increase park value objective target.
	PM_OBJECTIVE_PARK_VALUE_DECREASE,  ///< Objective editing: Decrease park value objective target.
	PM_OBJECTIVE_PARK_VALUE_TEXT,      ///< Objective editing: Park value objective text.
	PM_OBJECTIVE_DATE_ENABLE,          ///< Objective editing: Objective target enable date policy.
	PM_OBJECTIVE_DATE_NOT_EXACT,       ///< Objective editing: Objective target date policy Before.
	PM_OBJECTIVE_DATE_INCREASE,        ///< Objective editing: Increase objective target date.
	PM_OBJECTIVE_DATE_DECREASE,        ///< Objective editing: Decrease objective target date.
	PM_OBJECTIVE_DATE_TEXT,            ///< Objective editing: Objective target date text.
	PM_OBJECTIVE_APPLY,                ///< Objective editing: Apply changes.

	PM_PARKNAME,               ///< Park name edit box.
	PM_ENTRANCE_FEE_ENABLE,    ///< Park entrance fee enable checkbox.
	PM_ENTRANCE_FEE_TEXT,      ///< Park entrance fee text.
	PM_ENTRANCE_FEE_INCREASE,  ///< Park entrance fee increase button.
	PM_ENTRANCE_FEE_DECREASE,  ///< Park entrance fee decrease button.
	PM_GOTO,                   ///< Go to park entrance button.
	PM_OPEN_PARK_PANEL,        ///< Open park button.
	PM_CLOSE_PARK_PANEL,       ///< Close park button.
	PM_OPEN_PARK_LIGHT,        ///< Open park light.
	PM_CLOSE_PARK_LIGHT,       ///< Close park light.
};

/** Widgets of the tab bar. */
static const WidgetNumber _tab_bar[] = {PM_TABBUTTON_GENERAL, PM_TABBUTTON_GUESTS, PM_TABBUTTON_RATING, PM_TABBUTTON_OBJECTIVE, PM_TABBUTTON_AWARDS, INVALID_WIDGET_INDEX};

/** Widgets of the tab container. */
static const WidgetNumber _tab_container[] = {PM_TABPANEL_GENERAL, PM_TABPANEL_GUESTS, PM_TABPANEL_RATING, PM_TABPANEL_OBJECTIVE, PM_TABPANEL_AWARDS, INVALID_WIDGET_INDEX};

static const int GRAPH_WIDTH  = 420;  ///< Width of the graph display in pixels.
static const int GRAPH_HEIGHT = 200;  ///< Height of the graph display in pixels.

/**
 * Widget parts of the park management GUI.
 * @ingroup gui_group
 */
static const WidgetPart _pm_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, PM_TITLEBAR, COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
			Intermediate(0, 1),
				Intermediate(1, 0),
					Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
					Widget(WT_TEXT_TAB, PM_TABBUTTON_GENERAL, COL_RANGE_ORANGE_BROWN),
							SetData(GUI_PARK_MANAGEMENT_TAB_GENERAL, GUI_PARK_MANAGEMENT_TAB_GENERAL_TOOLTIP),
					Widget(WT_TEXT_TAB, PM_TABBUTTON_GUESTS, COL_RANGE_ORANGE_BROWN),
							SetData(GUI_PARK_MANAGEMENT_TAB_GUESTS, GUI_PARK_MANAGEMENT_TAB_GUESTS_TOOLTIP),
					Widget(WT_TEXT_TAB, PM_TABBUTTON_RATING, COL_RANGE_ORANGE_BROWN),
							SetData(GUI_PARK_MANAGEMENT_TAB_RATING, GUI_PARK_MANAGEMENT_TAB_RATING_TOOLTIP),
					Widget(WT_TEXT_TAB, PM_TABBUTTON_OBJECTIVE, COL_RANGE_ORANGE_BROWN),
							SetData(GUI_PARK_MANAGEMENT_TAB_OBJECTIVE, GUI_PARK_MANAGEMENT_TAB_OBJECTIVE_TOOLTIP),
					Widget(WT_TEXT_TAB, PM_TABBUTTON_AWARDS, COL_RANGE_ORANGE_BROWN),
							SetData(GUI_PARK_MANAGEMENT_TAB_AWARDS, GUI_PARK_MANAGEMENT_TAB_AWARDS_TOOLTIP),
					Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetFill(1, 0), SetResize(1, 0),
				EndContainer(),

				Widget(WT_TAB_PANEL, PM_TABPANEL_GENERAL, COL_RANGE_ORANGE_BROWN),
					Intermediate(0, 1),
						Widget(WT_TAB_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetPadding(4, 4, 4, 4),
							Intermediate(1, 2),
								Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN),
										SetData(GUI_PARK_MANAGEMENT_PARKNAME, STR_NULL),
								Widget(WT_TEXT_INPUT, PM_PARKNAME, COL_RANGE_ORANGE_BROWN),
										SetFill(1, 0), SetResize(1, 0), SetMinimalSize(GRAPH_WIDTH / 2, 1),
						Widget(WT_TAB_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetPadding(4, 4, 4, 4),
							Intermediate(1, 5),
								Widget(WT_CHECKBOX, PM_ENTRANCE_FEE_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
								Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_ENTRANCE_FEE, STR_NULL),
								Widget(WT_TEXT_PUSHBUTTON, PM_ENTRANCE_FEE_DECREASE, COL_RANGE_ORANGE_BROWN),
										SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
								Widget(WT_CENTERED_TEXT  , PM_ENTRANCE_FEE_TEXT    , COL_RANGE_ORANGE_BROWN),
										SetData(STR_ARG1, STR_NULL),
								Widget(WT_TEXT_PUSHBUTTON, PM_ENTRANCE_FEE_INCREASE, COL_RANGE_ORANGE_BROWN),
										SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
						Intermediate(1, 4),
							Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetFill(1, 0), SetResize(1, 0),
							Widget(WT_IMAGE_PUSHBUTTON, PM_GOTO, COL_RANGE_ORANGE_BROWN),
									SetData(SPR_GUI_BUILDARROW_START + EDGE_SE, STR_NULL), SetFill(0, 0), SetResize(0, 0),
							Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetFill(1, 0), SetResize(1, 0),
							Intermediate(2, 1),
								Widget(WT_PANEL, PM_CLOSE_PARK_PANEL, COL_RANGE_ORANGE_BROWN),
									Widget(WT_RADIOBUTTON, PM_CLOSE_PARK_LIGHT, COL_RANGE_RED  ), SetPadding(0, 2, 0, 0),
								Widget(WT_PANEL, PM_OPEN_PARK_PANEL, COL_RANGE_ORANGE_BROWN),
									Widget(WT_RADIOBUTTON, PM_OPEN_PARK_LIGHT , COL_RANGE_GREEN), SetPadding(0, 2, 0, 0),
					EndContainer(),

				Widget(WT_TAB_PANEL, PM_TABPANEL_GUESTS, COL_RANGE_ORANGE_BROWN),
					Intermediate(3, 1),
						Widget(WT_CENTERED_TEXT, PM_GUESTS_TEXT, COL_RANGE_ORANGE_BROWN), SetData(GUI_BOTTOMBAR_GUESTCOUNT, STR_NULL), SetPadding(4, 4, 4, 4),
						Widget(WT_PANEL, PM_MAX_GUESTS_PANEL, COL_RANGE_ORANGE_BROWN),
							Intermediate(1, 4),
								Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_MAX_GUESTS, STR_NULL),
								Widget(WT_TEXT_PUSHBUTTON, PM_MAX_GUESTS_DECREASE, COL_RANGE_ORANGE_BROWN),
										SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
								Widget(WT_CENTERED_TEXT  , PM_MAX_GUESTS_TEXT    , COL_RANGE_ORANGE_BROWN),
										SetData(STR_ARG1, STR_NULL),
								Widget(WT_TEXT_PUSHBUTTON, PM_MAX_GUESTS_INCREASE, COL_RANGE_ORANGE_BROWN),
										SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
						Widget(WT_EMPTY, PM_GUESTS_GRAPH, COL_RANGE_ORANGE_BROWN), SetMinimalSize(GRAPH_WIDTH, GRAPH_HEIGHT), SetFill(1, 1), SetResize(1, 1),

				Widget(WT_TAB_PANEL, PM_TABPANEL_RATING, COL_RANGE_ORANGE_BROWN),
					Intermediate(2, 1),
						Widget(WT_CENTERED_TEXT, PM_RATING_TEXT, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_RATING, STR_NULL), SetPadding(4, 4, 4, 4),
						Widget(WT_EMPTY, PM_RATING_GRAPH, COL_RANGE_ORANGE_BROWN), SetMinimalSize(GRAPH_WIDTH, GRAPH_HEIGHT), SetFill(1, 1), SetResize(1, 1),

				Widget(WT_TAB_PANEL, PM_TABPANEL_OBJECTIVE, COL_RANGE_ORANGE_BROWN),
					Intermediate(2, 1),
						Widget(WT_CENTERED_TEXT, PM_OBJECTIVE_TEXT, COL_RANGE_ORANGE_BROWN), SetData(STR_ARG1, STR_NULL), SetPadding(4, 4, 4, 4),
						Widget(WT_PANEL, PM_OBJECTIVE_EDIT_PANEL, COL_RANGE_ORANGE_BROWN),
							Intermediate(3, 1),
								Intermediate(5, 5),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_PARK_VALUE_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_PARK_VALUE, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_PARK_VALUE_DECREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CENTERED_TEXT  , PM_OBJECTIVE_PARK_VALUE_TEXT    , COL_RANGE_ORANGE_BROWN),
											SetData(STR_ARG1, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_PARK_VALUE_INCREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_GUESTS_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_GUESTS, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_GUESTS_DECREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CENTERED_TEXT  , PM_OBJECTIVE_GUESTS_TEXT    , COL_RANGE_ORANGE_BROWN),
											SetData(STR_ARG1, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_GUESTS_INCREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_RATING_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_RATING, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_RATING_DECREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CENTERED_TEXT  , PM_OBJECTIVE_RATING_TEXT    , COL_RANGE_ORANGE_BROWN),
											SetData(STR_ARG1, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_RATING_INCREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_DROP_POLICY_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_DROP_POLICY, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_DROP_POLICY_DECREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CENTERED_TEXT  , PM_OBJECTIVE_DROP_POLICY_TEXT    , COL_RANGE_ORANGE_BROWN),
											SetData(STR_ARG1, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_DROP_POLICY_INCREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_DATE_ENABLE, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_DATE, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_DATE_DECREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_DECREASE_BUTTON, STR_NULL), SetRepeating(true),
									Widget(WT_CENTERED_TEXT  , PM_OBJECTIVE_DATE_TEXT    , COL_RANGE_ORANGE_BROWN),
											SetData(STR_ARG1, STR_NULL),
									Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_DATE_INCREASE, COL_RANGE_ORANGE_BROWN),
											SetData(GUI_INCREASE_BUTTON, STR_NULL), SetRepeating(true),
								Intermediate(1, 2),
									Widget(WT_CHECKBOX, PM_OBJECTIVE_DATE_NOT_EXACT, COL_RANGE_ORANGE_BROWN), SetPadding(2, 4, 2, 0),
									Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_OBJECTIVE_DATE_NOT_EXACT, STR_NULL),
								Widget(WT_TEXT_PUSHBUTTON, PM_OBJECTIVE_APPLY, COL_RANGE_ORANGE_BROWN), SetData(GUI_APPLY, STR_NULL),

				Widget(WT_TAB_PANEL, PM_TABPANEL_AWARDS, COL_RANGE_ORANGE_BROWN),
					/* \todo Show list of awards once awards are implemented. */
					Widget(WT_CENTERED_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_ORANGE_BROWN), SetData(GUI_PARK_MANAGEMENT_NO_AWARDS, STR_NULL), SetPadding(4, 4, 4, 4),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

ParkManagementGui::ParkManagementGui(ParkManagementGuiTabs tab) : GuiWindow(WC_PARK_MANAGEMENT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_pm_build_gui_parts, lengthof(_pm_build_gui_parts));
	this->SelectTab(tab);

	TextInputWidget *txt = this->GetWidget<TextInputWidget>(PM_PARKNAME);
	txt->SetText(_game_observer.park_name);
	txt->text_changed = [txt]()
	{
		_game_observer.park_name = txt->GetText();
	};

	/* Initialize objective editor with some default values. */
	this->obj_editing_guests = {1000, false};
	this->obj_editing_rating = {600, false};
	this->obj_editing_park_value = {Money(5000000), false};
	this->obj_editing_drop_policy = {28, false};
	this->obj_editing_timeout = {{Date(31, 10, 1), false}, false};

	obj_editing_timeout = {{_scenario.objective->timeout_date, _scenario.objective->timeout_policy == TIMEOUT_BEFORE}, _scenario.objective->timeout_policy != TIMEOUT_NONE};
	for (const auto &objective : _scenario.objective->objectives) {
		switch (objective->Type()) {
			case OJT_GUESTS:
				obj_editing_guests = {static_cast<const ObjectiveGuests*>(objective.get())->nr_guests, true};
				break;
			case OJT_PARK_VALUE:
				obj_editing_park_value = {static_cast<const ObjectiveParkValue*>(objective.get())->park_value, true};
				break;
			case OJT_RATING: {
				obj_editing_rating = {static_cast<ObjectiveParkRating*>(objective.get())->rating, true};
				if (objective->drop_policy.days_after_drop > 0) {
					obj_editing_drop_policy = {objective->drop_policy.days_after_drop, true};
				}
				break;
			}
			default:
				break;
		}
	}

	this->UpdateButtons();
}

/**
 * Select a tab.
 * @param widget Widget number of the tab button.
 */
void ParkManagementGui::SelectTab(WidgetNumber widget)
{
	this->SetRadioButtonsSelected(_tab_bar, _tab_bar[widget]);
	for (int i = 0; _tab_bar[i] != INVALID_WIDGET_INDEX; ++i) {
		this->GetWidget<BackgroundWidget>(_tab_container[i])->SetVisible(this, _tab_bar[i] == widget);
	}
}

void ParkManagementGui::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case PM_TITLEBAR:
			_str_params.SetText(1, _game_observer.park_name);
			break;

		case PM_ENTRANCE_FEE_TEXT:
			_str_params.SetMoney(1, _game_observer.entrance_fee);
			break;

		case PM_MAX_GUESTS_TEXT:
			_str_params.SetNumber(1, _scenario.max_guests);
			break;

		case PM_GUESTS_TEXT:
			_str_params.SetNumberAndPlural(1, _game_observer.current_guest_count);
			break;

		case PM_RATING_TEXT:
			_str_params.SetNumber(1, _game_observer.current_park_rating);
			break;

		case PM_OBJECTIVE_GUESTS_TEXT:
			_str_params.SetNumber(1, this->obj_editing_guests.first);
			break;
		case PM_OBJECTIVE_RATING_TEXT:
			_str_params.SetNumber(1, this->obj_editing_rating.first);
			break;
		case PM_OBJECTIVE_PARK_VALUE_TEXT:
			_str_params.SetMoney(1, this->obj_editing_park_value.first);
			break;
		case PM_OBJECTIVE_DROP_POLICY_TEXT:
			_str_params.SetNumberAndPlural(1, this->obj_editing_drop_policy.first);
			_str_params.SetText(1, DrawText(GUI_NR_DAYS));
			break;
		case PM_OBJECTIVE_DATE_TEXT:
			_str_params.SetDate(1, this->obj_editing_timeout.first.first);
			break;

		case PM_OBJECTIVE_TEXT: {
			std::string str;
			if (_game_observer.won_lost != SCENARIO_RUNNING) {
				str += DrawText(_game_observer.won_lost == SCENARIO_WON ? GUI_MESSAGE_SCENARIO_WON : GUI_MESSAGE_SCENARIO_LOST);
				str += "\n\n";
			}

			// NOCOM break this over multiple lines!
			str += _scenario.descr;
			str += "\n\n";

			str += _scenario.objective->ToString();
			_str_params.SetText(1, str);
			break;
		}

		default:
			break;
	}
}

void ParkManagementGui::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	switch (number) {
		case PM_TABBUTTON_GENERAL:
		case PM_TABBUTTON_GUESTS:
		case PM_TABBUTTON_RATING:
		case PM_TABBUTTON_OBJECTIVE:
		case PM_TABBUTTON_AWARDS:
			SelectTab(number);
			break;

		case PM_MAX_GUESTS_INCREASE:
			if (_game_mode_mgr.InEditorMode()) {
				_scenario.max_guests += MAX_GUESTS_STEP_SIZE;
				this->UpdateButtons();
			}
			break;
		case PM_MAX_GUESTS_DECREASE:
			if (_game_mode_mgr.InEditorMode()) {
				if (_scenario.max_guests > MIN_MAX_GUESTS + MAX_GUESTS_STEP_SIZE) {
					_scenario.max_guests -= MAX_GUESTS_STEP_SIZE;
				} else {
					_scenario.max_guests = MIN_MAX_GUESTS;
				}
				this->UpdateButtons();
			}
			break;

		case PM_ENTRANCE_FEE_INCREASE:
			_game_observer.entrance_fee += PARK_ENTRANCE_FEE_STEP_SIZE;
			this->UpdateButtons();
			break;
		case PM_ENTRANCE_FEE_DECREASE:
			_game_observer.entrance_fee = std::max<int>(0, _game_observer.entrance_fee - PARK_ENTRANCE_FEE_STEP_SIZE);
			this->UpdateButtons();
			break;

		case PM_ENTRANCE_FEE_ENABLE:
			if (_game_mode_mgr.InEditorMode()) {
				_scenario.allow_entrance_fee = !_scenario.allow_entrance_fee;
				if (!_scenario.allow_entrance_fee) _game_observer.entrance_fee = 0;
				this->UpdateButtons();
			}
			break;

		case PM_OBJECTIVE_GUESTS_ENABLE:
			obj_editing_guests.second = !obj_editing_guests.second;
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_RATING_ENABLE:
			obj_editing_rating.second = !obj_editing_rating.second;
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_PARK_VALUE_ENABLE:
			obj_editing_park_value.second = !obj_editing_park_value.second;
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_DROP_POLICY_ENABLE:
			obj_editing_drop_policy.second = !obj_editing_drop_policy.second;
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_DATE_ENABLE:
			obj_editing_timeout.second = !obj_editing_timeout.second;
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_GUESTS_INCREASE:
			if (obj_editing_guests.second) {
				obj_editing_guests.first += OBJECTIVE_GUESTS_STEP_SIZE;
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_GUESTS_DECREASE:
			if (obj_editing_guests.second) {
				obj_editing_guests.first = obj_editing_guests.first > OBJECTIVE_GUESTS_STEP_SIZE ? obj_editing_guests.first - OBJECTIVE_GUESTS_STEP_SIZE : 0;
			}
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_RATING_INCREASE:
			if (obj_editing_rating.second) {
				obj_editing_rating.first += OBJECTIVE_RATING_STEP_SIZE;
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_RATING_DECREASE:
			if (obj_editing_rating.second) {
				obj_editing_rating.first = obj_editing_rating.first > OBJECTIVE_RATING_STEP_SIZE ? obj_editing_rating.first - OBJECTIVE_RATING_STEP_SIZE : 0;
			}
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_PARK_VALUE_INCREASE:
			if (obj_editing_park_value.second) {
				obj_editing_park_value.first += OBJECTIVE_PARK_VALUE_STEP_SIZE;
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_PARK_VALUE_DECREASE:
			if (obj_editing_park_value.second) {
				obj_editing_park_value.first = obj_editing_park_value.first > OBJECTIVE_PARK_VALUE_STEP_SIZE ?
						obj_editing_park_value.first - OBJECTIVE_PARK_VALUE_STEP_SIZE : Money(0);
			}
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_DROP_POLICY_INCREASE:
			if (obj_editing_drop_policy.second) {
				obj_editing_drop_policy.first += OBJECTIVE_DROP_POLICY_STEP_SIZE;
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_DROP_POLICY_DECREASE:
			if (obj_editing_drop_policy.second) {
				obj_editing_drop_policy.first = obj_editing_drop_policy.first > OBJECTIVE_DROP_POLICY_STEP_SIZE ?
						obj_editing_drop_policy.first - OBJECTIVE_DROP_POLICY_STEP_SIZE : 0;
			}
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_DATE_INCREASE:
			if (obj_editing_timeout.second) {
				if (obj_editing_timeout.first.first.month < LAST_MONTH) {
					obj_editing_timeout.first.first.month++;
				} else {
					obj_editing_timeout.first.first.month = FIRST_MONTH;
					obj_editing_timeout.first.first.year++;
				}
				obj_editing_timeout.first.first.day = _days_per_month[obj_editing_timeout.first.first.month];
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_DATE_DECREASE:
			if (obj_editing_timeout.second && (obj_editing_timeout.first.first.year > 1 || obj_editing_timeout.first.first.month > FIRST_MONTH)) {
				if (obj_editing_timeout.first.first.month > FIRST_MONTH) {
					obj_editing_timeout.first.first.month--;
				} else {
					obj_editing_timeout.first.first.month = LAST_MONTH;
					obj_editing_timeout.first.first.year--;
				}
				obj_editing_timeout.first.first.day = _days_per_month[obj_editing_timeout.first.first.month];
			}
			this->UpdateButtons();
			break;
		case PM_OBJECTIVE_DATE_NOT_EXACT:
			if (obj_editing_timeout.second) {
				this->obj_editing_timeout.first.second = !this->obj_editing_timeout.first.second;
			}
			this->UpdateButtons();
			break;

		case PM_OBJECTIVE_APPLY: {
			std::vector<std::shared_ptr<AbstractObjective>> new_objectives;

			if (obj_editing_guests.second) {
				new_objectives.emplace_back(new ObjectiveGuests(0, obj_editing_guests.first));
			}
			if (obj_editing_park_value.second) {
				new_objectives.emplace_back(new ObjectiveParkValue(0, obj_editing_park_value.first));
			}
			if (obj_editing_rating.second) {
				new_objectives.emplace_back(new ObjectiveParkRating(obj_editing_drop_policy.second ? obj_editing_drop_policy.first : 0, obj_editing_rating.first));
			}

			if (new_objectives.empty()) {
				new_objectives.emplace_back(new ObjectiveNone);
			}

			_scenario.objective.reset(new ScenarioObjective(0,
					obj_editing_timeout.second ? obj_editing_timeout.first.second ? TIMEOUT_BEFORE : TIMEOUT_EXACT : TIMEOUT_NONE,
					obj_editing_timeout.first.first, new_objectives));
			this->UpdateButtons();
			break;
		}

		case PM_CLOSE_PARK_PANEL:
		case PM_CLOSE_PARK_LIGHT:
			_game_observer.SetParkOpen(false);
			this->UpdateButtons();
			break;
		case PM_OPEN_PARK_PANEL:
		case PM_OPEN_PARK_LIGHT:
			_game_observer.SetParkOpen(true);
			this->UpdateButtons();
			break;

		case PM_GOTO: {
			const XYZPoint16 coords = _world.GetParkEntrance();
			if (coords != XYZPoint16::invalid()) _window_manager.GetViewport()->view_pos = VoxelToPixel(coords);
			break;
		}

		default:
			break;
	}
}

/** Update all buttons of the window. */
void ParkManagementGui::UpdateButtons()
{
	/* Entrance fee controls. */
	this->SetWidgetShaded(PM_ENTRANCE_FEE_DECREASE, _game_observer.entrance_fee <= 0 || !_scenario.allow_entrance_fee);
	this->SetWidgetShaded(PM_ENTRANCE_FEE_INCREASE, !_scenario.allow_entrance_fee);

	LeafWidget *fee_enable = this->GetWidget<LeafWidget>(PM_ENTRANCE_FEE_ENABLE);
	fee_enable->SetVisible(this, _game_mode_mgr.InEditorMode());
	fee_enable->SetChecked(_scenario.allow_entrance_fee);
	fee_enable->SetPressed(_scenario.allow_entrance_fee);

	/* Park opening / closing. */
	LeafWidget *button_open = this->GetWidget<LeafWidget>(PM_OPEN_PARK_LIGHT);
	LeafWidget *button_close = this->GetWidget<LeafWidget>(PM_CLOSE_PARK_LIGHT);
	button_open->shift = _game_observer.park_open ? GS_LIGHT : GS_NIGHT;
	button_close->shift = !_game_observer.park_open ? GS_LIGHT : GS_NIGHT;
	button_open->SetShaded(_game_observer.won_lost == SCENARIO_LOST);
	button_close->SetShaded(_game_observer.won_lost == SCENARIO_LOST);

	/* Max guest controls. */
	this->GetWidget<LeafWidget>(PM_MAX_GUESTS_PANEL)->SetVisible(this, _game_mode_mgr.InEditorMode());
	this->SetWidgetShaded(PM_MAX_GUESTS_DECREASE, _scenario.max_guests <= MIN_MAX_GUESTS);

	/* Objective editing controls. */
	this->GetWidget<LeafWidget>(PM_OBJECTIVE_EDIT_PANEL)->SetVisible(this, _game_mode_mgr.InEditorMode());

	this->SetWidgetShaded(PM_OBJECTIVE_GUESTS_INCREASE, !this->obj_editing_guests.second);
	this->SetWidgetShaded(PM_OBJECTIVE_GUESTS_DECREASE, !this->obj_editing_guests.second || this->obj_editing_guests.first == 0);

	this->SetWidgetShaded(PM_OBJECTIVE_RATING_INCREASE, !this->obj_editing_rating.second);
	this->SetWidgetShaded(PM_OBJECTIVE_RATING_DECREASE, !this->obj_editing_rating.second || this->obj_editing_rating.first == 0);

	this->SetWidgetShaded(PM_OBJECTIVE_PARK_VALUE_INCREASE, !this->obj_editing_park_value.second);
	this->SetWidgetShaded(PM_OBJECTIVE_PARK_VALUE_DECREASE, !this->obj_editing_park_value.second || this->obj_editing_park_value.first <= 0);

	this->SetWidgetShaded(PM_OBJECTIVE_DROP_POLICY_INCREASE, !this->obj_editing_drop_policy.second);
	this->SetWidgetShaded(PM_OBJECTIVE_DROP_POLICY_DECREASE, !this->obj_editing_drop_policy.second || this->obj_editing_drop_policy.first == 0);

	this->SetWidgetShaded(PM_OBJECTIVE_DATE_INCREASE, !this->obj_editing_timeout.second);
	this->SetWidgetShaded(PM_OBJECTIVE_DATE_DECREASE, !this->obj_editing_timeout.second ||
			(this->obj_editing_timeout.first.first.year <= 1 && this->obj_editing_timeout.first.first.month <= FIRST_MONTH));

	this->SetWidgetChecked(PM_OBJECTIVE_GUESTS_ENABLE,      this->obj_editing_guests.second);
	this->SetWidgetChecked(PM_OBJECTIVE_RATING_ENABLE,      this->obj_editing_rating.second);
	this->SetWidgetChecked(PM_OBJECTIVE_PARK_VALUE_ENABLE,  this->obj_editing_park_value.second);
	this->SetWidgetChecked(PM_OBJECTIVE_DROP_POLICY_ENABLE, this->obj_editing_drop_policy.second);

	this->SetWidgetChecked(PM_OBJECTIVE_DATE_ENABLE,    this->obj_editing_timeout.second);
	this->SetWidgetChecked(PM_OBJECTIVE_DATE_NOT_EXACT, this->obj_editing_timeout.second && this->obj_editing_timeout.first.second);
}

void ParkManagementGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != PM_GUESTS_GRAPH && wid_num != PM_RATING_GRAPH) return GuiWindow::DrawWidget(wid_num, wid);

	static const int LABEL_H = GetTextHeight();
	static const int LABEL_W = 2 * LABEL_H;
	static const int SPACING = 2;

	const int MAX_VALUE = (wid_num == PM_GUESTS_GRAPH) ? std::max(_game_observer.max_guests, 1u) : MAX_PARK_RATING;
	uint32 graph_colour = _palette[TEXT_WHITE];
	const uint32 warning_colour = _palette[COL_RANGE_RED * COL_SERIES_LENGTH + COL_SERIES_START + COL_SERIES_LENGTH / 2];
	Rectangle32 pos = wid->pos;
	pos.base += this->rect.base;
	pos.base.x += LABEL_W + SPACING;
	pos.width  -= (LABEL_W + 3 * SPACING);
	pos.height -= LABEL_H;

	for (const auto &objective : _scenario.objective->objectives) {
		int line = 0;
		if (wid_num == PM_GUESTS_GRAPH) {
			const ObjectiveGuests *o = dynamic_cast<const ObjectiveGuests*>(objective.get());
			if (o != nullptr) line = o->nr_guests;
		} else {
			const ObjectiveParkRating *o = dynamic_cast<const ObjectiveParkRating*>(objective.get());
			if (o != nullptr) line = o->rating;
		}
		if (line > 0) {
			if (line < MAX_VALUE) {
				_video.DrawLine(
						Point16(pos.base.x + pos.width, pos.base.y + pos.height * (MAX_VALUE - line) / MAX_VALUE),
						Point16(pos.base.x            , pos.base.y + pos.height * (MAX_VALUE - line) / MAX_VALUE),
						warning_colour);
			} else {
				graph_colour = warning_colour;
			}
		}
	}

	_video.DrawLine(pos.base, Point16(pos.base.x, pos.base.y + pos.height), _palette[TEXT_WHITE]);
	_video.DrawLine(Point16(pos.base.x + pos.width, pos.base.y + pos.height), Point16(pos.base.x, pos.base.y + pos.height), _palette[TEXT_WHITE]);

	const double month_step = pos.width / (LAST_MONTH - FIRST_MONTH + 1.0);
	for (double x = pos.width - month_step * (_date.day - 1) / _days_per_month[_date.month]; x > 0; x -= month_step) {
		_video.DrawLine(Point16(pos.base.x + x, pos.base.y + pos.height + SPACING), Point16(pos.base.x + x, pos.base.y + pos.height - SPACING), _palette[TEXT_WHITE]);
	}

	_video.BlitText("0", _palette[TEXT_WHITE], pos.base.x - LABEL_W - SPACING, pos.base.y + pos.height - LABEL_H + SPACING, LABEL_W, ALG_RIGHT);
	_video.BlitText(std::to_string(MAX_VALUE), _palette[TEXT_WHITE], pos.base.x - LABEL_W - SPACING, pos.base.y - SPACING, LABEL_W, ALG_RIGHT);

	_str_params.SetDate(1, _date);
	DrawString(STR_ARG1, TEXT_WHITE, pos.base.x + pos.width / 2, pos.base.y + pos.height + SPACING, pos.width / 2, ALG_RIGHT);
	_str_params.SetDate(1, Date(_date.day, _date.month, _date.year - 1));
	DrawString(STR_ARG1, TEXT_WHITE, pos.base.x                , pos.base.y + pos.height + SPACING, pos.width / 2, ALG_LEFT);

	const auto &history = (wid_num == PM_GUESTS_GRAPH) ? _game_observer.guest_count_history : _game_observer.park_rating_history;
	if (history.empty()) return;

	auto it = history.begin();
	const double STEP_X = static_cast<double>(pos.width) / STATISTICS_HISTORY;
	const double STEP_Y = static_cast<double>(pos.height) / MAX_VALUE;
	double x = pos.base.x + pos.width;
	double y = pos.base.y + pos.height - (*it * STEP_Y);

	_video.FillRectangle(Rectangle32(x - SPACING, y - SPACING, 2 * SPACING, 2 * SPACING), graph_colour);
	++it;
	for (; it != history.end(); ++it) {
		double nx = x - STEP_X;
		double ny = pos.base.y + pos.height - (*it * STEP_Y);
		_video.DrawLine(Point16(x, y), Point16(nx, ny), graph_colour);
		x = nx;
		y = ny;
	}
}

/**
 * Open the park management GUI.
 * @param tab Tab to show. Ignored if the GUI was already open.
 * @ingroup gui_group
 */
void ShowParkManagementGui(ParkManagementGuiTabs tab)
{
	if (HighlightWindowByType(WC_PARK_MANAGEMENT, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new ParkManagementGui(tab);
}
