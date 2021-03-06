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

#ifndef GHOST_TASKING_TASKING
#define GHOST_TASKING_TASKING

#include "ghost/kernel.h"
#include "ghost/stdint.h"
#include <system/cpu_state.hpp>
#include <tasking/thread.hpp>
#include <tasking/process.hpp>
#include <tasking/scheduling/scheduler.hpp>

/**
 *
 */
class g_tasking {
public:

	/**
	 * Called once by the BSP
	 */
	static void initialize();

	/**
	 * Called by each core (including BSP)
	 */
	static void enableForThisCore();

	/**
	 * Called by the timer for switching
	 */
	static g_cpu_state* switchTask(g_cpu_state* cpuState);

	/**
	 * Adds the task to the least loaded cores scheduler
	 */
	static void addTask(g_thread* proc, bool enforceCurrentCore = false);

	/**
	 * Returns the current task on the current core
	 */
	static g_thread* getCurrentThread();

	/**
	 * Kills all threads of the given process within all schedulers.
	 * Returns true if all are dead.
	 */
	static bool killAllThreadsOf(g_process* process);

	/**
	 * Returns the current scheduler on the current core
	 */
	static g_scheduler* getCurrentScheduler();

	/**
	 *
	 */
	static g_thread* getTaskById(uint32_t id);

	/**
	 *
	 */
	static g_thread* getTaskByIdentifier(const char* identifier);

	/**
	 *
	 */
	static bool registerTaskForIdentifier(g_thread* task, const char* newIdentifier);

	/**
	 *
	 */
	static g_thread* fork();

};

#endif
