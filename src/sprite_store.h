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
#include "fence.h"
#include "person_type.h"
#include "language.h"
#include "track_piece.h"
#include "weather.h"
#include "gui_sprites.h"
#include "sprite_data.h"
#include <map>
#include <set>

extern const uint8 _slope_rotation[NUM_SLOPE_SPRITES][4];

class RcdFileReader;
class ImageData;

/**
 * Block of data from a RCD file.
 * @ingroup sprites_group
 */
class RcdBlock {
public:
	RcdBlock() = default;
	virtual ~RcdBlock() = default;
};

typedef std::map<uint32, ImageData *> ImageMap; ///< Map of loaded image data blocks.

/** Texts of objects. */
class TextData : public RcdBlock {
public:
	/** Groups translations of one string in all languages. Memory is not owned. */
	struct TextString {
		std::vector<const char*> languages[LANGUAGE_COUNT];  ///< The string in all languages.
		const char *name;                                    ///< Name of this string.
	};

	void Load(RcdFileReader *rcd_file);

	uint string_count;   ///< Number of strings in #strings.
	std::unique_ptr<TextString[]> strings; ///< Strings of the text.
	std::unique_ptr<char[]> text_data;     ///< Text data (UTF-8) itself.
};

typedef std::map<uint32, TextData *> TextMap; ///< Map of loaded text blocks.

/**
 * A surface in all orientations.
 * @ingroup sprites_group
 */
class SurfaceData {
public:
	SurfaceData();

	bool HasAllSprites() const;

	ImageData *surface[NUM_SLOPE_SPRITES]; ///< Sprites displaying the slopes.
};

/**
 * Highlight the currently selected corner of a ground tile with a corner selection sprite.
 * @ingroup sprites_group
 */
class TileCorners {
public:
	TileCorners();

	ImageData *sprites[VOR_NUM_ORIENT][NUM_SLOPE_SPRITES]; ///< Corner selection sprites.
};

/**
 * %Fence sprites.
 * @ingroup sprite_group
 */
class Fence : public RcdBlock {
public:
	Fence();
	~Fence();

	void Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint8 type;    ///< Type of fence. (@see FenceType)
	uint16 width;  ///< Width of a tile.
	ImageData *sprites[FENCE_COUNT]; ///< Fence sprites, may contain \c nullptr sprites.
};

/**
 * %Path decoration sprites.
 * @ingroup sprites_group
 */
class PathDecoration {
public:
	static constexpr int LITTER_VOMIT_COUNT = 4;  ///< Number of different litter or vomit sprites.
	PathDecoration();

	ImageData *litterbin[4];        ///< Normal (non-demolished, non-full) litter bins.
	ImageData *overflow_bin[4];     ///< Overflowing (non-demolished) litter bins.
	ImageData *demolished_bin[4];   ///< Demolished litter bins.
	ImageData *lamp_post[4];        ///< Lamp posts (non-demolished).
	ImageData *demolished_lamp[4];  ///< Demolished lamp posts.
	ImageData *bench[4];            ///< Normal (non-demolished) benches.
	ImageData *demolished_bench[4]; ///< Demolished benches.

	ImageData *flat_litter[4];    ///< 4 types of litter at flat path tile (may be \c nullptr).
	ImageData *ramp_litter[4][4]; ///< For each ramp, 4 types of litter (may be \c nullptr).
	ImageData *flat_vomit[4];     ///< 4 types of vomit at flat path tile (may be \c nullptr).
	ImageData *ramp_vomit[4][4];  ///< For each ramp, 4 types of vomit at flat path tile (may be \c nullptr).
};

/**
 * %Foundation sprites.
 * @ingroup sprites_group
 */
class Foundation {
public:
	Foundation();

	ImageData *sprites[6]; ///< %Foundation sprites.
};

/**
 * %Platform sprites.
 * @ingroup sprites_group
 * @note There is only one type of platform, so no need to define its type.
 */
class Platform {
public:
	Platform();

	ImageData *flat[2]; ///< Flat platforms.
	ImageData *ramp[4]; ///< Raised platform at one edge. See #TileEdge for the order.
	ImageData *right_ramp[4]; ///< Raised platform at one edge with only the right leg. See #TileEdge for the order.
	ImageData *left_ramp[4];  ///< Raised platform at one edge with only the left leg. See #TileEdge for the order.
};

/**
 * %Support sprites.
 * @ingroup sprites_group
 */
class Support {
public:
	Support();

	ImageData *sprites[SSP_COUNT]; ///< %Support sprites.
};

/**
 * Frame sets.
 * @ingroup sprite_group
 */
class FrameSet : public RcdBlock {
public:
	FrameSet();

	void Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	ImageData* GetSprite(uint16 x, uint16 y, uint8 orientation, int zoom) const;

	uint16 width_x;                   ///< The number of voxels in x direction.
	uint16 width_y;                   ///< The number of voxels in y direction.

private:
	int scales;                                ///< Number of zoom scales.
	std::unique_ptr<uint16[]> width;           ///< Width of a tile for each scale.
	std::unique_ptr<ImageData*[]> sprites[4];  ///< Sprites for the ne,se,sw,nw orientations.
};
typedef std::pair<std::string, int> ImageSetKey;         ///< Pair of <RCD file name, RCD block number>.
typedef std::map<ImageSetKey, std::unique_ptr<FrameSet>> FrameSetsMap;  ///< Map of available frame sets.

/**
 * Timed animations.
 * @ingroup sprite_group
 */
class TimedAnimation : public RcdBlock {
public:
	TimedAnimation();
	~TimedAnimation();

	void Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	int GetTotalDuration() const;
	int GetFrame(int time, bool loop_around) const;

	int frames;   ///< Number of frames
	std::unique_ptr<int[]> durations;   ///< Duration of each frame in milliseconds.
	std::unique_ptr<const FrameSet*[]> views; ///< Each frame's set of sprites.
};
typedef std::map<ImageSetKey, std::unique_ptr<TimedAnimation>> TimedAnimationsMap;  ///< Map of available timed animations.

/**
 * Displayed object.
 * (For now, just the arrows pointing in the build direction, hopefully also usable for other 1x1 tile objects.)
 * @ingroup sprites_group
 */
class DisplayedObject {
public:
	DisplayedObject();

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

	ANIM_MECHANIC_REPAIR_NE =  5,  ///< Animation when a mechanic repairs a ride, NE view.
	ANIM_MECHANIC_REPAIR_SE =  6,  ///< Animation when a mechanic repairs a ride, SE view.
	ANIM_MECHANIC_REPAIR_SW =  7,  ///< Animation when a mechanic repairs a ride, SW view.
	ANIM_MECHANIC_REPAIR_NW =  8,  ///< Animation when a mechanic repairs a ride, NW view.

	ANIM_HANDYMAN_WATER_NE  =  9,  ///< Animation when a handyman waters the flowerbeds, NE view.
	ANIM_HANDYMAN_WATER_SE  = 10,  ///< Animation when a handyman waters the flowerbeds, SE view.
	ANIM_HANDYMAN_WATER_SW  = 11,  ///< Animation when a handyman waters the flowerbeds, SW view.
	ANIM_HANDYMAN_WATER_NW  = 12,  ///< Animation when a handyman waters the flowerbeds, NE view.

	ANIM_HANDYMAN_SWEEP_NE  = 13,  ///< Animation when a handyman sweeps the paths, NE view.
	ANIM_HANDYMAN_SWEEP_SE  = 14,  ///< Animation when a handyman sweeps the paths, SE view.
	ANIM_HANDYMAN_SWEEP_SW  = 15,  ///< Animation when a handyman sweeps the paths, SW view.
	ANIM_HANDYMAN_SWEEP_NW  = 16,  ///< Animation when a handyman sweeps the paths, NE view.

	ANIM_HANDYMAN_EMPTY_NE  = 17,  ///< Animation when a handyman empties a bin, NE view.
	ANIM_HANDYMAN_EMPTY_SE  = 18,  ///< Animation when a handyman empties a bin, SE view.
	ANIM_HANDYMAN_EMPTY_SW  = 19,  ///< Animation when a handyman empties a bin, SW view.
	ANIM_HANDYMAN_EMPTY_NW  = 20,  ///< Animation when a handyman empties a bin, NW view.

	ANIM_GUEST_BENCH_NE     = 21,  ///< Animation when a guest sits on a bench, NE view.
	ANIM_GUEST_BENCH_SE     = 22,  ///< Animation when a guest sits on a bench, SE view.
	ANIM_GUEST_BENCH_SW     = 23,  ///< Animation when a guest sits on a bench, SW view.
	ANIM_GUEST_BENCH_NW     = 24,  ///< Animation when a guest sits on a bench, NW view.

	ANIM_BEGIN = ANIM_WALK_NE,            ///< First animation.
	ANIM_LAST  = ANIM_GUEST_BENCH_NW,     ///< Last animation.
	ANIM_INVALID = 0xFF,                  ///< Invalid animation.
};
DECLARE_POSTFIX_INCREMENT(AnimationType)

/** Data structure holding a single animation. */
class Animation : public RcdBlock {
public:
	Animation();

	void Load(RcdFileReader *rcd_file);

	uint16 frame_count;      ///< Number of frames.
	uint8 person_type;       ///< Type of persons supported by this animation.
	AnimationType anim_type; ///< Animation ID.

	std::unique_ptr<AnimationFrame[]> frames;  ///< Frames of the animation.
};

typedef std::multimap<AnimationType, Animation *> AnimationsMap; ///< Multi-map of available animations.

/** Data structure holding sprite of an animation. */
class AnimationSprites: public RcdBlock {
public:
	AnimationSprites();

	void Load(RcdFileReader *rcd_file, const ImageMap &sprites);

	uint16 width;            ///< Width of the tile.
	uint8 person_type;       ///< Type of persons supported by this animation.
	AnimationType anim_type; ///< Animation ID.
	uint16 frame_count;      ///< Number of frames.

	std::unique_ptr<ImageData*[]> sprites;     ///< Sprites of the animation.
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

/** Red/orange/green light sprites. */
enum LightRedOrangeGreen {
	LROG_RED,    ///< Red light of the red/orange/green light.
	LROG_ORANGE, ///< Orange light of the red/orange/green light.
	LROG_GREEN,  ///< Green light of the red/orange/green light.
	LROG_NONE,   ///< No light of the red/orange/green light.
};

/** Red/green light sprites. */
enum LightRedGreen {
	LRG_RED,    ///< Red light of the red/green light.
	LRG_GREEN,  ///< Green light of the red/green light.
	LRG_NONE,   ///< No light of the red/green light.
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
	void LoadGBOR(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadGCHK(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadGSLI(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadGSCL(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadGSLP(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts);
	void LoadMENU(RcdFileReader *rcd_file, const ImageMap &sprites);

	bool HasSufficientGraphics() const;

	BorderSpriteData titlebar;              ///< Title bar sprite data.
	BorderSpriteData button;                ///< Normal button sprite data.
	BorderSpriteData left_tabbar;           ///< Sprite data of the left filler of a tab bar row.
	BorderSpriteData tab_tabbar;            ///< Sprite data of the tab in the tab bar row.
	BorderSpriteData right_tabbar;          ///< Sprite data of the right filler of a tab bar row.
	BorderSpriteData tabbar_panel;          ///< Sprite data of the panel below a tab bar row.
	BorderSpriteData panel;                 ///< Sprite data of the normal panel.

	CheckableWidgetSpriteData checkbox;     ///< Check box sprite data.
	CheckableWidgetSpriteData radio_button; ///< Radio button sprite data.

	SliderSpriteData hor_slider;            ///< Horizontal slider bar sprite data.
	SliderSpriteData vert_slider;           ///< Vertical slider bar sprite data.

	ScrollbarSpriteData hor_scroll;         ///< Horizontal scroll bar sprite data.
	ScrollbarSpriteData vert_scroll;        ///< Vertical scroll bar sprite data.

	ImageData *slope_select[TSL_COUNT_VERTICAL]; ///< Slope selection sprites.
	ImageData *bend_select[TBN_COUNT];      ///< Bend selection sprites.
	ImageData *bank_select[TPB_COUNT];      ///< Banking selection sprites.
	ImageData *platform_select[2];          ///< %Platform selection sprites.
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
	ImageData *weather[WTP_COUNT];          ///< Weather sprites.
	ImageData *lights_rog[4];               ///< Red/orange/green light. @see LightRedOrangeGreen
	ImageData *lights_rg[3];                ///< Red/green light. @see LightRedGreen

	ImageData *message_goto;                ///< Inbox button go to location sprite.
	ImageData *message_park;                ///< Inbox button park management sprite.
	ImageData *message_guest;               ///< Inbox button guest window sprite.
	ImageData *message_ride;                ///< Inbox button ride instance window sprite.
	ImageData *message_ride_type;           ///< Inbox button ride select gui sprite.

	ImageData *loadsave_err;                ///< Loadsave icon for files with errors.
	ImageData *loadsave_warn;               ///< Loadsave icon for files with warnings.
	ImageData *loadsave_ok;                 ///< Loadsave icon for files without warnings or errors.

	ImageData *speed_0;  ///< 0× speed icon.
	ImageData *speed_1;  ///< 1× speed icon.
	ImageData *speed_2;  ///< 2× speed icon.
	ImageData *speed_4;  ///< 4× speed icon.
	ImageData *speed_8;  ///< 8× speed icon.

	ImageData *toolbar_images[SPR_GUI_TOOLBAR_END - SPR_GUI_TOOLBAR_BEGIN];  ///< Toolbar button sprites.

	ImageData *mainmenu_logo;               ///< Main menu: FreeeRCT logo.
	ImageData *mainmenu_splash;             ///< Main menu: Splash screen.
	ImageData *mainmenu_new;                ///< Main menu: New Game button.
	ImageData *mainmenu_load;               ///< Main menu: Load Game button.
	ImageData *mainmenu_settings;           ///< Main menu: Settings button.
	ImageData *mainmenu_quit;               ///< Main menu: Quit button.
	uint32     mainmenu_splash_duration;    ///< Main menu: Duration of the splash screen.

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

	void RemoveAnimations(AnimationType anim_type, PersonType pers_type);
	void AddAnimationSprites(AnimationSprites *an_spr);
	void AddFence(Fence *fnc);

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
	 * Get a fence sprite.
	 * @param type FenceType of fence.
	 * @param edge The edge of the voxel.
	 * @param tslope The imploded slope of the voxel.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetFenceSprite(FenceType type, TileEdge edge, uint8 tslope, ViewOrientation orient) const
	{
		if (this->fence[type] == nullptr) return nullptr;

		uint8 corner_height[4];
		TileSlope slope = ExpandTileSlope(tslope);
		ComputeCornerHeight(slope, (slope & TSB_TOP) != 0 ? 1 : 0, corner_height);

		/*
		 * Compute edge case 0..2
		 * 0 = flat, 1 = one corner raised, 2 = other corner raised
		 */
		uint8 edge_case = (corner_height[edge] > corner_height[(edge+1) % 4] ? 1 : 0) +
				(corner_height[(edge+1) % 4] > corner_height[edge] ? 2 : 0);

		uint8 sprite = edge_case + edge * FENCE_EDGE_COUNT;
		sprite = (sprite + FENCE_COUNT - FENCE_EDGE_COUNT * orient) % FENCE_COUNT;
		return this->fence[type]->sprites[sprite];
	}

	/**
	 * Get a path tile sprite.
	 * @param type Type of path. @see PathType
	 * @param status Status of path. @see PathStatus
	 * @param slope Slope of the path (in #VOR_NORTH orientation).
	 * @param orient Display orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetPathSprite(PathType type, PathStatus status, uint8 slope, ViewOrientation orient) const
	{
		return this->path_sprites[type * PAS_COUNT * PATH_COUNT + status * PATH_COUNT + _path_rotation[slope][orient]];
	}

	/**
	 * Get a mouse tile cursor sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const ImageData *GetCursorSprite(uint8 surf_spr, ViewOrientation orient) const
	{
		return this->tile_select.surface[_slope_rotation[surf_spr][orient]];
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
		return this->tile_corners.sprites[cursor][_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get an arrow for indicating the build direction of paths and tracks.
	 * @param spr_num Sprite index.
	 * @param orient Orientation of the view.
	 * @return Requested arrow sprite if available.
	 */
	const ImageData *GetArrowSprite(uint8 spr_num, ViewOrientation orient) const
	{
		return this->build_arrows.sprites[(spr_num + 4 - orient) % 4];
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
		int anim_type_base = anim_type - ANIM_BEGIN;
		anim_type_base /= 4;
		anim_type_base *= 4;
		anim_type_base += ANIM_BEGIN;
		const int anim_type_off = (anim_type - anim_type_base - view) & 3;
		anim_type = AnimationType(anim_type_base + anim_type_off);
		assert(anim_type >= ANIM_BEGIN && anim_type <= ANIM_LAST);

		for (auto iter = this->animations.find(anim_type); iter != this->animations.end(); ++iter) {
			const AnimationSprites *asp = iter->second;

			if (asp->anim_type != anim_type) return nullptr;
			if (asp->person_type == pers_type) return asp->sprites[frame_index];
		}
		return nullptr;
	}

	/**
	 * Get a foundation sprite.
	 * @param f Foundation type.
	 * @param i Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetFoundationSprite(int f, int i) const
	{
		return this->foundation[f].sprites[i];
	}

	/**
	 * Get a support sprite.
	 * @param i Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetSupportSprite(int i) const
	{
		return this->support.sprites[i];
	}

	/**
	 * Get a flat platform sprite.
	 * @param i Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetFlatPlatformSprite(int i) const
	{
		return this->platform.flat[i];
	}

	/**
	 * Get a ramped platform sprite.
	 * @param i Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetRampPlatformSprite(int i) const
	{
		return this->platform.ramp[i];
	}

	/**
	 * Get a litter sprite.
	 * @param ramp Ramp direction of the path (#INVALID_EDGE for flat paths).
	 * @param index Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetLitterSprite(TileEdge ramp, int index) const
	{
		return (ramp == INVALID_EDGE ? this->path_decoration.flat_litter : this->path_decoration.ramp_litter[ramp])[index];
	}

	/**
	 * Get a demolished bench sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetDemolishedBenchSprite(uint8 orient) const
	{
		return this->path_decoration.demolished_bench[orient];
	}

	/**
	 * Get a demolished bin sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetDemolishedBinSprite(uint8 orient) const
	{
		return this->path_decoration.demolished_bin[orient];
	}

	/**
	 * Get a demolished lamp sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetDemolishedLampSprite(uint8 orient) const
	{
		return this->path_decoration.demolished_lamp[orient];
	}

	/**
	 * Get a normal bench sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetNormalBenchSprite(uint8 orient) const
	{
		return this->path_decoration.bench[orient];
	}

	/**
	 * Get a normal bin sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetNormalBinSprite(uint8 orient) const
	{
		return this->path_decoration.litterbin[orient];
	}

	/**
	 * Get a normal lamp sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetNormalLampSprite(uint8 orient) const
	{
		return this->path_decoration.lamp_post[orient];
	}

	/**
	 * Get an overflowing bin sprite.
	 * @param orient View orientation.
	 * @return The sprite.
	 */
	const ImageData *GetOverflowBinSprite(uint8 orient) const
	{
		return this->path_decoration.overflow_bin[orient];
	}

	/**
	 * Get a vomit sprite.
	 * @param ramp Ramp direction of the path (#INVALID_EDGE for flat paths).
	 * @param index Sprite index.
	 * @return The sprite.
	 */
	const ImageData *GetVomitSprite(TileEdge ramp, int index) const
	{
		return (ramp == INVALID_EDGE ? this->path_decoration.flat_vomit : this->path_decoration.ramp_vomit[ramp])[index];
	}

	const uint16 size; ///< Width of the tile.

	SurfaceData surface[GTP_COUNT];   ///< Surface data.
	Foundation foundation[FDT_COUNT]; ///< Foundation.
	Platform platform;                ///< %Platform block.
	Support support;                  ///< Support block.
	SurfaceData tile_select;          ///< Tile selection sprites.
	TileCorners tile_corners;         ///< Tile corner sprites.
	Fence *fence[FENCE_COUNT];        ///< Available fence types.
	ImageData *path_sprites[PAT_COUNT * PAS_COUNT * PATH_COUNT];  ///< Path sprites.
	PathDecoration path_decoration;   ///< %Path decoration sprites.
	DisplayedObject build_arrows;     ///< Arrows displaying build direction of paths and tracks.
	AnimationSpritesMap animations;   ///< %Animation sprites ordered by animation type.
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
	void AddBlock(std::unique_ptr<RcdBlock> block);

	void AddAnimation(Animation *anim);
	const ImageData *GetTableSprite(uint16 number) const;
	const Rectangle16 &GetTableSpriteSize(uint16 number);
	const Animation *GetAnimation(AnimationType anim_type, PersonType per_type) const;
	const Fence *GetFence(FenceType fence, int zoom) const;
	const FrameSet *GetFrameSet(const ImageSetKey &key) const;
	const TimedAnimation *GetTimedAnimation(const ImageSetKey &key) const;
	bool HasPath(PathType type, PathStatus status) const;

	/**
	 * Get a sprite from the sprite storage. If the sprite is only available at a different resolution, it will be scaled.
	 * @param zoom Zoom scale.
	 * @param f Functor to extract the image from a sprite storage.
	 * @param args Arguments to forward to the functor.
	 * @return The image.
	 */
	template<typename Fn, typename... Args>
	const ImageData *GetSprite(int zoom, Fn f, Args... args)
	{
		for (int z = zoom; z < ZOOM_SCALES_COUNT; ++z) {
			const ImageData *img = (this->store.at(z).*f)(args...);
			if (img != nullptr) return img->Scale(static_cast<float>(TileWidth(zoom)) / TileWidth(z));
		}
		for (int z = zoom - 1; z >= 0; --z) {
			const ImageData *img = (this->store.at(z).*f)(args...);
			if (img != nullptr) return img->Scale(static_cast<float>(TileWidth(zoom)) / TileWidth(z));
		}
		return nullptr;
	}

	/**
	 * Get the sprite storage at the size to use in the Gui.
	 * @return The sprite storage.
	 */
	inline SpriteStorage &GetGuiSpriteStore()
	{
		return this->store.at(DEFAULT_ZOOM);
	}

protected:
	void Load(const char *fname);

	std::vector<std::unique_ptr<RcdBlock>> blocks;  ///< List of loaded RCD data blocks.

	SpriteStorage *GetSpriteStore(int width);

	std::vector<SpriteStorage> store;        ///< Sprite storage for each zoom scale.
	AnimationsMap animations;                ///< Available animations.
	FrameSetsMap frame_sets;                 ///< Available frame sets.
	TimedAnimationsMap timed_animations;     ///< Available timed animations.

private:
	void LoadSURF(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadTSEL(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadTCOR(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadFUND(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadPATH(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadPDEC(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadBDIR(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadPLAT(RcdFileReader *rcd_file, const ImageMap &sprites);
	void LoadSUPP(RcdFileReader *rcd_file, const ImageMap &sprites);

	void SetSpriteSize(uint16 start, uint16 end, Rectangle16 &rect);
};

void LoadSpriteFromFile(RcdFileReader *rcd_file, const ImageMap &sprites, ImageData **spr);
void LoadTextFromFile(RcdFileReader *rcd_file, const TextMap &texts, TextData **txt);
Rectangle16 GetSpriteSize(const ImageData *imd);

extern SpriteManager _sprite_manager;
extern GuiSprites _gui_sprites;

#endif
