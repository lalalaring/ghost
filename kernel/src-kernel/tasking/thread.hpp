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

#ifndef GHOST_MULTITASKING_THREAD
#define GHOST_MULTITASKING_THREAD

#include "ghost/kernel.h"
#include "ghost/calls/calls.h"
#include "ghost/signal.h"
#include "memory/paging.hpp"
#include "system/cpu_state.hpp"
#include "memory/collections/address_range_pool.hpp"

// forward declarations
class g_process;
class g_waiter;

/**
 * Task types
 */
enum class g_thread_type
	: unsigned char {
		THREAD_MAIN = 0, THREAD = 1, THREAD_VM86 = 2
};

/**
 * Task priority
 */
enum class g_thread_priority
	: unsigned char {
		NORMAL = 0, IDLE = 1
};

/**
 * Data used by virtual 8086 processes
 */
struct g_thread_information_vm86 {
	g_thread_information_vm86() :
			cpuIf(false), out(0), interruptRecursionLevel(0) {
	}

	bool cpuIf;
	g_vm86_registers* out;
	uint32_t interruptRecursionLevel;
};

/**
 *
 */
enum class g_thread_interruption_info_type
	: uint8_t {
		NONE, IRQ, SIGNAL
};

/**
 *
 */
class g_thread_interruption_info {
public:
	g_cpu_state cpuState;
	g_cpu_state* cpuStateAddress;
	g_waiter* waitManager;

	g_thread_interruption_info_type type = g_thread_interruption_info_type::NONE;
	uint8_t handled_irq;
	int handled_signal;
};

/**
 *
 */
class g_thread {
private:
	g_thread_information_vm86* vm86Information;
	char* identifier;

public:
	g_thread(g_thread_type _type);
	~g_thread();

	g_tid id;
	bool alive;
	g_thread_type type;
	g_thread_priority priority;
	g_process* process;

	g_waiter* waitManager;
	uint32_t waitCount;

	void* userData;
	void* threadEntry;

	g_cpu_state* cpuState;
	g_virtual_address kernelStack;

	g_virtual_address kernelStackEsp0;
	g_virtual_address userStack;

	g_virtual_address user_thread_addr;
	g_virtual_address tls_copy_virt;

	g_thread_interruption_info* interruption_info;

	g_thread_information_vm86* getVm86Information();
	const char* getIdentifier();
	void setIdentifier(const char* newIdentifier);

	bool isWaiting();
	void wait(g_waiter* waitManager);
	void unwait();
	bool checkWaiting();

	void raise_signal(int signal);
	void enter_irq_handler(uintptr_t address, uint8_t irq, uintptr_t callback);
	void enter_signal_handler(uintptr_t address, int signal, uintptr_t callback);
	bool start_prepare_interruption();
	void finish_prepare_interruption(uintptr_t address, uintptr_t callback);

	void store_for_interruption();
	void restore_interrupted_state();
};

#endif
