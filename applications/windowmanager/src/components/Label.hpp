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

#ifndef LABEL_HPP_
#define LABEL_HPP_

#include <components/Component.hpp>
#include <components/TitledComponent.hpp>
#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>

/**
 *
 */
class Label: public Component, public TitledComponent {
private:
	g_font* font;
	int fontSize;
	std::string text;
	g_text_alignment alignment;
	g_color_argb color;

	g_layouted_text viewModel;

public:
	Label();
	virtual ~Label() {
	}

	virtual void paint();
	virtual void update();
	virtual bool handle(Event& e);

	virtual void setTitle(std::string title);
	virtual std::string getTitle();

	void setAlignment(g_text_alignment alignment);
	g_text_alignment getAlignment();
};

#endif
