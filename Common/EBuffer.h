// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// Size of the expected value seed 
// Larger numbers make the program (much) harder to hack - 32 is already enormous, so probably big enough
#define SEED_SZ_BYTES 32

// Number of iterations to run when creating the expected value seed hash
// Larger numbers make the program harder to hack but (slightly) slower to run
#define SEED_HASH_NITER 64*1024

// 64K buffer size
#define E_BIN_BUFFER_SZ_BYTES 64*1024
// When creating the E_BIN_BUFFER, create 1K buffers recursively and use the last one
#define E_BIN_BUFFER_DEPTH 1024

// Used to store an encryption buffer, desigend to be used in a set of encryption buffers
class EBuffer
{
public:

	friend EBuffer;

	inline EBuffer(void)
	{
		m_seed.resize(SEED_SZ_BYTES);
	}

	inline bool operator < (const EBuffer &obj) const
	{
		if (memcmp(&m_seed[0], &obj.m_seed[0], SEED_SZ_BYTES) < 0)
			return true;

		return false;
	}

	inline bool operator > (const EBuffer &obj) const
	{
		if (memcmp(&m_seed[0], &obj.m_seed[0], SEED_SZ_BYTES) > 0)
			return true;

		return false;
	}

	inline void InitializeAsToken(const uint8_t *seed)
	{
		memmove(&m_seed[0], seed, SEED_SZ_BYTES);
	}

	// Depth makes it computatationally expensive to generate the random buffer
	inline void InitializeBuffer(void) const
	{
		const uint8_t *key = &m_seed[0];

		m_e_bin.resize(E_BIN_BUFFER_SZ_BYTES);

		for (int idepth = 0; idepth < E_BIN_BUFFER_DEPTH; idepth++)
		{
			uint32_t seed1;
			if (SEED_SZ_BYTES / 2 < sizeof(seed1)) __debugbreak();
			memmove(&seed1, key + (SEED_SZ_BYTES / 2), sizeof(seed1));

			CreateRandomBufferFromSeed(key, SEED_SZ_BYTES, seed1, &m_e_bin[0], E_BIN_BUFFER_SZ_BYTES);

			if (idepth == 0)
			{   // henceforth key will be the last bytes in the previous incarnation of the random buffer
				key = &m_e_bin[0];
				key += E_BIN_BUFFER_SZ_BYTES;
				key -= SEED_SZ_BYTES;
			}
		}
	}

	inline void Initialize(const uint8_t *seed)
	{
		InitializeAsToken(seed);
		InitializeBuffer();
	}

	const vector<uint8_t> *GetEBin(void) const
	{
		return &m_e_bin;
	}

private:
	vector<uint8_t> m_seed;
	mutable vector<uint8_t> m_e_bin;
};

