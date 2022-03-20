/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file coaster_gui.cpp Roller coaster windows. */

#include "stdafx.h"
#include "math_func.h"
#include "memory.h"
#include "window.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "coaster.h"
#include "viewport.h"
#include "map.h"
#include "gui_sprites.h"
#include "entity_gui.h"

/** Window to prompt for removing a roller coaster. */
class CoasterRemoveWindow : public EntityRemoveWindow  {
public:
	CoasterRemoveWindow(CoasterInstance *instance);

	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	CoasterInstance *ci; ///< Roller coaster instance to remove.
};

/**
 * Constructor of the roller coaster remove window.
 * @param ci Roller coaster instance to remove.
 */
CoasterRemoveWindow::CoasterRemoveWindow(CoasterInstance *instance) : EntityRemoveWindow(WC_COASTER_REMOVE, instance->GetIndex()), ci(instance)
{
}

void CoasterRemoveWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	if (number == ERW_YES) {
		delete GetWindowByType(WC_COASTER_MANAGER, this->ci->GetIndex());
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
	delete this;
}

void CoasterRemoveWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	if (wid_num == ERW_MESSAGE) _str_params.SetText(1, this->ci->name);
}

/**
 * Open a coaster remove window for the given roller coaster.
 * @param ci Coaster instance to remove.
 */
void ShowCoasterRemove(CoasterInstance *ci)
{
	if (HighlightWindowByType(WC_COASTER_REMOVE, ci->GetIndex()) != nullptr) return;

	new CoasterRemoveWindow(ci);
}

/** Widget numbers of the roller coaster instance window. */
enum CoasterInstanceWidgets {
	CIW_TITLEBAR,            ///< Titlebar widget.
	CIW_REMOVE,              ///< Remove button widget.
	CIW_EDIT,                ///< Edit coaster widget.
	CIW_CLOSE,               ///< Close coaster widget.
	CIW_TEST,                ///< Test coaster widget.
	CIW_OPEN,                ///< Open coaster widget.
	CIW_ENTRANCE_FEE,           ///< Entrance fee display.
	CIW_ENTRANCE_FEE_DECREASE,  ///< Decrease entrance fee button.
	CIW_ENTRANCE_FEE_INCREASE,  ///< Increase entrance fee button.
	CIW_RELIABILITY,            ///< Reliability/breakdown display.
	CIW_MAINTENANCE,            ///< Maintenance interval display.
	CIW_MAINTENANCE_DECREASE,   ///< Decrease maintenance interval button.
	CIW_MAINTENANCE_INCREASE,   ///< Increase maintenance interval button.
	CIW_MIN_IDLE,               ///< Minimum idle time display.
	CIW_MIN_IDLE_DECREASE,      ///< Decrease minimum idle time button.
	CIW_MIN_IDLE_INCREASE,      ///< Increase minimum idle time button.
	CIW_MAX_IDLE,               ///< Maximum idle time display.
	CIW_MAX_IDLE_DECREASE,      ///< Decrease maximum idle time button.
	CIW_MAX_IDLE_INCREASE,      ///< Increase maximum idle time button.
	CIW_PLACE_ENTRANCE,      ///< Entrance placement.
	CIW_CHOOSE_ENTRANCE,     ///< Entrance style.
	CIW_PLACE_EXIT,          ///< Exit placement.
	CIW_CHOOSE_EXIT,         ///< Exit style.
	CIW_ENTRANCE_RECOLOUR1,  ///< First entrance colour.
	CIW_ENTRANCE_RECOLOUR2,  ///< Second entrance colour.
	CIW_ENTRANCE_RECOLOUR3,  ///< Third entrance colour.
	CIW_EXIT_RECOLOUR1,      ///< First exit colour.
	CIW_EXIT_RECOLOUR2,      ///< Second exit colour.
	CIW_EXIT_RECOLOUR3,      ///< Third exit colour.
	CIW_NUMBER_TRAINS,       ///< Number of trains dropdown.
	CIW_NUMBER_CARS,         ///< Number of cars dropdown.
	CIW_MONTHLY_COST,        ///< Monthly operating cost.
	CIW_EXCITEMENT_RATING,   ///< Ride's excitement rating.
	CIW_INTENSITY_RATING,    ///< Ride's intensity rating.
	CIW_NAUSEA_RATING,       ///< Ride's nausea rating.
	CIW_GRAPH,               ///< Speed/forces graph.
	CIW_GRAPH_SPEED,         ///< Speed graph tab.
	CIW_GRAPH_VG,            ///< Vertical G forces graph tab.
	CIW_GRAPH_HG,            ///< Horizontal G forces graph tab.
};

static const int GRAPH_HEIGHT = 100;  ///< Height of the graph display in pixels.

/** Widget parts of the #CoasterInstanceWindow. */
static const WidgetPart _coaster_instance_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, CIW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(4, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_MONTHLY_COST_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, CIW_MONTHLY_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_EXCITEMENT, STR_NULL),
				Widget(WT_RIGHT_TEXT, CIW_EXCITEMENT_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_INTENSITY, STR_NULL),
				Widget(WT_RIGHT_TEXT, CIW_INTENSITY_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_NAUSEA, STR_NULL),
				Widget(WT_RIGHT_TEXT, CIW_NAUSEA_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 1),
				Intermediate(4, 4),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_ENTRANCE_FEE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_ENTRANCE_FEE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, CIW_ENTRANCE_FEE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_ENTRANCE_FEE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MIN_IDLE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MIN_IDLE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, CIW_MIN_IDLE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MIN_IDLE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MAX_IDLE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MAX_IDLE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, CIW_MAX_IDLE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MAX_IDLE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MAINTENANCE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MAINTENANCE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, CIW_MAINTENANCE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, CIW_MAINTENANCE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
				Widget(WT_LEFT_TEXT, CIW_RELIABILITY, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(1, 3),
				Intermediate(2, 1),
					Intermediate(3, 2),
						Widget(WT_TEXT_PUSHBUTTON, CIW_PLACE_ENTRANCE, COL_RANGE_DARK_RED),
								SetData(GUI_PLACE_ENTRANCE, GUI_PLACE_ENTRANCE_TOOLTIP),
						Widget(WT_TEXT_PUSHBUTTON, CIW_PLACE_EXIT, COL_RANGE_DARK_RED),
								SetData(GUI_PLACE_EXIT, GUI_PLACE_EXIT_TOOLTIP),
						Widget(WT_DROPDOWN_BUTTON, CIW_CHOOSE_ENTRANCE, COL_RANGE_DARK_RED),
								SetData(GUI_CHOOSE_ENTRANCE, GUI_CHOOSE_ENTRANCE_TOOLTIP),
						Widget(WT_DROPDOWN_BUTTON, CIW_CHOOSE_EXIT, COL_RANGE_DARK_RED),
								SetData(GUI_CHOOSE_EXIT, GUI_CHOOSE_EXIT_TOOLTIP),
						Intermediate(1, 3),
							Widget(WT_DROPDOWN_BUTTON, CIW_ENTRANCE_RECOLOUR1, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, CIW_ENTRANCE_RECOLOUR2, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, CIW_ENTRANCE_RECOLOUR3, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							SetResize(1, 0),
						Intermediate(1, 3),
							Widget(WT_DROPDOWN_BUTTON, CIW_EXIT_RECOLOUR1, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, CIW_EXIT_RECOLOUR2, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, CIW_EXIT_RECOLOUR3, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							SetResize(1, 0),
					Intermediate(1, 2),
						Widget(WT_DROPDOWN_BUTTON, CIW_NUMBER_CARS, COL_RANGE_DARK_RED), SetData(GUI_COASTER_MANAGER_NUMBER_CARS, STR_NULL),
						Widget(WT_DROPDOWN_BUTTON, CIW_NUMBER_TRAINS, COL_RANGE_DARK_RED), SetData(GUI_COASTER_MANAGER_NUMBER_TRAINS, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
					Intermediate(0, 1),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
						Widget(WT_RADIOBUTTON, CIW_CLOSE, COL_RANGE_RED), SetPadding(0, 2, 0, 0),
						Widget(WT_RADIOBUTTON, CIW_TEST, COL_RANGE_YELLOW), SetPadding(0, 2, 0, 0),
						Widget(WT_RADIOBUTTON, CIW_OPEN, COL_RANGE_GREEN), SetPadding(0, 2, 0, 0),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
					EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 1),
				Intermediate(1, 0),
					Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
					Widget(WT_TEXT_TAB, CIW_GRAPH_SPEED, COL_RANGE_DARK_RED),
							SetData(GUI_COASTER_MANAGER_GRAPH_SPEED, GUI_COASTER_MANAGER_GRAPH_TOOLTIP_SPEED),
					Widget(WT_TEXT_TAB, CIW_GRAPH_VG, COL_RANGE_DARK_RED),
							SetData(GUI_COASTER_MANAGER_GRAPH_VERT_G, GUI_COASTER_MANAGER_GRAPH_TOOLTIP_VERT_G),
					Widget(WT_TEXT_TAB, CIW_GRAPH_HG, COL_RANGE_DARK_RED),
							SetData(GUI_COASTER_MANAGER_GRAPH_HORZ_G, GUI_COASTER_MANAGER_GRAPH_TOOLTIP_HORZ_G),
					Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 1), SetResize(1, 1),
				EndContainer(),
				Widget(WT_TAB_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
					Widget(WT_EMPTY, CIW_GRAPH, COL_RANGE_DARK_RED), SetMinimalSize(GRAPH_HEIGHT, GRAPH_HEIGHT), SetFill(1, 1), SetResize(1, 1),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(1, 2), SetEqualSize(true, true),
				Widget(WT_TEXT_PUSHBUTTON, CIW_EDIT, COL_RANGE_DARK_RED), SetData(GUI_COASTER_MANAGER_EDIT, STR_NULL), SetFill(1, 1),
				Widget(WT_TEXT_PUSHBUTTON, CIW_REMOVE, COL_RANGE_DARK_RED), SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
	EndContainer(),
};

/** What kind of data to display in the graph. */
enum GraphMode {
	GM_SPEED,   ///< Display speed graph.
	GM_VERT_G,  ///< Display vertical G forces graph.
	GM_HORZ_G,  ///< Display horizontal G forces graph.
};

/** Window to display and setup a roller coaster. */
class CoasterInstanceWindow : public GuiWindow {
public:
	CoasterInstanceWindow(CoasterInstance *instance);
	~CoasterInstanceWindow();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

	void SetGraphMode(GraphMode mode);
	void SetCoasterState();
	void SetRadioChecked(WidgetNumber radio, bool checked);
	void UpdateRecolourButtons();

private:
	CoasterInstance *ci;             ///< Roller coaster instance to display and control.
	mutable char text_buffer[1024];  ///< Buffer for custom strings.

	void ChooseEntranceExitClicked(bool entrance);
	RideMouseMode entrance_exit_placement;
	bool is_placing_entrance;
	GraphMode graph_mode;
};

/**
 * Constructor of the roller coaster instance window.
 * @param ci Roller coaster instance to display and control.
 */
CoasterInstanceWindow::CoasterInstanceWindow(CoasterInstance *instance) : GuiWindow(WC_COASTER_MANAGER, instance->GetIndex()), ci(instance)
{
	this->SetupWidgetTree(_coaster_instance_gui_parts, lengthof(_coaster_instance_gui_parts));
	this->SetCoasterState();
	this->UpdateRecolourButtons();
	this->SetGraphMode(GM_SPEED);

	entrance_exit_placement.cur_cursor = CUR_TYPE_INVALID;
	/* When opening the window of a newly built ride immediately prompt the user to place the entrance or exit. */
	if (this->ci->NeedsEntrance()) {
		ChooseEntranceExitClicked(true);
	} else if (this->ci->NeedsExit()) {
		ChooseEntranceExitClicked(false);
	}
}

CoasterInstanceWindow::~CoasterInstanceWindow()
{
	SetSelector(nullptr);
	if (!GetWindowByType(WC_COASTER_BUILD, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

assert_compile(MAX_RECOLOUR >= MAX_RIDE_RECOLOURS); ///< Check that the 3 recolourings of a gentle/thrill ride fit in the Recolouring::entries array.

/** Update all recolour buttons of the window. */
void CoasterInstanceWindow::UpdateRecolourButtons()
{
	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->ci->entrance_recolours.entries[i];
		this->GetWidget<LeafWidget>(CIW_ENTRANCE_RECOLOUR1 + i)->SetShaded(!re.IsValid());
	}
	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->ci->exit_recolours.entries[i];
		this->GetWidget<LeafWidget>(CIW_EXIT_RECOLOUR1 + i)->SetShaded(!re.IsValid());
	}
	ResetSize();
}

/**
 * Sets what kind of data to display in the graph.
 * @param mode Type of data to show.
 */
void CoasterInstanceWindow::SetGraphMode(const GraphMode mode)
{
	this->graph_mode = mode;
	this->SetWidgetPressed(CIW_GRAPH_SPEED, mode == GM_SPEED);
	this->SetWidgetPressed(CIW_GRAPH_VG, mode == GM_VERT_G);
	this->SetWidgetPressed(CIW_GRAPH_HG, mode == GM_HORZ_G);
}

static const int GRAPH_NR_Y_AXIS_LABELS = 5;  ///< Number of labels on the graph's vertical axis.
static const int GRAPH_LABELS_WIDTH = 30;     ///< Width reserved for the graph's labels.

void CoasterInstanceWindow::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != CIW_GRAPH) return GuiWindow::DrawWidget(wid_num, wid);

	Rectangle32 pos = wid->pos;
	pos.base += this->rect.base;

	bool has_valid_datapoint = false;
	std::set<Point32> datapoints;
	int32 ymax = 1, ymin = 0;
	for (const auto &pair : this->ci->intensity_statistics) {
		int32 y;
		switch (this->graph_mode) {
			case GM_SPEED:  y = pair.second.speed;        break;
			case GM_VERT_G: y = pair.second.vertical_g;   break;
			case GM_HORZ_G: y = pair.second.horizontal_g; break;
			default: NOT_REACHED();
		}
		ymax = std::max(y, ymax);
		ymin = std::min(y, ymin);
		datapoints.insert(Point32((pos.width - GRAPH_LABELS_WIDTH) * pair.first * COASTER_INTENSITY_STATISTICS_SAMPLING_PRECISION / this->ci->coaster_length, y));
		has_valid_datapoint |= pair.second.valid;
	}

	if (!has_valid_datapoint) {
		DrawString(GUI_COASTER_MANAGER_NO_GRAPHS_YET, TEXT_WHITE, pos.base.x, pos.base.y + (pos.height - GetTextHeight()) / 2, pos.width, ALG_CENTER);
		return;
	}
	if (ymax - ymin < GRAPH_NR_Y_AXIS_LABELS) {
		int delta = GRAPH_NR_Y_AXIS_LABELS - ymax + ymin - 1;
		if (this->graph_mode == GM_SPEED) {
			ymax += delta;
		} else {
			ymax += delta / 2;
			delta -= delta / 2;
			ymin -= delta;
		}
	}

	_video.FillRectangle(pos, 0xff);
	for (int i = 2; i < GRAPH_NR_Y_AXIS_LABELS; i++) {
		const int y = pos.base.y + pos.height * (GRAPH_NR_Y_AXIS_LABELS - i) / (GRAPH_NR_Y_AXIS_LABELS - 1);
		_video.DrawLine(Point32(pos.base.x, y), Point32(pos.base.x + pos.width - GRAPH_LABELS_WIDTH, y), _palette[TEXT_GREY]);
	}

	std::set<Point16> scaled_datapoints;
	for (const Point32 &p : datapoints) scaled_datapoints.insert(Point32(p.x + pos.base.x, pos.base.y + pos.height - pos.height * (p.y - ymin) / (ymax - ymin)));
	auto p1 = scaled_datapoints.begin();
	auto p2 = p1;
	p2++;
	for (; p2 != scaled_datapoints.end(); p1++, p2++) _video.DrawLine(*p1, *p2, _palette[TEXT_WHITE]);

	for (int i = 0; i < GRAPH_NR_Y_AXIS_LABELS; i++) {
		const int val = (ymax - ymin) * i / (GRAPH_NR_Y_AXIS_LABELS - 1) + ymin;
		const int y = pos.base.y + pos.height * (GRAPH_NR_Y_AXIS_LABELS - i - 1) / GRAPH_NR_Y_AXIS_LABELS;
		_str_params.SetNumber(1, val);
		DrawString(STR_ARG1, TEXT_GREY, pos.base.x, y, pos.width, ALG_RIGHT);
	}
}

void CoasterInstanceWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CIW_TITLEBAR:
			_str_params.SetText(1, this->ci->name);
			break;

		case CIW_MONTHLY_COST:
			_str_params.SetMoney(1, this->ci->GetRideType()->monthly_cost + (this->ci->state == RIS_OPEN ? this->ci->GetRideType()->monthly_open_cost : Money(0)));
			break;
		case CIW_EXCITEMENT_RATING:
			SetRideRatingStringParam(this->ci->excitement_rating);
			break;
		case CIW_INTENSITY_RATING:
			SetRideRatingStringParam(this->ci->intensity_rating);
			break;
		case CIW_NAUSEA_RATING:
			SetRideRatingStringParam(this->ci->nausea_rating);
			break;

		case CIW_ENTRANCE_FEE:
			_str_params.SetMoney(1, this->ci->item_price[0]);
			break;
		case CIW_MAX_IDLE:
			_str_params.SetNumber(1, this->ci->max_idle_duration / 1000);
			break;
		case CIW_MIN_IDLE:
			_str_params.SetNumber(1, this->ci->min_idle_duration / 1000);
			break;

		case CIW_MAINTENANCE:
			if (this->ci->maintenance_interval > 0) {
				_str_params.SetNumber(1, this->ci->maintenance_interval / (60 * 1000));
			} else {
				_str_params.SetStrID(1, GUI_RIDE_MANAGER_MAINTENANCE_NEVER);
			}
			break;
		case CIW_RELIABILITY:
			if (this->ci->broken) {
				_str_params.SetStrID(1, GUI_RIDE_MANAGER_BROKEN_DOWN);
			} else {
				snprintf(text_buffer, lengthof(text_buffer), _language.GetText(GUI_RIDE_MANAGER_RELIABILITY).c_str(),
						this->ci->reliability / 100.0);
				_str_params.SetText(1, text_buffer);
			}
			break;

		case CIW_ENTRANCE_RECOLOUR1:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ci->entrance_type]->recolour_description_1);
			break;
		case CIW_ENTRANCE_RECOLOUR2:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ci->entrance_type]->recolour_description_2);
			break;
		case CIW_ENTRANCE_RECOLOUR3:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ci->entrance_type]->recolour_description_3);
			break;
		case CIW_EXIT_RECOLOUR1:
			_str_params.SetStrID(1, _rides_manager.exits[this->ci->exit_type]->recolour_description_1);
			break;
		case CIW_EXIT_RECOLOUR2:
			_str_params.SetStrID(1, _rides_manager.exits[this->ci->exit_type]->recolour_description_2);
			break;
		case CIW_EXIT_RECOLOUR3:
			_str_params.SetStrID(1, _rides_manager.exits[this->ci->exit_type]->recolour_description_3);
			break;

		case CIW_NUMBER_TRAINS:
			_str_params.SetNumber(1, this->ci->number_of_trains);
			break;
		case CIW_NUMBER_CARS:
			_str_params.SetNumber(1, this->ci->cars_per_train);
			break;
	}
}

void CoasterInstanceWindow::OnClick(WidgetNumber widget, [[maybe_unused]] const Point16 &pos)
{
	switch (widget) {
		case CIW_REMOVE:
			ShowCoasterRemove(this->ci);
			break;

		case CIW_EDIT:
			this->ci->CloseRide();
			ShowCoasterBuildGui(this->ci);
			delete this;  // The user must not change ride settings while the coaster is under construction.
			break;

		case CIW_MAINTENANCE_INCREASE:
			this->ci->maintenance_interval += MAINTENANCE_INTERVAL_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_MAINTENANCE_DECREASE:
			this->ci->maintenance_interval -= MAINTENANCE_INTERVAL_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_ENTRANCE_FEE_INCREASE:
			this->ci->item_price[0] += ENTRANCE_FEE_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_ENTRANCE_FEE_DECREASE:
			this->ci->item_price[0] = std::max<int>(0, this->ci->item_price[0] - ENTRANCE_FEE_STEP_SIZE);
			this->SetCoasterState();
			break;
		case CIW_MAX_IDLE_INCREASE:
			this->ci->max_idle_duration += IDLE_DURATION_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_MAX_IDLE_DECREASE:
			this->ci->max_idle_duration -= IDLE_DURATION_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_MIN_IDLE_INCREASE:
			this->ci->min_idle_duration += IDLE_DURATION_STEP_SIZE;
			this->SetCoasterState();
			break;
		case CIW_MIN_IDLE_DECREASE:
			this->ci->min_idle_duration -= IDLE_DURATION_STEP_SIZE;
			this->SetCoasterState();
			break;

		case CIW_CLOSE:
			this->ci->CloseRide();
			this->SetCoasterState();
			break;
		case CIW_TEST:
			this->ci->TestRide();
			this->SetCoasterState();
			break;
		case CIW_OPEN:
			if (this->ci->CanOpenRide()) this->ci->OpenRide();
			this->SetCoasterState();
			break;

		case CIW_ENTRANCE_RECOLOUR1:
		case CIW_ENTRANCE_RECOLOUR2:
		case CIW_ENTRANCE_RECOLOUR3: {
			RecolourEntry *re = &this->ci->entrance_recolours.entries[widget - CIW_ENTRANCE_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(widget, re, COL_RANGE_DARK_RED);
			}
			break;
		}
		case CIW_EXIT_RECOLOUR1:
		case CIW_EXIT_RECOLOUR2:
		case CIW_EXIT_RECOLOUR3: {
			RecolourEntry *re = &this->ci->exit_recolours.entries[widget - CIW_EXIT_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(widget, re, COL_RANGE_DARK_RED);
			}
			break;
		}

		case CIW_PLACE_ENTRANCE:
			ChooseEntranceExitClicked(true);
			break;
		case CIW_PLACE_EXIT:
			ChooseEntranceExitClicked(false);
			break;

		case CIW_CHOOSE_ENTRANCE: {
			DropdownList itemlist;
			for (const auto &eetype : _rides_manager.entrances) {
				_str_params.SetText(1, _language.GetText(eetype->name));
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(widget, itemlist, this->ci->entrance_type, COL_RANGE_DARK_RED);
			break;
		}
		case CIW_CHOOSE_EXIT: {
			DropdownList itemlist;
			for (const auto &eetype : _rides_manager.exits) {
				_str_params.SetText(1, _language.GetText(eetype->name));
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(widget, itemlist, this->ci->exit_type, COL_RANGE_DARK_RED);
			break;
		}

		case CIW_GRAPH_SPEED:
			this->SetGraphMode(GM_SPEED);
			break;
		case CIW_GRAPH_VG:
			this->SetGraphMode(GM_VERT_G);
			break;
		case CIW_GRAPH_HG:
			this->SetGraphMode(GM_HORZ_G);
			break;

		case CIW_NUMBER_TRAINS: {
			DropdownList itemlist;
			const int max = this->ci->GetMaxNumberOfTrains(this->ci->cars_per_train);
			for (int i = 1; i <= max; i++) {
				_str_params.SetNumber(1, i);
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(widget, itemlist, this->ci->number_of_trains - 1, COL_RANGE_DARK_RED);
			break;
		}
		case CIW_NUMBER_CARS: {
			DropdownList itemlist;
			const int max = this->ci->GetMaxNumberOfCars();
			for (int i = 1; i <= max; i++) {
				_str_params.SetNumber(1, i);
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(widget, itemlist, this->ci->cars_per_train - 1, COL_RANGE_DARK_RED);
			break;
		}
	}
}

/**
 * Called when the Choose Entrance or Choose Exit button was clicked.
 * @param entrance Entrance or exit button.
 */
void CoasterInstanceWindow::ChooseEntranceExitClicked(const bool entrance)
{
	this->ci->temp_entrance_pos = XYZPoint16::invalid();
	this->ci->temp_exit_pos = XYZPoint16::invalid();

	if (this->selector == nullptr || (this->is_placing_entrance != entrance)) {
		this->is_placing_entrance = entrance;
		SetSelector(&entrance_exit_placement);
	} else {
		SetSelector(nullptr);
	}

	entrance_exit_placement.SetSize(0, 0);
	entrance_exit_placement.MarkDirty();
}

/**
 * Update the coaster instance associated with this window to a new state that is determined by the 
 * radio button clicked. If the radio button is not one of the ride state control radio buttons in this
 * window, an error will be raised.
 *
 * @param radio A radio button that was clicked by the user.
 */
void CoasterInstanceWindow::SetCoasterState()
{
	this->SetRadioChecked(CIW_CLOSE, this->ci->state == RIS_CLOSED);
	this->SetRadioChecked(CIW_TEST, this->ci->state == RIS_TESTING);
	this->SetRadioChecked(CIW_OPEN, this->ci->state == RIS_OPEN);

	this->SetWidgetShaded(CIW_EDIT, this->ci->state != RIS_CLOSED);
	this->SetWidgetShaded(CIW_OPEN, !this->ci->CanOpenRide());
	this->SetWidgetShaded(CIW_NUMBER_CARS, this->ci->state != RIS_CLOSED);
	this->SetWidgetShaded(CIW_NUMBER_TRAINS, this->ci->state != RIS_CLOSED);
	this->SetWidgetShaded(CIW_MAINTENANCE_DECREASE, this->ci->maintenance_interval <= 0);
	this->SetWidgetShaded(CIW_ENTRANCE_FEE_DECREASE, this->ci->item_price[0] <= 0);
	this->SetWidgetShaded(CIW_MIN_IDLE_DECREASE, this->ci->state == RIS_OPEN || this->ci->min_idle_duration <= 0);
	this->SetWidgetShaded(CIW_MIN_IDLE_INCREASE, this->ci->state == RIS_OPEN || this->ci->min_idle_duration >= this->ci->max_idle_duration);
	this->SetWidgetShaded(CIW_MAX_IDLE_DECREASE, this->ci->state == RIS_OPEN || this->ci->max_idle_duration <= this->ci->min_idle_duration);
	this->SetWidgetShaded(CIW_MAX_IDLE_INCREASE, this->ci->state == RIS_OPEN);
}

/**
 * Make a radio button either checked or not checked according the parameters.
 * @param radio Radio button acted on by this method.
 * @param checked Boolean flag to decided if the radio button should be checked (true) or unchecked (false).
 */
void CoasterInstanceWindow::SetRadioChecked(WidgetNumber radio, bool checked)
{
	this->SetWidgetChecked(radio, checked);
	this->SetWidgetPressed(radio, checked);
	this->MarkWidgetDirty(radio);
}

void CoasterInstanceWindow::OnChange(const ChangeCode code, const uint32 parameter)
{
	switch (code) {
		case CHG_DISPLAY_OLD:
			this->MarkDirty();
			break;
		case CHG_DROPDOWN_RESULT:
			switch ((parameter >> 16) & 0xFF) {
				case CIW_CHOOSE_ENTRANCE:
					this->ci->SetEntranceType(parameter & 0xFF);
					this->UpdateRecolourButtons();
					break;
				case CIW_CHOOSE_EXIT:
					this->ci->SetExitType(parameter & 0xFF);
					this->UpdateRecolourButtons();
					break;
				case CIW_NUMBER_TRAINS:
					this->ci->SetNumberOfTrains((parameter & 0xFF) + 1 /* Counting from 1 on because there can not be 0 cars/trains. */);
					break;
				case CIW_NUMBER_CARS:
					this->ci->SetNumberOfCars((parameter & 0xFF) + 1);
					/* This also updates the positions of all trains in case the train length changed. */
					this->ci->SetNumberOfTrains(std::min(this->ci->number_of_trains, this->ci->GetMaxNumberOfTrains(this->ci->number_of_trains)));
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void CoasterInstanceWindow::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	const Point32 world_pos = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);
	const int8 dx = _orientation_signum_dx[vp->orientation];
	const int8 dy = _orientation_signum_dy[vp->orientation];
	entrance_exit_placement.MarkDirty();
	bool placed = false;
	for (int z = WORLD_Z_SIZE - 1; z >= 0; z--) {
		const int dz = (z - (vp->view_pos.z / 256)) / 2;
		const XYZPoint16 location(world_pos.x / 256 + dz * dx, world_pos.y / 256 + dz * dy, z);
		if (!this->ci->CanPlaceEntranceOrExit(location, this->is_placing_entrance, nullptr)) continue;

		if (this->is_placing_entrance) {
			this->ci->temp_entrance_pos = location;
		} else {
			this->ci->temp_exit_pos = location;
		}
		this->entrance_exit_placement.SetSize(1, 1);
		this->entrance_exit_placement.SetPosition(location.x, location.y);
		this->entrance_exit_placement.AddVoxel(location);
		this->entrance_exit_placement.SetupRideInfoSpace();
		this->entrance_exit_placement.SetRideData(location, static_cast<SmallRideInstance>(this->ci->GetIndex()), SHF_ENTRANCE_BITS);
		placed = true;
		break;
	}
	if (!placed) {
		if (this->is_placing_entrance) {
			this->ci->temp_entrance_pos = XYZPoint16::invalid();
		} else {
			this->ci->temp_exit_pos = XYZPoint16::invalid();
		}
		entrance_exit_placement.SetSize(0, 0);
	}
	entrance_exit_placement.MarkDirty();
}

void CoasterInstanceWindow::SelectorMouseButtonEvent(const uint8 state)
{
	if (!IsLeftClick(state)) return;
	if (entrance_exit_placement.area.width != 1 || entrance_exit_placement.area.height != 1) return;

	if (this->ci->PlaceEntranceOrExit(this->is_placing_entrance ? this->ci->temp_entrance_pos : this->ci->temp_exit_pos, this->is_placing_entrance, nullptr)) {
		this->ci->temp_entrance_pos = XYZPoint16::invalid();
		this->ci->temp_exit_pos = XYZPoint16::invalid();
		SetSelector(nullptr);

		/* If the user is still in the process of building the ride, immediately prompt for the placement of another side-building as well. */
		const bool need_entrance = this->ci->NeedsEntrance();
		const bool need_exit = this->ci->NeedsExit();
		if (this->is_placing_entrance && need_exit) {
			/* After placing an entrance, preferably build an exit next. */
			ChooseEntranceExitClicked(false);
		} else if (!this->is_placing_entrance && need_entrance) {
			/* After placing an exit, preferably build an entrance next. */
			ChooseEntranceExitClicked(true);
		} else if (need_entrance) {
			ChooseEntranceExitClicked(true);
		} else if (need_exit) {
			ChooseEntranceExitClicked(false);
		}
		SetCoasterState();
	}
}

/**
 * Open a roller coaster management window for the given roller coaster ride.
 * @param coaster Coaster instance to display.
 */
void ShowCoasterManagementGui(RideInstance *coaster)
{
	if (coaster->GetKind() != RTK_COASTER) return;
	CoasterInstance *ci = static_cast<CoasterInstance *>(coaster);
	assert(ci != nullptr);

	if (!ci->MakePositionedPiecesLooping(nullptr)) {
		assert(ci->state == RIS_BUILDING);
		ShowCoasterBuildGui(ci);
		return;
	}

	/* Complete the construction process if necessary. */
	if (ci->state == RIS_BUILDING) ci->CloseRide();
	if (ci->cars_per_train < 1) ci->SetNumberOfCars(ci->GetMaxNumberOfCars());
	if (ci->number_of_trains < 1) ci->SetNumberOfTrains(ci->GetMaxNumberOfTrains(ci->cars_per_train));

	Window *w = HighlightWindowByType(WC_COASTER_MANAGER, coaster->GetIndex());
	if (w != nullptr) {
		static_cast<CoasterInstanceWindow*>(w)->SetCoasterState();
		return;
	}
	new CoasterInstanceWindow(ci);
}

/** Mouse selector for building/selecting new track pieces. */
class TrackPieceMouseMode : public RideMouseMode {
public:
	TrackPieceMouseMode(CoasterInstance *ci);
	~TrackPieceMouseMode();

	void SetTrackPiece(const XYZPoint16 &pos, ConstTrackPiecePtr &piece);

	CoasterInstance *ci;            ///< Roller coaster instance to build or edit.
	PositionedTrackPiece pos_piece; ///< Piece to display, actual piece may be \c nullptr if nothing to display.
};

/**
 * Constructor of the trackpiece mouse mode.
 * @param ci Coaster to edit.
 */
TrackPieceMouseMode::TrackPieceMouseMode(CoasterInstance *ci) : RideMouseMode()
{
	this->ci = ci;
	this->pos_piece.piece = nullptr;
}

TrackPieceMouseMode::~TrackPieceMouseMode()
{
}

/**
 * Setup the mouse selector for displaying a track piece.
 * @param pos Base position of the new track piece.
 * @param piece Track piece to display.
 */
void TrackPieceMouseMode::SetTrackPiece(const XYZPoint16 &pos, ConstTrackPiecePtr &piece)
{
	if (this->pos_piece.piece != nullptr) this->MarkDirty(); // Mark current area.

	this->pos_piece.piece = piece;
	if (this->pos_piece.piece != nullptr) {
		this->pos_piece.base_voxel = pos;

		this->area = piece->GetArea();
		this->area.base.x += pos.x; // Set new cursor area, origin may be different from piece_pos due to negative extent of a piece.
		this->area.base.y += pos.y;
		this->InitTileData();

		for (const auto &tv : this->pos_piece.piece->track_voxels) this->AddVoxel(this->pos_piece.base_voxel + tv->dxyz);

		this->SetupRideInfoSpace();
		for (const auto &tv : this->pos_piece.piece->track_voxels) {
			XYZPoint16 p(this->pos_piece.base_voxel + tv->dxyz);
			this->SetRideData(p, this->ci->GetRideNumber(), this->ci->GetInstanceData(tv.get()));
		}

		this->MarkDirty();
	}
}

/** Widgets of the coaster construction window. */
enum CoasterConstructionWidgets {
	CCW_TITLEBAR,            ///< Titlebar widget.
	CCW_BEND_WIDE_LEFT,      ///< Button for selecting wide left turn. Same order as #TrackBend.
	CCW_BEND_NORMAL_LEFT,    ///< Button for selecting normal left turn.
	CCW_BEND_TIGHT_LEFT,     ///< Button for selecting tight left turn.
	CCW_BEND_NONE,           ///< Button for selecting straight ahead (no turn).
	CCW_BEND_TIGHT_RIGHT,    ///< Button for selecting tight right turn.
	CCW_BEND_NORMAL_RIGHT,   ///< Button for selecting normal right turn.
	CCW_BEND_WIDE_RIGHT,     ///< Button for selecting wide right turn.
	CCW_BANK_NONE,           ///< Button for selecting no banking. Same order as #TrackPieceBanking.
	CCW_BANK_LEFT,           ///< Button for selecting banking to the left.
	CCW_BANK_RIGHT,          ///< Button for selecting banking to the right.
	CCW_NO_PLATFORM,         ///< Button for selecting tracks without platform.
	CCW_PLATFORM,            ///< Button for selecting tracks with platform.
	CCW_NOT_POWERED,         ///< Button for selecting unpowered tracks.
	CCW_POWERED,             ///< Button for selecting powered tracks.
	CCW_SLOPE_DOWN,          ///< Button for selecting gentle down slope. Same order as #TrackSlope.
	CCW_SLOPE_FLAT,          ///< Button for selecting level slope.
	CCW_SLOPE_UP,            ///< Button for selecting gentle up slope.
	CCW_SLOPE_STEEP_DOWN,    ///< Button for selecting steep down slope.
	CCW_SLOPE_STEEP_UP,      ///< Button for selecting steep up slope.
	CCW_SLOPE_VERTICAL_DOWN, ///< Button for selecting vertically down slope.
	CCW_SLOPE_VERTICAL_UP,   ///< Button for selecting vertically up slope.
	CCW_DISPLAY_PIECE,       ///< Display space for a track piece.
	CCW_REMOVE,              ///< Remove track piece button.
	CCW_BACKWARD,            ///< Move backward.
	CCW_FORWARD,             ///< Move forward.
	CCW_ROT_NEG,             ///< Rotate in negative direction.
	CCW_ROT_POS,             ///< Rotate in positive direction.
};

/** Widget parts of the #CoasterBuildWindow. */
static const WidgetPart _coaster_construction_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, CCW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(5, 1),
				Intermediate(1, 9), // Bend type.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_WIDE_LEFT,    COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_WIDE, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NORMAL_LEFT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_NORMAL, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_TIGHT_LEFT,   COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_LEFT_TIGHT, GUI_COASTER_BUILD_LEFT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NONE,         COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_STRAIGHT, GUI_COASTER_BUILD_NO_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_TIGHT_RIGHT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_TIGHT, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_NORMAL_RIGHT, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_NORMAL, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BEND_WIDE_RIGHT,   COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BEND_START + TBN_RIGHT_WIDE, GUI_COASTER_BUILD_RIGHT_BEND_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Intermediate(1, 11), // Banking, platforms, powered.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_LEFT,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BANK_START + TPB_LEFT, GUI_COASTER_BUILD_BANK_LEFT_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_NONE,  COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_BANK_START + TPB_NONE, GUI_COASTER_BUILD_BANK_NONE_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_BANK_RIGHT, COL_RANGE_DARK_RED), SetPadding(0, 0, 3, 0),
							SetData(SPR_GUI_BANK_START + TPB_RIGHT, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_PLATFORM, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_HAS_PLATFORM, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_NO_PLATFORM, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_NO_PLATFORM, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_POWERED, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_HAS_POWER, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_IMAGE_BUTTON, CCW_NOT_POWERED, COL_RANGE_DARK_RED), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_NO_POWER, GUI_COASTER_BUILD_BANK_RIGHT_TOOLTIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Intermediate(1, 9), // Slopes.
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_VERTICAL_DOWN, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STRAIGHT_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_STEEP_DOWN, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STEEP_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_DOWN, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_FLAT, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_FLAT, GUI_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_UP, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_STEEP_UP, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STEEP_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_IMAGE_BUTTON, CCW_SLOPE_VERTICAL_UP, COL_RANGE_GREY), SetPadding(0, 5, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_STRAIGHT_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetFill(1, 0),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetPadding(5, 2, 5, 2),
					Widget(WT_TEXT_PUSHBUTTON, CCW_DISPLAY_PIECE, COL_RANGE_DARK_RED),
							SetData(STR_NULL, GUI_COASTER_BUILD_BUY_TOOLTIP), SetFill(1, 1), SetMinimalSize(200, 200),
				Intermediate(1, 5), // delete, prev/next, rotate
					Widget(WT_TEXT_PUSHBUTTON, CCW_REMOVE, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_REMOVE, GUI_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_TEXT_PUSHBUTTON, CCW_BACKWARD, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_BACKWARD, GUI_PATH_GUI_BACKWARD_TIP),
					Widget(WT_TEXT_PUSHBUTTON, CCW_FORWARD, COL_RANGE_DARK_RED),  SetPadding(0, 3, 3, 0),
							SetData(GUI_PATH_GUI_FORWARD, GUI_PATH_GUI_FORWARD_TIP),
					Widget(WT_IMAGE_PUSHBUTTON, CCW_ROT_POS, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_ROT3D_POS, GUI_RIDE_SELECT_ROT_POS_TOOLTIP),
					Widget(WT_IMAGE_PUSHBUTTON, CCW_ROT_NEG, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
							SetData(SPR_GUI_ROT3D_NEG, GUI_RIDE_SELECT_ROT_NEG_TOOLTIP),
	EndContainer(),
};

/** Three-valued boolean. */
enum BoolSelect {
	BSL_FALSE, ///< Selected boolean is \c false.
	BSL_TRUE,  ///< Selected boolean is \c true.
	BSL_NONE,  ///< Boolean is not selectable.
};

/**
 * Window to build or edit a roller coaster.
 *
 * The build window can be in the following state
 * - #cur_piece is \c nullptr: An initial piece is being placed, The mouse mode defines where, #build_direction defines in which direction.
 * - #cur_piece is not \c nullptr, and #cur_after: A piece is added after #cur_piece.
 * - #cur_piece is not \c nullptr, and not #cur_after: A piece is added before #cur_piece.
 * In the latter two cases, #cur_sel points at the piece being replaced, if it exists.
 */
class CoasterBuildWindow : public GuiWindow {
public:
	CoasterBuildWindow(CoasterInstance *instance);
	~CoasterBuildWindow();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;

	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

private:
	CoasterInstance *ci; ///< Roller coaster instance to build or edit.

	PositionedTrackPiece *cur_piece;     ///< Current track piece, if available (else \c nullptr).
	bool cur_after;                      ///< Position relative to #cur_piece, \c false means before, \c true means after.
	const PositionedTrackPiece *cur_sel; ///< Selected track piece of #cur_piece and #cur_after, or \c nullptr if no piece selected.

	ConstTrackPiecePtr sel_piece; ///< Currently selected piece (and not yet build), if any.
	TileEdge build_direction;     ///< If #cur_piece is \c nullptr, the direction of building.
	TrackSlope sel_slope;         ///< Selected track slope at the UI, or #TSL_INVALID.
	TrackBend sel_bend;           ///< Selected bend at the UI, or #TPB_INVALID.
	TrackPieceBanking sel_bank;   ///< Selected bank at the UI, or #TBN_INVALID.
	BoolSelect sel_platform;      ///< Whether the track piece should have a platform, or #BSL_NONE.
	BoolSelect sel_power;         ///< Whether the selected piece should have power, or #BSL_NONE.

	void SetupSelection();
	int SetButtons(int start_widget, int count, uint avail, int cur_sel, int invalid_val);
	void BuildTrackPiece();
	void UpdateSelectedPiece();

	TrackPieceMouseMode piece_selector; ///< Selector for displaying new track pieces.
};

/**
 * Constructor of the roller coaster build window. The provided instance may be completely empty.
 * @param ci Coaster instance to build or modify.
 */
CoasterBuildWindow::CoasterBuildWindow(CoasterInstance *instance) : GuiWindow(WC_COASTER_BUILD, instance->GetIndex()), ci(instance), piece_selector(instance)
{
	this->SetupWidgetTree(_coaster_construction_gui_parts, lengthof(_coaster_construction_gui_parts));

	int first = this->ci->GetFirstPlacedTrackPiece();
	if (first >= 0) {
		this->cur_piece = &this->ci->pieces[first];
		first = this->ci->FindSuccessorPiece(*this->cur_piece);
		this->cur_sel = (first >= 0) ? &this->ci->pieces[first] : nullptr;
	} else {
		this->cur_piece = nullptr;
		this->cur_sel = nullptr;
	}
	this->cur_after = true;
	this->build_direction = EDGE_NE;
	this->sel_slope = TSL_INVALID;
	this->sel_bend = TBN_INVALID;
	this->sel_bank = TPB_INVALID;
	this->sel_platform = BSL_NONE;
	this->sel_power = BSL_NONE;

	this->SetSelector(&this->piece_selector);
	this->SetupSelection();
}

CoasterBuildWindow::~CoasterBuildWindow()
{
	this->SetSelector(nullptr);

	if (!GetWindowByType(WC_COASTER_MANAGER, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

void CoasterBuildWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CCW_TITLEBAR:
			_str_params.SetText(1, this->ci->name);
			break;
	}
}

void CoasterBuildWindow::OnClick(WidgetNumber widget, [[maybe_unused]] const Point16 &pos)
{
	switch (widget) {
		case CCW_BANK_NONE:
		case CCW_BANK_LEFT:
		case CCW_BANK_RIGHT:
			this->sel_bank = (TrackPieceBanking)(widget - CCW_BANK_NONE);
			break;

		case CCW_PLATFORM:
			this->sel_platform = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_TRUE;
			break;

		case CCW_NO_PLATFORM:
			this->sel_platform = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_FALSE;
			break;

		case CCW_POWERED:
			this->sel_power = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_TRUE;
			break;

		case CCW_NOT_POWERED:
			this->sel_power = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_FALSE;
			break;

		case CCW_SLOPE_DOWN:
		case CCW_SLOPE_FLAT:
		case CCW_SLOPE_UP:
		case CCW_SLOPE_STEEP_DOWN:
		case CCW_SLOPE_STEEP_UP:
		case CCW_SLOPE_VERTICAL_DOWN:
		case CCW_SLOPE_VERTICAL_UP:
			this->sel_slope = (TrackSlope)(widget - CCW_SLOPE_DOWN);
			break;

		case CCW_DISPLAY_PIECE: {
			this->BuildTrackPiece();
			break;
		}
		case CCW_REMOVE: {
			int pred_index = this->ci->FindPredecessorPiece(*this->cur_piece);
			this->ci->RemovePositionedPiece(*this->cur_piece);

			this->cur_piece = pred_index == -1 ? nullptr : &this->ci->pieces[pred_index];
			this->UpdateSelectedPiece();
			break;
		}

		case CCW_BEND_WIDE_LEFT:
		case CCW_BEND_NORMAL_LEFT:
		case CCW_BEND_TIGHT_LEFT:
		case CCW_BEND_NONE:
		case CCW_BEND_TIGHT_RIGHT:
		case CCW_BEND_NORMAL_RIGHT:
		case CCW_BEND_WIDE_RIGHT:
			this->sel_bend = (TrackBend)(widget - CCW_BEND_WIDE_LEFT);
			break;

		case CCW_ROT_NEG:
			if (this->cur_piece == nullptr) {
				this->build_direction = (TileEdge)((this->build_direction + 1) % 4);
			}
			break;

		case CCW_ROT_POS:
			if (this->cur_piece == nullptr) {
				this->build_direction = (TileEdge)((this->build_direction + 3) % 4);
			}
			break;

		case CCW_BACKWARD: {
			int pred_index = this->ci->FindPredecessorPiece(*this->cur_piece);
			this->cur_piece = &this->ci->pieces[pred_index];
			this->UpdateSelectedPiece();
			break;
		}

		case CCW_FORWARD: {
			int succ_index = this->ci->FindSuccessorPiece(*this->cur_piece);
			this->cur_piece = &this->ci->pieces[succ_index];
			this->UpdateSelectedPiece();
			break;
		}
	}
	this->SetupSelection();
}

/**
 * Set buttons according to availability of track pieces.
 * @param start_widget First widget of the buttons.
 * @param count Number of buttons.
 * @param avail Bitset of available track pieces for the buttons.
 * @param cur_sel Currently selected button.
 * @param invalid_val Invalid value for the selection.
 * @return New value for the current selection.
 */
int CoasterBuildWindow::SetButtons(int start_widget, int count, uint avail, int cur_sel, int invalid_val)
{
	int num_bits = CountBits(avail);
	for (int i = 0; i < count; i++) {
		if ((avail & (1 << i)) == 0) {
			this->SetWidgetShaded(start_widget + i, true);
			if (cur_sel == i) cur_sel = invalid_val;
		} else {
			this->SetWidgetShaded(start_widget + i, false);
			if (num_bits == 1) cur_sel = i;
			this->SetWidgetPressed(start_widget + i, cur_sel == i);
		}
	}
	return cur_sel;
}

/**
 * Find out whether the provided track piece has a platform.
 * @param piece Track piece to examine.
 * @return #BSL_TRUE if the track piece has a platform, else #BSL_FALSE.
 */
static BoolSelect GetPlatform(ConstTrackPiecePtr piece)
{
	return piece->HasPlatform() ? BSL_TRUE : BSL_FALSE;
}

/**
 * Find out whether the provided track piece has is powered.
 * @param piece Track piece to examine.
 * @return #BSL_TRUE if the track piece is powered, else #BSL_FALSE.
 */
static BoolSelect GetPower(ConstTrackPiecePtr piece)
{
	return piece->HasPower() ? BSL_TRUE : BSL_FALSE;
}

/** Set up the window so the user can make a selection. */
void CoasterBuildWindow::SetupSelection()
{
	uint directions = 0; // Build directions of initial pieces.
	uint avail_bank = 0;
	uint avail_slope = 0;
	uint avail_bend = 0;
	uint avail_platform = 0;
	uint avail_power = 0;
	this->sel_piece = nullptr;

	if (this->cur_piece == nullptr || this->cur_sel == nullptr) {
		/* Only consider track pieces when there is no current positioned track piece. */

		const CoasterType *ct = this->ci->GetCoasterType();

		bool selectable[1024]; // Arbitrary limit on the number of non-placed track pieces.
		uint count = ct->pieces.size();
		if (count > lengthof(selectable)) count = lengthof(selectable);
		/* Round 1: Select on connection or initial placement. */
		for (uint i = 0; i < count; i++) {
			ConstTrackPiecePtr piece = ct->pieces[i];
			bool avail = true;
			if (this->cur_piece != nullptr) {
				/* Connect after or before 'cur_piece'. */
				if (this->cur_after) {
					if (piece->entry_connect != this->cur_piece->piece->exit_connect) avail = false;
				} else {
					if (piece->exit_connect != this->cur_piece->piece->entry_connect) avail = false;
				}
			} else {
				/* Initial placement. */
				if (!piece->IsStartingPiece()) {
					avail = false;
				} else {
					directions |= 1 << piece->GetStartDirection();
					if (piece->GetStartDirection() != this->build_direction) avail = false;
				}
			}
			selectable[i] = avail;
		}

		/* Round 2: Setup banking. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			avail_bank |= 1 << piece->GetBanking();
		}
		if (this->sel_bank != TPB_INVALID && (avail_bank & (1 << this->sel_bank)) == 0) this->sel_bank = TPB_INVALID;

		/* Round 3: Setup slopes from pieces with the correct bank. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			if (this->sel_bank != TPB_INVALID && piece->GetBanking() != this->sel_bank) {
				selectable[i] = false;
			} else {
				avail_slope |= 1 << piece->GetSlope();
			}
		}
		if (this->sel_slope != TSL_INVALID && (avail_slope & (1 << this->sel_slope)) == 0) this->sel_slope = TSL_INVALID;

		/* Round 4: Setup bends from pieces with the correct slope. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			if (this->sel_slope != TSL_INVALID && piece->GetSlope() != this->sel_slope) {
				selectable[i] = false;
			} else {
				avail_bend |= 1 << piece->GetBend();
			}
		}
		if (this->sel_bend != TBN_INVALID && (avail_bend & (1 << this->sel_bend)) == 0) this->sel_bend = TBN_INVALID;

		/* Round 5: Setup platform from pieces with the correct bend. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			if (this->sel_bend != TBN_INVALID && piece->GetBend() != this->sel_bend) {
				selectable[i] = false;
			} else {
				avail_platform |= 1 << GetPlatform(piece);
			}
		}
		if (this->sel_platform != BSL_NONE && (avail_platform & (1 << this->sel_platform)) == 0) this->sel_platform = BSL_NONE;

		/* Round 6: Setup power from pieces with the correct platform. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			if (this->sel_platform != BSL_NONE && GetPlatform(piece) != this->sel_platform) {
				selectable[i] = false;
			} else {
				avail_power |= 1 << GetPower(piece);
			}
		}
		if (this->sel_power != BSL_NONE && (avail_power & (1 << this->sel_power)) == 0) this->sel_power = BSL_NONE;

		/* Round 7: Select a piece from the pieces with the correct power. */
		for (uint i = 0; i < count; i++) {
			if (!selectable[i]) continue;
			ConstTrackPiecePtr piece = ct->pieces[i];
			if (this->sel_power != BSL_NONE && GetPower(piece) != this->sel_power) continue;
			this->sel_piece = piece;
			break;
		}
	}

	/* Set shading of rotate buttons. */
	bool enabled = (this->cur_piece == nullptr && CountBits(directions) > 1);
	this->SetWidgetShaded(CCW_ROT_NEG,  !enabled);
	this->SetWidgetShaded(CCW_ROT_POS,  !enabled);

	/* Set shading of forward/back buttons. */
	enabled = this->cur_piece != nullptr && this->ci->FindSuccessorPiece(*this->cur_piece) != -1;
	this->SetWidgetShaded(CCW_FORWARD, !enabled);
	enabled = this->cur_piece != nullptr && this->ci->FindPredecessorPiece(*this->cur_piece) != -1;
	this->SetWidgetShaded(CCW_BACKWARD, !enabled);

	enabled = this->cur_piece != nullptr && this->ci->FindSuccessorPiece(*this->cur_piece) == -1;
	this->SetWidgetShaded(CCW_DISPLAY_PIECE, !enabled);

	enabled = this->cur_piece != nullptr;
	this->SetWidgetShaded(CCW_REMOVE, !enabled);

	this->sel_bank = static_cast<TrackPieceBanking>(this->SetButtons(CCW_BANK_NONE, TPB_COUNT, avail_bank, this->sel_bank, TPB_INVALID));
	this->sel_slope = static_cast<TrackSlope>(this->SetButtons(CCW_SLOPE_DOWN, TSL_COUNT_VERTICAL, avail_slope, this->sel_slope, TSL_INVALID));
	this->sel_bend = static_cast<TrackBend>(this->SetButtons(CCW_BEND_WIDE_LEFT, TBN_COUNT, avail_bend, this->sel_bend, TBN_INVALID));
	this->sel_platform = static_cast<BoolSelect>(this->SetButtons(CCW_NO_PLATFORM, 2, avail_platform, this->sel_platform, BSL_NONE));
	this->sel_power = static_cast<BoolSelect>(this->SetButtons(CCW_NOT_POWERED, 2, avail_power, this->sel_power, BSL_NONE));

	if (this->sel_piece == nullptr) { // Highlight current piece
		this->piece_selector.SetTrackPiece(this->cur_piece->base_voxel, this->cur_piece->piece);
		return;
	}

	if (this->cur_piece == nullptr) { // Display start-piece, moving it around.
		/** \todo this->build_direction is the direction of construction.
		 *  \todo Get a nice 'current' mouse position. */
		this->piece_selector.SetTrackPiece(XYZPoint16(0, 0, 0), this->sel_piece);
		return;
	}

	if (this->cur_after) { // Disply next coaster piece.
		/* \todo TileEdge dir = (TileEdge)(this->cur_piece->piece->exit_connect & 3); is the direction of construction.
		 * \todo Define this usage of bits in the data format. */
		this->piece_selector.SetTrackPiece(this->cur_piece->GetEndXYZ(), this->sel_piece);
		return;
	}

	/* Display previous coaster piece. */
	this->piece_selector.SetSize(0, 0); /// \todo Make this actually work when adding 'back'.
	this->piece_selector.pos_piece.piece = nullptr;
}

void CoasterBuildWindow::SelectorMouseMoveEvent(Viewport *vp, [[maybe_unused]] const Point16 &pos)
{
	if (this->selector == nullptr || this->piece_selector.pos_piece.piece == nullptr) return; // No active selector.
	if (this->sel_piece == nullptr || this->cur_piece != nullptr) return; // No piece, or fixed position.

	FinderData fdata(CS_GROUND, FW_TILE);
	if (vp->ComputeCursorPosition(&fdata) != CS_GROUND) return;
	XYZPoint16 &piece_base = this->piece_selector.pos_piece.base_voxel;
	int dx = fdata.voxel_pos.x - piece_base.x;
	int dy = fdata.voxel_pos.y - piece_base.y;
	if (dx == 0 && dy == 0) return;

	this->piece_selector.MarkDirty();

	this->piece_selector.SetPosition(this->piece_selector.area.base.x + dx, this->piece_selector.area.base.y + dy);
	uint8 height = _world.GetTopGroundHeight(fdata.voxel_pos.x, fdata.voxel_pos.y);
	this->piece_selector.SetTrackPiece(XYZPoint16(fdata.voxel_pos.x, fdata.voxel_pos.y, height), this->sel_piece);
}

void CoasterBuildWindow::SelectorMouseButtonEvent(uint8 state)
{
	if (!IsLeftClick(state)) return;

	this->BuildTrackPiece();
	this->SetupSelection();
}

/** Create the currently selected track piece in the world, and update the selection. */
void CoasterBuildWindow::BuildTrackPiece()
{
	if (this->selector == nullptr || this->piece_selector.pos_piece.piece == nullptr) return; // No active selector.
	if (this->sel_piece == nullptr) return; // No piece.

	if (!this->piece_selector.pos_piece.CanBePlaced()) {
		return; /// \todo Display error message.
	}

	/* Add the piece to the coaster instance. */
	int ptp_index = this->ci->AddPositionedPiece(this->piece_selector.pos_piece);
	if (ptp_index >= 0) {
		this->ci->PlaceTrackPieceInWorld(this->piece_selector.pos_piece); // Add the piece to the world.

		/* Piece was added, change the setup for the next piece. */
		this->cur_piece = &this->ci->pieces[ptp_index];

		this->UpdateSelectedPiece();

		this->cur_after = true;
	}
}

/** Update the instance variable cur_sel to reflect changes in the value of the current piece (cur_piece). */
void CoasterBuildWindow::UpdateSelectedPiece()
{
	if (this->cur_piece != nullptr){
		int succ = this->ci->FindSuccessorPiece(*this->cur_piece);
		this->cur_sel = succ != -1 ? &this->ci->pieces[succ] : nullptr;
	}
}

/**
 * Open a roller coaster build/edit window for the given roller coaster.
 * @param coaster Coaster instance to modify.
 */
void ShowCoasterBuildGui(CoasterInstance *coaster)
{
	if (coaster->GetKind() != RTK_COASTER) return;
	if (HighlightWindowByType(WC_COASTER_BUILD, coaster->GetIndex()) != nullptr) return;

	new CoasterBuildWindow(coaster);
}
