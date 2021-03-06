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

#include <components/Window.hpp>
#include <WindowManager.hpp>
#include <events/KeyEvent.hpp>
#include <events/MouseEvent.hpp>
#include <events/FocusEvent.hpp>

#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/graphics/polygon.hpp>

/**
 *
 */
Window::Window() :
		backgroundColor(RGB(240, 240, 240)), borderWidth(DEFAULT_BORDER_WIDTH), cornerSize(DEFAULT_CORNER_SIZE) {
	this->visible = false;
}

/**
 *
 */
void Window::paint() {

	graphics.clear();
	g_painter p(graphics);
	p.setColor(backgroundColor);
	p.fill(g_rectangle(0, 0, getBounds().width, getBounds().height));
	p.setColor(RGB(80, 80, 80));
	p.draw(g_rectangle(0, 0, getBounds().width - 1, getBounds().height - 1));

}

/**
 *
 */
void Window::handleBoundChange(g_rectangle oldBounds) {
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
bool Window::handle(Event& event) {

	// Catch focus event
	FocusEvent* focusEvent = dynamic_cast<FocusEvent*>(&event);
	if (focusEvent) {
		return true;
	}

	// Let components handle input
	if (Component::handle(event)) {
		return true;
	}

	// Handle dragging / resizing
	static g_point pressPoint;
	static g_rectangle pressBounds;
	static ResizeMode resizeMode;

	MouseEvent* mouseEvent = dynamic_cast<MouseEvent*>(&event);
	if (mouseEvent) {

		if (mouseEvent->type == MOUSE_EVENT_DRAG) {

			g_point newLocation = mouseEvent->screenPosition - pressPoint;

			// Calculate new bounds
			g_rectangle newBounds = getBounds();

			if (resizeMode == RESIZE_MODE_TOP_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_TOP_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_BOTTOM_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_BOTTOM_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_TOP) {
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;

			} else if (resizeMode == RESIZE_MODE_BOTTOM) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;

			} else {
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
			}

			// Apply bounds
			g_rectangle appliedBounds = getBounds();
			if (newBounds.width > 50) {
				appliedBounds.x = newBounds.x;
				appliedBounds.width = newBounds.width;
			}
			if (newBounds.height > 20) {
				appliedBounds.y = newBounds.y;
				appliedBounds.height = newBounds.height;
			}
			this->setBounds(appliedBounds);

		} else if (mouseEvent->type == MOUSE_EVENT_PRESS) {

			pressPoint = mouseEvent->position;
			pressBounds = getBounds();

			if ((pressPoint.x < cornerSize) && (pressPoint.y < cornerSize)) { // Corner resizing
				resizeMode = RESIZE_MODE_TOP_LEFT;
			} else if ((pressPoint.x > getBounds().width - cornerSize) && (pressPoint.y < cornerSize)) {
				resizeMode = RESIZE_MODE_TOP_RIGHT;
			} else if ((pressPoint.x < cornerSize) && (pressPoint.y > getBounds().height - cornerSize)) {
				resizeMode = RESIZE_MODE_BOTTOM_LEFT;
			} else if ((pressPoint.x > getBounds().width - cornerSize) && (pressPoint.y > getBounds().height - cornerSize)) {
				resizeMode = RESIZE_MODE_BOTTOM_RIGHT;

			} else if (pressPoint.y < borderWidth) {	// Edge resizing
				resizeMode = RESIZE_MODE_TOP;
			} else if (pressPoint.x < borderWidth) {
				resizeMode = RESIZE_MODE_LEFT;
			} else if (pressPoint.y > getBounds().height - borderWidth) {
				resizeMode = RESIZE_MODE_BOTTOM;
			} else if (pressPoint.x > getBounds().width - borderWidth) {
				resizeMode = RESIZE_MODE_RIGHT;

			} else {
				resizeMode = RESIZE_MODE_NONE;

			}

		}

	}

	return true;
}
