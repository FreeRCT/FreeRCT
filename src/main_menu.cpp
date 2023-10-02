/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main_menu.cpp Implementation of the main menu. */

#include "stdafx.h"
#include "config_reader.h"
#include "gamecontrol.h"
#include "sprite_store.h"
#include "viewport.h"
#include "window.h"
#include "fileio.h"

/**
 * The main menu.
 * @ingroup gui_group
 */
class MainMenuGui : public Window {
public:
	MainMenuGui();
	~MainMenuGui();

	WmMouseEvent OnMouseButtonEvent(MouseButtons state, WmMouseEventMode mode) override;
	void OnDraw(MouseModeSelector *selector) override;
	bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;

private:
	Realtime animstart;            ///< Time when the animation started.
	Realtime last_time;            ///< Time when the menu was last redrawn.
	static bool is_splash_screen;  ///< Whether we're currently displaying the splash screen. Static because the splash screen should be shown only once.

	ConfigFile camera_positions;   ///< Config file listing the camera positions for the savegame.
	uint32 nr_cameras;             ///< Total number of camera position.
	uint32 current_camera_id;      ///< ID of the current camera position.
	uint32 time_in_camera;         ///< Number of milliseconds since the last camera transition.

	Rectangle32 new_game_rect;       ///< Position of the New Game button.
	Rectangle32 load_game_rect;      ///< Position of the Load Game button.
	Rectangle32 launch_editor_rect;  ///< Position of the New Game button.
	Rectangle32 settings_rect;       ///< Position of the Settings button.
	Rectangle32 quit_rect;           ///< Position of the Quit button.
};

bool MainMenuGui::is_splash_screen = true;

MainMenuGui::MainMenuGui() : Window(WC_MAIN_MENU, ALL_WINDOWS_OF_TYPE), camera_positions(FindDataFile("data/mainmenu/camera"))
{
	this->SetSize(_video.Width(),_video.Height());
	this->animstart = this->last_time = Time();
	this->nr_cameras = this->camera_positions.GetNum("camera", "nr_cameras");
	this->time_in_camera = 0;
	this->current_camera_id = 0;

	/* Mark all expected config file entries as used. */
	for (uint32 i = 0; i < this->nr_cameras; i++) {
		const std::string section = std::to_string(i);
		for (const char *key : {"x", "y", "z", "orientation", "duration"}) camera_positions.GetValue(section, key);
	}
}

MainMenuGui::~MainMenuGui()
{
	_game_control.main_menu = false;
	is_splash_screen = false;
}

bool MainMenuGui::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (is_splash_screen) {
		is_splash_screen = false;
		return true;
	}
	return Window::OnKeyEvent(key_code, mod, symbol);
}

WmMouseEvent MainMenuGui::OnMouseButtonEvent(const MouseButtons state, WmMouseEventMode mode)
{
	if (mode != WMEM_PRESS) return WMME_NONE;

	if (is_splash_screen) {
		is_splash_screen = false;
		this->animstart = Time();
		return WMME_NONE;
	}

	if (state != MB_LEFT) return WMME_NONE;

	if (this->new_game_rect.IsPointInside(_video.GetMousePosition())) {
		_game_control.NewGame();
		delete this;
	} else if (this->launch_editor_rect.IsPointInside(_video.GetMousePosition())) {
		_game_control.LaunchEditor();
		delete this;
	} else if (this->load_game_rect.IsPointInside(_video.GetMousePosition())) {
		ShowLoadGameGui();
	} else if (this->quit_rect.IsPointInside(_video.GetMousePosition())) {
		_game_control.QuitGame();
	} else if (this->settings_rect.IsPointInside(_video.GetMousePosition())) {
		ShowSettingGui();
	}

	return WMME_NONE;
}

void MainMenuGui::OnDraw([[maybe_unused]] MouseModeSelector *selector)
{
	const Realtime current_time = Time();
	double frametime = Delta(this->animstart, current_time);
	if (is_splash_screen && frametime > 3 * _gui_sprites.mainmenu_splash_duration) {
		is_splash_screen = false;
		this->animstart = current_time;
		frametime = 0;
	}

	this->time_in_camera += Delta(last_time, current_time);
	if (static_cast<int64_t>(this->time_in_camera) > this->camera_positions.GetNum(std::to_string(this->current_camera_id), "duration")) {
		this->current_camera_id++;
		this->current_camera_id %= this->nr_cameras;
		this->time_in_camera = 0;
		const std::string section = std::to_string(this->current_camera_id);
		Viewport *vp = _window_manager.GetViewport();
		vp->orientation = static_cast<ViewOrientation>(this->camera_positions.GetNum(section, "orientation"));
		vp->view_pos.x  =                              this->camera_positions.GetNum(section, "x");
		vp->view_pos.y  =                              this->camera_positions.GetNum(section, "y");
		vp->view_pos.z  =                              this->camera_positions.GetNum(section, "z");
	}
	this->last_time = current_time;

	if (is_splash_screen && frametime < 2 * _gui_sprites.mainmenu_splash_duration) {
		_video.FillRectangle(Rectangle32(0, 0, _video.Width(), _video.Height()), 0xff);
	}

	constexpr int NR_BUTTONS = 5;  ///< Number of main menu buttons.
	const int MAIN_MENU_BUTTON_SIZE = _video.Width() / (2 * NR_BUTTONS);
	const int MAIN_MENU_PADDING = MAIN_MENU_BUTTON_SIZE / 4;

	const int button_x = (_video.Width() - (NR_BUTTONS * 2 - 1) * MAIN_MENU_BUTTON_SIZE) / 2;
	const int button_y = _video.Height() - MAIN_MENU_BUTTON_SIZE / 2;
	this->new_game_rect      = Rectangle32(button_x + 0 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->load_game_rect     = Rectangle32(button_x + 2 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->launch_editor_rect = Rectangle32(button_x + 4 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->settings_rect      = Rectangle32(button_x + 6 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);
	this->quit_rect          = Rectangle32(button_x + 8 * MAIN_MENU_BUTTON_SIZE, button_y - MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE, MAIN_MENU_BUTTON_SIZE);

	if (!is_splash_screen || frametime > 2 * _gui_sprites.mainmenu_splash_duration) {
		_video.BlitImage(Point32(_video.Width() / 2, _video.Height() / 4), _gui_sprites.mainmenu_logo);

		_video.BlitImage(CenterSprite(this->new_game_rect,      _gui_sprites.mainmenu_new),           _gui_sprites.mainmenu_new);
		_video.BlitImage(CenterSprite(this->load_game_rect,     _gui_sprites.mainmenu_load),          _gui_sprites.mainmenu_load);
		_video.BlitImage(CenterSprite(this->launch_editor_rect, _gui_sprites.mainmenu_launch_editor), _gui_sprites.mainmenu_launch_editor);
		_video.BlitImage(CenterSprite(this->settings_rect,      _gui_sprites.mainmenu_settings),      _gui_sprites.mainmenu_settings);
		_video.BlitImage(CenterSprite(this->quit_rect,          _gui_sprites.mainmenu_quit),          _gui_sprites.mainmenu_quit);

		DrawString(GUI_MAIN_MENU_NEW_GAME, TEXT_WHITE,
				this->new_game_rect.base.x,  this->new_game_rect.base.y  - MAIN_MENU_PADDING + this->new_game_rect.height,
				this->new_game_rect.width,  ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_LOAD, TEXT_WHITE,
				this->load_game_rect.base.x, this->load_game_rect.base.y - MAIN_MENU_PADDING + this->load_game_rect.height,
				this->load_game_rect.width, ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_LAUNCH_EDITOR, TEXT_WHITE,
				this->launch_editor_rect.base.x,  this->launch_editor_rect.base.y  - MAIN_MENU_PADDING + this->launch_editor_rect.height,
				this->launch_editor_rect.width,  ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_SETTINGS, TEXT_WHITE,
				this->settings_rect.base.x,  this->settings_rect.base.y  - MAIN_MENU_PADDING + this->settings_rect.height,
				this->settings_rect.width,  ALG_CENTER, true);
		DrawString(GUI_MAIN_MENU_QUIT, TEXT_WHITE,
				this->quit_rect.base.x,      this->quit_rect.base.y      - MAIN_MENU_PADDING + this->quit_rect.height,
				this->quit_rect.width,      ALG_CENTER, true);
	}

	if (is_splash_screen) {
		if (frametime < 2 * _gui_sprites.mainmenu_splash_duration) {
			_video.BlitImage(Point32(_video.Width() / 2, _video.Height() / 2), _gui_sprites.mainmenu_splash);
			if (frametime > _gui_sprites.mainmenu_splash_duration) {
				_video.FillRectangle(Rectangle32(0, 0, _video.Width(), _video.Height()),
						0xff * (frametime - _gui_sprites.mainmenu_splash_duration) / _gui_sprites.mainmenu_splash_duration);
			}
		} else {
			_video.FillRectangle(Rectangle32(0, 0, _video.Width(), _video.Height()),
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
