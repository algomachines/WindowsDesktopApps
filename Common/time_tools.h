// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#ifdef WIN32
#include "sysinfoapi.h"
#define CLOCK_REALTIME 0

#ifndef _INC_TIME
struct timespec { int64_t tv_sec; int32_t tv_nsec; };    //header part
#endif

inline int clock_gettime(int, timespec* spec)      
{
	int64_t t; 
	GetSystemTimeAsFileTime((FILETIME*)&t);
	t -= 116444736000000000i64;  //1jan1601 to 1jan1970
	spec->tv_sec = t / 10000000i64;           //seconds
	spec->tv_nsec = t % 10000000i64 * 100;      //nano-seconds
	return 0;
}



inline int64_t get_time_zone_offset_ms(void)
{
	static int64_t g_tz_offset_ms = -1;

	if (g_tz_offset_ms == -1)
	{
		SYSTEMTIME st_local, st_utc;
		GetLocalTime(&st_local);
		GetSystemTime(&st_utc);

		uint64_t t_local_ms, t_utc_ms;

		SystemTimeToFileTime(&st_local, (FILETIME*)&t_local_ms);
		SystemTimeToFileTime(&st_utc, (FILETIME*)&t_utc_ms);

		if (t_local_ms >= t_utc_ms)
			g_tz_offset_ms = 0;
		else
		{
			g_tz_offset_ms = t_utc_ms - t_local_ms;
			g_tz_offset_ms /= 10000;
		}
	}

	return g_tz_offset_ms;
}

inline const char *get_local_time_as_string(uint64_t unix_time_ms, string &s)
{
	// Convert unix_time_ms to windows_time
	uint64_t windows_time_utc; // 100 ns increments
	{
		uint64_t windows_time_utc_ms = unix_time_ms + 11644473600000i64;  //1jan1601 to 1jan1970 - ms
		windows_time_utc = windows_time_utc_ms * 10000; // 100 ns increments
	}

	int64_t tz_offset = get_time_zone_offset_ms();

	tz_offset *= 10000; // convert to 100 ns units

	uint64_t windows_time_local = windows_time_utc - tz_offset;

	ULARGE_INTEGER li;
	li.QuadPart = windows_time_local;

	FILETIME ft;
	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	SYSTEMTIME st;
	ZeroMemory(&st, sizeof(st));
	if (FileTimeToSystemTime(&ft, &st) == 0)
	{
		DWORD err = GetLastError();
		short break_here = true;
	}

	char s0[256];
	sprintf_s(s0, sizeof(s0) - 1, "%04u/%02u/%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	s = s0;

	return s.c_str();
}

#endif

inline double compute_delta_time_ms(const timespec &t0, const timespec &t1)
{	
	uint64_t t0_us = t0.tv_sec * 1000000LLU;
	t0_us += t0.tv_nsec / 1000;
	
	uint64_t t1_us = t1.tv_sec * 1000000LLU;
	t1_us += t1.tv_nsec / 1000;
	
	if (t1_us >= t0_us)
	{
		return (t1_us - t0_us)/1000.0;
	}
		
	return (t0_us - t1_us)/1000.0;
}

inline timespec *get_future_time(timespec &t, uint64_t sec, uint64_t nsec=0)
{
	clock_gettime(CLOCK_REALTIME, &t);
	
	t.tv_sec += sec;
	
	if (nsec == 0)
		return &t;
	
	uint64_t n = nsec + t.tv_nsec;
	if (n >= 1000000000)
	{
		t.tv_sec++;
		t.tv_nsec = n%1000000000;
	}
		
	return &t;
}

// Returns the current time as a unix time (seconds since Jan 1, 1970) multiplied by 1000 (resolution ms)
inline uint64_t get_time_ms(void)
{
	timespec t;
	clock_gettime(CLOCK_REALTIME, &t);

	uint64_t t_ms = t.tv_sec * 1000LLU;
	t_ms += t.tv_nsec / 1000000LLU;

	return t_ms;
}
