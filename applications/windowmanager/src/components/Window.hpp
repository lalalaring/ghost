/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include <components/Component.hpp>

#define DEFAULT_BORDER_WIDTH		5
#define DEFAULT_CORNER_SIZE			15

/**
 *
 */
enum ResizeMode {
	RESIZE_MODE_NONE,
	RESIZE_MODE_TOP,
	RESIZE_MODE_TOP_RIGHT,
	RESIZE_MODE_RIGHT,
	RESIZE_MODE_BOTTOM_RIGHT,
	RESIZE_MODE_BOTTOM,
	RESIZE_MODE_BOTTOM_LEFT,
	RESIZE_MODE_LEFT,
	RESIZE_MODE_TOP_LEFT
};

/**
 *
 */
class Window: public Component {
private:
	int borderWidth;
	int cornerSize;
	g_color_argb backgroundColor;

public:
	Window();

	virtual ~Window() {
	}

	void setBackground(g_color_argb color) {
		backgroundColor = color;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}

	g_color_argb getBackground() {
		return backgroundColor;
	}

	virtual void paint();

	/**
	 *
	 */
	virtual void handleBoundChange(g_rectangle oldBounds);
	virtual bool handle(Event& e);
};

#endif
