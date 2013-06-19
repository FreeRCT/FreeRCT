/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file nodes.h Declarations of the RCD nodes. */

#ifndef NODES_H
#define NODES_H

#include <string>
#include <list>
#include <set>
#include <map>
#include "image.h"

class FileWriter;
class FileBlock;
class NamedValueList;
class Position;

/** Base class for all nodes. */
class BlockNode {
public:
	BlockNode();
	virtual ~BlockNode();

	virtual BlockNode *GetSubNode(int row, int col, char *name, const Position &pos);
};

/** Base class for game blocks. */
class GameBlock : public BlockNode {
public:
	GameBlock(const char *blk_name, int version);
	/* virtual */ ~GameBlock();

	virtual int Write(FileWriter *fw) = 0;

	const char *blk_name; ///< %Name of the block.
	int version;          ///< Version of the block.
};

/** Node representing an RCD file. */
class FileNode : public BlockNode {
public:
	FileNode(char *file_name);
	/* virtual */ ~FileNode();

	void Write(FileWriter *fw);

	char *file_name;              ///< %Name of the RCD file.
	std::list<GameBlock *>blocks; ///< Blocks of the file.
};

/** A sequence of file nodes. */
class FileNodeList {
public:
	FileNodeList();
	~FileNodeList();

	std::list<FileNode *>files; ///< Output files.
};

/** Sprites of a surface. */
enum SurfaceSprites {
	SF_FLAT,       ///< Flat surface tile.
	SF_N,          ///< North corner up.
	SF_E,          ///< East corner up.
	SF_NE,         ///< North, east corners up.
	SF_S,          ///< South corner up.
	SF_NS,         ///< North, south corners up.
	SF_ES,         ///< East, south corners up.
	SF_NES,        ///< North, east, south corners up.
	SF_W,          ///< West corner up.
	SF_WN,         ///< West, north corners up.
	SF_WE,         ///< West, east corners up.
	SF_WNE,        ///< West, north, east corners up.
	SF_WS,         ///< West, south corners up.
	SF_WNS,        ///< West, north, south corners up.
	SF_WES,        ///< West, east, south corners up.
	SF_STEEP_NB,   ///< Steep north slope bottom.
	SF_STEEP_EB,   ///< Steep east  slope bottom.
	SF_STEEP_SB,   ///< Steep south slope bottom.
	SF_STEEP_WB,   ///< Steep west  slope bottom.
	SF_STEEP_NT,   ///< Steep north slope top
	SF_STEEP_ET,   ///< Steep east  slope top
	SF_STEEP_ST,   ///< Steep south slope top
	SF_STEEP_WT,   ///< Steep west  slope top
	SURFACE_COUNT, ///< Number of tiles in a surface.
};

/** Block containing a sprite. */
class SpriteBlock : public BlockNode {
public:
	SpriteBlock();
	/* virtual */ ~SpriteBlock();

	int Write(FileWriter *fw);

	SpriteImage sprite_image; ///< The stored sprite.
};

class BitMask;

/** Block containing a sprite sheet. */
class SheetBlock : public BlockNode {
public:
	SheetBlock(const Position &pos);
	/* virtual */ ~SheetBlock();


	/* virtual */ BlockNode *GetSubNode(int row, int col, char *name, const Position &pos);
	Image *GetSheet();

	Position pos;     ///< Line number defining the sheet.
	std::string file; ///< %Name of the file containing the sprite sheet.
	int x_base;       ///< Horizontal base offset in the sheet.
	int y_base;       ///< Vertical base offset in the sheet.
	int x_step;       ///< Column step size.
	int y_step;       ///< Row step size.
	int x_offset;     ///< Sprite offset (from the origin to the left edge of the sprite).
	int y_offset;     ///< Sprite offset (from the origin to the top edge of the sprite).
	int width;        ///< Width of a sprite.
	int height;       ///< Height of a sprite.

	Image *img_sheet; ///< Sheet of images.
	BitMask *mask;    ///< Bit mask to apply first (if available).
};

/** A 'TSEL' block. */
class TSELBlock : public GameBlock {
public:
	TSELBlock();
	/* virtual */ ~TSELBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[SURFACE_COUNT]; ///< Surface tiles.
};

/** A 'TCOR' block. */
class TCORBlock : public GameBlock {
public:
	TCORBlock();
	/* virtual */ ~TCORBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *north[SURFACE_COUNT]; ///< Corner select tiles while viewing to north.
	SpriteBlock *east[SURFACE_COUNT];  ///< Corner select tiles while viewing to east.
	SpriteBlock *south[SURFACE_COUNT]; ///< Corner select tiles while viewing to south.
	SpriteBlock *west[SURFACE_COUNT];  ///< Corner select tiles while viewing to west.
};

/** Ground surface block SURF. */
class SURFBlock : public GameBlock {
public:
	SURFBlock();
	/* virtual */ ~SURFBlock();

	/* virtual */ int Write(FileWriter *fw);

	int surf_type;  ///< Type of surface.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[SURFACE_COUNT]; ///< Surface tiles.
};

/** Sprites of a foundation. */
enum FoundationSprites {
	FND_SE_E0, ///< Vertical south-east foundation, east visible, south down.
	FND_SE_0S, ///< Vertical south-east foundation, east down, south visible.
	FND_SE_ES, ///< Vertical south-east foundation, east visible, south visible.
	FND_SW_S0, ///< Vertical south-west foundation, south visible, west down.
	FND_SW_0W, ///< Vertical south-west foundation, south down, west visible.
	FND_SW_SW, ///< Vertical south-west foundation, south visible, west visible.

	FOUNDATION_COUNT, ///< Number of foundation sprites.
};

/** Foundation game block FUND. */
class FUNDBlock : public GameBlock {
public:
	FUNDBlock();
	/* virtual */ ~FUNDBlock();

	/* virtual */ int Write(FileWriter *fw);

	int found_type; ///< Type of foundation.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[FOUNDATION_COUNT]; ///< Foundation tiles.
};

/** Colour ranges. */
enum ColourRange {
	COL_GREY,        ///< Colour range for grey.
	COL_GREEN_BROWN, ///< Colour range for green_brown.
	COL_BROWN,       ///< Colour range for brown.
	COL_YELLOW,      ///< Colour range for yellow.
	COL_DARK_RED,    ///< Colour range for dark_red.
	COL_DARK_GREEN,  ///< Colour range for dark_green.
	COL_LIGHT_GREEN, ///< Colour range for light_green.
	COL_GREEN,       ///< Colour range for green.
	COL_LIGHT_RED,   ///< Colour range for light_red.
	COL_DARK_BLUE,   ///< Colour range for dark_blue.
	COL_BLUE,        ///< Colour range for blue.
	COL_LIGHT_BLUE,  ///< Colour range for light_blue.
	COL_PURPLE,      ///< Colour range for purple.
	COL_RED,         ///< Colour range for red.
	COL_ORANGE,      ///< Colour range for orange.
	COL_SEA_GREEN,   ///< Colour range for sea_green.
	COL_PINK,        ///< Colour range for pink.
	COL_BEIGE,       ///< Colour range for beige.

	COLOUR_COUNT,    ///< Number of colour ranges.
};

/** Colour range remapping definition. */
class Recolouring : public BlockNode {
public:
	Recolouring();
	/* virtual */ ~Recolouring();

	uint8 orig;     ///< Colour range to replace.
	uint32 replace; ///< Bitset of colour ranges that may be used as replacement.

	uint32 Encode() const;
};

/** Definition of graphics of one type of person. */
class PersonGraphics : public BlockNode {
public:
	PersonGraphics();
	/* virtual */ ~PersonGraphics();

	int person_type;      ///< Type of person being defined.
	Recolouring recol[3]; ///< Recolour definitions.

	bool AddRecolour(uint8 orig, uint32 replace);
};

/** Person graphics game block. */
class PRSGBlock : public GameBlock {
public:
	PRSGBlock();
	/* virtual */ ~PRSGBlock();

	/* virtual */ int Write(FileWriter *fw);

	std::list<PersonGraphics> person_graphics; ///< Stored person graphics.
};

/** ANIM frame data for a single frame. */
class FrameData : public BlockNode {
public:
	FrameData();
	/* virtual */ ~FrameData();

	int duration; ///< Duration of this frame.
	int change_x; ///< Change in x after the frame is displayed.
	int change_y; ///< Change in y after the frame is displayed.
};

/** ANIM game block. */
class ANIMBlock : public GameBlock {
public:
	ANIMBlock();
	/* virtual */ ~ANIMBlock();

	/* virtual */ int Write(FileWriter *fw);

	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::list<FrameData> frames; ///< Frame data of every frame.
};

/** ANSP game block. */
class ANSPBlock : public GameBlock {
public:
	ANSPBlock();
	/* virtual */ ~ANSPBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width;  ///< Size of the tile.
	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::list<SpriteBlock *> frames; ///< Sprite for every frame.
};

/** Path sprites. */
enum PathSprites {
	PTS_EMPTY,               ///< EMPTY.
	PTS_NE,                  ///< NE.
	PTS_SE,                  ///< SE.
	PTS_NE_SE,               ///< NE_SE.
	PTS_NE_SE_E,             ///< NE_SE_E.
	PTS_SW,                  ///< SW.
	PTS_NE_SW,               ///< NE_SW.
	PTS_SE_SW,               ///< SE_SW.
	PTS_SE_SW_S,             ///< SE_SW_S.
	PTS_NE_SE_SW,            ///< NE_SE_SW.
	PTS_NE_SE_SW_E,          ///< NE_SE_SW_E.
	PTS_NE_SE_SW_S,          ///< NE_SE_SW_S.
	PTS_NE_SE_SW_E_S,        ///< NE_SE_SW_E_S.
	PTS_NW,                  ///< NW.
	PTS_NE_NW,               ///< NE_NW.
	PTS_NE_NW_N,             ///< NE_NW_N.
	PTS_NW_SE,               ///< NW_SE.
	PTS_NE_NW_SE,            ///< NE_NW_SE.
	PTS_NE_NW_SE_N,          ///< NE_NW_SE_N.
	PTS_NE_NW_SE_E,          ///< NE_NW_SE_E.
	PTS_NE_NW_SE_N_E,        ///< NE_NW_SE_N_E.
	PTS_NW_SW,               ///< NW_SW.
	PTS_NW_SW_W,             ///< NW_SW_W.
	PTS_NE_NW_SW,            ///< NE_NW_SW.
	PTS_NE_NW_SW_N,          ///< NE_NW_SW_N.
	PTS_NE_NW_SW_W,          ///< NE_NW_SW_W.
	PTS_NE_NW_SW_N_W,        ///< NE_NW_SW_N_W.
	PTS_NW_SE_SW,            ///< NW_SE_SW.
	PTS_NW_SE_SW_S,          ///< NW_SE_SW_S.
	PTS_NW_SE_SW_W,          ///< NW_SE_SW_W.
	PTS_NW_SE_SW_S_W,        ///< NW_SE_SW_S_W.
	PTS_NE_NW_SE_SW,         ///< NE_NW_SE_SW.
	PTS_NE_NW_SE_SW_N,       ///< NE_NW_SE_SW_N.
	PTS_NE_NW_SE_SW_E,       ///< NE_NW_SE_SW_E.
	PTS_NE_NW_SE_SW_N_E,     ///< NE_NW_SE_SW_N_E.
	PTS_NE_NW_SE_SW_S,       ///< NE_NW_SE_SW_S.
	PTS_NE_NW_SE_SW_N_S,     ///< NE_NW_SE_SW_N_S.
	PTS_NE_NW_SE_SW_E_S,     ///< NE_NW_SE_SW_E_S.
	PTS_NE_NW_SE_SW_N_E_S,   ///< NE_NW_SE_SW_N_E_S.
	PTS_NE_NW_SE_SW_W,       ///< NE_NW_SE_SW_W.
	PTS_NE_NW_SE_SW_N_W,     ///< NE_NW_SE_SW_N_W.
	PTS_NE_NW_SE_SW_E_W,     ///< NE_NW_SE_SW_E_W.
	PTS_NE_NW_SE_SW_N_E_W,   ///< NE_NW_SE_SW_N_E_W.
	PTS_NE_NW_SE_SW_S_W,     ///< NE_NW_SE_SW_S_W.
	PTS_NE_NW_SE_SW_N_S_W,   ///< NE_NW_SE_SW_N_S_W.
	PTS_NE_NW_SE_SW_E_S_W,   ///< NE_NW_SE_SW_E_S_W.
	PTS_NE_NW_SE_SW_N_E_S_W, ///< NE_NW_SE_SW_N_E_S_W.
	PTS_NE_EDGE_UP,          ///< NE_EDGE_UP.
	PTS_NW_EDGE_UP,          ///< NW_EDGE_UP.
	PTS_SE_EDGE_UP,          ///< SE_EDGE_UP.
	PTS_SW_EDGE_UP,          ///< SW_EDGE_UP.

	PTS_COUNT,               ///< Number of path sprites.
};

/** PATH game block. */
class PATHBlock : public GameBlock {
public:
	PATHBlock();
	/* virtual */ ~PATHBlock();

	/* virtual */ int Write(FileWriter *fw);

	int path_type;   ///< Type of path.
	int tile_width;  ///< Size of the tile.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.

	SpriteBlock *sprites[PTS_COUNT]; ///< Path sprites.
};

/** Platform sprites. */
enum PlatformSprites {
	PLA_NS,            ///< Flat platform for north and south view.
	PLA_EW,            ///< Flat platform for east and west view.
	PLA_RAMP_NE,       ///< Platform with two legs is raised at the NE edge.
	PLA_RAMP_SE,       ///< Platform with two legs is raised at the SE edge.
	PLA_RAMP_SW,       ///< Platform with two legs is raised at the SW edge.
	PLA_RAMP_NW,       ///< Platform with two legs is raised at the NW edge.
	PLA_RIGHT_RAMP_NE, ///< Platform with right leg is raised at the NE edge.
	PLA_RIGHT_RAMP_SE, ///< Platform with right leg is raised at the SE edge.
	PLA_RIGHT_RAMP_SW, ///< Platform with right leg is raised at the SW edge.
	PLA_RIGHT_RAMP_NW, ///< Platform with right leg is raised at the NW edge.
	PLA_LEFT_RAMP_NE,  ///< Platform with left leg is raised at the NE edge.
	PLA_LEFT_RAMP_SE,  ///< Platform with left leg is raised at the SE edge.
	PLA_LEFT_RAMP_SW,  ///< Platform with left leg is raised at the SW edge.
	PLA_LEFT_RAMP_NW,  ///< Platform with left leg is raised at the NW edge.

	PLA_COUNT, ///< Number of platform sprites.
};

/** PLAT game block. */
class PLATBlock : public GameBlock {
public:
	PLATBlock();
	/* virtual */ ~PLATBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width;    ///< Zoom-width of a tile of the surface.
	int z_height;      ///< Change in Z height (in pixels) when going up or down a tile level.
	int platform_type; ///< Type of platform.

	SpriteBlock *sprites[PLA_COUNT]; ///< Platform sprites.
};

/** Sprites of the supports. */
enum SupportSprites {
	SPP_S_NS,    ///< Single height for flat terrain, north and south view.
	SPP_S_EW,    ///< Single height for flat terrain, east and west view.
	SPP_D_NS,    ///< Double height for flat terrain, north and south view.Double height for flat terrain, east and west view.
	SPP_D_EW,    ///< Double height for flat terrain, east and west view.
	SPP_P_NS,    ///< Double height for paths, north and south view.
	SPP_P_EW,    ///< Double height for paths, east and west view.
	SPP_N,       ///< Single height, north leg up.
	SPP_E,       ///< Single height, east leg up.
	SPP_NE,      ///< Single height, north, east legs up.
	SPP_S,       ///< Single height, south leg up.
	SPP_NS,      ///< Single height, north, south legs up.
	SPP_ES,      ///< Single height, east, south legs up.
	SPP_NES,     ///< Single height, north, east, south legs up.
	SPP_W,       ///< Single height, west leg up.
	SPP_NW,      ///< Single height, west, north legs up.
	SPP_EW,      ///< Single height, west, east legs up.
	SPP_NEW,     ///< Single height, west, north, east legs up.
	SPP_SW,      ///< Single height, west, south legs up.
	SPP_NSW,     ///< Single height, west, north, south legs up.
	SPP_ESW,     ///< Single height, west, east, south legs up.
	SPP_STEEP_N, ///< Double height for steep north slope.
	SPP_STEEP_E, ///< Double height for steep east slope.
	SPP_STEEP_S, ///< Double height for steep south slope.
	SPP_STEEP_W, ///< Double height for steep west slope.

	SPP_COUNT,   ///< Number of support sprites.
};

/** SUPP game block. */
class SUPPBlock : public GameBlock {
public:
	SUPPBlock();
	/* virtual */ ~SUPPBlock();

	/* virtual */ int Write(FileWriter *fw);

	int support_type; ///< Type of support.
	int tile_width;   ///< Zoom-width of a tile of the surface.
	int z_height;     ///< Change in Z height (in pixels) when going up or down a tile level.

	SpriteBlock *sprites[SPP_COUNT]; ///< Support sprites.
};

/** Known languages. */
enum Languages {
	LNG_DEFAULT, ///< Default language.
	LNG_EN_GB,   ///< en_GB language.
	LNG_NL_NL,   ///< nl_NL language.

	LNG_COUNT,   ///< Number of known languages.
};

int GetLanguageIndex(const char *lname, const Position &pos);

/** Texts of a single string. */
class TextNode : public BlockNode {
public:
	TextNode();
	/* virtual */ ~TextNode();

	int GetSize() const;
	void Write(FileBlock *fb) const;

	std::string name;                     ///< Name of the textnode (used as key).
	mutable std::string texts[LNG_COUNT]; ///< Text of the text node, in each language.
	mutable Position pos[LNG_COUNT];      ///< Positions defining the text (negative lines means undefined).
};

/**
 * Comparator of #TextNode objects for sorting them in the Strings::texts set.
 * @param tn1 First node to compare.
 * @param tn2 Second node to compare.
 * @return \a tn1 should be in front of \a tn2.
 */
inline bool operator<(const TextNode &tn1, const TextNode &tn2)
{
	return tn1.name < tn2.name;
}

/** Collection of translated strings. */
class Strings : public BlockNode {
public:
	Strings();
	/* virtual */ ~Strings();

	void CheckTranslations(const char *names[], int name_count, const Position &pos);
	int Write(FileWriter *fw);

	std::set<TextNode> texts; ///< Translated text nodes.
};

/** Class for describing a SHOP game block. */
class SHOPBlock : public GameBlock {
public:
	SHOPBlock();
	/* virtual */ ~SHOPBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width;       ///< Zoom-width of a tile of the surface.
	int height;           ///< Height of the shop in voxels.
	int flags;            ///< Byte with flags of the shop.
	SpriteBlock *ne_view; ///< Unrotated view.
	SpriteBlock *se_view; ///< Rotated 90 degrees.
	SpriteBlock *sw_view; ///< Rotated 180 degrees.
	SpriteBlock *nw_view; ///< Rotated 270 degrees.
	Recolouring recol[3]; ///< Recolour definitions of the shop.
	int item_cost[2];     ///< Cost of both items at sale.
	int ownership_cost;   ///< Monthly cost of having the shop.
	int opened_cost;      ///< Additional monthly cost of having an opened shop.
	int item_type[2];     ///< Item type of both items at sale.
	Strings *shop_text;   ///< Texts of the shop.
};

/** GBOR game block. */
class GBORBlock : public GameBlock {
public:
	GBORBlock();
	/* virtual */ ~GBORBlock();

	/* virtual */ int Write(FileWriter *fw);

	int widget_type;   ///< Widget type.
	int border_top;    ///< Border width of the top edge.
	int border_left;   ///< Border width of the left edge.
	int border_right;  ///< Border width of the right edge.
	int border_bottom; ///< Border width of the bottom edge.
	int min_width;     ///< Minimal width of the border.
	int min_height;    ///< Minimal height of the border.
	int h_stepsize;    ///< Horizontal stepsize of the border.
	int v_stepsize;    ///< Vertical stepsize of the border.
	SpriteBlock *tl;   ///< Top-left sprite.
	SpriteBlock *tm;   ///< Top-middle sprite.
	SpriteBlock *tr;   ///< Top-right sprite.
	SpriteBlock *ml;   ///< Left sprite.
	SpriteBlock *mm;   ///< Middle sprite.
	SpriteBlock *mr;   ///< Right sprite.
	SpriteBlock *bl;   ///< Bottom-left sprite.
	SpriteBlock *bm;   ///< Bottom-middle sprite.
	SpriteBlock *br;   ///< Bottom-right sprite.
};

/** GCHK game block. */
class GCHKBlock : public GameBlock {
public:
	GCHKBlock();
	/* virtual */ ~GCHKBlock();

	/* virtual */ int Write(FileWriter *fw);

	int widget_type;             ///< Widget type.
	SpriteBlock *empty;          ///< Empty sprite.
	SpriteBlock *filled;         ///< Filled.
	SpriteBlock *empty_pressed;  ///< Empty pressed.
	SpriteBlock *filled_pressed; ///< Filled pressed.
	SpriteBlock *shaded_empty;   ///< Shaded empty button.
	SpriteBlock *shaded_filled;  ///< Shaded filled button.
};

/** GSLI game block. */
class GSLIBlock : public GameBlock {
public:
	GSLIBlock();
	/* virtual */ ~GSLIBlock();

	/* virtual */ int Write(FileWriter *fw);

	int min_length;      ///< Minimal length of the bar.
	int step_size;       ///< Stepsize of the bar.
	int width;           ///< Width of the slider button.
	int widget_type;     ///< Widget type.
	SpriteBlock *left;   ///< Left sprite.
	SpriteBlock *middle; ///< Middle sprite.
	SpriteBlock *right;  ///< Right sprite.
	SpriteBlock *slider; ///< Slider button.
};

/** GSCL game block. */
class GSCLBlock : public GameBlock {
public:
	GSCLBlock();
	/* virtual */ ~GSCLBlock();

	/* virtual */ int Write(FileWriter *fw);

	int min_length;     ///< Minimal length scrollbar.
	int step_back;      ///< Stepsize of background.
	int min_bar_length; ///< Minimal length bar.
	int bar_step;       ///< Stepsize of bar.
	int widget_type;    ///< Widget type.
	SpriteBlock *left_button;        ///< Left/up button.
	SpriteBlock *right_button;       ///< Right/down button.
	SpriteBlock *left_pressed;       ///< Left/up pressed button.
	SpriteBlock *right_pressed;      ///< Right/down pressed button.
	SpriteBlock *left_bottom;        ///< Left/top bar bottom (the background).
	SpriteBlock *middle_bottom;      ///< Middle bar bottom (the background).
	SpriteBlock *right_bottom;       ///< Right/down bar bottom (the background).
	SpriteBlock *left_top;           ///< Left/top bar top.
	SpriteBlock *middle_top;         ///< Middle bar top.
	SpriteBlock *right_top;          ///< Right/down bar top.
	SpriteBlock *left_top_pressed;   ///< Left/top pressed bar top.
	SpriteBlock *middle_top_pressed; ///< Middle pressed bar top.
	SpriteBlock *right_top_pressed;  ///< Right/down pressed bar top.
};

/** BDIR game block. */
class BDIRBlock : public GameBlock {
public:
	BDIRBlock();
	/* virtual */ ~BDIRBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width;   ///< Zoom-width of a tile of the surface.
	SpriteBlock *sprite_ne; ///< ne arrow.
	SpriteBlock *sprite_se; ///< se arrow.
	SpriteBlock *sprite_sw; ///< sw arrow.
	SpriteBlock *sprite_nw; ///< nw arrow.
};

/** GSLP Game block. */
class GSLPBlock : public GameBlock {
public:
	GSLPBlock();
	/* virtual */ ~GSLPBlock();

	/* virtual */ int Write(FileWriter *fw);

	SpriteBlock *vert_down;     ///< Slope going vertically down.
	SpriteBlock *steep_down;    ///< Slope going steeply down.
	SpriteBlock *gentle_down;   ///< Slope going gently down.
	SpriteBlock *level;         ///< Level slope.
	SpriteBlock *gentle_up;     ///< Slope going gently up.
	SpriteBlock *steep_up;      ///< Slope going steeply up.
	SpriteBlock *vert_up;       ///< Slope going vertically up.
	SpriteBlock *pos_2d;        ///< Flat rotation positive direction (counter clock wise)
	SpriteBlock *neg_2d;        ///< Flat rotation negative direction (clock wise)
	SpriteBlock *pos_3d;        ///< Diametric rotation positive direction (counter clock wise)
	SpriteBlock *neg_3d;        ///< Diametric rotation negative direction (clock wise)
	SpriteBlock *close_button;  ///< Close Button.
	SpriteBlock *maxi_button;   ///< Maximise button.
	SpriteBlock *mini_button;   ///< Minimise button.
	SpriteBlock *terraform_dot; ///< Terraform dot.
	Strings *gui_text;          ///< Text of the guis (reference to a TEXT block).
};

/** Class for storing the data of a single voxel in a track piece. */
class TrackVoxel : public BlockNode {
public:
	TrackVoxel();
	/* virtual */ ~TrackVoxel();

	void Write(FileWriter *fw, FileBlock *fb, int rot);

	int dx; ///< Relative X position of the voxel.
	int dy; ///< Relative Y position of the voxel.
	int dz; ///< Relative Z position of the voxel.
	int space; ///< Corners claimed by the voxel.
	SpriteBlock *back[4];  ///< Back coaster sprites.
	SpriteBlock *front[4]; ///< Front coaster sprites.
};

/** Class for storing a 'connection' between track pieces. */
class Connection : public BlockNode {
public:
	Connection();
	Connection(const Connection &c);
	Connection &operator=(const Connection &c);
	Connection(const std::string &name, int direction);
	/* virtual */ ~Connection();

	uint8 Encode(const std::map<std::string, int> &connections, int rot);

	std::string name; ///< Name of the connection.
	int direction;    ///< Direction of the connection.
};

/** A 'track_piece' block. */
class TrackPieceNode : public BlockNode {
public:
	TrackPieceNode();
	/* virtual */ ~TrackPieceNode();

	void UpdateConnectionMap(std::map<std::string, int> *connections);
	void Write(const std::map<std::string, int> &connections, FileWriter *fw, FileBlock *fb);

	int track_flags;   ///< Flags of the track piece.
	int cost;          ///< Cost of the track piece.
	Connection *entry; ///< Entry connection code.
	Connection *exit;  ///< Exit connection code.
	int exit_dx;       ///< Relative X position of the exit voxel.
	int exit_dy;       ///< Relative Y position of the exit voxel.
	int exit_dz;       ///< Relative Z position of the exit voxel.
	int speed;         ///< If non-0, minimal speed of cars (with direction).

	std::list<TrackVoxel *> track_voxels; ///< Voxels in the track piece.
};

/** 'RCST' game block. */
class RCSTBlock : public GameBlock {
public:
	RCSTBlock();
	/* virtual */ ~RCSTBlock();

	/* virtual */ int Write(FileWriter *fw);

	int coaster_type;  ///< Type of roller coaster.
	int platform_type; ///< Type of platform.
	Strings *text;     ///< Text of the coaster (reference to a TEXT block).

	std::list<TrackPieceNode *> track_blocks; ///< Track pieces of the coaster.
};

/** Node block containing a bitmask. */
class BitMask : public BlockNode {
public:
	BitMask();
	/* virtual */ ~BitMask();

	BitMaskData data; ///< Data of the bit mask.
};


FileNodeList *CheckTree(NamedValueList *values);

void GenerateStringsHeaderFile(const char *prefix, const char *base, const char *header);
void GenerateStringsCodeFile(const char *prefix, const char *base, const char *code);

#endif

