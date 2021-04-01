/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main_menu.cpp Implementation of the main menu. */

#include "stdafx.h"
#include "gamecontrol.h"
#include "sprite_store.h"
#include "window.h"

/**
 * The main menu.
 * @ingroup gui_group
 */
class MainMenuGui : public Window {
public:
	MainMenuGui();
	~MainMenuGui();

	WmMouseEvent OnMouseButtonEvent(uint8 state) override;
	void OnDraw(MouseModeSelector *selector) override;

private:
	uint32 animstart;              ///< SDL Time when the animation started.
	static bool is_splash_screen;  ///< Whether we're currently displaying the splash screen. Static because the splash screen should be shown only once.

	Rectangle32 new_game_rect;   ///< Position of the New Game button.
	Rectangle32 load_game_rect;  ///< Position of the Load Game button.
	Rectangle32 settings_rect;   ///< Position of the Settings button.
	Rectangle32 quit_rect;       ///< Position of the Quit button.
};

bool MainMenuGui::is_splash_screen = true;

MainMenuGui::MainMenuGui() : Window(WC_MAIN_MENU, ALL_WINDOWS_OF_TYPE)
{
	this->SetSize(_video.GetXSize(),_video.GetYSize());
	_game_control.main_menu = true;
	this->animstart = SDL_GetTicks();
}

MainMenuGui::~MainMenuGui()
{
	_game_control.main_menu = false;
}

WmMouseEvent MainMenuGui::OnMouseButtonEvent(const uint8 state)
{
	if (is_splash_screen) {
		is_splash_screen = false;
		this->animstart = SDL_GetTicks();
		return WMME_NONE;
	}

	if (!IsLeftClick(state)) return WMME_NONE;

	if (this->new_game_rect.IsPointInside(_window_manager.GetMousePosition())) {
		_game_control.NewGame();
		delete this;
	} else if (this->load_game_rect.IsPointInside(_window_manager.GetMousePosition())) {
		/* \todo Provide option to select the file to load. */
		_game_control.LoadGame("saved.fct");
		delete this;
	} else if (this->quit_rect.IsPointInside(_window_manager.GetMousePosition())) {
		_game_control.QuitGame();
	} else if (this->settings_rect.IsPointInside(_window_manager.GetMousePosition())) {
		ShowSettingGui();
	}

	return WMME_NONE;
}

static const int    MAIN_MENU_BUTTON_SIZE  =    96;  ///< Size of the main menu buttons.
static const int    MAIN_MENU_PADDING      =    24;  ///< Padding in the main menu.

void MainMenuGui::OnDraw(MouseModeSelector *selector)
{
	static Recolouring rc;
	const uint32 current_time = SDL_GetTicks();
	uint32 frametime = current_time - this->animstart;
	if (is_splash_screen && frametime > 3 * _gui_sprites.mainmenu_splash_duration) {
		is_splash_screen = false;
		this->animstart = current_time;
		frametime = 0;
	}

	_video.FillRectangle(Rectangle32(0, 0, _video.GetXSize(), _video.GetYSize()), 0xff);

	const int button_x = (_video.GetXSize() - 7 * MAIN_MENU_BUTTON_SIZE) / 2;
	const int button_y = _video.GetYSize() - MAIN_MENU_BUTTON_SIZE;
	this->new_game_rect  = Rectangle32(button_x + 0 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->load_game_rect = Rectangle32(button_x + 2 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->settings_rect  = Rectangle32(button_x + 4 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->quit_rect      = Rectangle32(button_x + 6 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);

	if (!is_splash_screen || frametime > 2 * _gui_sprites.mainmenu_splash_duration) {
		// NOCOM
		/* _video.BlitImage(Point32(_video.GetXSize() / 2, _video.GetYSize() / 2),
				_gui_sprites.mainmenu_animation[is_splash_screen ? 0 : (frametime / _gui_sprites.mainmenu_duration) % _gui_sprites.mainmenu_frames], rc, GS_NORMAL); */
		_video.BlitImage(Point32(_video.GetXSize() / 2, _video.GetYSize() / 4), _gui_sprites.mainmenu_logo, rc, GS_NORMAL);

		_video.BlitImage(this->new_game_rect.base,  _gui_sprites.mainmenu_new,      rc, GS_NORMAL);
		_video.BlitImage(this->load_game_rect.base, _gui_sprites.mainmenu_load,     rc, GS_NORMAL);
		_video.BlitImage(this->settings_rect.base,  _gui_sprites.mainmenu_settings, rc, GS_NORMAL);
		_video.BlitImage(this->quit_rect.base,      _gui_sprites.mainmenu_quit,     rc, GS_NORMAL);

		DrawString(GUI_MAIN_MENU_NEW_GAME, TEXT_WHITE,
				this->new_game_rect.base.x,  this->new_game_rect.base.y  - MAIN_MENU_PADDING + this->new_game_rect.height,
				this->new_game_rect.width,  ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_LOAD, TEXT_WHITE,
				this->load_game_rect.base.x, this->load_game_rect.base.y - MAIN_MENU_PADDING + this->load_game_rect.height,
				this->load_game_rect.width, ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_SETTINGS, TEXT_WHITE,
				this->settings_rect.base.x,  this->settings_rect.base.y  - MAIN_MENU_PADDING + this->settings_rect.height,
				this->settings_rect.width,  ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_QUIT, TEXT_WHITE,
				this->quit_rect.base.x,      this->quit_rect.base.y      - MAIN_MENU_PADDING + this->quit_rect.height,
				this->quit_rect.width,      ALG_CENTER, true);
	}

	if (is_splash_screen) {
		if (frametime < 2 * _gui_sprites.mainmenu_splash_duration) {
			_video.BlitImage(Point32(_video.GetXSize() / 2, _video.GetYSize() / 2), _gui_sprites.mainmenu_splash, rc, GS_NORMAL);
			if (frametime > _gui_sprites.mainmenu_splash_duration) {
				_video.FillRectangle(Rectangle32(0, 0, _video.GetXSize(), _video.GetYSize()),
						0xff * (frametime - _gui_sprites.mainmenu_splash_duration) / _gui_sprites.mainmenu_splash_duration);
			}
		} else {
			_video.FillRectangle(Rectangle32(0, 0, _video.GetXSize(), _video.GetYSize()),
						0xff - 0xff * (frametime - 2 * _gui_sprites.mainmenu_splash_duration) / _gui_sprites.mainmenu_splash_duration);
		}
	}
}

/**
 * Run the main menu.
 * @ingroup gui_group
 */
void ShowMainMenu()
{
	new MainMenuGui;
}
