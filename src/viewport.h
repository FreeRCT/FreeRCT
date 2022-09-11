/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.h %Main display data. */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "window.h"
#include "mouse_mode.h"

class Viewport;
class Person;
class RideInstance;

/**
 * Known mouse modes.
 * @ingroup viewport_group
 */
enum ViewportMouseMode {
	MM_INACTIVE,       ///< Inactive mode.
	MM_COUNT,          ///< Number of mouse modes.
};

/**
 * Sprites that trigger an action when clicked on.
 */
enum ClickableSprite {
	CS_NONE   = 0,      ///< No valid sprite.
	CS_GROUND = 1 << 0, ///< Ground sprite.
	CS_GROUND_EDGE = 1 << 1, ///< Ground edge sprite.
	CS_PATH   = 1 << 2, ///< %Path sprite.
	CS_RIDE   = 1 << 3, ///< Ride sprite.
	CS_PERSON = 1 << 4, ///< %Person sprite.

	CS_LENGTH = 8,      ///< Bitlength of the enum.
	CS_MASK = 0x1f,     ///< Used for converting between this and #SpriteOrder.
};
DECLARE_ENUM_AS_BIT_SET(ClickableSprite)

/**
 * Order of blitting sprites in a single voxel (earlier in the list is sooner).
 * The number shifted by #CS_LENGTH denotes the order, the lower bits are used to denote the kind of sprite
 * being plotted, for mouse-click detection.
 * @see PixelFinder, Viewport::ComputeCursorPosition
 */
enum SpriteOrder {
	SO_NONE            =   0,                           ///< No drawing.
	SO_FOUNDATION      = ( 1 << CS_LENGTH),             ///< Draw foundation sprites.
	SO_GROUND          = ( 2 << CS_LENGTH) | CS_GROUND, ///< Draw ground sprites.
	SO_GROUND_EDGE     = ( 3 << CS_LENGTH) | CS_GROUND_EDGE, ///< Used for ground edge detection.
	SO_FENCE_BACK      = ( 4 << CS_LENGTH),             ///< Draw fence on the back edges
	SO_SUPPORT         = ( 5 << CS_LENGTH),             ///< Draw support sprites.
	SO_PLATFORM        = ( 6 << CS_LENGTH) | CS_RIDE,   ///< Draw platform sprites.
	SO_PATH            = ( 7 << CS_LENGTH) | CS_PATH,   ///< Draw path sprites.
	SO_PATH_OBJECTS    = ( 8 << CS_LENGTH) | CS_PATH,   ///< Draw path object sprites.
	SO_PLATFORM_BACK   = ( 9 << CS_LENGTH) | CS_RIDE,   ///< Background behind the ride (platform background).
	SO_RIDE            = (10 << CS_LENGTH) | CS_RIDE,   ///< Draw ride sprites.
	SO_RIDE_CARS       = (11 << CS_LENGTH) | CS_RIDE,   ///< Cars at a ride.
	SO_RIDE_FRONT      = (12 << CS_LENGTH) | CS_RIDE,   ///< Ride sprite to draw after drawing the cars.
	SO_PLATFORM_FRONT  = (13 << CS_LENGTH) | CS_RIDE,   ///< Front of platform.
	SO_PERSON          = (14 << CS_LENGTH) | CS_PERSON, ///< Draw person sprites.
	SO_PERSON_OVERLAY  = (15 << CS_LENGTH) | CS_PERSON, ///< Draw overlays over person sprites.
	SO_FENCE_FRONT     = (16 << CS_LENGTH),             ///< Draw fence on the front edges.
	SO_CURSOR          = (17 << CS_LENGTH),             ///< Draw cursor sprites.
};

/** Part of a ground tile to select. */
enum GroundTilePart {
	FW_TILE,   ///< Find whole tile only.
	FW_CORNER, ///< Find whole tile or corner.
	FW_EDGE,   ///< Find tile edge.
};

/** Data found by Viewport::ComputeCursorPosition. */
class FinderData {
public:
	FinderData(ClickableSprite allowed_types, GroundTilePart select);

	ClickableSprite allowed; ///< Bit-set of sprite-types looking for. @see ClickableSprites
	GroundTilePart select;   ///< What part to select of a ground tile.

	CursorType cursor;       ///< Type of cursor suggested.
	XYZPoint16 voxel_pos;    ///< Position of the voxel with the closest sprite.
	const Person *person;    ///< Found person, if any.
	uint16 ride;             ///< Found ride instance, if any.
};

/** A text string drifting over the viewport and fading out. */
struct FloatawayText {
	/**
	 * Constructor.
	 * @param text Text to show.
	 * @param pos Pixel position in the world.
	 * @param speed Pixel position change per tick.
	 * @param col RGB colour of the text.
	 * @param fade Alpha decrease per tick.
	 */
	FloatawayText(std::string text, XYZPoint32 pos, XYZPoint32 speed, uint32 colour, int fade)
	: text(text), pos(pos), speed(speed), colour(colour), fade(fade)
	{
	}
	FloatawayText(std::string text, XYZPoint32 pos, uint32 col);

	std::string text;  ///< Text to display.
	XYZPoint32 pos;    ///< Current pixel position.
	XYZPoint32 speed;  ///< Pixel position change per tick.
	uint32 colour;     ///< RGBA colour.
	int fade;          ///< Opacity decrease per tick.
};

/**
 * Class for displaying parts of the world.
 * @ingroup viewport_group
 */
class Viewport : public Window {
public:
	Viewport(const XYZPoint32 &view_pos);
	~Viewport();

	void OnDraw(MouseModeSelector *selector) override;

	void Rotate(int direction);
	void MoveViewport(int dx, int dy);

	ClickableSprite ComputeCursorPosition(FinderData *fdata);

	Point32 ComputeHorizontalTranslation(int dx, int dy);
	int32 ComputeX(int32 xpos, int32 ypos);
	int32 ComputeY(int32 xpos, int32 ypos, int32 zpos);
	Point32 ComputeScreenCoordinate(const XYZPoint32 &pixel) const;

	bool IsUndergroundModeAvailable() const;
	void SetUndergroundMode();
	void ToggleUndergroundMode();
	void AddFloatawayMoneyAmount(const Money &money, const XYZPoint16 &voxel);

	XYZPoint32 view_pos;         ///< Position of the centre point of the viewport.
	uint16 tile_width;           ///< Width of a tile.
	uint16 tile_height;          ///< Height of a tile.
	ViewOrientation orientation; ///< Direction of view.
	Point16 mouse_pos;           ///< Last known position of the mouse.
	bool draw_fps;               ///< Whether to draw an FPS counter.
	bool additions_enabled;      ///< Flashing of world additions is enabled.
	bool underground_mode;       ///< Whether underground mode is displayed in this viewport.
	std::vector<FloatawayText> floataway_texts;  ///< Currently active floataway texts.

protected:
	bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;
	void OnMouseMoveEvent(const Point16 &pos) override;
	WmMouseEvent OnMouseButtonEvent(MouseButtons state, WmMouseEventMode mode) override;
	void OnMouseWheelEvent(int direction) override;
};

/**
 * Convert a voxel coordinate to the pixel coordinate of its top-left corner.
 * @param voxel The voxel coordinate.
 * @return The pixel coordinate.
 */
inline XYZPoint32 VoxelToPixel(const XYZPoint16 &voxel)
{
	return XYZPoint32(voxel.x * 256, voxel.y * 256, voxel.z * 256);
}

#endif
