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

#include "filesystem/fs_delegate_ramdisk.hpp"
#include "filesystem/filesystem.hpp"
#include "utils/string.hpp"
#include "logger/logger.hpp"
#include "kernel.hpp"
#include "ghost/utils/local.hpp"

/**
 *
 */
g_fs_node* g_fs_delegate_ramdisk::create_vfs_node(g_ramdisk_entry* ramdisk_node, g_fs_node* parent) {

	g_fs_node* node = g_filesystem::create_node();
	node->phys_fs_id = ramdisk_node->id;

	if (ramdisk_node->type == G_RAMDISK_ENTRY_TYPE_FILE) {
		node->type = G_FS_NODE_TYPE_FILE;
	} else {
		node->type = G_FS_NODE_TYPE_FOLDER;
	}

	int len = g_string::length(ramdisk_node->name);
	node->name = new char[len + 1];
	g_memory::copy(node->name, ramdisk_node->name, len);
	node->name[len] = 0;

	// add it to the parent
	parent->add_child(node);

	return node;
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// find on ramdisk
	g_ramdisk_entry* ramdisk_parent;
	if (parent->type == G_FS_NODE_TYPE_MOUNTPOINT) {
		ramdisk_parent = g_kernel_ramdisk->getRoot();
	} else {
		ramdisk_parent = g_kernel_ramdisk->findById(parent->phys_fs_id);
	}

	if (ramdisk_parent) {
		g_ramdisk_entry* ramdisk_node = g_kernel_ramdisk->findChild(ramdisk_parent, child);

		if (ramdisk_node) {
			// create the vfs node
			g_fs_node* node = create_vfs_node(ramdisk_node, parent);

			handler->status = G_FS_DISCOVERY_SUCCESSFUL;
		} else {
			handler->status = G_FS_DISCOVERY_NOT_FOUND;
		}
	} else {
		handler->status = G_FS_DISCOVERY_NOT_FOUND;
	}
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) {
	// nothing to do here
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_read* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	g_ramdisk_entry* ramdisk_node = g_kernel_ramdisk->findById(node->phys_fs_id);
	if (ramdisk_node == 0) {
		handler->status = G_FS_READ_INVALID_FD;

	} else {
		int64_t copy_amount = ((fd->offset + length) >= ramdisk_node->datalength) ? (ramdisk_node->datalength - fd->offset) : length;
		if (copy_amount > 0) {
			g_memory::copy(buffer(), &ramdisk_node->data[fd->offset], copy_amount);
			fd->offset += copy_amount;
		}
		handler->result = copy_amount;
		handler->status = G_FS_READ_SUCCESSFUL;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	handler->status = G_FS_WRITE_NOT_SUPPORTED;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_ramdisk_entry* ramdisk_node = g_kernel_ramdisk->findById(node->phys_fs_id);
	if (ramdisk_node == 0) {
		handler->status = G_FS_LENGTH_NOT_FOUND;
		handler->length = 0;
	} else {
		handler->status = G_FS_LENGTH_SUCCESSFUL;
		handler->length = ramdisk_node->datalength;
	}

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) {

}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_read_directory(g_thread* requester, g_fs_node* fs_parent, int position,
		g_fs_transaction_handler_read_directory* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_ramdisk_entry* rd_parent = g_kernel_ramdisk->findById(fs_parent->phys_fs_id);
	if (rd_parent == 0) {
		handler->status = G_FS_READ_DIRECTORY_ERROR;

	} else {
		g_ramdisk_entry* rd_child = g_kernel_ramdisk->getChildAt(fs_parent->phys_fs_id, position);
		if (rd_child) {
			// get real path to parent
			g_local<char> absolute(new char[G_PATH_MAX]);
			g_filesystem::get_real_path_to_node(fs_parent, absolute());

			// append child name
			int abs_cur_len = g_string::length((const char*) absolute());
			int childlen = g_string::length(rd_child->name);
			g_memory::copy(&absolute()[abs_cur_len], "/", 1);
			g_memory::copy(&absolute()[abs_cur_len + 1], rd_child->name, childlen);
			absolute()[abs_cur_len + 1 + childlen] = 0;

			// check if file exists as vfs node
			g_fs_node* fs_childs_parent = 0;
			g_fs_node* fs_child = 0;
			g_local<char> current(new char[G_PATH_MAX]);
			g_filesystem::find_existing(absolute(), &fs_childs_parent, &fs_child, current(), true);

			// if not, create it
			if (fs_child == 0) {
				fs_child = create_vfs_node(rd_child, fs_parent);
			}

			// copy name to output
			handler->child = fs_child;
			handler->status = G_FS_READ_DIRECTORY_SUCCESSFUL;
		} else {
			handler->status = G_FS_READ_DIRECTORY_EOD;
		}
	}

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_read_directory(g_thread* requester, g_fs_transaction_handler_read_directory* handler) {
}
