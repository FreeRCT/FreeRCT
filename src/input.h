/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file input.h Generic handling of mouse and keyboard input. */

#ifndef INPUT_H
#define INPUT_H

/** Known mouse buttons. */
enum MouseButtons {
	MB_NONE = 0, ///< No buttons down.
	MB_LEFT = 1, ///< Left button down.
};

class MouseMode; // Forward declaration.

/** Known mouse modes. */
enum MouseModes {
	MM_TILE_TERRAFORM, ///< Terraforming a (single) tile.
};

/**
 * Class tracking mouse and keyboard input, and sending input events to other parts of the program.
 */
class GenericInput {
public:
	GenericInput();
	~GenericInput();

	void SetMouseMode(MouseModes mode);

	void MouseMoveEvent(int newx, int newy);
	void MouseButtonEvent(MouseButtons button, bool pressed);
	void MouseWheelEvent(int direction);

	int mouse_x;   ///< Last known X position of the mouse.
	int mouse_y;   ///< Last known Y position of the mouse.
	uint8 buttons; ///< Mouse buttons state. @see MouseButtons

protected:
	MouseMode *handler; ///< Handler for mouse events in the current mode.
};

extern GenericInput _input;

#endif
