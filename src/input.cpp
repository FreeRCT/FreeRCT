/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file input.cpp Generic handling of mouse and keyboard input. */

#include "stdafx.h"
#include "input.h"
#include "window.h"

GenericInput _input; ///< Generic mouse and keyboard handler.


/** Base class for interacting mouse modes. */
class MouseMode {
public:
	MouseMode(GenericInput *manager);
	virtual ~MouseMode();

	/** Setup the mouse mode. */
	virtual void Start() = 0;

	/** De-activate the mouse mode. */
	virtual void Stop() = 0;

	virtual void MouseMoveEvent(int newx, int newy);
	virtual void MouseButtonEvent(MouseButtons button, bool pressed);
	virtual void MouseWheelEvent(int direction);

	GenericInput *manager; ///< Generic input handler.
};

/** Mouse mode to terraform (single) tiles. */
class TileMouseMode : public MouseMode {
public:
	TileMouseMode(GenericInput *manager);

	virtual void Start();
	virtual void Stop();

	virtual void MouseMoveEvent(int newx, int newy);
	virtual void MouseButtonEvent(MouseButtons button, bool pressed);

	bool dragging; ///< Mouse is used for dragging.
};

/**
 * Default constructor.
 * @param manager Input manager.
 */
MouseMode::MouseMode(GenericInput *manager)
{
	this->manager = manager;
}

/** Destructor. */
MouseMode::~MouseMode()
{
}

/**
 * Mouse moved to a new position.
 * @param newx New X position.
 * @param newy New Y position.
 * @note Previous position can be queried from the manager.
 */
/* virtual */ void MouseMode::MouseMoveEvent(int newx, int newy)
{
	/* Default mouse mode does not care for mouse positions. */
}

/**
 * Mouse button state changed.
 * @param button Mouse button involved.
 * @param pressed Event is a 'press' event, else it is a 'release' event.
 */
/* virtual */ void MouseMode::MouseButtonEvent(MouseButtons button, bool pressed)
{
	/* Default mouse mode does not care for mouse buttons. */
}

/**
 * Handle a mouse wheel event.
 * @param direction Direction of the event (\c +1 or \c -1).
 */
/* virtual */ void MouseMode::MouseWheelEvent(int direction)
{
	/* Default mouse mode does not care for mouse buttons. */
}


/**
 * Default constructor.
 * @param manager Input manager.
 */
TileMouseMode::TileMouseMode(GenericInput *manager) : MouseMode(manager)
{
	this->Start();
}

/* virtual */ void TileMouseMode::Start()
{
	this->dragging = false;
}

/* virtual */ void TileMouseMode::Stop()
{
	this->dragging = false;
}

/* virtual */ void TileMouseMode::MouseMoveEvent(int newx, int newy)
{
	if (this->dragging) {
		Viewport *w = static_cast<Viewport *>(GetWindowByType(WC_MAINDISPLAY));
		if (w != NULL) w->MoveViewport(newx - this->manager->mouse_x, newy - this->manager->mouse_y);
	}
}

/* virtual */ void TileMouseMode::MouseButtonEvent(MouseButtons button, bool pressed)
{
	if (button != MB_LEFT) return;
	this->dragging = pressed;
}

GenericInput::GenericInput()
{
	this->buttons = MB_NONE;
	this->mouse_x = 0;
	this->mouse_y = 0;
	this->handler = NULL;
}

GenericInput::~GenericInput()
{
}

/**
 * Set a new mouse mode.
 * @param mode New mouse mode to use.
 */
void GenericInput::SetMouseMode(MouseModes mode)
{
	static TileMouseMode *tile_mode = NULL;

	switch (mode) {
		case MM_TILE_TERRAFORM:
			if (tile_mode == NULL) tile_mode = new TileMouseMode(this);

			if (this->handler == tile_mode) return;
			if (this->handler != NULL) this->handler->Stop();
			this->handler = tile_mode;
			this->handler->Start();
			break;

		default:
			NOT_REACHED();
	}
}

/**
 * Handle a mouse movement event.
 * @param newx New X coordinate of the mouse.
 * @param newy New Y coordinate of the mouse.
 */
void GenericInput::MouseMoveEvent(int newx, int newy)
{
	if (this->mouse_x == newx && this->mouse_y == newy) return;
	if (this->handler != NULL) this->handler->MouseMoveEvent(newx, newy);
	this->mouse_x = newx;
	this->mouse_y = newy;
}

/**
 * Handle a mouse button event.
 * @param button Button involved in the event.
 * @param pressed Button was pressed (else it was released).
 */
void GenericInput::MouseButtonEvent(MouseButtons button, bool pressed)
{
	uint8 new_buttons = (pressed) ? MB_LEFT : MB_NONE;
	if (this->buttons == new_buttons) return;
	if (this->handler != NULL) this->handler->MouseButtonEvent(button, pressed);
	this->buttons = new_buttons;
}

/**
 * Handle a mouse wheel event.
 * @param direction Direction of the event (\c +1 or \c -1).
 */
void GenericInput::MouseWheelEvent(int direction)
{
	/* Simply pass on to the mode handler, no storage required. */
	if (this->handler != NULL) this->handler->MouseWheelEvent(direction);
}
