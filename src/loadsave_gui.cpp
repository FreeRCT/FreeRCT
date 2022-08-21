/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file loadsave_gui.cpp Gui for selecting the file to load from or save to. */

#include "stdafx.h"
#include "window.h"
#include "gamecontrol.h"
#include "gamelevel.h"
#include "fileio.h"
#include "rev.h"
#include "sprite_store.h"

/**
 * Game loading/saving gui.
 * @ingroup gui_group
 */
class LoadSaveGui : public GuiWindow {
public:
	/** Type of the loading/saving window. */
	enum Type {
		LOAD,  ///< Load a saved game.
		SAVE   ///< Save the running game.
	};
	LoadSaveGui(Type t);

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;

private:
	std::string FinalFilename() const;

	const Type type;                  ///< Type of this window.
	std::set<PreloadData> all_files;  ///< All files in the working directory.
};

/**
 * Widget numbers of the loading/saving Gui.
 * @ingroup gui_group
 */
enum LoadSaveWidgets {
	LSW_TITLEBAR,   ///< Window title bar.
	LSW_LIST,       ///< List of all files - wrapper.
	LSW_LIST_FILE,  ///< List of all files - filename column.
	LSW_LIST_TIME,  ///< List of all files - timestamp column.
	LSW_LIST_NAME,  ///< List of all files - scenario name column.
	LSW_LIST_REV,   ///< List of all files - revision compatibility column.
	LSW_SCROLLBAR,  ///< Scrollbar for the list.
	LSW_TEXTFIELD,  ///< Text field for the filename.
	LSW_OK,         ///< Confirmation button.
	LSW_CANCEL,     ///< Cancel button.
};

static const uint ITEM_COUNT   =  8;  ///< Number of files to display in the list.
static const uint ITEM_HEIGHT  = 26;  ///< Height of one item in the list.
static const uint ITEM_SPACING =  2;  ///< Spacing in the list.

/**
 * Widget parts of the saving GUI.
 * @ingroup gui_group
 */
static const WidgetPart _loadsave_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, LSW_TITLEBAR,         COL_RANGE_BLUE), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
		EndContainer(),

		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			Intermediate(1, 2),
				Widget(WT_PANEL, LSW_LIST, COL_RANGE_BLUE),
					Intermediate(2, 4),
						Widget(WT_TEXT_PUSHBUTTON, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_FILE, STR_NULL),
						Widget(WT_TEXT_PUSHBUTTON, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_TIME, STR_NULL),
						Widget(WT_TEXT_PUSHBUTTON, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_NAME, STR_NULL),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
						Widget(WT_EMPTY, LSW_LIST_FILE, COL_RANGE_BLUE),
							SetFill(0, ITEM_HEIGHT), SetResize(0, ITEM_HEIGHT), SetMinimalSize(200, ITEM_COUNT * ITEM_HEIGHT),
						Widget(WT_EMPTY, LSW_LIST_TIME, COL_RANGE_BLUE),
							SetFill(0, ITEM_HEIGHT), SetResize(0, ITEM_HEIGHT), SetMinimalSize(200, ITEM_COUNT * ITEM_HEIGHT),
						Widget(WT_EMPTY, LSW_LIST_NAME, COL_RANGE_BLUE),
							SetFill(0, ITEM_HEIGHT), SetResize(0, ITEM_HEIGHT), SetMinimalSize(200, ITEM_COUNT * ITEM_HEIGHT),
						Widget(WT_EMPTY, LSW_LIST_REV , COL_RANGE_BLUE),
							SetFill(0, ITEM_HEIGHT), SetResize(0, ITEM_HEIGHT), SetMinimalSize(ITEM_HEIGHT + 2 * ITEM_SPACING, ITEM_COUNT * ITEM_HEIGHT),
				Widget(WT_VERT_SCROLLBAR, LSW_SCROLLBAR, COL_RANGE_BLUE),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			Intermediate(2, 1),
				Widget(WT_TEXT_INPUT, LSW_TEXTFIELD, COL_RANGE_BLUE),
				Intermediate(1, 2),
					Widget(WT_TEXT_PUSHBUTTON, LSW_CANCEL, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_CANCEL, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, LSW_OK,     COL_RANGE_BLUE), SetData(STR_ARG1,            STR_NULL),

	EndContainer(),
};

LoadSaveGui::LoadSaveGui(const Type t) : GuiWindow(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE), type(t)
{
	/* Get all .fct files in the directory. */
	std::unique_ptr<DirectoryReader> dr(MakeDirectoryReader());
	dr->OpenPath((freerct_userdata_prefix() + DIR_SEP + SAVEGAME_DIRECTORY).c_str());
	for (;;) {
		const char *str = dr->NextEntry();
		if (str == nullptr) break;
		std::string name(str);
		if (name.size() > 4 && name.compare(name.size() - 4, 4, ".fct") == 0) {
			this->all_files.insert(PreloadGameFile(name.c_str()));
		}
	}
	dr->ClosePath();

	this->SetupWidgetTree(_loadsave_gui_parts, lengthof(_loadsave_gui_parts));
	this->SetScrolledWidget(LSW_LIST, LSW_SCROLLBAR);
	this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->SetItemCount(this->all_files.size());
}

void LoadSaveGui::SetWidgetStringParameters(const WidgetNumber wid_num) const
{
	switch (wid_num) {
		case LSW_TITLEBAR:
		case LSW_OK:
			_str_params.SetStrID(1, this->type == SAVE ? GUI_LOADSAVE_SAVE : GUI_LOADSAVE_LOAD);
			break;

		default: break;
	}
}

/**
 * Turn the current value of the text input box into a valid .fct filename.
 * @return The filename to use.
 * @todo Check that the filename is actually valid.
 */
std::string LoadSaveGui::FinalFilename() const
{
	std::string result = this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->GetText();
	if (result.size() < 5 || result.compare(result.size() - 4, 4, ".fct")) result += ".fct";
	return result;
}

bool LoadSaveGui::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (key_code == WMKC_CONFIRM) {
		this->OnClick(LSW_OK, Point16());
		return true;
	}
	return GuiWindow::OnKeyEvent(key_code, mod, symbol);
}

void LoadSaveGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case LSW_CANCEL:
			delete this;
			break;

		case LSW_LIST_FILE:
		case LSW_LIST_TIME:
		case LSW_LIST_NAME:
		case LSW_LIST_REV:
		case LSW_LIST: {
			const int index = pos.y / ITEM_HEIGHT + this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
			if (index < 0 || index >= static_cast<int>(this->all_files.size())) break;

			auto selected = this->all_files.begin();
			std::advance(selected, index);
			this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetText(selected->filename);
			break;
		}

		case LSW_OK: {
			const std::string filename = this->FinalFilename();
			std::string path = SavegameDirectory();
			path += filename;

			const PreloadData *existing_file = nullptr;
			for (const PreloadData &pd : this->all_files) {
				if (pd.filename == filename) {
					existing_file = &pd;
					break;
				}
			}

			switch (this->type) {
				case SAVE:
					if (existing_file != nullptr) {
						/* \todo Show a confirmation dialogue to ask the user whether the file should be overwritten. */
					}
					_game_control.SaveGame(path.c_str());
					break;
				case LOAD:
					if (existing_file == nullptr || !existing_file->load_success) return;  // The file does not exist or is invalid.
					_game_control.LoadGame(path.c_str());
					break;
			}
			delete this;
			break;
		}

		default: break;
	}
}

void LoadSaveGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != LSW_LIST_FILE && wid_num != LSW_LIST_NAME && wid_num != LSW_LIST_TIME && wid_num != LSW_LIST_REV) {
		return GuiWindow::DrawWidget(wid_num, wid);
	}

	int x = this->GetWidgetScreenX(wid) + 2 * ITEM_SPACING;
	int y = this->GetWidgetScreenY(wid) + 2 * ITEM_SPACING;
	const int w = wid->pos.width - 4 * ITEM_SPACING;
	const size_t first_index = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
	const size_t last_index = std::min<size_t>(this->all_files.size(), first_index + ITEM_COUNT);
	auto iterator = this->all_files.begin();
	std::advance(iterator, first_index);
	const std::string selected_filename = this->FinalFilename();

	std::function<void(const PreloadData&)> functor;

	switch (wid_num) {
		case LSW_LIST_FILE:
			functor = [&x, &y, &w](const PreloadData &pd) {
				_video.BlitText(pd.filename, _palette[pd.load_success ? TEXT_WHITE : TEXT_GREY], x, y, w, ALG_LEFT);
			};
			break;

		case LSW_LIST_TIME:
			functor = [&x, &y, &w](const PreloadData &pd) {
				if (!pd.load_success) return;
				static char buffer[256];
				if (pd.timestamp > 0) {
					std::strftime(buffer, sizeof(buffer), _language.GetSgText(GUI_DATETIME_FORMAT).c_str(), std::localtime(&pd.timestamp));
				} else {
					strcpy(buffer, _language.GetSgText(GUI_NOT_AVAILABLE).c_str());
				}
				_video.BlitText(buffer, _palette[pd.load_success ? TEXT_WHITE : TEXT_GREY], x, y, w, ALG_LEFT);
			};
			break;

		case LSW_LIST_NAME:
			functor = [&x, &y, &w](const PreloadData &pd) {
				_video.BlitText(pd.load_success ? pd.scenario->name : "", _palette[pd.load_success ? TEXT_WHITE : TEXT_GREY], x, y, w, ALG_LEFT);
			};
			break;

		case LSW_LIST_REV:
			functor = [&x, &y](const PreloadData &pd) {
				static const Recolouring r;
				const ImageData *imgdata = _sprite_manager.GetTableSprite(
						pd.load_success ? pd.revision == _freerct_revision ? SPR_GUI_LOADSAVE_OK : SPR_GUI_LOADSAVE_WARN : SPR_GUI_LOADSAVE_ERR);
				if (imgdata != nullptr) _video.BlitImage(Point32(x - ITEM_SPACING / 2, y - ITEM_SPACING / 2), imgdata, r, GS_NORMAL);
			};
			break;

		default:
			NOT_REACHED();
	}

	for (size_t i = first_index; i < last_index; i++, iterator++, y += ITEM_HEIGHT) {
		if (selected_filename == iterator->filename) {
			int sx = x - 2 * ITEM_SPACING;
			int sw = w + 4 * ITEM_SPACING;
			if (wid_num == LSW_LIST_REV) sw -= ITEM_SPACING;
			_video.FillRectangle(Rectangle32(sx, y - 2 * ITEM_SPACING, sw, ITEM_HEIGHT), _palette[COL_SERIES_START + COL_RANGE_BLUE * COL_SERIES_LENGTH + 1]);
		}
		functor(*iterator);
	}
}

/**
 * Open the GUI to load a game.
 * @ingroup gui_group
 */
void ShowLoadGameGui()
{
	if (HighlightWindowByType(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new LoadSaveGui(LoadSaveGui::LOAD);
}

/**
 * Open the GUI to save a game.
 * @ingroup gui_group
 */
void ShowSaveGameGui()
{
	if (HighlightWindowByType(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new LoadSaveGui(LoadSaveGui::SAVE);
}
