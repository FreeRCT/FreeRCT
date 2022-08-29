/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file nodes.h Declarations of the RCD nodes. */

#ifndef NODES_H
#define NODES_H

#include <list>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <array>
#include "image.h"
#include "../language_definitions.h"

class FileWriter;
class FileBlock;
class NamedValueList;
class Position;

/** Base class for all nodes. */
class BlockNode {
public:
	virtual std::shared_ptr<BlockNode> GetSubNode(int row, int col, const char *name, const Position &pos);
};

/** Base class for named blocks. */
class NamedBlock : public BlockNode {
public:
	NamedBlock(const char *blk_name, int version);

	virtual int Write(FileWriter *fw) = 0;

	const char *blk_name; ///< %Name of the block.
	int version;          ///< Version of the block.
};

/** Base class for game blocks. */
class GameBlock : public NamedBlock {
public:
	GameBlock(const char *blk_name, int version);
};

/** Base class for meta blocks. */
class MetaBlock : public NamedBlock {
public:
	MetaBlock(const char *blk_name, int version);
};

/** Node representing an RCD file. */
class FileNode : public BlockNode {
public:
	FileNode(const std::string &file_name);
	virtual ~FileNode() = default;

	void Write(FileWriter *fw);

	std::string file_name; ///< %Name of the RCD file.
	std::list<std::shared_ptr<NamedBlock>> blocks; ///< Blocks of the file.
};

/** A sequence of file nodes. */
class FileNodeList {
public:
	std::list<std::shared_ptr<FileNode>> files; ///< Output files.
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
	virtual ~SpriteBlock() = default;

	int Write(FileWriter *fw);

	SpriteImage sprite_image; ///< The stored sprite.
};

class BitMask;

/** Class for storage of a filename pattern of the form \c "prefix{seq(first..last,length)}suffix". */
class FilePattern {
public:
	FilePattern();

	void SetFilename(const std::string &fname);
	int GetCount() const;
	std::string MakeFilename(int index) const;

	std::string prefix; ///< Common prefix part of the name.
	std::string suffix; ///< Common suffix part of the name.
	int first;          ///< First number to insert.
	int last;           ///< Last number to insert.
	int length;         ///< Length of the number part, \c 0 means no number and suffix available, \c -1 means no filename at all.
};

/** Block containing a sprite sheet. */
class SheetBlock : public BlockNode {
public:
	SheetBlock(const Position &pos);
	virtual ~SheetBlock();

	std::shared_ptr<BlockNode> GetSubNode(int row, int col, const char *name, const Position &pos) override;
	Image *GetSheet();

	Position pos;         ///< Line number defining the sheet.
	std::string file;     ///< %Name of the file containing the sprite sheet.
	std::string recolour; ///< %Name of the file containing 32bpp recolour information (\c "" means no file).

	int x_base;   ///< Horizontal base offset in the sheet.
	int y_base;   ///< Vertical base offset in the sheet.
	int x_step;   ///< Column step size.
	int y_step;   ///< Row step size.
	int x_count;  ///< Number of sprites horizontally (\c -1 means 'infinite').
	int y_count;  ///< Number of sprites vertically (\c -1 means 'infinite').
	int x_offset; ///< Sprite offset (from the origin to the left edge of the sprite).
	int y_offset; ///< Sprite offset (from the origin to the top edge of the sprite).
	int width;    ///< Width of a sprite.
	int height;   ///< Height of a sprite.
	bool crop;    ///< Crop sprite.

	ImageFile *imf;   ///< Loaded image file.
	Image *img_sheet; ///< Sheet of images.
	std::shared_ptr<BitMask> mask; ///< Bit mask to apply first (if available).
	ImageFile *rmf;   ///< Loaded recolour file.
	Image8bpp *rim;   ///< Recolour image.
};

/** A 'spritefiles' block. */
class SpriteFilesBlock : public BlockNode {
public:
	virtual ~SpriteFilesBlock() = default;

	std::shared_ptr<BlockNode> GetSubNode(int row, int col, const char *name, const Position &pos) override;

	Position pos;         ///< Line number defining the sheet.
	FilePattern file;     ///< %File pattern representing the files to load.
	FilePattern recolour; ///< %File pattern representing the files containing 32bpp recolour information, if available.

	int xbase;   ///< Horizontal base offset in the sheet.
	int ybase;   ///< Vertical base offset in the sheet.
	int xoffset; ///< Sprite offset (from the origin to the left edge of the sprite).
	int yoffset; ///< Sprite offset (from the origin to the top edge of the sprite).
	int width;   ///< Width of a sprite.
	int height;  ///< Height of a sprite.
	bool crop;   ///< Crop sprite.

	std::shared_ptr<BitMask> mask; ///< Bit mask to apply first (if available).
};

/** A 'TSEL' block. */
class TSELBlock : public GameBlock {
public:
	TSELBlock();
	virtual ~TSELBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> sprites; ///< Surface tiles.
};

/** A 'TCOR' block. */
class TCORBlock : public GameBlock {
public:
	TCORBlock();
	virtual ~TCORBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> north; ///< Corner select tiles while viewing to north.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> east;  ///< Corner select tiles while viewing to east.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> south; ///< Corner select tiles while viewing to south.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> west;  ///< Corner select tiles while viewing to west.
};

/** Ground surface block SURF. */
class SURFBlock : public GameBlock {
public:
	SURFBlock();
	virtual ~SURFBlock() = default;

	int Write(FileWriter *fw) override;

	int surf_type;  ///< Type of surface.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	std::array<std::shared_ptr<SpriteBlock>, SURFACE_COUNT> sprites; ///< Surface tiles.
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
	virtual ~FUNDBlock() = default;

	int Write(FileWriter *fw) override;

	int found_type; ///< Type of foundation.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	std::array<std::shared_ptr<SpriteBlock>, FOUNDATION_COUNT> sprites; ///< Foundation tiles.
};

/** Colour ranges. */
enum ColourRange {
	COL_GREY,            ///< Colour range for grey.
	COL_GREEN_BROWN,     ///< Colour range for green_brown.
	COL_ORANGE_BROWN,    ///< Colour range for orange_brown.
	COL_YELLOW,          ///< Colour range for yellow.
	COL_DARK_RED,        ///< Colour range for dark_red.
	COL_DARK_GREEN,      ///< Colour range for dark_green.
	COL_LIGHT_GREEN,     ///< Colour range for light_green.
	COL_GREEN,           ///< Colour range for green.
	COL_PINK_BROWN,      ///< Colour range for pink_brown.
	COL_DARK_PURPLE,     ///< Colour range for dark_purple.
	COL_BLUE,            ///< Colour range for blue.
	COL_DARK_JADE_GREEN, ///< Colour range for jade_green.
	COL_PURPLE,          ///< Colour range for purple.
	COL_RED,             ///< Colour range for red.
	COL_ORANGE,          ///< Colour range for orange.
	COL_SEA_GREEN,       ///< Colour range for sea_green.
	COL_PINK,            ///< Colour range for pink.
	COL_BROWN,           ///< Colour range for brown.

	COLOUR_COUNT,        ///< Number of colour ranges.
};

/** Colour range remapping definition. */
class Recolouring : public BlockNode {
public:
	Recolouring();
	virtual ~Recolouring() = default;

	uint8 orig;     ///< Colour range to replace.
	uint32 replace; ///< Bitset of colour ranges that may be used as replacement.

	uint32 Encode() const;
};

/** Definition of graphics of one type of person. */
class PersonGraphics : public BlockNode {
public:
	virtual ~PersonGraphics() = default;

	int person_type;      ///< Type of person being defined.
	Recolouring recol[3]; ///< Recolour definitions.

	bool AddRecolour(uint8 orig, uint32 replace);
};

/** Person graphics game block. */
class PRSGBlock : public GameBlock {
public:
	PRSGBlock();
	virtual ~PRSGBlock() = default;

	int Write(FileWriter *fw) override;

	std::vector<std::shared_ptr<PersonGraphics>> person_graphics; ///< Stored person graphics.
};

/** ANIM frame data for a single frame. */
class FrameData : public BlockNode {
public:
	virtual ~FrameData() = default;

	int duration; ///< Duration of this frame.
	int change_x; ///< Change in x after the frame is displayed.
	int change_y; ///< Change in y after the frame is displayed.
};

/** ANIM game block. */
class ANIMBlock : public GameBlock {
public:
	ANIMBlock();
	virtual ~ANIMBlock() = default;

	int Write(FileWriter *fw) override;

	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::vector<std::shared_ptr<FrameData>> frames; ///< Frame data of every frame.
};

/** ANSP game block. */
class ANSPBlock : public GameBlock {
public:
	ANSPBlock();
	virtual ~ANSPBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width;  ///< Size of the tile.
	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::vector<std::shared_ptr<SpriteBlock>> frames; ///< Sprite for every frame.
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
	virtual ~PATHBlock() = default;

	int Write(FileWriter *fw) override;

	int path_type;  ///< Type of path.
	int tile_width; ///< Size of the tile.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.

	std::array<std::shared_ptr<SpriteBlock>, PTS_COUNT> sprites; ///< Path sprites.
};

/** Path decoration PDEC block. */
class PDECBlock : public GameBlock {
public:
	PDECBlock();
	virtual ~PDECBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width;                                   ///< Width of the tile.
	std::shared_ptr<SpriteBlock> litter_bin[4];       ///< Litter bin (4 edges).
	std::shared_ptr<SpriteBlock> overflow_bin[4];     ///< Overflowing litter bin (4 edges).
	std::shared_ptr<SpriteBlock> demolished_bin[4];   ///< Demolished litter bin (4 edges).
	std::shared_ptr<SpriteBlock> lamp_post[4];        ///< Lamp post (4 edges).
	std::shared_ptr<SpriteBlock> demolished_post[4];  ///< Demolished lamp post (4 edges).
	std::shared_ptr<SpriteBlock> bench[4];            ///< Benches (4 edges).
	std::shared_ptr<SpriteBlock> demolished_bench[4]; ///< Demolished benches (4 edges).
	std::shared_ptr<SpriteBlock> litter_flat[4];      ///< Litter at flat path (4 types).
	std::shared_ptr<SpriteBlock> litter_ne[4];        ///< Litter at ramp, NE edge up (4 types).
	std::shared_ptr<SpriteBlock> litter_se[4];        ///< Litter at ramp, SE edge up (4 types).
	std::shared_ptr<SpriteBlock> litter_sw[4];        ///< Litter at ramp, SW edge up (4 types).
	std::shared_ptr<SpriteBlock> litter_nw[4];        ///< Litter at ramp, NW edge up (4 types).
	std::shared_ptr<SpriteBlock> vomit_flat[4];       ///< Vomit at flat path (4 types).
	std::shared_ptr<SpriteBlock> vomit_ne[4];         ///< Vomit at ramp, NE edge up (4 types).
	std::shared_ptr<SpriteBlock> vomit_se[4];         ///< Vomit at ramp, SE edge up (4 types).
	std::shared_ptr<SpriteBlock> vomit_sw[4];         ///< Vomit at ramp, SW edge up (4 types).
	std::shared_ptr<SpriteBlock> vomit_nw[4];         ///< Vomit at ramp, NW edge up (4 types).
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
	virtual ~PLATBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width;    ///< Zoom-width of a tile of the surface.
	int z_height;      ///< Change in Z height (in pixels) when going up or down a tile level.
	int platform_type; ///< Type of platform.

	std::array<std::shared_ptr<SpriteBlock>, PLA_COUNT> sprites; ///< Platform sprites.
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
	virtual ~SUPPBlock() = default;

	int Write(FileWriter *fw) override;

	int support_type; ///< Type of support.
	int tile_width;   ///< Zoom-width of a tile of the surface.
	int z_height;     ///< Change in Z height (in pixels) when going up or down a tile level.

	std::array<std::shared_ptr<SpriteBlock>, SPP_COUNT> sprites; ///< Support sprites.
};

int GetLanguageIndex(const char *lname, const Position &pos);

/** Storage of a single string (name + text + language if present). */
class StringNode : public BlockNode {
public:
	StringNode();
	virtual ~StringNode() = default;

	std::string name;  ///< Name of the string.
	std::vector<std::string> text;  ///< Text of the string named #name, indexed by plural form.
	Position text_pos; ///< Position of the text.
	std::string key;   ///< Key of the bundle containing this string (if provided).
	int lang_index;    ///< Language of the #text, negative if not defined. @see Languages
};

/** Collection of strings. */
class StringsNode : public BlockNode {
public:
	StringsNode();
	virtual ~StringsNode() = default;

	void Add(const StringNode &node, const Position &pos);
	void SetKey(const std::string &key, const Position &pos);
	std::string GetKey() const;

	std::list<StringNode> strings; ///< Collected strings.

private:
	std::string key; ///< Key associated with the strings, if any.
};

/** Texts (translations) of a single string. */
class TextNode {
public:
	TextNode(const std::string &name);

	void MergeStorage(const TextNode &storage);
	int GetSize() const;
	void Write(FileBlock *fb) const;

	const std::string name;                     ///< Name of the text node (used as key).
	std::vector<std::string> texts[LANGUAGE_COUNT];  ///< Text of the text node, in each language, indexed by plural form.
	Position pos[LANGUAGE_COUNT];                    ///< Positions defining the text (negative lines means undefined).
};

/** Collection of translated strings for a game object. */
class StringBundle : public BlockNode {
public:
	virtual ~StringBundle() = default;

	void Fill(std::shared_ptr<StringsNode> strs, const Position &pos);
	void MergeStorage(const StringBundle &storage);
	void CheckTranslations(const char *names[], int name_count, const Position &pos);
	int Write(FileWriter *fw);

	std::string key; ///< Name of the bundle, if available.
	std::map<std::string, TextNode> texts; ///< Translated text nodes, one for each name.
};

/** Class for describing a FSET game block. */
class FSETBlock : public GameBlock {
public:
	FSETBlock();
	virtual ~FSETBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.
	int width_x;    ///< The number of voxels in x direction.
	int width_y;    ///< The number of voxels in y direction.
	std::unique_ptr<std::shared_ptr<SpriteBlock>[]> ne_views; ///< Unrotated views.
	std::unique_ptr<std::shared_ptr<SpriteBlock>[]> se_views; ///< Rotated 90 degrees.
	std::unique_ptr<std::shared_ptr<SpriteBlock>[]> sw_views; ///< Rotated 180 degrees.
	std::unique_ptr<std::shared_ptr<SpriteBlock>[]> nw_views; ///< Rotated 270 degrees.
	bool unrotated_views_only;                                ///< Whether only the unrotated views have been specified.
	bool unrotated_views_only_allowed;                        ///< Whether it is allowed to specify only the unrotated views.
};

/** Class for describing a TIMA game block. */
class TIMABlock : public GameBlock {
public:
	TIMABlock();
	virtual ~TIMABlock() = default;

	int Write(FileWriter *fw) override;

	int frames;                       ///< The number of frames in the animation.
	std::unique_ptr<int[]> durations; ///< The duration of each frame in milliseconds.
	std::unique_ptr<std::shared_ptr<FSETBlock>[]> views; ///< The frames' image sets.
	bool unrotated_views_only;                           ///< Whether only the unrotated views have been specified.
	bool unrotated_views_only_allowed;                   ///< Whether it is allowed to specify only the unrotated views.
};

/** Class for describing a RIEE game block. */
class RIEEBlock : public GameBlock {
public:
	RIEEBlock();
	virtual ~RIEEBlock() = default;

	int Write(FileWriter *fw) override;

	std::string internal_name;                ///< The type's internal name.
	bool is_entrance;                         ///< True if this is an entrance, false if it's an exit.
	int tile_width;                           ///< Zoom-width of a tile of the surface.
	std::shared_ptr<StringBundle> texts;      ///< The entrance/exit's strings.
	std::shared_ptr<SpriteBlock> ne_views[2]; ///< Unrotated view.
	std::shared_ptr<SpriteBlock> se_views[2]; ///< Rotated 90 degrees.
	std::shared_ptr<SpriteBlock> sw_views[2]; ///< Rotated 180 degrees.
	std::shared_ptr<SpriteBlock> nw_views[2]; ///< Rotated 270 degrees.
	Recolouring recol[3];                     ///< Recolour definitions of the entrance/exit.
};

/** Class for describing a SHOP game block. */
class SHOPBlock : public GameBlock {
public:
	SHOPBlock();
	virtual ~SHOPBlock() = default;

	int Write(FileWriter *fw) override;

	std::string internal_name;  ///< The ride's internal name.
	int height;           ///< Height of the shop in voxels.
	int flags;            ///< Byte with flags of the shop.
	Recolouring recol[3]; ///< Recolour definitions of the shop.
	int item_cost[2];     ///< Cost of both items at sale.
	int ownership_cost;   ///< Monthly cost of having the shop.
	int opened_cost;      ///< Additional monthly cost of having an opened shop.
	int item_type[2];     ///< Item type of both items at sale.

	std::shared_ptr<FSETBlock> views;
	std::shared_ptr<StringBundle> shop_text;   ///< Texts of the shop.
};

/** Class for describing a FGTR game block. */
class FGTRBlock : public GameBlock {
public:
	FGTRBlock();
	virtual ~FGTRBlock() = default;

	int Write(FileWriter *fw) override;

	std::string internal_name;  ///< The ride's internal name.
	bool is_thrill_ride;  ///< True for thrill rides, false for gentle rides.
	int8 ride_width_x;    ///< The number of voxels the ride occupies in x direction.
	int8 ride_width_y;    ///< The number of voxels the ride occupies in y direction.
	std::unique_ptr<int8[]> heights; ///< Heights of the ride in voxels.
	Recolouring recol[3]; ///< Recolour definitions of the ride.
	int entrance_fee;     ///< Cost of the ride.
	int ownership_cost;   ///< Monthly cost of having the ride.
	int opened_cost;      ///< Additional monthly cost of having an opened ride.
	int number_of_batches; ///< Number of guest batches that can enter the ride.
	int guests_per_batch;  ///< Number of guests in each guest batch.
	int idle_duration;     ///< Duration of the idle phase in milliseconds.
	int working_duration;  ///< Duration of the working phase in milliseconds.
	int16 working_cycles_min;            ///< Minimum number of working cycles.
	int16 working_cycles_max;            ///< Maximum number of working cycles.
	int16 working_cycles_default;        ///< Default number of working cycles.
	int16 reliability_max;               ///< Maximum reliability.
	int16 reliability_decrease_daily;    ///< Reliability decrease per day.
	int16 reliability_decrease_monthly;  ///< Maximum reliability decrease per month.
	int32 intensity_base;                ///< The ride's base intensity rating.
	int32 nausea_base;                   ///< The ride's base nausea rating.
	int32 excitement_base;               ///< The ride's base excitement rating.
	int32 excitement_increase_cycle;     ///< The ride's excitement rating increase per working cycle.
	int32 excitement_increase_scenery;   ///< The ride's excitement rating increase per nearby scenery item.

	std::shared_ptr<FSETBlock> idle_animation;
	std::shared_ptr<TIMABlock> starting_animation;
	std::shared_ptr<TIMABlock> working_animation;
	std::shared_ptr<TIMABlock> stopping_animation;
	std::shared_ptr<SpriteBlock> previews[4]; ///< Previews for ne,se,sw,nw.
	std::shared_ptr<StringBundle> ride_text;   ///< Texts of the ride.
};

/** Class for describing a SCNY game block. */
class SCNYBlock : public GameBlock {
public:
	SCNYBlock();
	virtual ~SCNYBlock() = default;

	int Write(FileWriter *fw) override;

	std::string internal_name;  ///< The ride's internal name.
	int8 category;                    ///< Item type category.
	int8 width_x;                     ///< The number of voxels the item occupies in x direction.
	int8 width_y;                     ///< The number of voxels the item occupies in y direction.
	std::unique_ptr<int8[]> heights;  ///< Heights of the item in voxels.
	bool symmetric;                   ///< Whether the item cannot be rotated.
	int32 buy_cost;                   ///< Cost to place this item.
	int32 return_cost;                ///< Cost when removing this item.
	int32 return_cost_dry;            ///< Cost when removing this item when dry.
	uint32 watering_interval;         ///< How often this item needs to be watered.
	uint32 min_watering_interval;     ///< How frequently this item may at most be watered.

	std::shared_ptr<TIMABlock> main_animation;  ///< Main image.
	std::shared_ptr<TIMABlock> dry_animation;   ///< Dry image.
	std::shared_ptr<SpriteBlock> previews[4];   ///< Previews for ne,se,sw,nw.
	std::shared_ptr<StringBundle> texts;        ///< Texts of the scenery item.
};

/** GBOR game block. */
class GBORBlock : public GameBlock {
public:
	GBORBlock();
	virtual ~GBORBlock() = default;

	int Write(FileWriter *fw) override;

	int widget_type;   ///< Widget type.
	int border_top;    ///< Border width of the top edge.
	int border_left;   ///< Border width of the left edge.
	int border_right;  ///< Border width of the right edge.
	int border_bottom; ///< Border width of the bottom edge.
	int min_width;     ///< Minimal width of the border.
	int min_height;    ///< Minimal height of the border.
	int h_stepsize;    ///< Horizontal stepsize of the border.
	int v_stepsize;    ///< Vertical stepsize of the border.

	std::shared_ptr<SpriteBlock> tl; ///< Top-left sprite.
	std::shared_ptr<SpriteBlock> tm; ///< Top-middle sprite.
	std::shared_ptr<SpriteBlock> tr; ///< Top-right sprite.
	std::shared_ptr<SpriteBlock> ml; ///< Left sprite.
	std::shared_ptr<SpriteBlock> mm; ///< Middle sprite.
	std::shared_ptr<SpriteBlock> mr; ///< Right sprite.
	std::shared_ptr<SpriteBlock> bl; ///< Bottom-left sprite.
	std::shared_ptr<SpriteBlock> bm; ///< Bottom-middle sprite.
	std::shared_ptr<SpriteBlock> br; ///< Bottom-right sprite.
};

/** GCHK game block. */
class GCHKBlock : public GameBlock {
public:
	GCHKBlock();
	virtual ~GCHKBlock() = default;

	int Write(FileWriter *fw) override;

	int widget_type; ///< Widget type.

	std::shared_ptr<SpriteBlock> empty;          ///< Empty sprite.
	std::shared_ptr<SpriteBlock> filled;         ///< Filled.
	std::shared_ptr<SpriteBlock> empty_pressed;  ///< Empty pressed.
	std::shared_ptr<SpriteBlock> filled_pressed; ///< Filled pressed.
	std::shared_ptr<SpriteBlock> shaded_empty;   ///< Shaded empty button.
	std::shared_ptr<SpriteBlock> shaded_filled;  ///< Shaded filled button.
};

/** GSLI game block. */
class GSLIBlock : public GameBlock {
public:
	GSLIBlock();
	virtual ~GSLIBlock() = default;

	int Write(FileWriter *fw) override;

	int min_length;  ///< Minimal length of the bar.
	int step_size;   ///< Stepsize of the bar.
	int width;       ///< Width of the slider button.
	int widget_type; ///< Widget type.

	std::shared_ptr<SpriteBlock> left;   ///< Left sprite.
	std::shared_ptr<SpriteBlock> middle; ///< Middle sprite.
	std::shared_ptr<SpriteBlock> right;  ///< Right sprite.
	std::shared_ptr<SpriteBlock> slider; ///< Slider button.
};

/** GSCL game block. */
class GSCLBlock : public GameBlock {
public:
	GSCLBlock();
	virtual ~GSCLBlock() = default;

	int Write(FileWriter *fw) override;

	int min_length;     ///< Minimal length scrollbar.
	int step_back;      ///< Stepsize of background.
	int min_bar_length; ///< Minimal length bar.
	int bar_step;       ///< Stepsize of bar.
	int widget_type;    ///< Widget type.

	std::shared_ptr<SpriteBlock> left_button;        ///< Left/up button.
	std::shared_ptr<SpriteBlock> right_button;       ///< Right/down button.
	std::shared_ptr<SpriteBlock> left_pressed;       ///< Left/up pressed button.
	std::shared_ptr<SpriteBlock> right_pressed;      ///< Right/down pressed button.
	std::shared_ptr<SpriteBlock> left_bottom;        ///< Left/top bar bottom (the background).
	std::shared_ptr<SpriteBlock> middle_bottom;      ///< Middle bar bottom (the background).
	std::shared_ptr<SpriteBlock> right_bottom;       ///< Right/down bar bottom (the background).
	std::shared_ptr<SpriteBlock> left_top;           ///< Left/top bar top.
	std::shared_ptr<SpriteBlock> middle_top;         ///< Middle bar top.
	std::shared_ptr<SpriteBlock> right_top;          ///< Right/down bar top.
	std::shared_ptr<SpriteBlock> left_top_pressed;   ///< Left/top pressed bar top.
	std::shared_ptr<SpriteBlock> middle_top_pressed; ///< Middle pressed bar top.
	std::shared_ptr<SpriteBlock> right_top_pressed;  ///< Right/down pressed bar top.
};

/** BDIR game block. */
class BDIRBlock : public GameBlock {
public:
	BDIRBlock();
	virtual ~BDIRBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.

	std::shared_ptr<SpriteBlock> sprite_ne; ///< ne arrow.
	std::shared_ptr<SpriteBlock> sprite_se; ///< se arrow.
	std::shared_ptr<SpriteBlock> sprite_sw; ///< sw arrow.
	std::shared_ptr<SpriteBlock> sprite_nw; ///< nw arrow.
};

/** GSLP Game block. */
class GSLPBlock : public GameBlock {
public:
	GSLPBlock();
	virtual ~GSLPBlock() = default;

	int Write(FileWriter *fw) override;

	std::shared_ptr<SpriteBlock> vert_down;       ///< Slope going vertically down.
	std::shared_ptr<SpriteBlock> steep_down;      ///< Slope going steeply down.
	std::shared_ptr<SpriteBlock> gentle_down;     ///< Slope going gently down.
	std::shared_ptr<SpriteBlock> level;           ///< Level slope.
	std::shared_ptr<SpriteBlock> gentle_up;       ///< Slope going gently up.
	std::shared_ptr<SpriteBlock> steep_up;        ///< Slope going steeply up.
	std::shared_ptr<SpriteBlock> vert_up;         ///< Slope going vertically up.
	std::shared_ptr<SpriteBlock> wide_left;       ///< Wide bend to the left.
	std::shared_ptr<SpriteBlock> normal_left;     ///< Normal bend to the left.
	std::shared_ptr<SpriteBlock> tight_left;      ///< Tight bend to the left.
	std::shared_ptr<SpriteBlock> no_bend;         ///< No bends.
	std::shared_ptr<SpriteBlock> tight_right;     ///< Tight bend to the right.
	std::shared_ptr<SpriteBlock> normal_right;    ///< Normal bend to the right.
	std::shared_ptr<SpriteBlock> wide_right;      ///< Wide bend to the right.
	std::shared_ptr<SpriteBlock> no_banking;      ///< No banking.
	std::shared_ptr<SpriteBlock> bank_left;       ///< Bank to the left.
	std::shared_ptr<SpriteBlock> bank_right;      ///< Bank to the right.
	std::shared_ptr<SpriteBlock> triangle_right;  ///< Arrow triangle to the right.
	std::shared_ptr<SpriteBlock> triangle_left;   ///< Arrow triangle to the left.
	std::shared_ptr<SpriteBlock> triangle_up;     ///< Arrow triangle upwards.
	std::shared_ptr<SpriteBlock> triangle_bottom; ///< Arrow triangle downwards.
	std::shared_ptr<SpriteBlock> has_platform;    ///< Select button for tracks with platforms.
	std::shared_ptr<SpriteBlock> no_platform;     ///< Select button for tracks without platforms.
	std::shared_ptr<SpriteBlock> has_power;       ///< Select button for powered tracks.
	std::shared_ptr<SpriteBlock> no_power;        ///< Select button for unpowered tracks.
	std::shared_ptr<SpriteBlock> disabled;        ///< Pattern to overlay over disabled buttons.
	std::shared_ptr<SpriteBlock> compass[4];      ///< Compass sprites denoting viewing directions.
	std::shared_ptr<SpriteBlock> bulldozer;       ///< Bulldoze/delete.
	std::shared_ptr<SpriteBlock> message_goto;       ///< Inbox: Go To Location button.
	std::shared_ptr<SpriteBlock> message_park;       ///< Inbox: Park Management button.
	std::shared_ptr<SpriteBlock> message_guest;      ///< Inbox: Guest Window button.
	std::shared_ptr<SpriteBlock> message_ride;       ///< Inbox: Ride Instance Window button.
	std::shared_ptr<SpriteBlock> message_ride_type;  ///< Inbox: Ride Select GUI button.
	std::shared_ptr<SpriteBlock> loadsave_err;       ///< Loadsave icon for files with errors.
	std::shared_ptr<SpriteBlock> loadsave_warn;      ///< Loadsave icon for files with warnings.
	std::shared_ptr<SpriteBlock> loadsave_ok;        ///< Loadsave icon for files without warnings or errors.
	std::shared_ptr<SpriteBlock> toolbar_main;      ///< Toolbar: Main menu GUI button.
	std::shared_ptr<SpriteBlock> toolbar_speed;     ///< Toolbar: Speed menu GUI button.
	std::shared_ptr<SpriteBlock> toolbar_path;      ///< Toolbar: Path build GUI button.
	std::shared_ptr<SpriteBlock> toolbar_ride;      ///< Toolbar: Ride select GUI button.
	std::shared_ptr<SpriteBlock> toolbar_fence;     ///< Toolbar: Fence build GUI button.
	std::shared_ptr<SpriteBlock> toolbar_scenery;   ///< Toolbar: Scenery GUI button.
	std::shared_ptr<SpriteBlock> toolbar_terrain;   ///< Toolbar: Terraform GUI button.
	std::shared_ptr<SpriteBlock> toolbar_staff;     ///< Toolbar: Staff GUI button.
	std::shared_ptr<SpriteBlock> toolbar_inbox;     ///< Toolbar: Inbox GUI button.
	std::shared_ptr<SpriteBlock> toolbar_finances;  ///< Toolbar: Finances GUI button.
	std::shared_ptr<SpriteBlock> toolbar_objects;   ///< Toolbar: Path objects GUI button.
	std::shared_ptr<SpriteBlock> toolbar_view;      ///< Toolbar: View menu GUI button.
	std::shared_ptr<SpriteBlock> toolbar_park;      ///< Toolbar: Park management GUI button.
	std::shared_ptr<SpriteBlock> weather[5];      ///< Weather sprites (sunny -> thunder).
	std::shared_ptr<SpriteBlock> rog_lights[4];   ///< Red/orange/green lights (red, orange, green, none).
	std::shared_ptr<SpriteBlock> rg_lights[3];    ///< Red/green lights (red, green, none).
	std::shared_ptr<SpriteBlock> pos_2d;          ///< Flat rotation positive direction (counter clock wise)
	std::shared_ptr<SpriteBlock> neg_2d;          ///< Flat rotation negative direction (clock wise)
	std::shared_ptr<SpriteBlock> pos_3d;          ///< Diametric rotation positive direction (counter clock wise)
	std::shared_ptr<SpriteBlock> neg_3d;          ///< Diametric rotation negative direction (clock wise)
	std::shared_ptr<SpriteBlock> close_button;    ///< Close Button.
	std::shared_ptr<SpriteBlock> maxi_button;     ///< Maximise button.
	std::shared_ptr<SpriteBlock> mini_button;     ///< Minimise button.
	std::shared_ptr<SpriteBlock> terraform_dot;   ///< Terraform dot.
	std::shared_ptr<StringBundle> gui_text;       ///< Text of the GUIs (reference to a TEXT block).
};

/** MENU Game block. */
class MENUBlock : public GameBlock {
public:
	MENUBlock();
	virtual ~MENUBlock() = default;

	int Write(FileWriter *fw) override;

	std::shared_ptr<SpriteBlock> logo;             ///< FreeRCT logo.
	std::shared_ptr<SpriteBlock> splash;           ///< FreeRCT splashscreen.
	std::shared_ptr<SpriteBlock> new_game;         ///< New Game button.
	std::shared_ptr<SpriteBlock> load_game;        ///< Load Game button.
	std::shared_ptr<SpriteBlock> settings;         ///< Settings button.
	std::shared_ptr<SpriteBlock> quit;             ///< Quit button.
	uint32                       splash_duration;  ///< Duration of the splash screen animation.
};

/**
 * Class for storing a cubic bezier spline, as (part of) a path in a track piece.
 * @note While the parameters of a cubic spline can be floating point numbers, rcdgen does not allow entering such values.
 */
class CubicSpline : public BlockNode {
public:
	CubicSpline();
	virtual ~CubicSpline() = default;

	int a;     ///< Fourth cubic spline parameter.
	int b;     ///< Fourth cubic spline parameter.
	int c;     ///< Fourth cubic spline parameter.
	int d;     ///< Fourth cubic spline parameter.
	int steps; ///< Length of the path segment (exclusive upper bound).
	int first; ///< Physical distance in the track piece of the first position, in \c 1/256 pixels.
	int last;  ///< Physical distance in the track piece of the last position, in \c 1/256 pixels.
};

/** Base class of a curved path for a coaster car. */
class Curve : public BlockNode {
public:
	Curve();
	virtual double GetValue(int index, int step) = 0;
};

/** Curve described by a segmented cubic bezier spline. */
class CubicSplines : public Curve {
public:
	CubicSplines();
	virtual ~CubicSplines() = default;

	double GetValue(int index, int step) override;

	std::vector<std::shared_ptr<CubicSpline>> curve; ///< Bezier curve describing the table values.
};

/** Curve that is always the same value. */
class FixedTable : public Curve {
public:
	FixedTable();
	virtual ~FixedTable() = default;

	double GetValue(int index, int step) override;

	int value; ///< Fixed value of the curve data.
};

/** Class for storing the data of a single voxel in a track piece. */
class TrackVoxel : public BlockNode {
public:
	TrackVoxel();
	virtual ~TrackVoxel() = default;

	void Write(FileWriter *fw, FileBlock *fb, int rot);

	int dx;    ///< Relative X position of the voxel.
	int dy;    ///< Relative Y position of the voxel.
	int dz;    ///< Relative Z position of the voxel.
	int flags; ///< Flags of the voxel (space corners, platform direction).

	std::array<std::shared_ptr<SpriteBlock>, 4> back;  ///< Back coaster sprites.
	std::array<std::shared_ptr<SpriteBlock>, 4> front; ///< Front coaster sprites.
};

/** Class for storing a 'connection' between track pieces. */
class Connection : public BlockNode {
public:
	Connection();
	Connection(const Connection &c);
	Connection &operator=(const Connection &c);
	Connection(const std::string &name, int direction);
	virtual ~Connection() = default;

	uint8 Encode(const std::map<std::string, int> &connections, int rot);

	std::string name; ///< Name of the connection.
	int direction;    ///< Direction of the connection.
};

/** A 'track_piece' block. */
class TrackPieceNode : public BlockNode {
public:
	virtual ~TrackPieceNode() = default;

	void UpdateConnectionMap(std::map<std::string, int> *connections);
	void Write(const std::map<std::string, int> &connections, FileWriter *fw, FileBlock *fb);

	void ComputeTrackLength(const Position &pos);

	int track_flags; ///< Flags of the track piece.
	int banking;     ///< Banking (0=no, 1=left, 2=right).
	int slope;       ///< Slope of the track piece (-3 vertically down to 3 vertically up).
	int bend;        ///< Bend of the track piece (-3 wide-left to 3 wide right).
	int cost;        ///< Cost of the track piece.
	int exit_dx;     ///< Relative X position of the exit voxel.
	int exit_dy;     ///< Relative Y position of the exit voxel.
	int exit_dz;     ///< Relative Z position of the exit voxel.
	int speed;       ///< If non-0, minimal speed of cars (with direction).

	std::shared_ptr<Connection> entry; ///< Entry connection code.
	std::shared_ptr<Connection> exit;  ///< Exit connection code.

	std::vector<std::shared_ptr<TrackVoxel>> track_voxels; ///< Voxels in the track piece.

	std::shared_ptr<Curve> car_xpos;  ///< Path for cars over the track piece in X direction.
	std::shared_ptr<Curve> car_ypos;  ///< Path for cars over the track piece in Y direction.
	std::shared_ptr<Curve> car_zpos;  ///< Path for cars over the track piece in Z direction.
	std::shared_ptr<Curve> car_pitch; ///< Pitch of cars over the tracks, may be \c NULL.
	std::shared_ptr<Curve> car_roll;  ///< Roll of cars over the tracks.
	std::shared_ptr<Curve> car_yaw;   ///< Yaw of cars over the tracks, may be \c NULL.
	int total_distance;               ///< Total distance of the piece.

private:
	void CheckBezierCount(int count, const Position &pos);
	void CheckSplineStepCount(int index, int steps, const Position &pos);
	void SetStartEnd(int index, bool set_start, int value);
};

/** 'RCST' game block. */
class RCSTBlock : public GameBlock {
public:
	RCSTBlock();
	virtual ~RCSTBlock() = default;

	int Write(FileWriter *fw) override;

	std::string internal_name;  ///< The ride's internal name.
	int coaster_type;  ///< Type of roller coaster.
	int platform_type; ///< Type of platform.
	int number_trains; ///< Maximum number of trains at the roller coaster.
	int number_cars;   ///< Maximum number of cars in a train.
	int16 reliability_max;               ///< Maximum reliability.
	int16 reliability_decrease_daily;    ///< Reliability decrease per day.
	int16 reliability_decrease_monthly;  ///< Maximum reliability decrease per month.

	std::shared_ptr<StringBundle> text; ///< Text of the coaster (reference to a TEXT block).

	std::vector<std::shared_ptr<TrackPieceNode>> track_blocks; ///< Track pieces of the roller coaster.
};

/** 'CARS' game block. */
class CARSBlock : public GameBlock {
public:
	CARSBlock();
	virtual ~CARSBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width;     ///< Zoom-width of a tile of the surface.
	int z_height;       ///< Change in Z height (in pixels) when going up or down a tile level.
	int length;         ///< Length of the car.
	int inter_length;   ///< Length between two cars.
	int num_passengers; ///< Number of passengers in a car.
	int num_entrances;  ///< Number of entrances to/from a car.
	std::array<std::shared_ptr<SpriteBlock>, 16*16*16> sprites;      ///< Car sprites.
	std::unique_ptr<std::shared_ptr<SpriteBlock>[]> guest_overlays;  ///< Guest overlay sprites.
	uint64 nr_guest_overlays;                                        ///< Number of guest overlay sprites.
	Recolouring recol[3];                                            ///< Recolour definitions of the car.
};

/** Node block containing a bitmask. */
class BitMask : public BlockNode {
public:
	virtual ~BitMask() = default;

	BitMaskData data; ///< Data of the bit mask.
};

/** 'CSPL' game block. */
class CSPLBlock : public GameBlock {
public:
	CSPLBlock();
	virtual ~CSPLBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.
	uint8 type;     ///< Type of platform.
	std::shared_ptr<SpriteBlock> ne_sw_back;  ///< Background platform sprite of the NE to SW direction.
	std::shared_ptr<SpriteBlock> ne_sw_front; ///< Foreground platform sprite of the NE to SW direction.
	std::shared_ptr<SpriteBlock> se_nw_back;  ///< Background platform sprite of the SE to NW direction.
	std::shared_ptr<SpriteBlock> se_nw_front; ///< Foreground platform sprite of the SE to NW direction.
	std::shared_ptr<SpriteBlock> sw_ne_back;  ///< Background platform sprite of the SW to NE direction.
	std::shared_ptr<SpriteBlock> sw_ne_front; ///< Foreground platform sprite of the SW to NE direction.
	std::shared_ptr<SpriteBlock> nw_se_back;  ///< Background platform sprite of the NW to SE direction.
	std::shared_ptr<SpriteBlock> nw_se_front; ///< Foreground platform sprite of the NW to SE direction.
};

/** 'FENC' game block. */
class FENCBlock : public GameBlock {
public:
	FENCBlock();
	virtual ~FENCBlock() = default;

	int Write(FileWriter *fw) override;

	int tile_width; ///< Zoom-width of a tile of the surface.
	uint16 type;    ///< Type of fence.
	std::shared_ptr<SpriteBlock> ne_hor; ///< Horizontal fence graphics of the north-east edge.
	std::shared_ptr<SpriteBlock> ne_n;   ///< Fence graphics of the north-east edge, north side raised.
	std::shared_ptr<SpriteBlock> ne_e;   ///< Fence graphics of the north-east edge, east side raised.
	std::shared_ptr<SpriteBlock> se_hor; ///< Horizontal fence graphics of the south-east edge.
	std::shared_ptr<SpriteBlock> se_e;   ///< Fence graphics of the south-east edge, east side raised.
	std::shared_ptr<SpriteBlock> se_s;   ///< Fence graphics of the south-east edge, south side raised.
	std::shared_ptr<SpriteBlock> sw_hor; ///< Horizontal fence graphics of the south-west edge.
	std::shared_ptr<SpriteBlock> sw_s;   ///< Fence graphics of the south-west edge, south side raised.
	std::shared_ptr<SpriteBlock> sw_w;   ///< Fence graphics of the south-west edge, west side raised.
	std::shared_ptr<SpriteBlock> nw_hor; ///< Horizontal fence graphics of the north-west edge.
	std::shared_ptr<SpriteBlock> nw_w;   ///< Fence graphics of the north-west edge, west side raised.
	std::shared_ptr<SpriteBlock> nw_n;   ///< Fence graphics of the north-west edge, north side raised.
};

/** 'INFO' eta block. */
class INFOBlock : public MetaBlock {
public:
	INFOBlock();
	virtual ~INFOBlock() = default;

	int Write(FileWriter *fw) override;

	std::string name;        ///< Name of the work.
	std::string uri;         ///< Technical unique identification of compatible versions of the worl.
	std::string build;       ///< Build number or date.
	std::string website;     ///< Optional website url.
	std::string description; ///< Optional description of the work.
};


FileNodeList *CheckTree(std::shared_ptr<NamedValueList> values);

void GenerateStringsHeaderFile(const char *prefix, const char *base, const char *header);
void GenerateStringsCodeFile(const char *prefix, const char *code);

#endif
