/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window_constants.h %Window and widget handling constants. */

#ifndef WINDOW_CONSTANTS_H
#define WINDOW_CONSTANTS_H

#include "stdafx.h"
#include "enum_type.h"

/**
 * Available types of windows.
 * @ingroup window_group
 */
enum WindowTypes {
	WC_MAIN_MENU,       ///< Main menu screen.
	WC_MAINDISPLAY,     ///< Main display of the world.
	WC_TOOLBAR,         ///< Main toolbar.
	WC_BOTTOM_TOOLBAR,  ///< Bottom toolbar.
	WC_QUIT,            ///< Quit program window.
	WC_ERROR_MESSAGE,   ///< Error message window.
	WC_PERSON_INFO,     ///< Person window.
	WC_COASTER_MANAGER, ///< Roller coaster manager window.
	WC_COASTER_BUILD,   ///< Roller coaster build/edit window.
	WC_COASTER_REMOVE,  ///< Roller coaster remove window.
	WC_RIDE_BUILD,      ///< Simple ride build window.
	WC_PATH_BUILDER,    ///< %Path build GUI.
	WC_RIDE_SELECT,     ///< Ride selection window.
	WC_SHOP_MANAGER,    ///< Management window of a shop.
	WC_SHOP_REMOVE,     ///< Shop remove window.
	WC_GENTLE_THRILL_RIDE_MANAGER, ///< Management window of a gentle/thrill ride.
	WC_GENTLE_THRILL_RIDE_REMOVE,  ///< Gentle/Thrill ride remove window.
	WC_FENCE,           ///< Fence window.
	WC_SCENERY,         ///< Scenery window.
	WC_PATH_OBJECTS,    ///< Path objects window.
	WC_TERRAFORM,       ///< Terraform window.
	WC_FINANCES,        ///< Finance management window.
	WC_STAFF,           ///< Staff management window.
	WC_INBOX,           ///< Inbox window.
	WC_PARK_MANAGEMENT, ///< Park management window.
	WC_MINIMAP,         ///< Minimap window.
	WC_SETTING,         ///< Setting window.
	WC_LOADSAVE,        ///< Save/load game window.
	WC_SCENARIO_SELECT, ///< Scenario select window.
	WC_CONFIRM,         ///< Confirmation prompt.
	WC_DROPDOWN,        ///< Dropdown window.

	WC_NONE,            ///< Invalid window type.
};

/** Codes of the #NotifyChange function, which gets forwarded through the #Window::OnChange method. */
enum ChangeCode {
	CHG_VIEWPORT_ROTATED,    ///< Viewport rotated.
	CHG_DROPDOWN_RESULT,     ///< The selection of a dropdown window.
	CHG_RESOLUTION_CHANGED,  ///< The size of the FreeRCT window was changed.
	CHG_PERSON_DELETED,      ///< A person has been deleted from the world.
};

/** Various state flags of the %Window. */
enum WindowFlags {
	WF_HIGHLIGHT = 1 << 0, ///< %Window edge is highlighted.
};

/**
 * Known mouse buttons.
 * @ingroup window_group
 */
enum MouseButtons {
	MB_NONE   = 0, ///< No button down.
	MB_LEFT   = 1, ///< Left button down.
	MB_MIDDLE = 2, ///< Middle button down.
	MB_RIGHT  = 4, ///< Right button down.
};
DECLARE_ENUM_AS_BIT_SET(MouseButtons)

/**
 * Mouse events of the window manager. Value is returned from the Window::OnMouseButtonEvent.
 * @ingroup window_group
 */
enum WmMouseEvent {
	WMME_NONE,         ///< Do nothing special.
	WMME_CLOSE_WINDOW, ///< Close the window.
	WMME_MOVE_WINDOW,  ///< Initiate a window move.
};

/**
 * Mouse event modes of the window manager.
 * @ingroup window_group
 */
enum WmMouseEventMode {
	WMEM_PRESS,    ///< Mouse button was pressed.
	WMEM_REPEAT,   ///< Mouse button is held down.
	WMEM_RELEASE,  ///< Mouse button was released.
};

/**
 * Key codes of the window manager.
 * @ingroup window_group
 */
enum WmKeyCode {
	WMKC_CURSOR_UP,    ///< Up arrow key is pressed.
	WMKC_CURSOR_LEFT,  ///< Left arrow key is pressed.
	WMKC_CURSOR_RIGHT, ///< Right arrow key is pressed.
	WMKC_CURSOR_DOWN,  ///< Down arrow key is pressed.
	WMKC_CURSOR_PAGEUP,    ///< PageUp key is pressed.
	WMKC_CURSOR_PAGEDOWN,  ///< PageDown key is pressed.
	WMKC_CURSOR_HOME,  ///< Home key is pressed.
	WMKC_CURSOR_END,   ///< End key is pressed.
	WMKC_BACKSPACE,    ///< Backspace is pressed.
	WMKC_DELETE,       ///< Delete is pressed.
	WMKC_CANCEL,       ///< Cancel is pressed.
	WMKC_CONFIRM,      ///< Confirm is pressed.
	WMKC_FN_BEGIN,                     ///< Beginning of the keys F1..F25.
	WMKC_FN_LAST = WMKC_FN_BEGIN + 25, ///< Last of the keys F1..F25.
	WMKC_SYMBOL,       ///< A symbol is entered.
};

/**
 * Key modifiers of the window manager.
 * @ingroup window_group
 */
enum WmKeyMod {
	WMKM_NONE  = 0,       ///< No modifiers are pressed.
	WMKM_SHIFT = 1 << 0,  ///< Shift key is pressed.
	WMKM_CTRL  = 1 << 1,  ///< Ctrl key is pressed.
	WMKM_ALT   = 1 << 2,  ///< Alt or GUI key is pressed. Some operating systems reserve one of these two keys for special actions;
	                      ///< we therefore treat both modifiers the same to ensure every platform can use at least one of them.
};
DECLARE_ENUM_AS_BIT_SET(WmKeyMod)

/**
 * Tabs of the park management GUI.
 * @note These constants must be in sync with their #ParkManagementWidgets counterparts.
 */
enum ParkManagementGuiTabs {
	PARK_MANAGEMENT_TAB_GENERAL = 0,  ///< General settings tab button.
	PARK_MANAGEMENT_TAB_GUESTS,       ///< Guests graph tab button.
	PARK_MANAGEMENT_TAB_RATING,       ///< Park rating graph tab button.
	PARK_MANAGEMENT_TAB_OBJECTIVE,    ///< Objective tab button.
	PARK_MANAGEMENT_TAB_AWARDS,       ///< Awards tab button.
};

/** All keyboard shortcuts. */
enum KeyboardShortcut {
	KS_BEGIN = 0,       ///< First shortcut ID.
	KS_FPS = KS_BEGIN,  ///< Toggle FPS counter.

	KS_MAINMENU_NEW,            ///< Main menu start new game.
	KS_MAINMENU_LOAD,           ///< Main menu load savegame.
	KS_MAINMENU_LAUNCH_EDITOR,  ///< Main menu launch scenario editor.
	KS_MAINMENU_SETTINGS,       ///< Main menu settings window.
	KS_MAINMENU_QUIT,           ///< Main menu quit FreeRCT.

	KS_INGAME_QUIT,      ///< Quit FreeRCT.
	KS_INGAME_SAVE,      ///< Save the game.
	KS_INGAME_LOAD,      ///< Load a game in-game.
	KS_INGAME_MAINMENU,  ///< Return to the main menu.
	KS_INGAME_SETTINGS,  ///< Open the settings window.

	KS_INGAME_SPEED_PAUSE,  ///< Set speed to paused.
	KS_INGAME_SPEED_1,      ///< Set speed to 1x.
	KS_INGAME_SPEED_2,      ///< Set speed to 2x.
	KS_INGAME_SPEED_4,      ///< Set speed to 4x.
	KS_INGAME_SPEED_8,      ///< Set speed to 8x.
	KS_INGAME_SPEED_UP,     ///< Set speed one level faster.
	KS_INGAME_SPEED_DOWN,   ///< Set speed one level slower.

	KS_INGAME_ROTATE_CW,   ///< Rotate view clockwise.
	KS_INGAME_ROTATE_CCW,  ///< Rotate view counter-clockwise.

	KS_INGAME_ZOOM_OUT,    ///< Zoom out.
	KS_INGAME_ZOOM_IN,     ///< Zoom in.

	KS_INGAME_MOVE_LEFT,   ///< Move the viewport to the left.
	KS_INGAME_MOVE_RIGHT,  ///< Move the viewport to the right.
	KS_INGAME_MOVE_UP,     ///< Move the viewport up.
	KS_INGAME_MOVE_DOWN,   ///< Move the viewport down.

	KS_INGAME_TERRAFORM,        ///< Open terraform window.
	KS_INGAME_PATHS,            ///< Open paths window.
	KS_INGAME_FENCES,           ///< Open fences window.
	KS_INGAME_SCENERY,          ///< Open scenery window.
	KS_INGAME_PATH_OBJECTS,     ///< Open path objects window.
	KS_INGAME_RIDES,            ///< Open ride select window.
	KS_INGAME_PARK_MANAGEMENT,  ///< Open park management window.
	KS_INGAME_STAFF,            ///< Open staff window.
	KS_INGAME_INBOX,            ///< Open inbox window.
	KS_INGAME_FINANCES,         ///< Open finances window.

	KS_INGAME_MINIMAP,           ///< Toggle the minimap.
	KS_INGAME_GRID,              ///< Toggle the grid.
	KS_INGAME_UNDERGROUND,       ///< Toggle underground view.
	KS_INGAME_UNDERWATER,        ///< Toggle underwater view.
	KS_INGAME_WIRE_RIDES,        ///< Toggle wireframe mode for rides.
	KS_INGAME_WIRE_SCENERY,      ///< Toggle wireframe mode for scenery.
	KS_INGAME_HIDE_PEOPLE,       ///< Toggle whether people are hidden.
	KS_INGAME_HIDE_SUPPORTS,     ///< Toggle whether supports are hidden.
	KS_INGAME_HIDE_SURFACES,     ///< Toggle whether surfaces are hidden.
	KS_INGAME_HIDE_FOUNDATIONS,  ///< Toggle whether foundations are hidden.
	KS_INGAME_HEIGHT_RIDES,      ///< Toggle height markers on rides.
	KS_INGAME_HEIGHT_PATHS,      ///< Toggle height markers on paths.
	KS_INGAME_HEIGHT_TERRAIN,    ///< Toggle height markers on terrain.

	KS_COUNT  ///< Number of keyboard shortcuts.
};
DECLARE_POSTFIX_INCREMENT(KeyboardShortcut)

/**
 * Get the key code for the fn-th function key.
 * @param fn Index of the key.
 * @return The key code.
 */
inline WmKeyCode FunctionKeyCode(int fn)
{
	assert(fn >= 1 && fn <= WMKC_FN_LAST - WMKC_FN_BEGIN);
	return static_cast<WmKeyCode>(WMKC_FN_BEGIN + fn);
}

typedef uint32 WindowNumber; ///< Type of a window number.

static const WindowNumber ALL_WINDOWS_OF_TYPE = UINT32_MAX; ///< Window number parameter meaning 'all windows of the window type'.

#endif
