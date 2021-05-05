/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_objects_gui.cpp %PathObject placement Gui. */

#include "stdafx.h"
#include "finances.h"
#include "gamecontrol.h"
#include "mouse_mode.h"
#include "scenery.h"
#include "viewport.h"
#include "window.h"

/**
 * %PathObject build GUI.
 * @ingroup gui_group
 */
class PathObjectGui : public GuiWindow {
public:
	PathObjectGui();
	~PathObjectGui();

	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;
	void SelectorMouseButtonEvent(uint8 state) override;

	void SetType(const PathObjectType *t);

private:
	RideMouseMode path_object_sel;               ///< Mouse selector for building path objects.
	const PathObjectType *type;                  ///< Type currently being placed (or \c nullptr).
	std::unique_ptr<PathObjectInstance> object;  ///< Item being placed.
};

/**
 * Widget numbers of the PathObject build GUI.
 * @ingroup gui_group
 */
enum PathObjectWidgets {
	POBJ_BUTTON_BENCH,  ///< Place bench button.
	POBJ_BUTTON_BIN,    ///< Place bin   button.
	POBJ_BUTTON_LAMP,   ///< Place lamp  button.
	POBJ_TEXT_BENCH,    ///< Bench price text field.
	POBJ_TEXT_BIN,      ///< Bin   price text field.
	POBJ_TEXT_LAMP,     ///< Lamp  price text field.
};

/**
 * Widget parts of the path objects build GUI.
 * @ingroup gui_group
 */
static const WidgetPart _path_objects_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_PATH_OBJECTS_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(3, 2),
				Widget(WT_IMAGE_BUTTON, POBJ_BUTTON_BENCH, COL_RANGE_DARK_GREEN), SetData(SPR_GUI_BENCH, GUI_PATH_OBJECTS_BENCH), SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,    POBJ_TEXT_BENCH,   COL_RANGE_DARK_GREEN), SetData(STR_ARG1,      STR_NULL),
				Widget(WT_IMAGE_BUTTON, POBJ_BUTTON_BIN,   COL_RANGE_DARK_GREEN), SetData(SPR_GUI_BIN,   GUI_PATH_OBJECTS_BIN),   SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,    POBJ_TEXT_BIN,     COL_RANGE_DARK_GREEN), SetData(STR_ARG1,      STR_NULL),
				Widget(WT_IMAGE_BUTTON, POBJ_BUTTON_LAMP,  COL_RANGE_DARK_GREEN), SetData(SPR_GUI_LAMP,  GUI_PATH_OBJECTS_LAMP),  SetMinimalSize(32, 32),
				Widget(WT_LEFT_TEXT,    POBJ_TEXT_LAMP,    COL_RANGE_DARK_GREEN), SetData(STR_ARG1,      STR_NULL),
		EndContainer(),
	EndContainer(),
};

PathObjectGui::PathObjectGui() : GuiWindow(WC_PATH_OBJECTS, ALL_WINDOWS_OF_TYPE)
{
	this->type = nullptr;
	this->SetupWidgetTree(_path_objects_build_gui_parts, lengthof(_path_objects_build_gui_parts));
}

PathObjectGui::~PathObjectGui()
{
	SetSelector(nullptr);
}

/**
 * Sets what kind of path object type is currently being placed.
 * @param t Type to select.
 */
void PathObjectGui::SetType(const PathObjectType *t)
{
	if (this->type == t) {
		this->type = nullptr;
	} else {
		this->type = t;
	}

	this->GetWidget<LeafWidget>(POBJ_BUTTON_BENCH)->SetPressed(this->type == &PathObjectType::BENCH);
	this->GetWidget<LeafWidget>(POBJ_BUTTON_BIN  )->SetPressed(this->type == &PathObjectType::LITTERBIN);
	this->GetWidget<LeafWidget>(POBJ_BUTTON_LAMP )->SetPressed(this->type == &PathObjectType::LAMP);

	if (this->type == nullptr) {
		this->SetSelector(nullptr);
	} else {
		SetSelector(&this->path_object_sel);
	}
	this->path_object_sel.SetSize(0, 0);
	this->MarkDirty();
}

void PathObjectGui::OnClick(WidgetNumber wid_num, const Point16 &pos)
{
	switch (wid_num) {
		case POBJ_BUTTON_BENCH:
			this->SetType(&PathObjectType::BENCH);
			break;
		case POBJ_BUTTON_BIN:
			this->SetType(&PathObjectType::LITTERBIN);
			break;
		case POBJ_BUTTON_LAMP:
			this->SetType(&PathObjectType::LAMP);
			break;

		default:
			break;
	}
}

void PathObjectGui::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case POBJ_TEXT_BENCH:
			_str_params.SetMoney(1, PathObjectType::BENCH.buy_cost);
			break;
		case POBJ_TEXT_BIN:
			_str_params.SetMoney(1, PathObjectType::LITTERBIN.buy_cost);
			break;
		case POBJ_TEXT_LAMP:
			_str_params.SetMoney(1, PathObjectType::LAMP.buy_cost);
			break;

		default:
			break;
	}
}

void PathObjectGui::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	if (this->type == nullptr) return;
	this->object.reset();

	const Point32 world_pos = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);
	const int8 dx = _orientation_signum_dx[vp->orientation];
	const int8 dy = _orientation_signum_dy[vp->orientation];
	path_object_sel.MarkDirty();

	for (int z = WORLD_Z_SIZE - 1; z >= 0; z--) {
		const int dz = (z - (vp->view_pos.z / 256)) / 2;
		XYZPoint16 location(world_pos.x / 256 + dz * dx, world_pos.y / 256 + dz * dy, z);

		if (!IsVoxelstackInsideWorld(location.x, location.y)) continue;
		if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(location.x, location.y) != OWN_PARK) continue;

		const Voxel *v = _world.GetVoxel(location);
		if (v == nullptr || !HasValidPath(v)) continue;

		if (!this->type->can_exist_on_slope && GetImplodedPathSlope(v) >= PATH_FLAT_COUNT) continue;

		/* The item can be placed here. */
		this->object.reset(new PathObjectInstance(this->type, location, XYZPoint16()));
		_scenery.temp_path_object = this->object.get();
		this->path_object_sel.SetPosition(location.x, location.y);
		this->path_object_sel.SetSize(1, 1);
		this->path_object_sel.SetupRideInfoSpace();
		this->path_object_sel.MarkDirty();
		return;
	}

	this->path_object_sel.SetSize(0, 0);
	this->path_object_sel.MarkDirty();
}

void PathObjectGui::SelectorMouseButtonEvent(uint8 state)
{
	if (!IsLeftClick(state)) return;
	if (this->object == nullptr) return;
	if (!BestErrorMessageReason::CheckActionAllowed(BestErrorMessageReason::ACT_BUILD, this->type->buy_cost)) return;

	_finances_manager.PayLandscaping(this->type->buy_cost);
	_scenery.SetPathObjectInstance(this->object->vox_pos, this->type);
	this->object.reset();
}

/**
 * Open the path objects GUI.
 * @ingroup gui_group
 */
void ShowPathObjectsGui()
{
	if (HighlightWindowByType(WC_PATH_OBJECTS, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new PathObjectGui;
}
