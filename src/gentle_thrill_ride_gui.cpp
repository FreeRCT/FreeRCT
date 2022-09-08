/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gentle_thrill_ride_gui.cpp %Window for interacting with gentle and thrill rides. */

#include "stdafx.h"
#include "math_func.h"
#include "window.h"
#include "money.h"
#include "language.h"
#include "sprite_store.h"
#include "gentle_thrill_ride_type.h"
#include "entity_gui.h"
#include "mouse_mode.h"
#include "viewport.h"
#include "generated/entrance_exit_strings.h"

/** Window to prompt for removing a gentle/thrill ride. */
class GentleThrillRideRemoveWindow : public EntityRemoveWindow  {
public:
	GentleThrillRideRemoveWindow(GentleThrillRideInstance *si);

	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	GentleThrillRideInstance *si; ///< Gentle/Thrill ride instance to remove.
};

/**
 * Constructor of the gentle/thrill ride remove window.
 * @param si Gentle/Thrill ride instance to remove.
 */
GentleThrillRideRemoveWindow::GentleThrillRideRemoveWindow(GentleThrillRideInstance *instance)
: EntityRemoveWindow(WC_GENTLE_THRILL_RIDE_REMOVE, instance->GetIndex()), si(instance)
{
}

void GentleThrillRideRemoveWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	if (number == ERW_YES) {
		delete GetWindowByType(WC_GENTLE_THRILL_RIDE_MANAGER, this->si->GetIndex());
		_rides_manager.DeleteInstance(this->si->GetIndex());
	}
	delete this;
}

void GentleThrillRideRemoveWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	if (wid_num == ERW_MESSAGE) _str_params.SetText(1, this->si->name);
}

/**
 * Open a gentle/thrill ride remove remove window for the given ride.
 * @param si ride instance to remove.
 */
void ShowGentleThrillRideRemove(GentleThrillRideInstance *si)
{
	if (HighlightWindowByType(WC_GENTLE_THRILL_RIDE_REMOVE, si->GetIndex()) != nullptr) return;

	new GentleThrillRideRemoveWindow(si);
}

/** Widgets of the gentle/thrill ride management window. */
enum GentleThrillRideManagerWidgets {
	GTRMW_TITLEBAR,               ///< Window titlebar.
	GTRMW_MONTHLY_COST,           ///< Ride monthly cost display.
	GTRMW_EXCITEMENT_RATING,      ///< Ride excitement rating display.
	GTRMW_INTENSITY_RATING,       ///< Ride intensity rating display.
	GTRMW_NAUSEA_RATING,          ///< Ride nausea rating display.
	GTRMW_ENTRANCE_FEE,           ///< Ride entrance fee display.
	GTRMW_ENTRANCE_FEE_DECREASE,  ///< Ride entrance fee decrease button.
	GTRMW_ENTRANCE_FEE_INCREASE,  ///< Ride entrance fee increase button.
	GTRMW_CYCLES,                 ///< Number of working cycles display.
	GTRMW_CYCLES_DECREASE,        ///< Number of working cycles decrease button.
	GTRMW_CYCLES_INCREASE,        ///< Number of working cycles increase button.
	GTRMW_MIN_IDLE,               ///< Minimum idle time display.
	GTRMW_MIN_IDLE_DECREASE,      ///< Minimum idle time decrease button.
	GTRMW_MIN_IDLE_INCREASE,      ///< Minimum idle time increase button.
	GTRMW_MAX_IDLE,               ///< Maximum idle time display.
	GTRMW_MAX_IDLE_DECREASE,      ///< Maximum idle time decrease button.
	GTRMW_MAX_IDLE_INCREASE,      ///< Maximum idle time increase button.
	GTRMW_OPEN_RIDE_PANEL,        ///< Open ride button.
	GTRMW_CLOSE_RIDE_PANEL,       ///< Close ride button.
	GTRMW_OPEN_RIDE_LIGHT,        ///< Open ride light.
	GTRMW_CLOSE_RIDE_LIGHT,       ///< Close ride light.
	GTRMW_RELIABILITY,            ///< Ride reliability display.
	GTRMW_MAINTENANCE,            ///< Ride maintenance interval display.
	GTRMW_MAINTENANCE_DECREASE,   ///< Ride maintenance interval decrease button.
	GTRMW_MAINTENANCE_INCREASE,   ///< Ride maintenance interval increase button.
	GTRMW_RECOLOUR1,              ///< First ride recolouring.
	GTRMW_RECOLOUR2,              ///< Second ride recolouring.
	GTRMW_RECOLOUR3,              ///< Third ride recolouring.
	GTRMW_PLACE_ENTRANCE,         ///< Place entrance button.
	GTRMW_CHOOSE_ENTRANCE,        ///< Choose entrance type button.
	GTRMW_PLACE_EXIT,             ///< Place exit button.
	GTRMW_CHOOSE_EXIT,            ///< Choose exit type button.
	GTRMW_ENTRANCE_RECOLOUR1,     ///< First entrance recolouring.
	GTRMW_ENTRANCE_RECOLOUR2,     ///< Second entrance recolouring.
	GTRMW_ENTRANCE_RECOLOUR3,     ///< Third entrance recolouring.
	GTRMW_EXIT_RECOLOUR1,         ///< First exit recolouring.
	GTRMW_EXIT_RECOLOUR2,         ///< Second exit recolouring.
	GTRMW_EXIT_RECOLOUR3,         ///< Third exit recolouring.
	GTRMW_REMOVE,                 ///< Ride deletion button.
};

/** Widget parts of the #GentleThrillRideManagerWindow. */
static const WidgetPart _gentle_thrill_ride_manager_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, GTRMW_TITLEBAR, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(4, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_MONTHLY_COST_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_MONTHLY_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_EXCITEMENT, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_EXCITEMENT_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_INTENSITY, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_INTENSITY_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_NAUSEA, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_NAUSEA_RATING, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 1),
				Intermediate(5, 4),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_ENTRANCE_FEE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_ENTRANCE_FEE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, GTRMW_ENTRANCE_FEE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_ENTRANCE_FEE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_CYCLES_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_CYCLES_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, GTRMW_CYCLES, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_CYCLES_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MIN_IDLE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MIN_IDLE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, GTRMW_MIN_IDLE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MIN_IDLE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MAX_IDLE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MAX_IDLE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, GTRMW_MAX_IDLE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MAX_IDLE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
					Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_RIDE_MANAGER_MAINTENANCE_TEXT, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MAINTENANCE_DECREASE, COL_RANGE_DARK_RED), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT, GTRMW_MAINTENANCE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, GTRMW_MAINTENANCE_INCREASE, COL_RANGE_DARK_RED), SetData(GUI_INCREASE_BUTTON, STR_NULL),
				Widget(WT_LEFT_TEXT, GTRMW_RELIABILITY, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(1, 3), SetEqualSize(false, true),
				Intermediate(2, 1),
					Intermediate(1, 3),
						Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR1, COL_RANGE_DARK_RED),
								SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR1, STR_NULL), SetPadding(2, 2, 2, 2),
						Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR2, COL_RANGE_DARK_RED),
								SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR2, STR_NULL), SetPadding(2, 2, 2, 2),
						Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR3, COL_RANGE_DARK_RED),
								SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR3, STR_NULL), SetPadding(2, 2, 2, 2),
					Intermediate(3, 2),
						Widget(WT_TEXT_PUSHBUTTON, GTRMW_PLACE_ENTRANCE, COL_RANGE_DARK_RED),
								SetData(GUI_PLACE_ENTRANCE, GUI_PLACE_ENTRANCE_TOOLTIP),
						Widget(WT_TEXT_PUSHBUTTON, GTRMW_PLACE_EXIT, COL_RANGE_DARK_RED),
								SetData(GUI_PLACE_EXIT, GUI_PLACE_EXIT_TOOLTIP),
						Widget(WT_DROPDOWN_BUTTON, GTRMW_CHOOSE_ENTRANCE, COL_RANGE_DARK_RED),
								SetData(GUI_CHOOSE_ENTRANCE, GUI_CHOOSE_ENTRANCE_TOOLTIP),
						Widget(WT_DROPDOWN_BUTTON, GTRMW_CHOOSE_EXIT, COL_RANGE_DARK_RED),
								SetData(GUI_CHOOSE_EXIT, GUI_CHOOSE_EXIT_TOOLTIP),
						Intermediate(1, 3),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_ENTRANCE_RECOLOUR1, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_ENTRANCE_RECOLOUR2, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_ENTRANCE_RECOLOUR3, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
						Intermediate(1, 3),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_EXIT_RECOLOUR1, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_EXIT_RECOLOUR2, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
							Widget(WT_DROPDOWN_BUTTON, GTRMW_EXIT_RECOLOUR3, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL), SetPadding(2, 2, 2, 2),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
					Intermediate(0, 1),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
						Widget(WT_PANEL, GTRMW_CLOSE_RIDE_PANEL, COL_RANGE_DARK_RED),
							Widget(WT_RADIOBUTTON, GTRMW_CLOSE_RIDE_LIGHT, COL_RANGE_RED  ), SetPadding(0, 2, 0, 0),
						Widget(WT_PANEL, GTRMW_OPEN_RIDE_PANEL, COL_RANGE_DARK_RED),
							Widget(WT_RADIOBUTTON, GTRMW_OPEN_RIDE_LIGHT , COL_RANGE_GREEN), SetPadding(0, 2, 0, 0),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
				EndContainer(),

			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_TEXT_PUSHBUTTON, GTRMW_REMOVE, COL_RANGE_DARK_RED), SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
		EndContainer(),
};

/** GUI window for interacting with a gentle/thrill ride instance. */
class GentleThrillRideManagerWindow : public GuiWindow {
public:
	GentleThrillRideManagerWindow(GentleThrillRideInstance *ri);
	~GentleThrillRideManagerWindow() override;

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(MouseButtons state) override;

private:
	GentleThrillRideInstance *ride;  ///< Gentle/Thrill ride instance getting managed by this window.

	void UpdateButtons();
	void UpdateRecolourButtons();

	void ChooseEntranceExitClicked(bool entrance);
	RideMouseMode entrance_exit_placement;
	bool is_placing_entrance;
};

/**
 * Constructor of the gentle/thrill ride management window.
 * @param ri Ride to manage.
 */
GentleThrillRideManagerWindow::GentleThrillRideManagerWindow(GentleThrillRideInstance *ri) : GuiWindow(WC_GENTLE_THRILL_RIDE_MANAGER, ri->GetIndex())
{
	this->ride = ri;
	this->SetRideType(this->ride->GetGentleThrillRideType());
	this->SetupWidgetTree(_gentle_thrill_ride_manager_gui_parts, lengthof(_gentle_thrill_ride_manager_gui_parts));
	this->UpdateButtons();
	this->UpdateRecolourButtons();

	SetSelector(nullptr);
	entrance_exit_placement.cur_cursor = CUR_TYPE_INVALID;

	/* When opening the window of a newly built ride immediately prompt the user to place the entrance or exit. */
	if (this->ride->entrance_pos == XYZPoint16::invalid()) {
		ChooseEntranceExitClicked(true);
	} else if (this->ride->exit_pos == XYZPoint16::invalid()) {
		ChooseEntranceExitClicked(false);
	}
}

GentleThrillRideManagerWindow::~GentleThrillRideManagerWindow()
{
	SetSelector(nullptr);
}

assert_compile(MAX_RECOLOUR >= MAX_RIDE_RECOLOURS); ///< Check that the 3 recolourings of a gentle/thrill ride fit in the Recolouring::entries array.

/** Update all recolour buttons of the window. */
void GentleThrillRideManagerWindow::UpdateRecolourButtons()
{
	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->ride->recolours.entries[i];
		this->GetWidget<LeafWidget>(GTRMW_RECOLOUR1 + i)->SetShaded(!re.IsValid());
	}
	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->ride->entrance_recolours.entries[i];
		this->GetWidget<LeafWidget>(GTRMW_ENTRANCE_RECOLOUR1 + i)->SetShaded(!re.IsValid());
	}
	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->ride->exit_recolours.entries[i];
		this->GetWidget<LeafWidget>(GTRMW_EXIT_RECOLOUR1 + i)->SetShaded(!re.IsValid());
	}
	ResetSize();
}

/** Update all buttons of the window related to the ride's open/closed state. */
void GentleThrillRideManagerWindow::UpdateButtons()
{
	this->GetWidget<LeafWidget>(GTRMW_OPEN_RIDE_LIGHT )->shift = this->ride->state == RIS_OPEN   ? GS_LIGHT : GS_NIGHT;
	this->GetWidget<LeafWidget>(GTRMW_CLOSE_RIDE_LIGHT)->shift = this->ride->state == RIS_CLOSED ? GS_LIGHT : GS_NIGHT;
	this->SetWidgetShaded(GTRMW_OPEN_RIDE_LIGHT, !this->ride->CanOpenRide());

	this->SetWidgetShaded(GTRMW_PLACE_ENTRANCE, this->ride->state != RIS_CLOSED);
	this->SetWidgetShaded(GTRMW_PLACE_EXIT, this->ride->state != RIS_CLOSED);
	this->SetWidgetShaded(GTRMW_MAINTENANCE_DECREASE, this->ride->maintenance_interval <= 0);
	this->SetWidgetShaded(GTRMW_ENTRANCE_FEE_DECREASE, this->ride->item_price[0] <= 0);
	this->SetWidgetShaded(GTRMW_CYCLES_DECREASE,
			this->ride->state != RIS_CLOSED || this->ride->working_cycles <= this->ride->GetGentleThrillRideType()->working_cycles_min);
	this->SetWidgetShaded(GTRMW_CYCLES_INCREASE,
			this->ride->state != RIS_CLOSED || this->ride->working_cycles >= this->ride->GetGentleThrillRideType()->working_cycles_max);
	this->SetWidgetShaded(GTRMW_MIN_IDLE_DECREASE, this->ride->state != RIS_CLOSED || this->ride->min_idle_duration <= 0);
	this->SetWidgetShaded(GTRMW_MIN_IDLE_INCREASE, this->ride->state != RIS_CLOSED || this->ride->min_idle_duration >= this->ride->max_idle_duration);
	this->SetWidgetShaded(GTRMW_MAX_IDLE_DECREASE, this->ride->state != RIS_CLOSED || this->ride->max_idle_duration <= this->ride->min_idle_duration);
	this->SetWidgetShaded(GTRMW_MAX_IDLE_INCREASE, this->ride->state != RIS_CLOSED);
}

void GentleThrillRideManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case GTRMW_TITLEBAR:
			_str_params.SetText(1, _language.GetSgText(this->ride->GetKind() == RTK_GENTLE ? GUI_GENTLE_RIDES_MANAGER_TITLE : GUI_THRILL_RIDES_MANAGER_TITLE));
			_str_params.SetText(2, this->ride->name);
			break;

		case GTRMW_MONTHLY_COST:
			_str_params.SetMoney(1, this->ride->GetRideType()->monthly_cost + (this->ride->state == RIS_OPEN ? this->ride->GetRideType()->monthly_open_cost : Money(0)));
			break;
		case GTRMW_EXCITEMENT_RATING:
			SetRideRatingStringParam(this->ride->excitement_rating);
			break;
		case GTRMW_INTENSITY_RATING:
			SetRideRatingStringParam(this->ride->intensity_rating);
			break;
		case GTRMW_NAUSEA_RATING:
			SetRideRatingStringParam(this->ride->nausea_rating);
			break;

		case GTRMW_RELIABILITY:
			if (this->ride->broken) {
				_str_params.SetStrID(1, GUI_RIDE_MANAGER_BROKEN_DOWN);
			} else {
				_str_params.SetText(1, Format(GUI_RIDE_MANAGER_RELIABILITY, this->ride->reliability / 100.0));
			}
			break;

		case GTRMW_MAX_IDLE:
			_str_params.SetNumber(1, this->ride->max_idle_duration / 1000);
			break;
		case GTRMW_MIN_IDLE:
			_str_params.SetNumber(1, this->ride->min_idle_duration / 1000);
			break;
		case GTRMW_CYCLES:
			_str_params.SetNumber(1, this->ride->working_cycles);
			break;
		case GTRMW_ENTRANCE_FEE:
			_str_params.SetMoney(1, this->ride->item_price[0]);
			break;
		case GTRMW_MAINTENANCE:
			if (this->ride->maintenance_interval > 0) {
				_str_params.SetNumber(1, this->ride->maintenance_interval / (60 * 1000));
			} else {
				_str_params.SetStrID(1, GUI_RIDE_MANAGER_MAINTENANCE_NEVER);
			}
			break;

		case GTRMW_ENTRANCE_RECOLOUR1:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ride->entrance_type]->recolour_description_1);
			break;
		case GTRMW_ENTRANCE_RECOLOUR2:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ride->entrance_type]->recolour_description_2);
			break;
		case GTRMW_ENTRANCE_RECOLOUR3:
			_str_params.SetStrID(1, _rides_manager.entrances[this->ride->entrance_type]->recolour_description_3);
			break;
		case GTRMW_EXIT_RECOLOUR1:
			_str_params.SetStrID(1, _rides_manager.exits[this->ride->exit_type]->recolour_description_1);
			break;
		case GTRMW_EXIT_RECOLOUR2:
			_str_params.SetStrID(1, _rides_manager.exits[this->ride->exit_type]->recolour_description_2);
			break;
		case GTRMW_EXIT_RECOLOUR3:
			_str_params.SetStrID(1, _rides_manager.exits[this->ride->exit_type]->recolour_description_3);
			break;
	}
}

void GentleThrillRideManagerWindow::OnClick(WidgetNumber wid_num, [[maybe_unused]] const Point16 &pos)
{
	switch (wid_num) {
		case GTRMW_OPEN_RIDE_LIGHT:
		case GTRMW_OPEN_RIDE_PANEL:
			if (this->ride->CanOpenRide()) {
				this->ride->OpenRide();
				this->UpdateButtons();
			}
			break;

		case GTRMW_CLOSE_RIDE_LIGHT:
		case GTRMW_CLOSE_RIDE_PANEL:
			if (this->ride->state != RIS_CLOSED) {
				this->ride->CloseRide();
				this->UpdateButtons();
			}
			break;

		case GTRMW_RECOLOUR1:
		case GTRMW_RECOLOUR2:
		case GTRMW_RECOLOUR3: {
			RecolourEntry *re = &this->ride->recolours.entries[wid_num - GTRMW_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(wid_num, re, COL_RANGE_DARK_RED);
			}
			break;
		}
		case GTRMW_ENTRANCE_RECOLOUR1:
		case GTRMW_ENTRANCE_RECOLOUR2:
		case GTRMW_ENTRANCE_RECOLOUR3: {
			RecolourEntry *re = &this->ride->entrance_recolours.entries[wid_num - GTRMW_ENTRANCE_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(wid_num, re, COL_RANGE_DARK_RED);
			}
			break;
		}
		case GTRMW_EXIT_RECOLOUR1:
		case GTRMW_EXIT_RECOLOUR2:
		case GTRMW_EXIT_RECOLOUR3: {
			RecolourEntry *re = &this->ride->exit_recolours.entries[wid_num - GTRMW_EXIT_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(wid_num, re, COL_RANGE_DARK_RED);
			}
			break;
		}

		case GTRMW_ENTRANCE_FEE_INCREASE:
			this->ride->item_price[0] += RIDE_ENTRANCE_FEE_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_ENTRANCE_FEE_DECREASE:
			this->ride->item_price[0] = std::max<int>(0, this->ride->item_price[0] - RIDE_ENTRANCE_FEE_STEP_SIZE);
			this->UpdateButtons();
			break;
		case GTRMW_CYCLES_INCREASE:
			this->ride->working_cycles++;
			this->UpdateButtons();
			break;
		case GTRMW_CYCLES_DECREASE:
			this->ride->working_cycles--;
			this->UpdateButtons();
			break;
		case GTRMW_MAX_IDLE_INCREASE:
			this->ride->max_idle_duration += IDLE_DURATION_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_MAX_IDLE_DECREASE:
			this->ride->max_idle_duration -= IDLE_DURATION_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_MIN_IDLE_INCREASE:
			this->ride->min_idle_duration += IDLE_DURATION_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_MIN_IDLE_DECREASE:
			this->ride->min_idle_duration -= IDLE_DURATION_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_MAINTENANCE_INCREASE:
			this->ride->maintenance_interval += MAINTENANCE_INTERVAL_STEP_SIZE;
			this->UpdateButtons();
			break;
		case GTRMW_MAINTENANCE_DECREASE:
			this->ride->maintenance_interval -= MAINTENANCE_INTERVAL_STEP_SIZE;
			this->UpdateButtons();
			break;

		case GTRMW_PLACE_ENTRANCE:
			ChooseEntranceExitClicked(true);
			break;
		case GTRMW_PLACE_EXIT:
			ChooseEntranceExitClicked(false);
			break;

		case GTRMW_CHOOSE_ENTRANCE: {
			DropdownList itemlist;
			for (const auto &eetype : _rides_manager.entrances) {
				_str_params.SetText(1, _language.GetSgText(eetype->name));
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(wid_num, itemlist, this->ride->entrance_type, COL_RANGE_DARK_RED);
			break;
		}
		case GTRMW_CHOOSE_EXIT: {
			DropdownList itemlist;
			for (const auto &eetype : _rides_manager.exits) {
				_str_params.SetText(1, _language.GetSgText(eetype->name));
				itemlist.push_back(DropdownItem(STR_ARG1));
			}
			this->ShowDropdownMenu(wid_num, itemlist, this->ride->exit_type, COL_RANGE_DARK_RED);
			break;
		}

		case GTRMW_REMOVE:
			ShowGentleThrillRideRemove(this->ride);
			break;
	}
}

/**
 * Called when the Choose Entrance or Choose Exit button was clicked.
 * @param entrance Entrance or exit button.
 */
void GentleThrillRideManagerWindow::ChooseEntranceExitClicked(const bool entrance)
{
	this->ride->temp_entrance_pos = XYZPoint16::invalid();
	this->ride->temp_exit_pos = XYZPoint16::invalid();

	if (this->selector == nullptr || (this->is_placing_entrance != entrance)) {
		this->is_placing_entrance = entrance;
		SetSelector(&entrance_exit_placement);
	} else {
		SetSelector(nullptr);
	}

	entrance_exit_placement.SetSize(0, 0);
}

void GentleThrillRideManagerWindow::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	const Point32 world_pos = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);
	const int8 dx = _orientation_signum_dx[vp->orientation];
	const int8 dy = _orientation_signum_dy[vp->orientation];
	const int dz = (this->ride->vox_pos.z - (vp->view_pos.z / 256)) / 2;
	const XYZPoint16 location(world_pos.x / 256 + dz * dx, world_pos.y / 256 + dz * dy, this->ride->vox_pos.z);

	entrance_exit_placement.SetPosition(location.x, location.y);
	if (this->ride->CanPlaceEntranceOrExit(location, this->is_placing_entrance)) {
		if (this->is_placing_entrance) {
			this->ride->temp_entrance_pos = location;
		} else {
			this->ride->temp_exit_pos = location;
		}
		entrance_exit_placement.SetSize(1, 1);
		entrance_exit_placement.AddVoxel(location);
		entrance_exit_placement.SetupRideInfoSpace();
		entrance_exit_placement.SetRideData(location, static_cast<SmallRideInstance>(this->ride->GetIndex()), SHF_ENTRANCE_BITS);
	} else {
		if (this->is_placing_entrance) {
			this->ride->temp_entrance_pos = XYZPoint16::invalid();
		} else {
			this->ride->temp_exit_pos = XYZPoint16::invalid();
		}
		entrance_exit_placement.SetSize(0, 0);
	}
}

void GentleThrillRideManagerWindow::SelectorMouseButtonEvent(const MouseButtons state)
{
	if (state != MB_LEFT) return;
	if (entrance_exit_placement.area.width != 1 || entrance_exit_placement.area.height != 1) return;

	if (this->is_placing_entrance) {
		assert(this->ride->CanPlaceEntranceOrExit(this->ride->temp_entrance_pos, true));
		this->ride->SetEntrancePos(this->ride->temp_entrance_pos);
	} else {
		assert(this->ride->CanPlaceEntranceOrExit(this->ride->temp_exit_pos, false));
		this->ride->SetExitPos(this->ride->temp_exit_pos);
	}

	this->ride->temp_entrance_pos = XYZPoint16::invalid();
	this->ride->temp_exit_pos = XYZPoint16::invalid();
	SetSelector(nullptr);

	/* If the user is still in the process of building the ride, immediately prompt for the placement of the other side-building as well. */
	if (this->ride->entrance_pos == XYZPoint16::invalid()) {
		ChooseEntranceExitClicked(true);
	} else if (this->ride->exit_pos == XYZPoint16::invalid()) {
		ChooseEntranceExitClicked(false);
	}
	this->UpdateButtons();
}

void GentleThrillRideManagerWindow::OnChange(const ChangeCode code, const uint32 parameter)
{
	switch (code) {
		case CHG_DROPDOWN_RESULT:
			switch ((parameter >> 16) & 0xFF) {
				case GTRMW_CHOOSE_ENTRANCE:
					this->ride->SetEntranceType(parameter & 0xFF);
					this->UpdateRecolourButtons();
					break;
				case GTRMW_CHOOSE_EXIT:
					this->ride->SetExitType(parameter & 0xFF);
					this->UpdateRecolourButtons();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

/**
 * Open a window to manage a given gentle/thrill ride.
 * @param number Ride to manage.
 */
void ShowGentleThrillRideManagementGui(uint16 number)
{
	if (HighlightWindowByType(WC_GENTLE_THRILL_RIDE_MANAGER, number) != nullptr) return;

	RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr || (ri->GetKind() != RTK_GENTLE && ri->GetKind() != RTK_THRILL)) return;

	new GentleThrillRideManagerWindow(static_cast<GentleThrillRideInstance *>(ri));
}
