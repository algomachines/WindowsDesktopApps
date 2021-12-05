// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define ERROR_LOCATION(err_msg) { err_msg += __FUNCTION__; err_msg += "() line="; append_integer(err_msg,__LINE__); err_msg += " : "; }

inline int find_in_buffer(const char* s_find, const vector<uint8_t>& buf)
{
	int len = StrLen(s_find);
	if (len == 0)
		return -1;

	if (len > buf.size())
		return -1;

	const uint8_t* b = &buf[0];
	int imax = buf.size() - len;
	for (int i = 0; i < imax; i++)
	{
		if (memcmp(s_find, b++, len) == 0)
			return i;
	}

	return -1;
}

inline void append_integer(std::string& s, uint32_t i)
{
	char snum[32];
	sprintf(snum, "%u", i);
	s += snum;
}

inline bool is_ascii_char(uint8_t c)
{
	if (c < 0x20 || c > 0x7F)
		return false;

	return true;
}

inline bool validate_is_ascii(const char* s, int max_len)
{
	for (int i = 0; i < max_len; i++)
	{
		if (s[i] == 0)
			return true;

		if (is_ascii_char(s[i]) == false)
			return false;
	}

	return true;
}

inline bool extract_port_from_url(const char* s_URL, STRING& url, int& port, string &err_msg)
{
	if (s_URL == 0)
	{
		ERROR_LOCATION(err_msg);
		err_msg += "parameter s_URL is NULL";
		return false;
	}

	port = 80;
	const char* s_port = strrchr(s_URL, ':');
	if (s_port)
	{
		s_port++;
		if (sscanf_s(s_port, "%ld", &port) != 1)
		{
			ERROR_LOCATION(err_msg);
			err_msg += "Invalid URL.";
			return false;
		}

		url.Insert(0, s_URL, s_port - s_URL - 1);
	}
	else
	{
		url = s_URL;
	}

	return true;
}


