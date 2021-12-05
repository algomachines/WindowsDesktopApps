#include "inttypes.h"
#include "string.h"
#include <vector>
#include "MurmurHash3.h"

#pragma once

#define E_VECTOR_INCLUDED

namespace WorkbenchLib
{

	// Encrypted string
	class e_vector
	{
	public:

		inline void initialize(void)
		{
			MurmurHash3_x86_32(&m_last_seed, sizeof(m_last_seed), m_nseeds_generated, &m_seed);
			m_last_seed = m_seed;
			m_nseeds_generated++;
			m_s = 0;
		}

		inline e_vector(void)
		{
			initialize();
		}

		inline e_vector(const char *s)
		{
			initialize();

			(*this) = s;
		}

		inline ~e_vector(void)
		{
			if (m_s)
			{
				delete[] m_s;
				m_s = 0;
			}
		}

		// random data source
		static std::vector<uint8_t> m_e;
		static uint32_t m_last_seed;
		static uint32_t m_nseeds_generated;

		inline bool StrStr(const char *substr, int &idx) const
		{
			int len = strlen(substr);
			for (int i = 0; i < m_data.size() - len; i++)
			{
				if (IsEqualTo(substr, i, len))
				{
					idx = i;
					return true;
				}
			}

			return false;
		}

		inline e_vector &operator = (const std::string &s)
		{
			m_data.resize(s.size() + 1);

			if (m_e.size() == 0)
			{
				memmove(&m_data[0], &s[0], s.size());
				return (*this);
			}

			uint32_t idx = m_seed % m_e.size();

			for (int i = 0; i < m_data.size(); i++)
			{
				uint8_t c = s[i] ^ m_e[idx++];
				idx %= m_e.size();
				m_data[i] = c;
			}

			set((int)s.size() - 1, 0); // null termination

			if (get((int)m_data.size() - 1)) __debugbreak(); // Make sure that there is a null termination

			{
				char buf[1024];
				extract((uint8_t *)buf, (int)(m_data.size() - 1));
				short break_here = true;
			}

			return (*this);
		}

		inline e_vector &operator += (const std::string &s)
		{
			if (s.size() == 0)
				return (*this);

			int n0 = (int)m_data.size();
			if (n0 && get(n0 - 1))
			{
				char buf[1024];
				extract((uint8_t *)buf, n0);
				__debugbreak(); // Make sure that this is a null terminated string
			}

			int n = n0 + (int)s.size();

			m_data.resize(n);

			for (int i = 0; i < s.size(); i++)
			{
				set(n0 - 1 + i, s[i]);
			}

			set(n - 1, 0); // null termination

			if (get((int)m_data.size() - 1)) __debugbreak(); // Make sure that there is a null termination

			{
				char buf[1024];
				extract((uint8_t *)buf, n);
				short break_here = true;
			}

			return (*this);
		}

		inline e_vector &operator = (const char *s)
		{
			int len = (int)strlen(s);

			assign((const uint8_t *)s, len + 1);

			return (*this);
		}

		inline e_vector &operator = (const e_vector &e)
		{
			m_data.resize(e.m_data.size());

			for (int i = 0; i < m_data.size(); i++)
			{
				set(i, e.get(i));
			}

			return (*this);
		}

		inline e_vector &operator = (const double &d)
		{
			assign((const uint8_t *)&d, sizeof(d));
			return (*this);
		}


		inline e_vector &operator = (const int &d)
		{
			assign((const uint8_t *)&d, sizeof(d));
			return (*this);
		}

		inline e_vector &operator = (const uint64_t &v)
		{
			assign((const uint8_t *)&v, sizeof(v));
			return (*this);
		}

		inline void set(int i, uint8_t v)
		{
			if (m_e.size() == 0)
			{
				m_data[i] = v;
				return;
			}

			if (i < 0 || i >= m_data.size()) __debugbreak();
			uint32_t idx = (m_seed + i) % m_e.size();
			m_data[i] = v ^ m_e[idx];
		}

		inline char operator[](int i) const
		{
			return get(i);
		}


		// Gets one unencrypted character
		inline uint8_t get(int i) const
		{
			if (i < 0 || i >= m_data.size()) __debugbreak();

			if (m_e.size() == 0)
				return m_data[i];

			uint32_t idx = (m_seed + i) % m_e.size();
			return m_data[i] ^ m_e[idx];
		}

		inline void assign(const uint8_t *v, int n)
		{
			m_data.resize(n);

			if (m_e.size() == 0)
			{
				memmove(&m_data[0], v, n);
				return;
			}

			uint32_t idx = m_seed % m_e.size();
			for (int i = 0; i < n; i++)
			{
				uint8_t c = v[i] ^ m_e[idx++];
				idx %= m_e.size();
				m_data[i] = c;
			}
		}

		inline void assign(const e_vector &s, int i0, int n)
		{
			int n0 = (int)m_data.size();
			m_data.resize(n + n0);
			for (int i = 0; i < n; i++)
			{
				set(i, s[i0 + i]);
			}
		}

		// Append null if necessary - used to terminate strings
		inline void append_null(void)
		{
			int n = (int)m_data.size();
			if (n && get(n - 1))
			{
				m_data.resize(n + 1);
				set(n, 0);
			}
		}

		// Extract unencrypted content
		// v_dest - holds the unencrypted content on return
		// n - number of bytes to extract
		inline void extract(uint8_t *v_dest, int n) const
		{
			int i = 0;
			while (i < n)
			{
				v_dest[i] = get(i);
				i++;
			}
		}

		inline size_t size(void) const
		{
			return m_data.size();
		}

		inline operator const char *()
		{
			if (m_s)
				delete[] m_s;
			m_s = new char[m_data.size()+1];

			for (int i = 0; i < m_data.size(); i++)
			{
				m_s[i] = get(i);
			}

			m_s[m_data.size()] = 0;

			return m_s;
		}

		inline operator int() const
		{
			if (m_data.size() != sizeof(int)) __debugbreak();
			int ret;
			extract((uint8_t *)&ret, sizeof(ret));
			return ret;
		}

		inline operator double() const
		{
			if (m_data.size() != sizeof(double)) __debugbreak();
			double ret;
			extract((uint8_t *)&ret, sizeof(ret));
			return ret;
		}

		inline operator uint64_t() const
		{
			if (m_data.size() != sizeof(double)) __debugbreak();
			uint64_t ret;
			extract((uint8_t *)&ret, sizeof(ret));
			return ret;
		}

		inline bool IsEqualTo(const char *s, int n = 0) const
		{
			if (!n)
			{
				n = (int)strlen(s);
				if (n != m_data.size() - 1)
					return false;
			}

			if (m_data.size() < n) return false;

			for (int i = 0; i < n; i++)
			{
				char c = get(i);
				if (s[i] != c)
					return false;
			}

			return true;
		}

		inline bool IsEqualTo(const char *s, int ibegin, int n) const
		{
			if (m_data.size() < ibegin + n) return false;

			int j = 0;
			for (int i = ibegin; i < ibegin + n; i++)
			{
				char c = get(i);

				if (s[j] != c)
					return false;

				j++;
			} 

			return true;
		}

		inline bool IsEqualTo(const e_vector &e, int n = 0) const
		{
			if (!n)
			{
				n = (int)e.size() - 1;
				if (n != m_data.size() - 1)
					return false;
			}

			if (m_data.size() < n) return false;

			for (int i = 0; i < n; i++)
			{
				char c = e.get(i);
				char c0 = get(i);
				if (e.get(i) != get(i))
					return false;
			}

			return true;
		}

		uint32_t m_seed;
		std::vector<uint8_t> m_data;
		char *m_s;
	};

	inline void append_to_string(const e_vector &e, std::string &s)
	{
		int n0 = (int)s.size();
		int n = n0 + (int)e.size() - 1;

		s.resize(n);

		int j = 0;
		for (int i = n0; i < n; i++)
		{
			s[i] = e[j++];
		}
	}
};