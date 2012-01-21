/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_gui.cpp Path building and editing. */

#include "stdafx.h"
#include "map.h"
#include "window.h"
#include "viewport.h"
#include "language.h"
#include "table/strings.h"
#include "table/gui_sprites.h"

PathBuildData _path_builder; ///< Path build helper data.

/** Imploded path tile sprite number to use for an 'up' slope from a given edge. */
static const PathSprites _path_up_from_edge[EDGE_COUNT] = {
	PATH_RAMP_NE, ///< EDGE_NE
	PATH_RAMP_SE, ///< EDGE_SE
	PATH_RAMP_SW, ///< EDGE_SW
	PATH_RAMP_NW, ///< EDGE_NW
};

/** Imploded path tile sprite number to use for an 'down' slope from a given edge. */
static const PathSprites _path_down_from_edge[EDGE_COUNT] = {
	PATH_RAMP_SW, ///< EDGE_NE
	PATH_RAMP_NW, ///< EDGE_SE
	PATH_RAMP_NE, ///< EDGE_SW
	PATH_RAMP_SE, ///< EDGE_NW
};

/**
 * Get the right path sprite for putting in the world.
 * @param tsl Slope of the path.
 * @param edge Edge to connect from.
 * @todo Path sprites should connect to neighbouring paths.
 */
static uint8 GetPathSprite(TrackSlope tsl, TileEdge edge)
{
	assert(edge < EDGE_COUNT);

	switch (tsl) {
		case TSL_FLAT: return PATH_EMPTY;
		case TSL_DOWN: return _path_down_from_edge[edge];
		case TSL_UP:   return _path_up_from_edge[edge];
		default: NOT_REACHED();
	}
}

/**
 * In the given voxel, can a path be build in the voxel from the bottom at the given edge?
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param edge Entry edge.
 * @return Bit-set of track slopes, indicating the directions of building paths.
 */
static uint8 CanBuildPathFromEdge(int16 xpos, int16 ypos, int8 zpos, TileEdge edge)
{
	if (zpos < 0 || zpos >= MAX_VOXEL_STACK_SIZE - 1) return 0;

	const VoxelStack *vs = _world.GetStack(xpos, ypos);

	const Voxel *above = (zpos < MAX_VOXEL_STACK_SIZE) ? vs->Get(zpos + 1) : NULL;
	if (above != NULL && above->GetType() != VT_EMPTY) return 0; // Not empty just above us -> path will not work here.

	const Voxel *below = (zpos > 0) ? vs->Get(zpos - 1) : NULL;
	if (below != NULL && below->GetType() == VT_SURFACE) {
		const SurfaceVoxelData *svd = below->GetSurface();
		if (svd->path.type != PT_INVALID) return 0; // A path just below us won't work either.
	}

	const Voxel *level = vs->Get(zpos);
	if (level != NULL) {
		switch (level->GetType()) {
			case VT_COASTER:
				return 0; // A roller-coaster  is in the way.

			case VT_SURFACE: {
				const SurfaceVoxelData *svd = level->GetSurface();
				if (svd->foundation.type != FDT_INVALID) return 0;
				if (svd->path.type != PT_INVALID) {
					if (svd->path.slope < PATH_FLAT_COUNT) return 1 << TSL_FLAT; // Already a flat path there.
					if (_path_down_from_edge[edge] == svd->path.slope) return 1 << TSL_UP; // Already a sloped path up.
					return 0; // A path but cannot connect to it.
				}
				if (svd->ground.type != GTP_INVALID) {
					TileSlope ts = ExpandTileSlope(svd->ground.slope);
					if ((ts & TCB_STEEP) != 0) return 0; // Steep slope in the way.
					if ((ts & _corners_at_edge[edge]) != 0) return 0; // Some hilly stuff at the entry.
				}
				break;
			}

			case VT_EMPTY:
				break;

			case VT_REFERENCE:
				// XXX return 0 is a too brute-force.
				return 0;

			default:
				NOT_REACHED();
		}
	}

	/* No trivial cases or show stoppers, do a more precise check for each slope.
	 * Above: empty
	 * Below: Does not contain a surface + path.
	 * Level: Is not a coaster or a reference, does not contain a path or foundations, has no steep ground nor raised corners at the entrance edge.
	 */
	uint8 result = 0;

	/* Try building a upward slope.
	 * Works if not at the top, and the voxel at z+2 is also empty.
	 */
	if (zpos < MAX_VOXEL_STACK_SIZE - 2) {
		const Voxel *v = vs->Get(zpos + 2);
		if (v == NULL || v->GetType() == VT_EMPTY) result |= 1 << TSL_UP;
	}

	/* Try building a level slope. */
	{
		if (level == NULL) {
			result |= 1 << TSL_FLAT;
		} else {
			VoxelType vt = level->GetType();
			if (vt == VT_EMPTY) {
				result |= 1 << TSL_FLAT;
			} else {
				assert(vt == VT_SURFACE);
				const SurfaceVoxelData *svd = level->GetSurface();
				assert(svd->path.type == PT_INVALID && svd->foundation.type == FDT_INVALID);
				if (svd->ground.type != GTP_INVALID && svd->ground.slope == 0) result |= 1 << TSL_FLAT;
			}
		}
	}

	/* Try building a downward slope. */
	if ((level == NULL || level->GetType() == VT_EMPTY) && zpos > 0) {
		VoxelType vt = (below != NULL) ? below->GetType() : VT_EMPTY;
		if (vt == VT_EMPTY) {
			result |= 1 << TSL_DOWN;
		} else if (vt == VT_SURFACE) {
			const SurfaceVoxelData *svd = below->GetSurface();
			if (svd->foundation.type == FDT_INVALID && svd->path.type == PT_INVALID) {
				/* No foundations and paths. */
				if (svd->ground.type == GTP_INVALID) {
					result |= 1 << TSL_DOWN;
				} else {
					TileSlope ts = ExpandTileSlope(svd->ground.slope);
					if (((TCB_STEEP | _corners_at_edge[(edge + 2) % 4]) & ts) == 0) result |= 1 << TSL_DOWN;
				}
			}
		}
	}

	return result;
}

/**
 * Compute the attach points of a path in a voxel.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
static uint8 GetPathAttachPoints(int16 xpos, int16 ypos, int8 zpos)
{
	if (xpos < 0 || xpos >= _world.GetXSize()) return 0;
	if (ypos < 0 || ypos >= _world.GetYSize()) return 0;
	if (zpos < 0 || zpos >= MAX_VOXEL_STACK_SIZE - 1) return 0; // the voxel containing the flat path, and one above it.

	const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
	if (v->GetType() != VT_SURFACE) return 0; // XXX Maybe also handle referenced surface voxels?
	const SurfaceVoxelData *svd = v->GetSurface();

	uint8 edges = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		int16 x = xpos + _tile_dxy[edge].x;
		int16 y = ypos + _tile_dxy[edge].y;
		if (x < 0 || x >= _world.GetXSize() || y < 0 || y >= _world.GetYSize()) continue;

		if (svd->path.type != PT_INVALID) {
			if (svd->path.slope < PATH_FLAT_COUNT) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (_path_down_from_edge[edge] == svd->path.slope
						&& CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
				if (_path_up_from_edge[edge] == svd->path.slope
						&& CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		if (svd->ground.type != GTP_INVALID) {
			TileSlope ts = ExpandTileSlope(svd->ground.slope);
			if ((ts & TCB_STEEP) != 0) continue;
			if ((ts & _corners_at_edge[edge]) == 0) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		/* No path and no ground -> Invalid, do next edge. */
	}
	return edges;
}

/** Constructor of the path builder data storage. */
PathBuildData::PathBuildData()
{
	this->tile_selected = false;
	this->path_gui_opened = false;
	this->world_additions = false;
}

/** Restart the path build interaction sequence. */
void PathBuildData::Reset()
{
	this->tile_selected = false;
	this->world_additions = false;
	DisableWorldAdditions();
	if (this->path_gui_opened) NotifyChange(WC_PATH_BUILDER, CHG_UPDATE_BUTTONS, 0);
}

/**
 * Set the state of the #path_gui_opened flag.
 * @param opened The gui is being opened (if \c false, it is being closed).
 */
void PathBuildData::SetPathGui(bool opened)
{
	this->path_gui_opened = opened;
}

/**
 * User clicked to select a tile.
 * Decide whether the click was valid.
 * @param xpos X coordinate of the clicked tile.
 * @param ypos Y coordinate of the clicked tile.
 * @param zpos Z coordinate of the clicked tile.
 * @return Click was valid, tile cursor should be updated.
 */
bool PathBuildData::IsTileClickValid(int16 xpos, int16 ypos, int8 zpos)
{
	return GetPathAttachPoints(xpos, ypos, zpos) != 0;
}

/** User clicked at a new and valid tile. #Viewport::tile_cursor has been updated. */
void PathBuildData::TileClicked()
{
	this->tile_selected = true;
	NotifyChange(WC_PATH_BUILDER, CHG_UPDATE_BUTTONS, 0);
}

/**
 * Get the allowed directions of the arrows.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
uint8 PathBuildData::GetArrowDirections()
{
	if (!this->tile_selected) return 0;

	Viewport *vp = GetViewport();
	return GetPathAttachPoints(vp->tile_cursor.xpos, vp->tile_cursor.ypos, vp->tile_cursor.zpos);
}

/**
 * Path build Gui.
 * @ingroup gui_group
 */
class PathBuildGui : public GuiWindow {
public:
	PathBuildGui();
	~PathBuildGui();

	virtual void OnClick(int16 number);
	virtual void OnChange(int code, uint32 parameter);

	/**
	 * Allowed build directions for the currently selected tile (unrotated).
	 * Lower 4 bits connect next to the voxel, upper 4 bits connect to the voxel above it.
	 */
	uint8 build_directions;
	TileEdge selected_dir; ///< Selected edge (unrotated).

	void SetWorldAdditions();
	void SetButtons();
};

/**
 * Widget numbers of the path build Gui.
 * @ingroup gui_group
 */
enum PathBuildWidgets {
	PATH_GUI_SLOPE_DOWN,         ///< Button 'go down'.
	PATH_GUI_SLOPE_FLAT,         ///< Button 'flat'.
	PATH_GUI_SLOPE_UP,           ///< Button 'go up'.
	PATH_GUI_NE_DIRECTION,       ///< Build arrow in NE direction.
	PATH_GUI_SE_DIRECTION,       ///< Build arrow in SE direction.
	PATH_GUI_SW_DIRECTION,       ///< Build arrow in SW direction.
	PATH_GUI_NW_DIRECTION,       ///< Build arrow in NW direction.
	PATH_GUI_FORWARD,            ///< Move the arrow a path tile forward.
	PATH_GUI_BACKWARD,           ///< Move the arrow a path tile backward.
	PATH_GUI_LONG,               ///< Build a long path.
	PATH_GUI_BUY,                ///< Buy a path tile.
	PATH_GUI_REMOVE,             ///< Remove a path tile.
};

static const int SPR_NE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NE; ///< Sprite for building in NE direction.
static const int SPR_SE_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SE; ///< Sprite for building in SE direction.
static const int SPR_SW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_SW; ///< Sprite for building in SW direction.
static const int SPR_NW_DIRECTION = SPR_GUI_BUILDARROW_START + EDGE_NW; ///< Sprite for building in NW direction.

/**
 * Widget parts of the path build Gui.
 * @ingroup gui_group
 */
static const WidgetPart _path_build_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, 0), SetData(STR_PATH_GUI_TITLE, STR_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, 0),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, 0),
			Intermediate(0, 1),
				Intermediate(1, 5), SetPadding(5, 5, 0, 5),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Slope down/level/up. */
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_DOWN, 0),
							SetData(STR_PATH_GUI_SLOPE_DOWN, STR_PATH_GUI_SLOPE_DOWN_TIP),
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_FLAT, 0), SetPadding(0, 0, 0, 5),
							SetData(STR_PATH_GUI_SLOPE_FLAT, STR_PATH_GUI_SLOPE_FLAT_TIP),
					Widget(WT_TEXT_BUTTON, PATH_GUI_SLOPE_UP, 0), SetPadding(0, 0, 0, 5),
							SetData(STR_PATH_GUI_SLOPE_UP, STR_PATH_GUI_SLOPE_UP_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Intermediate(1, 3), SetPadding(5, 5, 0, 5),
					/* Four arrows direction. */
					Intermediate(2, 2), SetHorPIP(0, 2, 5), SetVertPIP(0, 2, 0),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NW_DIRECTION, 0),
								SetData(SPR_NW_DIRECTION, STR_PATH_GUI_NW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_NE_DIRECTION, 0),
								SetData(SPR_NE_DIRECTION, STR_PATH_GUI_NE_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SW_DIRECTION, 0),
								SetData(SPR_SW_DIRECTION, STR_PATH_GUI_SW_DIRECTION_TIP),
						Widget(WT_IMAGE_BUTTON, PATH_GUI_SE_DIRECTION, 0),
								SetData(SPR_SE_DIRECTION, STR_PATH_GUI_SE_DIRECTION_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					/* Forward/backward. */
					Intermediate(2, 1),
						Widget(WT_TEXT_BUTTON, PATH_GUI_FORWARD, 0),
								SetData(STR_PATH_GUI_FORWARD, STR_PATH_GUI_FORWARD_TIP),
						Widget(WT_TEXT_BUTTON, PATH_GUI_BACKWARD, 0),
								SetData(STR_PATH_GUI_BACKWARD, STR_PATH_GUI_BACKWARD_TIP),
				Intermediate(1, 7), SetPadding(5, 5, 5, 5), SetHorPIP(0, 2, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_LONG, 0), SetData(STR_PATH_GUI_LONG, STR_PATH_GUI_LONG_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_BUY, 0), SetData(STR_PATH_GUI_BUY, STR_PATH_GUI_BUY_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
					Widget(WT_TEXT_BUTTON, PATH_GUI_REMOVE, 0), SetData(STR_PATH_GUI_REMOVE, STR_PATH_GUI_BULLDOZER_TIP),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
			EndContainer(),
	EndContainer(),
};

PathBuildGui::PathBuildGui() : GuiWindow(WC_PATH_BUILDER)
{
	build_directions = 0;

	this->SetupWidgetTree(_path_build_gui_parts, lengthof(_path_build_gui_parts));
	assert(!_path_builder.path_gui_opened);
	_path_builder.SetPathGui(true);
	SetViewportMousemode();
}

PathBuildGui::~PathBuildGui()
{
	_path_builder.SetPathGui(false);
	if (GetViewport()->GetMouseMode() == MM_PATH_BUILDING) SetViewportMousemode();
}

/** Array with slope selection widget numbers. */
static const WidgetNumber _slope_widgets[] = {
	PATH_GUI_SLOPE_DOWN, PATH_GUI_SLOPE_FLAT, PATH_GUI_SLOPE_UP, INVALID_WIDGET_INDEX
};

/** Array with direction selection widget numbers. */
static const WidgetNumber _direction_widgets[] = {
	PATH_GUI_NE_DIRECTION, PATH_GUI_SE_DIRECTION, PATH_GUI_SW_DIRECTION, PATH_GUI_NW_DIRECTION, INVALID_WIDGET_INDEX
};

/* virtual */ void PathBuildGui::OnClick(WidgetNumber number)
{
	switch (number) {
		case PATH_GUI_SLOPE_DOWN:
		case PATH_GUI_SLOPE_FLAT:
		case PATH_GUI_SLOPE_UP:
			this->SetRadioButtonsSelected(_slope_widgets, number);
			this->SetWorldAdditions();
			break;

		case PATH_GUI_NE_DIRECTION:
		case PATH_GUI_SE_DIRECTION:
		case PATH_GUI_SW_DIRECTION:
		case PATH_GUI_NW_DIRECTION: {
			Viewport *vp = GetViewport();
			TileEdge edge = (TileEdge)AddOrientations((ViewOrientation)(number - PATH_GUI_NE_DIRECTION), vp->orientation);
			this->selected_dir = edge;
			this->SetButtons();
			break;
		}

		case PATH_GUI_FORWARD:
		case PATH_GUI_BACKWARD:
		case PATH_GUI_LONG:
		case PATH_GUI_BUY:
			if (_path_builder.world_additions) {
				_additions.Commit();
				Viewport *vp = GetViewport();
				WidgetNumber wnum = this->GetSelectedRadioButton(_slope_widgets);
				Point16 dxy = _tile_dxy[this->selected_dir];
				if ((vp->tile_cursor.xpos == 0 && dxy.x < 0) || (vp->tile_cursor.ypos == 0 && dxy.y < 0)
						|| (vp->tile_cursor.zpos == 0 && wnum == PATH_GUI_SLOPE_DOWN)) {
					_path_builder.Reset();
					break;
				}
				uint16 newx = vp->tile_cursor.xpos + dxy.x;
				uint16 newy = vp->tile_cursor.ypos + dxy.y;
				uint8  newz = vp->tile_cursor.zpos;
				if (wnum == PATH_GUI_SLOPE_DOWN) newz--;
				if (wnum == PATH_GUI_SLOPE_UP) newz++;
				if (newx >= _world.GetXSize() || newy >= _world.GetYSize() || newz >= MAX_VOXEL_STACK_SIZE) {
					_path_builder.Reset();
					break;
				}
				vp->tile_cursor.MarkDirty();
				vp->tile_cursor.xpos = newx;
				vp->tile_cursor.ypos = newy;
				vp->tile_cursor.zpos = newz;
				vp->tile_cursor.MarkDirty();
				this->SetButtons();
			}
			break;

		case PATH_GUI_REMOVE:
		default:
			break;
	}
}

/** Set the buttons at the path builder gui. */
void PathBuildGui::SetButtons()
{
	Viewport *vp = GetViewport();
	/* Update the arrow cursor. */
	if (this->selected_dir != INVALID_EDGE) {
		Point16 dxy = _tile_dxy[this->selected_dir];
		if (((1 << this->selected_dir) & this->build_directions) != 0) {
			vp->arrow_cursor.SetCursor(vp->tile_cursor.xpos + dxy.x,
					vp->tile_cursor.ypos + dxy.y, vp->tile_cursor.zpos, (CursorType)(CUR_TYPE_ARROW_NE + this->selected_dir));
		} else if (((0x10 << this->selected_dir) & this->build_directions) != 0) {
			vp->arrow_cursor.SetCursor(vp->tile_cursor.xpos + dxy.x,
					vp->tile_cursor.ypos + dxy.y, vp->tile_cursor.zpos + 1, (CursorType)(CUR_TYPE_ARROW_NE + this->selected_dir));
		} else {
			vp->arrow_cursor.SetInvalid();
			this->selected_dir = INVALID_EDGE;
		}
	}
	/* Stop or enable the arrow display. */
	if (this->selected_dir != INVALID_EDGE) {
		EnableWorldAdditions();
	} else {
		DisableWorldAdditions();
	}
	/* Update arrow buttons. */
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		TileEdge rot_edge = (TileEdge)SubtractOrientations((ViewOrientation)edge, vp->orientation);
		if (((0x11 << edge) & this->build_directions) != 0) {
			this->SetWidgetShaded(PATH_GUI_NE_DIRECTION + rot_edge, false);
			this->SetWidgetPressed(PATH_GUI_NE_DIRECTION + rot_edge, edge == this->selected_dir);
		} else {
			this->SetWidgetShaded(PATH_GUI_NE_DIRECTION + rot_edge, true);
		}
	}
	/* Update the slope buttons. */
	WidgetNumber wnum = (this->selected_dir != INVALID_EDGE) ? this->GetSelectedRadioButton(_slope_widgets) : INVALID_WIDGET_INDEX;
	uint8 allowed_slopes = CanBuildPathFromEdge(vp->arrow_cursor.xpos,
			vp->arrow_cursor.ypos, vp->arrow_cursor.zpos, (TileEdge)((this->selected_dir + 2) % 4));
	for (TrackSlope ts = TSL_BEGIN; ts < TSL_COUNT_GENTLE; ts++) {
		if (((1 << ts) & allowed_slopes) != 0) {
			this->SetWidgetShaded(PATH_GUI_SLOPE_DOWN + ts, false);
			this->SetWidgetPressed(PATH_GUI_SLOPE_DOWN + ts, ts + PATH_GUI_SLOPE_DOWN == wnum);
		} else {
			this->SetWidgetShaded(PATH_GUI_SLOPE_DOWN + ts, true);
		}
	}

	this->SetWorldAdditions();
}

/** Build a path in the #_additions world to show what will be build. */
void PathBuildGui::SetWorldAdditions()
{
	_additions.Clear();
	_path_builder.world_additions = false;

	if (!_path_builder.tile_selected || this->selected_dir == INVALID_EDGE) return;

	WidgetNumber wnum = this->GetSelectedRadioButton(_slope_widgets);
	if (wnum == INVALID_WIDGET_INDEX) return;

	Viewport *vp = GetViewport();
	uint8 allowed_slopes = CanBuildPathFromEdge(vp->arrow_cursor.xpos,
			vp->arrow_cursor.ypos, vp->arrow_cursor.zpos, (TileEdge)((this->selected_dir + 2) % 4));

	if (((1 << (wnum - PATH_GUI_SLOPE_DOWN)) & allowed_slopes) != 0) {
		uint8 zpos = (wnum == PATH_GUI_SLOPE_DOWN) ? vp->arrow_cursor.zpos - 1 : vp->arrow_cursor.zpos;
		Voxel *v = _additions.GetCreateVoxel(vp->arrow_cursor.xpos, vp->arrow_cursor.ypos, zpos, true);
		if (v->GetType() == VT_EMPTY) {
			SurfaceVoxelData svd;
			svd.path.type = PT_CONCRETE;
			svd.path.slope = GetPathSprite((TrackSlope)(wnum - PATH_GUI_SLOPE_DOWN),
					(TileEdge)((this->selected_dir + 2) % 4));
			svd.ground.type = GTP_INVALID;
			svd.foundation.type = FDT_INVALID;
			v->SetSurface(svd);
		} else if (v->GetType() == VT_SURFACE) {
			SurfaceVoxelData *svd = v->GetSurface();
			svd->path.type = PT_CONCRETE;
			svd->path.slope = GetPathSprite((TrackSlope)(wnum - PATH_GUI_SLOPE_DOWN),
					(TileEdge)((this->selected_dir + 2) % 4));
		}
		_path_builder.world_additions = true;
	}
}

/* virtual */ void PathBuildGui::OnChange(int code, uint32 parameter)
{
	switch (code) {
		case CHG_UPDATE_BUTTONS:
			/* Is a tile selected? */
			if (!_path_builder.tile_selected) {
				this->build_directions = 0;
				this->selected_dir = INVALID_EDGE;
				this->SetButtons();
				break;
			}

			/* A tile is selected, get the allowed arrow directions. */
			this->build_directions = _path_builder.GetArrowDirections();
			if (this->build_directions == 0) {
				this->selected_dir = INVALID_EDGE;
				Viewport *vp = GetViewport();
				vp->arrow_cursor.SetInvalid();
				_path_builder.tile_selected = false;
				this->SetButtons();
				break;
			}

			/* A slope is selected, update the world additions too. */
			this->SetWorldAdditions();
			this->SetButtons();
			break;

		case CHG_VIEWPORT_ROTATED:
			this->SetButtons();
			break;

		default:
			break;
	}
}

/**
 * Open the path build gui.
 * @ingroup gui_group
 */
void ShowPathBuildGui()
{
	if (HighlightWindowByType(WC_PATH_BUILDER)) return;
	new PathBuildGui();
}

