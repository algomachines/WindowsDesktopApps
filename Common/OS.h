// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#ifdef _WIN32

#define WINDOWS

#pragma warning( once : 4244 )
#pragma warning( once : 4267 )
#pragma warning( once : 4018 )

// Windows

// // Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "WinINet.h"
#include "inttypes.h"
#include "memory.h"
#include "time.h"
#include "direct.h"
#include <vector>
#include <string>
#include "io.h"

#include "psapi.h"
#include "fcntl.h"

using namespace std;

typedef struct
{
	FLOAT x;
	FLOAT y;
} 	POINTF;

#define MCTRL_NO_EXPORT
#include "MC_EXPORT_DEF.h"
#include "MC_STRING.h"
#include "LSTTMPL.H"
#include "e_types.h"
#include "ProcessControl.h"

typedef struct
{
	uint64_t data[4];
} InstallID;

using namespace WorkbenchLib;

#include "random_number.h"
#include "Encryption.h"

#define ZERO(V) { ZeroMemory(&V,sizeof(V)); }

inline void Zero(string& s)
{
	memset(&s[0], 0, s.size());
}

inline void Zero(STRING& s)
{
	int i = 0;
	while (s[i]) s[i++] = 0;
}

inline void Zero(vector<uint8_t>& b)
{
	ZeroMemory(&b[0], b.size());
}

#else
// Linux

#endif


#include "file_tools.h"
#include "string_tools.h"
#include "time_tools.h"
#include "drawing_tools.h"

#define STB(data) { memmove((uint8_t *)b+i, &data, sizeof(data)); i += sizeof(data); }
#define LFB(data) { memmove(&data, (const uint8_t *)b+i, sizeof(data)); i += sizeof(data); }


#define STBv(vect)\
{\
	uint32_t n = (uint32_t)vect.size();\
	memmove((uint8_t *)b+i, &n, sizeof(n));\
	i += sizeof(n);\
\
	if (n)\
	{\
		memmove((uint8_t*)b + i, &vect[0], n * sizeof(vect[0]));\
		i += n * sizeof(vect[0]);\
	}\
}

#define LFBv(vect) \
{\
	uint32_t n;\
	memmove(&n, (const uint8_t *)b + i, sizeof(n));\
	i += sizeof(n);\
\
	vect.resize(n);\
	if (n)\
	{\
		memmove(&vect[0], (const uint8_t *)b+i,n*sizeof(vect[0]));\
		i += vect.size()*sizeof(vect[0]);\
	}\
}

#define GSSv(vect)  sizeof(uint32_t) + vect.size()*sizeof(vect[0]);