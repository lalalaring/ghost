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

#ifndef GHOST_INTERRUPTS_REQUEST_HANDLERS
#define GHOST_INTERRUPTS_REQUEST_HANDLERS

#include "ghost/stdint.h"
#include <system/cpu_state.hpp>
#include "ghost/kernel.h"

/**
 * Type of an interrupt handler
 */
typedef struct {
	g_tid thread_id;
	uintptr_t handler;
	uintptr_t callback;
} g_irq_handler;

/**
 * Interrupt request handling class
 */
class g_interrupt_request_handler {
public:

	/**
	 * @see source
	 */
	static g_cpu_state* handle(g_cpu_state* cpuState);

	/**
	 * Checks if the given IRQ has happened and was not handled yet.
	 *
	 * @param irq the number of the irq to check
	 * @return true if the irq has occured and should be handled
	 */
	static bool pollIrq(uint8_t irq);

	/**
	 *
	 */
	static void set_handler(uint8_t irq, g_tid thread_id, uintptr_t handler_addr, uintptr_t callback_addr);

};

#endif
