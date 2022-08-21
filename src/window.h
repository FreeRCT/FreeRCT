/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling data structures. */

#ifndef WINDOW_H
#define WINDOW_H

#include "window_constants.h"
#include "geometry.h"
#include "orientation.h"
#include "widget.h"

#include <functional>
#include <vector>

class Viewport;
class RideType;
class Person;
class Message;
class MouseModeSelector;

/** An item in a dropdown list. */
class DropdownItem {
public:
	explicit DropdownItem(StringID strid);

	const std::string str;  ///< Item, as a string.
};

/** A dropdown list is a collection of #DropdownItem items. */
typedef std::vector<DropdownItem> DropdownList;

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
	virtual bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol);

	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);
	virtual void OnChange(ChangeCode code, uint32 parameter);
	virtual void ResetSize();

	virtual BaseWidget *FindTooltipWidget(Point16 pt);
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
	virtual void OnMouseWheelEvent(int direction) override;
	virtual bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;
	virtual void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos);
	virtual void SelectorMouseButtonEvent(uint8 state);
	virtual void SelectorMouseWheelEvent(int direction);
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

	BaseWidget *FindTooltipWidget(Point16 pt) override;

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

	bool closeable;  ///< This window can be closed by the user.

private:
	std::unique_ptr<BaseWidget> tree;     ///< Tree of widgets.
	std::unique_ptr<BaseWidget*[]>widgets; ///< Array of widgets with a non-negative index (use #GetWidget to get the widgets from this array).
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
	bool KeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol);
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
	BaseWidget *tooltip_widget;   ///< The widget for which we are currently showing a tooltip.

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
Window *HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber);
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter);

class RideInstance;
class CoasterInstance;

void ShowMainMenu();
void ShowMainDisplay(const XYZPoint32 &view_pos);
void ShowToolbar();
void ShowBottomToolbar();
void ShowSaveGameGui();
void ShowLoadGameGui();
void ShowQuitProgram(bool back_to_main_menu);
void ShowPersonInfoGui(const Person *person);
void ShowStaffManagementGui();
void ShowPathBuildGui();
void ShowRideSelectGui();
bool ShowRideManagementGui(uint16 ride);
void ShowShopManagementGui(uint16 ri);
void ShowGentleThrillRideManagementGui(uint16 ri);
void ShowFenceGui();
void ShowSceneryGui();
void ShowPathObjectsGui();
void ShowTerraformGui();
void ShowFinancesGui();
void ShowParkManagementGui();
void ShowCoasterManagementGui(RideInstance *coaster);
void ShowCoasterBuildGui(CoasterInstance *coaster);
void ShowRideBuildGui(RideInstance *instance);
void ShowSettingGui();
void ShowInboxGui();
void ShowMinimap();
void DrawMessage(const Message *msg, const Rectangle32 &rect, bool narrow);

static const uint32 DEFAULT_ERROR_MESSAGE_TIMEOUT = 8000;   ///< Number of ticks after which an error message auto-closes by default.
void ShowErrorMessage(StringID str1, StringID str2, const std::function<void()> &string_params, uint32 timeout = DEFAULT_ERROR_MESSAGE_TIMEOUT);
void ShowCostOrReturnEstimate(const Money &cost);

#endif
