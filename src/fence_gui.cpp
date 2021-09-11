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

/** How much it costs to build a fence segment. */
static const Money FENCE_COST_BUILD[] = {
	Money( 300),  ///< FENCE_TYPE_WOODEN
	Money( 600),  ///< FENCE_TYPE_CONIFER_HEDGE
	Money(1100),  ///< FENCE_TYPE_BRICK_WALL
};

/** How much it costs to remove a fence segment. */
static const Money FENCE_COST_REMOVE[] = {
	Money(-100),  ///< FENCE_TYPE_WOODEN
	Money(-200),  ///< FENCE_TYPE_CONIFER_HEDGE
	Money(-400),  ///< FENCE_TYPE_BRICK_WALL
};

assert_compile(lengthof(FENCE_COST_BUILD) == FENCE_TYPE_COUNT - FENCE_TYPE_BUILDABLE_BEGIN);
assert_compile(lengthof(FENCE_COST_BUILD) == lengthof(FENCE_COST_REMOVE));

/**
 * Check how much it costs to build a fence segment.
 * @param t Fence type to check (must be buildable).
 * @return The cost.
 */
const Money &GetFenceCostBuild(const FenceType t)
{
	assert(t >= FENCE_TYPE_BUILDABLE_BEGIN && t < FENCE_TYPE_COUNT);
	return FENCE_COST_BUILD[t - FENCE_TYPE_BUILDABLE_BEGIN];
}

/**
 * Check how much it costs to remove a fence segment.
 * @param t Fence type to check (must be buildable).
 * @return The cost.
 */
const Money &GetFenceCostRemove(const FenceType t)
{
	assert(t >= FENCE_TYPE_BUILDABLE_BEGIN && t < FENCE_TYPE_COUNT);
	return FENCE_COST_REMOVE[t - FENCE_TYPE_BUILDABLE_BEGIN];
}

/**
 * %Fence build GUI.
 * @ingroup gui_group
 */
class FenceGui : public GuiWindow {
public:
	FenceGui();
	~FenceGui();

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

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
 * Widget numbers of the Fence build GUI.
 * @ingroup gui_group
 */
enum FenceWidgets {
	FENCE_TEXT_WOOD,                         ///< Wooden fence price text field.
	FENCE_TEXT_HEDGE,                        ///< Hedge  fence price text field.
	FENCE_TEXT_BRICK,                        ///< Brick  fence price text field.
	/* Keep the ordering of the buttons intact! */
	FENCE_BUTTON_FIRST,
	FENCE_BUTTON_WOOD = FENCE_BUTTON_FIRST,  ///< Place wooden fence button.
	FENCE_BUTTON_HEDGE,                      ///< Place hedge  fence button.
	FENCE_BUTTON_BRICK,                      ///< Place brick  fence button.
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
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(3, 2),
				Widget(WT_TEXT_BUTTON, FENCE_BUTTON_WOOD,  COL_RANGE_DARK_GREEN), SetData(STR_NULL, GUI_FENCE_TYPE_WOOD ), SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,   FENCE_TEXT_WOOD,    COL_RANGE_DARK_GREEN), SetData(STR_ARG1, STR_NULL),
				Widget(WT_TEXT_BUTTON, FENCE_BUTTON_HEDGE, COL_RANGE_DARK_GREEN), SetData(STR_NULL, GUI_FENCE_TYPE_HEDGE), SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,   FENCE_TEXT_HEDGE,   COL_RANGE_DARK_GREEN), SetData(STR_ARG1, STR_NULL),
				Widget(WT_TEXT_BUTTON, FENCE_BUTTON_BRICK, COL_RANGE_DARK_GREEN), SetData(STR_NULL, GUI_FENCE_TYPE_BRICK), SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,   FENCE_TEXT_BRICK,   COL_RANGE_DARK_GREEN), SetData(STR_ARG1, STR_NULL),
		EndContainer(),
	EndContainer(),
};

FenceGui::FenceGui() : GuiWindow(WC_FENCE, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_fence_build_gui_parts, lengthof(_fence_build_gui_parts));

	this->fence_type = FENCE_TYPE_INVALID;
	this->fence_edge = INVALID_EDGE;
	this->fence_sel.SetSize(0, 0);
}

FenceGui::~FenceGui()
{
	this->SetSelector(nullptr);
}

void FenceGui::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case FENCE_TEXT_WOOD:
			_str_params.SetMoney(1, GetFenceCostBuild(FENCE_TYPE_WOODEN));
			break;
		case FENCE_TEXT_HEDGE:
			_str_params.SetMoney(1, GetFenceCostBuild(FENCE_TYPE_CONIFER_HEDGE));
			break;
		case FENCE_TEXT_BRICK:
			_str_params.SetMoney(1, GetFenceCostBuild(FENCE_TYPE_BRICK_WALL));
			break;

		default:
			break;
	}
}

void FenceGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case FENCE_BUTTON_WOOD:
		case FENCE_BUTTON_HEDGE:
		case FENCE_BUTTON_BRICK: {
			const Fence *fence = _sprite_manager.GetFence(FenceType(FENCE_TYPE_BUILDABLE_BEGIN + wid_num - FENCE_BUTTON_FIRST));
			if (fence == nullptr) break;

			const ImageData *sprite = fence->sprites[FENCE_NE_FLAT];
			if (sprite == nullptr) break;

			static Recolouring recolouring;
			_video.BlitImage({this->GetWidgetScreenX(wid) - sprite->xoffset + (wid->pos.width - sprite->width) / 2,
					this->GetWidgetScreenY(wid) - sprite->yoffset + (wid->pos.height - sprite->height) / 2}, sprite, recolouring, GS_NORMAL);
			return;
		}
	}

	return GuiWindow::DrawWidget(wid_num, wid);
}

void FenceGui::OnClick(WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case FENCE_BUTTON_WOOD:
			OnClickFence(_sprite_manager.GetFence(FENCE_TYPE_WOODEN));
			return;
		case FENCE_BUTTON_HEDGE:
			OnClickFence(_sprite_manager.GetFence(FENCE_TYPE_CONIFER_HEDGE));
			return;
		case FENCE_BUTTON_BRICK:
			OnClickFence(_sprite_manager.GetFence(FENCE_TYPE_BRICK_WALL));
			return;

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

	this->GetWidget<LeafWidget>(FENCE_BUTTON_WOOD )->SetPressed(clicked_type == FENCE_TYPE_WOODEN       );
	this->GetWidget<LeafWidget>(FENCE_BUTTON_HEDGE)->SetPressed(clicked_type == FENCE_TYPE_CONIFER_HEDGE);
	this->GetWidget<LeafWidget>(FENCE_BUTTON_BRICK)->SetPressed(clicked_type == FENCE_TYPE_BRICK_WALL   );

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

	/* Does this edge contain two connected paths or a connected path and ride entrance/exit? */
	uint16 instance_data = v->GetInstanceData();
	if (HasValidPath(v)) {
		uint8 slope = GetImplodedPathSlope(instance_data);
		slope = _path_expand[slope];

		if (GB(slope, PATHBIT_NE + edge, 1) != 0) edge = INVALID_EDGE; // Prevent placing on top of paths.
	}

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
	if (HighlightWindowByType(WC_FENCE, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new FenceGui;
}
