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

#include "filesystem/fs_delegate.hpp"
#include "filesystem/fs_transaction_handler_read_directory.hpp"

#include "logger/logger.hpp"

/**
 *
 */
g_fs_transaction_handler_status g_fs_transaction_handler_read_directory::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	if (delegate) {
		delegate->finish_read_directory(thread, this);
	}

	data()->status = status;
	if (status == G_FS_READ_DIRECTORY_SUCCESSFUL) {
		g_memory::copy(data()->iterator->entry_buffer.name, child->name, g_string::length(child->name) + 1);
		++data()->iterator->position;
		data()->iterator->entry_buffer.node_id = child->id;
		data()->iterator->entry_buffer.type = child->type;
	}

	return G_FS_TRANSACTION_HANDLING_DONE;
}
