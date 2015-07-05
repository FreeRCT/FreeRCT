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

		for (const TrackVoxel *tv : this->pos_piece.piece->track_voxels) this->AddVoxel(this->pos_piece.base_voxel + tv->dxyz);

		this->SetupRideInfoSpace();
		for (const TrackVoxel *tv : this->pos_piece.piece->track_voxels) {
			XYZPoint16 p(this->pos_piece.base_voxel + tv->dxyz);
			this->SetRideData(p, this->ci->GetRideNumber(), this->ci->GetInstanceData(tv));
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
	CoasterBuildWindow(CoasterInstance *ci);
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

	TrackPieceMouseMode piece_selector; ///< Selector for displaying new track pieces.
};

/**
 * Constructor of the roller coaster build window. The provided instance may be completely empty.
 * @param ci Coaster instance to build or modify.
 */
CoasterBuildWindow::CoasterBuildWindow(CoasterInstance *ci) : GuiWindow(WC_COASTER_BUILD, ci->GetIndex()), piece_selector(ci)
{
	this->ci = ci;
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

	if (this->sel_piece == nullptr) {
		this->piece_selector.SetSize(0, 0); // Nothing to display.
		this->piece_selector.pos_piece.piece = nullptr;
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

void CoasterBuildWindow::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
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
		int succ = this->ci->FindSuccessorPiece(*this->cur_piece);
		this->cur_sel = (succ >= 0) ? &this->ci->pieces[succ] : nullptr;
		this->cur_after = true;
	}
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
