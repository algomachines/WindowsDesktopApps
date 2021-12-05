// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#pragma once

inline int filelength(const char *file_name)
{
  struct stat st;
  if (stat(file_name, &st))
    return -1;
  
  return (int)st.st_size;
}

#ifdef WIN32
inline int64_t filelength_64(const char* file_name)
{
	FILE* stream = 0;
	fopen_s(&stream, file_name, "rb");
	if (stream == 0)
		return -1;
	int64_t sz = _filelengthi64(_fileno(stream));
	fclose(stream);
	return sz;
}
#endif

inline bool DoesFileExist(const char* file_name)
{
#ifdef WIN32
	return _access(file_name, 0) ? false : true;
#else
	return access(file_name, 0) ? false : true;
#endif
}

#ifndef WIN32
inline bool DeleteFile(const char* file_name)
{
	if (DoesFileExist(file_name) == false)
		return true;
		
	return unlink(file_name) ? false : true;
}
#endif

inline bool make_unique_filename (std::string &unique_file_name, const char *file_name)
{
	for (int i=0; i<99; i++)
	{
		char ext[4];
#ifdef WIN32
		sprintf_s(ext, ".%02ld", i);
#else
		sprintf(ext,".%02d",i);
#endif
		unique_file_name = file_name;
		unique_file_name += ext;
		if (DoesFileExist (unique_file_name.c_str()) == false)
			return true;
	}
	
	return false;
}

inline bool WriteBufferToFile(const char* file_name, const vector<uint8_t>& buf)
{
	FILE* stream = fopen(file_name, "wb");
	if (stream == 0)
		return false;

	if (fwrite(&buf[0], 1, buf.size(), stream) != buf.size())
	{
		fclose(stream);
		return false;
	}

	fclose(stream);

	return true;
}

inline bool ReadFileToBuffer(const char* file_name, vector<uint8_t>& buf)
{
	if (DoesFileExist(file_name) == false)
		return false;

	int len = filelength(file_name);

	if (len <= 0)
		return false;

	buf.resize(len);

	FILE* stream = 0;

#ifdef WIN32
	fopen_s(&stream, file_name, "rb");
	if (stream == 0)
		return false;
#endif

	if (fread(&buf[0], 1, len, stream) != len)
	{
		fclose(stream);
		return false;
	}

	fclose(stream);

	return true;
}
