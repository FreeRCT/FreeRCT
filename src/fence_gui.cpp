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
#include "gamecontrol.h"
#include "gui_sprites.h"
#include "sprite_data.h"

assert_compile(FENCE_TYPE_COUNT <= FENCE_TYPE_INVALID); ///< #FENCE_TYPE_INVALID should be the biggest value as it is tested below.
assert_compile(FENCE_TYPE_INVALID <= 0xF); ///< Fence type for one side must fit in 4 bit.

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
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);

	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

	FenceType fence_type;      ///< Currently selected fence type (#FENCE_TYPE_INVALID means no type selected).
	XYZPoint16 fence_base;     ///< Voxel position (base of the ground) where fence has been placed (only valid if #fence_edge is valid).
	TileEdge fence_edge;       ///< Edge where new fence has been placed, #INVALID_EDGE for no placed fence.
	FencesMouseMode fence_sel; ///< Mouse selector for building fences.
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

	this->fence_type = FENCE_TYPE_INVALID;
	this->fence_edge = INVALID_EDGE;
	this->fence_sel.SetSize(0, 0);
}

FenceGui::~FenceGui()
{
	this->SetSelector(nullptr);
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

/**
 * Handle a click on a fence in the GUI.
 * @param fence The clicked fence.
 */
void FenceGui::OnClickFence(const Fence *fence)
{
	FenceType clicked_type = (FenceType)fence->type;
	assert(clicked_type != FENCE_TYPE_INVALID);

	if (this->fence_type == FENCE_TYPE_INVALID) {
		this->fence_sel.SetSize(1, 1);
		this->SetSelector(&this->fence_sel);
	}
	this->fence_type = clicked_type;
}

void FenceGui::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	if (this->fence_type == FENCE_TYPE_INVALID || this->selector == nullptr) return;

	FinderData fdata(CS_GROUND_EDGE, FW_EDGE);
	if (vp->ComputeCursorPosition(&fdata) != CS_GROUND_EDGE) return;
	if (fdata.cursor < CUR_TYPE_EDGE_NE || fdata.cursor > CUR_TYPE_EDGE_NW) return;

	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(fdata.voxel_pos.x, fdata.voxel_pos.y) != OWN_PARK) return;

	/* Normalize voxel position to the base ground voxel. */
	const Voxel *v = _world.GetVoxel(fdata.voxel_pos);
	assert(v->GetGroundType() != GTP_INVALID);
	uint8 slope = v->GetGroundSlope();
	if (IsImplodedSteepSlopeTop(slope)) {
		fdata.voxel_pos.z--; // Select base of the ground for the edge cursor.
		/* Post-pone 'slope' update until decided that a new fence position needs to be calculated. */
	}

	TileEdge edge = static_cast<TileEdge>(EDGE_NE + (fdata.cursor - CUR_TYPE_EDGE_NE));
	if (edge == this->fence_edge && fdata.voxel_pos == this->fence_base) return;

	/* New fence, or moved fence. Update the mouse selector. */
	this->fence_sel.MarkDirty();

	this->fence_edge = edge; // Store new edge and base position.
	this->fence_base = fdata.voxel_pos;

	/* Compute actual voxel that contains the fence. */
	if (IsImplodedSteepSlopeTop(slope)) {
		slope = _world.GetVoxel(fdata.voxel_pos)->GetGroundSlope();
	}
	fdata.voxel_pos.z += GetVoxelZOffsetForFence(edge, slope);

	this->fence_sel.SetPosition(fdata.voxel_pos.x, fdata.voxel_pos.y);
	this->fence_sel.AddVoxel(fdata.voxel_pos);
	this->fence_sel.SetupRideInfoSpace();
	this->fence_sel.SetFenceData(fdata.voxel_pos, this->fence_type, edge);

	this->fence_sel.MarkDirty();
}

void FenceGui::SelectorMouseButtonEvent(uint8 state)
{
	if (!IsLeftClick(state)) return;
	if (this->fence_sel.area.width != 1 || this->fence_sel.area.height != 1) return;
	if (this->fence_edge == INVALID_EDGE) return;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(this->fence_base.x, this->fence_base.y) != OWN_PARK) return;

	VoxelStack *vs = _world.GetModifyStack(this->fence_base.x, this->fence_base.y);
	uint16 fences = GetGroundFencesFromMap(vs, this->fence_base.z);
	fences = SetFenceType(fences, this->fence_edge, this->fence_type);
	AddGroundFencesToMap(fences, vs, this->fence_base.z);
	MarkVoxelDirty(this->fence_base);
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
