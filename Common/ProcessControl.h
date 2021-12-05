// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace WorkbenchLib
{
	bool RunProcess
	(
		const char *exe_file,
		const char *working_directory,
		const char *params,
		const char *target_file,
		STRING &err_msg,
		DWORD timeout_seconds=INFINITE,
		DWORD *exit_code=0,
		bool bShowProcess=false
	);

	// Start a process and return immediately
	bool StartProcess
	(
		const char *exe_file,
		const char *working_directory,
		const char *params,
		HANDLE &process_handle,
		HWND &process_hwnd,
		STRING &err_msg,
		const char *target_file=0,
		HANDLE *h_target_file=0,
		DWORD timeout_seconds=INFINITE,
		bool bShowProcess=false
	);
}