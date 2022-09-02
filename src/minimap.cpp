/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file minimap.cpp Park overview %Minimap Gui. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "ride_type.h"
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
	Point32 GetRenderingBase(const Rectangle32 &widget_pos) const;

	int zoom;   ///< Size of a voxel in pixels on the minimap.
};

static const int MIN_ZOOM =  1;  ///< Minimum size of a voxel in pixels on the minimap.
static const int MAX_ZOOM = 16;  ///< Maximum size of a voxel in pixels on the minimap.

/**
 * Widget numbers of the minimap window.
 * @ingroup gui_group
 */
enum MinimapWidgets {
	MM_MAIN,         ///< Main minimap view.
	MM_ZOOM_IN,      ///< Zoom in button.
	MM_ZOOM_OUT,     ///< Zoom out button.
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
			Intermediate(1, 3),
				Widget(WT_TEXT_PUSHBUTTON, MM_ZOOM_OUT,          COL_RANGE_GREY), SetData(GUI_DECREASE_BUTTON, STR_NULL),
				Widget(WT_IMAGE_BUTTON,    INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(SPR_GUI_COMPASS_START + TC_NORTH, STR_NULL),
				Widget(WT_TEXT_PUSHBUTTON, MM_ZOOM_IN,           COL_RANGE_GREY), SetData(GUI_INCREASE_BUTTON, STR_NULL),
	EndContainer(),
};

Minimap::Minimap() : GuiWindow(WC_MINIMAP, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_minimap_build_gui_parts, lengthof(_minimap_build_gui_parts));
	this->SetScrolledWidget(MM_MAIN, MM_SCROLL_HORZ);
	this->SetScrolledWidget(MM_MAIN, MM_SCROLL_VERT);

	this->zoom = 4;
	this->UpdateButtons();
}

/** Update whether the zoom buttons are enabled, and the size of the scrollbars. */
void Minimap::UpdateButtons()
{
	this->GetWidget<LeafWidget>(MM_ZOOM_OUT)->SetShaded(this->zoom <= MIN_ZOOM);
	this->GetWidget<LeafWidget>(MM_ZOOM_IN )->SetShaded(this->zoom >= MAX_ZOOM);

	for (ScrollbarWidget *s : {this->GetWidget<ScrollbarWidget>(MM_SCROLL_HORZ), this->GetWidget<ScrollbarWidget>(MM_SCROLL_VERT)}) {
		s->SetItemSize(this->zoom);
		s->SetItemCount(_world.Width() + _world.Height());
	}
}

/**
 * Compute the pixel position of the (0|0) voxel on the minimap.
 * @param widget_pos Position and size of the canvas widget.
 * @return The pixel position, relative to the canvas's top-left corner.
 */
Point32 Minimap::GetRenderingBase(const Rectangle32 &widget_pos) const
{
	const unsigned required_size = this->zoom * (_world.Width() + _world.Height());
	Point32 base;
	if (widget_pos.height < required_size) {
		base.y = this->zoom * (1 - this->GetWidget<ScrollbarWidget>(MM_SCROLL_VERT)->GetStart());
	} else {
		base.y = (widget_pos.height - required_size) / 2;
	}
	if (widget_pos.width < required_size) {
		base.x = -this->GetWidget<ScrollbarWidget>(MM_SCROLL_HORZ)->GetStart() * this->zoom;
	} else {
		base.x = (widget_pos.width - required_size) / 2;
	}
	base.x += this->zoom * _world.Width();
	return base;
}

void Minimap::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != MM_MAIN) return GuiWindow::DrawWidget(wid_num, wid);

	int baseX = this->GetWidgetScreenX(wid);
	int baseY = this->GetWidgetScreenY(wid);

	Rectangle32 clip(baseX, baseY, wid->pos.width, wid->pos.height);
	_video.FillRectangle(clip, _palette[TEXT_BLACK]);
	_video.PushClip(clip);

	/* First pass: Find highest and lowest Z positions in the world, to adjust the colour ranges. */
	int minZ = WORLD_Z_SIZE;
	int maxZ = 0;
	for (int x = 0; x < _world.Width(); x++) {
		for (int y = 0; y < _world.Height(); y++) {
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
	for (int x = 0; x < _world.Width(); x++) {
		for (int y = 0; y < _world.Height(); y++) {
			const VoxelStack *vs = _world.GetStack(x, y);
			const int h = vs->GetTopGroundOffset();

			ColourRange col_range = _ground_type_colour[vs->voxels[h]->GetGroundType()];
			for (int i = vs->voxels.size() - 1; i >= h; i--) {
				const Voxel *v = vs->voxels[i].get();
				if (v->instance == SRI_PATH && HasValidPath(v)) {
					col_range = COL_RANGE_GREY;
					break;
				} else if (v->instance >= SRI_FULL_RIDES) {
					switch (_rides_manager.GetRideInstance(v->instance)->GetKind()) {
						case RTK_SHOP:    col_range = COL_RANGE_SEA_GREEN;  break;
						case RTK_GENTLE:  col_range = COL_RANGE_PINK_BROWN; break;
						case RTK_THRILL:  col_range = COL_RANGE_ORANGE;     break;
						case RTK_WET:     col_range = COL_RANGE_BLUE;       break;
						case RTK_COASTER: col_range = COL_RANGE_PURPLE;     break;
						default: NOT_REACHED();
					}
					break;
				}
			}

			const Rectangle32 r(baseX + this->zoom * (y - x - 1), baseY + this->zoom * (y + x - 0.5f), 2 * this->zoom, this->zoom);
			_video.FillRectangle(r, _palette[static_cast<int>(COL_SERIES_START + col_range * COL_SERIES_LENGTH + colour_base + colour_step * (h + vs->base - minZ))]);
			if (vs->owner != OWN_PARK) _video.FillRectangle(r, _palette[OVERLAY_DARKEN]);
		}
	}

	/* Finally, add the viewport overlay. */
	{
		const Viewport *vp = _window_manager.GetViewport();
		const float x = vp->view_pos.x / 256.0f;
		const float y = vp->view_pos.y / 256.0f;
		float w = 2.f * this->zoom * vp->rect.width / vp->tile_width;
		float h = static_cast<float>(this->zoom) * vp->rect.height / vp->tile_height;
		if (vp->orientation % 2 == 1) std::swap(w, h);
		_video.DrawRectangle(Rectangle32(baseX + this->zoom * (y - x) - w / 2, baseY + this->zoom * (y + x) - h / 2, w, h), _palette[TEXT_WHITE]);
	}

	_video.PopClip();
}

void Minimap::OnClick(WidgetNumber number, const Point16 &clicked_pos)
{
	switch (number) {
		case MM_ZOOM_IN:
			if (this->zoom < MAX_ZOOM) {
				this->zoom *= 2;
				this->UpdateButtons();
			}
			break;
		case MM_ZOOM_OUT:
			if (this->zoom > MIN_ZOOM) {
				this->zoom /= 2;
				this->UpdateButtons();
			}
			break;

		case MM_MAIN: {
			const Point32 base = this->GetRenderingBase(this->GetWidget<BaseWidget>(MM_MAIN)->pos);
			const float voxelX = (clicked_pos.y - base.y + base.x - clicked_pos.x) / (2.0f * this->zoom) + 0.25f;
			const float voxelY = voxelX + 1.0f + (clicked_pos.x - base.x) / this->zoom;
			if (voxelX >= 0 && voxelY >= 0 && voxelX < _world.Width() && voxelY < _world.Height()) {
				Viewport *vp = _window_manager.GetViewport();
				vp->view_pos.x = voxelX * 256;
				vp->view_pos.y = voxelY * 256;
			}
			break;
		}

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
