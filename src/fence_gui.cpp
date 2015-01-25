/* $Id: fence_gui.cpp */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fence_gui.cpp %Fence building and editing. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "fence_build.h"
#include "gui_sprites.h"
#include "sprite_data.h"

/**
 * %Fence build GUI.
 * @ingroup gui_group
 */
class FenceGui : public GuiWindow {
public:
	FenceGui();
	~FenceGui();

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);

private:
	void OnClickFence(const Fence *fence);
};

/**
 * Widget numbers of the fence build GUI.
 * @ingroup gui_group
 */
enum FenceWidgets {
	FENCE_GUI_LIST,         ///< List of fence types.
	FENCE_GUI_SCROLL_LIST,  ///< Scrollbar of the list.
};

/**
 * Widget parts of the fence build GUI.
 * @ingroup gui_group
 */
static const WidgetPart _fence_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_FENCE_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),
		/* List of fences. */
		Intermediate(1, 2),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
				Widget(WT_EMPTY, FENCE_GUI_LIST, COL_RANGE_DARK_GREEN), SetFill(0, 1), SetResize(0, 1), SetMinimalSize(100, 100),
			Widget(WT_VERT_SCROLLBAR, FENCE_GUI_SCROLL_LIST, COL_RANGE_DARK_GREEN),
		EndContainer(),
	EndContainer(),
};

FenceGui::FenceGui() : GuiWindow(WC_FENCE, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_fence_build_gui_parts, lengthof(_fence_build_gui_parts));
	this->SetScrolledWidget(FENCE_GUI_LIST, FENCE_GUI_SCROLL_LIST);
	_fence_builder.OpenWindow();
}

FenceGui::~FenceGui()
{
	_fence_builder.CloseWindow();
}

void FenceGui::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case FENCE_GUI_LIST:
			wid->resize_y = GetTextHeight();
			wid->min_y = 5 * wid->resize_y;

			for (uint8 i = (uint8)FENCE_TYPE_BUILDABLE_BEGIN; i < (uint8)FENCE_TYPE_COUNT; i++) {
				const Fence *fence = _sprite_manager.GetFence((FenceType)i);
				if (fence == nullptr) continue;

				int width = fence->sprites[FENCE_NE_FLAT]->width;
				if (width > wid->min_x) wid->min_x = width;
			}
			break;
	}
}

void FenceGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case FENCE_GUI_LIST: {
			Point32 rect;
			int lines = -1; // computed later when we know sprite size
			rect.x = this->GetWidgetScreenX(wid);
			rect.y = this->GetWidgetScreenY(wid);
			for (uint8 i = FENCE_TYPE_BUILDABLE_BEGIN; i < FENCE_TYPE_COUNT; i++) {
				const Fence *fence = _sprite_manager.GetFence((FenceType)i);
				if (fence == nullptr) continue;

				const ImageData *sprite = fence->sprites[FENCE_NE_FLAT];
				if (lines == -1) {
					lines = wid->pos.height / sprite->height; /// \todo Implement scrollbar position & size.
				}

				if (lines <= 0) break;
				lines--;

				Recolouring recolouring;
				_video.BlitImage({rect.x, rect.y - sprite->yoffset}, sprite, recolouring, GS_NORMAL);
				rect.y += sprite->height;
			}
			break;
		}
	}
}

void FenceGui::OnClick(WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case FENCE_GUI_LIST: {
			BaseWidget *wid = GetWidget<BaseWidget>(number);
			Point16 mouse = _window_manager.GetMousePosition();
			Point32 rect;
			int lines = -1; // computed later when we know sprite size
			rect.x = this->GetWidgetScreenX(wid);
			rect.y = this->GetWidgetScreenY(wid);
			for (uint8 i = FENCE_TYPE_BUILDABLE_BEGIN; i < FENCE_TYPE_COUNT; i++) {
				const Fence *fence = _sprite_manager.GetFence((FenceType)i);
				if (fence == nullptr) continue;

				uint16 sprite_height = fence->sprites[FENCE_NE_FLAT]->height;
				if (lines == -1) {
					lines = wid->pos.height / sprite_height; /// \todo Implement scrollbar position & size.
				}

				if (lines <= 0) break;
				lines--;

				if (mouse.x >= rect.x && mouse.x <= rect.x + wid->pos.width &&
						mouse.y >= rect.y && mouse.y <= rect.y + sprite_height) {
					this->OnClickFence(fence);
					return;
				}

				rect.y += sprite_height;
			}
			break;
		}

		default:
			break;
	}
}

void FenceGui::OnChange(ChangeCode code, uint32 parameter)
{
	if (code != CHG_MOUSE_MODE_LOST) return;

	FenceType selected = (FenceType)(parameter - (uint32)FENCE_TYPE_BUILDABLE_BEGIN);
	_fence_builder.SelectFenceType(selected);
}

/**
 * Handle a click on a fence.
 * @param fence The clicked fence.
 */
void FenceGui::OnClickFence(const Fence *fence)
{
	_fence_builder.SelectFenceType(fence != nullptr ? (FenceType)fence->type : FENCE_TYPE_INVALID);
}

/**
 * Open the fence GUI.
 * @ingroup gui_group
 */
void ShowFenceGui()
{
	if (HighlightWindowByType(WC_FENCE, ALL_WINDOWS_OF_TYPE)) return;
	new FenceGui;
}
