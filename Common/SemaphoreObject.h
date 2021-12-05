// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"

#pragma once

#ifdef WIN32

class SemaphoreObject
{
public:

	inline SemaphoreObject(HANDLE& h, uint32_t tmout_ms)
	{
		m_h = 0;
		if (h == 0)
		{
			h = CreateSemaphore(NULL, 1, 1, NULL);
			if (h == NULL)
			{
				m_err = "Failed to create semaphore.";
				return;
			}
		}

		m_h = h;

		DWORD status = WaitForSingleObject(h, tmout_ms);

		if (status != WAIT_OBJECT_0)
		{

			if (status == WAIT_ABANDONED)
				m_err = "WAIT_ANANDONED";
			else if (status == WAIT_TIMEOUT)
				m_err = "WAIT_TIMEOUT";
			else if (status == WAIT_FAILED)
				m_err = "WAIT_FAILED";

			m_h = 0;
			return;
		}
	}

	inline bool Failed(string& msg)
	{
		if (m_err.size())
		{
			msg = m_err;
			return true;
		}

		return false;
	}

	inline ~SemaphoreObject(void)
	{
		ReleaseSemaphore(m_h, 1, NULL);
	}

private:

	HANDLE m_h;
	string m_err;
};

#endif
