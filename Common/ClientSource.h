// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"
#include "io.h"
#include "ProcessControl.h"
#include "DRM_ProgramRecord.h"

#pragma once

inline const char *GetConfigurationFileName(STRING& file_name)
{
	string s0;

	file_name.SetValidFileName(s_data_dir, DecodeText("70986060465401E05A9C7EE27EF3948230F30E", s0)/*Config.bin*/);

	Zero(s0);

	return file_name.GetStringConst();
}

inline const char* GetByteCodeFileName(STRING& bytecode_file)
{
	STRING config_file;
	bytecode_file.LoadFileName(GetConfigurationFileName(config_file), TRUE, FALSE, FALSE);
	bytecode_file.AppendSubDir("code.bin");
	return bytecode_file.GetStringConst();
}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash - 16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
bool ValidatePasswordProtectedConfigFile(const vector<uint8_t>& pwd_hash, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Config file does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	FILE* stream = 0;

	fopen_s(&stream, file_name, "rb");

	if (stream == 0)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Unable to open the config file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	return true;
}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash - 16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool SaveRemoteServerURLToConfigFile(const char* s_URL, const vector<uint8_t>& pwd_hash, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg += "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	if (flen > 32 + 4 + 16)
	{
		// Don't need to add the URL
		return true;
	}

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "r+b");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	// Verify the password
	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	uint8_t instance_hash[16];
	if (fread(&instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int url_len = flen - sizeof(data) + sizeof(instance_hash);

	vector<uint8_t> b;
	b.resize(strlen(s_URL) + 1);
	memmove(&b[0], s_URL, b.size());

	symmetric_encryption(&b[0], b.size(), &pwd_hash[0], pwd_hash.size());

	if (fwrite(&b[0], 1, b.size(), stream) != b.size())
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to write to config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	return true;
}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool GetURLFromConfigFile(string &url, const vector<uint8_t>& pwd_hash, string& err_msg, const char *config_file=0)
{
	STRING file_name;

	if (config_file == 0)
		GetConfigurationFileName(file_name);
	else
		file_name = config_file;

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg += "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "rb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	uint8_t instance_hash[16];
	if (fread(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int url_len = flen - (sizeof(data) + sizeof(instance_hash));
	if (url_len == 0)
	{
		fclose(stream);
		url = "";
		return true; // need to prompt for the URL
	}

	url.resize(url_len);
	if (fread(&url[0], 1, url_len, stream) != url_len)
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	symmetric_encryption(&url[0], url.length(), &pwd_hash[0], pwd_hash.size());

	return true;
}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
// prev_instance_hash is returned if this variable is not zero
inline bool UpdateInstanceHash(const vector<uint8_t>& pwd_hash, const uint8_t* instance_hash, uint8_t *prev_instance_hash, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		ERROR_LOCATION(err_msg);
		err_msg = "File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	if (flen < 32 + 4 + 16)
	{
		ERROR_LOCATION(err_msg);
		err_msg += "Invalid config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "r+b");
	if (!stream)
	{
		ERROR_LOCATION(err_msg);
		err_msg += "Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	//// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		ERROR_LOCATION(err_msg);
		err_msg += "Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	// Verify the password
	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		ERROR_LOCATION(err_msg);
		err_msg += "Incorrect password";
		return false;
	}

	if (prev_instance_hash) // read the previous instance hash if a buffer is provided to return this value - seeks are expensive, so best to avoid this if not necessary
	{
		if (fseek(stream, 36, SEEK_SET))
		{
			fclose(stream);
			ERROR_LOCATION(err_msg);
			err_msg += "Problem moving to position in file: ";
			err_msg += (const char*)file_name;
			return false;
		}

		if (fread(prev_instance_hash, 1, 16, stream) != 16)
		{
			fclose(stream);
			ERROR_LOCATION(err_msg);
			err_msg += "Problem trying to read config file: ";
			err_msg += (const char*)file_name;
			return false;
		}
	}

	if (fseek(stream, 36, SEEK_SET))
	{
		fclose(stream);
		ERROR_LOCATION(err_msg);
		err_msg += "Problem moving to position in file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	if (fwrite(instance_hash, 1, 16, stream) != 16)
	{
		fclose(stream);
		ERROR_LOCATION(err_msg);
		err_msg += "Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);
	return true;
}


// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool GetInstanceHash(const vector<uint8_t>& pwd_hash, vector<uint8_t>& instance_hash, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg = "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	if (flen < 32 + 4 + 16)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Invalid config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "rb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	//// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	// Verify the password
	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	instance_hash.resize(16);
	if (fread(&instance_hash[0], 1, instance_hash.size(), stream) != instance_hash.size())
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);
	return true;
}


// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool CreatePasswordProtectedConfigFileIfNecessary(const vector<uint8_t>& pwd_hash, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name))
		return true;

	string s0;

	SYSTEMTIME st;
	GetSystemTime(&st);
	uint32_t seed = (uint32_t)st.wSecond * 1000;
	seed *= (uint32_t)st.wMilliseconds;

	InstallID install_id;
	create_random_buffer((uint8_t*)&install_id, (uint32_t)sizeof(install_id));

	FILE* stream = 0;
	fopen_s(&stream, file_name, "wb");

	Zero(s0);

	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += file_name.GetStringConst();
		return false;
	}

	uint32_t checksum;
	MurmurHash3_x86_32(&install_id, sizeof(install_id), 0, &checksum);

	uint8_t data[sizeof(install_id) + sizeof(checksum)];
	memmove(data, &install_id, sizeof(install_id));
	memmove(data + sizeof(install_id), &checksum, sizeof(checksum));

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	if (fwrite(&data, 1, sizeof(data), stream) != sizeof(data))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += file_name.GetStringConst();
		fclose(stream);
		DeleteFile(file_name);
		return false;
	}

	uint8_t instance_hash[16];
	ZERO(instance_hash); // Initially zero

	if (fwrite(&instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += file_name.GetStringConst();
		fclose(stream);
		DeleteFile(file_name);
		return false;
	}

	fclose(stream);

	return true;
}

//inline bool CreateConfigFileIfNecessary(void)
//{
//	STRING file_name;
//	GetConfigurationFileName(file_name);
//
//	if (DoesFileExist(file_name) == false)
//	{
//		string s0;
//
//		string err_msg;
//		SYSTEMTIME st;
//		GetSystemTime(&st);
//		uint32_t seed = (uint32_t)st.wSecond * 1000;
//		seed *= (uint32_t)st.wMilliseconds;
//
//		InstallID install_id;
//		create_random_buffer((uint8_t *)&install_id, (uint32_t)sizeof(install_id));
//
//		FILE* stream = 0;
//		fopen_s(&stream, file_name, DecodeText("5879457EF2E9E9891B45", s0)/*wb*/);
//
//		Zero(s0);
//
//		if (!stream)
//			return false;
//
//		if (fwrite(&install_id, 1, sizeof(install_id), stream) != sizeof(install_id))
//		{
//			fclose(stream);
//			DeleteFile (file_name);
//			return false;
//		}
//
//		uint8_t instance_hash[16];
//		ZERO(instance_hash);
//
//		if (fwrite(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
//		{
//			fclose(stream);
//			if (DeleteFile(file_name) == FALSE)
//				return false;
//		}
//
//		fclose(stream);
//
//		return true;
//	}
//
//	return true;
//}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool GetInstallID(InstallID &install_id, const vector<uint8_t> &pwd_hash, string &err_msg)
{
	if (CreatePasswordProtectedConfigFileIfNecessary(pwd_hash, err_msg) == false)
		return false;

	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg = "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	if (flen < 32 + 4)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Invalid config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "rb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	// Validate the config file / password hash
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	symmetric_encryption(data, sizeof(data), &pwd_hash[0], pwd_hash.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	// Verify the password
	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	memmove(&install_id, data, sizeof(install_id));

	return true;
}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool ChangeConfigFilePassword(const vector<uint8_t>& pwd_old, const vector<uint8_t>& pwd_new, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg += "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "rb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd_old[0], pwd_old.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	uint8_t instance_hash[16];
	if (fread(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int url_len = flen - (sizeof(data) + sizeof(instance_hash));
	if (url_len == 0)
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Server url not present, can't change the password: ";
		err_msg += (const char*)file_name;
		return true; // need to prompt for the URL
	}

	string url;
	url.resize(url_len);
	if (fread(&url[0], 1, url_len, stream) != url_len)
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	symmetric_encryption(&url[0], url.length(), &pwd_old[0], pwd_old.size()); // decrypt the url

	STRING backup_file;
	backup_file = file_name;
	backup_file += ".000";

	fopen_s(&stream, (const char*)backup_file, "wb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't create file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	// now we are ready to reconstruct the config file with the new password
	// encrypt the id and the checksum with the new password
	symmetric_encryption(data, sizeof(data), &pwd_new[0], pwd_new.size());

	// Write the id and the checksum
	if (fwrite(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	if (fwrite(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	// re-encrypt the URL with the new password
	string url_org = url;
	symmetric_encryption(&url[0], url.length(), &pwd_new[0], pwd_new.size());

	if (fwrite(&url[0], 1, url.length(), stream) != url.length())
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	fclose(stream);

	// validate
	if (GetURLFromConfigFile(url, pwd_new, err_msg, backup_file) == false)
	{
		_unlink(backup_file);
		return false;
	}

	if (strcmp(url.c_str(), url_org.c_str()))
	{
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to validate replacment config file: ";
		err_msg += (const char*)backup_file;
	}

	if (_unlink(file_name))
	{
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Unable to delete file: ";
		err_msg += (const char*)file_name;
	}

	if (rename(backup_file, file_name))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Unable to rename file: ";
		err_msg += (const char*)backup_file;
		err_msg += " -> ";
		err_msg += (const char*)file_name;
		return false;
	}

	return true;

}

// CONFIG FILE CONTENTS
// 
// 1) InstallID - 32 bytes
// 2) Checksum - 4 bytes
// Items 1 and 2 encrypted with the password hash
// 3) Instance hash -  16 bytes - encrypted on the server
// 4) URL/IP address of the server - encrypted with the password hash
//
inline bool ChangeServerURL(const vector<uint8_t>& pwd, const char *new_url, string& err_msg)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	if (DoesFileExist(file_name) == false)
	{
		err_msg = __FUNCTION__;
		err_msg += "() File does not exist: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int flen = filelength(file_name);

	FILE* stream = 0;
	fopen_s(&stream, (const char*)file_name, "rb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't open file for reading: ";
		err_msg += (const char*)file_name;
		return false;
	}

	// Validate the config file / password hash
	InstallID install_id;
	uint32_t checksum;

	uint8_t data[sizeof(install_id) + sizeof(checksum)];

	if (fread(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	symmetric_encryption(data, sizeof(data), &pwd[0], pwd.size());

	MurmurHash3_x86_32(data, sizeof(install_id), 0, &checksum);

	if (memcmp(&checksum, &data[sizeof(install_id)], sizeof(checksum)))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Incorrect password";
		return false;
	}

	uint8_t instance_hash[16];
	if (fread(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	int url_len = flen - (sizeof(data) + sizeof(instance_hash));
	if (url_len == 0)
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Server url not present: ";
		err_msg += (const char*)file_name;
		return true; // need to prompt for the URL
	}

	int ipos = ftell(stream); // get the position of the URL

	string url;
	url.resize(url_len);
	if (fread(&url[0], 1, url_len, stream) != url_len)
	{
		fclose(stream);
		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to read config file: ";
		err_msg += (const char*)file_name;
		return false;
	}

	fclose(stream);

	symmetric_encryption(&url[0], url.length(), &pwd[0], pwd.size()); // decrypt the url

	if (!strcmp(new_url, url.c_str()))
		return true; // no changes

	vector<uint8_t> new_url_buf;
	new_url_buf.resize(StrLen(new_url) + 1);

	memmove(&new_url_buf[0], new_url, new_url_buf.size());

	symmetric_encryption(&new_url_buf[0], new_url_buf.size(), &pwd[0], pwd.size()); // encrypt the new url

	STRING backup_file;
	backup_file = file_name;
	backup_file += ".000";

	fopen_s(&stream, (const char*)backup_file, "wb");
	if (!stream)
	{
		err_msg = __FUNCTION__;
		err_msg += "() Can't create file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	// now we are ready to reconstruct the config file with the new URL
	// encrypt the id and the checksum with the new password
	symmetric_encryption(data, sizeof(data), &pwd[0], pwd.size());

	// Write the id and the checksum
	if (fwrite(data, 1, sizeof(data), stream) != sizeof(data))
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	if (fwrite(instance_hash, 1, sizeof(instance_hash), stream) != sizeof(instance_hash))
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	if (fwrite(&new_url_buf[0], 1, new_url_buf.size(), stream) != new_url_buf.size())
	{
		fclose(stream);
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem writing to file: ";
		err_msg += (const char*)backup_file;
		return false;
	}

	fclose(stream);

	// validate
	if (GetURLFromConfigFile(url, pwd, err_msg, backup_file) == false)
	{
		_unlink(backup_file);
		return false;
	}

	if (strcmp(url.c_str(), new_url))
	{
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Problem trying to validate replacment config file: ";
		err_msg += (const char*)backup_file;
	}

	if (_unlink(file_name))
	{
		_unlink(backup_file);

		err_msg = __FUNCTION__;
		err_msg += "() Unable to delete file: ";
		err_msg += (const char*)file_name;
	}

	if (rename(backup_file, file_name))
	{
		err_msg = __FUNCTION__;
		err_msg += "() Unable to rename file: ";
		err_msg += (const char*)backup_file;
		err_msg += " -> ";
		err_msg += (const char*)file_name;
		return false;
	}

	return true;
}


inline bool GetHashedInstallID_ascii(string &s, const vector<uint8_t>& pwd_hash, string& err_msg)
{
	InstallID id;
	if (GetInstallID(id, pwd_hash, err_msg) == false)
		return false;

	uint8_t* b = (uint8_t*)&id;
	hash_id(b);
	hash_id(b);

	bin_to_ascii_char(b, sizeof(InstallID), s);
	return true;
}

inline bool GetHashedInstallID(vector<uint8_t>& hashed_install_id, const vector<uint8_t>& pwd_hash, string& err_msg)
{
	InstallID id;
	if (GetInstallID(id, pwd_hash, err_msg) == false)
		return false;

	uint8_t* b = (uint8_t*)&id;
	hash_id(b);
	hash_id(b);

	hashed_install_id.resize(sizeof(id));
	memmove(&hashed_install_id[0], b, sizeof(id));

	return true;
}

// e_url - complete url (eg. http://104.168.157.47/cgi-bin/OwnershipValidation_Demo2.cgi) as encoded text
// send_buf - binary data will be converted to hex text for sending
// receive_buf - binary data converted from hex text on receipt
inline bool SendBufferToServer(const char *e_url, const vector<uint8_t> &send_buf, vector<uint8_t> &receive_buf, STRING& err_msg)
{
	char params[1024];

	// From of the query
	// curl -d "buf=http://104.168.157.47/cgi-bin/OwnershipValidation_Demo2.cgi"

	string s0,s_format;
	DecodeText("DAF05E0D80C849A9690D0DBD392FD25C42BD4C", s_format)/*-d "buf=%s"*/;

	sprintf_s(params, sizeof(params) - 1, s_format.c_str(), DecodeText(e_url, s0));
	Zero(s_format);
	Zero(s0);

	STRING target_file;
	target_file.SetValidFileName(s_data_dir, DecodeText("4C8274701E07418239901C3073", s0)/*curl.txt*/);

	Zero(s0);

	if (DoesFileExist(target_file))
	{
		if (DeleteFile(target_file) == false)
		{
#if _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " Unable to delete file: ";
			err_msg += target_file;
#endif
			Zero(target_file);
			return false;
		}
	}

	SYSTEMTIME st;
	uint64_t t0, t1;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, (FILETIME*)&t0);
	DWORD timeout_sec = 10;
	bool status = RunProcess(DecodeText("1D46748E60F19A5E6606E6E63B6637", s0)/*curl.exe*/, s_data_dir, params, target_file, err_msg, timeout_sec);
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, (FILETIME*)&t1);

	uint64_t delta_ms = (t1 - t0) / 10000;

	Zero(s0);
	ZeroMemory(params, sizeof(params));

	if (status == false)
	{
		Zero(target_file);
		return false;
	}

	if (DoesFileExist(target_file) == false)
	{
#if _DEBUG
		err_msg = __FUNCTION__;
		err_msg += "() line=";
		err_msg += (UINT)__LINE__;
		err_msg += " File does not exist: ";
		err_msg += target_file;
		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
		Zero(target_file);
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, target_file, DecodeText("A04D397EE2F6F7840684C8", s0)/*rb*/);

	Zero(s0);

	if (!stream)
	{
#if _DEBUG
		err_msg = __FUNCTION__;
		err_msg += "() line=";
		err_msg += (UINT)__LINE__;
		err_msg += " Unable to open file for reading: ";
		err_msg += target_file;
		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
		Zero(target_file);
		return false;
	}

	int len = _filelength(_fileno(stream));

	//	if (len == 0)
	//	{
	//#if _DEBUG
	//		err_msg = __FUNCTION__;
	//		err_msg += "() line=";
	//		err_msg += (UINT)__LINE__;
	//		err_msg += " File is empty: ";
	//		err_msg += target_file;
	//		MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
	//#endif
	//		Zero(target_file);
	//		return false;
	//	}

	string receive_buf_txt;
	if (len > 0)
	{
		receive_buf_txt.resize((size_t)len + 1);

		if (fread(&receive_buf_txt[0], 1, len, stream) != len)
		{
#if _DEBUG
			err_msg = __FUNCTION__;
			err_msg += "() line=";
			err_msg += (UINT)__LINE__;
			err_msg += " Problem reading file: ";
			err_msg += target_file;
			MessageBox(NULL, err_msg, "Error", MB_ICONERROR);
#endif
			Zero(target_file);
			return false;
		}
	}

	fclose(stream);

	hex_char_to_bin(receive_buf_txt, receive_buf);

	Zero(receive_buf_txt);

	//GetSystemTime(&st);
	//char ext[64];
	//sprintf_s(ext, sizeof(ext), ".%04u%02u%02u_%02u%02u%02u.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//STRING new_file;
	//new_file.LoadFileName(target_file, TRUE, TRUE, FALSE);
	//new_file += ext;

	//rename(target_file, new_file);

	DeleteFile(target_file);

	Zero(target_file);

	return true;
}

inline bool QueryServerOwnershipStatus(vector<uint8_t> &receive_buf)
{
	STRING file_name;
	GetConfigurationFileName(file_name);

	vector<uint8_t> send_buf;
	if (ReadFileToBuffer(file_name, send_buf) == false)
		return false;
	
	STRING err_msg;
	if (SendBufferToServer(s_e_url_ownership_status, send_buf, receive_buf, err_msg) == false)
		return false;

	return true;
}

