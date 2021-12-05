// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace WorkbenchLib
{
	inline BOOL CopyTextToClipboard(HWND hWnd, HANDLE text_data_handle)
	{
		// Given text_data handle is assumed to be in CF_TEXT format:
		//   CR/LF deliminated and null terminated text string

		::OpenClipboard(hWnd);

		UINT clipboard_format = CF_TEXT;

		HANDLE ret;

		::EmptyClipboard();
		ret = ::SetClipboardData(CF_TEXT, text_data_handle);
		::CloseClipboard();
		return TRUE;
	}

	inline BOOL CopyStringToClipboard(const char *s, HWND hWnd/*=0*/)
	{
		if (hWnd == 0)
			hWnd = ::GetDesktopWindow();

		DWORD nbytes = StrLen(s);
		nbytes++;

		////////////////////////////////////////////////////////////
		// Allocate the memory and get a handle
		UINT alloc_parameters = GMEM_NODISCARD | GMEM_MOVEABLE;
		HANDLE h = ::GlobalAlloc(alloc_parameters, nbytes);

		if (!h)
			return FALSE;

		// Check the allocation
		DWORD check = GlobalSize(h);
		if (check < nbytes)
			return FALSE;
		/////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////
		// Get a pointer to the allocated memory
		char *buffer = (char *) ::GlobalLock(h);
		if (!buffer)
			return FALSE;
		const char *buffer0 = buffer;
		/////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////
		// Move data into the buffer
		MemMove(buffer, s, StrLen(s));
		buffer[nbytes - 1] = 0;
		/////////////////////////////////////////////////////////////

		::GlobalUnlock(h);

		return CopyTextToClipboard(hWnd, h);
	}

	inline BOOL CanPaste(HWND hWnd)
	{
		UINT clipboard_format = CF_TEXT;

		if (::OpenClipboard(hWnd) == FALSE)
			return FALSE;

		HANDLE text_data_handle = ::GetClipboardData(clipboard_format);

		::CloseClipboard();

		if (text_data_handle)
			return TRUE;

		return FALSE;
	}

}