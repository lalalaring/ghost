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

#include <calls/syscall_handler.hpp>
#include <logger/logger.hpp>

#include "ghost/kernel.h"
#include "ghost/utils/local.hpp"
#include <kernel.hpp>
#include <ramdisk/ramdisk.hpp>
#include <tasking/thread.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/wait/waiter_call_vm86.hpp>
#include <memory/address_space.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/physical/pp_reference_tracker.hpp>
#include <executable/elf32_loader.hpp>

#define CREATE_PAGE_IN_SPACE_MAXIMUM_PAGES 100

/**
 *
 */
G_SYSCALL_HANDLER(create_empty_process) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	// Prepare data
	g_syscall_create_empty_process* data = (g_syscall_create_empty_process*) G_SYSCALL_DATA(state);
	data->processObject = 0;

	// Only kernel level
	if (process->securityLevel == G_SECURITY_LEVEL_KERNEL) {
		g_thread* emptyMainThread = g_thread_manager::createProcess(data->securityLevel);
		data->processObject = emptyMainThread;

		g_log_debug("%! (%i:%i) created empty process %i", "syscall", process->main->id, thread->id, emptyMainThread->id);
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: create empty process", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 * TODO
 */
G_SYSCALL_HANDLER(cli_args_store) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_cli_args_store* data = (g_syscall_cli_args_store*) G_SYSCALL_DATA(state);

	if (task->process->securityLevel == G_SECURITY_LEVEL_KERNEL) {

		g_thread* other = (g_thread*) data->processObject;
		g_process* otherProcess = other->process;

		otherProcess->cliArguments = new char[G_CLIARGS_BUFFER_LENGTH];
		uint32_t argsLen = g_string::length(data->arguments);
		g_memory::copy(otherProcess->cliArguments, data->arguments, argsLen);
		otherProcess->cliArguments[argsLen] = 0;

		g_log_debug("%! task %i stored cli arguments for task %i", "syscall", task->id, other->id);

	} else {
		g_log_warn("%! task %i tried to store another tasks cli arguments but is not permitted", "syscall", task->id);
	}

	return state;
}

/**
 * TODO
 */
G_SYSCALL_HANDLER(cli_args_release) {

	g_thread* task = g_tasking::getCurrentThread();
	g_process* process = task->process;

	g_syscall_cli_args_release* data = (g_syscall_cli_args_release*) G_SYSCALL_DATA(state);

	// Copy args if available
	if (process->cliArguments != 0) {
		g_memory::copy(data->buffer, process->cliArguments, G_CLIARGS_BUFFER_LENGTH);
		data->buffer[g_string::length(process->cliArguments)] = 0;

		delete process->cliArguments;
		process->cliArguments = 0;
	} else {
		// Null-terminate in case of failure
		data->buffer[0] = 0;
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(create_pages_in_space) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	// Prepare data
	g_syscall_create_pages_in_space* data = (g_syscall_create_pages_in_space*) G_SYSCALL_DATA(state);
	data->resultVirtualAddress = 0;

	// Only kernel level
	if (process->securityLevel == G_SECURITY_LEVEL_KERNEL) {

		g_thread* targetThread = (g_thread*) data->processObject;
		g_process* targetProcess = targetThread->process;

		// Create page in target space
		// Below we do a temporary switch, where 'data' is not available (because we switch to the
		// target userspace, and 'data' is in the current userspace), so we copy the values:
		uint32_t virtualAddressInTargetSpace = data->targetSpaceVirtualAddress;
		uint32_t numberOfPages = data->numberOfPages;

		// Only allow a certain number of pages to be cross-allocated
		if (numberOfPages > 0 && numberOfPages <= CREATE_PAGE_IN_SPACE_MAXIMUM_PAGES) {

			// Adjust the image range of the other process if necessary. This is required so that the kernel
			// can keep track of where the process image lays in this address space.
			if (targetProcess->imageStart == 0 || targetProcess->imageStart > virtualAddressInTargetSpace) {
				targetProcess->imageStart = virtualAddressInTargetSpace;
			}
			if (targetProcess->imageEnd == 0 || targetProcess->imageEnd < (virtualAddressInTargetSpace + numberOfPages * G_PAGE_SIZE)) {
				targetProcess->imageEnd = virtualAddressInTargetSpace + numberOfPages * G_PAGE_SIZE;
			}

			// Create physical pages and map them into the target space. Remember the physical addresses
			g_physical_address physicalPages[numberOfPages];

			// Perform temporary switch to target process and map pages
			g_address_space::switch_to_space(targetProcess->pageDirectory);
			for (uint32_t i = 0; i < numberOfPages; i++) {
				physicalPages[i] = g_pp_allocator::allocate();
				g_address_space::map(virtualAddressInTargetSpace + i * G_PAGE_SIZE, physicalPages[i], DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
				g_pp_reference_tracker::increment(physicalPages[i]);
			}
			g_address_space::switch_to_space(process->pageDirectory);

			// Map all pages (which physical addresses are in the array) to the current tasks space
			g_virtual_address virtAddrHere = process->virtualRanges.allocate(numberOfPages, G_PROC_VIRTUAL_RANGE_FLAG_NONE);
			for (uint32_t i = 0; i < numberOfPages; i++) {
				g_address_space::map(virtAddrHere + i * G_PAGE_SIZE, physicalPages[i], DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			}

			data->resultVirtualAddress = virtAddrHere;

			g_log_debug("%! (%i:%i) created %i pages in space of process %i. virtual: here %h, there %h", "syscall", process->main->id, thread->id,
					data->numberOfPages, targetThread->id, virtAddrHere, data->targetSpaceVirtualAddress);

		} else {
			g_log_warn("%! (%i:%i) error: illegal arguments: tried to create pages in a space. %i is not a valid number of pages (1-%i)", "syscall",
					process->main->id, thread->id, data->numberOfPages, CREATE_PAGE_IN_SPACE_MAXIMUM_PAGES);
		}
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: create a page in a space", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(attach_created_process) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	g_syscall_attach_created_process* data = (g_syscall_attach_created_process*) G_SYSCALL_DATA(state);

	// Only kernel level
	if (thread->process->securityLevel == G_SECURITY_LEVEL_KERNEL) {
		g_thread* targetMainThread = (g_thread*) data->processObject;
		targetMainThread->cpuState->eip = data->eip;
		g_thread_manager::prepare_thread_local_storage(targetMainThread);
		g_tasking::addTask(targetMainThread);

		g_log_debug("%! (%i:%i) attached task %i, starting at eip %h", "syscall", process->main->id, thread->id, targetMainThread->id,
				targetMainThread->cpuState->eip);
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: attach created process", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(cancel_process_creation) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	g_syscall_cancel_process_creation* data = (g_syscall_cancel_process_creation*) G_SYSCALL_DATA(state);

	// Only kernel level
	if (thread->process->securityLevel == G_SECURITY_LEVEL_KERNEL) {
		g_thread* target = (g_thread*) data->processObject;
		g_thread_manager::deleteTask(target);

		g_log_debug("%! (%i:%i) cancelled creation of process %i", "syscall", process->main->id, thread->id, target->id);
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: cancel process creation", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(get_created_process_id) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	g_syscall_get_created_process_id* data = (g_syscall_get_created_process_id*) G_SYSCALL_DATA(state);

	// Only on kernel level
	if (process->securityLevel == G_SECURITY_LEVEL_KERNEL) {
		g_thread* createdMainThread = (g_thread*) (data->processObject);
		data->resultId = createdMainThread->id;

		g_log_debug("%! (%i:%i) get id of created process: %i", "syscall", process->main->id, thread->id, createdMainThread->id);
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: get created process id", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(write_tls_master_for_process) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	g_syscall_write_tls_master_for_process* data = (g_syscall_write_tls_master_for_process*) G_SYSCALL_DATA(state);
	data->result = false;

	// Only on kernel level
	if (process->securityLevel == G_SECURITY_LEVEL_KERNEL) {

		// Get target main thread
		g_thread* target_thread = (g_thread*) (data->processObject);
		g_process* target_process = target_thread->process;

		// Get a virtual address range for the TLS master copy
		uint32_t required_pages = PAGE_ALIGN_UP(data->copysize) / G_PAGE_SIZE;
		g_virtual_address tls_master_virt = target_process->virtualRanges.allocate(required_pages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);

		// Temporarily copy master contents to kernel heap because we switch directories
		g_local < uint8_t > temporary_store(new uint8_t[data->copysize]);
		g_memory::copy(temporary_store(), data->content, data->copysize);
		uint32_t temporary_copy_size = data->copysize;

		// Temporarily switch to target process directory, map the pages & zero/copy content
		g_address_space::switch_to_space(target_process->pageDirectory);
		for (uint32_t i = 0; i < required_pages; i++) {
			g_physical_address phys = g_pp_allocator::allocate();
			g_address_space::map(tls_master_virt + i * G_PAGE_SIZE, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			g_pp_reference_tracker::increment(phys);
		}
		g_memory::copy((void*) tls_master_virt, temporary_store(), temporary_copy_size);
		g_address_space::switch_to_space(process->pageDirectory);

		// Write info to process
		target_process->tls_master_in_proc_location = tls_master_virt;
		target_process->tls_master_copysize = data->copysize;
		target_process->tls_master_totalsize = data->totalsize;
		target_process->tls_master_alignment = data->alignment;

		data->result = true;
		g_log_debug("%! (%i:%i) created tls master for process: %i, at: %h, size: %h, alignment: %h", "syscall", process->main->id, thread->id,
				target_thread->id, target_process->tls_master_in_proc_location, target_process->tls_master_copysize, target_process->tls_master_alignment);
	} else {
		g_log_warn("%! (%i:%i) error: insufficient permissions: create tls space in process", "syscall", process->main->id, thread->id);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(ramdisk_spawn) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;

	g_syscall_ramdisk_spawn* data = (g_syscall_ramdisk_spawn*) G_SYSCALL_DATA(state);

	if (process->securityLevel == G_SECURITY_LEVEL_KERNEL) {
		g_ramdisk_entry* file = g_kernel_ramdisk->findAbsolute(data->path);

		if (file) {
			g_thread* spawnedThread;
			g_process* spawnedProcess;

			g_elf32_spawn_status status = g_elf32_loader::spawnFromRamdisk(file, data->securityLevel, &spawnedThread);

			if (status == ELF32_SPAWN_STATUS_SUCCESSFUL) {
				data->spawnStatus = G_RAMDISK_SPAWN_STATUS_SUCCESSFUL;
				spawnedProcess = spawnedThread->process;
				g_log_debug("%! (%i:%i) spawn '%s' from ramdisk -> %i <%h-%h> #%i", "syscall", process->main->id, thread->id, data->path,
						spawnedProcess->imageStart, spawnedProcess->imageEnd, spawnedThread->id, data->securityLevel);

			} else {
				data->spawnStatus = G_RAMDISK_SPAWN_STATUS_FAILED_NOT_VALID;
				g_log_warn("%! task %i tried to spawn ramdisk binary '%s' but validation failed", "syscall", thread->id, data->path);
				g_log_warn("%! (%i:%i) error: invalid elf: spawn '%s' from ramdisk", "syscall", process->main->id, thread->id, data->path);
			}

		} else {
			data->spawnStatus = G_RAMDISK_SPAWN_STATUS_FAILED_NOT_FOUND;
			g_log_warn("%! (%i:%i) error: file does not exist: spawn '%s' from ramdisk", "syscall", process->main->id, thread->id, data->path);
		}
	} else {
		data->spawnStatus = G_RAMDISK_SPAWN_STATUS_FAILED_NOT_PERMITTED;
		g_log_warn("%! (%i:%i) error: insufficient permissions: spawn '%s' from ramdisk", "syscall", process->main->id, thread->id, data->path);
	}

	return state;
}

/**
 * Returns the entry point for the current thread. If the executing task is not
 * a thread, zero is returned.
 */
G_SYSCALL_HANDLER(get_thread_entry) {

	g_syscall_get_thread_entry* data = (g_syscall_get_thread_entry*) G_SYSCALL_DATA(state);

	g_thread* currentTask = g_tasking::getCurrentThread();
	if (currentTask->type == g_thread_type::THREAD) {
		data->userEntry = currentTask->threadEntry;
		data->userData = currentTask->userData;
		g_log_debug("%! process %i retrieved its thread entry %h", "syscall", currentTask->id, currentTask->threadEntry);
	} else {
		data->userEntry = 0;
		g_log_warn("%! process %i tried to retrieve its thread entry but is a process", "syscall", currentTask->id);
	}

	return state;
}

/**
 * Creates a thread. The given data must contain the initial entry library function
 * and the user entry. The user entry is then added to the thread data and is later
 * retrieved by the library function via getThreadEntry.
 */
G_SYSCALL_HANDLER(create_thread) {

	g_syscall_create_thread* data = (g_syscall_create_thread*) G_SYSCALL_DATA(state);

	g_thread* task = g_tasking::getCurrentThread();

	g_log_debug("%! (%i:%i) creates thread", "syscall", task->process->main->id, task->id);

	g_thread* thread = g_thread_manager::createThread(task->process);

	if (thread != 0) {
		g_log_debug("%! (%i:%i) spawned thread %i", "syscall", task->process->main->id, task->id, thread->id);
		thread->cpuState->eip = (uint32_t) data->initialEntry;
		thread->threadEntry = data->userEntry;
		thread->userData = data->userData;

		data->processId = thread->id;
		g_tasking::addTask(thread);
	} else {
		g_log_warn("%! (%i:%i) failed to spawn thread", "syscall", task->process->main->id, task->id);
		data->processId = 0;
	}

	// A process is forced to give away time when creating a thread
	return g_tasking::switchTask(state);
}

