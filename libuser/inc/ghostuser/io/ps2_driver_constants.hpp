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

#ifndef GHOSTLIBRARY_IO_PS2DRIVERCONSTANTS
#define GHOSTLIBRARY_IO_PS2DRIVERCONSTANTS

#define G_PS2_DRIVER_IDENTIFIER							"ps2driver"

typedef int g_ps2_command;
const g_ps2_command G_PS2_COMMAND_REGISTER_KEYBOARD = 0;
const g_ps2_command G_PS2_COMMAND_REGISTER_MOUSE = 1;

typedef struct {
	g_ps2_command command;
}__attribute__((packed)) g_ps2_register_request;

typedef struct {
	int16_t x;
	int16_t y;
	int32_t flags;
}__attribute__((packed)) g_ps2_mouse_packet;

typedef struct {
	uint8_t scancode;
}__attribute__((packed)) g_ps2_keyboard_packet;

#endif
