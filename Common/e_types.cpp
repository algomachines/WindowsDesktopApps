// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"
#include "e_types.h"
#include "Encryption.h"

vector<uint8_t> s_e_types_rand_buf;

int s_e_types_random_seed = 0;

void Initialize_e_types(void)
{
	ASSERT(s_e_types_rand_buf.size() == 0); //  Should only call this once

	const int buf_sz = 0x100000;
	if (s_e_types_rand_buf.size() == 0)
	{
		s_e_types_rand_buf.resize(buf_sz);

		SYSTEMTIME st;
		GetSystemTime(&st);
		uint64_t t;
		SystemTimeToFileTime(&st, (FILETIME*)&t);

		CreateRandomBufferFromSeed((const uint8_t*)&t, sizeof(t), 0, (uint8_t*)&s_e_types_rand_buf[0], s_e_types_rand_buf.size() * sizeof(uint8_t), 1);
	}
}