/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file text-buffer.cpp Text-handling functions. */

#include "stdafx.h"
#include "text_buffer.h"

TextBuffer::TextBuffer()
{
	this->current_position = 0;
}

TextBuffer::~TextBuffer()
{
}

void TextBuffer::SetText(const char *txt)
{
	this->text = txt;
	this->current_position = this->text.length();
}

const std::string &TextBuffer::GetText() const
{
	return this->text;
}

void TextBuffer::AppendText(const char *txt)
{
	if (std::string(this->text + txt).length() > max_length) return;
	this->text += txt;
	this->current_position = this->text.length();
}

void TextBuffer::InsertText(const char *txt)
{
	if (std::string(this->text + txt).length() > max_length) return;
	int prev_length = this->text.length();
	this->text.insert(this->current_position, txt);
	int inc_position = this->text.length() - prev_length;
	this->current_position += inc_position;
}

void TextBuffer::RemoveLastCharacter()
{
	if (this->text.empty()) return;
	this->text.pop_back();
	this->current_position--;
}

void TextBuffer::RemovePrevCharacter()
{
	if (this->text.empty() || this->current_position == 0) return;
	this->current_position--;
	this->text.erase(this->current_position, 1);
}

void TextBuffer::RemoveCurrentCharacter()
{
	if (this->text.empty()) return;
	this->text.erase(this->current_position, 1);
}
