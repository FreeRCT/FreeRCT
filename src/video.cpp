/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.cpp Graphics system handling. */

#include <GL/glew.h>  // This include must come first!

#include "video.h"
#include "gamecontrol.h"
#include "rev.h"
#include "sprite_data.h"
#include "sprite_store.h"
#include "string_func.h"
#include "window.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#ifdef WEBASSEMBLY
#include <emscripten.h>
#endif

#if __has_include(<ft2build.h>)
#include <ft2build.h>
#elif __has_include(<freetype2/ft2build.h>)
#include <freetype2/ft2build.h>
#elif __has_include(<freetype2/freetype/ft2build.h>)
#include <freetype2/freetype/ft2build.h>
#else
#error "Freetype not found"
#endif
#include FT_FREETYPE_H

VideoSystem _video;           ///< The #VideoSystem singleton instance.
TextRenderer _text_renderer;  ///< The #TextRenderer singleton instance.

/* Text renderer implementation. */

constexpr const uint32 BEARING_CHARACTER = 'H';                ///< An arbitrary ASCII character whose bearing to use as reference for text alignment.
constexpr const uint32 CHARACTER_NOT_FOUND[] = {0xFFFD, '?'};  ///< Characters that may represent a missing character glyph.
constexpr const float FONT_PADDING_V = 0.3f;                   ///< Total vertical padding around all text, relative to the font size.
constexpr const float FONT_PADDING_H = 0.2f;                   ///< Total horizontal padding around all text, relative to the font size.

/** Initialize the text renderer. */
void TextRenderer::Initialize()
{
	this->shader = _video.ConfigureShader("text");
	glUniform1i(glGetUniformLocation(this->shader, "text"), 0);
	glGenVertexArrays(1, &this->vao);
	glGenBuffers(1, &this->vbo);
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

/**
 * Find the next-highest codepoint that we want to be able to render, skipping over codepoints we don't care about,
 * @param c Unicode codepoint from which to start searching.
 * @return Next unicode codepoint to render or \c 0xffffffff if none.
 */
static uint32 NextCodepointToLoad(uint32 c)
{
	if (c < 0x303f) return c + 1;     // Lots of interesting characters in the lower ranges.
	/* \todo When we get translations with non-latin character sets, these may need to be added here as well. */
	c = std::max<uint32>(c, 0xFD3E);  // Start of the next moderately useful character block.
	if (c < 0xfffd) return c + 1;     // Some more interesting characters in the high four-digit ranges.
	return 0xffffffff;                // Nothing of interest in the very high ranges.
}

/**
 * Load a font. This font will be used for all subsequent rendering operations. Any previously loaded font will be forgotten.
 * @param font_path File path of the font file.
 * @param font_size Size of the font to load.
 */
void TextRenderer::LoadFont(const std::string &font_path, GLuint font_size)
{
	this->font_size = font_size;

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		error("TextRenderer::LoadFont: Could not init FreeType Library");
	}
	FT_Face face;
	if (FT_New_Face(ft, font_path.c_str(), 0, &face)) {
		error("TextRenderer::LoadFont: Failed to load font '%s'", font_path.c_str());
	}

	FT_Select_Charmap(face, FT_ENCODING_UNICODE);

	FT_Set_Pixel_Sizes(face, 0, font_size);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Load all characters we may need. */
	for (uint32 codepoint = 1; codepoint <= MAX_CODEPOINT; codepoint = NextCodepointToLoad(codepoint)) {
		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
			this->characters[codepoint].valid = false;
			char buffer[] = {0, 0, 0, 0, 0};
			EncodeUtf8Char(codepoint, buffer);
			printf("WARNING: Failed to load glyph U+%04x '%s'\n", codepoint, buffer);
			continue;
		}

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_R8,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		this->characters[codepoint] = {
			texture,
			Point16(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			Point16(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<GLuint>(face->glyph->advance.x),
			true
		};
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	/* Check that we have at least a bearing character and a glyph for invalid characters. */
	std::string sample_text = {BEARING_CHARACTER};
	const char *c = sample_text.c_str();
	size_t i = 1;
	this->GetFontGlyph(&c, i);  // Checks that the bearing character glyph exists.
	this->GetFontGlyph(&c, i);  // Now i is 0, so this checks that an Invalid glyph is present.
}

/**
 * Look up the font glygh to use for a given character.
 * If the current font does not have a matching glygh, a default value is returned.
 * @param text [inout] Pointer to the UTF-8 encoded character. Will be advanced to the next character.
 * @param length [inout] Number of bytes left in the text. Will be decremented by the number of bytes by which the text is advanced.
 * @return Glyph to use.
 */
const TextRenderer::FontGlyph &TextRenderer::GetFontGlyph(const char **text, size_t &length) const
{
	uint32 codepoint;
	int bytes_read = length < 1 ? 0 : DecodeUtf8Char(*text, length, &codepoint);

	if (bytes_read < 1) {
		if (length > 0) {
			++*text;
			++length;
		}
		/* Fall though to default glyph selection. */
	} else {
		*text += bytes_read;
		length -= bytes_read;
		if (codepoint <= MAX_CODEPOINT && this->characters[codepoint].valid) return this->characters[codepoint];

		/* The codepoint is valid, but we don't have a glyph for it. Fall though to default glyph selection. */
	}

	static const FontGlyph *default_glyph = nullptr;
	if (default_glyph != nullptr) return *default_glyph;
	for (uint32 c : CHARACTER_NOT_FOUND) {
		if (c <= MAX_CODEPOINT && this->characters[c].valid) {
			default_glyph = &this->characters[c];
			return *default_glyph;
		}
	}

	error("The font is missing essential characters\n");
}

/**
 * Render text to the screen.
 * @param text Text to draw.
 * @param x Horizontal screen position where to draw the text.
 * @param y Vertical screen position where to draw the text.
 * @param max_width Maximum available width.
 * @param colour Colour in which to draw the text.
 * @param scale Scaling factor for the text size.
 */
void TextRenderer::Draw(const std::string &text, float x, float y, float max_width, uint32 colour, float scale)
{
	if (text.empty()) return;

	glUseProgram(this->shader);
	glUniform1f(glGetUniformLocation(this->shader, "text_colour_r"), FGetR(colour));
	glUniform1f(glGetUniformLocation(this->shader, "text_colour_g"), FGetG(colour));
	glUniform1f(glGetUniformLocation(this->shader, "text_colour_b"), FGetB(colour));
	glUniform1f(glGetUniformLocation(this->shader, "text_colour_a"), FGetA(colour));

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(this->vao);

	/* Insert some padding around the text.
	 * Horizontal spacing is distributed equally on both sides of the text,
	 * but we want more vertical spacing above than below.
	 */
	y += 0.75f * FONT_PADDING_V * this->font_size * scale;
	x += FONT_PADDING_H * 0.5f;
	max_width -= FONT_PADDING_H;

	size_t text_length = text.size();
	for (const char *c = text.c_str(); *c != '\0';) {
		const FontGlyph &fg = this->GetFontGlyph(&c, text_length);

		GLfloat x1 = x + fg.bearing.x * scale;
		GLfloat y1 = y - (fg.bearing.y - this->characters[BEARING_CHARACTER].bearing.y) * scale;
		GLfloat x2 = x1 + fg.size.x * scale;
		GLfloat y2 = y1 + fg.size.y * scale;

		max_width -= x2 - x1;
		if (max_width < 0) break;

		/* Prevent fuzzy rendering. */
		x1 = round(x1); y1 = round(y1); x2 = round(x2); y2 = round(y2);

		_video.CoordsToGL(&x1, &y1);
		_video.CoordsToGL(&x2, &y2);

		GLfloat vertices[6][4] = {
			{ x1, y2,   0.f, 1.f },
			{ x2, y1,   1.f, 0.f },
			{ x1, y1,   0.f, 0.f },

			{ x1, y2,   0.f, 1.f },
			{ x2, y2,   1.f, 1.f },
			{ x2, y1,   1.f, 0.f }
		};
		glBindTexture(GL_TEXTURE_2D, fg.texture_id);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (fg.advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * Estimate the bounding rectangle of a string.
 * @param text Text to estimate.
 * @param add_padding Include text padding in the result.
 * @param scale Scaling factor for the text size.
 */
PointF TextRenderer::EstimateBounds(const std::string &text, bool add_padding, float scale) const
{
	float x = 0;
	float width = 0;
	float height = add_padding ? this->font_size * scale : 0;
	size_t text_length = text.size();
	for (const char *c = text.c_str(); *c != '\0';) {
		const FontGlyph &fg = this->GetFontGlyph(&c, text_length);
		GLfloat xpos = x + fg.bearing.x * scale;
		GLfloat ypos = (fg.bearing.y - this->characters[BEARING_CHARACTER].bearing.y) * scale;
		GLfloat w = fg.size.x * scale;
		GLfloat h = fg.size.y * scale;
		width = std::max(width, xpos + w);
		height = std::max(height, ypos + h);
		x += (fg.advance >> 6) * scale;
	}
	if (add_padding) {
		width += FONT_PADDING_H * this->font_size * scale;
		height += FONT_PADDING_V * this->font_size * scale;
	}
	return PointF(width, height);
}

/**
 * Get the height of a line of text.
 * @return Height of text.
 */
GLuint TextRenderer::GetTextHeight() const
{
	return this->font_size * (FONT_PADDING_V + 1.f);
}

/* Graphics framework implementation. */

#ifdef WEBASSEMBLY
/** Emscripten definitions to query the size of the canvas. */
EM_JS(int, GetEmscriptenCanvasWidth , (), { return canvas.clientWidth ; });
EM_JS(int, GetEmscriptenCanvasHeight, (), { return canvas.clientHeight; });
#endif

/**
 * Called by OpenGL when the window size changes.
 * @param window Window whose size changed.
 * @param new_w New width.
 * @param new_h New height.
 */
void VideoSystem::FramebufferSizeCallback([[maybe_unused]] GLFWwindow *window, int new_w, int new_h)
{
	assert(window == _video.window);
	assert(new_w >= 0);
	assert(new_h >= 0);

	_video.width = new_w;
	_video.height = new_h;
	_video.UpdateClip();

	_window_manager.RepositionAllWindows(_video.width, _video.height);
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_RESOLUTION_CHANGED, 0);
}

/**
 * Called by OpenGL when a key was pressed or released.
 * @param window Window in which the event occurred.
 * @param key Constant of the pressed key.
 * @param scancode Scancode of the pressed key.
 * @param action Whether the mouse button was pressed or released.
 * @param mods Bitset of modifier keys.
 */
void VideoSystem::KeyCallback([[maybe_unused]] GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action, int mods)
{
	assert(window == _video.window);
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

	WmKeyMod mod_mask = WMKM_NONE;
	if ((mods & GLFW_MOD_CONTROL) != 0) mod_mask |= WMKM_CTRL;
	if ((mods & GLFW_MOD_SHIFT  ) != 0) mod_mask |= WMKM_SHIFT;
	if ((mods & GLFW_MOD_ALT    ) != 0) mod_mask |= WMKM_ALT;

#ifdef GLFW_MOD_NUM_LOCK
	const bool numlock = (mods & GLFW_MOD_NUM_LOCK) != 0;
#else
	/* If GLFW doesn't provide num lock state querying, we have to assume it's always on so we don't get duplicate key presses. */
	const bool numlock = true;
#endif

	WmKeyCode key_code;
	std::string symbol;
	switch (key) {
		case GLFW_KEY_KP_6:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_RIGHT:
			key_code = WMKC_CURSOR_RIGHT;
			break;

		case GLFW_KEY_KP_4:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_LEFT :
			key_code = WMKC_CURSOR_LEFT;
			break;

		case GLFW_KEY_KP_2:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_DOWN :
			key_code = WMKC_CURSOR_DOWN;
			break;

		case GLFW_KEY_KP_8:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_UP   :
			key_code = WMKC_CURSOR_UP;
			break;

		case GLFW_KEY_KP_3:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_PAGE_DOWN:
			key_code = WMKC_CURSOR_PAGEDOWN;
			break;

		case GLFW_KEY_KP_9:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_PAGE_UP  :
			key_code = WMKC_CURSOR_PAGEUP;
			break;

		case GLFW_KEY_KP_7:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_HOME     :
			key_code = WMKC_CURSOR_HOME;
			break;

		case GLFW_KEY_KP_1:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_END      :
			key_code = WMKC_CURSOR_END;
			break;

		case GLFW_KEY_KP_DECIMAL:
			if (numlock) return;
			/* FALL-THROUGH */
		case GLFW_KEY_DELETE   :
			key_code = WMKC_DELETE;
			break;

		case GLFW_KEY_BACKSPACE:
			key_code = WMKC_BACKSPACE;
			break;

		case GLFW_KEY_ESCAPE:
			key_code = WMKC_CANCEL;
			break;

		case GLFW_KEY_ENTER:
		case GLFW_KEY_KP_ENTER:
			key_code = WMKC_CONFIRM;
			break;

		default:
			if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
				key_code = static_cast<WmKeyCode>(WMKC_FN_BEGIN + 1 + (key - GLFW_KEY_F1));
				break;
			}

			/* Text input events with modifiers may or may not be recognized as a text event, so we need to convert them
			 * manually. Using Shift but no other modifiers is an exception as this simply generates uppercase text
			 * input. All keysyms that correspond to an ASCII character have the same integer value as this character;
			 * all others are larger than the largest valid ASCII character (which is 0x7F).
			 */
			if ((mod_mask & ~WMKM_SHIFT) != 0 && key <= 0x7F) {
				symbol += tolower(key);
				key_code = WMKC_SYMBOL;
				break;
			}
			return;
	}

	_window_manager.KeyEvent(key_code, mod_mask, symbol);
}

/**
 * Called by OpenGL when text is entered.
 * @param window Window in which the event occurred.
 * @param codepoint The unicode character.
 */
void VideoSystem::TextCallback([[maybe_unused]] GLFWwindow *window, uint32 codepoint)
{
	assert(window == _video.window);
	char buffer[] = {0, 0, 0, 0, 0};
	EncodeUtf8Char(codepoint, buffer);
	_window_manager.KeyEvent(WMKC_SYMBOL, WMKM_NONE, buffer);
}

/**
 * Called by OpenGL when the mouse was moved.
 * @param window Window in which the event occurred.
 * @param x New mouse position X coordinate.
 * @param y New mouse position Y coordinate.
 */
void VideoSystem::MouseMoveCallback([[maybe_unused]] GLFWwindow *window, double x, double y)
{
	assert(window == _video.window);
	_video.mouse_x = std::max(0.0, std::min<double>(x, _video.width));
	_video.mouse_y = std::max(0.0, std::min<double>(y, _video.height));
	_window_manager.MouseMoveEvent();
}

/**
 * Called by OpenGL when the mouse wheel was moved.
 * @param window Window in which the event occurred.
 * @param xdelta Amount by which the wheel was moved in the horizontal direction..
 * @param ydelta Amount by which the wheel was moved in the vertical direction.
 */
void VideoSystem::ScrollCallback([[maybe_unused]] GLFWwindow *window, [[maybe_unused]] double xdelta, double ydelta)
{
	assert(window == _video.window);
	if (abs(ydelta) < 0.01) return;
	_window_manager.MouseWheelEvent((ydelta > 0) ? 1 : -1);
}

/**
 * Called by OpenGL when a mouse click occurs.
 * @param window Window in which the event occurred.
 * @param button Mouse button constant.
 * @param action Whether the mouse button was pressed or released.
 * @param mods Bitset of modifier keys.
 */
void VideoSystem::MouseClickCallback([[maybe_unused]] GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) {
	assert(window == _video.window);

	MouseButtons mouse_button;
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT : mouse_button = MB_RIGHT ; break;
		case GLFW_MOUSE_BUTTON_LEFT  : mouse_button = MB_LEFT  ; break;
		case GLFW_MOUSE_BUTTON_MIDDLE: mouse_button = MB_MIDDLE; break;
		default:
			return;
	}

	_window_manager.MouseButtonEvent(mouse_button, action == GLFW_PRESS ? WMEM_PRESS : WMEM_RELEASE);
}

/**
 * Start or stop dragging the mouse.
 * @param button The mouse button to update,
 * @param dragging The mouse button that is being dragged.
 * @param hide_cursor Hide the mouse cursor while dragging.
 */
void VideoSystem::SetMouseDragging(MouseButtons button, bool dragging, bool hide_cursor)
{
	assert(!hide_cursor || dragging);  // Can't hide the cursor when not dragging.
	if (dragging) {
		this->mouse_dragging |= button;
	} else {
		this->mouse_dragging &= ~button;
	}
	glfwSetInputMode(window, GLFW_CURSOR, hide_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

/** Shut down the video system. */
void VideoSystem::Shutdown()
{
	glfwTerminate();
}

/**
 * Initialize the graphics system.
 * @param font Font file to load.
 * @param font_size Size in which to load the font.
 */
void VideoSystem::Initialize(const std::string &font, int font_size)
{
	if (!glfwInit()) error("Failed to initialize GLFW\n");

	/* Create a window. */
	glfwWindowHint(GLFW_SAMPLES, 4);  // 4x antialiasing.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // Require OpenGL 3.3 or higher.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	this->width = 800;
	this->height = 600;
	this->mouse_x = this->width / 2;
	this->mouse_y = this->height / 2;
	this->mouse_dragging = MB_NONE;

	std::string caption = "FreeRCT ";
	caption += _freerct_revision;
	this->window = glfwCreateWindow(this->width, this->height, caption.c_str(), nullptr, nullptr);
	if (this->window == nullptr) {
		glfwTerminate();
		error("Failed to open GLFW window\n");
	}

	glfwMakeContextCurrent(this->window);
	if (glewInit() != GLEW_OK) error("Failed to initialize GLEW\n");

	this->UpdateClip();
	glfwSetFramebufferSizeCallback(this->window, FramebufferSizeCallback);
#ifndef WEBASSEMBLY
	glfwSetInputMode(this->window, GLFW_STICKY_KEYS, GL_TRUE);
#endif
	glfwSetCursorPosCallback(this->window, MouseMoveCallback);
	glfwSetScrollCallback(this->window, ScrollCallback);
	glfwSetMouseButtonCallback(this->window, MouseClickCallback);
	glfwSetKeyCallback(this->window, KeyCallback);
	glfwSetCharCallback(this->window, TextCallback);
	GLFWimage img{WINDOW_ICON_WIDTH, WINDOW_ICON_HEIGHT, _icon_data.get()};
	glfwSetWindowIcon(this->window, 1, &img);

	/* Prepare the window. */
	glClearColor(0.f, 0.f, 0.f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_POINT_SMOOTH);
	glGetError();  // Clear error messages.

	/* List available resolutions. */
	{
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		int count;
		const GLFWvidmode *modes = glfwGetVideoModes(monitor, &count);
		for (int i = 0; i < count; ++i) this->resolutions.emplace(modes[i].width, modes[i].height);
	}

	/* Initialize basic rendering functionality. */
	glGenVertexArrays(1, &this->vao);
	glGenBuffers(1, &this->vbo);
	glGenBuffers(1, &this->ebo);

	this->colour_shader = this->ConfigureShader("colour");
	this->image_shader = this->ConfigureShader("image");

	/* Initialize the text renderer. */
	_text_renderer.Initialize();
	_text_renderer.LoadFont(font, font_size);

	/* Initialize remaining data structures. */
	this->last_frame = std::chrono::high_resolution_clock::now();
	this->cur_frame = this->last_frame;
	this->average_frametime = 1;
}

/** Run the main loop. */
void VideoSystem::MainLoop()
{
	while (this->MainLoopDoCycle());
}

/** Perform one cycle of the main loop. */
/* static */ void VideoSystem::MainLoopCycle()
{
	_video.MainLoopDoCycle();
}

/**
 * Perform one cycle of the main loop.
 * @return The game has not ended yet.
 */
bool VideoSystem::MainLoopDoCycle()
{
	constexpr uint32 FRAME_DELAY = 30;        ///< Minimum number of milliseconds between two frames.
	constexpr double AVERAGE_FPS_STEPS = 15;  ///< Number of frame iterations in the average framerate computation.
	this->last_frame = this->cur_frame;
	this->cur_frame = std::chrono::high_resolution_clock::now();
	this->average_frametime = ((this->average_frametime * AVERAGE_FPS_STEPS) + Delta(this->last_frame, this->cur_frame)) / (AVERAGE_FPS_STEPS + 1);

#ifdef WEBASSEMBLY
	this->SetResolution({GetEmscriptenCanvasWidth(), GetEmscriptenCanvasHeight()});
#endif

	/* Handle input events. */
	glfwPollEvents();

	/* Prepare for the next rendering step. */
	glClear(GL_COLOR_BUFFER_BIT);

	/* Progress the game. */
	OnNewFrame(FRAME_DELAY);
	_game_control.DoNextAction();
	if (!_game_control.running || glfwWindowShouldClose(this->window)) return false;

	/* Cap the FPS rate. */
	double time = Delta(this->cur_frame);
	if (time < FRAME_DELAY) std::this_thread::sleep_for(Duration(FRAME_DELAY - time));

	return true;
}

/** Finish repainting, perform the final steps. */
void VideoSystem::FinishRepaint()
{
	glfwSwapBuffers(this->window);
}

/**
 * Calculate the current framerate.
 * @return Frames per second.
 */
double VideoSystem::FPS() const
{
	double time = Delta(this->last_frame, this->cur_frame);
	return time > 0 ? (1000.0 / time) : 0;
}

/**
 * Calculate the average framerate.
 * @return Frames per second.
 */
double VideoSystem::AvgFPS() const
{
	return 1000.0 / this->average_frametime;
}

/**
 * Change the resolution of the game window.
 * @param res Resolution to set the screen to.
 */
void VideoSystem::SetResolution(const Point32 &res)
{
	glfwSetWindowSize(this->window, res.x, res.y);
}

/**
 * Set the current clipping area.
 * @param rect New clipping area.
 * @see #PopClip
 */
void VideoSystem::PushClip(const Rectangle32 &rect)
{
	assert(rect.width > 0 && rect.height > 0);
	this->clip.push_back(rect);
	this->UpdateClip();
}

/**
 * Restore the clipping area.
 * @see #PushClip
 */
void VideoSystem::PopClip()
{
	assert(!this->clip.empty());
	this->clip.pop_back();
	this->UpdateClip();
}

/** Update the current clipping area. */
void VideoSystem::UpdateClip()
{
	float x, y, w, h;
	if (this->clip.empty()) {
		x = 0;
		y = 0;
		w = this->width;
		h = this->height;
	} else {
		w = this->clip.back().width;
		h = this->clip.back().height;
		x = this->clip.back().base.x;
		y = this->height - h - this->clip.back().base.y;
	}
	glViewport(x, y, w, h);
}

/**
 * Convert a coordinate from the window coordinate system to OpenGL's coordinate system.
 * @param x [inout] X coordinate.
 * @param y [inout] Y coordinate.
 */
void VideoSystem::CoordsToGL(float *x, float *y) const {
	float w, h;
	if (this->clip.empty()) {
		w = this->width;
		h = this->height;
	} else {
		w = this->clip.back().width;
		h = this->clip.back().height;
		*x -= this->clip.back().base.x;
		*y -= this->clip.back().base.y;
	}

	*x = 2.0f * *x / w - 1.0f;
	*y = 1.0f - 2.0f * *y / h;
}

/**
 * Load and configure an OpenGL shader.
 * @param name Name of the shader.
 * @return Handle of the created shader.
 */
GLuint VideoSystem::ConfigureShader(const std::string &name)
{
	std::string v = "data";
	std::string f = "data";
	v += DIR_SEP;
	f += DIR_SEP;
	v += "shaders";
	f += "shaders";
	v += DIR_SEP;
	f += DIR_SEP;
	v += name;
	f += name;
	v += ".vp";
	f += ".fp";
	return this->LoadShaders(FindDataFile(v).c_str(), FindDataFile(f).c_str());
}

/**
 * Load an OpenGL shader pair.
 * @param vp Path to the vertext shader.
 * @param fp Path to the fragment shader.
 * @return Handle of the loaded shader pair.
 */
GLuint VideoSystem::LoadShaders(const char *vp, const char *fp)
{
	/* Create the shaders. */
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	/* Read the Vertex Shader code from the file. */
	std::string vertex_shader_code;
	std::ifstream vertex_shader_stream(vp, std::ios::in);
	if (vertex_shader_stream.is_open()) {
		std::stringstream sstr;
		sstr << vertex_shader_stream.rdbuf();
		vertex_shader_code = sstr.str();
		vertex_shader_stream.close();
	} else {
		error("ERROR: Unable to open shader '%s'!\n", vp);
	}

	/* Read the Fragment Shader code from the file. */
	std::string fragment_shader_code;
	std::ifstream fragment_shader_stream(fp, std::ios::in);
	if (fragment_shader_stream.is_open()) {
		std::stringstream sstr;
		sstr << fragment_shader_stream.rdbuf();
		fragment_shader_code = sstr.str();
		fragment_shader_stream.close();
	}

	GLint result = GL_FALSE;
	int info_log_length;

	/* Compile Vertex Shader. */
	char const* vertex_source_pointer = vertex_shader_code.c_str();
	glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr);
	glCompileShader(vertex_shader_id);

	/* Check Vertex Shader. */
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector<char> vertex_shader_error_message(info_log_length + 1);
		glGetShaderInfoLog(vertex_shader_id, info_log_length, nullptr, vertex_shader_error_message.data());
		error("Compile error in shader %s: %s\n", vp, vertex_shader_error_message.data());
	}

	/* Compile Fragment Shader. */
	char const* FragmentSourcePointer = fragment_shader_code.c_str();
	glShaderSource(fragment_shader_id, 1, &FragmentSourcePointer, nullptr);
	glCompileShader(fragment_shader_id);

	/* Check Fragment Shader. */
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector<char> fragment_shader_error_message(info_log_length + 1);
		glGetShaderInfoLog(fragment_shader_id, info_log_length, nullptr, fragment_shader_error_message.data());
		error("Compile error in shader %s: %s\n", fp, fragment_shader_error_message.data());
	}

	/* Link the program. */
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);

	/* Check the program. */
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector<char> program_error_message(info_log_length + 1);
		glGetProgramInfoLog(program_id, info_log_length, nullptr, program_error_message.data());
		error("Linking error in shader pair %s/%s: %s\n", vp, fp, program_error_message.data());
	}

	glDetachShader(program_id, vertex_shader_id);
	glDetachShader(program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	return program_id;
}

/**
 * Create a texture for the given image if one did not exist yet.
 * @param img Image to load.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 * @return The image's texture.
 */
GLuint VideoSystem::GetImageTexture(const ImageData *img, const Recolouring &recolour, GradientShift shift) {
	std::pair<const ImageData*, RecolourData> map_key(img, RecolourData(shift, recolour.ToCondensed()));
	const auto it = this->image_textures.find(map_key);
	if (it != this->image_textures.end()) return it->second;

	GLuint t = 0;
	glGenTextures(1, &t);
	glBindTexture(GL_TEXTURE_2D, t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	std::unique_ptr<uint8[]> rgba = img->GetRecoloured(shift, recolour);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.get());
	this->image_textures.emplace(map_key, t);
	return t;
}

/**
 * Draw an image to the screen.
 * @param pos Where to draw the image's centre.
 * @param img Image to draw.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 * @param col RGBA colour to overlay over the image.
 */
void VideoSystem::BlitImage(const Point32 &pos, const ImageData *img, const Recolouring &recolour, GradientShift shift, uint32 col)
{
	this->DoDrawImage(this->GetImageTexture(img, recolour, shift),
			pos.x + img->xoffset             , pos.y + img->yoffset,
			pos.x + img->xoffset + img->width, pos.y + img->yoffset + img->height, col);
}

/**
 * Tile an image across an area.
 * @param img Image to draw.
 * @param rect Rectangle to fill.
 * @param tile_hor Tile the image horizontally. If \c false, stretch the image instead.
 * @param tile_vert Tile the image vertically. If \c false, stretch the image instead.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 * @param col RGBA colour to overlay over the image.
 * @note The image's offset is ignored.
 */
void VideoSystem::TileImage(const ImageData *img, const Rectangle32 &rect, bool tile_hor, bool tile_vert,
		const Recolouring &recolour, GradientShift shift, uint32 col)
{
	this->DoDrawImage(this->GetImageTexture(img, recolour, shift),
			rect.base.x, rect.base.y, rect.base.x + static_cast<float>(rect.width), rect.base.y + static_cast<float>(rect.height), col,
			WXYZPointF(0.0f, 0.0f,
					tile_vert ? static_cast<float>(rect.height) / img->height : 1.0f,
					tile_hor ? static_cast<float>(rect.width) / img->width : 1.0f
				));
}

/**
 * Get the text-size of a number between \a smallest and \a biggest.
 * @param smallest Smallest possible number to display.
 * @param biggest Biggest possible number to display.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void VideoSystem::GetNumberRangeSize(int64 smallest, int64 biggest, int *width, int *height)
{
	if (width == nullptr && height == nullptr) return;
	assert(smallest <= biggest);

	if (width != nullptr) *width = 0;
	if (height != nullptr) *height = 0;
	for (; smallest <= biggest; ++smallest) {
		PointF vec = _text_renderer.EstimateBounds(std::to_string(smallest));
		if (width != nullptr) *width = std::max<int>(*width, vec.x);
		if (height != nullptr) *height = std::max<int>(*height, vec.y);
	}
}

/**
 * Get the text-size of a string.
 * @param text Text to calculate.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 * @param add_padding Include text padding in the result.
 */
void VideoSystem::GetTextSize(const std::string &text, int *width, int *height, bool add_padding)
{
	if (width == nullptr && height == nullptr) return;
	PointF vec = _text_renderer.EstimateBounds(text, add_padding);
	if (width != nullptr) *width = vec.x;
	if (height != nullptr) *height = vec.y;
}

/**
 * Blit text to the screen.
 * @param text Text to display.
 * @param colour Colour of the text.
 * @param xpos Absolute horizontal position at the display.
 * @param ypos Absolute vertical position at the display.
 * @param width Available width of the text (in pixels).
 * @param align Horizontal alignment of the string.
 */
void VideoSystem::BlitText(const std::string &text, uint32 colour, int xpos, int ypos, int width, Alignment align)
{
	float x = xpos;
	if (align != ALG_LEFT) {
		PointF vec = _text_renderer.EstimateBounds(text);
		if (align == ALG_RIGHT) {
			x += width - vec.x;
		} else {
			x += (width - vec.x) / 2.f;
		}
	}

	_text_renderer.Draw(text, x, ypos, width, colour);
}

/**
 * Draw an image on the screen.
 * @param texture Texture ID to draw.
 * @param x1 Upper left destination X coordinate of the image, in window space.
 * @param y1 Upper left destination Y coordinate of the image, in window space.
 * @param x2 Lower right destination X coordinate of the image, in window space.
 * @param y2 Lower right destination Y coordinate of the image, in window space.
 * @param col RGBA colour to overlay over the image.
 * @param tex Texture coordinate rectangle to clip, in GL space.
 */
void VideoSystem::DoDrawImage(GLuint texture, float x1, float y1, float x2, float y2, uint32 col, const WXYZPointF &tex)
{
	this->CoordsToGL(&x1, &y1);
	this->CoordsToGL(&x2, &y2);
	float vertices[] = {
		// positions  // colours                                      // texture coords
		x2, y1, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col), tex.z, tex.w, // top right
		x2, y2, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col), tex.z, tex.y, // bottom right
		x1, y2, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col), tex.x, tex.y, // bottom left
		x1, y1, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col), tex.x, tex.w  // top left
	};
	GLuint indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUseProgram(this->image_shader);
	glBindVertexArray(this->vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

/**
 * Render triangles in a solid colour.
 * @param points Vector of triangle coordinates.
 * @param col RGBA colour to use.
 */
void VideoSystem::DoDrawPlainColours(const std::vector<Point<float>> &points, uint32 col) {
	struct PerVertexData {
		float gl_x;
		float gl_y;
		float r;
		float g;
		float b;
		float alpha;
	};
	static_assert(sizeof(PerVertexData) == 6 * sizeof(float), "PerVertexData must be tightly packed.");

	std::vector<PerVertexData> vertices;
	vertices.reserve(points.size());
	for (const auto &p : points) {
		vertices.emplace_back();
		PerVertexData& back = vertices.back();
		back.gl_x = p.x;
		back.gl_y = p.y;
		this->CoordsToGL(&back.gl_x, &back.gl_y);
		back.r = FGetR(col);
		back.g = FGetG(col);
		back.b = FGetB(col);
		back.alpha = FGetA(col);
	}
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glUseProgram(this->colour_shader);
	glBindVertexArray(this->vao);
	glDrawArrays(GL_POINTS, 0, vertices.size());
}

/**
 * Draw a straight line on the screen.
 * @param x1 X coordinate of the starting point.
 * @param y1 Y coordinate of the starting point.
 * @param x2 X coordinate of the end point.
 * @param y2 Y coordinate of the end point.
 * @param col RGBA colour to use.
 */
void VideoSystem::DoDrawLine(float x1, float y1, float x2, float y2, uint32 col) {
	this->CoordsToGL(&x1, &y1);
	this->CoordsToGL(&x2, &y2);
	float vertices[] = {
		x1, y1, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
		x2, y2, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
	};
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glUseProgram(this->colour_shader);
	glBindVertexArray(this->vao);
	glDrawArrays(GL_LINES, 0, 2);
}

/**
 * Fill a rectangle in a solid colour.
 * @param x1 X coordinate of the upper left corner.
 * @param y1 Y coordinate of the upper left corner.
 * @param x2 X coordinate of the lower right corner.
 * @param y2 Y coordinate of the lower right corner.
 * @param col RGBA colour to use.
 */
void VideoSystem::DoFillPlainColour(float x1, float y1, float x2, float y2, uint32 col) {
	this->CoordsToGL(&x1, &y1);
	this->CoordsToGL(&x2, &y2);
	float vertices[] = {
		// positions  // colours
		x2, y1, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
		x2, y2, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
		x1, y1, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
		x1, y2, 0.0f, FGetR(col), FGetG(col), FGetB(col), FGetA(col),
	};
	GLuint indices[] = {
		0, 1, 2,
		1, 2, 3,
	};
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glUseProgram(this->colour_shader);
	glBindVertexArray(this->vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
