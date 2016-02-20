/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file text_buffer.h Text handling functions. */

#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <string>

class TextBuffer {
	public:
		TextBuffer();
		~TextBuffer();

		void SetText(const char *txt);
		const std::string &GetText() const;
		void AppendText(const char *txt);
		void InsertText(const char *txt);
		void RemoveLastCharacter();
		void RemovePrevCharacter();
		void RemoveCurrentCharacter();

		void SetPosition(int position) { this->current_position = position; };
		const int GetPosition() const { return this->current_position; };
		void IncPosition() { if (this->current_position < this->text.length()) this->current_position++; };
		void DecPosition() { if (this->current_position > 0) this->current_position--; };

		void SetMaxLength(uint max_length) { this->max_length = max_length; };
	private:
		std::string text;
		uint current_position;
		uint max_length;
};

#endif
