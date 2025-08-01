/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 *
 * prerr.c
 * This file is automatically generated; please do not edit it.
 */
#include "prerror.h"
static const struct PRErrorMessage text[] = {
	{"PR_OUT_OF_MEMORY_ERROR",    "Memory allocation attempt failed"},
	{"PR_BAD_DESCRIPTOR_ERROR",    "Invalid file descriptor"},
	{"PR_WOULD_BLOCK_ERROR",    "The operation would have blocked"},
	{"PR_ACCESS_FAULT_ERROR",    "Invalid memory address argument"},
	{"PR_INVALID_METHOD_ERROR",    "Invalid function for file type"},
	{"PR_ILLEGAL_ACCESS_ERROR",    "Invalid memory address argument"},
	{"PR_UNKNOWN_ERROR",    "Some unknown error has occurred"},
	{"PR_PENDING_INTERRUPT_ERROR",    "Operation interrupted by another thread"},
	{"PR_NOT_IMPLEMENTED_ERROR",    "function not implemented"},
	{"PR_IO_ERROR",    "I/O function error"},
	{"PR_IO_TIMEOUT_ERROR",    "I/O operation timed out"},
	{"PR_IO_PENDING_ERROR",    "I/O operation on busy file descriptor"},
	{"PR_DIRECTORY_OPEN_ERROR",    "The directory could not be opened"},
	{"PR_INVALID_ARGUMENT_ERROR",    "Invalid function argument"},
	{"PR_ADDRESS_NOT_AVAILABLE_ERROR",    "Network address not available (in use?)"},
	{"PR_ADDRESS_NOT_SUPPORTED_ERROR",    "Network address type not supported"},
	{"PR_IS_CONNECTED_ERROR",    "Already connected"},
	{"PR_BAD_ADDRESS_ERROR",    "Network address is invalid"},
	{"PR_ADDRESS_IN_USE_ERROR",    "Local Network address is in use"},
	{"PR_CONNECT_REFUSED_ERROR",    "Connection refused by peer"},
	{"PR_NETWORK_UNREACHABLE_ERROR",    "Network address is presently unreachable"},
	{"PR_CONNECT_TIMEOUT_ERROR",    "Connection attempt timed out"},
	{"PR_NOT_CONNECTED_ERROR",    "Network file descriptor is not connected"},
	{"PR_LOAD_LIBRARY_ERROR",    "Failure to load dynamic library"},
	{"PR_UNLOAD_LIBRARY_ERROR",    "Failure to unload dynamic library"},
	{"PR_FIND_SYMBOL_ERROR",    "Symbol not found in any of the loaded dynamic libraries"},
	{"PR_INSUFFICIENT_RESOURCES_ERROR",    "Insufficient system resources"},
	{"PR_DIRECTORY_LOOKUP_ERROR",    "A directory lookup on a network address has failed"},
	{"PR_TPD_RANGE_ERROR",    "Attempt to access a TPD key that is out of range"},
	{"PR_PROC_DESC_TABLE_FULL_ERROR",    "Process open FD table is full"},
	{"PR_SYS_DESC_TABLE_FULL_ERROR",    "System open FD table is full"},
	{"PR_NOT_SOCKET_ERROR",    "Network operation attempted on non-network file descriptor"},
	{"PR_NOT_TCP_SOCKET_ERROR",    "TCP-specific function attempted on a non-TCP file descriptor"},
	{"PR_SOCKET_ADDRESS_IS_BOUND_ERROR",    "TCP file descriptor is already bound"},
	{"PR_NO_ACCESS_RIGHTS_ERROR",    "Access Denied"},
	{"PR_OPERATION_NOT_SUPPORTED_ERROR",    "The requested operation is not supported by the platform"},
	{"PR_PROTOCOL_NOT_SUPPORTED_ERROR",    "The host operating system does not support the protocol requested"},
	{"PR_REMOTE_FILE_ERROR",    "Access to the remote file has been severed"},
	{"PR_BUFFER_OVERFLOW_ERROR",    "The value requested is too large to be stored in the data buffer provided"},
	{"PR_CONNECT_RESET_ERROR",    "TCP connection reset by peer"},
	{"PR_RANGE_ERROR",    "Unused"},
	{"PR_DEADLOCK_ERROR",    "The operation would have deadlocked"},
	{"PR_FILE_IS_LOCKED_ERROR",    "The file is already locked"},
	{"PR_FILE_TOO_BIG_ERROR",    "Write would result in file larger than the system allows"},
	{"PR_NO_DEVICE_SPACE_ERROR",    "The device for storing the file is full"},
	{"PR_PIPE_ERROR",    "Unused"},
	{"PR_NO_SEEK_DEVICE_ERROR",    "Unused"},
	{"PR_IS_DIRECTORY_ERROR",    "Cannot perform a normal file operation on a directory"},
	{"PR_LOOP_ERROR",    "Symbolic link loop"},
	{"PR_NAME_TOO_LONG_ERROR",    "File name is too long"},
	{"PR_FILE_NOT_FOUND_ERROR",    "File not found"},
	{"PR_NOT_DIRECTORY_ERROR",    "Cannot perform directory operation on a normal file"},
	{"PR_READ_ONLY_FILESYSTEM_ERROR",    "Cannot write to a read-only file system"},
	{"PR_DIRECTORY_NOT_EMPTY_ERROR",    "Cannot delete a directory that is not empty"},
	{"PR_FILESYSTEM_MOUNTED_ERROR",    "Cannot delete or rename a file object while the file system is busy"},
	{"PR_NOT_SAME_DEVICE_ERROR",    "Cannot rename a file to a file system on another device"},
	{"PR_DIRECTORY_CORRUPTED_ERROR",    "The directory object in the file system is corrupted"},
	{"PR_FILE_EXISTS_ERROR",    "Cannot create or rename a filename that already exists"},
	{"PR_MAX_DIRECTORY_ENTRIES_ERROR",    "Directory is full.  No additional filenames may be added"},
	{"PR_INVALID_DEVICE_STATE_ERROR",    "The required device was in an invalid state"},
	{"PR_DEVICE_IS_LOCKED_ERROR",    "The device is locked"},
	{"PR_NO_MORE_FILES_ERROR",    "No more entries in the directory"},
	{"PR_END_OF_FILE_ERROR",    "Encountered end of file"},
	{"PR_FILE_SEEK_ERROR",    "Seek error"},
	{"PR_FILE_IS_BUSY_ERROR",    "The file is busy"},
	{"PR_OPERATION_ABORTED_ERROR",    "The I/O operation was aborted"},
	{"PR_IN_PROGRESS_ERROR",    "Operation is still in progress (probably a non-blocking connect)"},
	{"PR_ALREADY_INITIATED_ERROR",    "Operation has already been initiated (probably a non-blocking connect)"},
	{"PR_GROUP_EMPTY_ERROR",    "The wait group is empty"},
	{"PR_INVALID_STATE_ERROR",    "Object state improper for request"},
	{"PR_NETWORK_DOWN_ERROR",    "Network is down"},
	{"PR_SOCKET_SHUTDOWN_ERROR",    "Socket shutdown"},
	{"PR_CONNECT_ABORTED_ERROR",    "Connection aborted"},
	{"PR_HOST_UNREACHABLE_ERROR",    "Host is unreachable"},
	{"PR_MAX_ERROR",    "Placeholder for the end of the list"},
	{0, 0}
};

static const struct PRErrorTable et = { text, "prerr", -6000L, 75 };

void nspr_InitializePRErrorTable(void) {
    PR_ErrorInstallTable(&et);
}
