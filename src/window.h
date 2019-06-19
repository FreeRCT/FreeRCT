/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling data structures. */

#ifndef WINDOW_H
#define WINDOW_H

#include "geometry.h"
#include "orientation.h"
#include "widget.h"

#include <vector>

class Viewport;
class RideType;
class Person;
class MouseModeSelector;

/** An item in a dropdown list. */
class DropdownItem {
public:
	DropdownItem(StringID strid);

	uint8 str[128]; ///< Item, as a string (arbitary length).
};

/** A dropdown list is a collection of #DropdownItem items. */
typedef std::vector<DropdownItem> DropdownList;

/**
 * Available types of windows.
 * @ingroup window_group
 */
enum WindowTypes {
	WC_MAINDISPLAY,     ///< Main display of the world.
	WC_TOOLBAR,         ///< Main toolbar.
	WC_BOTTOM_TOOLBAR,  ///< Bottom toolbar.
	WC_QUIT,            ///< Quit program window.
	WC_ERROR_MESSAGE,   ///< Error message window.
	WC_GUEST_INFO,      ///< Person window.
	WC_COASTER_MANAGER, ///< Roller coaster manager window.
	WC_COASTER_BUILD,   ///< Roller coaster build/edit window.
	WC_COASTER_REMOVE,  ///< Roller coaster remove window.
	WC_RIDE_BUILD,      ///< Simple ride build window.
	WC_PATH_BUILDER,    ///< %Path build GUI.
	WC_RIDE_SELECT,     ///< Ride selection window.
	WC_SHOP_MANAGER,    ///< Management window of a shop.
	WC_SHOP_REMOVE,     ///< Shop remove window.
	WC_FENCE,           ///< Fence window.
	WC_TERRAFORM,       ///< Terraform window.
	WC_FINANCES,        ///< Finance management window.
	WC_SETTING,         ///< Setting window.
	WC_DROPDOWN,        ///< Dropdown window.
	WC_EDIT_TEXT,       ///< Edit text window.

	WC_NONE,            ///< Invalid window type.
};

/** Codes of the #NotifyChange function, which gets forwarded through the #Window::OnChange method. */
enum ChangeCode {
	CHG_UPDATE_BUTTONS,   ///< Recompute the state of the buttons.
	CHG_VIEWPORT_ROTATED, ///< Viewport rotated.
	CHG_DISPLAY_OLD,      ///< Displayed data is old.
	CHG_PIECE_POSITIONED, ///< The track piece is at the correct position.
	CHG_DROPDOWN_RESULT,  ///< The selection of a dropdown window.
	CHG_GUEST_COUNT,      ///< Number of guests in the park has changed.
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
	MB_LEFT   = 1, ///< Left button down.
	MB_MIDDLE = 2, ///< Middle button down.
	MB_RIGHT  = 4, ///< Right button down.

	MB_CURRENT    = 0x07, ///< Bitmask for current mouse state.
	MB_PREVIOUS   = 0x70, ///< Bitmask for previous mouse state.
	MB_PREV_SHIFT = 4,    ///< Amount of shifting to get previous mouse state.
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
 * Key codes of the window manager.
 * @ingroup window_group
 */
enum WmKeyCode {
	WMKC_CURSOR_UP,    ///< Up arrow key is pressed.
	WMKC_CURSOR_LEFT,  ///< Left arrow key is pressed.
	WMKC_CURSOR_RIGHT, ///< Right arrow key is pressed.
	WMKC_CURSOR_DOWN,  ///< Down arrow key is pressed.
	WMKC_BACKSPACE,    ///< Backspace is pressed.
	WMKC_DELETE,       ///< Delete is pressed.
	WMKC_CANCEL,       ///< Cancel is pressed.
	WMKC_CONFIRM,      ///< Confirm is pressed.
	WMKC_SPACE,        ///< Space is pressed.
	WMKC_SYMBOL,       ///< A symbol is entered.
};

/**
 * Available mouse modes of the window manager.
 * @ingroup window_group
 */
enum WmMouseModes {
	WMMM_PASS_THROUGH, ///< No special mode, pass events on to the windows.
	WMMM_MOVE_WINDOW,  ///< Move the current window.
};

typedef uint32 WindowNumber; ///< Type of a window number.

static const WindowNumber ALL_WINDOWS_OF_TYPE = UINT32_MAX; ///< Window number parameter meaning 'all windows of the window type'.

/**
 * %Window base class.
 * @ingroup window_group
 */
class Window {
public:
	Window(WindowTypes wtype, WindowNumber wnumber);
	virtual ~Window();

	Rectangle32 rect;           ///< Screen area covered by the window.
	const WindowTypes wtype;    ///< %Window type.
	const WindowNumber wnumber; ///< %Window number.

	/**
	 * Timeout counter.
	 * Decremented on each iteration. When it reaches 0, #TimeoutCallback is called.
	 */
	uint8 timeout;
	uint8 flags;    ///< %Window flags. @see WindowFlags
	Window *higher; ///< %Window above this window (managed by #WindowManager).
	Window *lower;  ///< %Window below this window (managed by #WindowManager).

	virtual void SetSize(uint width, uint height);
	void SetPosition(int x, int y);
	void SetPosition(Point32 pos);
	virtual Point32 OnInitialPosition();

	void MarkDirty();

	virtual void OnDraw(MouseModeSelector *selector);
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();
	virtual bool OnKeyEvent(WmKeyCode key_code, const uint8 *symbol);

	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);
	virtual void OnChange(ChangeCode code, uint32 parameter);
	virtual void ResetSize();
};

/**
 * Base class for windows with a widget tree.
 * @ingroup window_group
 */
class GuiWindow : public Window {
public:
	GuiWindow(WindowTypes wtype, WindowNumber wnumber);
	virtual ~GuiWindow();
	virtual void OnDraw(MouseModeSelector *selector) override;

	virtual void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);
	virtual void SetWidgetStringParameters(WidgetNumber wid_num) const;
	virtual void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const;
	virtual void SetSize(uint width, uint height) override;
	StringID TranslateStringNumber(StringID str_id) const;
	virtual void ResetSize() override;

	virtual void OnMouseMoveEvent(const Point16 &pos) override;
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state) override;
	virtual void OnMouseLeaveEvent() override;
	virtual void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos);
	virtual void SelectorMouseButtonEvent(uint8 state);
	virtual void SelectorMouseWheelEvent(int direction);
	virtual bool OnKeyEvent(WmKeyCode key_code, const uint8 *symbol) override;
	virtual void TimeoutCallback() override;
	virtual void SetHighlight(bool value) override;

	/**
	 * Get the horizontal position of the top-left corner of a widget (of this window) at the screen.
	 * @param wid %Widget to use.
	 * @return Horizontal position of the top-left corner of the widget at the screen.
	 */
	int GetWidgetScreenX(const BaseWidget *wid) const
	{
		return this->rect.base.x + wid->pos.base.x;
	}

	/**
	 * Get the vertical position of the top-left corner of a widget (of this window) at the screen.
	 * @param wid %Widget to use.
	 * @return Vertical position of the top-left corner of the widget at the screen.
	 */
	int GetWidgetScreenY(const BaseWidget *wid) const
	{
		return this->rect.base.y + wid->pos.base.y;
	}

	inline void SetSelector(MouseModeSelector *selector);
	inline void MarkWidgetDirty(WidgetNumber wnum);

	bool initialized;            ///< Flag telling widgets whether the window has already been initialized.
	MouseModeSelector *selector; ///< Currently active selector of this window. May be \c nullptr. Change through #SetSelector.

protected:
	Point16 mouse_pos;         ///< Mouse position relative to the window (negative coordinates means 'out of window').
	const RideType *ride_type; ///< Ride type being used by this window, for translating its strings. May be \c nullptr.

	void SetupWidgetTree(const WidgetPart *parts, int length);
	void SetScrolledWidget(WidgetNumber scrolled, WidgetNumber scrollbar);

	template <typename WID>
	inline WID *GetWidget(WidgetNumber wnum);

	template <typename WID>
	inline const WID *GetWidget(WidgetNumber wnum) const;

	void SetWidgetChecked(WidgetNumber widget, bool value);
	bool IsWidgetChecked(WidgetNumber widget) const;

	void SetWidgetPressed(WidgetNumber widget, bool value);
	bool IsWidgetPressed(WidgetNumber widget) const;

	void SetWidgetShaded(WidgetNumber widget, bool value);
	bool IsWidgetShaded(WidgetNumber widget) const;

	void SetRadioButtonsSelected(const WidgetNumber *wids, WidgetNumber selected);
	WidgetNumber GetSelectedRadioButton(const WidgetNumber *wids);

	virtual void OnClick(WidgetNumber widget, const Point16 &pos);

	void SetRideType(const RideType *ride_type);

	/* In dropdown.cpp */
	void ShowDropdownMenu(WidgetNumber widnum, const DropdownList &items, int selected_index, ColourRange colour = COL_RANGE_INVALID);
	void ShowRecolourDropdown(WidgetNumber widnum, RecolourEntry *entry, ColourRange colour = COL_RANGE_INVALID);

private:
	BaseWidget *tree;     ///< Tree of widgets.
	BaseWidget **widgets; ///< Array of widgets with a non-negative index (use #GetWidget to get the widgets from this array).
	uint16 num_widgets;   ///< Number of widgets in #widgets.
};

/**
 * Get the widget.
 * @tparam WID %Widget class.
 * @param wnum %Widget number to get.
 * @return Address of the widget.
 */
template <typename WID>
inline WID *GuiWindow::GetWidget(WidgetNumber wnum)
{
	assert(wnum < this->num_widgets);
	return dynamic_cast<WID *>(this->widgets[wnum]);
}

/**
 * Get the widget.
 * @tparam WID %Widget class.
 * @param wnum %Widget number to get.
 * @return Address of the widget.
 */
template <typename WID>
inline const WID *GuiWindow::GetWidget(WidgetNumber wnum) const
{
	assert(wnum < this->num_widgets);
	return dynamic_cast<WID *>(this->widgets[wnum]);
}

/**
 * Specialized template for #BaseWidget
 * @param wnum %Widget number to get.
 * @return Address of the base widget.
 */
template <>
inline BaseWidget *GuiWindow::GetWidget(WidgetNumber wnum)
{
	assert(wnum < this->num_widgets);
	return this->widgets[wnum];
}

/**
 * Specialized template for #BaseWidget
 * @param wnum %Widget number to get.
 * @return Address of the base widget.
 */
template <>
inline const BaseWidget *GuiWindow::GetWidget(WidgetNumber wnum) const
{
	assert(wnum < this->num_widgets);
	return this->widgets[wnum];
}

/**
 * Mark the specified widget as dirty (in need of repainting).
 * @param wnum %Widget to use.
 */
inline void GuiWindow::MarkWidgetDirty(WidgetNumber wnum)
{
	BaseWidget *bw = this->GetWidget<BaseWidget>(wnum);
	bw->MarkDirty(this->rect.base);
}

/**
 * %Window manager class, manages the window stack.
 * @ingroup window_group
 */
class WindowManager {
public:
	WindowManager();
	~WindowManager();

	bool HasWindow(Window *w);
	void AddToStack(Window *w);
	void RemoveFromStack(Window *w);
	void RaiseWindow(Window *w);
	void SetSelector(GuiWindow *w, MouseModeSelector *selector);

	void CloseAllWindows();
	void ResetAllWindows();
	void RepositionAllWindows(uint new_width, uint new_height);

	void MouseMoveEvent(const Point16 &pos);
	void MouseButtonEvent(MouseButtons button, bool pressed);
	void MouseWheelEvent(int direction);
	bool KeyEvent(WmKeyCode key_code, const uint8 *symbol);
	void Tick();

	/**
	 * Mouse moved in the viewport. Forward the call to the selector window.
	 * @param vp %Viewport where the mouse moved.
	 * @param pos New position of the mouse in the viewport.
	 * @return Call could be forwarded.
	 */
	inline bool SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos)
	{
		GuiWindow *gw = this->GetSelector();
		if (gw != nullptr) {
			gw->SelectorMouseMoveEvent(vp, pos);
			return true;
		}
		return false;
	}

	/**
	 * Mouse button changed in the viewport. Forward the call to the selector window.
	 * @param state Previous and current state of the mouse buttons. @see MouseButtons
	 * @return Call could be forwarded.
	 */
	inline bool SelectorMouseButtonEvent(uint8 state)
	{
		GuiWindow *gw = this->GetSelector();
		if (gw != nullptr) {
			gw->SelectorMouseButtonEvent(state);
			return true;
		}
		return false;
	}

	/**
	 * Mouse wheel turned in the viewport. Forward the call to the selector window.
	 * @param direction Direction of turning (-1 or +1).
	 * @return Call could be forwarded.
	 */
	inline bool SelectorMouseWheelEvent(int direction)
	{
		GuiWindow *gw = this->GetSelector();
		if (gw != nullptr) {
			gw->SelectorMouseWheelEvent(direction);
			return true;
		}
		return false;
	}

	/**
	 * Get last reported position of the mouse cursor (at the screen).
	 * @return Last known position of the mouse at the screen.
	 */
	inline Point16 GetMousePosition() const
	{
		return this->mouse_pos;
	}

	/**
	 * Get last reported state of the buttons (lower 4 bits).
	 * @return Last reported state of the mouse buttons.
	 * @see MouseButtons
	 */
	inline uint8 GetMouseState() const
	{
		return this->mouse_state;
	}

	/**
	 * Retrieve the main world diaplay window.
	 * @return The main display window, or \c nullptr if not available.
	 */
	inline Viewport *GetViewport() const
	{
		return this->viewport;
	}

	void UpdateWindows();

	Window *top;    ///< Top-most window in the window stack.
	Window *bottom; ///< Lowest window in the window stack.

private:
	Window *FindWindowByPosition(const Point16 &pos) const;
	void UpdateCurrentWindow();
	GuiWindow *GetSelector();

	void StartWindowMove();

	Point16 mouse_pos;        ///< Last reported mouse position.
	Window *current_window;   ///< 'Current' window under the mouse.
	GuiWindow *select_window; ///< Cache containing the highest window with active GuiWindow::selector field
	                          ///< (\c nullptr if no such window exists). Only valid if #select_valid holds.
	Viewport *viewport;       ///< Viewport window (\c nullptr if not available).
	bool select_valid;        ///< State of the #select_window cache.
	uint8 mouse_state;        ///< Last reported mouse button state (lower 4 bits).
	uint8 mouse_mode;         ///< Mouse mode of the window manager. @see WmMouseModes

	Point16 move_offset;      ///< Offset from the top-left of the #current_window being moved in #WMMM_MOVE_WINDOW mode to the mouse position.
};

extern WindowManager _window_manager;

/**
 * Set a new selector for the window.
 * @param selector Selector to set. May be \c nullptr to deselect a selector.
 */
inline void GuiWindow::SetSelector(MouseModeSelector *selector)
{
	_window_manager.SetSelector(this, selector);
}

bool IsLeftClick(uint8 state);

Window *GetWindowByType(WindowTypes wtype, WindowNumber wnumber);
bool HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber);
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter);

class RideInstance;
class CoasterInstance;

void ShowMainDisplay(const XYZPoint32 &view_pos);
void ShowToolbar();
void ShowBottomToolbar();
void ShowGuestInfoGui(const Person *person);
void ShowPathBuildGui();
void ShowRideSelectGui();
void ShowShopManagementGui(uint16 ri);
void ShowFenceGui();
void ShowTerraformGui();
void ShowFinancesGui();
void ShowCoasterManagementGui(RideInstance *coaster);
void ShowCoasterBuildGui(CoasterInstance *coaster);
void ShowRideBuildGui(RideInstance *instance);
void ShowErrorMessage(StringID strid);
void ShowSettingGui();
void ShowEditTextGui(uint8 *text);

#endif
