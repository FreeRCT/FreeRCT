/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.h Graphics system handling. */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "stdafx.h"
#include "geometry.h"
#include "palette.h"
#include "window_constants.h"

#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <GLFW/glfw3.h>

struct FontGlyph;
class ImageData;

using Realtime = std::chrono::high_resolution_clock::time_point;  ///< Represents a time point in real time.
using Duration = std::chrono::duration<double, std::milli>;       ///< Difference between two time points with millisecond precision.

/**
 * Get the current real time.
 * @return The time.
 */
inline Realtime Time()
{
	return std::chrono::high_resolution_clock::now();
}

/**
 * Get the time difference between two time points in milliseconds.
 * @return The time difference.
 */
inline double Delta(const Realtime &start, const Realtime &end = Time())
{
	Duration d = end - start;
	return d.count();
}

/** Class responsible for rendering text. */
class TextRenderer {
public:
	static constexpr const uint32 MAX_CODEPOINT = 0xFFFD;  ///< Highest unicode codepoint we can render (arbitrary limit).

	void Initialize();
	void LoadFont(const std::string &font_path, GLuint font_size);

	GLuint GetTextHeight() const;
	PointF EstimateBounds(const std::string &text, float scale = 1.0f) const;

	void Draw(const std::string &text, float x, float y, float max_width, uint32 colour, float scale = 1.0f);

private:
	/** Helper struct representing a font glyph. */
	struct FontGlyph {
		GLuint texture_id;   ///< The OpenGL texture used to render this glyph.
		Point16 size;        ///< Size of this glyph in pixels.
		Point16 bearing;     ///< Alignment offset from the baseline.
		GLuint advance;      ///< Horizontal spacing.
		bool valid = false;  ///< If \c false, all data in this struct is invalid.
	};

	const FontGlyph &GetFontGlyph(const char **text, size_t &length) const;

	FontGlyph characters[MAX_CODEPOINT + 1];  ///< All character glyphs in the current font indexed by their unicode codepoint.
	GLuint font_size;                         ///< Current font size.
	GLuint shader;                            ///< The font shader.
	GLuint vao;                               ///< The OpenGL vertex array.
	GLuint vbo;                               ///< The OpenGL vertex buffer.
};

extern TextRenderer _text_renderer;

/** How to align text during drawing. */
enum Alignment {
	ALG_LEFT,    ///< Align to the left edge.
	ALG_CENTER,  ///< Centre the text.
	ALG_RIGHT,   ///< Align to the right edge.
};

/** Class providing the interface to the OpenGL rendering backend. */
class VideoSystem {
public:
	void Initialize(const std::string &font, int font_size);

	static void MainLoopCycle();
	void MainLoop();
	void Shutdown();

	double FPS() const;
	double AvgFPS() const;

	void SetMouseDragging(MouseButtons button, bool dragging);

	/**
	 * Get the mouse button currently being dragged.
	 * @return The button, or #MB_NONE for none.
	 */
	MouseButtons GetMouseDragging() const
	{
		return this->mouse_dragging;
	}

	/**
	 * Get the current width of the window.
	 * @return The width in pixels.
	 */
	float Width() const
	{
		return this->width;
	}

	/**
	 * Get the current height of the window.
	 * @return The height in pixels.
	 */
	float Height() const
	{
		return this->height;
	}

	/**
	 * Get the current mouse position.
	 * @return The mouse X coordinate.
	 */
	float MouseX() const
	{
		return this->mouse_x;
	}

	/**
	 * Get the current mouse position.
	 * @return The mouse Y coordinate.
	 */
	float MouseY() const
	{
		return this->mouse_y;
	}

	/**
	 * Get the current mouse position.
	 * @return The mouse coordinates.
	 */
	Point32 GetMousePosition() const
	{
		return Point32(this->MouseX(), this->MouseY());
	}

	void SetResolution(const Point32 &res);

	/**
	 * List all available window resolutions.
	 * @return The resolutions.
	 */
	const std::set<Point32> &Resolutions() const
	{
		return this->resolutions;
	}

	void CoordsToGL(float *x, float *y) const;

	GLuint LoadShader(const std::string &name);

	int GetTextHeight() const
	{
		return _text_renderer.GetTextHeight();
	}

	void BlitText(const std::string &text, uint32 colour, int xpos, int ypos, int width = 0x7FFF, Alignment align = ALG_LEFT);
	void GetTextSize(const std::string &text, int *width, int *height);
	void GetNumberRangeSize(int64 smallest, int64 biggest, int *width, int *height);

	/**
	 * Draw a line from \a start to \a end using the specified \a colour.
	 * @param start Starting point at the screen.
	 * @param end End point at the screen.
	 * @param colour Colour to use.
	 */
	inline void DrawLine(const Point16 &start, const Point16 &end, uint32 colour)
	{
		this->DoDrawLine(start.x, start.y, end.x, end.y, colour);
	}

	/**
	 * Draw the outline of a rectangle at the screen.
	 * @param rect %Rectangle to draw.
	 * @param col Colour to use.
	 */
	inline void DrawRectangle(const Rectangle32 &rect, uint32 col)
	{
		this->DoDrawLine(rect.base.x, rect.base.y, rect.base.x + rect.width, rect.base.y, col);
		this->DoDrawLine(rect.base.x, rect.base.y, rect.base.x, rect.base.y + rect.height, col);
		this->DoDrawLine(rect.base.x + rect.width, rect.base.y + rect.height, rect.base.x + rect.width, rect.base.y, col);
		this->DoDrawLine(rect.base.x + rect.width, rect.base.y + rect.height, rect.base.x, rect.base.y + rect.height, col);
	}

	/**
	 * Paint a rectangle at the screen.
	 * @param rect %Rectangle to draw.
	 * @param colour Colour to use.
	 */
	inline void FillRectangle(const Rectangle32 &rect, uint32 colour)
	{
		this->DoFillPlainColour(rect.base.x, rect.base.y, rect.base.x + rect.width, rect.base.y + rect.height, colour);
	}

	void TileImage(const ImageData *img, const Rectangle32 &rect, bool tile_hor, bool tile_vert,
			const Recolouring &recolour = _no_recolour,
			GradientShift shift = GS_NORMAL, uint32 col = 0xffffffff);
	void BlitImage(const Point32 &pos, const ImageData *img, const Recolouring &recolour = _no_recolour,
			GradientShift shift = GS_NORMAL, uint32 col = 0xffffffff);

	void PushClip(const Rectangle32 &rect);
	void PopClip();

	void FinishRepaint();

private:
	bool MainLoopDoCycle();

	GLuint LoadShaders(const char *vp, const char *fp);
	void UpdateClip();

	GLuint GetImageTexture(const ImageData *img, const Recolouring &recolour, GradientShift shift);
	void DoDrawImage(GLuint texture, float x1, float y1, float x2, float y2,
			uint32 col = 0xffffffff, const WXYZPointF &tex = WXYZPointF(0.0f, 0.0f, 1.0f, 1.0f));

	void DoDrawPlainColours(const std::vector<Point<float>> &points, uint32 colour);
	void DoDrawLine(float x1, float y1, float x2, float y2, uint32 colour);
	void DoFillPlainColour(float x1, float y1, float x2, float y2, uint32 colour);

	static void FramebufferSizeCallback(GLFWwindow *window, int w, int h);
	static void MouseClickCallback(GLFWwindow *window, int button, int action, int mods);
	static void MouseMoveCallback(GLFWwindow *window, double x, double y);
	static void ScrollCallback(GLFWwindow *window, double xdelta, double ydelta);
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void TextCallback(GLFWwindow *window, uint32 codepoint);

	uint32_t width;   ///< Current window width in pixels.
	uint32_t height;  ///< Current window height in pixels.
	double mouse_x;   ///< Current mouse X position.
	double mouse_y;   ///< Current mouse Y position.
	MouseButtons mouse_dragging;  ///< The mouse button being dragged, if any.

	std::set<Point32> resolutions;  ///< Available window resolutions.

	Realtime last_frame;       ///< Time when the last frame started.
	Realtime cur_frame;        ///< Time when the current frame started.
	double average_frametime;  ///< Long-term average framerate in milliseconds per frame.

	std::map<std::pair<const ImageData*, RecolourData>, GLuint> image_textures;  ///< Textures for all loaded images.

	GLuint image_shader;   ///< Shader for images.
	GLuint colour_shader;  ///< Shader for plain colours.
	GLuint vao;            ///< The OpenGL vertex array.
	GLuint vbo;            ///< The OpenGL vertex buffer.
	GLuint ebo;            ///< The OpenGL element buffer.

	std::vector<Rectangle32> clip;  ///< Current clipping area stack.

	GLFWwindow *window;  ///< The GLFW window.
};

constexpr int WINDOW_ICON_WIDTH  = 32;  ///< Width of the window/taskbar icon.
constexpr int WINDOW_ICON_HEIGHT = 32;  ///< Height of the window/taskbar icon.
extern std::unique_ptr<uint8[]> _icon_data;

extern VideoSystem _video;

#endif
