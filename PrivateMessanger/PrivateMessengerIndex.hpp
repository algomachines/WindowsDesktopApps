// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"

#include "SimpleDB.hpp"

class PrivateMessangerIndexElement
{
public:

	inline PrivateMessangerIndexElement(void)
	{
		m_reserved = 0;
		ZERO(m_Nickname);
		ZERO(m_remote_id);
	}

	inline bool operator < (const PrivateMessangerIndexElement& obj) const
	{
		return memcmp(m_remote_id, obj.m_remote_id, sizeof(m_remote_id)) < 0 ? true : false;
	}

	inline bool operator > (const PrivateMessangerIndexElement& obj) const
	{
		return memcmp(m_remote_id, obj.m_remote_id, sizeof(m_remote_id)) > 0 ? true : false;
	}

	static inline uint32_t GetSizeBytes(void)
	{
		return sizeof(m_reserved) + sizeof(m_Nickname) + sizeof(m_remote_id);
	}

	inline uint32_t LoadFromBuffer(const void* buf)
	{
		const uint8_t* b = (const uint8_t*)buf;
		int i = 0;
		LFB(m_reserved);
		LFB(m_Nickname);
		LFB(m_remote_id);

		return i;
	}

	inline uint32_t SaveToBuffer(void* buf) const
	{
		uint8_t* b = (uint8_t*)buf;
		int i = 0;
		STB(m_reserved);
		STB(m_Nickname);
		STB(m_remote_id);

		return i;
	}

	inline bool HasSameData(const PrivateMessangerIndexElement& obj) const
	{
		if (m_reserved != obj.m_reserved) return false;
		if (IsEqualTo(m_Nickname, obj.m_Nickname, TRUE, 32) == FALSE) return false;
		return true;
	}

	inline bool Update(const PrivateMessangerIndexElement& obj, string &err_msg)
	{
		m_reserved = obj.m_reserved;
		
		ZERO(m_Nickname);
		int i = 0;
		while (i < 32 && obj.m_Nickname[i])
		{
			m_Nickname[i] = obj.m_Nickname[i];
			i++;
		}

		return true;
	}

	inline bool Assign(const PrivateMessangerIndexElement& obj, string& err_msg)
	{
		memmove(m_remote_id, obj.m_remote_id, 32);

		m_reserved = obj.m_reserved;

		ZERO(m_Nickname);
		int i = 0;
		while (i < 32 && obj.m_Nickname[i])
		{
			m_Nickname[i] = obj.m_Nickname[i];
			i++;
		}

		return true;
	}

	inline bool SetNickname(const char* nickname) const 
	{
		if (IsEqualTo(nickname, m_Nickname, TRUE, sizeof(m_Nickname)) == FALSE)
		{
			ZERO(m_Nickname);
			if (nickname == 0 && m_Nickname[0])
			{
				if (m_Nickname[0])
					return false;

				return true;
			}

			int i=0;
			while (i < sizeof(m_Nickname) && nickname[i])
			{
				m_Nickname[i] = nickname[i];
				i++;
			}
			
			return true;
		}

		return false;
	}


	mutable uint64_t m_reserved;
	mutable char m_Nickname[32];
	uint8_t m_remote_id[32];
};
