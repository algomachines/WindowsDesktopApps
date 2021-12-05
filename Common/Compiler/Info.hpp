#pragma once

#include "ParamTools.hpp"

static void(*info_callback)(const char *msg, const char *heading) = 0;

bool EF_string_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_STRING(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];
	for (uint32_t i = 0; i < array_sz; i++)
	{
		string s;
		val->GetValueAsString(s, i);

		if (s.length() > MAX_IMMEDIATE_STRING_LEN)
			s.erase(s.begin() + MAX_IMMEDIATE_STRING_LEN, s.end());

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}

bool EF_barray_save_to_file(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_STRING(0); // file
	VERIFY_PARAM_IS_STRING(1); // name
	VERIFY_PARAM_IS_BARRAY(2); // data

	const char *file_name = params->GetObjectConst(0)->get_string(0).c_str();
	const char *name = params->GetObjectConst(1)->get_string(0).c_str();

	FILE *stream = 0;
	fopen_s(&stream, file_name, "w");
	if (stream == 0)
	{
		char msg[1024];
		sprintf_s(msg, sizeof(msg), "%s() Can't create file: %s", __FUNCTION__, file_name);
		MessageBoxA(NULL, msg, "Error", MB_OK);
		return false;
	}

	const Value *data = params->GetObjectConst(2);

	uint32_t array_sz = data->GetArraySize();

	for (uint32_t i = 0; i < array_sz; i++)
	{
		string s;
		data->GetValueAsString(s, i, MAX_IMMEDIATE_STRING_LEN);

		if (s.length() > MAX_IMMEDIATE_STRING_LEN)
			s.erase(s.begin() + MAX_IMMEDIATE_STRING_LEN, s.end());

		fprintf_s(stream, "%s[%ld] = 0x%s", name, i, s.c_str());
	}

	fclose(stream);

	return true;
}

bool EF_barray_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_BARRAY(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];

	if (array_sz > 1)
	{
		sprintf_s(tmp, sizeof(tmp) - 1, "elements=%ld\n", array_sz);
		msg += tmp;
	}

	for (uint32_t i = 0; i < array_sz; i++)
	{
		string s;
		val->GetValueAsString(s, i);

		if (s.length() > 4096)
			s.erase(s.begin() + 4096, s.end());

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		sprintf_s(tmp, sizeof(tmp) - 1, "sz=%lld : ", val->get_barray(i).size());
		msg += tmp;

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}

bool EF_double_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_DOUBLE(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];

	if (array_sz > 1)
	{
		sprintf_s(tmp, sizeof(tmp) - 1, "elements=%ld\n", array_sz);
		msg += tmp;
	}

	for (uint32_t i = 0; i < array_sz; i++)
	{
		string s;
		val->GetValueAsString(s, i);

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}

bool EF_uint64_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_UINT64(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];

	if (array_sz > 1)
	{
		sprintf_s(tmp, sizeof(tmp) - 1, "elements=%ld\n", array_sz);
		msg += tmp;
	}

	for (uint32_t i = 0; i < array_sz; i++)
	{
		uint64_t v = val->get_uint64(i);

		char tmp[100];
		sprintf_s(tmp, sizeof(tmp), "0x%016llX", v);

		string s = tmp;

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}

bool EF_int_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_INT(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];

	if (array_sz > 1)
	{
		sprintf_s(tmp, sizeof(tmp) - 1, "elements=%ld\n", array_sz);
		msg += tmp;
	}

	for (uint32_t i = 0; i < array_sz; i++)
	{
		string s;
		val->GetValueAsString(s, i);

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}

bool EF_uint32_info(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_UINT32(1);

	const Value *val = params->GetObjectConst(1);

	uint32_t array_sz = val->GetArraySize();

	string msg;
	char tmp[32];

	if (array_sz > 1)
	{
		sprintf_s(tmp, sizeof(tmp) - 1, "elements=%ld\n", array_sz);
		msg += tmp;
	}

	for (uint32_t i = 0; i < array_sz; i++)
	{
		uint32_t v = val->get_uint32(i);
		char tmp[100];
		sprintf_s(tmp, sizeof(tmp), "0x%08lX", v);

		string s = tmp;

		if (array_sz > 1)
		{
			sprintf_s(tmp, sizeof(tmp) - 1, "%lu) ", i);
			s.insert(0, tmp);
		}

		msg += s;
		msg += "\n";

		if (i > 20)
		{
			msg += "...\n";
			break;
		}
	}

	if (info_callback)
		info_callback(msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str());
	else
		MessageBox(NULL, msg.c_str(), params->GetObjectConst(0)->get_string(0).c_str(), MB_OK);

	return true;
}