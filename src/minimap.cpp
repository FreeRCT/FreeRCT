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
		s->SetItemSize(this->zoom);
		s->SetItemCount(_world.GetXSize() + _world.GetYSize());
	}
}

void Minimap::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != MM_MAIN) return GuiWindow::DrawWidget(wid_num, wid);

	int baseX = this->GetWidgetScreenX(wid);
	int baseY = this->GetWidgetScreenY(wid);

	const ClippedRectangle old_clip = _video.GetClippedRectangle();
	const Rectangle32 new_clip(baseX, baseY, wid->pos.width, wid->pos.height);
	Rectangle32 new_clip_adjusted(new_clip);
	int adjustX = 0;
	int adjustY = 0;
	if (new_clip_adjusted.base.x < old_clip.absx) {
		adjustX = new_clip_adjusted.base.x - old_clip.absx;
		new_clip_adjusted.width += adjustX;
		new_clip_adjusted.base.x = old_clip.absx;
	}
	if (new_clip_adjusted.base.y < old_clip.absy) {
		adjustY = new_clip_adjusted.base.y - old_clip.absy;
		new_clip_adjusted.height += adjustY;
		new_clip_adjusted.base.y = old_clip.absy;
	}
	if (new_clip_adjusted.base.x + new_clip_adjusted.width >= old_clip.absx + old_clip.width) {
		new_clip_adjusted.width = old_clip.absx + old_clip.width - new_clip_adjusted.base.x;
	}
	if (new_clip_adjusted.base.y + new_clip_adjusted.height >= old_clip.absy + old_clip.height) {
		new_clip_adjusted.height = old_clip.absy + old_clip.height - new_clip_adjusted.base.y;
	}
	_video.SetClippedRectangle(ClippedRectangle(new_clip_adjusted.base.x, new_clip_adjusted.base.y, new_clip_adjusted.width, new_clip_adjusted.height));

	const int required_size = this->zoom * (_world.GetXSize() + _world.GetYSize());
	if (new_clip.height < required_size) {
		baseY = this->zoom * (1 - this->GetWidget<ScrollbarWidget>(MM_SCROLL_VERT)->GetStart());
	} else {
		baseY = (new_clip.height - required_size) / 2;
	}
	if (new_clip.width < required_size) {
		baseX = -this->GetWidget<ScrollbarWidget>(MM_SCROLL_HORZ)->GetStart() * this->zoom;
	} else {
		baseX = (new_clip.width - required_size) / 2;
	}
	baseX += this->zoom * _world.GetXSize() + adjustX;
	baseY += adjustY;

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

			_video.FillRectangle(Rectangle32(baseX + this->zoom * (y - x - 1), baseY + this->zoom * (y + x - 0.5f), 2 * this->zoom, this->zoom),
					_palette[static_cast<int>(COL_SERIES_START + _ground_type_colour[g] * COL_SERIES_LENGTH + colour_base + colour_step * (h - minZ))]);
		}
	}

	/* Finally, add the viewport overlay. */
	{
		const Viewport *vp = _window_manager.GetViewport();
		const float x = vp->view_pos.x / 256.0f;
		const float y = vp->view_pos.y / 256.0f;
		const float w = 2.f * this->zoom * vp->rect.width / vp->tile_width;
		const float h = this->zoom * vp->rect.height / vp->tile_height;
		_video.DrawRectangle(Rectangle32(baseX + this->zoom * (y - x) - w / 2, baseY + this->zoom * (y + x) - h / 2, w, h), _palette[TEXT_WHITE]);
	}

	_video.SetClippedRectangle(old_clip);
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
