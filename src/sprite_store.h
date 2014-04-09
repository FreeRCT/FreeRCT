/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h Sprite storage data structure. */

#ifndef SPRITE_STORE_H
#define SPRITE_STORE_H

#include "orientation.h"
#include "path.h"
#include "tile.h"
#include "person_type.h"
#include "language.h"
#include "track_piece.h"
#include <map>

extern const uint8 _slope_rotation[NUM_SLOPE_SPRITES][4];

class RcdFileReader;
class ImageData;

/**
 * Block of data from a RCD file.
 * @ingroup sprites_group
 */
class RcdBlock {
public:
	RcdBlock();
	virtual ~RcdBlock();

	RcdBlock *next; ///< Pointer to next block.
};

typedef std::map<uint32, ImageData *> ImageMap; ///< Map of loaded image data blocks.

/** Texts of objects. */
class TextData : public RcdBlock {
public:
	TextData();
	~TextData();

	bool Load(RcdFileReader *rcd_file);

	uint string_count;   ///< Number of strings in #strings.
	TextString *strings; ///< Strings of the text.
	uint8 *text_data;    ///< Text data (UTF-8) itself.
};

typedef std::map<uint32, TextData *> TextMap; ///< Map of loaded text blocks.

/**
 * A surface in all orientations.
 * @ingroup sprites_group
 */
class SurfaceData {
public:
	SurfaceData();

	ImageData *surface[NUM_SLOPE_SPRITES]; ///< Sprites displaying the slopes.
};

/**
 * Highlight the currently selected ground tile with a selection cursor.
 * @ingroup sprites_group
 */
class TileSelection : public RcdBlock {
public:
	TileSelection();
	~TileSelection();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *surface[NUM_SLOPE_SPRITES]; ///< Sprites with a selection cursor.
};

/**
 * Highlight the currently selected corner of a ground tile with a corner selection sprite.
 * @ingroup sprites_group
 */
class TileCorners : public RcdBlock {
public:
	TileCorners();
	~TileCorners();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *sprites[VOR_NUM_ORIENT][NUM_SLOPE_SPRITES]; ///< Corner selection sprites.
};

/**
 * %Path sprites.
 * @ingroup sprites_group
 */
class Path : public RcdBlock {
public:
	Path();
	~Path();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 type;   ///< Type of the path. @see PathTypes
	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *sprites[PATH_COUNT]; ///< Path sprites, may contain \c nullptr sprites.
};

/**
 * %Foundation sprites.
 * @ingroup sprites_group
 */
class Foundation : public RcdBlock {
public:
	Foundation();
	~Foundation();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 type;   ///< Type of the foundation. @see FoundationType
	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *sprites[6]; ///< Foundation sprites.
};

/**
 * %Platform sprites.
 * @ingroup sprites_group
 * @note There is only one type of platform, so no need to define its type.
 */
class Platform : public RcdBlock {
public:
	Platform();
	~Platform();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *flat[2]; ///< Flat platforms.
	ImageData *ramp[4]; ///< Raised platform at one edge. See #TileEdge for the order.
	ImageData *right_ramp[4]; ///< Raised platform at one edge with only the right leg. See #TileEdge for the order.
	ImageData *left_ramp[4];  ///< Raised platform at one edge with only the left leg. See #TileEdge for the order.
};

/**
 * %Support sprites.
 * @ingroup sprites_group
 */
class Support : public RcdBlock {
public:
	Support();
	~Support();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 type;   ///< Type of the foundation. @see FoundationType
	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile. @todo Remove height from RCD file.
	ImageData *sprites[SSP_COUNT]; ///< Foundation sprites.
};

/**
 * Displayed object.
 * (For now, just the arrows pointing in the build direction, hopefully also usable for other 1x1 tile objects.)
 * @ingroup sprites_group
 */
class DisplayedObject : public RcdBlock {
public:
	DisplayedObject();
	~DisplayedObject();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width; ///< Width of the tile.
	ImageData *sprites[4]; ///< Object displayed towards NE, SE, SW, NW edges.
};

/** One frame in an animation. */
struct AnimationFrame {
	uint16 duration; ///< Length of this frame in milliseconds.
	int16 dx;        ///< Person movement in X direction after displaying this frame.
	int16 dy;        ///< Person movement in Y direction after displaying this frame.
};

/** Available animations. */
enum AnimationType {
	ANIM_WALK_NE = 1, ///< Walk in north-east direction.
	ANIM_WALK_SE = 2, ///< Walk in south-east direction.
	ANIM_WALK_SW = 3, ///< Walk in south-west direction.
	ANIM_WALK_NW = 4, ///< Walk in north-west direction.

	ANIM_BEGIN = ANIM_WALK_NE, ///< First animation.
	ANIM_LAST  = ANIM_WALK_NW, ///< Last animation.
	ANIM_INVALID = 0xFF,       ///< Invalid animation.
};
DECLARE_POSTFIX_INCREMENT(AnimationType)

/** Data structure holding a single animation. */
class Animation : public RcdBlock {
public:
	Animation();
	~Animation();

	bool Load(RcdFileReader *rcd_file);

	uint16 frame_count;      ///< Number of frames.
	uint8 person_type;       ///< Type of persons supported by this animation.
	AnimationType anim_type; ///< Animation ID.

	AnimationFrame *frames;  ///< Frames of the animation.
};

typedef std::multimap<AnimationType, Animation *> AnimationsMap; ///< Multi-map of available animations.

/** Data structure holding sprite of an animation. */
class AnimationSprites: public RcdBlock {
public:
	AnimationSprites();
	~AnimationSprites();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width;            ///< Width of the tile.
	uint8 person_type;       ///< Type of persons supported by this animation.
	AnimationType anim_type; ///< Animation ID.
	uint16 frame_count;      ///< Number of frames.

	ImageData **sprites;     ///< Sprites of the animation.
};

typedef std::multimap<AnimationType, AnimationSprites *> AnimationSpritesMap; ///< Multi-map of available animation sprites.

/**
 * Sprites of a border.
 * @ingroup gui_sprites_group
 */
enum WidgetBorderSprites {
	WBS_TOP_LEFT,      ///< Top left corner.
	WBS_TOP_MIDDLE,    ///< Top centre edge.
	WBS_TOP_RIGHT,     ///< Top right corner.
	WBS_MIDDLE_LEFT,   ///< Left edge.
	WBS_MIDDLE_MIDDLE, ///< Centre sprite.
	WBS_MIDDLE_RIGHT,  ///< Right edge.
	WBS_BOTTOM_LEFT,   ///< Bottom left corner.
	WBS_BOTTOM_MIDDLE, ///< Bottom centre edge.
	WBS_BOTTOM_RIGHT,  ///< Bottom right corner.

	WBS_COUNT,         ///< Number of sprites of a border.
};

/**
 * Sprite data of a border.
 * @ingroup gui_sprites_group
 */
struct BorderSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint8 border_top;    ///< Extra space at the top.
	uint8 border_left;   ///< Extra space at the left.
	uint8 border_right;  ///< Extra space at the right,
	uint8 border_bottom; ///< Extra space at the bottom.

	uint16 min_width;    ///< Minimal width of content.
	uint16 min_height;   ///< Minimal height of content.
	uint8 hor_stepsize;  ///< Width increment size.
	uint8 vert_stepsize; ///< Height increment size.

	ImageData *normal[WBS_COUNT];  ///< Sprites to draw for a normal border.
	ImageData *pressed[WBS_COUNT]; ///< Sprites to draw for a pressed border.
};

/**
 * Sprites of checkable sprites, like checkbox or radio button.
 * @ingroup gui_sprites_group
 */
enum WidgetCheckableSprites {
	WCS_EMPTY,           ///< Check mark is off.
	WCS_CHECKED,         ///< Check mark is on.
	WCS_EMPTY_PRESSED,   ///< Empty button is pressed.
	WCS_CHECKED_PRESSED, ///< Button with check mark is pressed.
	WCS_SHADED_EMPTY,    ///< Empty shaded button.
	WCS_SHADED_CHECKED,  ///< Filled shaded button.

	WCS_COUNT,           ///< Number of sprites for a checkable widget.
};

/**
 * Sprite data of a checkable sprite.
 * @ingroup gui_sprites_group
 */
struct CheckableWidgetSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint16 width;  ///< Width of the sprite.
	uint16 height; ///< Height of the sprite.

	ImageData *sprites[WCS_COUNT]; ///< Sprites to draw.
};

/**
 * Slider sprites.
 * @ingroup gui_sprites_group
 */
enum WidgetSliderSprites {
	WSS_LEFT_BED,   ///< Left/up bar sprite.
	WSS_MIDDLE_BED, ///< Middle bar sprite.
	WSS_RIGHT_BED,  ///< Right/down sprite.
	WSS_BUTTON,     ///< Button sprite.

	WSS_COUNT,      ///< Number of sprites of a slider bar.
};

/**
 * Sprite data of a slider bar.
 * @ingroup gui_sprites_group
 */
struct SliderSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint16 min_bar_length; ///< Minimal length of a slider bar.
	uint8 stepsize;        ///< Length increment size.
	uint8 height;          ///< Height/width of the bar.

	ImageData *normal[WSS_COUNT]; ///< Sprites for normal slider.
	ImageData *shaded[WSS_COUNT]; ///< Sprites for shaded slider.
};

/**
 * Scrollbar sprites.
 * @ingroup gui_sprites_group
 */
enum WidgetScrollbarSprites {
	WLS_LEFT_BUTTON,           ///< Left/up unpressed button.
	WLS_RIGHT_BUTTON,          ///< Right/down unpressed button.
	WLS_LEFT_PRESSED_BUTTON,   ///< Left/up pressed button.
	WLS_RIGHT_PRESSED_BUTTON,  ///< Right/down pressed button.
	WLS_LEFT_BED,              ///< Left/up part of the slider underground.
	WLS_MIDDLE_BED,            ///< Middle part of the slider underground.
	WLS_RIGHT_BED,             ///< Right/down part of the slider underground.
	WLS_LEFT_SLIDER,           ///< Left/up slider part.
	WLS_MIDDLE_SLIDER,         ///< Middle slider part.
	WLS_RIGHT_SLIDER,          ///< Right/down slider part.
	WLS_LEFT_PRESSED_SLIDER,   ///< Left/up slider part, pressed.
	WLS_MIDDLE_PRESSED_SLIDER, ///< Middle slider part, pressed.
	WLS_RIGHT_PRESSED_SLIDER,  ///< Right/down slider part, pressed.

	WLS_COUNT,                 ///< Number of scrollbar sprites.
};

/**
 * Sprite data of a scrollbar.
 * @ingroup gui_sprites_group
 */
struct ScrollbarSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint16 min_length_all;    ///< Total minimal length of the bar.
	uint16 min_length_slider; ///< Minimal length of the slider.
	uint8 stepsize_bar;       ///< Length increment of the slider underground.
	uint8 stepsize_slider;    ///< Length increment of the slider.
	uint16 height;            ///< Height/width of the entire bar.

	ImageData *normal[WLS_COUNT]; ///< Sprites for a normal scrollbar.
	ImageData *shaded[WLS_COUNT]; ///< Sprites for a shaded scrollbar.
};

/**
 * Offsets of weather sprites to #SPR_GUI_WEATHER_START, for different types of weather.
 * @todo Add weather.
 */
enum WeatherSprites {
	WES_SUNNY,        ///< Sprite offset for sunny weather.
	WES_LIGHT_CLOUDS, ///< Sprite offset for light clouds.
	WES_THICK_CLOUDS, ///< Sprite offset for thick clouds.
	WES_RAINING,      ///< Sprite offset of rain.

	WES_COUNT,        ///< Number of weather sprites.
};

/**
 * Sprites of the gui.
 * @todo Add widget for rounded_button
 * @todo Add widget for inset_frame
 * @todo Add widget for checkbox
 * @todo Add widget for radio_button
 * @todo Add widget for hor_slider
 * @todo Add widget for vert_slider
 * @todo Add sprites for resize box (currently a button)
 * @todo Add sprites for close box (currently a button)
 * @ingroup gui_sprites_group
 */
struct GuiSprites {
	GuiSprites();

	void Clear();
	bool LoadGBOR(RcdFileReader *rcd_file, const ImageMap &sprites);
	bool LoadGCHK(RcdFileReader *rcd_file, const ImageMap &sprites);
	bool LoadGSLI(RcdFileReader *rcd_file, const ImageMap &sprites);
	bool LoadGSCL(RcdFileReader *rcd_file, const ImageMap &sprites);
	bool LoadGSLP(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts);

	bool HasSufficientGraphics() const;

	BorderSpriteData titlebar;              ///< Title bar sprite data.
	BorderSpriteData button;                ///< Normal button sprite data.
	BorderSpriteData rounded_button;        ///< Rounded button sprite data.
	BorderSpriteData frame;                 ///< Frame sprite data.
	BorderSpriteData panel;                 ///< Panel sprite data.
	BorderSpriteData inset_frame;           ///< Inset frame sprite data.

	CheckableWidgetSpriteData checkbox;     ///< Check box sprite data.
	CheckableWidgetSpriteData radio_button; ///< Radio button sprite data.

	SliderSpriteData hor_slider;            ///< Horizontal slider bar sprite data.
	SliderSpriteData vert_slider;           ///< Vertical slider bar sprite data.

	ScrollbarSpriteData hor_scroll;         ///< Horizontal scroll bar sprite data.
	ScrollbarSpriteData vert_scroll;        ///< Vertical scroll bar sprite data.

	ImageData *slope_select[TSL_COUNT_VERTICAL]; ///< Slope selection sprites.
	ImageData *bend_select[TBN_COUNT];      ///< Bend selection sprites.
	ImageData *bank_select[TPB_COUNT];      ///< Banking selection sprites.
	ImageData *platform_select[2];          ///< Platform selection sprites.
	ImageData *power_select[2];             ///< Power selection sprites.
	ImageData *triangle_left;               ///< Triangular arrow to the left.
	ImageData *triangle_right;              ///< Triangular arrow to the right.
	ImageData *triangle_up;                 ///< Triangular upward arrow.
	ImageData *triangle_down;               ///< Triangular downward arrow.
	ImageData *disabled;                    ///< Overlay sprite over disabled buttons.
	ImageData *rot_2d_pos;                  ///< 2D rotation positive direction.
	ImageData *rot_2d_neg;                  ///< 2D rotation negative direction.
	ImageData *rot_3d_pos;                  ///< 3D rotation positive direction.
	ImageData *rot_3d_neg;                  ///< 3D rotation negative direction.
	ImageData *close_sprite;                ///< Close window button.
	ImageData *dot_sprite;                  ///< 'Dot' in the terraform window, represents an area of (0, 0) tiles.
	ImageData *bulldozer;                   ///< Bulldozer sprite.
	ImageData *compass[TC_END];             ///< Viewing direction sprites.
	ImageData *weather[WES_COUNT];          ///< Weather sprites.

	TextData *text;                         ///< Texts of the GUI.
};


/**
 * Storage of all sprites for a single tile size.
 * @note The data does not belong to this class, it is managed by #SpriteManager instead.
 * @ingroup sprites_group
 */
class SpriteStorage {
public:
	SpriteStorage(uint16 size);
	~SpriteStorage();

	void AddTileSelection(TileSelection *tsel);
	void AddTileCorners(TileCorners *tc);
	void AddFoundations(Foundation *fnd);
	void AddPlatform(Platform *plat);
	void AddSupport(Support *supp);
	void AddPath(Path *path);
	void AddBuildArrows(DisplayedObject *obj);
	void RemoveAnimations(AnimationType anim_type, PersonType pers_type);
	void AddAnimationSprites(AnimationSprites *an_spr);

	/**
	 * Get a ground sprite.
	 * @param type Type of surface.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetSurfaceSprite(uint8 type, uint8 surf_spr, ViewOrientation orient) const
	{
		return this->surface[type].surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a path tile sprite.
	 * @param type Type of path. @see PathTypes
	 * @param slope Slope of the path (in #VOR_NORTH orientation).
	 * @param orient Display orientation.
	 * @return Requested sprite if available.
	 * @todo [low] \a type is not used due to lack of other path types (sprites).
	 */
	const ImageData *GetPathSprite(uint8 type, uint8 slope, ViewOrientation orient) const
	{
		// this->path[type] is not used, as there exists only one type of paths.
		return this->path_sprites->sprites[_path_rotation[slope][orient]];
	}

	/**
	 * Get a mouse tile cursor sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetCursorSprite(uint8 surf_spr, ViewOrientation orient) const
	{
		if (this->tile_select == nullptr) return nullptr;
		return this->tile_select->surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a mouse tile corner sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation (selected corner).
	 * @param cursor Ground cursor orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetCornerSprite(uint8 surf_spr, ViewOrientation orient, ViewOrientation cursor) const
	{
		if (this->tile_corners == nullptr) return nullptr;
		return this->tile_corners->sprites[cursor][_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get an arrow for indicating the build direction of paths and tracks.
	 * @param spr_num Sprite index.
	 * @param orient Orientation of the view.
	 * @return Requested arrow sprite if available.
	 */
	const ImageData *GetArrowSprite(uint8 spr_num, ViewOrientation orient) const
	{
		if (this->build_arrows == nullptr) return nullptr;
		assert(spr_num < 4);
		return this->build_arrows->sprites[(spr_num + 4 - orient) % 4];
	}

	/**
	 * Get a sprite of an animation.
	 * @param anim_type Type of animation to retrieve.
	 * @param pers_type Type of person to retrieve.
	 * @param frame_index Index of the frame to display.
	 * @param view Orientation of the view.
	 * @return The sprite, if it is available.
	 * @todo [high] Pulling animations from a map for drawing sprites is too expensive.
	 */
	const ImageData *GetAnimationSprite(AnimationType anim_type, uint16 frame_index, PersonType pers_type, ViewOrientation view) const
	{
		/* anim_type = 1..4, normalize with -1, subtract orientation,
		 * add 4 + mod 4 to get the normalized animation, +1 to get the real animation. */
		anim_type = (AnimationType)(1 + (4 + (anim_type - 1) - view) % 4);

		for (auto iter = this->animations.find(anim_type); iter != this->animations.end(); ++iter) {
			const AnimationSprites *asp = iter->second;

			if (asp->anim_type != anim_type) return nullptr;
			if (asp->person_type == pers_type) return asp->sprites[frame_index];
		}
		return nullptr;
	}

	const uint16 size; ///< Width of the tile.

	SurfaceData surface[GTP_COUNT];   ///< Surface data.
	Foundation *foundation[FDT_COUNT]; ///< Foundation.
	Platform *platform;                ///< Platform block.
	Support *support;                  ///< Support block.
	TileSelection *tile_select;        ///< Tile selection sprites.
	TileCorners *tile_corners;         ///< Tile corner sprites.
	Path *path_sprites;                ///< Path sprites.
	DisplayedObject *build_arrows;     ///< Arrows displaying build direction of paths and tracks.
	AnimationSpritesMap animations;    ///< %Animation sprites ordered by animation type.

protected:
	void Clear();
};

/**
 * Storage and management of all sprites.
 * @ingroup sprites_group
 */
class SpriteManager {
public:
	SpriteManager();
	~SpriteManager();

	void LoadRcdFiles();
	void AddBlock(RcdBlock *block);

	const SpriteStorage *GetSprites(uint16 size) const;
	void AddAnimation(Animation *anim);
	const ImageData *GetTableSprite(uint16 number) const;
	const Rectangle16 &GetTableSpriteSize(uint16 number);
	const Animation *GetAnimation(AnimationType anim_type, PersonType per_type) const;

protected:
	const char *Load(const char *fname);
	SpriteStorage *GetSpriteStore(uint16 width);

	RcdBlock *blocks;         ///< List of loaded RCD data blocks.

	SpriteStorage store;      ///< Sprite storage of size 64.
	AnimationsMap animations; ///< Available animations.

private:
	bool LoadSURF(RcdFileReader *rcd_file, const ImageMap &sprites);

	void SetSpriteSize(uint16 start, uint16 end, Rectangle16 &rect);
};

bool LoadSpriteFromFile(RcdFileReader *rcd_file, const ImageMap &sprites, ImageData **spr);
bool LoadTextFromFile(RcdFileReader *rcd_file, const TextMap &texts, TextData **txt);

extern SpriteManager _sprite_manager;
extern GuiSprites _gui_sprites;

#endif
