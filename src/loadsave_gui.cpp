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

#include <cerrno>
#ifdef _WIN32
#ifdef _MSC_VER
#include <cstdio>
#else
#include <cstdint>
#endif
#endif
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#ifdef _WIN32
#include <dos.h>
#include <windows.h>
#ifdef _MSC_VER
#include <corecrt_io.h>
#define S_ISDIR(x) ((x & _S_IFDIR) ? 1 : 0)
#endif
#else  // not _WIN32
#include <fcntl.h>
#include <glob.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/**
 * List all files in a directory.
 * Code copied from Widelands (https://github.com/widelands/widelands/tree/master/src/io/filesystem/disk_filesystem.cc) with gratitude
 * (with some minor changes to make it compile here).
 * @param path Directory path.
 * @return All files in the directory.
 */
static std::set<std::string> ListDirectory(const std::string& path) {
	const std::string directory_ = ".";
	const std::string root_ = directory_;

#ifdef _WIN32
	std::string buf;
	struct _finddata_t c_file;
	intptr_t hFile;

	if (path.size())
		buf = directory_ + '\\' + path + "\\*";
	else
		buf = directory_ + "\\*";

	std::set<std::string> results;

	hFile = _findfirst(buf.c_str(), &c_file);
	if (hFile == -1)
		return results;

	std::string realpath = path;

	if (!realpath.empty()) {
		realpath.append("\\");
	}
	do {
		if ((strcmp(c_file.name, ".") == 0) || (strcmp(c_file.name, "..") == 0)) {
			continue;
		}
		const std::string filename = realpath + c_file.name;
		std::string result = filename.substr(root_.size() + 1);

		// Paths should not contain any windows line separators.
		boost::replace_all(result, "\\", "/");
		results.insert(result);
	} while (_findnext(hFile, &c_file) == 0);

	_findclose(hFile);

	return results;
#else
	std::string buf;
	glob_t gl;
	int32_t ofs;

	if (!path.empty()) {
		buf = directory_ + '/' + path + "/*";
		ofs = directory_.length() + 1;
	} else {
		buf = directory_ + "/*";
		ofs = directory_.length() + 1;
	}
	std::set<std::string> results;

	if (glob(buf.c_str(), 0, nullptr, &gl)) {
		return results;
	}

	for (size_t i = 0; i < gl.gl_pathc; ++i) {
		const std::string filename(&gl.gl_pathv[i][ofs]);
		results.insert(filename.substr(root_.size() + 1));
	}

	globfree(&gl);

	return results;
#endif
}

/**
 * %Game loading/saving gui.
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
	const Type type;                  ///< Type of this window.
	std::set<std::string> all_files;  ///< All files in the working directory.
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
static const uint ITEM_SPACING =  4;  ///< Spacing in the list.

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
				Widget(WT_EMPTY,          LSW_LIST,      COL_RANGE_BLUE),
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
	for (const std::string& name : ListDirectory(".")) {
		if (name.size() > 4 && name.compare(name.size() - 4, 4, ".fct") == 0) this->all_files.insert(name);
	}

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

void LoadSaveGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case LSW_CANCEL:
			delete this;
			break;

		case LSW_LIST: {
			const int index = pos.y / (ITEM_HEIGHT + ITEM_SPACING);
			if (index < 0) break;

			const int first_index = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
			if (index < first_index || index + first_index >= static_cast<int>(this->all_files.size())) break;

			auto selected = this->all_files.begin();
			std::advance(selected, index);
			uint8 *buffer = new uint8[selected->size() + 1];
			strcpy(reinterpret_cast<char*>(buffer), selected->c_str());
			this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->SetText(buffer);
			break;
		}

		case LSW_OK:
			switch (this->type) {
				case SAVE: {
					const char *filename = reinterpret_cast<const char*>(this->GetWidget<TextInputWidget>(LSW_TEXTFIELD)->GetText());
					const size_t filename_length = strlen(filename);
					char final_filename[filename_length + 5];
					strcpy(final_filename, filename);
					if (filename_length < 5 || strcmp(filename + filename_length - 4, ".fct") != 0) {
						strcpy(final_filename + filename_length, ".fct");
					}
					printf("NOCOM saving as '%s'\n", final_filename);
					if (this->all_files.count(final_filename)) {
						// NOCOM show confirmation dialogue
						printf("NOCOM Overwriting %s\n", final_filename);
					}
					_game_control.SaveGame(final_filename);
					delete this;
					break;
				}
				case LOAD:
					// NOCOM
					break;
			}
			break;

		default: break;
	}
}

void LoadSaveGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != LSW_LIST) return GuiWindow::DrawWidget(wid_num, wid);

	int x = this->GetWidgetScreenX(wid) + ITEM_SPACING;
	int y = this->GetWidgetScreenY(wid);
	const size_t first_index = this->GetWidget<ScrollbarWidget>(LSW_SCROLLBAR)->GetStart();
	const size_t last_index = std::min<size_t>(this->all_files.size(), first_index + ITEM_COUNT);
	auto iterator = this->all_files.begin();
	std::advance(iterator, first_index);

	for (size_t i = first_index; i < last_index; i++, iterator++, y += ITEM_HEIGHT + ITEM_SPACING) {
		_video.BlitText(reinterpret_cast<const uint8*>(iterator->c_str()), _palette[TEXT_WHITE], x, y, wid->pos.width - 2 * ITEM_SPACING, ALG_LEFT);
	}
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
