/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_gui.cpp %Path building and editing. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "gamecontrol.h"
#include "mouse_mode.h"
#include "path_build.h"
#include "gui_sprites.h"

/**
 * %Path build GUI.
 * @ingroup gui_group
 */
class PathBuildGui : public GuiWindow {
public:
	PathBuildGui();
	~PathBuildGui();

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;

	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;

	void SelectorMouseButtonEvent(uint8 state) override;
	void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos) override;


	RideMouseMode ride_selector; ///< Mouse mode selector for displaying new (or existing) paths.

private:
	void TryAddRemovePath(uint8 m_state);
	void BuildSinglePath(const XYZPoint16 &pos);
	void SetButtons();
	void SetupSelector();
	void BuyPathTile();
	void RemovePathTile();
	bool MoveSelection(bool move_forward);

	Rectangle16 path_type_button_size;             ///< Size of the path type buttons.
	const ImageData *path_type_sprites[PAT_COUNT]; ///< Sprite to use for showing the path type in the Gui.
	bool normal_path_types[PAT_COUNT];             ///< Which path types are normal paths.
	bool queue_path_types[PAT_COUNT];              ///< Which path types are queue paths.

	XYZPoint16 mouse_pos;     ///< Last found mouse position in the viewport window.
	XYZPoint16 build_pos;     ///< Position of directional build. Invalid if x coordinate is negative.
	TileEdge build_direction; ///< Direction of building (#INVALID_EDGE if no direction decided).
	PathType path_type;       ///< Selected type of path to use for building.
	TrackSlope sel_slope;     ///< Selected slope (#TSL_INVALID if not slope decided).
	ClickableSprite mouse_at; ///< Sprite below the mouse cursor (#CS_NONE means none).
	bool single_tile_mode;    ///< If set, build single tiles at the ground, else build directional
};

/**
 * Widget numbers of the path build GUI.
 * @ingroup gui_group
 */
enum PathBuildWidgets {
	PATH_GUI_SLOPE_DOWN,   ///< Button 'go down'.
	PATH_GUI_SLOPE_FLAT,   ///< Button 'flat'.
	PATH_GUI_SLOPE_UP,     ///< Button 'go up'.
	PATH_GUI_NE_DIRECTION, ///< Build arrow in NE direction.
	PATH_GUI_SE_DIRECTION, ///< Build arrow in SE direction.
	PATH_GUI_SW_DIRECTION, ///< Build arrow in SW direction.
	PATH_GUI_NW_DIRECTION, ///< Build arrow in NW direction.
	PATH_GUI_FORWARD,      ///< Move the arrow a path tile forward.
	PATH_GUI_BACKWARD,     ///< Move the arrow a path tile backward.
	PATH_GUI_BUY,          ///< Buy a path tile.
	PATH_GUI_REMOVE,       ///< Remove a path tile.
	PATH_GUI_NORMAL_PATH0, ///< Button to select #PAT_WOOD type normal paths.
	PATH_GUI_NORMAL_PATH1, ///< Button to select #PAT_TILED type normal paths.
	PATH_GUI_NORMAL_PATH2, ///< Button to select #PAT_ASPHALT type normal paths.
	PATH_GUI_NORMAL_PATH3, ///< Button to select #PAT_CONCRETE type normal paths.
	PATH_GUI_QUEUE_PATH0,  ///< Button to select #PAT_WOOD type queue paths.
	PATH_GUI_QUEUE_PATH1,  ///< Button to select #PAT_TILED type queue paths.
	PATH_GUI_QUEUE_PATH2,  ///< Button to select #PAT_ASPHALT type queue paths.
	PATH_GUI_QUEUE_PATH3,  ///< Button to select #PAT_CONCRETE type queue paths.
	PATH_GUI_SINGLE,       ///< Build a single path.
	PATH_GUI_DIRECTIONAL,  ///< Build a path using the path build interface.
};

static const int SPR_NE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NE; ///< Sprite for building in NE direction.
static const int SPR_SE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SE; ///< Sprite for building in SE direction.
static const int SPR_SW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SW; ///< Sprite for building in SW direction.
static const int SPR_NW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NW; ///< Sprite for building in NW direction.

/**
 * Widget parts of the path build GUI.
 * @ingroup gui_group
 */
static const WidgetPart _path_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_PATH_GUI_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(0, 1),
				Intermediate(1, 5), SetPadding(5, 5, 0, 5),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
					/* Slope down/level/up. */
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_DOWN, COL_RANGE_GREY),
							SetData(SPR_GUI_SLOPES_START + TSL_DOWN, GUI_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_FLAT, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_FLAT, GUI_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_IMAGE_BUTTON, PATH_GUI_SLOPE_UP, COL_RANGE_GREY), SetPadding(0, 0, 0, 5),
							SetData(SPR_GUI_SLOPES_START + TSL_UP, GUI_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Intermediate(1, 3), SetPadding(5, 5, 0, 5),
					/* Four arrows direction. */
					Intermediate(2, 2), SetHorPIP(0, 2, 5), SetVertPIP(0, 2, 0),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NW_DIRECTION, COL_RANGE_GREY),
								SetData(SPR_NW_DIRECTION, GUI_PATH_GUI_NW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NE_DIRECTION, COL_RANGE_GREY),
								SetData(SPR_NE_DIRECTION, GUI_PATH_GUI_NE_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SW_DIRECTION, COL_RANGE_GREY),
								SetData(SPR_SW_DIRECTION, GUI_PATH_GUI_SW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SE_DIRECTION, COL_RANGE_GREY),
								SetData(SPR_SE_DIRECTION, GUI_PATH_GUI_SE_DIRECTION_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
					/* Forward/backward. */
					Intermediate(2, 1),
						Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_FORWARD, COL_RANGE_GREY),
								SetData(GUI_PATH_GUI_FORWARD, GUI_PATH_GUI_FORWARD_TIP),
						Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_BACKWARD, COL_RANGE_GREY),
								SetData(GUI_PATH_GUI_BACKWARD, GUI_PATH_GUI_BACKWARD_TIP),
				Intermediate(1, 6), SetPadding(5, 5, 5, 5), SetHorPIP(0, 2, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX,      COL_RANGE_INVALID), SetFill(1, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX,      COL_RANGE_INVALID), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_BUY,    COL_RANGE_GREEN),   SetData(GUI_PATH_GUI_BUY, GUI_PATH_GUI_BUY_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX,      COL_RANGE_INVALID), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, PATH_GUI_REMOVE, COL_RANGE_GREY),    SetData(GUI_PATH_GUI_REMOVE, GUI_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX,      COL_RANGE_INVALID), SetFill(1, 0),
				Intermediate(5, 2), SetPadding(5, 2, 2, 2), SetHorPIP(0, 2, 0),
					Widget(WT_CENTERED_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetFill(1, 0), SetData(GUI_PATH_GUI_QUEUE_PATH, STR_NULL),
					Widget(WT_CENTERED_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetFill(1, 0), SetData(GUI_PATH_GUI_NORMAL_PATH, STR_NULL),

					Widget(WT_TEXT_BUTTON, PATH_GUI_QUEUE_PATH0, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),
					Widget(WT_TEXT_BUTTON, PATH_GUI_NORMAL_PATH0, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),

					Widget(WT_TEXT_BUTTON, PATH_GUI_QUEUE_PATH1, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),
					Widget(WT_TEXT_BUTTON, PATH_GUI_NORMAL_PATH1, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),

					Widget(WT_TEXT_BUTTON, PATH_GUI_QUEUE_PATH2, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),
					Widget(WT_TEXT_BUTTON, PATH_GUI_NORMAL_PATH2, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),

					Widget(WT_TEXT_BUTTON, PATH_GUI_QUEUE_PATH3, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),
					Widget(WT_TEXT_BUTTON, PATH_GUI_NORMAL_PATH3, COL_RANGE_GREY), SetData(STR_NULL, STR_NULL),
				Intermediate(1, 2),
					Widget(WT_TEXT_BUTTON, PATH_GUI_SINGLE, COL_RANGE_GREY), SetData(GUI_PATH_GUI_SINGLE, GUI_PATH_GUI_SINGLE_TIP),
					Widget(WT_TEXT_BUTTON, PATH_GUI_DIRECTIONAL, COL_RANGE_GREY), SetData(GUI_PATH_GUI_DIRECTIONAL, GUI_PATH_GUI_DIRECTIONAL_TIP),
			EndContainer(),
	EndContainer(),
};

/** Constructor of the path build gui. */
PathBuildGui::PathBuildGui() : GuiWindow(WC_PATH_BUILDER, ALL_WINDOWS_OF_TYPE), ride_selector(), path_type_button_size()
{
	const SpriteStorage *store = _sprite_manager.GetSprites(64); // GUI size.
	for (int i = 0; i < PAT_COUNT; i++) {
		switch (store->path_sprites[i].status) {
			case PAS_UNUSED:
				this->normal_path_types[i] = false;
				this->queue_path_types[i]  = false;
				this->path_type_sprites[i] = nullptr;
				break;

			case PAS_NORMAL_PATH:
				this->normal_path_types[i] = true;
				this->queue_path_types[i]  = false;
				this->path_type_sprites[i] = store->GetPathSprite(i, PATH_NE_NW_SE_SW, VOR_NORTH);
				this->path_type_button_size.MergeArea(GetSpriteSize(this->path_type_sprites[i]));
				break;

			case PAS_QUEUE_PATH:
				this->normal_path_types[i] = false;
				this->queue_path_types[i]  = true;
				this->path_type_sprites[i] = store->GetPathSprite(i, PATH_NE_SW, VOR_NORTH);
				this->path_type_button_size.MergeArea(GetSpriteSize(this->path_type_sprites[i]));
				break;

			default:
				NOT_REACHED();
		}
	}

	this->SetupWidgetTree(_path_build_gui_parts, lengthof(_path_build_gui_parts));

	/* Select an initial path type. */
	this->path_type = PAT_INVALID;
	for (int i = 0; i < PAT_COUNT; i++) {
		if (this->normal_path_types[i]) {
			this->path_type = (PathType)i;
			break;
		}
		if (this->path_type == PAT_INVALID && this->queue_path_types[i]) {
			this->path_type = (PathType)i; // Queue path is better than an invalid one, continue searching.
		}
	}

	this->SetSelector(&this->ride_selector);
	if (this->path_type == PAT_INVALID) {
		this->ride_selector.SetSize(0, 0);
	} else {
		this->ride_selector.SetSize(1, 1);
	}
	this->mouse_pos.x = -1; // Incorrect position, first mouse move will fix it.

	this->single_tile_mode = true;
	this->build_pos.x = -1;
	this->build_direction = INVALID_EDGE;
	this->sel_slope = TSL_INVALID;

	this->SetButtons();
}

PathBuildGui::~PathBuildGui()
{
	this->SetSelector(nullptr);
}

/**
 * Try to add (if LMB pressed) or remove (if RMB pressed) a path tile at the voxel pointed to by the mouse.
 * @param m_state Mouse button state.
 */
void PathBuildGui::TryAddRemovePath(uint8 m_state)
{
	if ((m_state & (MB_LEFT | MB_RIGHT)) == 0) return; // No buttons pressed.

	const Voxel *v = _world.GetVoxel(this->mouse_pos);
	if (!v) return; // No voxel -> no ground there.

	uint16 instance = v->GetInstance();
	if ((m_state & MB_LEFT) != 0) {
		this->BuildSinglePath(this->mouse_pos); // Build new path or change type of path.
	} else if (instance == SRI_PATH && (m_state & MB_RIGHT) != 0) {
		RemovePath(this->mouse_pos, false);
	}
}

void PathBuildGui::SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
{
	if (this->path_type == PAT_INVALID) return;

	FinderData fdata(CS_GROUND | CS_PATH, FW_TILE);
	this->mouse_at = vp->ComputeCursorPosition(&fdata);
	if (this->mouse_at == CS_NONE || fdata.voxel_pos == this->mouse_pos) return;

	this->mouse_pos = fdata.voxel_pos;
	if (this->single_tile_mode) this->TryAddRemovePath(_window_manager.GetMouseState());

	this->SetButtons();
	this->SetupSelector();
}

void PathBuildGui::SelectorMouseButtonEvent(uint8 state)
{
	if (this->ride_selector.area.width == 0 || this->path_type == PAT_INVALID) return;

	if (this->single_tile_mode) {
		this->TryAddRemovePath(state);
		return;
	}
	/* Directional build. */

	if (!IsLeftClick(state)) return;

	/* Click with directional build, set or move the build position. */
	this->ride_selector.MarkDirty();

	this->ride_selector.SetSize(1, 1);
	this->build_pos = this->mouse_pos;
	this->ride_selector.SetPosition(this->mouse_pos.x, this->mouse_pos.y);
	this->ride_selector.MarkDirty();
	this->SetButtons();
	this->SetupSelector();
}

/**
 * From a position with a path tile, get the neighbouring position, where the entry edge at the bottom matches with the exit edge at \a pos.
 * @param pos Start position.
 * @param direction Direction to move.
 * @return The coordinate of the neighbour voxel with its entry at the bottom from the path at \a pos, in the given direction.
 */
static XYZPoint16 GetNeighbourPathPosition(const XYZPoint16 &pos, TileEdge direction)
{
	uint8 dirs = GetPathAttachPoints(pos);
	int extra_z = ((dirs & (0x10 << direction)) != 0) ? 1 : 0;

	const Point16 &dxy = _tile_dxy[direction];
	return XYZPoint16(pos.x + dxy.x, pos.y + dxy.y, pos.z + extra_z);
}

/** Construct selector display. */
void PathBuildGui::SetupSelector()
{
	if (this->single_tile_mode || (!this->single_tile_mode && this->build_pos.x < 0)) {
		/* Single tile mode or directional mode without build position, always follow the mouse. */
		const Point16 &sel_base = this->ride_selector.area.base;
		if (sel_base.x != this->mouse_pos.x || sel_base.y != this->mouse_pos.y) {
			this->ride_selector.MarkDirty();

			this->ride_selector.SetSize(1, 1);
			this->ride_selector.SetPosition(this->mouse_pos.x, this->mouse_pos.y);
			this->ride_selector.MarkDirty();
		}
		return;
	}

	/* Directional mode, if a build position but no direction and slope, use build position for the cursor. */
	if (this->build_direction == INVALID_EDGE || this->sel_slope == TSL_INVALID) {
		const Point16 &sel_base = this->ride_selector.area.base;
		if (sel_base.x != this->build_pos.x || sel_base.y != this->build_pos.y) {
			this->ride_selector.MarkDirty();

			this->ride_selector.SetSize(1, 1);
			this->ride_selector.SetPosition(this->build_pos.x, this->build_pos.y);
			this->ride_selector.MarkDirty();
		}
		return;
	}

	/* Directional mode, build position, a direction and a slope. Show what will be build. */
	XYZPoint16 add_pos = GetNeighbourPathPosition(this->build_pos, this->build_direction);

	uint8 path_slope;
	switch (this->sel_slope) {
		case TSL_UP:
			path_slope = _path_up_from_edge[(this->build_direction + 2) % 4];
			break;

		case TSL_FLAT:
			path_slope = PATH_EMPTY;
			break;

		case TSL_DOWN:
			add_pos.z--;
			path_slope = _path_down_from_edge[(this->build_direction + 2) % 4];
			break;

		default: NOT_REACHED();
	}

	const Point16 &sel_base = this->ride_selector.area.base;
	if (sel_base.x != add_pos.x || sel_base.y != add_pos.y) this->ride_selector.MarkDirty();
	this->ride_selector.SetSize(1, 1);
	this->ride_selector.SetPosition(add_pos.x, add_pos.y);
	this->ride_selector.AddVoxel(add_pos);
	this->ride_selector.SetupRideInfoSpace(); // Make space for the ride data at 'add_pos'.

	VoxelTileData<VoxelRideData> &vtd = this->ride_selector.GetTileData(add_pos);
	if (!vtd.cursor_enabled) return;
	VoxelRideData &vrd = vtd.ride_info[add_pos.z - vtd.lowest];
	vrd.sri = SRI_PATH;
	vrd.instance_data = MakePathInstanceData(path_slope, this->path_type);
	this->ride_selector.MarkDirty();
}

/**
 * Build a single path tile ath \a pos of the current type. If it contains a path already, the path type is changed.
 * @param pos Position to build the path. Must be a ground voxel.
 */
void PathBuildGui::BuildSinglePath(const XYZPoint16 &pos)
{
	assert(this->path_type != PAT_INVALID);

	Voxel *v = _world.GetCreateVoxel(pos, false);
	if (v == nullptr || v->GetGroundType() == GTP_INVALID) return; // No voxel or no ground here.

	SmallRideInstance sri = v->GetInstance();
	if (sri == SRI_PATH) {
		/* Rebuilding the same path type can be useful for queue paths after their neighbours have
		 * changed, as queue paths prefer to connect to other queue paths.
		 */
		ChangePath(pos, this->path_type, false);
		return;
	}
	if (sri != SRI_FREE) return; // Some other ride here.

	TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
	if (ts == SL_FLAT) {
		BuildFlatPath(pos, this->path_type, false);
		return;
	}

	ts ^= TSB_NORTH | TSB_EAST | TSB_SOUTH | TSB_WEST; // Swap raised and not raised corners.
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		if (ts == _corners_at_edge[edge]) { // 'edge' is at the low end due to swapping.
			BuildUpwardPath(pos, edge, this->path_type, false);
			return;
		}
	}

	/* Wrong slope, ignore build request. */
}

void PathBuildGui::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case PATH_GUI_NORMAL_PATH0:
		case PATH_GUI_NORMAL_PATH1:
		case PATH_GUI_NORMAL_PATH2:
		case PATH_GUI_NORMAL_PATH3:
		case PATH_GUI_QUEUE_PATH0:
		case PATH_GUI_QUEUE_PATH1:
		case PATH_GUI_QUEUE_PATH2:
		case PATH_GUI_QUEUE_PATH3:
			wid->min_x = this->path_type_button_size.width + 2 + 2;
			wid->min_y = this->path_type_button_size.height + 2 + 2;
			break;
	}
}

void PathBuildGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	static Recolouring recolour; // Never changed.

	switch (wid_num) {
		case PATH_GUI_NORMAL_PATH0:
		case PATH_GUI_NORMAL_PATH1:
		case PATH_GUI_NORMAL_PATH2:
		case PATH_GUI_NORMAL_PATH3:
			if (this->normal_path_types[wid_num - PATH_GUI_NORMAL_PATH0]) {
				const ImageData *img = this->path_type_sprites[wid_num - PATH_GUI_NORMAL_PATH0];
				if (img != nullptr) {
					int dx = (wid->pos.width - path_type_button_size.width) / 2;
					int dy = (wid->pos.height - path_type_button_size.height) / 2;
					Point32 pt(GetWidgetScreenX(wid) + dx - path_type_button_size.base.x, GetWidgetScreenY(wid) + dy - path_type_button_size.base.y);
					_video.BlitImage(pt, img, recolour, GS_NORMAL);
				}
			}
			break;

		case PATH_GUI_QUEUE_PATH0:
		case PATH_GUI_QUEUE_PATH1:
		case PATH_GUI_QUEUE_PATH2:
		case PATH_GUI_QUEUE_PATH3:
			if (this->queue_path_types[wid_num - PATH_GUI_QUEUE_PATH0]) {
				const ImageData *img = this->path_type_sprites[wid_num - PATH_GUI_QUEUE_PATH0];
				if (img != nullptr) {
					int dx = (wid->pos.width - path_type_button_size.width) / 2;
					int dy = (wid->pos.height - path_type_button_size.height) / 2;
					Point32 pt(GetWidgetScreenX(wid) + dx - path_type_button_size.base.x, GetWidgetScreenY(wid) + dy - path_type_button_size.base.y);
					_video.BlitImage(pt, img, recolour, GS_NORMAL);
				}
			}
			break;
	}
}

void PathBuildGui::OnClick(WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case PATH_GUI_SLOPE_DOWN:
		case PATH_GUI_SLOPE_FLAT:
		case PATH_GUI_SLOPE_UP:
			this->sel_slope = static_cast<TrackSlope>(number - PATH_GUI_SLOPE_DOWN); // Verified in 'SetButtons'.
			this->SetButtons();
			this->SetupSelector();
			break;

		case PATH_GUI_NE_DIRECTION:
		case PATH_GUI_SE_DIRECTION:
		case PATH_GUI_SW_DIRECTION:
		case PATH_GUI_NW_DIRECTION: {
			Viewport *vp = _window_manager.GetViewport();
			if (vp != nullptr) {
				ViewOrientation clicked = static_cast<ViewOrientation>(number - PATH_GUI_NE_DIRECTION);
				TileEdge edge = static_cast<TileEdge>(AddOrientations(clicked, vp->orientation));
				this->build_direction = edge; // Verified in 'SetButtons'.
				this->SetButtons();
				this->SetupSelector();
			}
			break;
		}

		case PATH_GUI_FORWARD:
		case PATH_GUI_BACKWARD:
			this->MoveSelection(number == PATH_GUI_FORWARD);
			this->SetButtons();
			this->SetupSelector();
			break;

		case PATH_GUI_REMOVE:
			this->RemovePathTile();
			break;

		case PATH_GUI_BUY:
			this->BuyPathTile();
			break;

		case PATH_GUI_NORMAL_PATH0:
		case PATH_GUI_NORMAL_PATH1:
		case PATH_GUI_NORMAL_PATH2:
		case PATH_GUI_NORMAL_PATH3:
			if (this->normal_path_types[number - PATH_GUI_NORMAL_PATH0]) {
				this->path_type = static_cast<PathType>(number - PATH_GUI_NORMAL_PATH0);
				this->SetButtons();
				this->SetupSelector();
			}
			break;

		case PATH_GUI_QUEUE_PATH0:
		case PATH_GUI_QUEUE_PATH1:
		case PATH_GUI_QUEUE_PATH2:
		case PATH_GUI_QUEUE_PATH3:
			if (this->queue_path_types[number - PATH_GUI_QUEUE_PATH0]) {
				this->path_type = static_cast<PathType>(number - PATH_GUI_QUEUE_PATH0);
				this->SetButtons();
				this->SetupSelector();
			}
			break;

		case PATH_GUI_SINGLE:
			this->single_tile_mode = true;
			this->SetButtons();
			this->SetupSelector();
			break;

		case PATH_GUI_DIRECTIONAL:
			this->single_tile_mode = false;
			this->build_pos.x = -1;
			this->build_direction = INVALID_EDGE;

			this->SetButtons();
			this->SetupSelector();
			break;

		default:
			break;
	}
}

/**
 * Decide at which edges a path could be started from the given voxel.
 * @param v The voxel to investigate.
 * @param invalid Return value in case there is no ground at the voxel.
 * @param bottom If a path can be started at the lower level, which edges should be returned?
 *        Useful values are \c 1 (only the lower edge) and \c 0x11 (both lower end upper edge of the voxel).
 * @return For each edge which edges are useful to consider for starting a path.
 *         Low nibble defines interesting edge at the bottom of the voxel, High nibble defines interesting edges at the top of the voxel.
 */
static uint8 GetGroundEdgesForPaths(const Voxel *v, uint8 invalid = 0xFF, uint8 bottom = 1)
{
	if (v == nullptr || v->GetGroundType() == GTP_INVALID) return invalid;
	TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
	if ((slope & (TSB_STEEP | TSB_TOP)) == TSB_STEEP) return 0; // Bottom of a steep slope cannot have a path.
	uint directions = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		directions |= ((slope & _corners_at_edge[edge]) != 0) ? (0x10 << edge) : (bottom << edge);
	}
	return directions;
}

/**
 * As a first upper limit, which edges of the given voxel can be used to build a path from?
 * @param pos Current position.
 * @return For all 8 edges of the voxel (4 at the bottom in the low nibble, 4 at the top at the high nibble), whether they should be checked further.
 */
static uint8 GetDirections(const XYZPoint16 &pos)
{
	if (!IsVoxelstackInsideWorld(pos.x, pos.y)) {
		return 0xFF; // Assume any slope off-world.
	}

	const Voxel *v = _world.GetVoxel(pos);
	if (v == nullptr) {
		return 0xFF; // Free space is also free form.
	}

	SmallRideInstance sri = v->GetInstance();
	if (sri == SRI_PATH) { // Follow path at 'pos'.
		return GetPathExits(GetImplodedPathSlope(v->GetInstanceData()), false);
	}
	if (sri == SRI_FREE) { // No ride, but perhaps some ground?
		return GetGroundEdgesForPaths(v, 0x0F, 0x01);
	}
	return 0;
}

/** Set the buttons at the path builder GUI. */
void PathBuildGui::SetButtons()
{
	Viewport *vp = _window_manager.GetViewport();
	if (vp == nullptr) return;

	/* Compute feasible directions for the arrow buttons. */
	bool disabled = this->single_tile_mode || this->build_pos.x < 0;
	uint8 directions; // Bits in low nibble indicate buildable directions.
	uint8 exits; // For the valid 'build_pos' voxel, which edges of the voxel can have a path (low nibble bottom, high nibble top).
	if (disabled) {
		directions = 0; // Don't care, buttons are disabled.
		exits = 0;
	} else {
		assert(this->build_pos.x >= 0); // Realized by 'disabled' above.
		exits = GetDirections(this->build_pos);
		directions = 0;
		for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
			TileEdge reverse_edge = static_cast<TileEdge>((edge + 2) % 4);
			XYZPoint16 neighbour(this->build_pos.x + _tile_dxy[edge].x, this->build_pos.y + _tile_dxy[edge].y, this->build_pos.z);
			if (((0x01 << edge) & exits) != 0 && CanBuildPathFromEdge(neighbour, reverse_edge) != 0) {
				directions |= 1 << edge;
				continue;
			}
			if (((0x10 << edge) & exits) != 0) {
				neighbour.z++;
				if (CanBuildPathFromEdge(neighbour, reverse_edge) != 0) {
					directions |= 1 << edge;
					continue;
				}
			}
		}
	}

	/* Auto-(de)select build direction if possible. */
	if (this->build_direction != INVALID_EDGE && (directions & (1 << this->build_direction)) == 0) this->build_direction = INVALID_EDGE;

	if (this->build_direction == INVALID_EDGE) {
		for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
			if (directions == (1 << edge)) {
				this->build_direction = edge;
				break;
			}
		}
	}

	/* Update arrow buttons. */
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		TileEdge rot_edge = static_cast<TileEdge>(SubtractOrientations(static_cast<ViewOrientation>(edge), vp->orientation));
		this->SetWidgetShaded(PATH_GUI_NE_DIRECTION + rot_edge, disabled || ((0x11 << edge) & directions) == 0);
		this->SetWidgetPressed(PATH_GUI_NE_DIRECTION + rot_edge, edge == this->build_direction);
	}

	/* Compute allowed slopes. */
	disabled = this->single_tile_mode || this->build_pos.x < 0 || this->build_direction == INVALID_EDGE;
	uint8 allowed_slopes;
	if (disabled) {
		allowed_slopes = 0; // Slopes are disabled.
	} else {
		assert(this->build_pos.x >= 0); // Realized by 'disabled' above.
		assert(this->build_direction != INVALID_EDGE);

		TileEdge reverse_edge = static_cast<TileEdge>((this->build_direction + 2) % 4);
		XYZPoint16 neighbour(this->build_pos.x + _tile_dxy[this->build_direction].x, this->build_pos.y + _tile_dxy[this->build_direction].y, this->build_pos.z);
		if ((exits & (0x01 << this->build_direction)) != 0) {
			allowed_slopes = CanBuildPathFromEdge(neighbour, reverse_edge);
		} else if ((exits & (0x10 << this->build_direction)) != 0) {
			neighbour.z++;
			allowed_slopes = CanBuildPathFromEdge(neighbour, reverse_edge);
		} else {
			allowed_slopes = 0;
		}
	}

	/* Auto-(de)select a slope. */
	if (this->sel_slope != TSL_INVALID && (allowed_slopes & (1 << this->sel_slope)) == 0) this->sel_slope = TSL_INVALID;

	if (this->sel_slope == TSL_INVALID) {
		if (allowed_slopes == (1 << TSL_UP))   this->sel_slope = TSL_UP;
		if (allowed_slopes == (1 << TSL_FLAT)) this->sel_slope = TSL_FLAT;
		if (allowed_slopes == (1 << TSL_DOWN)) this->sel_slope = TSL_DOWN;
	}

	/* Update the slope buttons. */
	for (TrackSlope ts = TSL_BEGIN; ts < TSL_COUNT_GENTLE; ts++) {
		this->SetWidgetShaded(PATH_GUI_SLOPE_DOWN + ts, disabled || (allowed_slopes & (1 << ts)) == 0);
		this->SetWidgetPressed(PATH_GUI_SLOPE_DOWN + ts, ts == sel_slope);
	}

	disabled = this->single_tile_mode || this->build_direction == INVALID_EDGE || this->sel_slope == TSL_INVALID;
	this->SetWidgetShaded(PATH_GUI_BUY,      disabled);
	this->SetWidgetShaded(PATH_GUI_REMOVE,   disabled);
	this->SetWidgetShaded(PATH_GUI_FORWARD,  disabled);
	this->SetWidgetShaded(PATH_GUI_BACKWARD, disabled);

	/* Path type selection buttons. */
	for (int i = 0; i < PAT_COUNT; i++) {
		if (this->normal_path_types[i]) {
			this->SetWidgetShaded(PATH_GUI_NORMAL_PATH0 + i, false);
			this->SetWidgetShaded(PATH_GUI_QUEUE_PATH0 + i, true);
			this->SetWidgetPressed(PATH_GUI_NORMAL_PATH0 + i, (i == this->path_type));
		} else if (this->queue_path_types[i]) {
			this->SetWidgetShaded(PATH_GUI_NORMAL_PATH0 + i, true);
			this->SetWidgetShaded(PATH_GUI_QUEUE_PATH0 + i, false);
			this->SetWidgetPressed(PATH_GUI_QUEUE_PATH0 + i, (i == this->path_type));
		} else {
			this->SetWidgetShaded(PATH_GUI_NORMAL_PATH0 + i, true);
			this->SetWidgetShaded(PATH_GUI_QUEUE_PATH0 + i, true);
		}
	}

	/* Path mode selection. */
	this->SetWidgetPressed(PATH_GUI_SINGLE,      this->single_tile_mode);
	this->SetWidgetPressed(PATH_GUI_DIRECTIONAL, !this->single_tile_mode);
}

/** Add a path tile to the current position and orientation in the directional build mode. */
void PathBuildGui::BuyPathTile()
{
	if (this->path_type == PAT_INVALID) return;
	if (this->single_tile_mode) return;
	if (this->build_pos.x < 0) return;
	if (this->build_direction == INVALID_EDGE) return;

	XYZPoint16 path_pos = GetNeighbourPathPosition(this->build_pos, this->build_direction);
	switch (this->sel_slope) {
		case TSL_UP: {
			TileEdge start_edge = static_cast<TileEdge>((this->build_direction + 2) % 4);
			BuildUpwardPath(path_pos, start_edge, this->path_type, false);
			break;
		}
		case TSL_FLAT:
			BuildFlatPath(path_pos, this->path_type, false);
			break;

		case TSL_DOWN: {
			TileEdge start_edge = static_cast<TileEdge>((this->build_direction + 2) % 4);
			BuildDownwardPath(path_pos, start_edge, this->path_type, false);
			path_pos.z--;
			break;
		}

		case TSL_INVALID:
		default:
			return; // Ignore request.
	}
	this->build_pos = path_pos;
	this->SetButtons();
	this->SetupSelector();
}

/** Remove a path tile from the game in directional mode. */
void PathBuildGui::RemovePathTile()
{
	if (this->path_type == PAT_INVALID) return;
	if (this->single_tile_mode) return;
	if (this->build_pos.x < 0) return;
	if (this->build_direction == INVALID_EDGE) return;

	XYZPoint16 remove_pos = this->build_pos;
	if (RemovePath(remove_pos, true)) {
		if (!this->MoveSelection(false)) {
			this->build_pos.x = -1; // Moving failed, let the user select a new tile.
		}
		RemovePath(remove_pos, false);
		this->SetButtons();
		this->SetupSelector();
	}
}

/**
 * Move the cursor in forward or backward direction.
 * @param move_forward If set, move forward (in #build_direction), else move backward.
 * @return Whether the position was moved.
 */
bool PathBuildGui::MoveSelection(bool move_forward)
{
	if (this->path_type == PAT_INVALID) return false;
	if (this->single_tile_mode) return false; // Single tile mode has no direction.
	if (this->build_pos.x < 0) return false;
	if (this->build_direction == INVALID_EDGE) return false;

	TileEdge edge = move_forward ? this->build_direction : static_cast<TileEdge>((this->build_direction + 2) % 4);

	bool move_up;
	const Voxel *v = _world.GetVoxel(this->build_pos);
	if (v == nullptr) return false;
	if (HasValidPath(v)) {
		move_up = (GetImplodedPathSlope(v) == _path_down_from_edge[edge]);
	} else if (v->GetGroundType() != GTP_INVALID) {
		TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
		if ((ts & TSB_STEEP) != 0) return false;
		move_up = ((ts & _corners_at_edge[edge]) != 0);
	} else {
		return false; // Surface type but no valid ground/path surface.
	}

	/* Test whether we can move in the indicated direction. */
	Point16 dxy = _tile_dxy[edge];
	if ((dxy.x < 0 && this->build_pos.x == 0) || (dxy.x > 0 && this->build_pos.x == _world.GetXSize() - 1)) return false;
	if ((dxy.y < 0 && this->build_pos.y == 0) || (dxy.y > 0 && this->build_pos.y == _world.GetYSize() - 1)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(this->build_pos.x + dxy.x, this->build_pos.y + dxy.y) != OWN_PARK) return false;

	const Voxel *v_top, *v_bot;
	if (move_up) {
		/* Exit of current tile is at the top. */
		v_top = (this->build_pos.z > WORLD_Z_SIZE - 2) ? nullptr : _world.GetVoxel(this->build_pos + XYZPoint16(dxy.x, dxy.y, 1));
		v_bot = _world.GetVoxel(this->build_pos + XYZPoint16(dxy.x, dxy.y, 0));
	} else {
		/* Exit of current tile is at the bottom. */
		v_top = _world.GetVoxel(this->build_pos + XYZPoint16(dxy.x, dxy.y, 0));
		v_bot = (this->build_pos.z == 0) ? nullptr : _world.GetVoxel(this->build_pos + XYZPoint16(dxy.x, dxy.y, -1));
	}

	/* Try to find a voxel with a path. */
	if (v_top != nullptr && HasValidPath(v_top)) {
		this->build_pos.x += dxy.x;
		this->build_pos.y += dxy.y;
		if (move_up) this->build_pos.z++;
		return true;
	}
	if (v_bot != nullptr && HasValidPath(v_bot)) {
		this->build_pos.x += dxy.x;
		this->build_pos.y += dxy.y;
		if (!move_up) this->build_pos.z--;
		return true;
	}

	/* Try to find a voxel with surface. */
	if (v_top != nullptr && v_top->GetGroundType() != GTP_INVALID && !IsImplodedSteepSlope(v_top->GetGroundSlope())) {
		this->build_pos.x += dxy.x;
		this->build_pos.y += dxy.y;
		if (move_up) this->build_pos.z++;
		return true;
	}
	if (v_bot != nullptr && v_bot->GetGroundType() != GTP_INVALID && !IsImplodedSteepSlope(v_bot->GetGroundSlope())) {
		this->build_pos.x += dxy.x;
		this->build_pos.y += dxy.y;
		if (!move_up) this->build_pos.z--;
		return true;
	}
	return false;
}

void PathBuildGui::OnChange(ChangeCode code, uint32 parameter)
{
	switch (code) {
		case CHG_VIEWPORT_ROTATED:
			this->SetButtons();
			break;

		default:
			break;
	}
}

/**
 * Open the path build GUI.
 * @ingroup gui_group
 */
void ShowPathBuildGui()
{
	if (HighlightWindowByType(WC_PATH_BUILDER, ALL_WINDOWS_OF_TYPE)) return;
	new PathBuildGui;
}
