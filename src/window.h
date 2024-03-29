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

#include <array>
#include <functional>
#include <optional>
#include <vector>

class ConfigFile;
class Viewport;
class RideType;
class Person;
class Message;
class MouseModeSelector;

/** Information about a dropdown item. */
enum DropdownItemFlags {
	DDIF_NONE = 0,        ///< Nothing special about this item.
	DDIF_SELECTABLE = 1,  ///< The item can be selected like a checkbox.
	DDIF_SELECTED = 2,    ///< The item is currently selected. Only valid in combination with #DDIF_SELECTABLE.
	DDIF_DISABLED = 4,    ///< This item can not be clicked by the user.
};
DECLARE_ENUM_AS_BIT_SET(DropdownItemFlags)

/** An item in a dropdown list. */
class DropdownItem {
public:
	explicit DropdownItem(StringID strid, DropdownItemFlags flags = DDIF_NONE);

	const std::string str;    ///< Item, as a string.
	DropdownItemFlags flags;  ///< Properties of the item.
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

	/**
	 * Get the current mouse position relative to this window's top-left corner.
	 * @return The relative mouse X coordinate.
	 */
	inline float GetRelativeMouseX() const
	{
		return _video.MouseX() - this->rect.base.x;
	}

	/**
	 * Get the current mouse position relative to this window's top-left corner.
	 * @return The relative mouse Y coordinate.
	 */
	inline float GetRelativeMouseY() const
	{
		return _video.MouseY() - this->rect.base.y;
	}

	virtual void SetSize(uint width, uint height);
	void SetPosition(int x, int y);
	void SetPosition(Point32 pos);
	virtual Point32 OnInitialPosition();

	virtual void OnDraw(MouseModeSelector *selector);
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(MouseButtons state, WmMouseEventMode mode);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();
	virtual bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol);

	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);
	virtual void OnChange(ChangeCode code, uint32 parameter);
	virtual void ResetSize();

	virtual BaseWidget *FindTooltipWidget(Point16 pt);
	virtual void SetTooltipStringParameters(BaseWidget *tooltip_widget) const;
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

	virtual WmMouseEvent OnMouseButtonEvent(MouseButtons state, WmMouseEventMode mode) override;
	virtual void OnMouseWheelEvent(int direction) override;
	virtual bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;
	virtual void SelectorMouseMoveEvent(Viewport *vp, const Point16 &pos);
	virtual void SelectorMouseButtonEvent(MouseButtons state);
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

	bool initialized;            ///< Flag telling widgets whether the window has already been initialized.
	MouseModeSelector *selector; ///< Currently active selector of this window. May be \c nullptr. Change through #SetSelector.

	BaseWidget *FindTooltipWidget(Point16 pt) override;

protected:
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

	void SetWidgetCheckedAndPressed(WidgetNumber widget, bool value);

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
 * %Window manager class, manages the window stack.
 * @ingroup window_group
 */
class WindowManager {
public:
	WindowManager();

	bool HasWindow(Window *w);
	void AddToStack(Window *w);
	void RemoveFromStack(Window *w);
	void PreDelete(Window *w);
	void RaiseWindow(Window *w);
	void SetSelector(GuiWindow *w, MouseModeSelector *selector);

	void CloseAllWindows();
	void ResetAllWindows();
	void RepositionAllWindows(uint new_width, uint new_height);

	void MouseMoveEvent();
	void MouseButtonEvent(MouseButtons button, WmMouseEventMode mode);
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
	inline bool SelectorMouseButtonEvent(MouseButtons state)
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

	Window *current_window;   ///< 'Current' window under the mouse.
	GuiWindow *select_window; ///< Cache containing the highest window with active GuiWindow::selector field
	                          ///< (\c nullptr if no such window exists). Only valid if #select_valid holds.
	Viewport *viewport;       ///< Viewport window (\c nullptr if not available).
	bool select_valid;        ///< State of the #select_window cache.

	Point16 move_offset;      ///< Offset from the top-left of the #current_window being moved in #WMMM_MOVE_WINDOW mode to the mouse position.

	MouseButtons held_mouse_buttons;   ///< Mouse buttons currently held down.
	int mouse_buttons_repeat_counter;  ///< Ticks since last repeated mouse button event.
};

extern WindowManager _window_manager;

/** Class that assigns all keyboard shortcuts in the game. */
class Shortcuts {
public:
	Shortcuts();

	/** The scope in which a shortcut is valid. */
	enum class Scope {
		NONE,       ///< Never valid.
		GLOBAL,     ///< Always valid.
		INGAME,     ///< Valid during a game.
		MAIN_MENU,  ///< Valid in the main menu.
	};

	/** A keystroke assigned to an abstract shortcut. */
	struct Keybinding {
		/** Constructor for a textual binding. */
		explicit Keybinding(std::string s, WmKeyMod m = WMKM_NONE) : key(WMKC_SYMBOL), mod(m), symbol(s) {}
		/** Constructor for a non-textual binding. */
		explicit Keybinding(WmKeyCode k, WmKeyMod m = WMKM_NONE) : key(k), mod(m) {}
		/** Constructor for an empty, invalid shortcut. */
		Keybinding() : key(WMKC_SYMBOL), mod(WMKM_NONE) {}

		/**
		 * Check whether this keybinding represents an actual keystroke.
		 * @return The binding is valid.
		 */
		bool Valid() const
		{
			return this->key != WMKC_SYMBOL || !this->symbol.empty();
		}

		WmKeyCode key;       ///< The key to press.
		WmKeyMod  mod;       ///< The modifiers to press.
		std::string symbol;  ///< If #key is #WMKC_SYMBOL, the key text.
	};

	/** Data associated with an abstract shortcut. */
	struct ShortcutInfo {
		/** Constructor. */
		ShortcutInfo() : scope(Scope::NONE) {}
		ShortcutInfo(std::string n, Keybinding k, Scope s) : default_binding(k), current_binding(k), config_name(n), scope(s) {}

		/**
		 * Check whether this shortcut has been initialized.
		 * @return The shortcit is valid.
		 */
		bool Valid() const
		{
			return this->scope != Scope::NONE && !this->config_name.empty() && this->default_binding.Valid() && this->current_binding.Valid();
		}

		Keybinding default_binding;  ///< The default keybinding.
		Keybinding current_binding;  ///< The currently assigned keybinding.
		std::string config_name;     ///< The shortcut's name in the config file.
		Scope scope;                 ///< In which scope the shortcut is valid.
	};

	/**
	 * Look up the keybinding configured for a specific shortcut.
	 * @param ks Shortcut to look up.
	 * @return The keybinding.
	 */
	const Keybinding &GetShortcut(KeyboardShortcut ks) const
	{
		return this->values[ks].current_binding;
	}

	std::optional<KeyboardShortcut> GetShortcut(Keybinding binding, Scope scope) const;

	/**
	 * Change the keybinding configured for a specific shortcut.
	 * @param ks Shortcut to modify.
	 * @param binding The new keybinding.
	 */
	void SetShortcut(KeyboardShortcut ks, Keybinding binding)
	{
		this->values[ks].current_binding = binding;
	}

	void ReadConfig(ConfigFile &cfg_file);

	std::array<ShortcutInfo, KS_COUNT> values;  ///< All configured keyboard shortcuts.
};
extern Shortcuts _shortcuts;

/**
 * Set a new selector for the window.
 * @param selector Selector to set. May be \c nullptr to deselect a selector.
 */
inline void GuiWindow::SetSelector(MouseModeSelector *selector)
{
	_window_manager.SetSelector(this, selector);
}

Window *GetWindowByType(WindowTypes wtype, WindowNumber wnumber);
Window *HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber);
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter);
void NotifyChange(ChangeCode code, uint32 parameter);

class RideInstance;
class FixedRideInstance;
class CoasterInstance;

void ShowMainMenu();
void ShowMainDisplay(const XYZPoint32 &view_pos);
void ShowToolbar();
void ShowBottomToolbar();
void ShowConfirmationPrompt(StringID title, StringID message, std::function<void()> callback);
void ShowSaveGameGui();
void ShowLoadGameGui();
void ShowScenarioSelectGui();
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
void ShowParkManagementGui(ParkManagementGuiTabs tab);
void ShowCoasterManagementGui(RideInstance *coaster);
void ShowCoasterBuildGui(CoasterInstance *coaster, int16 design = -1);
void ShowRideBuildGui(FixedRideInstance *instance);
void ShowSettingGui();
void ShowInboxGui();
void ShowMinimap();
void DrawMessage(const Message *msg, const Rectangle32 &rect, bool narrow, float obscure_fraction = 0.f);

static const uint32 DEFAULT_ERROR_MESSAGE_TIMEOUT = 8000;   ///< Number of ticks after which an error message auto-closes by default.
void ShowErrorMessage(StringID str1, StringID str2, const std::function<void()> &string_params, uint32 timeout = DEFAULT_ERROR_MESSAGE_TIMEOUT);
void ShowCostOrReturnEstimate(const Money &cost);

#endif
