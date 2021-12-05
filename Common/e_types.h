// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

extern vector<uint8_t> s_e_types_rand_buf;

void Initialize_e_types();

inline void e_types_symmetric_encrypt(void* buf, size_t sz, int idx)
{
	if (idx == -1)
		return;

	if (s_e_types_rand_buf.size() == 0)
	{
		// if there is no random buffer, then don't encrypt
		idx = -1;
		return;
	}

	uint8_t* b = (uint8_t*)buf;

	int i = 0;
	while (i < sz)
	{
		idx %= s_e_types_rand_buf.size();
		b[i] ^= s_e_types_rand_buf[idx];
		i++;
		idx++;
	}
}

inline int e_types_random_idx(int seed)
{
	if (s_e_types_rand_buf.size() == 0)
		return -1;

	seed %= s_e_types_rand_buf.size();

	union
	{
		int idx;
		uint8_t u8[4];
	};

	int i = 0;
	while (i < 4)
	{
		u8[i] = s_e_types_rand_buf[seed];
		seed++;
		seed %= s_e_types_rand_buf.size();
		i++;
	}

	idx %= s_e_types_rand_buf.size();
	return idx;
}

inline int e_types_encrypt_int(int v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

inline int e_types_decrypt_int(int v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

static int e_types_random_seed;

// Encapsulates an encrypted single integer
class e_int
{
public:

	inline e_int(void)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = 0;
		}

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_int((int)0, m_idx);
	}

	inline operator int (void) const
	{
		int v = e_types_decrypt_int(m_data, m_idx);
		
		if (s_e_types_rand_buf.size() == 0)
			return v;

		// Reshuffle
		m_idx = e_types_random_idx(m_idx + v); 
		m_data = e_types_encrypt_int(v, m_idx);

		return v;
	}

	inline e_int &operator = (const int v)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = v;
		}
		else
		{
			m_idx = e_types_random_idx(m_idx + v); // Get a random index for this assignment
			m_data = e_types_encrypt_int(v, m_idx);
		}

		return *this;
	}

private:

	mutable int m_data;
	mutable int m_idx;
};

inline uint64_t e_types_encrypt_uint64(uint64_t v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

inline uint64_t e_types_decrypt_uint64(uint64_t v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

// Encapsulates an encrypted single uint64
class e_uint64
{
public:

	inline e_uint64(void)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = 0;
		}

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_uint64((uint64_t)0, m_idx);
	}

	inline operator uint64_t (void) const
	{
		uint64_t v = e_types_decrypt_uint64(m_data, m_idx);

		if (s_e_types_rand_buf.size() == 0)
			return v;

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_uint64(v, m_idx);

		return v;
	}

	inline e_uint64& operator = (const uint64_t v)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = v;
		}
		else
		{
			m_idx = e_types_random_idx(m_idx + v); // Get a random index for this assignment
			m_data = e_types_encrypt_uint64(v, m_idx);
		}

		return *this;
	}

private:

	mutable uint64_t m_data;
	mutable int m_idx;
};

inline uint32_t e_types_encrypt_uint32(uint32_t v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

inline uint32_t e_types_decrypt_uint32(uint32_t v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

// Encapsulates an encrypted single uint32
class e_uint32
{
public:

	inline e_uint32(void)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = 0;
		}

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_uint32((uint32_t)0, m_idx);
	}

	inline operator uint32_t (void) const
	{
		uint32_t v = e_types_decrypt_uint32(m_data, m_idx);

		if (s_e_types_rand_buf.size() == 0)
			return v;

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_uint32(v, m_idx);

		return v;
	}

	inline e_uint32& operator = (const uint32_t v)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = v;
		}
		else
		{
			m_idx = e_types_random_idx(m_idx + v); // Get a random index for this assignment
			m_data = e_types_encrypt_uint32(v, m_idx);
		}

		return *this;
	}

private:

	mutable uint32_t m_data;
	mutable int m_idx;
};

inline double e_types_encrypt_double(double v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

inline double e_types_decrypt_double(double v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

// Encapsulates an encrypted single double
class e_double
{
public:

	inline e_double(void)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = 0;
		}

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_double((double)0, m_idx);
	}

	inline operator double (void) const
	{
		double v = e_types_decrypt_double(m_data, m_idx);

		if (s_e_types_rand_buf.size() == 0)
			return v;

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_double(v, m_idx);

		return v;
	}

	inline e_double& operator = (const double v)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = v;
		}
		else
		{
			m_idx = e_types_random_idx(m_idx + v); // Get a random index for this assignment
			m_data = e_types_encrypt_double(v, m_idx);
		}

		return *this;
	}

private:

	mutable double m_data;
	mutable int m_idx;
};

inline float e_types_encrypt_float(float v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

inline float e_types_decrypt_float(float v, int idx)
{
	e_types_symmetric_encrypt(&v, sizeof(v), idx);
	return v;
}

// Encapsulates an encrypted single float
class e_float
{
public:

	inline e_float(void)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = 0;
		}

		m_idx = e_types_random_idx(e_types_random_seed++);
		m_data = e_types_encrypt_float((float)0, m_idx);
	}

	inline operator float (void) const
	{
		float v = e_types_decrypt_float(m_data, m_idx);

		if (s_e_types_rand_buf.size() == 0)
			return v;

		m_idx = e_types_random_idx(m_idx + v); // Reshuffle
		m_data = e_types_encrypt_float(v, m_idx);

		return v;
	}

	inline e_float& operator = (const float v)
	{
		if (s_e_types_rand_buf.size() == 0)
		{
			m_idx = -1;
			m_data = v;
		}
		else
		{
			m_idx = e_types_random_idx(m_idx + v); // Get a random index for this assignment
			m_data = e_types_encrypt_float(v, m_idx);
		}

		return *this;
	}

private:

	mutable float m_data;
	mutable int m_idx;
};

class e_SIZE
{
public:
	e_int cx;
	e_int cy;

	inline e_SIZE &operator = (const SIZE& sz)
	{
		cx = sz.cx;
		cy = sz.cy;
		return *this;
	}

	inline operator SIZE (void) const
	{
		SIZE sz;
		sz.cx = cx;
		sz.cy = cy;
		return sz;
	}
};

class e_POINTF
{
public:

	e_float x;
	e_float y;

	inline e_POINTF& operator = (const POINTF& pt)
	{
		x = pt.x;
		y = pt.y;
		return *this;
	}
};