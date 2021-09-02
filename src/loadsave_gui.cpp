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
#include "fileio.h"
#include "rev.h"

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

private:
	std::string FinalFilename() const;

	const Type type;                  ///< Type of this window.
	std::set<std::string> all_files;  ///< All files in the working directory.
	std::string dir_sep;              ///< Directory separator.
};

/**
 * Widget numbers of the loading/saving Gui.
 * @ingroup gui_group
 */
enum LoadSaveWidgets {
	LSW_TITLEBAR,   ///< Window title bar.
	LSW_LIST,       ///< List of all files.
	LSW_SCROLLBAR,  ///< Scrollbar for the list.
	LSW_TEXTFIELD,  ///< Text field for the filename.
	LSW_OK,         ///< Confirmation button.
	LSW_CANCEL,     ///< Cancel button.
};

static const uint ITEM_COUNT   =  8;  ///< Number of files to display in the list.
static const uint ITEM_HEIGHT  = 20;  ///< Height of one item in the list.
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
				Widget(WT_EMPTY, LSW_LIST, COL_RANGE_BLUE),
						SetFill(0, ITEM_HEIGHT), SetResize(0, ITEM_HEIGHT), SetMinimalSize(300, ITEM_COUNT * ITEM_HEIGHT),
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
	dr->OpenPath(freerct_userdata_prefix());
	this->dir_sep += dr->dir_sep;
	const char *str;
	while ((str = dr->NextEntry()) != nullptr) {
		std::string name(str);
		if (name.size() > 4 && name.compare(name.size() - 4, 4, ".fct") == 0) this->all_files.insert(name.substr(name.find_last_of(this->dir_sep) + 1));
	}
	dr->ClosePath();

	this->SetupWidgetTree(_loadsave_gui_parts, lengthof(_loadsave_gui_parts));
	this->SetScrolledWidget(LSW_LIST, LSW_SCROLLBAR);
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

void LoadSaveGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case LSW_CANCEL:
			delete this;
			break;

		case LSW_LIST: {
			const int index = pos.y / ITEM_HEIGHT;
			if (index < 0) break;

			const int first_index = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
			if (index < first_index || index + first_index >= static_cast<int>(this->all_files.size())) break;

			auto selected = this->all_files.begin();
			std::advance(selected, index);
			this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetText(*selected);
			break;
		}

		case LSW_OK: {
			const std::string filename = this->FinalFilename();
			std::string path = freerct_userdata_prefix();
			path += this->dir_sep;
			path += filename;
			switch (this->type) {
				case SAVE:
					if (this->all_files.count(filename)) {
						/* \todo Show a confirmation dialogue to ask the user whether the file should be overwritten. */
					}
					_game_control.SaveGame(path.c_str());
					break;
				case LOAD:
					if (!this->all_files.count(filename)) return;  // The file does not exist.
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
	if (wid_num != LSW_LIST) return GuiWindow::DrawWidget(wid_num, wid);

	int x = this->GetWidgetScreenX(wid) + 2 * ITEM_SPACING;
	int y = this->GetWidgetScreenY(wid);
	const int w = wid->pos.width - 4 * ITEM_SPACING;
	const size_t first_index = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
	const size_t last_index = std::min<size_t>(this->all_files.size(), first_index + ITEM_COUNT);
	auto iterator = this->all_files.begin();
	std::advance(iterator, first_index);
	const std::string selected_filename = this->FinalFilename();

	for (size_t i = first_index; i < last_index; i++, iterator++, y += ITEM_HEIGHT) {
		if (selected_filename == *iterator) {
			_video.FillRectangle(Rectangle32(x - ITEM_SPACING, y, w + 2 * ITEM_SPACING, ITEM_HEIGHT),
					_palette[COL_SERIES_START + COL_RANGE_BLUE * COL_SERIES_LENGTH + 1]);
		}
		_video.BlitText(reinterpret_cast<const uint8*>(iterator->c_str()), _palette[TEXT_WHITE], x, y, w, ALG_LEFT);
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
