// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"
#include "Encryption.h"

#pragma once

inline bool SendBufferToServer(const char* url, int port, const char *component, const string& send_buf, string& receive_buf, STRING& err_msg, char term_char = 0, int tmout_ms = 1000)
{
	HINTERNET hInet=0, hConnection=0, hFile=0;

	bool status = false;
	while (1)
	{
		string s0;
		hInet = InternetOpen(DecodeText("3F5561214B77916F9E8C970D94073347160B07", s0)/*AlgoMachines*/, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInet == NULL)
		{
#ifdef _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " err: ";
			err_msg += GetLastError();
#endif
			break;
		}

		hConnection = InternetConnect(hInet, url, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

		if (hConnection == NULL)
		{
#ifdef _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " err: ";
			err_msg += GetLastError();
#endif
			break;
		}

		//if (component[0] == '/')
		//	component++;

		hFile = HttpOpenRequest(hConnection, DecodeText("69497CA3F6C704B1C6", s0)/*POST*/, component, NULL, NULL, NULL, 0, 0);

		if (hFile == NULL)
		{
#ifdef _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " err: ";
			err_msg += GetLastError();
#endif
			break;
		}

		if (HttpSendRequest(hFile, NULL, 0, (void *)send_buf.c_str(), send_buf.size()) == 0)
		{
#ifdef _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " err: ";
			err_msg += GetLastError();
#endif
			break;
		}

		DWORD avail_bytes = 0;
		int niter = 0;
		while (InternetQueryDataAvailable(hFile, &avail_bytes,0,0))
		{
			if (niter++ > tmout_ms / 10)
				break;

			if (avail_bytes == 0)
			{
				Sleep(10);
				continue;
			}

			string buf;
			buf.resize(avail_bytes);

			DWORD nread = 0;
			InternetReadFile(hFile, &buf[0], buf.size(), &nread);

			if (nread != avail_bytes)
			{
#ifdef _DEBUG
				err_msg = __FUNCTION__;
				err_msg += "() line=";
				err_msg += (UINT)__LINE__;
				err_msg += " err: ";
				err_msg += GetLastError();
#endif
				break;
			}

			receive_buf += buf;

			if (term_char && nread)
			{
				if (buf[nread - 1] == term_char)
				{
					status = true;
					break;
				}
			}
		}

		break;
	}

	if (hFile) InternetCloseHandle(hFile);
	if (hConnection) InternetCloseHandle(hConnection);
	if (hInet) InternetCloseHandle(hInet);

	int send_len = strlen(send_buf.c_str());

	return status;
}

#if 0
// Old implementation using curl.exe - 
inline bool SendBufferToServer(const char *url, const string& send_buf, string& receive_buf, STRING& err_msg)
{
	char dir[MAX_PATH];
	_getcwd(dir, sizeof(dir)); // Get current working directory

	char params[1024];
	string s0, s1, s2;

	sprintf_s(params, sizeof(params), "-d 0x%s %s", send_buf.c_str(), url);

	Zero(s0);
	Zero(s1);
	Zero(s2);

	STRING target_file;
	target_file.SetValidFileName(dir, DecodeText("4C8274701E07418239901C3073", s1)/*curl.txt*/);

	Zero(s1);

	if (DoesFileExist(target_file))
	{
		if (DeleteFile(target_file) == false)
		{
#if _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " Unable to delete file: ";
			err_msg += target_file;
#endif
			Zero(target_file);
			return false;
		}
	}

	SYSTEMTIME st;
	uint64_t t0, t1;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, (FILETIME*)&t0);
	DWORD timeout_sec = 10;
	bool status = RunProcess(DecodeText("1D46748E60F19A5E6606E6E63B6637", s1)/*curl.exe*/, dir, params, target_file, err_msg, timeout_sec);
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, (FILETIME*)&t1);

	uint64_t delta_ms = (t1 - t0) / 10000;

	Zero(s1);
	ZeroMemory(params, sizeof(params));
	ZeroMemory(dir, sizeof(dir));

	if (status == false)
	{
		Zero(target_file);
		return false;
	}

	if (DoesFileExist(target_file) == false)
	{
#if _DEBUG
		err_msg = __FUNCTION__;
		err_msg += "() line=";
		err_msg += (UINT)__LINE__;
		err_msg += " File does not exist: ";
		err_msg += target_file;
		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
		Zero(target_file);
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, target_file, DecodeText("A04D397EE2F6F7840684C8", s0)/*rb*/);

	Zero(s0);

	if (!stream)
	{
#if _DEBUG
		err_msg = __FUNCTION__;
		err_msg += "() line=";
		err_msg += (UINT)__LINE__;
		err_msg += " Unable to open file for reading: ";
		err_msg += target_file;
		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
		Zero(target_file);
		return false;
	}

	int len = _filelength(_fileno(stream));

	//	if (len == 0)
	//	{
	//#if _DEBUG
	//		err_msg = __FUNCTION__;
	//		err_msg += "() line=";
	//		err_msg += (UINT)__LINE__;
	//		err_msg += " File is empty: ";
	//		err_msg += target_file;
	//		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
	//#endif
	//		Zero(target_file);
	//		return false;
	//	}

	if (len > 0)
	{
		receive_buf.resize((size_t)len + 1);

		if (fread(&receive_buf[0], 1, len, stream) != len)
		{
#if _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " Problem reading file: ";
			err_msg += target_file;
			MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
			Zero(target_file);
			return false;
		}
	}

	fclose(stream);

	//GetSystemTime(&st);
	//char ext[64];
	//sprintf_s(ext, sizeof(ext), ".%04u%02u%02u_%02u%02u%02u.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//STRING new_file;
	//new_file.LoadFileName(target_file, TRUE, TRUE, FALSE);
	//new_file += ext;

	//rename(target_file, new_file);

	DeleteFile(target_file);

	Zero(target_file);

	return true;
}
#endif