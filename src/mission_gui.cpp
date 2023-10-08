/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file mission_gui.cpp Gui for selecting the scenario to start. */

#include "stdafx.h"
#include "window.h"
#include "gamecontrol.h"
#include "gamelevel.h"

/**
  * GUI for selecting a scenario to play.
  * @ingroup gui_group
  */
class ScenarioSelectGui : public GuiWindow {
public:
	ScenarioSelectGui();

	void OnDraw(MouseModeSelector *selector) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void SetTooltipStringParameters(BaseWidget *tooltip_widget) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;

	MissionScenario *GetScenario(WidgetNumber wid_num) const;
	bool SelectTab(WidgetNumber wid_num);

private:
	std::vector<WidgetNumber> tab_bar;  ///< Widget indices of the tab bar.
	uint32 current_mission;             ///< Currently selected mission index.
};

static const int SSW_NR_ROWS = 5;       ///< Number of scenario rows in the scenario select window.
static const int BUTTON_WIDTH = 400;    ///< Pixel width of a scenario row.
static const int BUTTON_HEIGHT = 65;    ///< Pixel height of a scenario row.
static const int SCENARIO_PADDING = 2;  ///< Padding inside a scenario row.

/**
 * Widget numbers of the scenario select GUI.
 * The widget indices for the scenario rows are 1..#SSW_NR_ROWS.
 * @ingroup gui_group
 */
enum ScenarioSelectWidgets {
	SSW_SCROLLBAR = 2 * SSW_NR_ROWS,  ///< Widget index of the scrollbar.
	SSW_MAIN_PANEL,                   ///< Widget index of the main panel.
	SSW_END                           ///< Last hardcoded widget index.
};

#define SCENARIO_ROW_BUTTON(index) \
	Widget(WT_EMPTY, index, COL_RANGE_BLUE), SetData(STR_ARG1, STR_ARG1), SetResize(0, BUTTON_HEIGHT), SetMinimalSize(BUTTON_WIDTH, BUTTON_HEIGHT) \

ScenarioSelectGui::ScenarioSelectGui() : GuiWindow(WC_SCENARIO_SELECT, ALL_WINDOWS_OF_TYPE)
{
	WidgetNumber last_widnum = SSW_END;

	std::vector<WidgetPart> scenario_select_gui_parts = {
		Intermediate(0, 1),
			Intermediate(1, 0),
				Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_SCENARIO_SELECT_TITLE, GUI_TITLEBAR_TIP),
				Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			EndContainer(),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
				Intermediate(2, 1),
					Intermediate(1, 0),
						Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
	};

	for (const auto &mission : _missions) {
		WidgetNumber widnum = ++last_widnum;
		this->tab_bar.push_back(widnum);

		scenario_select_gui_parts.insert(scenario_select_gui_parts.end(), {
						Widget(WT_TEXT_TAB, widnum, COL_RANGE_BLUE),
							SetData(mission->name, mission->descr)
		});
	}

	scenario_select_gui_parts.insert(scenario_select_gui_parts.end(), {
						Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetFill(1, 1), SetResize(1, 1),
					EndContainer(),
					Intermediate(1, 2),
						Widget(WT_PANEL, SSW_MAIN_PANEL, COL_RANGE_BLUE),
							Intermediate(SSW_NR_ROWS, 1),
								SCENARIO_ROW_BUTTON(1),
								SCENARIO_ROW_BUTTON(2),
								SCENARIO_ROW_BUTTON(3),
								SCENARIO_ROW_BUTTON(4),
								SCENARIO_ROW_BUTTON(5),
				Widget(WT_VERT_SCROLLBAR, SSW_SCROLLBAR, COL_RANGE_BLUE),
		EndContainer(),
	});

	this->SetupWidgetTree(scenario_select_gui_parts.data(), scenario_select_gui_parts.size());
	this->SetScrolledWidget(SSW_MAIN_PANEL, SSW_SCROLLBAR);

	this->SelectTab(this->tab_bar.front());
}

/**
 * Change the active tab.
 * @param wid_num ID of the tab button widget.
 * @return A tab was selected.
 */
bool ScenarioSelectGui::SelectTab(WidgetNumber wid_num)
{
	for (uint i = 0; i < tab_bar.size(); ++i) {
		if (tab_bar.at(i) == wid_num) {
			this->current_mission = i;
			return true;
		}
	}
	return false;
}

/**
 * The scenario which is represented by the indicated scenario row.
 * @param wid_num Scenario row ID.
 * @return The scenario, or \c nullptr if there is no scenario here.
 */
MissionScenario *ScenarioSelectGui::GetScenario(const WidgetNumber wid_num) const
{
	if (wid_num < 1 || wid_num > SSW_NR_ROWS) return nullptr;

	const ScrollbarWidget *scrollbar = this->GetWidget<ScrollbarWidget>(SSW_SCROLLBAR);
	auto &all_scenarios = _missions.at(this->current_mission)->scenarios;
	const int nr_scenarios = all_scenarios.size();
	const int scenario_index = wid_num + scrollbar->GetStart() - 1;

	if (scenario_index < 0 || scenario_index >= nr_scenarios) return nullptr;
	return &all_scenarios.at(scenario_index);
}

void ScenarioSelectGui::OnClick(const WidgetNumber wid_num, const Point16 &pos)
{
	MissionScenario *scenario = this->GetScenario(wid_num);
	if (scenario == nullptr) {
		if (this->SelectTab(wid_num)) return;
		return GuiWindow::OnClick(wid_num, pos);
	}

	if (scenario->required_to_unlock == 0) {
		_game_control.NewGame(scenario);  // Also deletes this window.
	} else {
		ShowErrorMessage(scenario->name, GUI_SCENARIO_SELECT_UNLOCK, [scenario]() {
			_str_params.SetNumberAndPlural(1, scenario->required_to_unlock);
		});
	}
}

void ScenarioSelectGui::SetTooltipStringParameters(BaseWidget *tooltip_widget) const
{
	MissionScenario *scenario = this->GetScenario(tooltip_widget->number);
	if (scenario != nullptr) {
		if (scenario->required_to_unlock == 0) {
			_str_params.SetStrID(1, scenario->descr);
		} else {
			_str_params.SetNumberAndPlural(1, scenario->required_to_unlock);
			std::string tooltip = DrawText(GUI_SCENARIO_SELECT_UNLOCK);
			_str_params.SetText(1, tooltip);
		}
	} else {
		GuiWindow::SetTooltipStringParameters(tooltip_widget);
	}
}

void ScenarioSelectGui::OnDraw(MouseModeSelector *s)
{
	this->GetWidget<ScrollbarWidget>(SSW_SCROLLBAR)->SetItemCount(_missions.at(this->current_mission)->scenarios.size());
	GuiWindow::OnDraw(s);
}

void ScenarioSelectGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	MissionScenario *scenario = this->GetScenario(wid_num);
	if (scenario == nullptr) return GuiWindow::DrawWidget(wid_num, wid);

	const int x = this->GetWidgetScreenX(wid) + SCENARIO_PADDING;
	const int y = this->GetWidgetScreenY(wid) + SCENARIO_PADDING;
	const int w = wid->pos.width - 2 * SCENARIO_PADDING;
	const int h = wid->pos.height - 2 * SCENARIO_PADDING;
	Rectangle32 rect(x, y, w, h);

	_video.FillRectangle(rect, 0x7f);
	_video.DrawRectangle(rect, 0xff);
	DrawString(scenario->name, scenario->required_to_unlock == 0 ? TEXT_WHITE : TEXT_GREY, x, y + SCENARIO_PADDING, w, ALG_CENTER, true);

	if (scenario->solved.has_value()) {
		char buffer[256];
		std::strftime(buffer, sizeof(buffer), _language.GetSgText(GUI_DATETIME_FORMAT).c_str(), std::localtime(&scenario->solved->timestamp));

		_str_params.SetText(1, scenario->solved->user);
		_str_params.SetText(2, buffer);
		_str_params.SetMoney(3, scenario->solved->company_value);

		const int off = GetTextHeight() + SCENARIO_PADDING;
		DrawMultilineString(GUI_SCENARIO_SELECT_SOLVED, x, y + off, w, h - off, TEXT_WHITE, ALG_CENTER);
	}
}

/**
 * Open the GUI to select a scenario.
 * @ingroup gui_group
 */
void ShowScenarioSelectGui()
{
	if (HighlightWindowByType(WC_SCENARIO_SELECT, ALL_WINDOWS_OF_TYPE) != nullptr) return;

	if (_missions.empty()) {
		ShowErrorMessage(GUI_SCENARIO_SELECT_NO_MISSIONS, STR_NULL, [](){}, 0);
	} else {
		new ScenarioSelectGui();
	}
}
