/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file minimap.cpp %Park overview minimap Gui. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "finances.h"
#include "gamecontrol.h"
#include "gui_sprites.h"
#include "sprite_data.h"

/**
 * %Minimap window.
 * @ingroup gui_group
 */
class Minimap : public GuiWindow {
public:
	Minimap();

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;

private:
	void UpdateButtons();

	int zoom;   ///< Size of a voxel in pixels on the minimap.
};

static const int MIN_ZOOM =  1;  ///< Minimum size of a voxel in pixels on the minimap.
static const int MAX_ZOOM = 16;  ///< Maximum size of a voxel in pixels on the minimap.

/**
 * Widget numbers of the minimap window.
 * @ingroup gui_group
 */
enum MinimapWidgets {
	MM_MAIN,      ///< Main view
	MM_ZOOM_IN,   ///< Zoom in button.
	MM_ZOOM_OUT,  ///< Zoom out button.
	MM_SCROLL_HORZ,  ///< Horizontal scrollbar.
	MM_SCROLL_VERT,  ///< Vertical scrollbar.
};

/**
 * Widget parts of the minimap window.
 * @ingroup gui_group
 */
static const WidgetPart _minimap_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_MINIMAP_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(1, 2),
				Intermediate(2, 1),
					Widget(WT_EMPTY, MM_MAIN, COL_RANGE_GREY), SetFill(64, 64), SetResize(64, 64), SetMinimalSize(384, 384),
					Widget(WT_HOR_SCROLLBAR, MM_SCROLL_HORZ, COL_RANGE_GREY),
				Widget(WT_VERT_SCROLLBAR, MM_SCROLL_VERT, COL_RANGE_GREY),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(1, 2),
				Widget(WT_TEXT_BUTTON, MM_ZOOM_OUT, COL_RANGE_GREY), SetData(GUI_DECREASE_BUTTON, STR_NULL),
				Widget(WT_TEXT_BUTTON, MM_ZOOM_IN,  COL_RANGE_GREY), SetData(GUI_INCREASE_BUTTON, STR_NULL),
	EndContainer(),
};

Minimap::Minimap() : GuiWindow(WC_MINIMAP, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_minimap_build_gui_parts, lengthof(_minimap_build_gui_parts));
	this->SetScrolledWidget(MM_MAIN, MM_SCROLL_HORZ);
	this->SetScrolledWidget(MM_MAIN, MM_SCROLL_VERT);

	this->zoom = 8;
	this->UpdateButtons();
}

/** Update whether the zoom buttons are enabled, and the size of the scrollbars. */
void Minimap::UpdateButtons()
{
	this->GetWidget<LeafWidget>(MM_ZOOM_OUT)->SetShaded(this->zoom <= MIN_ZOOM);
	this->GetWidget<LeafWidget>(MM_ZOOM_IN )->SetShaded(this->zoom >= MAX_ZOOM);

	for (ScrollbarWidget *s : {this->GetWidget<ScrollbarWidget>(MM_SCROLL_HORZ), this->GetWidget<ScrollbarWidget>(MM_SCROLL_VERT)}) {
		s->SetItemSize(1);
		s->SetItemCount(this->zoom * (_world.GetXSize() + _world.GetYSize()));
	}
}

void Minimap::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != MM_MAIN) return GuiWindow::DrawWidget(wid_num, wid);

	int baseX = this->GetWidgetScreenX(wid);
	int baseY = this->GetWidgetScreenY(wid);

	const ClippedRectangle clip = _video.GetClippedRectangle();
	_video.SetClippedRectangle(ClippedRectangle(baseX, baseY, wid->pos.width, wid->pos.height));

	baseX = this->zoom * _world.GetXSize() - this->GetWidget<ScrollbarWidget>(MM_SCROLL_HORZ)->GetStart();
	baseY =                                - this->GetWidget<ScrollbarWidget>(MM_SCROLL_VERT)->GetStart();

	/* First pass: Find highest and lowest Z positions in the world, to adjust the colour ranges. */
	int minZ = WORLD_Z_SIZE;
	int maxZ = 0;
	for (int x = 0; x < _world.GetXSize(); x++) {
		for (int y = 0; y < _world.GetYSize(); y++) {
			const int h = _world.GetTopGroundHeight(x, y);
			minZ = std::min(minZ, h);
			maxZ = std::max(maxZ, h);
		}
	}
	int colour_base;
	float colour_step;
	if (maxZ - minZ < COL_SERIES_LENGTH) {
		colour_step = 1;
		colour_base = (COL_SERIES_LENGTH - maxZ + minZ) / 2;
	} else {
		colour_step = COL_SERIES_LENGTH / (maxZ - minZ + 1.0f);
		colour_base = 0;
	}

	/* Second pass: Draw the map. */
	for (int x = 0; x < _world.GetXSize(); x++) {
		for (int y = 0; y < _world.GetYSize(); y++) {
			const VoxelStack *vs = _world.GetStack(x, y);
			int h = vs->GetTopGroundOffset();
			const GroundType g = vs->voxels[h]->GetGroundType();
			h += vs->base;

			_video.FillRectangle(Rectangle32(baseX + this->zoom * (y - x), baseY + this->zoom * (y + x), 2 * this->zoom, this->zoom),
					_palette[static_cast<int>(COL_SERIES_START + _ground_type_colour[g] * COL_SERIES_LENGTH + colour_base + colour_step * (h - minZ))]);
		}
	}

	_video.SetClippedRectangle(clip);
}

void Minimap::OnClick(WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case MM_ZOOM_IN:
			if (this->zoom < MAX_ZOOM) {
				this->zoom *= 2;
				this->UpdateButtons();
				this->MarkDirty();
			}
			break;
		case MM_ZOOM_OUT:
			if (this->zoom > MIN_ZOOM) {
				this->zoom /= 2;
				this->UpdateButtons();
				this->MarkDirty();
			}
			break;

		case MM_MAIN:
			// NOCOM
			break;

		default: break;
	}
}

/**
 * Open the minimap window.
 * @ingroup gui_group
 */
void ShowMinimap()
{
	if (HighlightWindowByType(WC_MINIMAP, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new Minimap;
}
