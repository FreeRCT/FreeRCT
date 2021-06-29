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
	const Type type;
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
				Widget(WT_TEXT_INPUT, LSW_TEXTFIELD, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_CANCEL /* NOCOM */, STR_NULL),
				Intermediate(1, 2),
					Widget(WT_TEXT_PUSHBUTTON, LSW_CANCEL, COL_RANGE_BLUE), SetData(GUI_LOADSAVE_CANCEL, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, LSW_OK,     COL_RANGE_BLUE), SetData(STR_ARG1,            STR_NULL),

	EndContainer(),
};

LoadSaveGui::LoadSaveGui(const Type t) : GuiWindow(WC_LOADSAVE, ALL_WINDOWS_OF_TYPE), type(t)
{
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

		case LSW_LIST:
			// NOCOM
			break;

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
					// NOCOM check if file final_filename exists already
					printf("NOCOM saving as '%s'\n", final_filename);
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

	// NOCOM
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
