#pragma once 

class ExternalFunctionCallLogElement
{
public:

	string m_func_name;
	vector<string> m_params;
};

#if _DEBUG
extern int g_ext_function_call_log_idx;
extern vector<ExternalFunctionCallLogElement> g_ext_function_call_log;
#endif

inline void LOG_EXT_FUNCTION_CALL(const PtrSet<Value>* params, const char* func_name)
{
#if _DEBUG		
	if (g_ext_function_call_log.size() == 0)
		g_ext_function_call_log.resize(200);

	int idx = g_ext_function_call_log_idx;
	idx %= g_ext_function_call_log.size();

	g_ext_function_call_log_idx++;

	g_ext_function_call_log[idx].m_func_name = func_name;
	g_ext_function_call_log[idx].m_params.resize(params->GetCount());

	string tmp, tmp1;
	char buf[1024];
	for (int i = 0; i < params->GetCount(); i++)
	{
		const Value* v = params->GetObjectConst(i);
		sprintf_s(buf, sizeof(buf), "[%ld] %s %s", i, v->GetValueTypeName(tmp), v->GetValueAsString(tmp1));

		g_ext_function_call_log[idx].m_params[i] = buf;
	}
#endif
}

#define VERIFY_PARAM_COUNT(N) \
{\
	LOG_EXT_FUNCTION_CALL(params, __FUNCTION__);\
	ASSERT(params->GetCount() == N);\
}

#define VERIFY_PARAM_IS_INT(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_int)
#define VERIFY_PARAM_IS_UINT32(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_uint32)
#define VERIFY_PARAM_IS_UINT64(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_uint64)
#define VERIFY_PARAM_IS_DOUBLE(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_double)
#define VERIY_PARAM_SIZE(I, SZ) ASSERT(params->GetObjectConst(I)->m_array_sz == SZ)
#define VERIFY_PARAM_IS_BARRAY(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_barray)
#define VERIFY_PARAM_IS_STRING(I) ASSERT(params->GetObjectConst(I)->m_val_type == VT_string)

#define RETURN_ERROR_2(IPARAM, MSG1, MSG2)\
{\
	params->GetObjectA(IPARAM)->AppendString(MSG1);\
	params->GetObjectA(IPARAM)->AppendString(MSG2);\
	return false;\
}