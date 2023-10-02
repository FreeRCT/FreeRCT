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

/** Sorting order for a list of savegames. */
using SortBy = bool(*)(const PreloadData&, const PreloadData&);

/** Sort by timestamp. */
static bool SortByTimestamp(const PreloadData &a, const PreloadData &b) {
	return a.timestamp > b.timestamp;
}

/** Sort alphabetically by filename. */
static bool SortByFilename(const PreloadData &a, const PreloadData &b) {
	return a.filename < b.filename;
}

/** Sort alphabetically by scenario name, with the filename as tie-breaker. */
static bool SortByScenario(const PreloadData &a, const PreloadData &b) {
	std::string name1 = a.scenario != nullptr ? _language.GetSgText(_language.GetStringByName(a.scenario->name_key)) : "";
	std::string name2 = b.scenario != nullptr ? _language.GetSgText(_language.GetStringByName(b.scenario->name_key)) : "";
	return name1 != name2 ? name1 < name2 : a.filename < b.filename;
}

/** Sort by revision compatibility, with the timestamp as tie-breaker. */
static bool SortByCompatibility(const PreloadData &a, const PreloadData &b) {
	int val1 = a.load_success ? a.revision == _freerct_revision ? 0 : 1 : 2;
	int val2 = b.load_success ? b.revision == _freerct_revision ? 0 : 1 : 2;
	/* \todo The revision would be a better tie-breaker, with timestamp as secondary breaker.
	 * But revisions have to be compared semantically, not just lexicographically, to handle cases like "git99" → "git100".
	 */
	return val1 != val2 ? val1 < val2 : a.timestamp > b.timestamp;
}

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
	void Sort(SortBy sort);
	void FileDeleted(std::string path);

private:
	std::string FinalFilename() const;
	int GetSelectedFileIndex() const;

	const Type type;                     ///< Type of this window.
	std::vector<PreloadData> all_files;  ///< All files in the working directory.
	SortBy current_sort;                 ///< Current sorting order.
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
	LSW_SORT_FILE,  ///< Sort by filename button.
	LSW_SORT_TIME,  ///< Sort by time button.
	LSW_SORT_NAME,  ///< Sort by scenario name button.
	LSW_SORT_REV,   ///< Sort by revision compatibility button.
	LSW_TEXTFIELD,  ///< Text field for the filename.
	LSW_OK,         ///< Confirmation button.
	LSW_DELETE,     ///< Delete savegame button.
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
						Widget(WT_TEXT_BUTTON, LSW_SORT_FILE, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_FILE, STR_NULL),
						Widget(WT_TEXT_BUTTON, LSW_SORT_TIME, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_TIME, STR_NULL),
						Widget(WT_TEXT_BUTTON, LSW_SORT_NAME, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_COLUMN_NAME, STR_NULL),
						Widget(WT_TEXT_BUTTON, LSW_SORT_REV , COL_RANGE_BLUE),
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
				Intermediate(1, 3),
					Widget(WT_TEXT_PUSHBUTTON, LSW_CANCEL, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_CANCEL, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, LSW_DELETE, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_DELETE, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, LSW_OK,     COL_RANGE_BLUE), SetData(STR_ARG1,            STR_NULL),

	EndContainer(),
};

LoadSaveGui::LoadSaveGui(const Type t) : GuiWindow(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE), type(t), current_sort(nullptr)
{
	/* Get all .fct files in the directory. */

	for (const std::string &name : GetAllEntries((freerct_userdata_prefix() + DIR_SEP + SAVEGAME_DIRECTORY))) {
		if (name.size() > 4 && name.compare(name.size() - 4, 4, ".fct") == 0) {
			this->all_files.push_back(PreloadGameFile(name.c_str()));
		}
	}

	this->SetupWidgetTree(_loadsave_gui_parts, lengthof(_loadsave_gui_parts));
	this->SetScrolledWidget(LSW_LIST, LSW_SCROLLBAR);
	this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->SetItemCount(this->all_files.size());
	this->Sort(SortByTimestamp);
	this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetFocus(true);
}

/**
 * A savegame file was deleted, update the list of files.
 * @param path Path of the deleted savegame.
 */
void LoadSaveGui::FileDeleted(std::string path)
{
	if (path.compare(0, SavegameDirectory().size(), SavegameDirectory()) != 0) return;
	path.erase(0, SavegameDirectory().size());
	for (auto it = this->all_files.begin(); it != this->all_files.end(); ++it) {
		if (it->filename == path) {
			this->all_files.erase(it);
			this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->SetItemCount(this->all_files.size());
			return;
		}
	}
}

/**
 * Sort the list of savegames. If the sort criterium is the same that already applies, reverse the order.
 * @param sort Sorting comparison functor to use.
 */
void LoadSaveGui::Sort(SortBy sort)
{
	if (sort == this->current_sort) {
		/* Already sorted, just invert the order. */
		std::reverse(this->all_files.begin(), this->all_files.end());
		return;
	}

	this->current_sort = sort;
	std::sort(this->all_files.begin(), this->all_files.end(), sort);

	this->SetWidgetPressed(LSW_SORT_FILE, this->current_sort == &SortByFilename);
	this->SetWidgetPressed(LSW_SORT_TIME, this->current_sort == &SortByTimestamp);
	this->SetWidgetPressed(LSW_SORT_NAME, this->current_sort == &SortByScenario);
	this->SetWidgetPressed(LSW_SORT_REV, this->current_sort == &SortByCompatibility);
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

/**
 * Find out which index, if any, in %all_files is currently selected.
 * @return The selected index or \c -1 for no selection.
 */
int LoadSaveGui::GetSelectedFileIndex() const
{
	const std::string filename = this->FinalFilename();
	for (uint i = 0; i < this->all_files.size(); ++i) {
		if (this->all_files[i].filename == filename) {
			return i;
		}
	}
	return -1;
}

bool LoadSaveGui::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	switch (key_code) {
		case WMKC_CONFIRM:
			this->OnClick(LSW_OK, Point16());
			return true;

		case WMKC_CURSOR_DOWN:
		case WMKC_CURSOR_UP:
		case WMKC_CURSOR_PAGEDOWN:
		case WMKC_CURSOR_PAGEUP: {
			if (this->all_files.empty()) return false;

			ScrollbarWidget *scrollbar = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR);
			const int old_index = this->GetSelectedFileIndex();
			int new_index = old_index;
			const int delta = (key_code == WMKC_CURSOR_PAGEUP || key_code == WMKC_CURSOR_PAGEDOWN) ? scrollbar->GetVisibleCount() : 1;
			if (key_code == WMKC_CURSOR_UP || key_code == WMKC_CURSOR_PAGEUP) {
				if (old_index < 0) {
					new_index = this->all_files.size() - 1;
				} else {
					new_index = std::max(0, old_index - delta);
				}
			} else {
				if (old_index < 0) {
					new_index = 0;
				} else {
					new_index = std::min<int>(old_index + delta, this->all_files.size() - 1);
				}
			}
			if (new_index == old_index) return false;

			this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetText(this->all_files[new_index].filename);

			if (static_cast<int>(scrollbar->GetStart()) > new_index) {
				scrollbar->SetStart(new_index);
			} else if (static_cast<int>(scrollbar->GetStart() + scrollbar->GetVisibleCount()) < new_index + 1) {
				scrollbar->SetStart(new_index + 1 - scrollbar->GetVisibleCount());
			}

			return true;
		}

		default:
			break;
	}
	return GuiWindow::OnKeyEvent(key_code, mod, symbol);
}

void LoadSaveGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case LSW_CANCEL:
			delete this;
			break;

		case LSW_SORT_FILE:
			this->Sort(SortByFilename);
			break;
		case LSW_SORT_TIME:
			this->Sort(SortByTimestamp);
			break;
		case LSW_SORT_NAME:
			this->Sort(SortByScenario);
			break;
		case LSW_SORT_REV:
			this->Sort(SortByCompatibility);
			break;

		case LSW_LIST_FILE:
		case LSW_LIST_TIME:
		case LSW_LIST_NAME:
		case LSW_LIST_REV:
		case LSW_LIST: {
			const int index = pos.y / ITEM_HEIGHT + this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
			if (index < 0 || index >= static_cast<int>(this->all_files.size())) break;

			this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetText(this->all_files.at(index).filename);
			break;
		}

		case LSW_DELETE: {
			const int sel = this->GetSelectedFileIndex();
			if (sel < 0) break;
			std::string path = SavegameDirectory();
			path += this->FinalFilename();
			ShowConfirmationPrompt(GUI_LOADSAVE_CONFIRM_TITLE, GUI_LOADSAVE_CONFIRM_DELETE, [path]() {
				RemoveFile(path);
				Window *w = GetWindowByType(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE);
				if (w != nullptr) static_cast<LoadSaveGui*>(w)->FileDeleted(path);
			});
			break;
		}

		case LSW_OK: {
			std::string path = SavegameDirectory();
			path += this->FinalFilename();
			const int sel = this->GetSelectedFileIndex();

			switch (this->type) {
				case SAVE:
					if (sel >= 0) {
						ShowConfirmationPrompt(GUI_LOADSAVE_CONFIRM_TITLE, GUI_LOADSAVE_CONFIRM_OVERWRITE, [path]() {
							_game_control.SaveGame(path);
							delete GetWindowByType(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE);
						});
						return;
					} else {
						_game_control.SaveGame(path);
					}
					break;
				case LOAD:
					if (sel < 0 || !this->all_files.at(sel).load_success) return;  // The file does not exist or is invalid.
					_game_control.LoadGame(path);
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
				_video.BlitText(pd.load_success ? _language.GetSgText(_language.GetStringByName(pd.scenario->name_key)) : "",
						_palette[pd.load_success ? TEXT_WHITE : TEXT_GREY], x, y, w, ALG_LEFT);
			};
			break;

		case LSW_LIST_REV:
			functor = [&x, &y](const PreloadData &pd) {
				const ImageData *imgdata = _sprite_manager.GetTableSprite(
						pd.load_success ? pd.revision == _freerct_revision ? SPR_GUI_LOADSAVE_OK : SPR_GUI_LOADSAVE_WARN : SPR_GUI_LOADSAVE_ERR);
				if (imgdata != nullptr) _video.BlitImage(Point32(x - ITEM_SPACING / 2, y - ITEM_SPACING / 2), imgdata);
			};
			break;

		default:
			NOT_REACHED();
	}

	for (size_t i = first_index; i < last_index; i++, y += ITEM_HEIGHT) {
		if (selected_filename == this->all_files[i].filename) {
			int sx = x - 2 * ITEM_SPACING;
			int sw = w + 4 * ITEM_SPACING;
			if (wid_num == LSW_LIST_REV) sw -= ITEM_SPACING;
			_video.FillRectangle(Rectangle32(sx, y - 2 * ITEM_SPACING, sw, ITEM_HEIGHT), _palette[COL_SERIES_START + COL_RANGE_BLUE * COL_SERIES_LENGTH + 1]);
		}
		functor(this->all_files[i]);
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
