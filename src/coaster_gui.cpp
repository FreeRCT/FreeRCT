/* $Id$ */

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
#include "coaster_build.h"
#include "map.h"
#include "gui_sprites.h"

CoasterBuildMode _coaster_builder; ///< Coaster build mouse mode handler.

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

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	CoasterInstance *ci; ///< Roller coaster instance to display and control.
};

/**
 * Constructor of the roller coaster instance window.
 * @param ci Roller coaster instance to display and control.
 */
CoasterInstanceWindow::CoasterInstanceWindow(CoasterInstance *ci) : GuiWindow(WC_COASTER_MANAGER, ci->GetIndex())
{
	this->ci = ci;
	this->SetupWidgetTree(_coaster_instance_gui_parts, lengthof(_coaster_instance_gui_parts));
}

CoasterInstanceWindow::~CoasterInstanceWindow()
{
	if (!GetWindowByType(WC_COASTER_BUILD, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

void CoasterInstanceWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
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
	CoasterInstance *ci = static_cast<CoasterInstance *>(coaster);
	assert(ci != nullptr);

	RideInstanceState ris = ci->DecideRideState();
	if (ris == RIS_TESTING || ris == RIS_CLOSED || ris == RIS_OPEN) {
		if (HighlightWindowByType(WC_COASTER_MANAGER, coaster->GetIndex())) return;

		new CoasterInstanceWindow(ci);
		return;
	}
	ShowCoasterBuildGui(ci);
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
	CoasterBuildWindow(CoasterInstance *ci);
	~CoasterBuildWindow();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;

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
	int BuildTrackPiece();
};

/**
 * Constructor of the roller coaster build window. The provided instance may be completely empty.
 * @param ci Coaster instance to build or modify.
 */
CoasterBuildWindow::CoasterBuildWindow(CoasterInstance *ci) : GuiWindow(WC_COASTER_BUILD, ci->GetIndex())
{
	this->ci = ci;
	this->SetupWidgetTree(_coaster_construction_gui_parts, lengthof(_coaster_construction_gui_parts));
	_coaster_builder.OpenWindow(this->ci->GetIndex());
	_mouse_modes.SetMouseMode(MM_COASTER_BUILD);

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

	this->SetupSelection();
}

CoasterBuildWindow::~CoasterBuildWindow()
{
	_coaster_builder.CloseWindow(this->ci->GetIndex());

	if (!GetWindowByType(WC_COASTER_MANAGER, this->wnumber) && !this->ci->IsAccessible()) {
		_rides_manager.DeleteInstance(this->ci->GetIndex());
	}
}

void CoasterBuildWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CCW_TITLEBAR:
			_str_params.SetUint8(1, (uint8 *)this->ci->name);
			break;
	}
}

void CoasterBuildWindow::OnClick(WidgetNumber widget, const Point16 &pos)
{
	switch (widget) {
		case CCW_BANK_NONE:
		case CCW_BANK_LEFT:
		case CCW_BANK_RIGHT:
			this->sel_bank = (TrackPieceBanking)(widget - CCW_BANK_NONE);
			this->SetupSelection();
			break;

		case CCW_PLATFORM:
			this->sel_platform = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_TRUE;
			this->SetupSelection();
			break;

		case CCW_NO_PLATFORM:
			this->sel_platform = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_FALSE;
			this->SetupSelection();

		case CCW_POWERED:
			this->sel_power = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_TRUE;
			this->SetupSelection();
			break;

		case CCW_NOT_POWERED:
			this->sel_power = this->IsWidgetPressed(widget) ? BSL_NONE : BSL_FALSE;
			this->SetupSelection();
			break;

		case CCW_SLOPE_DOWN:
		case CCW_SLOPE_FLAT:
		case CCW_SLOPE_UP:
		case CCW_SLOPE_STEEP_DOWN:
		case CCW_SLOPE_STEEP_UP:
		case CCW_SLOPE_VERTICAL_DOWN:
		case CCW_SLOPE_VERTICAL_UP:
			this->sel_slope = (TrackSlope)(widget - CCW_SLOPE_DOWN);
			this->SetupSelection();
			break;

		case CCW_DISPLAY_PIECE: {
			int index = this->BuildTrackPiece();
			if (index >= 0) {
				/* Piece was added, change the setup for the next piece. */
				this->cur_piece = &this->ci->pieces[index];
				int succ = this->ci->FindSuccessorPiece(*this->cur_piece);
				this->cur_sel = (succ >= 0) ? &this->ci->pieces[succ] : nullptr;
				this->cur_after = true;
			}
			this->SetupSelection();
			break;
		}
		case CCW_REMOVE: {
			int pred_index = this->ci->FindPredecessorPiece(*this->cur_piece);
			_additions.Clear();
			this->ci->RemovePositionedPiece(*this->cur_piece);
			_additions.Commit();
			this->cur_piece = pred_index == -1 ? nullptr : &this->ci->pieces[pred_index];
			this->SetupSelection();
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
			this->SetupSelection();
			break;

		case CCW_ROT_NEG:
			if (this->cur_piece == nullptr) {
				this->build_direction = (TileEdge)((this->build_direction + 1) % 4);
				this->SetupSelection();
			}
			break;

		case CCW_ROT_POS:
			if (this->cur_piece == nullptr) {
				this->build_direction = (TileEdge)((this->build_direction + 3) % 4);
				this->SetupSelection();
			}
			break;
	}
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
	enabled = (this->cur_piece != nullptr && this->cur_sel != nullptr);
	this->SetWidgetShaded(CCW_BACKWARD, !enabled);
	this->SetWidgetShaded(CCW_FORWARD,  !enabled);
	enabled = (this->cur_piece != nullptr && this->cur_sel == nullptr);
	this->SetWidgetShaded(CCW_DISPLAY_PIECE, !enabled);
	this->SetWidgetShaded(CCW_REMOVE, !enabled);

	this->sel_bank = static_cast<TrackPieceBanking>(this->SetButtons(CCW_BANK_NONE, TPB_COUNT, avail_bank, this->sel_bank, TPB_INVALID));
	this->sel_slope = static_cast<TrackSlope>(this->SetButtons(CCW_SLOPE_DOWN, TSL_COUNT_VERTICAL, avail_slope, this->sel_slope, TSL_INVALID));
	this->sel_bend = static_cast<TrackBend>(this->SetButtons(CCW_BEND_WIDE_LEFT, TBN_COUNT, avail_bend, this->sel_bend, TBN_INVALID));
	this->sel_platform = static_cast<BoolSelect>(this->SetButtons(CCW_NO_PLATFORM, 2, avail_platform, this->sel_platform, BSL_NONE));
	this->sel_power = static_cast<BoolSelect>(this->SetButtons(CCW_NOT_POWERED, 2, avail_power, this->sel_power, BSL_NONE));

	if (this->sel_piece != nullptr) {
		if (this->cur_piece == nullptr) {
			_coaster_builder.SelectPosition(this->wnumber, this->sel_piece, this->build_direction);
		} else {
			if (this->cur_after) {
				TileEdge dir = (TileEdge)(this->cur_piece->piece->exit_connect & 3); /// \todo Define this in the data format
				_coaster_builder.DisplayPiece(this->wnumber, this->sel_piece, this->cur_piece->GetEndX(),
						this->cur_piece->GetEndY(), this->cur_piece->GetEndZ(), dir);
			}
			// XXX else: display before start.
		}
	} else {
		_coaster_builder.ShowNoPiece(this->wnumber);
	}
}

/**
 * Create the currently selected track piece in the world.
 * @return Position of the new piece in the coaster instance, or \c -1.
 */
int CoasterBuildWindow::BuildTrackPiece()
{
	if (this->sel_piece == nullptr) return -1;
	PositionedTrackPiece ptp(_coaster_builder.track_xpos, _coaster_builder.track_ypos,
			_coaster_builder.track_zpos, this->sel_piece);
	if (!ptp.CanBePlaced()) return -1;

	/* Add the piece to the coaster instance. */
	int ptp_index = this->ci->AddPositionedPiece(ptp);
	if (ptp_index >= 0) {
		/* Add the piece to the world. */
		_additions.Clear();
		this->ci->PlaceTrackPieceInAdditions(ptp);
		_additions.Commit();
	}
	return ptp_index;
}

void CoasterBuildWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code != CHG_PIECE_POSITIONED || parameter != 0) return;

	int index = this->BuildTrackPiece();
	if (index >= 0) {
		/* Piece was added, change the setup for the next piece. */
		this->cur_piece = &this->ci->pieces[index];
		int succ = this->ci->FindSuccessorPiece(*this->cur_piece);
		this->cur_sel = (succ >= 0) ? &this->ci->pieces[succ] : nullptr;
		this->cur_after = true;
	}
	this->SetupSelection();
}

/**
 * Open a roller coaster build/edit window for the given roller coaster.
 * @param coaster Coaster instance to modify.
 */
void ShowCoasterBuildGui(CoasterInstance *coaster)
{
	if (coaster->GetKind() != RTK_COASTER) return;
	if (HighlightWindowByType(WC_COASTER_BUILD, coaster->GetIndex())) return;

	new CoasterBuildWindow(coaster);
}

/** State in the coaster build mouse mode. */
class CoasterBuildState {
public:
	/**
	 * Constructor of a build mouse mode state.
	 * @param mode Mouse mode reference.
	 */
	CoasterBuildState(CoasterBuildMode &mode) : mode(&mode)
	{
	}

	/**
	 * Notification to the mouse mode that a coaster construction window has been opened.
	 * @param instance Ride number of the window.
	 */
	virtual void OpenWindow(uint16 instance) const = 0;

	/**
	 * Notification to the mouse mode that a coaster construction window has been closed.
	 * @param instance Ride number of the window.
	 */
	virtual void CloseWindow(uint16 instance) const = 0;

	/**
	 * Query from the viewport whether the mouse mode may be activated.
	 * @return The mouse mode may be activated.
	 * @see MouseMode::MayActivateMode
	 */
	virtual bool MayActivateMode() const = 0;

	/**
	 * Notification from the viewport that the mouse mode has been activated.
	 * @param pos Current mouse position.
	 * @see MouseMode::ActivateMode
	 */
	virtual void ActivateMode(const Point16 &pos) const = 0;

	/**
	 * Notification from the viewport that the mouse has moved.
	 * @param vp The viewport.
	 * @param old_pos Previous position of the mouse.
	 * @param pos Current mouse position.
	 * @see MouseMode::OnMouseMoveEvent
	 */
	virtual void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const = 0;

	/**
	 * Notification from the viewport that a mouse button has changed value.
	 * @param vp The viewport.
	 * @param state Old and new state of the mouse buttons.
	 * @see MouseMode::OnMouseButtonEvent
	 */
	virtual void OnMouseButtonEvent(Viewport *vp, uint8 state) const = 0;

	/**
	 * Notification from the viewport that the mouse mode has been de-activated.
	 * @see MouseMode::LeaveMode
	 */
	virtual void LeaveMode() const = 0;

	/**
	 * Query from the viewport whether the mouse mode wants to have cursors displayed.
	 * @return Cursors should be enabled.
	 * @see MouseMode::EnableCursors
	 */
	virtual bool EnableCursors() const = 0;

	/**
	 * Notification from the construction window to display no track piece.
	 * @param instance Ride number of the window.
	 */
	virtual void ShowNoPiece(uint16 instance) const = 0;

	/**
	 * Notification from the construction window to display a track piece attached to the mouse cursor.
	 * @param instance Ride number of the window.
	 * @param piece Track piece to display.
	 * @param direction Direction of building (to use with a cursor).
	 */
	virtual void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const = 0;

	/**
	 * Notification from the construction window to display a track piece at a given position.
	 * @param instance Ride number of the window.
	 * @param piece Track piece to display.
	 * @param x X position of the piece.
	 * @param y Y position of the piece.
	 * @param z Z position of the piece.
	 * @param direction Direction of building (to use with a cursor).
	 */
	virtual void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const = 0;

	CoasterBuildMode * const mode; ///< Coaster build mouse mode.
};

/** State when no coaster construction window is opened. */
class CoasterBuildStateOff : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateOff state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateOff(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
		mode->instance = instance;
		mode->SetNoPiece();
		mode->DisableDisplay();
		mode->SetState(BS_STARTING);
	}

	void CloseWindow(uint16 instance) const override
	{
	}

	bool MayActivateMode() const override
	{
		return false;
	}

	void ActivateMode(const Point16 &pos) const override
	{
		NOT_REACHED(); // Should never happen.
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
	}

	void LeaveMode() const override
	{
	}

	bool EnableCursors() const override
	{
		return false;
	}

	void ShowNoPiece(uint16 instance) const override
	{
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
	}
};

/** State with opened window, but no active mouse mode yet. */
class CoasterBuildStateStarting : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateStarting state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateStarting(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
		mode->instance = instance; // Nothing has happened yet, so switching can still be done.
	}

	void CloseWindow(uint16 instance) const override
	{
		if (mode->instance != instance) return;

		mode->instance = INVALID_RIDE_INSTANCE;
		mode->SetState(BS_OFF);
	}

	bool MayActivateMode() const override
	{
		return true;
	}

	void ActivateMode(const Point16 &pos) const override
	{
		mode->EnableDisplay();
		mode->UpdateDisplay(false);
		BuilderState new_state;
		if (mode->cur_piece == nullptr) {
			new_state = BS_ON;
		} else if (mode->use_mousepos) {
			new_state = BS_MOUSE;
		} else {
			new_state = BS_FIXED;
		}
		mode->SetState(new_state);
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
	}

	void LeaveMode() const override
	{
	}

	bool EnableCursors() const override
	{
		return false;
	}

	void ShowNoPiece(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->SetNoPiece();
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetSelectPosition(piece, direction);
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetFixedPiece(piece, x, y, z, direction);
	}
};

/** State for display a not-available track piece, or suppressing display of an available track piece. */
class CoasterBuildStateOn : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateOn state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateOn(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
	}

	void CloseWindow(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_STARTING);
		_mouse_modes.SetViewportMousemode(); // Select another mouse mode.
	}

	bool MayActivateMode() const override
	{
		return true; // But is already enabled, doesn't matter.
	}

	void ActivateMode(const Point16 &pos) const override
	{
		mode->SetMousePosition(pos);
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
		mode->SetMousePosition(pos); // Nothing is displayed.
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
		/* Nothing is displayed. */
	}

	void LeaveMode() const override
	{
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_STARTING);
	}

	bool EnableCursors() const override
	{
		return false;
	}

	void ShowNoPiece(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->SetNoPiece();
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetSelectPosition(piece, direction);
		mode->UpdateDisplay(false);
		mode->SetState(BS_MOUSE);
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetFixedPiece(piece, x, y, z, direction);
		mode->UpdateDisplay(false);
		mode->SetState(BS_FIXED);
	}
};

/** State to display a track piece at the position of the mouse (at the ground). */
class CoasterBuildStateMouse : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateMouse state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateMouse(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
	}

	void CloseWindow(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_DOWN);
		_mouse_modes.SetViewportMousemode(); // Select another mouse mode.
	}

	bool MayActivateMode() const override
	{
		return true;
	}

	void ActivateMode(const Point16 &pos) const override
	{
		mode->SetMousePosition(pos);
		mode->UpdateDisplay(true);
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
		mode->SetMousePosition(pos);
		mode->UpdateDisplay(true);
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
		PositionedTrackPiece ptp(mode->track_xpos, mode->track_ypos, mode->track_zpos, mode->cur_piece);
		if (ptp.CanBePlaced()) {
			mode->SetNoPiece();
			mode->UpdateDisplay(false);
			NotifyChange(WC_COASTER_BUILD, mode->instance, CHG_PIECE_POSITIONED, 0);
			mode->SetState(BS_ON);
		}
	}

	void LeaveMode() const override
	{
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_STARTING);
	}

	bool EnableCursors() const override
	{
		return true;
	}

	void ShowNoPiece(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->SetNoPiece();
		mode->UpdateDisplay(false);
		mode->SetState(BS_ON);
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetSelectPosition(piece, direction);
		mode->UpdateDisplay(false);
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetFixedPiece(piece, x, y, z, direction);
		mode->UpdateDisplay(false);
		mode->SetState(BS_FIXED);
	}
};

/** Display a track piece at a fixed position in the world. */
class CoasterBuildStateFixed : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateFixed state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateFixed(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
	}

	void CloseWindow(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_DOWN);
		_mouse_modes.SetViewportMousemode(); // Select another mouse mode.
	}

	bool MayActivateMode() const override
	{
		return true; // But is already enabled, doesn't matter.
	}

	void ActivateMode(const Point16 &pos) const override
	{
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
	}

	void LeaveMode() const override
	{
		mode->DisableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_STARTING);
	}

	bool EnableCursors() const override
	{
		return true;
	}

	void ShowNoPiece(uint16 instance) const override
	{
		if (mode->instance != instance) return;
		mode->SetNoPiece();
		mode->UpdateDisplay(false);
		mode->SetState(BS_ON);
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetSelectPosition(piece, direction);
		mode->UpdateDisplay(false);
		mode->SetState(BS_MOUSE);
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
		if (mode->instance != instance) return;
		mode->SetFixedPiece(piece, x, y, z, direction);
		mode->UpdateDisplay(false);
	}
};

/** Stopping state, mouse mode wants to deactivate, but has to wait until it gets permission to do so. */
class CoasterBuildStateStopping : public CoasterBuildState {
public:
	/**
	 * Constructor of the #CoasterBuildStateStopping state.
	 * @param mode Builder mode.
	 */
	CoasterBuildStateStopping(CoasterBuildMode &mode) : CoasterBuildState(mode)
	{
	}

	void OpenWindow(uint16 instance) const override
	{
		mode->instance = instance;
		mode->SetNoPiece();
		mode->EnableDisplay();
		mode->UpdateDisplay(false);
		mode->SetState(BS_ON);
	}

	void CloseWindow(uint16 instance) const override
	{
	}

	bool MayActivateMode() const override
	{
		return false;
	}

	void ActivateMode(const Point16 &pos) const override
	{
		mode->SetMousePosition(pos);
	}

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) const override
	{
	}

	void OnMouseButtonEvent(Viewport *vp, uint8 state) const override
	{
	}

	void LeaveMode() const override
	{
		mode->SetState(BS_OFF);
	}

	bool EnableCursors() const override
	{
		return false;
	}

	void ShowNoPiece(uint16 instance) const override
	{
	}

	void SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction) const override
	{
	}

	void DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction) const override
	{
	}
};

static const CoasterBuildStateOff      _coaster_build_state_off(_coaster_builder);      ///< Off state of the coaster build mouse mode.
static const CoasterBuildStateStarting _coaster_build_state_starting(_coaster_builder); ///< Starting state of the coaster build mouse mode.
static const CoasterBuildStateOn       _coaster_build_state_on(_coaster_builder);       ///< On state of the coaster build mouse mode.
static const CoasterBuildStateMouse    _coaster_build_state_mouse(_coaster_builder);    ///< Select position state of the coaster build mouse mode.
static const CoasterBuildStateFixed    _coaster_build_state_fixed(_coaster_builder);    ///< Show fixed position state of the coaster build mouse mode.
static const CoasterBuildStateStopping _coaster_build_state_stopping(_coaster_builder); ///< Stopping state of the coaster build mouse mode.

/** Available states of the coaster build mouse mode. */
static const CoasterBuildState *_coaster_build_states[] = {
	&_coaster_build_state_off,      // BS_OFF
	&_coaster_build_state_starting, // BS_STARTING
	&_coaster_build_state_on,       // BS_ON
	&_coaster_build_state_mouse,    // BS_MOUSE
	&_coaster_build_state_fixed,    // BS_FIXED
	&_coaster_build_state_stopping, // BS_DOWN
};

CoasterBuildMode::CoasterBuildMode() : MouseMode(WC_COASTER_BUILD, MM_COASTER_BUILD)
{
	this->instance = INVALID_RIDE_INSTANCE;
	this->state = BS_OFF;
	this->mouse_state = 0;
	this->SetNoPiece();
}

/**
 * Notification to the mouse mode that a coaster construction window has been opened.
 * @param instance Ride number of the window.
 */
void CoasterBuildMode::OpenWindow(uint16 instance)
{
	_coaster_build_states[this->state]->OpenWindow(instance);
}

/**
 * Notification to the mouse mode that a coaster construction window has been closed.
 * @param instance Ride number of the window.
 */
void CoasterBuildMode::CloseWindow(uint16 instance)
{
	_coaster_build_states[this->state]->CloseWindow(instance);
}

/**
 * Notification from the construction window to display no track piece.
 * @param instance Ride number of the window.
 */
void CoasterBuildMode::ShowNoPiece(uint16 instance)
{
	_coaster_build_states[this->state]->ShowNoPiece(instance);
}

/**
 * Notification from the construction window to display a track piece attached to the mouse cursor.
 * @param instance Ride number of the window.
 * @param piece Track piece to display.
 * @param direction Direction of building (to use with a cursor).
 */
void CoasterBuildMode::SelectPosition(uint16 instance, ConstTrackPiecePtr piece, TileEdge direction)
{
	_coaster_build_states[this->state]->SelectPosition(instance, piece, direction);
}

/**
 * Notification from the construction window to display a track piece at a given position.
 * @param instance Ride number of the window.
 * @param piece Track piece to display.
 * @param x X position of the piece.
 * @param y Y position of the piece.
 * @param z Z position of the piece.
 * @param direction Direction of building (to use with a cursor).
 */
void CoasterBuildMode::DisplayPiece(uint16 instance, ConstTrackPiecePtr piece, int x, int y, int z, TileEdge direction)
{
	_coaster_build_states[this->state]->DisplayPiece(instance, piece, x, y, z, direction);
}

/**
 * Query from the viewport whether the mouse mode may be activated.
 * @return The mouse mode may be activated.
 * @see MouseMode::MayActivateMode
 */
bool CoasterBuildMode::MayActivateMode()
{
	return _coaster_build_states[this->state]->MayActivateMode();
}

/**
 * Notification from the viewport that the mouse mode has been activated.
 * @param pos Current mouse position.
 * @see MouseMode::ActivateMode
 */
void CoasterBuildMode::ActivateMode(const Point16 &pos)
{
	_coaster_build_states[this->state]->ActivateMode(pos);
}

/**
 * Notification from the viewport that the mouse mode has been de-activated.
 * @see MouseMode::LeaveMode
 */
void CoasterBuildMode::LeaveMode()
{
	_coaster_build_states[this->state]->LeaveMode();
}

/**
 * Notification from the viewport that the mouse has moved.
 * @param vp The viewport.
 * @param old_pos Previous position of the mouse.
 * @param pos Current mouse position.
 * @see MouseMode::OnMouseMoveEvent
 */
void CoasterBuildMode::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	if ((this->mouse_state & MB_RIGHT) != 0) {
		/* Drag the window if button is pressed down. */
		Viewport *vp = GetViewport();
		if (vp != nullptr) vp->MoveViewport(pos.x - old_pos.x, pos.y - old_pos.y);
	}
	_coaster_build_states[this->state]->OnMouseMoveEvent(vp, old_pos, pos);
}

/**
 * Notification from the viewport that a mouse button has changed value.
 * @param vp The viewport.
 * @param state Old and new state of the mouse buttons.
 * @see MouseMode::OnMouseButtonEvent
 */
void CoasterBuildMode::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;
	_coaster_build_states[this->state]->OnMouseButtonEvent(vp, state);
}

/**
 * Query from the viewport whether the mouse mode wants to have cursors displayed.
 * @return Cursors should be enabled.
 * @see MouseMode::EnableCursors
 */
bool CoasterBuildMode::EnableCursors()
{
	return _coaster_build_states[this->state]->EnableCursors();
}

/**
 * Update the displayed position.
 * @param mousepos_changed If set, check the mouse position to x/y/z again (and only update the display if it is different).
 */
void CoasterBuildMode::UpdateDisplay(bool mousepos_changed)
{
	if (this->suppress_display || this->cur_piece == nullptr) {
		DisableWorldAdditions();
		return;
	}

	Viewport *vp = GetViewport();
	if (use_mousepos) {
		FinderData fdata(CS_GROUND, FW_TILE);
		if (vp->ComputeCursorPosition(&fdata) != CS_GROUND) {
			DisableWorldAdditions();
			return;
		}
		/* Found ground, is the position the same? */
		if (mousepos_changed && fdata.xvoxel == this->track_xpos && fdata.yvoxel == this->track_ypos && fdata.zvoxel == this->track_zpos) {
			return;
		}

		this->track_xpos = fdata.xvoxel;
		this->track_ypos = fdata.yvoxel;
		this->track_zpos = fdata.zvoxel;
	}
	_additions.Clear();
	EnableWorldAdditions();
	PositionedTrackPiece ptp(this->track_xpos, this->track_ypos, this->track_zpos, this->cur_piece);
	CoasterInstance *ci = static_cast<CoasterInstance *>(_rides_manager.GetRideInstance(this->instance));
	if (ptp.CanBePlaced()) ci->PlaceTrackPieceInAdditions(ptp);
	vp->arrow_cursor.SetCursor(this->track_xpos, this->track_ypos, this->track_zpos, (CursorType)(CUR_TYPE_ARROW_NE + this->direction));
}
