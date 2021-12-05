#pragma once

#include "Windows.h"

#include <string>
#include <vector>
#include "inttypes.h"
#include <set>
#include "io.h"

#include "FloatToStringTools.h"
#include "Encryption.h"
#include "EBuffer.h"

#include "LSTTMPL.H"

using namespace std;

#define MAX_IMMEDIATE_STRING_LEN 2048

#ifdef _DEBUG
inline void BREAK_HERE(const char *msg)
{
	short break_here = true;
}

#define ERROR_MSG(MSG) \
{\
char msg[1024]; \
sprintf_s(msg, sizeof(msg), "%s(%ld) %s() : %s", __FILE__, __LINE__, __FUNCTION__, MSG); \
err_msg = msg; \
BREAK_HERE(msg); \
}
#else
#define ERROR_MSG(MSG)
#endif


namespace WorkbenchLib
{

	enum Operations
	{
		OP_BeginScope = 0,
		OP_EndScope = 1,
		OP_VariableAssignment = 2,
		OP_VariableDefinition = 3,
		OP_IfCondition = 4,
		OP_WhileCondition = 5,
		OP_Function = 6,
		OP_ExternalFunction = 7,
		OP_Return = 8,
		OP_Continue = 9,
		OP_Break = 10,
		OP_end = 11
	};

#if LOGGING
	inline const char *GetOpName(Operations op, string &tmp)
	{
		switch (op)
		{
		case OP_BeginScope: return "OP_BeginScope";
		case OP_EndScope: return "OP_EndScope";
		case OP_VariableAssignment: return "OP_VariableAssignment";
		case OP_VariableDefinition: return "OP_VariableDefinition";
		case OP_IfCondition: return "OP_IfCondition";
		case OP_WhileCondition: return "OP_WhileCondition";
		case OP_Function: return "OP_Function";
		case OP_ExternalFunction: return "OP_ExternalFunction";
		case OP_Return: return "OP_Return";
		case OP_Continue: return "OP_Continue";
		case OP_Break: return "OP_Break";
		}

		return "Undefined Operation"; 
	}
#endif

	enum LineTypes
	{
		LT_BeginScope = 0,
		LT_EndScope = 1,
		LT_VariableAssignment = 2,
		LT_VariableDefinition = 3,
		LT_IfCondition = 4,
		LT_WhileCondition = 5,
		LT_FunctionCall = 6,
		LT_NoOp = 7,
		LT_FunctionDefinition = 8,
		LT_Error = 9
	};

	enum ValTypes
	{
		VT_Undefined = -1,
		VT_string = 0,
		VT_int = 1,
		VT_double = 2,
		VT_barray = 3,
		VT_uint64 = 4,
		VT_uint32 = 5,
		VT_end = 6
	};

	enum MathOperators
	{
		MO_Undefined = -1,
		MO_Multiply = 0,				// *
		MO_Modulus = 1,					// %
		MO_Divide = 2,					// /
		MO_Add = 3,						// +
		MO_Subtract = 4,				// -
		MO_ShiftLeft = 5,				// <<
		MO_ShiftRight = 6,				// >>
		MO_BinaryAnd = 7,				// &
		MO_BinaryXor = 8,				// ^
		MO_BinaryOr = 9,				// |
		MO_BinaryOnesComplement = 10,	// ~
		MO_NoOp = 11,                   // Used for assignment with no operation
		MO_End = 12
	};

	enum LogicalOperators
	{
		LO_LessThan = 0, // <
		LO_GreaterThan = 1, // >
		LO_EqualTo = 2, // ==
		LO_NotEqualTo = 3, // !=
		LO_LessThanOrEqualTo = 4, // <=
		LO_GreaterThanOrEqualTo = 5, // >=
		LO_Undefined = 6,
	};


#define MATH_OPERATION(OP) \
switch (m_val_type)\
		{\
case VT_int:\
		{\
	int v = get_int(idx);\
	\
	switch (right.m_val_type)\
			{\
	case VT_int:\
		v OP right.get_int(idx_in);\
		break;\
	case VT_uint32:\
		v OP right.get_uint32(idx_in);\
		break;\
	case VT_uint64:\
		v OP right.get_uint64(idx_in);\
		break;\
	case VT_double:\
		v OP right.get_double(idx_in);\
		break;\
	default:\
	{\
    string tmp;\
	ERROR_MSG(DecodeText("DD418497225C2A1913F547E2D299E4934B8372E356E435F28AE296998A35",tmp)/*Invalid math operation*/); \
		return false; \
	}\
			}\
\
	return set_int(idx, v);\
		}\
case VT_uint32:\
		{\
	int v = get_uint32(idx);\
	\
	switch (right.m_val_type)\
			{\
	case VT_int:\
		v OP right.get_int(idx_in);\
		break;\
	case VT_uint32:\
		v OP right.get_uint32(idx_in);\
		break;\
	case VT_uint64:\
		v OP right.get_uint64(idx_in);\
		break;\
	case VT_double:\
		v OP right.get_double(idx_in);\
		break;\
	default:\
	{\
    string tmp;\
	ERROR_MSG(DecodeText("3768942F559E62199F1F47516E993E630C934750BDBFBAD1945DF606332F395D06BAD1",tmp)/*Invalid math operation*/); \
		return false; \
	}\
			}\
\
	return set_int(idx, v);\
		}\
\
case VT_uint64:\
		{\
	uint64_t v = get_uint64(idx);\
	\
	switch (right.m_val_type)\
			{\
	case VT_int:\
		v OP right.get_int(idx_in);\
		break;\
	case VT_uint32:\
		v OP right.get_uint32(idx_in);\
		break;\
	case VT_uint64:\
		v OP right.get_uint64(idx_in);\
		break;\
	case VT_double:\
		v OP right.get_double(idx_in);\
		break;\
	default:\
		ERROR_MSG("Invalid math operation");\
		return false;\
			}\
	\
	return set_uint64(idx, v);\
		}\
\
case VT_double:\
		{\
	double v = get_double(idx);\
	\
	switch (right.m_val_type)\
			{\
	case VT_int:\
		v OP right.get_int(idx_in);\
		break;\
	case VT_uint32:\
		v OP right.get_uint32(idx_in);\
		break;\
	case VT_uint64:\
		v OP right.get_uint64(idx_in);\
		break;\
	case VT_double:\
		v OP right.get_double(idx_in);\
		break;\
	default:\
	{\
		string tmp;\
		ERROR_MSG(DecodeText("07CE5996DF9B5DF2D24E0DDB5ECFCF93A0DE96469DDDBFA0BF79B3",tmp)/*Invalid math operation*/); \
		return false; \
	}\
			}\
	\
	return set_double(idx, v);\
		}\
\
default:\
	{\
	string tmp;\
	ERROR_MSG(DecodeText("4BAEC758BE275D91F96782BA1398C193B24EC5176941F76BC002B1",tmp)/*Invalid math operation*/);\
	return false;\
	}\
		}\

#define MATH_OPERATION_BINARY(OP) \
switch (m_val_type)\
		{\
	case VT_int:\
				{\
		int v = get_int(idx);\
		switch (right.m_val_type)\
						{\
		case VT_int:\
			v OP right.get_int(idx_in);\
			break;\
		case VT_uint32:\
			v OP right.get_uint32(idx_in);\
			break;\
		case VT_uint64:\
			v OP right.get_uint64(idx_in);\
			break;\
		default:\
		{\
			string tmp;\
			ERROR_MSG(DecodeText("BF5D8192566FB60F9A97835B4FFB3E0A411F96179C57F7F23A2FF33A346F",tmp)/*Invalid math operation*/); \
			return false; \
		}\
						}\
		\
		return set_int(idx, v);\
				}\
	\
	case VT_uint32:\
				{\
		uint32_t v = get_uint32(idx);\
		switch (right.m_val_type)\
						{\
		case VT_int:\
			v OP right.get_int(idx_in);\
			break;\
		case VT_uint32:\
			v OP right.get_uint32(idx_in);\
			break;\
		case VT_uint64:\
			v OP right.get_uint64(idx_in);\
			break;\
		default:\
		{\
			string tmp;\
			ERROR_MSG(DecodeText("66980797128B92D20E58A6512611764AA0F696278D9C97332F39D239D20C639771",tmp)/*Invalid math operation*/); \
			return false; \
		}\
						}\
		\
		return set_uint32(idx, v);\
				}\
	\
	case VT_uint64:\
			{\
		uint64_t v = get_uint64(idx);\
		switch (right.m_val_type)\
				{\
		case VT_int:\
			v OP right.get_int(idx_in);\
			break;\
		case VT_uint32:\
			v OP right.get_uint32(idx_in);\
			break;\
		case VT_uint64:\
			v OP right.get_uint64(idx_in);\
			break;\
		default:\
		{\
			string tmp;\
			ERROR_MSG(DecodeText("453B70969E1731138F2FA626755C86F14B6D17279D8CF7E239CF630C55",tmp)/*Invalid math operation*/); \
			return false; \
				}\
		}\
		\
		return set_uint64(idx, v);\
			}\
	\
default:\
{\
	string tmp;\
	ERROR_MSG(DecodeText("6DFA11F5124DF0B413F50AFC0FFB435C4167506F21BC247EF77E4ED206D224673341942467", tmp)/*Invalid math operation*/);\
	return false;\
}\
		}\


	// Value may be an immediate value or a variable
	class Value
	{
	public:

		inline Value(void)
		{
			m_data = 0;
		}

		inline Value(ValTypes vt)
		{
			m_data = 0;
			m_array_sz = 1;
			m_var_id = 0;
			m_val_type = vt;
		}

		inline Value(const Value &v)
		{
			m_data = 0;

			m_val_type = v.m_val_type;
			m_var_id = v.m_var_id;

			switch (m_val_type)
			{
			case VT_int: initialize_int(v.m_array_sz, v.get_data()); return;
			case VT_uint64: initialize_uint64(v.m_array_sz, v.get_data()); return;
			case VT_double: initialize_double(v.m_array_sz, v.get_data()); return;
			case VT_string: initialize_string(v.m_array_sz, v.get_data()); return;
			case VT_barray: initialize_barray(v.m_array_sz, v.get_data()); return;
			default: return;
			}

		}

		inline void initialize_data(void)
		{
			if (m_data)
				return;

			ASSERT(m_array_sz);

			switch (m_val_type)
			{
			case VT_int: initialize_int(m_array_sz); return;
			case VT_uint32: initialize_uint32(m_array_sz); return;
			case VT_uint64: initialize_uint64(m_array_sz); return;
			case VT_double: initialize_double(m_array_sz); return;
			case VT_string: initialize_string(m_array_sz); return;
			case VT_barray: initialize_barray(m_array_sz); return;
			default: ASSERT(false);
			}

		}

		inline ~Value(void)
		{
			clear();
		}

		inline bool operator < (const Value &obj) const
		{
			if (m_var_id < obj.m_var_id) return true;
			return false;
		}

		inline bool operator >(const Value &obj) const
		{
			if (m_var_id > obj.m_var_id) return true;
			return false;
		}

		inline void clear(void)
		{
			if (m_data == 0)
				return;

			switch (m_val_type)
			{
			case VT_Undefined:
				if (m_data) __debugbreak();
				return;

			case VT_double:
			{
				double *v = (double *)m_data;
				delete[] v;
				m_data = 0;
				return;
			}

			case VT_int:
			{
				int *v = (int *)m_data;
				delete[] v;
				m_data = 0;
				return;
			}

			case VT_uint32:
			{
				uint32_t *v = (uint32_t *)m_data;
				delete[] v;
				m_data = 0;
				return;
			}

			case VT_uint64:
			{
				uint64_t *v = (uint64_t *)m_data;
				delete[] v;
				m_data = 0;
				return;
			}

			case VT_string:
			{
				vector<string> *v = (vector<string>*) m_data;
				delete v;
				m_data = 0;
				return;
			}

			case VT_barray:
			{
				vector<vector<uint8_t>> *v = (vector<vector<uint8_t>>*) m_data;
				delete v;
				m_data = 0;
				return;
			}
			}

			__debugbreak();
		}

		inline bool IsImmediateValue(void) const
		{
			if (m_val_type == VT_Undefined)
				return false;

			if (m_var_id == 0)
				return true;

			return false;
		}

		inline int GetDataSizeBits(void) const
		{
			if (m_var_id) return 0;
			if (m_val_type == VT_Undefined) return 0;
			if (m_val_type >= VT_end) __debugbreak();
			if (m_data == 0) return 0;

			if (m_array_sz > 1) __debugbreak(); // not supported yet

			int len;
			switch (m_val_type)
			{
			case VT_string:
			{
				const vector<string> *v = (const vector<string> *)m_data;
				len = (*v)[0].length();
				len++;
				return len * 8;
			}

			case VT_barray:
			{
				const vector<vector<uint8_t>> *v = (const vector<vector<uint8_t>> *)m_data;
				len = (*v)[0].size();
				return len * 8;
			}

			case VT_int:
				return sizeof(int) * 8;

			case VT_uint32:
				return sizeof(uint32_t) * 8;

			case VT_uint64:
				return sizeof(uint64_t) * 8;

			case VT_double:
				return sizeof(double) * 8;
			}

			__debugbreak();
			return 0;
		}

		inline const uint8_t *GetData(void) const
		{
			switch (m_val_type)
			{
			case VT_int:
			case VT_uint32:
			case VT_double:
			case VT_uint64:
				return (const uint8_t *)m_data;

			case VT_string:
			{
				return (const uint8_t *)m_data;
				//const vector<string> *v = (const vector<string> *)m_data;
				//return (const uint8_t *)(*v)[0].c_str();
			}

			case VT_barray:
			{
				return (const uint8_t *)m_data;
				//const vector<vector<uint8_t>> *v = (const vector<vector<uint8_t>> *)m_data;
				//return &(*v)[0][0];
			}
			}

			ASSERT(FALSE);
			return 0;
		}

		inline uint32_t GetArraySize(void) const
		{
			return m_array_sz;
		}

		// assign the value while retaining the existing value type
		// This supports conversion from numeric types to string and string to numeric types
		inline bool AssignValue(const Value &vin, string *err_msg = 0, bool force_assignment = false, int idx = 0/*array index for this variable*/, int idx_in = 0/*array_index for the vin variable*/)
		{
			if (force_assignment == false && vin.IsImmediateValue() == false)
			{
				if (err_msg) DecodeText("2893CA04929CAE3F3DE79BB5FA6510220487DAFBD65FB375A1871AD2FD5DB42DC76A2E9726CB730D83B49056B45AF95EF11531582C3E605B065BA671585B", *err_msg)/*Value::AssignValue() passed in value must be immediate.*/;
				return false;
			}

			ASSERT(idx >= 0 && idx < GetArraySize());
			ASSERT(idx_in >= 0 && idx_in < vin.GetArraySize());

			switch (m_val_type)
			{
			case VT_string:
				if (vin.m_val_type == VT_string) // current value is string, assigned value is string
					return set_string(idx, vin.get_string(idx_in).c_str());

				if (vin.m_val_type == VT_int)
				{
					char snum[128];
					if (sprintf_s(snum, sizeof(snum), "%ld", vin.get_int(idx_in)) < 0)
					{
						ASSERT(FALSE); 
						return false;
					}

					return set_string(idx, snum);
				}

				if (vin.m_val_type == VT_uint32)
				{
					char snum[128];
					if (sprintf_s(snum, sizeof(snum), "%lu", vin.get_uint32(idx_in)) < 0)
					{
						ASSERT(FALSE); 
						return false;
					}

					return set_string(idx, snum);
				}

				if (vin.m_val_type == VT_uint64)
				{
					char snum[128];
					if (sprintf_s(snum, sizeof(snum), "%llu", vin.get_uint64(idx_in)) < 0)
					{
						ASSERT(FALSE); 
						return false;
					}

					return set_string(idx, snum);
				}

				if (vin.m_val_type == VT_double)
				{
					char snum[128];

					if (DoubleToString(vin.get_double(idx_in), snum, sizeof(snum) - 1) == false)
					{
						ASSERT(FALSE);
						string tmp;
						if (err_msg) *err_msg = DecodeText("84E89B4C87C8E83F4A3D9F19FA77F543E1EC5ED30619B90FFC4F83955F5C0F82563581BDFA4592A086199656E19412B713E05CB4A10A804699EECCF6E2D2210633397EF694212FF633897E33FBF6394E2FFB942FFBF7FB152189", tmp)/*Value::AssignValue () - unable to convert double to string.*/;
							
						return false;
					}

					return set_string(idx, snum);
				}

				if (vin.m_val_type == VT_barray)
				{
					ASSERT(FALSE);
					string tmp;
					if (err_msg) *err_msg = DecodeText("289336DC92095FA38116173DC7A0C82283630F2EDFF2EDD283349738C53F45960A264C69414597843A0FDF3BDB28C856263A40D73D72178B6891BA4E945AA95AF733D2F7C0942FA97E943339C0F7A9C0655AA9", tmp)/*Value::AssignValue () - unable to convert varray to string.*/;
					return false;
				}

				break;

			case VT_barray:

				if (vin.m_val_type == VT_barray)
					return set_barray(idx, vin.get_barray(idx_in));

				ASSERT(FALSE);
				{
					string tmp;
					if (err_msg) *err_msg = DecodeText("A0329A5C561569B6A1E75090686FB312461B097B100FDAC246FA52FDBE93C272992691F098704AED4DBF08FE5BA08337D24D69B4A186ED50BC61BA06537E9FF60629D2297E2F539F2906", tmp)/*Value::AssignValue () - unable to convert varray to string.*/;
				}
				return false;

			case VT_int:

				if (vin.m_val_type == VT_int)
					return set_int(idx, vin.get_int(idx_in));

				if (vin.m_val_type == VT_double)
					return set_int(idx, (int)vin.get_double(idx_in));

				if (vin.m_val_type == VT_uint64)
					return set_int(idx, (int)vin.get_uint64(idx_in));

				if (vin.m_val_type == VT_uint32)
					return set_int(idx, (int)vin.get_uint32(idx_in));

				if (vin.m_val_type == VT_string)
				{
					int data;
					if (sscanf_s(vin.get_string(idx_in).c_str(), "%ld", &data) != 1)
					{
						string tmp;
						if (err_msg) *err_msg = DecodeText("B56DBE3B63CC747881626F196865839E965D5E1038D7532C764A98F6607455315A8DFA951B84729F6229130D86748B57842C8356F25A8CE016F77E337E4EF6E24E390C7E3339F2F7E3F6060CF2944E399411E3380C", tmp)/*Value::AssignValue () - problem converting string to int.*/;
						ASSERT(FALSE);
						return false;
					}

					return set_int(idx, data);
				}

				if (vin.m_val_type == VT_barray)
				{
					string tmp;
					if (err_msg) *err_msg = DecodeText("D91F9BFA934492CE81409B48C62893ECFAE56EDACB192B0F65E1071EE0D20AEF10F9E5E6FBA0644E745DC9ABD6F4110F6F5D0A60E2807EF2F77EF269A8067EF28FD22F3369A88F80", tmp)/*Value::AssignValue () - can't convert barray to int.*/;
					ASSERT(FALSE);
					return false;
				}

				break;

			case VT_uint32:

				if (vin.m_val_type == VT_int)
					return set_uint32(idx, (uint32_t)vin.get_int(idx_in));

				if (vin.m_val_type == VT_uint32)
					return set_uint32(idx, (uint32_t)vin.get_uint32(idx_in));

				if (vin.m_val_type == VT_double)
					return set_int(idx, (uint32_t)vin.get_double(idx_in));

				if (vin.m_val_type == VT_uint64)
					return set_int(idx, (uint32_t)vin.get_uint64(idx_in));

				if (vin.m_val_type == VT_string)
				{
					int data;
					if (sscanf_s(vin.get_string(idx_in).c_str(), "%lu", &data) != 1)
					{
						string tmp;
						if (err_msg) *err_msg = DecodeText("651F6AE1877BAE47400D932F101E58430BB76E1B576105D6760C9D2A2DF1425E995C4137635DE05A9929D7915B0C272690683A98D20BBB98A67DA617D206943994063906FA33F6067EF606F27EE24E39FA484128", tmp)/*Value::AssignValue () - problem converting string to uint32.*/;
						ASSERT(FALSE);
						return false;
					}

					return set_int(idx, data);
				}

				if (vin.m_val_type == VT_barray)
				{
					string tmp;
					if (err_msg) *err_msg = DecodeText("6F59E40B974409A340A1FD7920A12F54F2633E56DB5EDF131D671069726EB15C8C682BA04CC25A96A0744E4AC247E819466FFA0A0A0A288806F62FF73387F7944EF71E887EF2F71EF28894F23838871E88", tmp)/*Value::AssignValue () - can't convert barray to uint32.*/;
					return false;
				}

				break;

			case VT_uint64:

				if (vin.m_val_type == VT_int)
					return set_uint64(idx, (uint64_t)vin.get_int(idx_in));

				if (vin.m_val_type == VT_uint32)
					return set_uint64(idx, (uint64_t)vin.get_uint32(idx_in));

				if (vin.m_val_type == VT_double)
					return set_uint64(idx, (uint64_t)vin.get_double(idx_in));

				if (vin.m_val_type == VT_uint64)
					return set_uint64(idx, (uint64_t)vin.get_uint64(idx_in));


				if (vin.m_val_type == VT_string)
				{
					uint64_t data;
					string tmp;
					if (sscanf_s(vin.get_string(idx_in).c_str(), DecodeText("235F5583F77EE74FD645", tmp)/*%llu*/, &data) != 1)
					{
						if (err_msg) *err_msg = DecodeText("50BB34A787CD09D5844046903628529E2D2B1956711375D74985DE4D22B713D28137BC80F1A02CB34119F22B3AE6179C19F22C9878AFFD4172197A9F5839DAF67EF2F7B1F7582F7E4E06584E332FF706F6D22F06F7B14E2FB15858DA", tmp)/*Value::AssignValue () - problem converting string to uint64.*/;
						ASSERT(FALSE);
						return false;
					}

					return set_uint64(idx, data);
				}

				if (vin.m_val_type == VT_barray)
				{
					string tmp;
					if (err_msg) *err_msg = DecodeText("46968E821BD544D540A1FD959846B2434697232EC6F363D2601FFA14A626F75C8CA48A0C64194D9771A16756234D5C6E97277E77C01BC194F206394E0639D22F10F710EF7E10EF10334EF23903D310EF", tmp)/*Value::AssignValue () - can't convert barray to uint64.*/;
					ASSERT(FALSE);
					return false;
				}

				break;

			case VT_double:

				if (vin.m_val_type == VT_int)
					return set_double(idx, (double)vin.get_int(idx_in));

				if (vin.m_val_type == VT_uint32)
					return set_double(idx, (double)vin.get_uint32(idx_in));

				if (vin.m_val_type == VT_double)
					return set_double(idx, (double)vin.get_double(idx_in));

				if (vin.m_val_type == VT_uint64)
					return set_double(idx, (double)vin.get_uint64(idx_in));

				if (vin.m_val_type == VT_string)
				{
					double data;
					string tmp;
					if (sscanf_s(vin.get_string(idx_in).c_str(), DecodeText("F7F891E2254E949FAF2548", tmp)/*%lf*/, &data) != 1)
					{
						if (err_msg) *err_msg = DecodeText("651F9AE12015C8933DE353850C134E0E734A1966DEB4025E4A37FF958E4A9AB4915C20E65F9083BB98225E4AA684F957540F0AFF1AF48D30641297BE453AD29462903A45", tmp)/*Value::AssignValue () - problem converting string to double.*/;
						ASSERT(FALSE);
						return false;
					}

					return set_double(idx, data);
				}

				if (vin.m_val_type == VT_barray)
				{
					string tmp;
					if (err_msg) *err_msg = DecodeText("A04EF8971BD6CCADE74050549C86678E7389C2F806191ED791588C144D5E4CFB9C819FA03AC290968880B39B26A699F26221832CF84A28F7FAF7F2D0F7D2ECD2ECD294D07E39F6F2FAD2ECD0", tmp)/*Value::AssignValue () - can't convert barray to double.*/;
					ASSERT(FALSE);
					return false;
				}

				break;
			}

			string tmp;
			if (err_msg) *err_msg = DecodeText("EA675F4C2B163C6E7587458557EA679E97929CDE06D4DA1946FA406492CC11743A741A19525A8050BD9817067EF4947ED2942F7EF2F43339112CF4BC", tmp)/*Value::AssignValue () - unsupported action.*/;
			return false;
		}

		// Make this value array have an exact copy of the data of the passed in array
		// Types must match exactly
		inline bool AssignArray(const Value &vin)
		{
			if (m_val_type != vin.m_val_type)
			{
				ASSERT(FALSE);
				return false; // types must match
			}

			clear();

			switch (m_val_type)
			{
			case VT_int: return initialize_int(vin.GetArraySize(), vin.get_data());
			case VT_uint32: return initialize_uint32(vin.GetArraySize(), vin.GetData());
			case VT_uint64: return initialize_uint64(vin.GetArraySize(), vin.GetData());
			case VT_double: return initialize_double(vin.GetArraySize(), vin.GetData());
			case VT_string: return initialize_string(vin.GetArraySize(), vin.GetData());
			case VT_barray: return initialize_barray(vin.GetArraySize(), vin.GetData());
			}

			ASSERT(FALSE);
			return false;
		}

		// Perform math operation without changing the result type
		inline bool PerformMathOp(const MathOperators &op, const Value &right, string &err_msg, bool b_use_non_immediate_value = false, int array_idx_this = 0, int array_idx_right = 0)
		{
			if (right.m_array_sz > 1) __debugbreak();

			if (b_use_non_immediate_value == false)
			{
				if (right.IsImmediateValue() == false)
				{
					DecodeText("6583BE73D8AE03564A41B4C00C6BF0B35B0976A01B4819A2975C0DF157F046C1D7D9B1225A1B0F9B87A38313DF9741D40CF9565D8B52825617D2AFBDAFD061BD", err_msg)/*Value::PerformMathOp() passed in value must be immediate.*/;
					return false;
				}
			}

			const int idx = array_idx_this;
			const int idx_in = array_idx_right;

			switch (op)
			{
			case MO_NoOp: return AssignValue(right, &err_msg, true, idx, idx_in);

			case MO_Divide: MATH_OPERATION(/= );
			case MO_Multiply: MATH_OPERATION(*= );
			case MO_Add: MATH_OPERATION(+= );
			case MO_Subtract: MATH_OPERATION(-= );
			case MO_Modulus: MATH_OPERATION_BINARY(%= );
			case MO_ShiftRight: MATH_OPERATION_BINARY(>>= );
			case MO_ShiftLeft: MATH_OPERATION_BINARY(<<= );
			case MO_BinaryAnd: MATH_OPERATION_BINARY(&= );
			case MO_BinaryXor: MATH_OPERATION_BINARY(^= );
			case MO_BinaryOr: MATH_OPERATION_BINARY(|= );

			default:
			{
				string tmp;
				ERROR_MSG(DecodeText("4F41A48322793A199F2F193613FB824A3C96E09E3110F61039F710942B105E6D2B", tmp)/*Invalid math operation*/);
				return false;
			}
			}

			__debugbreak();
			return false;
		}

		// Perform math operation without changing the result type
		inline bool ValidateMathOp(const MathOperators &op, const Value &right, ValTypes &val_type, string &err_msg)
		{
			switch (op)
			{
			case MO_Divide:

				if (right.IsImmediateValue() && right.IsZero())
				{
					DecodeText("63805D9D16555EB65AA48B5793B46440F215B7740BB26439F2064E7EF2649EF76F6F649C9E", err_msg)/*error: divide by zero.*/;
					return false;
				}

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;

					case VT_uint32:
						val_type = VT_uint32;
						return true;

					case VT_uint64:
						val_type = VT_uint64;
						return true;

					case VT_double:
						val_type = VT_double;
						return true;

					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("BF108F96127ADD33134E828A0F99929C84670AFD96FAF6B9D213F9D2C213F9B9", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;

					case VT_uint32:
						val_type = VT_uint32;
						return true;

					case VT_uint64:
						val_type = VT_uint64;
						return true;

					case VT_double:
						val_type = VT_double;
						return true;

					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("BEA847B3C69B86D6AB83869F5EFB761B84A745AFC510E24E9439F67EC6AAC67EF2C631C6AA", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint64;
						return true;

					case VT_uint32:
						val_type = VT_uint64;
						return true;

					case VT_uint64:
						val_type = VT_uint64;
						return true;

					case VT_double:
						val_type = VT_double;
						return true;

					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("3768389355FD4BAE23832B51D2305F5B41675B9E5C102F8E5FD63E", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_double:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_double;
						return true;

					case VT_uint32:
						val_type = VT_double;
						return true;

					case VT_uint64:
						val_type = VT_double;
						return true;

					case VT_double:
						val_type = VT_double;
						return true;

					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("F12A21673BBB92198FB35BBA5EBD0E56804E8327EEFAE206E24E394E44D2F68157B844", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("70BC9467BE46CBF2F993CA51D640CE635D83CA1798FABEE2F77ED26FAFBE23", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_Multiply:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("37BCC967F89F1AD6CADED39A6E8AD7DA85524D8C118CF7A7BF2F06BFE57EA7BFE5F6", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("B98C4593F82733D0F9360A8AD0113E4A2552239E9D3906F7E006F6F0E2D2E07357F0", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint64;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("D568A44E9A9B86D29B67B3FC6E99A22B745286BB1199337ED29439E23999C79CE999", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_double:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_double;
						return true;
					case VT_uint64:
						val_type = VT_double;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("916F45B99AB193ACA8835BADF2560EA0E6A4E09EB7FAF78CF78CC5D801", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("66FA94675F4F966E42527C96D7495F973CF5478B694139E9D29406F208FEE926", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_Add:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("078C38839A5AF0D28F2F8326F299765684672A509898A739A74E941E4A5EA7", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("DD4136F512274B13BF217247D721E44A324E83D19599534E9439F63394FA4039D240FA53B8", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint64;
						return true;
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("8357E296A6FD2A19A7898316D70D92634B52BE46A5867E062F4ECA0662CAE86E", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_double:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_double;
						return true;
					case VT_uint64:
						val_type = VT_double;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("BF7C389622465DB49A9365FC26D00E7474F5A55A40BCF6F212F73912F6062F11121112D1BF", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("AB99412F9A4693F25483155F41E8865C13833A5EE85DF28739D233E233947EF68287826137", tmp)/*Invalid math operation*/);
					return false;
				}
				}

			case MO_Subtract:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("37984FF5206162C423697C242640764A741F83BB99667EF2F7F294F2F74C08E408", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("379CA47E558B96C2234E7216D248825C851F83D298103927BB2FF7F64E7E27BB1F", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("7041452F9A4644199A4183472640BC00462FE049989C6534F22F346520CC", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_double:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_double;
						return true;
					case VT_uint64:
						val_type = VT_double;
						return true;
					case VT_double:
						val_type = VT_double;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("66294F708EFD866D9F966516316937493C585B9FFB5D1133112C33392C1139F7112C474D", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("DD9C4B679A9B5D6E8F010ABA2698A293AEB3729F0F8CF77E3D7ED27E6A2FD2606ADC3D", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_Modulus:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("ABB1A4BEB85093D69AF5965BB42192971883A69FBDB194F60639E24E94336ADFD484", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("669881B39E6F92B423B3E03E63983E9C90960A9B8D97335643F94E", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("B6108083689BF013F996A69726FB499A3C4EA66F5C57D2F6D294F639044E04A7ADFF04", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("079984989B8B9201BD524D20D998A087414E3A17279CD2D048D2F74821F9D0", tmp)/*Invalid math operation*/);
					return false;
				}
				}

			case MO_ShiftRight:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("FF5D3897886F4BB48FF590BAD7114A56A05B4DBBD98A4EF633D22FF6393AF6F7393AF6E4B6", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("90244F215FBB576E9F215B8AD62F926341585B5A25994AD97EF733D24E774AD940", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint64;
						return true;
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("37414558A79BD7131393C78AD68D76568552A6FDDC41D2067EF27EB37E39CBE5ACB3", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("379CDA95439C62F26B93809FD521435CA052A627FB57062FAC06F4F5ACF4ED", tmp)/*Invalid math operation*/);
					return false;
				}
				}

			case MO_ShiftLeft:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("4A5D84586AB24BD6A22F139EFB088663BD9613A640987E0367CFDB", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("229CE22F4327F0D29A93F58AD7395F333C37A65A1141F2817EF6064EE2BE6B8156", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("37988458885057B49F67478CF29D929341724D8B1198134E06F64E13F8F2E2F213F8E45B", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("D68C806AD950925E4214EB16B4117BB73C52861C115DB5F70B2FF7B5757EF239B5759F0B", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_BinaryAnd:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("0741841F6A9B866E429D2D8A19114387E61F5B6FFB5D4E944EE27E5936FC89", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("DDA7A4975FA0926EF952BD0DE9118287171F8227FB32C9E239FE5CFED27EFE655CC9", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("DD1B38969EBB93015E1F72BA5E5E3954851F86FD111BF77575312149", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("B5FA81835F6FB66E9FB34747D73F4391E71F5B398D922FF239F650FCF495", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_BinaryXor:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("3757A44E8E6F615EF997969F199DE41BA083049E8D2006334E2F4E7E39D24E394E08708605", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("EC5D709343CC1A1913870A8AC69D874374F596BB2168F294A5943326F2A526E64C", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("379C8158F8FF93FE8F97725B2611E497AC52E0276999394E96DEE87E062F96E8DE96", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("07FA8D4EBE9F620F8F2FE0BA6EFBBA1B80AB5B27FB99391D06CAFB", tmp)/*Invalid math operation*/);
					return false;
				}
				}


			case MO_BinaryOr:

				switch (m_val_type)
				{
				case VT_int:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_int;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("BB8780526A31F0D7F996E847C2118A5C851FA69E5CFAF758E92F9C58E9FA", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint32:
					switch (right.m_val_type)
					{
					case VT_int:
						val_type = VT_uint32;
						return true;
					case VT_uint32:
						val_type = VT_uint32;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("7054B6839A3C2AC2BF1F3A6C26BD4AB7E65B702740993AF23304C03A942F55C03A0455", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				case VT_uint64:
					switch (right.m_val_type)
					{
					case VT_int:
					case VT_uint32:
						val_type = VT_uint64;
						return true;
					case VT_uint64:
						val_type = VT_uint64;
						return true;
					default:
					{
						string tmp;
						ERROR_MSG(DecodeText("77BC80584327E4195E9772C10F985F2BDB4EDD27569806F2D2943E80A92F", tmp)/*Invalid math operation*/);
						return false;
					}
					}

				default:
				{
					string tmp;
					ERROR_MSG(DecodeText("BFFA80527C469613A2587247ED9892AA3C83B97969FA399433E2CDCF1610", tmp)/*Invalid math operation*/);
					return false;
				}
				}

			default:
			{
				string tmp;
				ERROR_MSG(DecodeText("70993F52F89FD6F242960A9AB4475FD73B1FA641694694B22FF777B22849", tmp)/*Invalid math operation*/);
				return false;
			}
			}

			__debugbreak();
			return false;
		}

		// Transfer the value verbatim
		inline Value &operator = (const Value &vin)
		{
			clear();

			m_val_type = vin.m_val_type;
			m_var_id = vin.m_var_id;
			m_array_sz = vin.m_array_sz;

			if (m_var_id == 0)
			{
				switch (m_val_type)
				{
				case VT_string: initialize_string(m_array_sz, vin.m_data); break;
				case VT_barray: initialize_barray(m_array_sz, vin.m_data); break;
				case VT_int: initialize_int(m_array_sz, vin.m_data); break;
				case VT_uint32: initialize_uint32(m_array_sz, vin.m_data); break;
				case VT_uint64: initialize_uint64(m_array_sz, vin.m_data); break;
				case VT_double: initialize_double(m_array_sz, vin.m_data); break;
				}
			}

			return *this;
		}

		inline bool IsZero(void) const
		{
			if (m_val_type == VT_Undefined) return true;
			if (m_var_id) return true;

			if (m_array_sz == 0) return true;

			switch (m_val_type)
			{
			case VT_int:
			{
				const int *v = (const int *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
					if (v[idx])
						return false;
				return true;
			}

			case VT_uint32:
			{
				const uint32_t *v = (const uint32_t *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
					if (v[idx])
						return false;
				return true;
			}

			case VT_uint64:
			{
				const uint64_t *v = (const uint64_t *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
					if (v[idx])
						return false;
				return true;
			}

			case VT_double:
			{
				const double *v = (const double *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
					if (v[idx])
						return false;
				return true;
			}

			case VT_string:
			{
				const vector<string> *v = (const vector<string> *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
					if ((*v)[idx].length())
						return false;
				return true;
			}

			case VT_barray:
			{
				const vector<vector<uint8_t>> *v = (const vector<vector<uint8_t>> *)m_data;
				for (uint32_t idx = 0; idx < m_array_sz; idx++)
				{
					if ((*v)[idx].size() == 0)
						continue;

					for (int i = 0; i < m_array_sz; i++)
					{
						if (((*v)[idx])[i])
							return false;
					}
				}
				return true;
			}
			}

			__debugbreak();

			return true;
		}

		ValTypes m_val_type = { VT_Undefined };
		uint64_t m_var_id = { 0 };
		uint32_t m_array_sz = { 1 };

		const ValTypes &GetValueType(void) const
		{
			return m_val_type;
		}

		const char *GetValueTypeName(string &s) const
		{
			switch (m_val_type)
			{
			case ValTypes::VT_barray: DecodeText("2C1F8541A84033C9E27EC98AEFC9D2", s)/*barray*/; return s.c_str();
			case ValTypes::VT_double: DecodeText("93663B5B12922709550527", s)/*double*/; return s.c_str();
			case ValTypes::VT_int:    DecodeText("50BCE0C0F68B39AE708BC0", s)/*int*/; return s.c_str();
			case ValTypes::VT_uint32: DecodeText("824E8C5BC40B45E23345274547C1", s)/*uint32*/; return s.c_str();
			case ValTypes::VT_string: DecodeText("1D6D9E66B9D3CCD27E2FA6CCE610", s)/*STRING*/; return s.c_str();
			case ValTypes::VT_uint64: DecodeText("2ABB5482300D33F233F75F743A88", s)/*uint64*/; return s.c_str();
			default:
				DecodeText("BFBC4FF58E27F08EAD9CD2ADAD9C618E", s)/*Invalid*/;
				return s.c_str();
			}
		}

		void AppendString(const string &s)
		{
			ASSERT(m_val_type == ValTypes::VT_string);

			if (m_data == 0)
				initialize_string();

			vector<string> *a = (vector<string> *)m_data;
			(*a)[0] += s.c_str();
		}

		const char *GetValueAsString(string &s, uint32_t idx = 0, int max_str_size=256) const
		{
			switch (m_val_type)
			{
			case ValTypes::VT_barray:
			{
				char hex[3];
				
				size_t sz = 0;
				if (m_data)
					sz = get_barray(idx).size();

				for (size_t i = 0; i < min(sz, max_str_size); i++)
				{
					// Output as hex
					uint8_t b = get_barray(idx)[i];
					byte_to_ascii_hex(b, hex);

//					if (i && (i % 2) == 0)
//						s += " ";
					s += hex;
				}

				return s.c_str();
			}

			case ValTypes::VT_double:
			{
				char tmp[64];
				DoubleToString(get_double(idx), tmp, sizeof(tmp) - 1);
				s = tmp;
				return s.c_str();
			}

			case ValTypes::VT_int:
			{
				char tmp[64];
				sprintf_s(tmp, sizeof(tmp), "%ld", get_int(idx));
				s = tmp;
				return s.c_str();
			}

			case ValTypes::VT_uint32:
			{
				char tmp[64];
				sprintf_s(tmp, sizeof(tmp), "%lu", get_uint32(idx));
				s = tmp;
				return s.c_str();
			}

			case ValTypes::VT_string:
			{
				if (m_data != 0)
					s = get_string(idx);

				return s.c_str();
			}

			case ValTypes::VT_uint64:
			{
				char tmp[128];
				sprintf_s(tmp, sizeof(tmp), "%llu", get_uint64(idx));
				s = tmp;
				return s.c_str();
			}

			default:
			{
				s = "";
				return s.c_str();
			}
			}
		}

		inline bool Convert(ValTypes &itype)
		{
			switch (itype)
			{
			case VT_int:
				switch (m_val_type)
				{
				case VT_int:
					return true;

				case VT_uint32:
				{
					m_val_type = VT_uint32;
					return true;
				}

				case VT_uint64: // currently uint64 converting to int
				{
					m_val_type = VT_int;
					uint64_t *vold = (uint64_t *)m_data;
					m_data = 0;
					int *vnew = initialize_int(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (int)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_double:  // currently double, converting to int
				{
					m_val_type = VT_int;
					double *vold = (double *)m_data;
					m_data = 0;
					int *vnew = initialize_int(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (int)vold[idx];
					delete[] vold;
					return true;
				}

				default:
					return false;
				}

			case VT_uint32:
				switch (m_val_type)
				{
				case VT_int:
				{
					m_val_type = VT_uint32;
					return true;
				}

				case VT_uint32:
					return true;

				case VT_uint64: // currently uint64 converting to int
				{
					m_val_type = VT_uint32;
					uint64_t *vold = (uint64_t *)m_data;
					m_data = 0;
					uint32_t *vnew = initialize_uint32(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (uint32_t)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_double:  // currently double, converting to int
				{
					m_val_type = VT_uint32;
					double *vold = (double *)m_data;
					m_data = 0;
					uint32_t *vnew = initialize_uint32(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (uint32_t)vold[idx];
					delete[] vold;
					return true;
				}

				default:
					return false;
				}



			case VT_uint64:
				switch (m_val_type)
				{
				case VT_int: // currently int convert to uint64
				{
					m_val_type = VT_uint64;
					int *vold = (int *)m_data;
					m_data = 0;
					uint64_t *vnew = initialize_uint64(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (uint64_t)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_uint32: // currently int convert to uint64
				{
					m_val_type = VT_uint64;
					uint32_t *vold = (uint32_t *)m_data;
					m_data = 0;
					uint64_t *vnew = initialize_uint64(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (uint64_t)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_uint64:
					return true;

				case VT_double: // current double convert to uint64
				{
					m_val_type = VT_uint64;
					double *vold = (double *)m_data;
					m_data = 0;
					uint64_t *vnew = initialize_uint64(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (uint64_t)vold[idx];
					delete[] vold;
					return true;
				}

				default:
					return false;
				}

			case VT_double:
				switch (m_val_type)
				{
				case VT_int: // currently int convert to double
				{
					m_val_type = VT_double;
					int *vold = (int *)m_data;
					m_data = 0;
					double *vnew = initialize_double(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (double)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_uint32: // currently int convert to double
				{
					m_val_type = VT_double;
					uint32_t *vold = (uint32_t *)m_data;
					m_data = 0;
					double *vnew = initialize_double(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (double)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_uint64: // currently uint64 convert to double
				{
					m_val_type = VT_double;
					uint64_t *vold = (uint64_t *)m_data;
					m_data = 0;
					double *vnew = initialize_double(m_array_sz);
					for (int idx = 0; idx < m_array_sz; idx++) vnew[idx] = (double)vold[idx];
					delete[] vold;
					return true;
				}

				case VT_double:
					return true;
				default:
					return false;
				}

			default:
				return false;
			}
		}

		inline double get_double(uint32_t idx) const
		{
			ASSERT(m_array_sz > idx);
			ASSERT(idx >= 0);

			if (m_data)
			{
				const double *v = (const double *)m_data;
				return v[idx];
			}

			return 0;
		}

		inline bool set_double(uint32_t idx, double data)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				m_data = new double[m_array_sz];
				memset(m_data, 0, sizeof(double)*m_array_sz);
			}

			double *v = (double *)m_data;
			v[idx] = data;
			return true;
		}

		inline int get_int(uint32_t idx) const
		{
			ASSERT(m_array_sz > idx);
			ASSERT(idx >= 0);

			if (m_data)
			{
				const int *v = (const int *)m_data;
				return v[idx];
			}

			return 0;
		}

		inline int get_uint32(uint32_t idx) const
		{
			ASSERT(m_array_sz > idx);
			ASSERT(idx >= 0);

			if (m_data)
			{
				const uint32_t *v = (const uint32_t *)m_data;
				return v[idx];
			}

			return 0;
		}

		inline bool set_int(uint32_t idx, int data)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				m_data = new int[m_array_sz];
				memset(m_data, 0, sizeof(int)*m_array_sz);
			}

			int *v = (int *)m_data;
			v[idx] = data;
			return true;
		}

		inline bool set_uint32(uint32_t idx, uint32_t data)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				m_data = new int[m_array_sz];
				memset(m_data, 0, sizeof(int)*m_array_sz);
			}

			uint32_t *v = (uint32_t *)m_data;
			v[idx] = data;
			return true;
		}

		inline uint64_t get_uint64(uint32_t idx) const
		{
			ASSERT(m_array_sz > idx);
			ASSERT(idx >= 0);

			if (m_data)
			{
				const uint64_t *v = (const uint64_t *)m_data;
				return v[idx];
			}

			return 0;
		}

		inline bool set_uint64(uint32_t idx, uint64_t data)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				m_data = new uint64_t[m_array_sz];
				memset(m_data, 0, sizeof(uint64_t)*m_array_sz);
			}

			uint64_t *v = (uint64_t *)m_data;
			v[idx] = data;
			return true;
		}

		inline const string &get_string(uint32_t idx) const
		{
			ASSERT(m_array_sz > idx);
			ASSERT(idx >= 0);

			const vector<string> *v = (const vector<string> *)m_data;
			return (*v)[idx];
		}

		// returns a pointer to the beginning of the string
		inline char *resize_string(uint32_t idx, int n)
		{
			vector<string> *v = (vector<string> *)m_data;
			if (v->size() < idx + 1)
				v->resize(idx + 1);

			(*v)[idx].resize(n);
			return &(*v)[idx][0];
		}

		inline bool set_string(int idx, const char *s)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				vector<string> *tmp = new vector < string >;
				tmp->resize(m_array_sz);
				m_data = tmp;
			}

			vector<string> *v = (vector<string> *)m_data;

			if (v->size() < idx + 1)
				v->resize(idx + 1);

			(*v)[idx] = s;
			return true;
		}

		inline const vector<uint8_t> &get_barray(int idx) const
		{
			const vector<vector<uint8_t>> *v = (const vector<vector<uint8_t>> *)m_data;
			return (*v)[idx];
		}

		inline vector<uint8_t> &get_barray_writable(int idx)
		{
			if (m_data == 0)
				initialize_barray();

			vector<vector<uint8_t>> *v = (vector<vector<uint8_t>> *)m_data;
			return (*v)[idx];
		}

		inline bool set_barray(int idx, const vector<uint8_t> &data)
		{
			if (m_data == 0)
			{
				ASSERT(m_array_sz);
				vector<vector<uint8_t>> *tmp = new vector<vector<uint8_t>>;
				tmp->resize(m_array_sz);
				m_data = tmp;
			}

			vector<vector<uint8_t>> *v = (vector<vector<uint8_t>> *)m_data;
			(*v)[idx] = data;
			return true;
		}

		inline double *initialize_double(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_double || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_double;
				m_array_sz = array_sz;
			}

			if (m_data == 0)
				m_data = new double[array_sz];

			if (data)
				memmove(m_data, data, sizeof(double)*array_sz);
			else
				memset(m_data, 0, sizeof(double)*array_sz);

			return (double *)m_data;
		}

		inline uint64_t *initialize_uint64(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_uint64 || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_uint64;
				m_array_sz = array_sz;
			}

			if (!m_data)
				m_data = new uint64_t[array_sz];

			if (data)
				memmove(m_data, data, sizeof(uint64_t)*array_sz);
			else
				memset(m_data, 0, sizeof(uint64_t)*array_sz);

			return (uint64_t *)m_data;
		}

		inline int *initialize_int(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_int || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_int;
				m_array_sz = array_sz;
			}

			if (m_data == 0)
				m_data = new int[array_sz];

			if (data)
				memmove(m_data, data, sizeof(int)*array_sz);
			else
				memset(m_data, 0, sizeof(int)*array_sz);

			return (int *)m_data;
		}

		inline uint32_t *initialize_uint32(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_uint32 || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_uint32;
				m_array_sz = array_sz;
			}

			if (m_data == 0)
				m_data = new int[array_sz];

			if (data)
				memmove(m_data, data, sizeof(uint32_t)*array_sz);
			else
				memset(m_data, 0, sizeof(uint32_t)*array_sz);

			return (uint32_t *)m_data;
		}

		inline vector<string> *initialize_string(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_string || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_string;
				m_array_sz = array_sz;
			}

			vector<string> *tmp = 0;
			if (m_data == 0)
			{
				tmp = new vector < string >;
				tmp->resize(array_sz);
			}

			if (data)
			{
				const vector<string> *vin = (const vector<string> *)data;
				*tmp = *vin;
				if (vin->size() != array_sz)
					tmp->resize(array_sz);
			}

			m_data = tmp;
			return tmp;
		}

		inline vector<vector<uint8_t>> *initialize_barray(uint32_t array_sz = 1, const void *data = 0)
		{
			if (m_val_type != VT_barray || m_array_sz != array_sz)
			{
				clear();

				m_val_type = VT_barray;
				m_array_sz = array_sz;
			}

			vector<vector<uint8_t>> *tmp = 0;

			if (m_data == 0)
			{
				tmp = new vector < vector<uint8_t> >;
				tmp->resize(array_sz);
			}
			else
			{
				tmp = (vector<vector<uint8_t>> *)m_data;
				ASSERT(tmp->size() == array_sz);
			}

			if (data)
			{
				const vector<vector<uint8_t>> *tmp_in = (const vector<vector<uint8_t>> *)data;
				ASSERT(tmp_in->size() == array_sz);
				*tmp = *tmp_in;
			}

			m_data = tmp;
			return tmp;
		}

		inline const void *get_data(void) const
		{
			return m_data;
		}

		inline void *get_data_writable(void)
		{
			if (m_data == 0)
				initialize_data();

			return m_data;
		}

	private:

		friend Value;

		// encapsulated data
		void *m_data = 0;

		//union
		//{
		//	double m_double;
		//	uint64_t m_uint64;
		//	int m_int;
		//	string *m_string;
		//	vector<uint8_t> *m_barray;
		//} m_data;
	};

#ifdef _DEBUG
	__forceinline bool ReportValues(const vector<Value *> &v, vector<string> &report)
	{
		for (int i = 0; i < v.size(); i++)
		{
			char s[1024];
			string tmp1, tmp2;
			sprintf_s(s, sizeof(s), "[%ld] %s = %s", i, v[i]->GetValueTypeName(tmp1), v[i]->GetValueAsString(tmp2));
			report.push_back(s);
		}

		return true;
	}
#endif




};