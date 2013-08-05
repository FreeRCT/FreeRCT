/* $Id$ */

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

class Viewport;
class ShopType;
class Person;

/**
 * Available types of windows.
 * @ingroup window_group
 */
enum WindowTypes {
	WC_MAINDISPLAY,    ///< Main display of the world.
	WC_TOOLBAR,        ///< Main toolbar.
	WC_BOTTOM_TOOLBAR, ///< Bottom toolbar.
	WC_QUIT,           ///< Quit program window.
	WC_ERROR_MESSAGE,  ///< Error message window.
	WC_GUEST_INFO,     ///< Person window.
	WC_COASTER_MANAGER, ///< Roller coaster manager window.
	WC_PATH_BUILDER,   ///< %Path build gui.
	WC_RIDE_SELECT,    ///< Ride selection window.
	WC_SHOP_MANAGER,   ///< Management window of a shop.
	WC_TERRAFORM,      ///< Terraform window.
	WC_FINANCES,       ///< Finance management window.

	WC_NONE,           ///< Invalid window type.
};

/** Codes of the #NotifyChange function, which gets forwarded through the #Window::OnChange method. */
enum ChangeCode {
	CHG_UPDATE_BUTTONS,   ///< Recompute the state of the buttons.
	CHG_VIEWPORT_ROTATED, ///< Viewport rotated.
	CHG_MOUSE_MODE_LOST,  ///< Lost the mouse mode.
	CHG_DISPLAY_OLD,      ///< Displayed data is old.
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
	virtual Point32 OnInitialPosition();

	void MarkDirty();

	virtual void OnDraw();
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();

	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);
	virtual void OnChange(ChangeCode code, uint32 parameter);
};

/**
 * Base class for windows with a widget tree.
 * @ingroup window_group
 */
class GuiWindow : public Window {
public:
	GuiWindow(WindowTypes wtype, WindowNumber wnumber);
	virtual ~GuiWindow();
	virtual void OnDraw();

	virtual void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);
	virtual void SetWidgetStringParameters(WidgetNumber wid_num) const;
	virtual void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const;
	virtual void SetSize(uint width, uint height);
	StringID TranslateStringNumber(StringID str_id) const;

	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseLeaveEvent();
	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);

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

	inline void MarkWidgetDirty(WidgetNumber wnum);

protected:
	Point16 mouse_pos;    ///< Mouse position relative to the window (negative coordinates means 'out of window').
	const ShopType *ride_type; ///< Ride type being used by this window, for translating its strings. May be \c NULL.

	void SetupWidgetTree(const WidgetPart *parts, int length);

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

	virtual void OnClick(WidgetNumber widget);

	void SetShopType(const ShopType *ride_type);

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
	void RemoveFromStack(Window* w);
	void DeleteWindow(Window *w);

	void CloseAllWindows();

	void MouseMoveEvent(const Point16 &pos);
	void MouseButtonEvent(MouseButtons button, bool pressed);
	void MouseWheelEvent(int direction);
	void Tick();

	Point16 GetMousePosition() const;

	Window *top;        ///< Top-most window in the window stack.
	Window *bottom;     ///< Lowest window in the window stack.

private:
	Window *FindWindowByPosition(const Point16 &pos) const;
	void UpdateCurrentWindow();

	void StartWindowMove();

	Point16 mouse_pos;      ///< Last reported mouse position.
	Window *current_window; ///< 'Current' window under the mouse.
	uint8 mouse_state;      ///< Last reported mouse button state (lower 4 bits).
	uint8 mouse_mode;       ///< Mouse mode of the window manager. @see WmMouseModes

	Point16 move_offset;    ///< Offset from the top-left of the #current_window being moved in #WMMM_MOVE_WINDOW mode to the mouse position.
};

extern WindowManager _manager;

bool IsLeftClick(uint8 state);

void UpdateWindows();
Window *GetWindowByType(WindowTypes wtype, WindowNumber wnumber);
bool HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber);
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter);

class RideInstance;

Viewport *ShowMainDisplay();
void ShowToolbar();
void ShowBottomToolbar();
void ShowGuestInfoGui(const Person *person);
void ShowPathBuildGui();
void ShowRideSelectGui();
void ShowShopManagementGui(uint16 ri);
void ShowTerraformGui();
void ShowFinancesGui();
void ShowCoasterManagementGui(RideInstance *coaster);
void ShowErrorMessage(StringID strid);

#endif
