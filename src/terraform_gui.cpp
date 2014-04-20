/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform_gui.cpp Terraforming gui code. */

#include "stdafx.h"
#include "window.h"
#include "viewport.h"
#include "terraform.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "gui_sprites.h"

static const int TERRAFORM_MAX_SIZE = 9;      ///< Maximum length of tiles for terraforming (both X and Y).
static const int TERRAFORM_ELEMENT_SIZE = 16; ///< Horizontal size of a tile in the display (pixels).

/** GUI for setting properties for terraforming. */
class TerraformGui : public GuiWindow {
public:
	TerraformGui();
	~TerraformGui();

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;

	bool level; ///< If true, level the area, else move it up/down as-is.
	int xsize;  ///< Size of the terraform area in horizontal direction.
	int ysize;  ///< Size of the terraform area in vertical direction.

private:
	void Setlevelling(bool level);
	void SetTerraformSize(int xs = -1, int ys = -1);
	void IncreaseSize();
	void DecreaseSize();
};

/** Widget numbers of the terraform GUI. */
enum TerraformWidgets {
	TERR_DISPLAY, ///< Widget displaying the current terraform size.
	TERR_ADD,     ///< 'Increase' button.
	TERR_SUB,     ///< 'Decrease' button.
	TERR_LEVEL,   ///< Level the terraform area.
	TERR_MOVE,    ///< Move the terraform area.
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
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_LEVEL_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, TERR_MOVE, COL_RANGE_DARK_GREEN), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_TERRAFORM_MOVE_TEXT, STR_NULL),
	EndContainer(),
};

TerraformGui::TerraformGui() : GuiWindow(WC_TERRAFORM, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_terraform_gui_parts, lengthof(_terraform_gui_parts));
	this->Setlevelling(true);
	this->SetTerraformSize(1, 1);

	_terraformer.OpenWindow();
}

TerraformGui::~TerraformGui()
{
	_terraformer.CloseWindow();
}

void TerraformGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	Point32 base;
	static const Recolouring recolour; // Not changed.

	if (wid_num != TERR_DISPLAY || _video == nullptr) return;

	/* Draw the tile area, case of a null-area. */
	if (this->xsize == 0 && this->ysize == 0) {
		const ImageData *dot = _gui_sprites.dot_sprite;
		if (dot == nullptr) return;

		base.x = this->GetWidgetScreenX(wid) + (wid->pos.width - dot->width) / 2;
		base.y = this->GetWidgetScreenY(wid) + (wid->pos.height - dot->height) / 2;
		_video->BlitImage(base, dot, recolour, 0);
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
			Point16 left;
			left.x = base.x + (x + y) * TERRAFORM_ELEMENT_SIZE / 2;
			left.y = base.y + (y - x) * TERRAFORM_ELEMENT_SIZE / 4;
			Point16 top, bottom;
			top.x = left.x + TERRAFORM_ELEMENT_SIZE / 2;
			top.y = left.y - TERRAFORM_ELEMENT_SIZE / 4;
			_video->DrawLine(left, top, MakeRGBA(255, 255, 255, OPAQUE));
			bottom.x = top.x;
			bottom.y = left.y + TERRAFORM_ELEMENT_SIZE / 4;
			_video->DrawLine(left, bottom, MakeRGBA(255, 255, 255, OPAQUE));
			left.x += TERRAFORM_ELEMENT_SIZE; // Move 'left' to the right.
			_video->DrawLine(top, left, MakeRGBA(255, 255, 255, OPAQUE));
			_video->DrawLine(bottom, left, MakeRGBA(255, 255, 255, OPAQUE));
		}
	}
}

void TerraformGui::OnClick(WidgetNumber wid, const Point16 &pos)
{
	switch (wid) {
		case TERR_ADD:
			this->IncreaseSize();
			break;

		case TERR_SUB:
			this->DecreaseSize();
			break;

		case TERR_LEVEL:
			if (!this->level) this->Setlevelling(true);
			break;

		case TERR_MOVE:
			if (this->level) this->Setlevelling(false);
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
void TerraformGui::Setlevelling(bool level)
{
	this->level = level;
	this->SetWidgetChecked(TERR_LEVEL, this->level);
	this->SetWidgetChecked(TERR_MOVE, !this->level);
	this->SetWidgetPressed(TERR_LEVEL, this->level);
	this->SetWidgetPressed(TERR_MOVE, !this->level);
	this->MarkWidgetDirty(TERR_LEVEL);
	this->MarkWidgetDirty(TERR_MOVE);

	_terraformer.Setlevelling(level);
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

	this->MarkWidgetDirty(TERR_DISPLAY);
	_terraformer.SetSize(this->xsize, this->ysize);
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
	if (HighlightWindowByType(WC_TERRAFORM, ALL_WINDOWS_OF_TYPE)) return;
	new TerraformGui;
}
