/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file window.cpp %Window handling functions.
 * @todo [low] Consider the case where we don't have a video system.
 */

/**
 * @defgroup window_group %Window code
 */

/**
 * @defgroup gui_group Actual windows
 * @ingroup window_group
 */

#include "stdafx.h"
#include "window.h"
#include "video.h"
#include "math_func.h"
#include "palette.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "viewport.h"
#include "mouse_mode.h"
#include <cmath>

/**
 * %Window manager.
 * @ingroup window_group
 */
WindowManager _window_manager;

static uint GetWindowZPriority(WindowTypes wt);

/**
 * %Window constructor. It automatically adds the window to the window stack.
 * @param wtype %Window type Type of window.
 * @param wnumber Number of the window within the \a wtype.
 */
Window::Window(WindowTypes wtype, WindowNumber wnumber)
: rect(0, 0, 0, 0), wtype(wtype), wnumber(wnumber), timeout(0), flags(0), higher(nullptr), lower(nullptr)
{
	_window_manager.AddToStack(this); // Add to window stack.
}

/** Destructor. */
Window::~Window()
{
	_window_manager.RemoveFromStack(this);
}

/**
 * Set the initial size of a window.
 * @param width Initial width of the new window.
 * @param height Initial height of the new window.
 */
void Window::SetSize(uint width, uint height)
{
	this->rect.width = width;
	this->rect.height = height;
}

/**
 * Set the position of the top-left corner of the window.
 * @param x X position.
 * @param y Y position.
 */
void Window::SetPosition(int x, int y)
{
	this->SetPosition({x, y});
}

/**
 * Set the position of the top-left corner of the window.
 * @param pos Point to set to.
 */
void Window::SetPosition(Point32 pos)
{
	this->rect.base = pos;
}

/** Compute the initial position of a window. */
class ComputeInitialPosition {
public:
	ComputeInitialPosition();

	Point32 FindPosition(Window *w);

	bool IsScreenEmpty(const Rectangle32 &rect);

protected:
	int base_pos; ///< Base position of a new window.
	Window *skip; ///< Window to skip in the search.

	static const int GAP; ///< Gap between the old and the new window.
};

const int ComputeInitialPosition::GAP = 5;

ComputeInitialPosition::ComputeInitialPosition()
{
	this->base_pos = 10;
	this->skip = nullptr;
}

/**
 * Get distance of a position to the mouse.
 * @param pt Queried point.
 * @return The distance.
 */
static inline int GetDistanceToMouse(const Point32 &pt)
{
	return hypot(_video.MouseX() - pt.x, _video.MouseY() - pt.y);
}

/**
 * Find an initial position for new window \a w.
 * @param new_w New window to position.
 * @return Best position of the window.
 * @pre Size of the window has been decided.
 */
Point32 ComputeInitialPosition::FindPosition(Window *new_w)
{
	static const Point16 test_positions[] = {
		{0, 1}, {0, 2}, {3, 1}, {3, 2},
		{1, 0}, {2, 0}, {1, 3}, {2, 3}
	};

	int32 x[4], y[4];

	this->skip = new_w;

	Point32 best(this->base_pos, this->base_pos);

	bool found_empty = false;
	for (Window *w = _window_manager.top; w != nullptr; w = w->lower) {
		if (w == new_w) continue;
		x[0] = w->rect.base.x - new_w->rect.width - GAP;
		x[1] = w->rect.base.x;
		x[2] = w->rect.base.x + w->rect.width - new_w->rect.width;
		x[3] = w->rect.base.x + w->rect.width + GAP;

		y[0] = w->rect.base.y - new_w->rect.height - GAP;
		y[1] = w->rect.base.y;
		y[2] = w->rect.base.y + w->rect.height - new_w->rect.height;
		y[3] = w->rect.base.y + w->rect.height + GAP;

		for (uint i = 0; i < lengthof(test_positions); i++) {
			Point32 pt(x[test_positions[i].x], y[test_positions[i].y]);
			if (this->IsScreenEmpty(Rectangle32(pt.x, pt.y, new_w->rect.width, new_w->rect.height))) {
				if (!found_empty || GetDistanceToMouse(best) > GetDistanceToMouse(pt)) {
					best = pt;
					found_empty = true;
				}
			}
		}
	}

	if (best.x == this->base_pos) {
		this->base_pos += 10;
		if (this->base_pos + 100 > _video.Height()) this->base_pos = 10;
	}

	return best;
}

/**
 * Is the screen empty below the rectangle?
 * @param rect Area to test.
 * @return Tested area is clear.
 */
bool ComputeInitialPosition::IsScreenEmpty(const Rectangle32 &rect)
{
	uint new_prio = GetWindowZPriority(this->skip->wtype);

	if (rect.base.x < 0 || rect.base.y < 0
			|| rect.base.x + rect.width  > _video.Width()
			|| rect.base.y + rect.height > _video.Height()) return false;

	for (Window *w = _window_manager.top; w != nullptr; w = w->lower) {
		if (w == this->skip || new_prio > GetWindowZPriority(w->wtype)) continue;
		if (w->rect.Intersects(rect)) return false;
	}
	return true;
}

/**
 * Find a nice initial position for the new window.
 * @return Initial position of the window.
 */
Point32 Window::OnInitialPosition()
{
	static ComputeInitialPosition compute_pos;

	return compute_pos.FindPosition(this);
}

/**
 * Paint the window to the screen.
 * @param selector Mouse mode selector to render.
 * @note The window manager already locked the surface.
 */
void Window::OnDraw([[maybe_unused]] MouseModeSelector *selector)
{
}

/**
 * Mouse moved to new position.
 * @param pos New position.
 */
void Window::OnMouseMoveEvent([[maybe_unused]] const Point16 &pos)
{
}

/**
 * Mouse buttons changed state.
 * @param state Updated state. @see MouseButtons
 * @return Action to perform as a result of the event (use #WMME_NONE if no special action needed).
 */
WmMouseEvent Window::OnMouseButtonEvent([[maybe_unused]] MouseButtons state)
{
	return WMME_NONE;
}

/**
 * Mousewheel rotated.
 * @param direction Direction of change (\c +1 or \c -1).
 */
void Window::OnMouseWheelEvent([[maybe_unused]] int direction)
{
}

/** Mouse entered window. */
void Window::OnMouseEnterEvent()
{
}

/** Mouse left window. */
void Window::OnMouseLeaveEvent()
{
}

/**
 * Process input from the keyboard.
 * @param key_code Kind of input.
 * @param mod Bitmask of pressed modifiers.
 * @param symbol Entered symbol, if \a key_code is #WMKC_SYMBOL. Utf-8 encoded.
 * @return Key event has been processed.
 */
bool Window::OnKeyEvent([[maybe_unused]] WmKeyCode key_code, [[maybe_unused]] WmKeyMod mod, [[maybe_unused]] const std::string &symbol)
{
	return false;
}

/**
 * Timeout callback.
 * Called when #timeout decremented to 0.
 */
void Window::TimeoutCallback()
{
}

/**
 * Enable or disable highlighting. Base class does nothing.
 * If enabled, the #timeout is used to automatically disable it again.
 * @param value New highlight value.
 */
void Window::SetHighlight([[maybe_unused]] bool value)
{
}

/**
 * An important (window-specific) change has happened.
 * @param code Unique identification of the change.
 * @param parameter Parameter of the \a code.
 * @note Meaning of number values are documented near this method in derived classes.
 */
void Window::OnChange([[maybe_unused]] ChangeCode code, [[maybe_unused]] uint32 parameter)
{
}

/**
 * Reset the size of a window.
 * @note Only if the window is a #GuiWindow.
 */
void Window::ResetSize()
{
}

/**
 * Find the widget for which a tooltip should be shown.
 * @param pt Mouse cursor position.
 * @return The widget (may be \c nullptr).
 */
BaseWidget *Window::FindTooltipWidget([[maybe_unused]] Point16 pt)
{
	return nullptr;
}

/**
 * Set string parameters of the tooltip string of the widget.
 * @param tooltip_widget The widget whose tooltip is being drawn.
 */
void Window::SetTooltipStringParameters([[maybe_unused]] BaseWidget *tooltip_widget) const
{
}

/**
 * Gui window constructor.
 * @param wtype %Window type (for finding a window in the stack).
 * @param wnumber Number of the window within the \a wtype.
 * @note Initialize the widget tree from the derived window class.
 */
GuiWindow::GuiWindow(WindowTypes wtype, WindowNumber wnumber) : Window(wtype, wnumber),
	initialized(false),
	selector(nullptr),
	ride_type(nullptr),
	closeable(true),
	tree(nullptr),
	widgets(nullptr),
	num_widgets(0)
{
	SetHighlight(true);
}

GuiWindow::~GuiWindow()
{
	/* The derived window should have released the selector before arriving here. */
	assert(this->selector == nullptr);
}

/**
 * Set the size of the window.
 * @param width Initial width of the new window.
 * @param height Initial height of the new window.
 */
void GuiWindow::SetSize([[maybe_unused]] uint width, [[maybe_unused]] uint height)
{
	// XXX Do nothing for now, in the future, this should cause a window resize.
}

/**
 * Set the string translation base for the generic strings of this window.
 * @param ride_type Ride type to use as base for the string translation.
 */
void GuiWindow::SetRideType(const RideType *ride_type)
{
	this->ride_type = ride_type;
}

/**
 * Translate the string number if necessary.
 * @param str_id String number to translate.
 * @return Translated string number.
 */
StringID GuiWindow::TranslateStringNumber(StringID str_id) const
{
	assert(str_id != STR_INVALID);

	if (this->ride_type != nullptr && str_id >= STR_GENERIC_SHOP_START) return this->ride_type->GetString(str_id);
	return str_id;
}

void GuiWindow::ResetSize()
{
	this->tree->SetupMinimalSize(this, this->widgets.get());
	this->rect = Rectangle32(this->rect.base.x, this->rect.base.y, this->tree->min_x, this->tree->min_y);

	Rectangle16 min_rect(0, 0, this->tree->min_x, this->tree->min_y);
	this->tree->SetSmallestSizePosition(min_rect);
}

/**
 * Construct the widget tree of the window, and initialize the window with it.
 * @param parts %Widget parts describing the window.
 * @param length Number of parts.
 * @pre The tree has not been setup before.
 */
void GuiWindow::SetupWidgetTree(const WidgetPart *parts, int length)
{
	assert(this->tree == nullptr && this->widgets == nullptr);

	int16 biggest;
	this->tree.reset(MakeWidgetTree(parts, length, &biggest));

	if (biggest >= 0) {
		this->num_widgets = biggest + 1;
		this->widgets.reset(new BaseWidget *[biggest + 1]);
		for (int16 i = 0; i <= biggest; i++) this->widgets[i] = nullptr;
	}
	this->ResetSize();

	Point32 pt = this->OnInitialPosition();
	this->SetPosition(pt.x, pt.y);
	this->initialized = true;
}

/**
 * Connect a scrollbar and a scrolled window with each other.
 * @param scrolled %Widget number of the widget being scrolled by the scrollbar.
 * @param scrollbar Scrollbar controlling the scrolled widget.
 * @pre The widgets must have been initialized.
 */
void GuiWindow::SetScrolledWidget(WidgetNumber scrolled, WidgetNumber scrollbar)
{
	const BaseWidget *sw = this->GetWidget<BaseWidget>(scrolled);
	ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(scrollbar);
	sb->SetScrolled(sw);
}

/**
 * Allow for last minute changes in the initial widget size. If the function does nothing, you'll get the default widgets.
 * The values BaseWidget::min_x and BaseWidget::min_y may be altered, but it may be a bad idea to make them smaller.
 * BaseWidget::fill_x, BaseWidget::fill_y, BaseWidget::resize_x, BaseWidget::resize_y may also be changed.
 * Since this function is being called while the window is being constructed, there is not much else that has a sane value at this point.
 * @param wid_num Widget number of the provided widget.
 * @param wid The widget itself.
 */
void GuiWindow::UpdateWidgetSize([[maybe_unused]] WidgetNumber wid_num, [[maybe_unused]] BaseWidget *wid)
{
	/* Do nothing by default. */
}

/**
 * Set string parameters of the data string of the widget.
 * @param wid_num Widget number of the widget.
 */
void GuiWindow::SetWidgetStringParameters([[maybe_unused]] WidgetNumber wid_num) const
{
	/* Do nothing by default. */
}

/**
 * Draw custom contents of the widget.
 * The code should not do anything else but render contents to the screen.
 * @param wid_num Widget number of the widget being drawn.
 * @param wid The widget itself.
 */
void GuiWindow::DrawWidget([[maybe_unused]] WidgetNumber wid_num, [[maybe_unused]] const BaseWidget *wid) const
{
	/* Do nothing by default. */
}

void GuiWindow::OnDraw([[maybe_unused]] MouseModeSelector *selector)
{
	this->tree->Draw(this);
	if ((this->flags & WF_HIGHLIGHT) != 0) _video.DrawRectangle(this->rect, MakeRGBA(255, 255, 255, OPAQUE));
}

bool GuiWindow::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (this->tree->OnKeyEvent(key_code, mod, symbol)) return true;
	if (this->closeable && (key_code == WMKC_DELETE || key_code == WMKC_BACKSPACE || key_code == WMKC_CANCEL)) {
		delete this;
		return true;
	}
	return Window::OnKeyEvent(key_code, mod, symbol);
}

void GuiWindow::OnMouseWheelEvent(int direction)
{
	this->tree->OnMouseWheelEvent(direction);
}

WmMouseEvent GuiWindow::OnMouseButtonEvent(MouseButtons state)
{
	if (state != MB_LEFT || this->GetRelativeMouseX() < 0) return WMME_NONE;

	BaseWidget *bw = this->tree->GetWidgetByPosition(Point32(this->GetRelativeMouseX(), this->GetRelativeMouseY()));
	if (bw == nullptr) return WMME_NONE;
	if (bw->wtype == WT_TITLEBAR) return WMME_MOVE_WINDOW;
	if (bw->wtype == WT_CLOSEBOX) return WMME_CLOSE_WINDOW;

	Point16 widget_pos(static_cast<int16>(this->GetRelativeMouseX() - bw->pos.base.x), static_cast<int16>(this->GetRelativeMouseY() - bw->pos.base.y));
	LeafWidget *lw = dynamic_cast<LeafWidget *>(bw);
	if (lw != nullptr) {
		if (lw->IsShaded()) return WMME_NONE;

		if (bw->wtype == WT_TEXT_PUSHBUTTON || bw->wtype == WT_IMAGE_PUSHBUTTON) {
			/* For mono-stable buttons, 'press' the button, and set a timeout for 'releasing' it again. */
			lw->SetPressed(true);
			this->timeout = 4;
		}
		ScrollbarWidget *sw = dynamic_cast<ScrollbarWidget *>(bw);
		if (sw != nullptr) {
			sw->OnClick(this->rect.base, widget_pos);
			return WMME_NONE;
		}
	}
	if (bw->number >= 0) this->OnClick(bw->number, widget_pos);
	return WMME_NONE;
}

/**
 * Mouse moved in the viewport while the window has an active mouse selector.
 * @param vp %Viewport where the mouse moved.
 * @param pos New position of the mouse in the viewport.
 */
void GuiWindow::SelectorMouseMoveEvent([[maybe_unused]] Viewport *vp, [[maybe_unused]] const Point16 &pos)
{
}

/**
 * Mouse buttons changed state while the window has an active mouse selector.
 * @param state Previous and current state of the mouse buttons. @see MouseButtons
 */
void GuiWindow::SelectorMouseButtonEvent([[maybe_unused]] MouseButtons state)
{
}

/**
 * Mouse wheel turned while the window has an active mouse selector.
 * @param direction Direction of turning (-1 or +1).
 */
void GuiWindow::SelectorMouseWheelEvent([[maybe_unused]] int direction)
{
}

/**
 * A click with the left button at a widget has been detected.
 * @param widget %Widget number.
 * @param pos %Position in the \a widget.
 */
void GuiWindow::OnClick([[maybe_unused]] WidgetNumber widget, [[maybe_unused]] const Point16 &pos)
{
}

/**
 * Set the checked state of the given widget.
 * @param widget %Widget number.
 * @param value New checked state value.
 */
void GuiWindow::SetWidgetChecked(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsChecked() != value) {
		lw->SetChecked(value);
	}
}

/**
 * Is widget \a widget checked?
 * @param widget %Widget number to use.
 * @return Whether the widget is checked.
 */
bool GuiWindow::IsWidgetChecked(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsChecked();
}

/**
 * Set the pressed state of the given widget.
 * @param widget %Widget number.
 * @param value New pressed state value.
 */
void GuiWindow::SetWidgetPressed(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsPressed() != value) {
		lw->SetPressed(value);
	}
}

/**
 * Is widget \a widget pressed?
 * @param widget %Widget number to use.
 * @return Whether the widget is pressed.
 */
bool GuiWindow::IsWidgetPressed(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsPressed();
}

/**
 * Set the shaded state of the given widget.
 * @param widget %Widget number.
 * @param value New shaded state value.
 */
void GuiWindow::SetWidgetShaded(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsShaded() != value) {
		lw->SetShaded(value);
	}
}

/**
 * Is widget \a widget shaded?
 * @param widget %Widget number to use.
 * @return Whether the widget is shaded.
 */
bool GuiWindow::IsWidgetShaded(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsShaded();
}

/**
 * Change the state of a set of radio buttons (#WT_RADIOBUTTON or bi-stable button widgets).
 * @param wids Array of widgets, terminate with #INVALID_WIDGET_INDEX.
 * @param selected Current selected widget of the radio buttons.
 */
void GuiWindow::SetRadioButtonsSelected(const WidgetNumber *wids, WidgetNumber selected)
{
	while (*wids != INVALID_WIDGET_INDEX) {
		BaseWidget *wid = this->GetWidget<BaseWidget>(*wids);
		if (wid->wtype == WT_RADIOBUTTON) {
			this->SetWidgetChecked(*wids, *wids == selected);
		} else {
			this->SetWidgetPressed(*wids, *wids == selected);
		}
		wids++;
	}
}

/**
 * Find the currently selected widget from a set of radio buttons (#WT_RADIOBUTTON or bi-stable button widgets).
 * @param wids Array of widgets, terminate with #INVALID_WIDGET_INDEX.
 * @return Currently pressed widget, or #INVALID_WIDGET_INDEX if none found.
 */
WidgetNumber GuiWindow::GetSelectedRadioButton(const WidgetNumber *wids)
{
	while (*wids != INVALID_WIDGET_INDEX) {
		LeafWidget *wid = this->GetWidget<LeafWidget>(*wids);
		if (!wid->IsShaded()) {
			if ((wid->wtype == WT_RADIOBUTTON && wid->IsChecked()) ||
					(wid->wtype != WT_RADIOBUTTON && wid->IsPressed())) return *wids;
		}
		wids++;
	}
	return INVALID_WIDGET_INDEX;
}

void GuiWindow::TimeoutCallback()
{
	this->tree->AutoRaiseButtons(this->rect.base);
	if ((this->flags & WF_HIGHLIGHT) != 0) this->SetHighlight(false);
}

void GuiWindow::SetHighlight(bool value)
{
	if (value) {
		this->flags |= WF_HIGHLIGHT;
		this->timeout = 5;
	} else {
		this->flags &= ~WF_HIGHLIGHT;
	}
}

BaseWidget *GuiWindow::FindTooltipWidget(Point16 pt)
{
	if (this->tree == nullptr) return nullptr;
	pt -= this->rect.base;
	return this->tree->FindTooltipWidget(pt);
}

/**
 * %Window manager default constructor.
 * @todo Move this to a separate InitWindowSystem?
 */
WindowManager::WindowManager()
:
	top(nullptr),
	bottom(nullptr),
	current_window(nullptr),
	select_window(nullptr),
	select_valid(true)
{
}

/** %Window manager destructor. */
WindowManager::~WindowManager()
{
}

/** Close all windows at the display. */
void WindowManager::CloseAllWindows()
{
	while (this->top != nullptr) delete this->top;
}

/** Reinitialize all windows in the display. */
void WindowManager::ResetAllWindows()
{
	for (Window *w = this->top; w != nullptr; w = w->lower) {
		w->ResetSize(); /// \todo This call should preserve the window size as much as possible.
	}
}

/**
 * Moves relevant windows if they've been moved offscreen by a window resize.
 * Also forces the bottom toolbar to be moved, as that will always be in the wrong position.
 * @param new_width New width of the display.
 * @param new_height New height of the display.
 */
void WindowManager::RepositionAllWindows(uint new_width, uint new_height)
{
	Rectangle32 rect(0, 0, new_width, new_height);
	for (Window *w = this->top; w != nullptr; w = w->lower) {
		if (w->wtype == WC_MAINDISPLAY || w->wtype == WC_MAIN_MENU) {
			w->SetSize(new_width, new_height);

		/* Add an arbitrary amount for closebox/titlebar, so the window is still actually accessible. */
		} else if (!rect.IsPointInside(Point32(w->rect.base.x + 20, w->rect.base.y + 20)) || w->wtype == WC_BOTTOM_TOOLBAR) {
			w->SetPosition(w->OnInitialPosition());
		}
	}
}

/**
 * Get the z-priority of a window type (higher number means further up in the window stack).
 * @param wt Window type.
 * @return Z-priority of the provided type.
 * @ingroup window_group
 */
static uint GetWindowZPriority(WindowTypes wt)
{
	switch (wt) {
		case WC_DROPDOWN:       return 12; // Dropdown menus.
		case WC_ERROR_MESSAGE:  return 10; // Error messages.
		default:                return 5;  // 'Normal' window.
		case WC_TOOLBAR:        return 1;  // Top toolbar.
		case WC_BOTTOM_TOOLBAR: return 1;  // Bottom toolbar.
		case WC_MAINDISPLAY:    return 0;  // Main display at the bottom of the stack.
		case WC_MAIN_MENU:      return 2;  // Main menu at the bottom of the stack but above the viewport.
	}
	NOT_REACHED();
}

/**
 * Add a window to the window stack.
 * @param w Window to add.
 */
void WindowManager::AddToStack(Window *w)
{
	assert(w->lower == nullptr && w->higher == nullptr);
	assert(!this->HasWindow(w));

	if (w->wtype == WC_MAINDISPLAY) { // Add the main world display as viewport.
		assert(this->viewport == nullptr);
		this->viewport = static_cast<Viewport *>(w);
	}

	this->select_valid = false;

	uint w_prio = GetWindowZPriority(w->wtype);
	if (this->top == nullptr || w_prio >= GetWindowZPriority(this->top->wtype)) {
		/* Add to the top. */
		w->lower = this->top;
		w->higher = nullptr;
		if (this->top != nullptr) this->top->higher = w;
		this->top = w;
		if (this->bottom == nullptr) this->bottom = w;
		return;
	}
	Window *stack = this->top;
	while (stack->lower != nullptr && w_prio < GetWindowZPriority(stack->lower->wtype)) stack = stack->lower;

	w->lower = stack->lower;
	if (stack->lower != nullptr) {
		stack->lower->higher = w;
	} else {
		assert(this->bottom == stack);
		this->bottom = w;
	}
	w->higher = stack;
	stack->lower = w;
}

/**
 * Remove a window from the list.
 * @param w Window to remove.
 */
void WindowManager::RemoveFromStack(Window *w)
{
	assert(this->HasWindow(w));

	if (w == this->viewport) this->viewport = nullptr;

	this->select_valid = false;
	if (w == this->current_window) this->current_window = nullptr;

	if (w->higher == nullptr) {
		this->top = w->lower;
	} else {
		w->higher->lower = w->lower;
	}

	if (w->lower == nullptr) {
		this->bottom = w->higher;
	} else {
		w->lower->higher = w->higher;
	}

	w->higher = nullptr;
	w->lower  = nullptr;
}

/**
 * Raise a window.
 * @param w Window to raise.
 */
void WindowManager::RaiseWindow(Window *w)
{
	if (w != this->top && GetWindowZPriority(w->wtype) >= GetWindowZPriority(w->higher->wtype)) {
		this->RemoveFromStack(w);
		this->AddToStack(w);
	}
}

/**
 * Set a new mouse mode selector for the given window, the current selector may become invalid.
 * @param w %Window owning the selector to set.
 * @param selector Selector to set. May be \c nullptr to deselect a selector.
 */
void WindowManager::SetSelector(GuiWindow *w, MouseModeSelector *selector)
{
	if (w->selector == selector) return; // Setting the same selector in the window again is fine.

	if (!this->select_valid) {
		w->selector = selector; // Cache is invalid, any change is fine.
		return;
	}

	if (this->select_window == nullptr) { // No selected window yet, invalidate cache if a real selector is added to the window.
		w->selector = selector;
		if (selector != nullptr) this->select_valid = false;

	} else if (this->select_window == w) { // Currently selected window changes its selector.
		this->select_window->selector = selector;
		if (selector == nullptr) {
			this->select_valid = false;
		}

	} else if (w->selector != nullptr) { // A non-selected window changes its selector.
		w->selector = selector; // w is definitely below this->select_window.
	} else {
		w->selector = selector; // w may be above this->select_window, invalidate cache.

		this->select_valid = false;
	}
}

/**
 * Get the currently active selector.
 * @return The currently active selector, or \c nullptr if no such window exists.
 */
GuiWindow *WindowManager::GetSelector()
{
	if (this->select_valid) return this->select_window;

	Window *w = this->top;
	while (w != nullptr) {
		GuiWindow *gw = dynamic_cast<GuiWindow *>(w);
		if (gw != nullptr && gw->selector != nullptr) {
			this->select_window = gw;
			this->select_valid = true;
			return this->select_window;
		}
		w = w->lower;
	}
	this->select_window = nullptr;
	this->select_valid = true;
	return this->select_window;
}

/**
 * Test whether a particular window exists in the window stack.
 * @param w Window to look for.
 * @return Window exists in the window stack.
 * @note Mainly used for paranoia checking.
 */
bool WindowManager::HasWindow(Window *w)
{
	Window *v = this->top;
	while (v != nullptr) {
		if (v == w) return true;
		v = v->lower;
	}
	return false;
}

/**
 * Find the window that covers a given position of the display.
 * @param pos Position of the display.
 * @return The window displayed at the given position, or \c nullptr if no window.
 */
Window *WindowManager::FindWindowByPosition(const Point16 &pos) const
{
	Window *w = this->top;
	while (w != nullptr) {
		if (w->rect.IsPointInside(pos)) break;
		w = w->lower;
	}
	return w;
}

/**
 * Mouse moved to new coordinates.
 */
void WindowManager::MouseMoveEvent()
{
	if (_video.GetMouseDragging() != MB_NONE) {
		assert(this->current_window != nullptr);
		assert(this->current_window->wtype != WC_MAIN_MENU);  // Cannot move the main display.
		if ((_video.GetMouseDragging() & MB_LEFT) != 0 && this->current_window->wtype != WC_MAINDISPLAY) {
			this->current_window->SetPosition(_video.MouseX() - this->move_offset.x, _video.MouseY() - this->move_offset.y);
		} else {
			this->current_window->OnMouseMoveEvent(Point16(this->current_window->GetRelativeMouseX(), this->current_window->GetRelativeMouseY()));
		}
		return;
	}

	this->UpdateCurrentWindow();

	if (this->current_window != nullptr) {
		this->current_window->OnMouseMoveEvent(Point16(this->current_window->GetRelativeMouseX(), this->current_window->GetRelativeMouseY()));
	}
}

/**
 * Update the #current_window variable.
 * This may happen when the mouse has moved, but also because of a change in the window stack.
 * @todo Hook a call to this function into the window stack changing code.
 */
void WindowManager::UpdateCurrentWindow()
{
	Window *w = this->FindWindowByPosition(_video.GetMousePosition());
	if (w == this->current_window) return;

	/* Windows are different, send mouse leave/enter events. */
	if (this->current_window != nullptr && this->HasWindow(this->current_window)) this->current_window->OnMouseLeaveEvent();

	this->current_window = w;
	if (this->current_window != nullptr) this->current_window->OnMouseEnterEvent();
}

/**
 * A mouse button was pressed or released.
 * @param button The button that changed state.
 * @param pressed The button was pressed (\c false means it was released instead).
 */
void WindowManager::MouseButtonEvent(MouseButtons button, bool pressed)
{
	assert(button == MB_LEFT || button == MB_MIDDLE || button == MB_RIGHT);

	this->UpdateCurrentWindow();
	if (this->current_window == nullptr) {
		_video.SetMouseDragging(button, pressed, false);
		return;
	}

	/* Close dropdown window if click is not inside it */
	Window *w = GetWindowByType(WC_DROPDOWN, ALL_WINDOWS_OF_TYPE);
	if (pressed && w != nullptr && !w->rect.IsPointInside(_video.GetMousePosition())) {
		delete w;
		return; // Don't handle click any further.
	}

	if (button == MB_LEFT && pressed) this->RaiseWindow(this->current_window);

	if ((_video.GetMouseDragging() & button) != MB_NONE) {
		if (!pressed) _video.SetMouseDragging(button, false, false);
	} else if (pressed) {
		WmMouseEvent me = this->current_window->OnMouseButtonEvent(button);
		switch (me) {
			case WMME_NONE:
				break;

			case WMME_MOVE_WINDOW:
				if (this->current_window != nullptr && this->current_window->rect.IsPointInside(_video.GetMousePosition())) {
					_video.SetMouseDragging(button, pressed, false);
					this->move_offset.x = this->current_window->GetRelativeMouseX();
					this->move_offset.y = this->current_window->GetRelativeMouseY();
				}
				break;

			case WMME_CLOSE_WINDOW:
				delete this->current_window;
				break;

			default:
				NOT_REACHED();
		}
	}
}

/**
 * The mouse wheel has been turned.
 * @param direction Direction of the turn (either \c +1 or \c -1).
 */
void WindowManager::MouseWheelEvent(int direction)
{
	this->UpdateCurrentWindow();
	if (this->current_window != nullptr) this->current_window->OnMouseWheelEvent(direction);
}

/**
 * Process input from the keyboard.
 * @param key_code Kind of input.
 * @param mod Bitmask of pressed modifiers.
 * @param symbol Entered symbol, if \a key_code is #WMKC_SYMBOL. Utf-8 encoded.
 * @return Key event has been processed.
 */
bool WindowManager::KeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	for (Window *w = this->top; w != nullptr; w = w->lower) {
		if (w->OnKeyEvent(key_code, mod, symbol)) return true;
	}
	return false;
}

/**
 * Redraw (parts of) the windows.
 * @todo [medium/difficult] Do this much less stupid.
 * @ingroup window_group
 */
void WindowManager::UpdateWindows()
{
	BaseWidget *tooltip_widget = nullptr;
	Window *tooltip_window = nullptr;
	if (_video.GetMouseDragging() == MB_NONE && this->current_window != nullptr) {
		tooltip_widget = this->current_window->FindTooltipWidget(_video.GetMousePosition());
		if (tooltip_widget != nullptr) tooltip_window = this->current_window;
	}

	/* Until the entire background is covered by the main display, clean the entire display to ensure deleted
	 * windows truly disappear (even if there is no other window behind it).
	 */
	Rectangle32 rect(0, 0, _video.Width(), _video.Height());
	_video.FillRectangle(rect, MakeRGBA(0, 0, 0, OPAQUE));

	GuiWindow *sel_window = this->GetSelector();
	MouseModeSelector *selector = (sel_window == nullptr) ? nullptr : sel_window->selector;
	for (Window *w = this->bottom; w != nullptr; w = w->higher) w->OnDraw(selector);

	if (tooltip_widget != nullptr) {
		tooltip_window->SetTooltipStringParameters(tooltip_widget);
		tooltip_widget->DrawTooltip(tooltip_window->rect.base);
	}

	_video.FinishRepaint();
}

/** A tick has passed, update whatever must be updated. */
void WindowManager::Tick()
{
	Window *w = _window_manager.top;
	while (w != nullptr) {
		Window *next = w->lower;
		if (w->timeout > 0) {
			w->timeout--;
			if (w->timeout == 0) w->TimeoutCallback();  // This might delete the window, do not use hereafter.
		}
		w = next;
	}

	this->UpdateWindows();
}

/**
 * Find an opened window by window type.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @return The window with the requested type if it is opened, else \c nullptr.
 * @ingroup window_group
 */
Window *GetWindowByType(WindowTypes wtype, WindowNumber wnumber)
{
	Window *w = _window_manager.top;
	while (w != nullptr) {
		if (w->wtype == wtype && (wnumber == ALL_WINDOWS_OF_TYPE || w->wnumber == wnumber)) return w;
		w = w->lower;
	}
	return nullptr;
}

/**
 * Notify the window of the given type of the change with the specified number.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @param code Unique change number.
 * @param parameter Parameter of the change number
 */
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter)
{
	Window *w = GetWindowByType(wtype, wnumber);
	if (w != nullptr) w->OnChange(code, parameter);
}

/**
 * Highlight and raise a window of a given type.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @return The window which has been highlighted and raised, or \c nullptr if no such window exists.
 * @ingroup window_group
 */
Window *HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber)
{
	Window *w = GetWindowByType(wtype, wnumber);
	if (w != nullptr) {
		_window_manager.RaiseWindow(w);
		w->SetHighlight(true);
	}
	return w;
}

/**
 * Open a window of the correct type to manage a ride.
 * @param ride Ride to manage.
 * @return A window was opened.
 */
bool ShowRideManagementGui(const uint16 ride)
{
	RideInstance *ri = _rides_manager.GetRideInstance(ride);
	if (ri == nullptr) return false;

	switch (ri->GetKind()) {
		case RTK_SHOP:
			ShowShopManagementGui(ride);
			return true;

		case RTK_GENTLE:
		case RTK_THRILL:
			ShowGentleThrillRideManagementGui(ride);
			return true;

		case RTK_COASTER:
			ShowCoasterManagementGui(ri);
			return true;

		default: NOT_REACHED();  // Other types are not implemented yet.
	}
	return false;
}
