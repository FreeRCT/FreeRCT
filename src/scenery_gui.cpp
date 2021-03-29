/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scenery_gui.cpp %Scenery building and editing. */

#include "scenery.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "finances.h"
#include "gamecontrol.h"
#include "gui_sprites.h"
#include "sprite_data.h"

/**
 * %Scenery build GUI.
 * @ingroup gui_group
 */
class SceneryGui : public GuiWindow {
public:
	SceneryGui();
	~SceneryGui();

	void SetCategory(SceneryCategory cat);

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void SetType(const SceneryType *t);

	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

private:
	RideMouseMode scenery_sel;  ///< Mouse selector for building scenery items. The logic is the same as for rides.

	SceneryCategory category;                   ///< Category of item types to display.
	std::vector<const SceneryType*> types;      ///< Scenery types in the current category.
	const SceneryType *selected_type;           ///< Currently selected item type.
	uint8 orientation;                          ///< Current orientation.
	std::unique_ptr<SceneryInstance> instance;  ///< Instance being placed.
	StringID build_forbidden_reason;            ///< Reason why we may not place the instance at the given location, if any.
};

/**
 * Widget numbers of the scenery build GUI.
 * @ingroup gui_group
 */
enum SceneryWidgets {
	SCENERY_GUI_LIST,             ///< List of Scenery types.
	SCENERY_GUI_SCROLL_LIST,      ///< Scrollbar of the list.
	SCENERY_ROTATE_POS,           ///< Counter clockwise rotate button.
	SCENERY_ROTATE_NEG,           ///< Clockwise rotate button.
	SCENERY_CATEGORY_TREES,       ///< Tab for the Trees category.
	SCENERY_CATEGORY_FLOWERBEDS,  ///< Tab for the Flowerbeds category.
	SCENERY_CATEGORY_FOUNTAINS,   ///< Tab for the Fountains category.
};

static const uint ITEM_COUNT   =   5;  ///< Number of items to display.
static const uint ITEM_WIDTH   = 128;  ///< Width of one item in the list.
static const uint ITEM_SPACING =   4;  ///< Horizontal spacing in the list.
static const uint ITEM_HEIGHT  =  64;  ///< Height of one item in the list.
static const uint TEXT_HEIGHT  =  20;  ///< Height of texts below and above the items;

/**
 * Widget parts of the scenery build GUI.
 * @ingroup gui_group
 */
static const WidgetPart _scenery_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_SCENERY_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(3, 1),
				Intermediate(1, 0),
					Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
					Widget(WT_TEXT_TAB, SCENERY_CATEGORY_TREES, COL_RANGE_DARK_GREEN), SetData(GUI_SCENERY_CATEGORY_TREES, STR_NULL),
					Widget(WT_TEXT_TAB, SCENERY_CATEGORY_FLOWERBEDS, COL_RANGE_DARK_GREEN), SetData(GUI_SCENERY_CATEGORY_FLOWERBEDS, STR_NULL),
					Widget(WT_TEXT_TAB, SCENERY_CATEGORY_FOUNTAINS, COL_RANGE_DARK_GREEN), SetData(GUI_SCENERY_CATEGORY_FOUNTAINS, STR_NULL),
					Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 1),
					Widget(WT_IMAGE_PUSHBUTTON, SCENERY_ROTATE_POS, COL_RANGE_DARK_GREEN), SetData(SPR_GUI_ROT3D_POS, GUI_SCENERY_ROTATE_POS),
					Widget(WT_IMAGE_PUSHBUTTON, SCENERY_ROTATE_NEG, COL_RANGE_DARK_GREEN), SetData(SPR_GUI_ROT3D_NEG, GUI_SCENERY_ROTATE_NEG),
				EndContainer(),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
				Widget(WT_EMPTY, SCENERY_GUI_LIST, COL_RANGE_DARK_GREEN),
						SetFill(ITEM_WIDTH, 0), SetResize(ITEM_WIDTH, 0), SetMinimalSize(ITEM_WIDTH * ITEM_COUNT, ITEM_HEIGHT + 2 * TEXT_HEIGHT),
				Widget(WT_HOR_SCROLLBAR, SCENERY_GUI_SCROLL_LIST, COL_RANGE_DARK_GREEN),

	EndContainer(),
};

SceneryGui::SceneryGui() : GuiWindow(WC_SCENERY, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_scenery_build_gui_parts, lengthof(_scenery_build_gui_parts));
	this->SetScrolledWidget(SCENERY_GUI_LIST, SCENERY_GUI_SCROLL_LIST);

	this->orientation = 0;
	this->SetType(nullptr);
	this->SetCategory(SCC_TREES);
}

SceneryGui::~SceneryGui()
{
	this->SetSelector(nullptr);
}

/**
 * Sets what kind of scenery types to offer.
 * @param cat Category to select.
 */
void SceneryGui::SetCategory(const SceneryCategory cat)
{
	this->SetType(nullptr);
	this->category = cat;
	this->types = _scenery.GetAllTypes(cat);
	this->GetWidget<ScrollbarWidget>(SCENERY_GUI_SCROLL_LIST)->SetItemCount(this->types.size());

	this->SetWidgetPressed(SCENERY_CATEGORY_TREES,      cat == SCC_TREES);
	this->SetWidgetPressed(SCENERY_CATEGORY_FLOWERBEDS, cat == SCC_FLOWERBEDS);
	this->SetWidgetPressed(SCENERY_CATEGORY_FOUNTAINS,  cat == SCC_FOUNTAINS);
}

void SceneryGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != SCENERY_GUI_LIST) return GuiWindow::DrawWidget(wid_num, wid);

	int x = this->GetWidgetScreenX(wid);
	int y = this->GetWidgetScreenY(wid);

	const uint first_index = this->GetWidget<ScrollbarWidget>(SCENERY_GUI_SCROLL_LIST)->GetStart();
	const uint last_index = std::min<uint>(this->types.size(), first_index + ITEM_COUNT);
	static Recolouring rc;  // Never modified.
	const uint8 rotation = (this->orientation - _window_manager.GetViewport()->orientation) & 3;
	for (uint i = first_index; i < last_index; i++) {
		Rectangle32 r(x + ITEM_SPACING, y + TEXT_HEIGHT, ITEM_WIDTH - 2 * ITEM_SPACING, ITEM_HEIGHT);
		const SceneryType *t = this->types[i];

		if (t == this->selected_type) _video.FillRectangle(r, _palette[COL_SERIES_START + (COL_RANGE_DARK_GREEN + 1) * COL_SERIES_LENGTH - 1]);
		_video.BlitImage(Point32(
				x + (ITEM_WIDTH - t->previews[rotation]->width) / 2,
				y + TEXT_HEIGHT + (ITEM_HEIGHT - t->previews[rotation]->height) / 2),
			t->previews[rotation], rc, GS_NORMAL);
		_video.DrawRectangle(r, _palette[COL_SERIES_START + COL_RANGE_DARK_GREEN * COL_SERIES_LENGTH]);

		_str_params.SetMoney(1, t->buy_cost);
		DrawString(STR_ARG1, TEXT_BLACK, x, y + ITEM_HEIGHT + TEXT_HEIGHT, ITEM_WIDTH, ALG_CENTER);
		DrawString(t->name, TEXT_BLACK, x, y, ITEM_WIDTH, ALG_CENTER);

		x += ITEM_WIDTH;
	}
}

/**
 * Set the type of scenery we're currently placing.
 * @param t Type to place (may be \c nullptr).
 */
void SceneryGui::SetType(const SceneryType *t)
{
	this->selected_type = t;
	this->SetWidgetShaded(SCENERY_ROTATE_NEG, t == nullptr || t->symmetric);
	this->SetWidgetShaded(SCENERY_ROTATE_POS, t == nullptr || t->symmetric);
	if (t == nullptr) {
		this->SetSelector(nullptr);
		this->instance.reset();
	} else {
		this->instance.reset(new SceneryInstance(t));
		this->instance->orientation = this->orientation;
		this->SetSelector(&this->scenery_sel);
	}
	_scenery.temp_item = nullptr;
	this->build_forbidden_reason = STR_NULL;
	this->scenery_sel.SetSize(0, 0);
	this->MarkDirty();
}

void SceneryGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case SCENERY_ROTATE_POS:
		case SCENERY_ROTATE_NEG:
			this->orientation += (number == SCENERY_ROTATE_POS ? 3 : 1);
			this->orientation %= 4;
			_scenery.temp_item = nullptr;
			this->build_forbidden_reason = STR_NULL;
			this->scenery_sel.SetSize(0, 0);
			this->MarkDirty();
			break;

		case SCENERY_CATEGORY_TREES:
			this->SetCategory(SCC_TREES);
			break;
		case SCENERY_CATEGORY_FLOWERBEDS:
			this->SetCategory(SCC_FLOWERBEDS);
			break;
		case SCENERY_CATEGORY_FOUNTAINS:
			this->SetCategory(SCC_FOUNTAINS);
			break;

		case SCENERY_GUI_LIST: {
			if (pos.y < TEXT_HEIGHT || pos.y > TEXT_HEIGHT + ITEM_HEIGHT) break;
			const int index = pos.x / ITEM_WIDTH;
			if (index < 0) break;

			const int first_index = this->GetWidget<ScrollbarWidget>(SCENERY_GUI_SCROLL_LIST)->GetStart();
			if (index < first_index || index + first_index >= static_cast<int>(this->types.size())) break;

			this->SetType(this->types[index + first_index]);
			break;
		}

		default:
			break;
	}
}

void SceneryGui::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	if (this->instance.get() == nullptr) return;
	this->instance->RemoveFromWorld();
	_scenery.temp_item = nullptr;
	this->build_forbidden_reason = STR_NULL;
	this->instance->orientation = this->orientation;
	const Point32 world_pos = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);
	const int8 dx = _orientation_signum_dx[vp->orientation];
	const int8 dy = _orientation_signum_dy[vp->orientation];
	scenery_sel.MarkDirty();
	bool placed = false;
	for (int z = WORLD_Z_SIZE - 1; z >= 0; z--) {
		const int dz = (z - (vp->view_pos.z / 256)) / 2;
		XYZPoint16 location(world_pos.x / 256 + dz * dx, world_pos.y / 256 + dz * dy, z);
		this->instance->vox_pos = location;
		const StringID err = this->instance->CanPlace();
		if (err != STR_NULL) {
			if (IsMoreImportantReason(this->build_forbidden_reason, err)) this->build_forbidden_reason = err;
			continue;
		}

		XYZPoint16 extent = OrientatedOffset(this->instance->orientation, this->selected_type->width_x, this->selected_type->width_y);
		if (extent.x < 0) {
			location.x += extent.x + 1;
			extent.x *= -1;
		}
		if (extent.y < 0) {
			location.y += extent.y + 1;
			extent.y *= -1;
		}
		this->scenery_sel.SetPosition(location.x, location.y);
		this->scenery_sel.SetSize(extent.x, extent.y);
		for (int8 x = 0; x < this->selected_type->width_x; x++) {
			for (int8 y = 0; y < this->selected_type->width_y; y++) {
				this->scenery_sel.AddVoxel(this->instance->vox_pos + OrientatedOffset(this->instance->orientation, x, y));
			}
		}
		this->scenery_sel.SetupRideInfoSpace();
		const uint16 voxel_data = _scenery.GetSceneryTypeIndex(this->selected_type);
		for (int8 x = 0; x < this->selected_type->width_x; x++) {
			for (int8 y = 0; y < this->selected_type->width_y; y++) {
				const XYZPoint16 pos = this->instance->vox_pos + OrientatedOffset(this->instance->orientation, x, y);
				this->scenery_sel.SetRideData(pos, SRI_SCENERY, voxel_data);
			}
		}

		_scenery.temp_item = this->instance.get();
		this->instance->InsertIntoWorld();
		placed = true;
		this->build_forbidden_reason = STR_NULL;
		break;
	}
	if (!placed) {
		this->instance->vox_pos = XYZPoint16::invalid();
		scenery_sel.SetSize(0, 0);
	}
	scenery_sel.MarkDirty();
}

void SceneryGui::SelectorMouseButtonEvent(const uint8 state)
{
	if ((state & MB_RIGHT) != 0) {
		Viewport *vp = _window_manager.GetViewport();
		const Point32 world_pos = vp->ComputeHorizontalTranslation(
				vp->rect.width  / 2 - _window_manager.GetMousePosition().x,
				vp->rect.height / 2 - _window_manager.GetMousePosition().y);
		const int8 dx = _orientation_signum_dx[vp->orientation];
		const int8 dy = _orientation_signum_dy[vp->orientation];
		for (int z = WORLD_Z_SIZE - 1; z >= 0; z--) {
			const int dz = (z - (vp->view_pos.z / 256)) / 2;
			XYZPoint16 location(world_pos.x / 256 + dz * dx, world_pos.y / 256 + dz * dy, z);
			if (!IsVoxelstackInsideWorld(location.x, location.y)) continue;

			SceneryInstance *i = _scenery.GetItem(location);
			if (i != nullptr && i != _scenery.temp_item) {
				if (i->type->category == SCC_SCENARIO && _game_mode_mgr.InPlayMode()) {
					ShowActionErrorMessage(ACT_REMOVE, GUI_ERROR_MESSAGE_UNREMOVABLE);
				} else {
					const Money &cost = i->NeedsWatering() ? i->type->return_cost_dry : i->type->return_cost;
					if (CheckActionAllowed(ACT_REMOVE, location, cost)) {
						_finances_manager.PayLandscaping(cost);
						_scenery.RemoveItem(i->vox_pos);
					}
				}
				return;
			}
		}
		return;
	}

	if (this->instance.get() == nullptr) return;
	if (!IsLeftClick(state)) return;
	if (scenery_sel.area.width < 1 || scenery_sel.area.height < 1) {
		ShowActionErrorMessage(ACT_BUILD, this->build_forbidden_reason);
		return;
	}
	if (!CheckActionAllowed(ACT_BUILD, this->instance->vox_pos, this->selected_type->buy_cost)) return;

	this->instance->RemoveFromWorld();  // The scenery manager will want to re-insert it, so we must unlink it first.
	_finances_manager.PayLandscaping(this->selected_type->buy_cost);
	_scenery.AddItem(this->instance.release());

	this->SetType(this->selected_type);  // Prepare to place another instance.
}

/**
 * Open the scenery GUI.
 * @ingroup gui_group
 */
void ShowSceneryGui()
{
	if (HighlightWindowByType(WC_SCENERY, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new SceneryGui;
}
