/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform_gui.cpp Terraforming gui code. */

#include <optional>
#include "stdafx.h"
#include "window.h"
#include "viewport.h"
#include "terraform.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "gui_sprites.h"
#include "mouse_mode.h"

static const int TERRAFORM_MAX_SIZE = 9;      ///< Maximum length of tiles for terraforming (both X and Y).
static const int TERRAFORM_ELEMENT_SIZE = 16; ///< Horizontal size of a tile in the display (pixels).

/** GUI for setting properties for terraforming. */
class TerraformGui : public GuiWindow {
public:
	TerraformGui();
	~TerraformGui();

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;

	void SelectorMouseWheelEvent(int direction) override;
	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(MouseButtons state) override;

	bool level; ///< If true, level the area, else move it up/down as-is.
	int xsize;  ///< Size of the terraform area in horizontal direction.
	int ysize;  ///< Size of the terraform area in vertical direction.
	std::optional<TileOwner> change_owner;  ///< Ownership to apply to the selected area.

private:
	void SetLevelling(bool level);
	void SetChangeOwner(std::optional<TileOwner> owner);
	void SetTerraformSize(int xs = -1, int ys = -1);
	void IncreaseSize();
	void DecreaseSize();
	void DoChangeLandOwner();

	CursorMouseMode tiles_selector;  ///< %Selector for displaying/handling tile(s).
};

/** Widget numbers of the terraform GUI. */
enum TerraformWidgets {
	TERR_DISPLAY,    ///< Widget displaying the current terraform size.
	TERR_ADD,        ///< 'Increase' button.
	TERR_SUB,        ///< 'Decrease' button.
	TERR_LEVEL,      ///< Level the terraform area.
	TERR_LEVEL_TEXT, ///< Text of the 'level' radio button.
	TERR_MOVE,       ///< Move the terraform area.
	TERR_MOVE_TEXT,  ///< Textof the 'move' radio button.
	TERR_OWNER_PANEL,           ///< Land ownership panel.
	TERR_OWNER_UNCHANGED,       ///< Do not change land ownership radio button.
	TERR_OWNER_UNCHANGED_TEXT,  ///< Do not change land ownership label.
	TERR_OWNER_NONE,            ///< Set land ownership to None radio button.
	TERR_OWNER_NONE_TEXT,       ///< Set land ownership to None label.
	TERR_OWNER_PARK,            ///< Set land ownership to Own Park radio button.
	TERR_OWNER_PARK_TEXT,       ///< Set land ownership to Own Park label.
	TERR_OWNER_SALE,            ///< Set land ownership to For Sale radio button.
	TERR_OWNER_SALE_TEXT,       ///< Set land ownership to For Sale label.
};

/** Widget parts of the #TerraformGui window. */
static const WidgetPart _terraform_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(1, 2),
				Widget(WT_EMPTY, TERR_DISPLAY, COL_RANGE_INVALID), SetMinimalSize(200, 120),
				Intermediate(2, 1),
					Widget(WT_TEXT_PUSHBUTTON, TERR_ADD, COL_RANGE_DARK_GREEN),
							SetData(GUI_TERRAFORM_ADD_TEXT, GUI_TERRAFORM_ADD_TOOLTIP),
					Widget(WT_TEXT_PUSHBUTTON, TERR_SUB, COL_RANGE_DARK_GREEN),
							SetData(GUI_TERRAFORM_SUB_TEXT, GUI_TERRAFORM_SUB_TOOLTIP),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(2, 2),
				Widget(WT_RADIOBUTTON, TERR_LEVEL, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_LEVEL_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_LEVEL_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, TERR_MOVE, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_MOVE_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_MOVE_TEXT, STR_NULL),
		Widget(WT_PANEL, TERR_OWNER_PANEL, COL_RANGE_DARK_GREEN),
			Intermediate(4, 2),
				Widget(WT_RADIOBUTTON, TERR_OWNER_UNCHANGED, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_OWNER_UNCHANGED_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_OWNER_UNCHANGED_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, TERR_OWNER_NONE, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_OWNER_NONE_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_OWNER_NONE_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, TERR_OWNER_PARK, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_OWNER_PARK_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_OWNER_PARK_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, TERR_OWNER_SALE, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, TERR_OWNER_SALE_TEXT, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_OWNER_SALE_TEXT, STR_NULL),
	EndContainer(),
};

TerraformGui::TerraformGui() : GuiWindow(WC_TERRAFORM, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_terraform_gui_parts, lengthof(_terraform_gui_parts));
	this->tiles_selector.default_enable_cursors = true;
	this->SetLevelling(true);
	this->SetChangeOwner(std::nullopt);
	this->SetTerraformSize(1, 1);
}

TerraformGui::~TerraformGui()
{
	this->SetSelector(nullptr);
}

void TerraformGui::SelectorMouseWheelEvent(int direction)
{
	if (this->selector == nullptr) return;

	if (this->xsize <= 1 && this->ysize <= 1) { // 'dot' mode, or single tile mode..
		ChangeTileCursorMode(this->tiles_selector.area.base, this->tiles_selector.cur_cursor, this->level, direction, (this->xsize == 0 && this->ysize == 0));
	} else {
		ChangeAreaCursorMode(this->tiles_selector.area, this->level, direction);
	}
	this->tiles_selector.InitTileData();
}

void TerraformGui::SelectorMouseMoveEvent(Viewport *vp, [[maybe_unused]] const Point16 &pos)
{
	if (this->selector == nullptr) return;

	FinderData fdata(CS_GROUND, (this->xsize <= 1 && this->ysize <= 1) ? FW_CORNER : FW_TILE);
	if (vp->ComputeCursorPosition(&fdata) != CS_GROUND) return;
	Rectangle16 &sel_rect = this->tiles_selector.area;
	int xsel = sel_rect.base.x + sel_rect.width / 2;
	int ysel = sel_rect.base.y + sel_rect.height / 2;
	if (fdata.cursor == this->tiles_selector.cur_cursor && fdata.voxel_pos.x == xsel && fdata.voxel_pos.y == ysel) return;

	this->tiles_selector.cur_cursor = fdata.cursor; // Copy cursor and position.
	this->tiles_selector.SetPosition(fdata.voxel_pos.x - sel_rect.width / 2, fdata.voxel_pos.y - sel_rect.height / 2);

	if (_video.GetMouseDragging()) this->DoChangeLandOwner();
}

void TerraformGui::SelectorMouseButtonEvent(MouseButtons state)
{
	if (state == MB_LEFT) {
		_video.SetMouseDragging(state, true, false);
		this->DoChangeLandOwner();
	}
}

/** Change the land ownership in the current area if applicable. */
void TerraformGui::DoChangeLandOwner()
{
	if (this->selector == nullptr || this->xsize < 1 || this->ysize < 1 || !_game_mode_mgr.InEditorMode()) return;

	if (this->change_owner.has_value()) {
		_world.SetTileOwnerRect(this->tiles_selector.area.base.x, this->tiles_selector.area.base.y,
				this->tiles_selector.area.width, this->tiles_selector.area.height, *this->change_owner);
	}
}

void TerraformGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != TERR_DISPLAY) return;
	Point32 base;

	/* Draw the tile area, case of a null-area. */
	if (this->xsize == 0 && this->ysize == 0) {
		const ImageData *dot = _gui_sprites.dot_sprite;
		if (dot == nullptr) return;

		base.x = this->GetWidgetScreenX(wid) + (wid->pos.width - dot->width) / 2;
		base.y = this->GetWidgetScreenY(wid) + (wid->pos.height - dot->height) / 2;
		_video.BlitImage(base, dot);
		return;
	}

	/* Draw area with >= 1 tile.
	 * Here, size == xsize, but also (size / 2)  == ysize, since xsize == ysize * 2 in a flat tile world.
	 */
	int size = this->xsize * TERRAFORM_ELEMENT_SIZE / 2 + this->ysize * TERRAFORM_ELEMENT_SIZE / 2;
	base.x = this->GetWidgetScreenX(wid) + (wid->pos.width - size) / 2; // Left position of the drawn tiles.
	base.y = this->GetWidgetScreenY(wid) + (wid->pos.height - size / 2) / 2; // Top position of the drawn tiles.
	base.y += this->xsize * TERRAFORM_ELEMENT_SIZE / 4; // Lower to (0, 0) of the drawn tiles (left-most position).

	for (int x = 0; x < this->xsize; x++) {
		for (int y = 0; y < this->ysize; y++) {
			Point16 left(base.x + (x + y) * TERRAFORM_ELEMENT_SIZE / 2, base.y + (y - x) * TERRAFORM_ELEMENT_SIZE / 4);

			Point16 top(left.x + TERRAFORM_ELEMENT_SIZE / 2, left.y - TERRAFORM_ELEMENT_SIZE / 4);
			_video.DrawLine(left, top, MakeRGBA(255, 255, 255, OPAQUE));

			Point16 bottom(top.x, left.y + TERRAFORM_ELEMENT_SIZE / 4);
			_video.DrawLine(left, bottom, MakeRGBA(255, 255, 255, OPAQUE));
			left.x += TERRAFORM_ELEMENT_SIZE; // Move 'left' to the right.
			_video.DrawLine(top, left, MakeRGBA(255, 255, 255, OPAQUE));
			_video.DrawLine(bottom, left, MakeRGBA(255, 255, 255, OPAQUE));
		}
	}
}

void TerraformGui::OnClick(WidgetNumber wid, [[maybe_unused]] const Point16 &pos)
{
	switch (wid) {
		case TERR_ADD:
			this->IncreaseSize();
			break;

		case TERR_SUB:
			this->DecreaseSize();
			break;

		case TERR_LEVEL_TEXT:
		case TERR_LEVEL:
			if (!this->level) this->SetLevelling(true);
			break;

		case TERR_MOVE_TEXT:
		case TERR_MOVE:
			if (this->level) this->SetLevelling(false);
			break;

		case TERR_OWNER_UNCHANGED_TEXT:
		case TERR_OWNER_UNCHANGED:
			if (_game_mode_mgr.InEditorMode()) this->SetChangeOwner(std::nullopt);
			break;
		case TERR_OWNER_NONE_TEXT:
		case TERR_OWNER_NONE:
			if (_game_mode_mgr.InEditorMode()) this->SetChangeOwner(OWN_NONE);
			break;
		case TERR_OWNER_PARK_TEXT:
		case TERR_OWNER_PARK:
			if (_game_mode_mgr.InEditorMode()) this->SetChangeOwner(OWN_PARK);
			break;
		case TERR_OWNER_SALE_TEXT:
		case TERR_OWNER_SALE:
			if (_game_mode_mgr.InEditorMode()) this->SetChangeOwner(OWN_FOR_SALE);
			break;

		default:
			break;
	}
}

/**
 * Set mode of terraforming:
 * - 'levelling': Lowest part up, or highest part up.
 * - 'moving':    Move entire area up or down.
 * @param level If \c true, set levelling mode, else set moving mode.
 */
void TerraformGui::SetLevelling(bool level)
{
	this->level = level;

	this->SetWidgetCheckedAndPressed(TERR_LEVEL, this->level);
	this->SetWidgetCheckedAndPressed(TERR_MOVE, !this->level);
}

void TerraformGui::SetChangeOwner(std::optional<TileOwner> owner)
{
	this->change_owner = owner;

	this->GetWidget<LeafWidget>(TERR_OWNER_PANEL)->SetVisible(this, _game_mode_mgr.InEditorMode());
	this->SetWidgetCheckedAndPressed(TERR_OWNER_UNCHANGED, !this->change_owner.has_value());
	this->SetWidgetCheckedAndPressed(TERR_OWNER_NONE, this->change_owner.has_value() && this->change_owner == OWN_NONE);
	this->SetWidgetCheckedAndPressed(TERR_OWNER_PARK, this->change_owner.has_value() && this->change_owner == OWN_PARK);
	this->SetWidgetCheckedAndPressed(TERR_OWNER_SALE, this->change_owner.has_value() && this->change_owner == OWN_FOR_SALE);
}

/**
 * Set the size of the terraform area.
 * @param xs If positive, the horizontal size of the terraform area.
 * @param ys If positive, the vertical size of the terraform area.
 */
void TerraformGui::SetTerraformSize(int xs, int ys)
{
	if (xs >= 0) this->xsize = xs;
	if (ys >= 0) this->ysize = ys;

	if (this->xsize == 0 && this->ysize == 0) {
		this->tiles_selector.SetSize(1, 1);
	} else {
		this->tiles_selector.SetSize(this->xsize, this->ysize);
	}
	SetSelector(&this->tiles_selector);
}

/** Increase the size of the terraform area. */
void TerraformGui::IncreaseSize()
{
	if (this->xsize >= TERRAFORM_MAX_SIZE && this->ysize >= TERRAFORM_MAX_SIZE) return;

	if (this->xsize == 0 && this->ysize == 0) {
		this->SetTerraformSize(1, 1);
	} else if (this->xsize > this->ysize) {
		this->SetTerraformSize(-1, this->ysize + 1);
	} else {
		this->SetTerraformSize(this->xsize + 1, -1);
	}
}

/** Decrease the size of the terraform area. */
void TerraformGui::DecreaseSize()
{
	if (this->xsize < 1 && this->ysize < 1) return;

	if (this->xsize == 1 && this->ysize == 1) {
		this->SetTerraformSize(0, 0);
	} else if (this->xsize > this->ysize) {
		this->SetTerraformSize(this->xsize - 1, -1);
	} else {
		this->SetTerraformSize(-1, this->ysize - 1);
	}
}

/** Open the terraform window (or if it is already opened, highlight and raise it). */
void ShowTerraformGui()
{
	if (HighlightWindowByType(WC_TERRAFORM, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new TerraformGui;
}
