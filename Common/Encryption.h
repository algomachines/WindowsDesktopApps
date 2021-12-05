// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "MurmurHash3.h"
#include "Windows.h"

#include "e_vector.h"
#include "e_types.h"

using namespace std;

// randomize_multipe - make this a large number e.g. 10000 to force a cpu intensive encryption / decription processing time
//                     by making decryption computationally expensive, the number of passwords that someone can test is limited
// This is the one function which does reversable data encryption, combining the e file with the buffer
// buffer - buffer to encrypt
// buffer_sz - size of buffer in bytes
// key - password or hash used to randomize the encryption
// key_sz - size of the key in bytes
// e - a random set of data used to encrypt the buffer
// e_sz - size of e in bytes
// randomize_interval - smaller for more obfuscation and slower speed, larger for faster but less obfuscation
__forceinline void __encrypt__(uint8_t *buffer, int32_t buffer_sz, uint8_t *key, int32_t key_sz, const uint8_t *e, uint32_t e_sz, uint32_t randomize_interval=1, uint32_t randomize_multiple=1)
{
#ifdef HIST
	// Create a histogram of indexes of e[] used
	if (hist == 0)
	{
		hist = new DWORD[e_sz];
		memset(hist, 0, sizeof(DWORD)*e_sz);
	}
#endif

#if _DEBUG
	if (e == 0)
		__debugbreak();
#endif

	uint32_t idx = 0;
	uint32_t _idx = 0xFFFFFFFF;
	uint32_t seed = (uint32_t)idx;

	for (int i = 0; i < buffer_sz; i++)
	{
		if ((i%randomize_interval) == 0)
		{
			for (int j = 0; j < (int)randomize_multiple; j++)
				MurmurHash3_x86_32(key, key_sz, seed, &idx);
		}

		idx %= e_sz;

		if (idx == _idx)
		{
			idx++;
			idx %= e_sz;
		}

		_idx = idx;

#ifdef HIST
		hist[idx]++;
#endif

		buffer[i] ^= e[idx];

		key[i%key_sz] = e[idx];

		idx++;
	}
}

// randomize_multipe - make this a large number e.g. 10000 to force a cpu intensive encryption / decription processing time
//                     by making decryption computationally expensive, the number of passwords that someone can test is limited
// This is the one function which does reversable data encryption, combining the e file with the buffer
// buffer - buffer to encrypt
// buffer_sz - size of buffer in bytes
// key - password or hash used to randomize the encryption
// key_sz - size of the key in bytes
// e - a random set of data used to encrypt the buffer
// e_sz - size of e in bytes
// ev - encrypted vector
// randomize_interval - smaller for more obfuscation and slower speed, larger for faster but less obfuscation
__forceinline void __encrypt__(const uint8_t *buffer, int32_t buffer_sz, uint8_t *key, int32_t key_sz, const uint8_t *e, uint32_t e_sz, WorkbenchLib::e_vector &ev)
{
#ifdef HIST
	// Create a histogram of indexes of e[] used
	if (hist == 0)
	{
		hist = new DWORD[e_sz];
		memset(hist, 0, sizeof(DWORD)*e_sz);
	}
#endif

#if _DEBUG
	if (e == 0)
		__debugbreak();
#endif

	uint32_t idx = 0;
	uint32_t _idx = 0xFFFFFFFF;
	uint32_t seed = (uint32_t)idx;

	//extern uint64_t g_t0;
	extern uint64_t g_debug_tick;

	if (ev.m_data.size() < buffer_sz)
		ev.m_data.resize(buffer_sz);

	uint32_t randomize_interval = 1;
	uint32_t randomize_multiple = 1;

	for (int i = 0; i < buffer_sz; i++)
	{
		if ((i%randomize_interval) == 0)
		{
			for (int j = 0; j < (int)randomize_multiple; j++)
				MurmurHash3_x86_32(key, key_sz, seed, &idx);
		}

		idx %= e_sz;

		if (idx == _idx)
		{
			idx++;
			idx %= e_sz;
		}

		_idx = idx;

#ifdef HIST
		hist[idx]++;
#endif

		ev.set(i, buffer[i] ^ e[idx]);

		key[i%key_sz] = e[idx];

		idx++;
	}
}


// file_name - fully qualified file name - will be created with random binary data
// sz - file size in bytes
bool CreateRandomFile(const char *file_name, int32_t sz, std::string &err_msg); 


inline bool create_random_buffer(uint8_t *b, uint32_t sz, const uint8_t *seed_hash)
{
	if (sz % 16)
	{
		__debugbreak();
		return false;
	}

//	const GUID g = { 0x60bc8156, 0xca43, 0x498e, { 0x97, 0xfb, 0x6b, 0x70, 0x8e, 0x79, 0x25, 0xf0 } };

	uint8_t hash[16];
	memmove(&hash, seed_hash, sizeof(hash));

	uint32_t seed;
	uint32_t n = sz / 16;

	uint32_t idx = 0;

	for (uint32_t i = 0; i < n; i++)
	{
		// Create a seed from the current hash
		memmove(&seed, hash, sizeof(seed));

		// Create a new hash from the current hash
		MurmurHash3_x64_128(hash, 16, seed, hash);

		memmove(b, hash, 16);
		b += 16;
	}

	return true;
}

// key - arbitrary size binary data to use to create the hash
// key_sz - size of the key
// seed - 4 byte seed
// b - buffer where random data will be written
// sz - size of buffer, will be completly filled with random data
// niter - number of iterations
// 
// Time required is about 1 or 2 seconds per GB per iteration
__forceinline bool CreateRandomBufferFromSeed(const uint8_t *key, int key_sz, uint32_t seed, uint8_t *b, uint32_t sz, int niter=1)
{
	uint8_t *b0 = b;
	for (int iter = 0; iter < niter; iter++)
	{
		uint8_t data[16];

		// Hash the key to 16 bytes
		if (iter == 0)
		{
			MurmurHash3_x86_128(key, key_sz, seed, &data);

			if (sz < sizeof(data))
			{
				memmove(b, data, sz);
				return true;
			}
		}
		else
		{
			memmove(data, &b[sz-sizeof(data)], sizeof(data));   // Refresh data[] from the end of the last buffer
			MurmurHash3_x86_32(data, sizeof(data), seed, &seed);   // Create a new seed
			MurmurHash3_x86_128(data, sizeof(data), seed, &data);  // Refresh data[] again
		}

		// The first 16 bytes of the output buffer b are the initial hash
		memmove(b, data, sizeof(data));
		uint32_t n = sizeof(data);


		if (n != sz)
		{
			for (uint32_t i = sizeof(data); i < sz; i += sizeof(data))
			{
				// Hash the previous 16 bytes to get the next 16 bytes
				MurmurHash3_x64_128(b, sizeof(data), seed, &data);

				b += sizeof(data);

				if ((sz - n) <= sizeof(data))
				{
					memmove(b, data, (sz - n));
					break;
				}

				memmove(b, data, sizeof(data));
				n += sizeof(data);
			}
		}

		b = b0; // rewind the output buffer pointer
	}

	return true;
}

inline bool symmetric_encryption(void* b, size_t buf_sz, const GUID& guid)
{
	uint8_t* buf = (uint8_t*)b;

	uint8_t key[sizeof(guid)];
	const uint8_t* p = (const uint8_t*)&guid;

	//printf("sizeof(guid)=%ld\n",sizeof(guid));

	e_int i;
	i = 0;
	while (1)
	{
		key[i] = (*p);
		if (i == sizeof(guid) - 1)
			break;

		p++;
		key[i] ^= (*p);

		i = i + 1;
	}

	__encrypt__(buf, buf_sz, key, sizeof(key), (const uint8_t*)&guid, sizeof(guid));

	return true;
}


inline bool symmetric_encryption(void* b, size_t buf_sz, const InstallID& install_id)
{
	uint8_t* buf = (uint8_t*)b;

	uint8_t key[sizeof(InstallID)];
	const uint8_t* p = (const uint8_t*)&install_id;

	e_int i;
	i = 0;
	while (1)
	{
		key[i] = (*p);
		if (i == sizeof(InstallID) - 1)
			break;

		p++;
		key[i] ^= (*p);

		i = i + 1;
	}

	__encrypt__(buf, buf_sz, key, sizeof(key), (const uint8_t*)&install_id, sizeof(InstallID));

	return true;
}


inline bool symmetric_encryption(void* b, size_t buf_sz, const void *v_key, int keysize)
{
	if (keysize < 16)
	{
		__debugbreak();
		return false;
	}

	uint32_t seed = 0;
	memmove(&seed, v_key, sizeof(seed));
	uint8_t key[16];
	MurmurHash3_x64_128(v_key, keysize, seed, key);

	__encrypt__((uint8_t *)b, (uint32_t)buf_sz, key, (uint32_t)sizeof(key), (const uint8_t*)v_key, keysize);

	return true;
}


inline void bin_to_hex_char(const uint8_t* buf, int buf_sz, string& s)
{
	s.resize(buf_sz * 2 + 1);

	e_int idx;
	idx = 0;
	for (int i = 0; i < buf_sz; i++)
	{
		int ic = (buf[i] & 0xF0) >> 4;
		if (ic < 0x0A)
			s[idx] = '0' + ic;
		else
			s[idx] = 'A' + ic - 10;

		idx = idx + 1;

		ic = (buf[i] & 0x0F);
		if (ic < 0x0A)
			s[idx] = '0' + ic;
		else
			s[idx] = 'A' + ic - 10;

		idx = idx + 1;
	}

	s[idx] = 0;
}

inline void bin_to_ascii_char(const uint8_t* buf, int buf_sz, string& s)
{
	int bits = buf_sz * 8;

	int nchar_out = bits / 6;
	int remainder = bits % 6;
	if (remainder)
		nchar_out++;

	s.resize(nchar_out + 1);


	// 0-9 : 10 : 10
	// A-Z : 26 : 36
	// a-z : 26 : 62
	// . :  1 : 63
	// ? :  1 : 64

	for (int i = 0; i < nchar_out; i++)
	{
		s[i] = 0;

		int v = 0;
		for (int ibit = 0; ibit < 6; ibit++)
		{
			int ibit_total = i * 6 + ibit;

			int ibyte_in = ibit_total / 8;
			int ibit_in = ibit_total % 8;

			if (ibyte_in == buf_sz)
				break;

			if ((buf[ibyte_in] & (0x01 << ibit_in)) == 0)
				continue;
			
			v |= 0x01 << ibit;
		}

		if (v < 10)
			s[i] = '0' + v;
		else if (v < 36)
			s[i] = 'A' + (v - 10);
		else if (v < 62)
			s[i] = 'a' + (v - 36);
		else if (v == 62)
			s[i] = '.';
		else if (v == 63)
			s[i] = '~';
		else
			__debugbreak();
	}
}

inline void bin_to_ascii_char(const vector<uint8_t>& bin, string& ascii)
{
	bin_to_ascii_char(&bin[0], bin.size(), ascii);
}

inline int get_ascii_binary_value(char c)
{
	// 0-9 : 10 : 10
	// A-Z : 26 : 36
	// a-z : 26 : 62
	// . :  1 : 63
	// ~ :  1 : 64

	if (c >= '0' && c <= '9')
		return c - '0';

	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;

	if (c >= 'a' && c <= 'z')
		return c - 'a' + 36;

	if (c == '.')
		return 62;

	if (c == '~')
		return 63;

	return 0;
}

inline void ascii_char_to_bin(const char *s, vector<uint8_t> &buf)
{
	int len = StrLen(s);

	int bits = len * 6;

	int sz = bits / 8;
	int remainder = bits % 8;
	if (remainder)
		sz++;

	buf.resize(sz);

	// 0-9 : 10 : 10
	// A-Z : 26 : 36
	// a-z : 26 : 62
	// . :  1 : 63
	// ~ :  1 : 64

	for (int ibyte = 0; ibyte < sz; ibyte++)
	{
		int v = 0;
		for (int ibit = 0; ibit < 8; ibit++)
		{
			int ibit_total = ibyte * 8 + ibit;

			int ichar = ibit_total / 6;
			int ichar_bit = ibit_total % 6;

			if (ichar == len)
			{
				buf.resize(ibyte);
				return;
			}

			int char_value = get_ascii_binary_value(s[ichar]);

			if ((char_value & (0x01 << ichar_bit)) == 0)
				continue;

			v |= 0x01 << ibit;
		}

		buf[ibyte] = v;
	}
}

inline bool hex_char_to_bin(const char* s, void* b, int b_sz)
{
	e_int n;

	n = b_sz * 2;

	uint8_t* buf = (uint8_t *)b;

	e_int idx;
	idx = 0;
	e_int i;
	for (i = 0; i < n; i = i + 1)
	{
		e_int v;
		v = 0;

		if (s[i] >= '0' && s[i] <= '9')
			v = s[i] - '0';
		else if (s[i] >= 'A' && s[i] <= 'F')
			v = 10 + s[i] - 'A';
		else
			return 0; // invalid character

		if (i % 2 == 0)
			buf[idx] = v << 4;
		else
		{
			buf[idx] += v;
			idx = idx + 1;
		}
	}

	return true;
}

inline const uint8_t *hex_char_to_bin(const string& s, vector<uint8_t>& buf)
{
	e_int n;

	n = strlen(s.c_str());

	if (n % 2)
		return 0; // the string must have an even number of characters

	buf.resize(n / 2);

	if (hex_char_to_bin(s.c_str(), &buf[0], n / 2) == false)
		return 0;

	return &buf[0];
}

inline bool EncodeText(const char* s, string& s_encoded)
{
	e_int len;
	len = StrLen(s);
	if (len < 1)
		return false;

	SYSTEMTIME st;
	GetLocalTime(&st);

	uint32_t seed;

	MurmurHash3_x86_32(&st, sizeof(st), st.wMilliseconds, &seed);

	e_int len_extra;
	len_extra = st.wMilliseconds % max(len / 2, 5);
	len_extra = len_extra + 1; // Make sure that it is always at least one, for a null terminator

	vector<uint8_t> buf;
	buf.resize(len + len_extra + sizeof(seed));
	memset(&buf[0], 0, buf.size());

	// Move the text to the beginning of the buffer
	memmove(&buf[0], (const char*)s, len);

	// {464EEEA8-F27E-4EE2-9433-39D2F6062FF7}
	GUID guid = { 0x464eeea8, 0xf27e, 0x4ee2, { 0x94, 0x33, 0x39, 0xd2, 0xf6, 0x6, 0x2f, 0xf7 } };

	// Scramble the guid with the seed
	memmove(&guid, &seed, sizeof(seed));

	// Append the seed the the end of the buffer
	memmove(&buf[len + len_extra], &seed, sizeof(seed));

	// Encrypt the buffer, not inluding the seed
	symmetric_encryption(&buf[0], buf.size() - sizeof(seed), guid);

	bin_to_hex_char(&buf[0], buf.size(), s_encoded);

	return true;
}

inline const char *DecodeText(const char* s, string &s_decoded)
{
	memset(&s_decoded[0], 0, s_decoded.size());

	e_int len;
	len = StrLen(s);
	if (len < 6)
		return "";

	vector<uint8_t> buf;
	hex_char_to_bin(s, buf);

	uint32_t seed;
	memmove(&seed, &buf[buf.size() - sizeof(seed)], sizeof(seed));

	// {464EEEA8-F27E-4EE2-9433-39D2F6062FF7}
	GUID guid = { 0x464eeea8, 0xf27e, 0x4ee2, { 0x94, 0x33, 0x39, 0xd2, 0xf6, 0x6, 0x2f, 0xf7 } };

	// Scramble the guid with the seed
	memmove(&guid, &seed, sizeof(seed));

	// Encrypt the buffer, not inluding the seed
	symmetric_encryption(&buf[0], buf.size() - sizeof(seed), guid);

	len = 0;
	while (len < buf.size())
	{
		if (buf[len])
		{
			len = len + 1;
			continue;
		}

		break;
	}

	s_decoded.resize(len + 1);

	e_int i;
	for (i = 0; i < len; i = i + 1)
		s_decoded[i] = buf[i];
	
	s_decoded[len] = 0;

	return s_decoded.c_str();
}

