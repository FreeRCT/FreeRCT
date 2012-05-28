/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h %Sprite storage data structure. */

#ifndef SPRITE_STORE_H
#define SPRITE_STORE_H

#include "orientation.h"
#include "path.h"
#include "tile.h"
#include "person_type.h"
#include <map>

extern const uint8 _slope_rotation[NUM_SLOPE_SPRITES][4];

class RcdFile;

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

/**
 * Image data of 8bpp images.
 * @ingroup sprites_group
 */
class ImageData : public RcdBlock {
public:
	ImageData();
	~ImageData();

	bool Load(RcdFile *rcd_file, size_t length);

	uint8 GetPixel(uint16 xoffset, uint16 yoffset) const;

	uint16 width;  ///< Width of the image.
	uint16 height; ///< Height of the image.
	uint32 *table; ///< The jump table. For missing entries, #INVALID_JUMP is used.
	uint8 *data;   ///< The image data itself.

	static const uint32 INVALID_JUMP;
};

typedef std::map<uint32, ImageData *> ImageMap; ///< Map of loaded image data blocks.

/**
 * %Sprite data.
 * @todo Move the offsets to the image data.
 * @ingroup sprites_group
 */
class Sprite : public RcdBlock {
public:
	Sprite();
	~Sprite();

	bool Load(RcdFile *rcd_file, size_t length, const ImageMap &images);

	/**
	 * Return the pixel-value of the provided position.
	 * @param xoffset Horizontal offset in the sprite.
	 * @param yoffset Vertical offset in the sprite.
	 * @return Pixel value at the given postion, or \c 0 if transparent.
	 */
	inline uint8 GetPixel(uint16 xoffset, uint16 yoffset) const {
		return img_data->GetPixel(xoffset, yoffset);
	}

	ImageData *img_data;  ///< Image data of the sprite.
	int16 xoffset;        ///< Horizontal offset of the image.
	int16 yoffset;        ///< Vertical offset of the image.
};

typedef std::map<uint32, Sprite *> SpriteMap; ///< Map of loaded sprite blocks.

/**
 * A surface in all orientations.
 * @ingroup sprites_group
 */
class SurfaceData : public RcdBlock {
public:
	SurfaceData();
	~SurfaceData();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	uint8 type;    ///< Type of surface.
	Sprite *surface[NUM_SLOPE_SPRITES]; ///< Sprites displaying the slopes.
};

/**
 * Highlight the currently selected ground tile with a selection cursor.
 * @ingroup sprites_group
 */
class TileSelection : public RcdBlock {
public:
	TileSelection();
	~TileSelection();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *surface[NUM_SLOPE_SPRITES]; ///< Sprites with a selection cursor.
};

/**
 * Highlight the currently selected corner of a ground tile with a corner selection sprite.
 * @ingroup sprites_group
 */
class TileCorners : public RcdBlock {
public:
	TileCorners();
	~TileCorners();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *sprites[VOR_NUM_ORIENT][NUM_SLOPE_SPRITES]; ///< Corner selection sprites.
};

/**
 * %Path sprites.
 * @ingroup sprites_group
 */
class Path : public RcdBlock {
public:
	Path();
	~Path();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 type;   ///< Type of the path. @see PathTypes
	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *sprites[PATH_COUNT]; ///< Path sprites, may contain \c NULL sprites.
};

/**
 * %Foundation sprites.
 * @ingroup sprites_group
 */
class Foundation : public RcdBlock {
public:
	Foundation();
	~Foundation();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 type;        ///< Type of the foundation. @see FoundationType
	uint16 width;       ///< Width of a tile.
	uint16 height;      ///< Height of a tile.
	Sprite *sprites[6]; ///< Foundation sprites.
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

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;       ///< Width of the tile.
	Sprite *sprites[4]; ///< Object displayed towards NE, SE, SW, NW edges.
};

/** One frame in an animation. */
struct AnimationFrame {
	uint16 duration;      ///< Length of this frame in milli seconds.
	int16 dx;             ///< Person movement in X direction after displaying this frame.
	int16 dy;             ///< Person movement in Y direction after displaying this frame.
};

/** Available animations. */
enum AnimationType {
	ANIM_WALK_NE = 1,          ///< Walk in north-east direction.
	ANIM_WALK_SE = 2,          ///< Walk in south-east direction.
	ANIM_WALK_SW = 3,          ///< Walk in south-west direction.
	ANIM_WALK_NW = 4,          ///< Walk in north-west direction.

	ANIM_BEGIN = ANIM_WALK_NE, ///< First animation.
	ANIM_LAST  = ANIM_WALK_NW, ///< Last animation.
	ANIM_INVALID = 0xFF,       ///< Invalid animation.
};

/** Data structure holding a single animation. */
class Animation : public RcdBlock {
public:
	Animation();
	~Animation();

	bool Load(RcdFile *rcd_file, size_t length);

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

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;            ///< Width of the tile.
	uint8 person_type;       ///< Type of persons supported by this animation.
	AnimationType anim_type; ///< Animation ID.
	uint16 frame_count;      ///< Number of frames.

	Sprite **sprites;        ///< Sprites of the animation.
};

typedef std::multimap<AnimationType, AnimationSprites *> AnimationSpritesMap; ///< Multi-map of available animation sprites.

/**
 * Sprites of a border.
 * @ingroup gui_sprites_group
 */
enum WidgetBorderSprites {
	WBS_TOP_LEFT,      ///< Top left corner.
	WBS_TOP_MIDDLE,    ///< Top center edge.
	WBS_TOP_RIGHT,     ///< Top right corner.
	WBS_MIDDLE_LEFT,   ///< Left edge.
	WBS_MIDDLE_MIDDLE, ///< Center sprite.
	WBS_MIDDLE_RIGHT,  ///< Right edge.
	WBS_BOTTOM_LEFT,   ///< Bottom left corner.
	WBS_BOTTOM_MIDDLE, ///< Bottom center edge.
	WBS_BOTTOM_RIGHT,  ///< Bottom right corner.

	WBS_COUNT,         ///< Number of sprites of a border.
};

/**
 * %Sprite data of a border.
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

	Sprite *normal[WBS_COUNT];  ///< Sprites to draw for a normal border.
	Sprite *pressed[WBS_COUNT]; ///< Sprites to draw for a pressed border.
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
 * %Sprite data of a checkable sprite.
 * @ingroup gui_sprites_group
 */
struct CheckableWidgetSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint16 width;  ///< Width of the sprite.
	uint16 height; ///< Height of the sprite.

	Sprite *sprites[WCS_COUNT]; ///< Sprites to draw.
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
 * %Sprite data of a slider bar.
 * @ingroup gui_sprites_group
 */
struct SliderSpriteData {
	void Clear();

	bool IsLoaded() const;

	uint16 min_bar_length; ///< Minimal length of a slider bar.
	uint8 stepsize;        ///< Length increment size.
	uint8 height;          ///< Height/width of the bar.

	Sprite *normal[WSS_COUNT]; ///< Sprites for normal slider.
	Sprite *shaded[WSS_COUNT]; ///< Sprites for shaded slider.
};

/**
 * Scrollbar sprites.
 * @ingroup gui_sprites_group
 */
enum WidgetScrollbarSprites {
	WLS_LEFT_BUTTON,           ///< Left/up unpressed button.
	WLS_LEFT_PRESSED_BUTTON,   ///< Left/up pressed button.
	WLS_RIGHT_BUTTON,          ///< Right/down unpressed button.
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
 * %Sprite data of a scrollbar.
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

	Sprite *normal[WLS_COUNT]; ///< Sprites for a normal scrollbar.
	Sprite *shaded[WLS_COUNT]; ///< Sprites for a shaded scrollbar.
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
	bool LoadGBOR(RcdFile*, size_t, const SpriteMap&);
	bool LoadGCHK(RcdFile*, size_t, const SpriteMap&);
	bool LoadGSLI(RcdFile*, size_t, const SpriteMap&);
	bool LoadGSCL(RcdFile*, size_t, const SpriteMap&);

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

	void AddSurfaceSprite(SurfaceData *sd);
	void AddTileSelection(TileSelection *tsel);
	void AddTileCorners(TileCorners *tc);
	void AddFoundations(Foundation *fnd);
	void AddPath(Path *path);
	void AddBuildArrows(DisplayedObject *obj);
	void RemoveAnimations(AnimationType anim_type, PersonType pers_type);
	void AddAnimationSprites(AnimationSprites *an_spr);

	bool HasSufficientGraphics() const;

	/**
	 * Get a ground sprite.
	 * @param type Type of surface.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const Sprite *GetSurfaceSprite(uint8 type, uint8 surf_spr, ViewOrientation orient) const
	{
		if (this->surface[type] == NULL) return NULL;
		return this->surface[type]->surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a path tile sprite.
	 * @param type Type of path. @see PathTypes
	 * @param slope Slope of the path (in #VOR_NORTH orientation).
	 * @param orient Display orientation.
	 * @return Requested sprite if available.
	 * @todo [low] \a type is not used due to lack of other path types (sprites).
	 */
	const Sprite *GetPathSprite(uint8 type, uint8 slope, ViewOrientation orient) const
	{
		// this->path[type] is not used, as there exists only one type of paths.
		return this->path_sprites->sprites[_path_roation[slope][orient]];
	}

	/**
	 * Get a mouse tile cursor sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const Sprite *GetCursorSprite(uint8 surf_spr, ViewOrientation orient) const
	{
		if (this->tile_select == NULL) return NULL;
		return this->tile_select->surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a mouse tile corner sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation (selected corner).
	 * @param cursor Ground cursor orientation.
	 * @return Requested sprite if available.
	 */
	const Sprite *GetCornerSprite(uint8 surf_spr, ViewOrientation orient, ViewOrientation cursor) const
	{
		if (this->tile_corners == NULL) return NULL;
		return this->tile_corners->sprites[cursor][_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get an arrow for indicating the build direction of paths and tracks.
	 * @param spr_num Sprite index.
	 * @param orient Orientation of the view.
	 * @return Requested arrow sprite if available.
	 */
	const Sprite *GetArrowSprite(uint8 spr_num, ViewOrientation orient) const
	{
		if (this->build_arrows == NULL) return NULL;
		assert(spr_num < 4);
		return this->build_arrows->sprites[(spr_num + 4 - orient) % 4];
	}

	/**
	 * Get a sprite of an animation.
	 * @param anim_type Type of animation to retrieve.
	 * @param pers_type Type of person to retrieve.
	 * @param frame_index Index of the frame to display.
	 * @param orient Orientation of the view.
	 * @return The sprite, if it is available.
	 * @todo [high] Pulling animations from a map for drawing sprites is too expensive.
	 */
	const Sprite *GetAnimationSprite(AnimationType anim_type, uint16 frame_index, PersonType pers_type, ViewOrientation view) const
	{
		/* anim_type = 1..4, normalize with -1, subtract orientation,
		 * add 4 + mod 4 to get the normalized animation, +1 to get the real animation. */
		anim_type = (AnimationType)(1 + (4 + (anim_type - 1) - view) % 4);

		AnimationSpritesMap::const_iterator iter = this->animations.find(anim_type);
		while (iter != this->animations.end()) {
			const AnimationSprites *asp = (*iter).second;

			if (asp->anim_type != anim_type) return NULL;
			if (asp->person_type == pers_type) return asp->sprites[frame_index];
			iter++;
		}
		return NULL;
	}

	const uint16 size; ///< Width of the tile.

	SurfaceData *surface[GTP_COUNT];   ///< Surface data.
	Foundation *foundation[FDT_COUNT]; ///< Foundation.
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

	const char *LoadRcdFiles();
	void AddBlock(RcdBlock *block);

	bool HasSufficientGraphics() const;
	const SpriteStorage *GetSprites(uint16 size) const;
	void AddAnimation(Animation *anim);
	const ImageData *GetTableSprite(uint16 number) const;
	const Animation *GetAnimation(AnimationType anim_type, PersonType per_type) const;

protected:
	const char *Load(const char *fname);

	RcdBlock *blocks;         ///< List of loaded Rcd data blocks.

	SpriteStorage store;      ///< %Sprite storage of size 64.
	AnimationsMap animations; ///< Available animations.
};

extern SpriteManager _sprite_manager;
extern GuiSprites _gui_sprites;

#endif
