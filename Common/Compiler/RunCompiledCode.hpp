#pragma once
#include "CompiledCodeCommon.h"

namespace WorkbenchLib
{

// This class facilitates the running of one function's source code.
// This class is meant to be used in the debug proof implementation for release.
class RunCompiledCode
{
public:

	// Set of encryption buffer objects
	static set<EBuffer> m_buffer_set;

	// List of pointers to external functions
	static vector<bool(*)(PtrSet<Value> *)> m_ext_func;

	inline void SetExtFunc(bool(*func)(PtrSet<Value> *params))
	{
		m_ext_func.push_back(func);
	}

	inline RunCompiledCode(void)
	{
		m_cc = 0;
		m_bits = 0;
		m_e_bin = 0;
	}

	inline bool IsInitialized(void)
	{
		if (m_cc)
			return true;

		return false;
	}

	// cc - compiled code
	// nbits - size of compiled code in bits
	// seed - the encryption seed which will be used to create the EBuffer which will used to decrypt the code
	inline bool Initialize(const vector <uint8_t> *cc, uint64_t &nbits, const uint8_t *seed)
	{
		m_cc = cc;
		m_bits = nbits;

		EBuffer item;
		item.InitializeAsToken(seed);

		// Add a new EBuffer object to the set if necessary
		auto obj = m_buffer_set.emplace(item);
		if (obj.second == true) // item was added, need to initialize it
			obj.first->InitializeBuffer();

		m_e_bin = obj.first->GetEBin();

		return true;
	}

	// Don't expect is_array_element bit if array_idx < 0
	inline bool assign_value(Value &target_val, uint64_t &ibit_idx, int array_idx = 0)
	{
		int is_array_element = 0;

		if (RD_data(is_array_element, 1, ibit_idx) == false)
			return false;

		while (is_array_element) // array element being assigned
		{
			int is_immediate_value = 0;
			if (RD_data(is_immediate_value, 1, ibit_idx) == false)
				return false;

			Value array_idx_val(VT_int);
			if (is_immediate_value)
			{
				uint64_t array_idx_64;

				if (RD_compressed_integer(array_idx_64, ibit_idx) == false)
					return false;

				array_idx = (int)array_idx_64;

				break;
			}

			uint64_t array_element_var_id;
			if (RD_variable_id(array_element_var_id, ibit_idx) == false)
				return false;

			e_int iscope;
			int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, array_element_var_id, iscope);

			if (idx < 0)
				return false;

			array_idx = m_exe_values[iscope][idx].get_int(0);

			break;
		}

		MathOperators op;
		if (RD_math_operator(op, ibit_idx) == false)
			return false;

		Value val, *ret_val = 0;
		if (RD_value(val, m_exe_values, m_exe_values_idx, m_exe_N, ibit_idx, &ret_val) == false)
			return false;

#ifdef _DEBUG
		string check_ret_val, check_val;
		if (ret_val) ret_val->GetValueAsString(check_ret_val);
		val.GetValueAsString(check_val);
#endif

		if (ret_val == 0)
		{
			string err_msg;
			if (target_val.PerformMathOp(op, val, err_msg, false, array_idx) == false)
				return false;
		}
		else
		{
			string err_msg;
			if (target_val.PerformMathOp(op, *ret_val, err_msg, true, array_idx) == false)
				return false;

			if (ret_val->m_var_id == 0)
				delete ret_val; // delete temporary value
		}

#ifdef _DEBUG
		string check1;
		target_val.GetValueAsString(check1, array_idx);
#endif

		return true;
	}

	inline bool define_variable(uint64_t &ibit_idx)
	{
		ValTypes type;
		if (RD_variable_type(type, ibit_idx) == false)
			return false;

		uint64_t id, array_sz;
		if (RD_variable_id(id, ibit_idx) == false)
			return false;

		if (RD_array_sz(array_sz, ibit_idx) == false)
			return false;

		Value v;
		v.m_var_id = id;
		v.m_val_type = type;
		v.m_array_sz = array_sz;

		int iscope = exe_get_scope();

		if (append_value(m_exe_values[iscope], m_exe_values_idx[iscope], m_exe_N[iscope], v) == false)
			return false;

		return true;
	}

	inline bool do_logical_op_int(LogicalOperators lop, int left, int right)
	{
		switch (lop)
		{
		case LO_EqualTo: return left == right;
		case LO_GreaterThan: return left > right;
		case LO_GreaterThanOrEqualTo: return left >= right;
		case LO_LessThan: return left < right;
		case LO_LessThanOrEqualTo: return left <= right;
		case LO_NotEqualTo: return left != right;
		}

		ASSERT(false);
		return false;
	}

	inline bool do_logical_op_uint32(LogicalOperators lop, uint32_t left, uint32_t right)
	{
		switch (lop)
		{
		case LO_EqualTo: return left == right;
		case LO_GreaterThan: return left > right;
		case LO_GreaterThanOrEqualTo: return left >= right;
		case LO_LessThan: return left < right;
		case LO_LessThanOrEqualTo: return left <= right;
		case LO_NotEqualTo: return left != right;
		}

		ASSERT(false);
		return false;
	}

	inline bool do_logical_op_uint64(LogicalOperators lop, uint64_t left, uint64_t right)
	{
		switch (lop)
		{
		case LO_EqualTo: return left == right;
		case LO_GreaterThan: return left > right;
		case LO_GreaterThanOrEqualTo: return left >= right;
		case LO_LessThan: return left < right;
		case LO_LessThanOrEqualTo: return left <= right;
		case LO_NotEqualTo: return left != right;
		}

		ASSERT(false);
		return false;
	}

	inline bool do_logical_op_double(LogicalOperators lop, double left, double right)
	{
		switch (lop)
		{
		case LO_EqualTo: return left == right;
		case LO_GreaterThan: return left > right;
		case LO_GreaterThanOrEqualTo: return left >= right;
		case LO_LessThan: return left < right;
		case LO_LessThanOrEqualTo: return left <= right;
		case LO_NotEqualTo: return left != right;
		}

		ASSERT(false);
		return false;
	}

	inline bool perform_logical_operation(LogicalOperators lop, const Value *v_left, const Value *v_right, bool &result)
	{
		int array_idx_left = 0;
		int array_idx_right = 0;

		if (v_left == 0)
		{
			if (v_right->IsZero())
				result = false;
			else
				result = true;

			return true;
		}

		switch (v_left->m_val_type)
		{
		case VT_int:
		{
			int left = v_left->get_int(array_idx_left);

			switch (v_right->m_val_type)
			{
			case VT_int:
			{
				int right = v_right->get_int(array_idx_right);
				result = do_logical_op_int(lop, left, right);
				return true;
			}

			case VT_uint32:
			{
				uint32_t right = v_right->get_uint32(array_idx_right);
				result = do_logical_op_int(lop, left, (int)right);
				return true;
			}

			case VT_uint64:
			{
				uint64_t right = v_right->get_uint64(array_idx_right);
				result = do_logical_op_uint64(lop, (uint64_t)left, right);
				return true;
			}

			case VT_double:
			{
				double right = v_right->get_double(array_idx_right);
				result = do_logical_op_double(lop, (double)left, right);
				return true;
			}
			}
		}

		case VT_uint32:
		{
			uint32_t left = v_left->get_uint32(array_idx_left);

			switch (v_right->m_val_type)
			{
			case VT_int:
			{
				int right = v_right->get_int(array_idx_right);
				result = do_logical_op_int(lop, left, (uint32_t)right);
				return true;
			}

			case VT_uint32:
			{
				uint32_t right = v_right->get_uint32(array_idx_right);
				result = do_logical_op_int(lop, left, right);
				return true;
			}

			case VT_uint64:
			{
				uint64_t right = v_right->get_uint64(array_idx_right);
				result = do_logical_op_uint64(lop, (uint64_t)left, right);
				return true;
			}

			case VT_double:
			{
				double right = v_right->get_double(array_idx_right);
				result = do_logical_op_double(lop, (double)left, right);
				return true;
			}
			}
		}


		case VT_uint64:
		{
			uint64_t left = v_left->get_uint64(array_idx_left);

			switch (v_right->m_val_type)
			{
			case VT_int:
			{
				int right = v_right->get_int(array_idx_right);
				result = do_logical_op_uint64(lop, left, (uint64_t)right);
				return true;
			}

			case VT_uint32:
			{
				uint32_t right = v_right->get_uint32(array_idx_right);
				result = do_logical_op_int(lop, left, (uint64_t)right);
				return true;
			}

			case VT_uint64:
			{
				uint64_t right = v_right->get_uint64(array_idx_right);
				result = do_logical_op_uint64(lop, left, right);
				return true;
			}

			case VT_double:
			{
				double right = v_right->get_double(array_idx_right);
				result = do_logical_op_double(lop, (double)left, right);
				return true;
			}
			}
		}

		case VT_double:
		{
			double left = v_left->get_double(array_idx_left);

			switch (v_right->m_val_type)
			{
			case VT_int:
			{
				int right = v_right->get_int(array_idx_right);
				result = do_logical_op_double(lop, left, (double)right);
				return true;
			}

			case VT_uint32:
			{
				uint32_t right = v_right->get_uint32(array_idx_right);
				result = do_logical_op_int(lop, left, (double)right);
				return true;
			}

			case VT_uint64:
			{
				uint64_t right = v_right->get_uint64(array_idx_right);
				result = do_logical_op_double(lop, left, (double)right);
				return true;
			}

			case VT_double:
			{
				double right = v_right->get_double(array_idx_right);
				result = do_logical_op_double(lop, left, right);
				return true;
			}
			}
		}
		}

		ASSERT(false);
		return false;
	}

	inline bool Run(vector<Value *> &params, uint64_t starting_address = 0)
	{
		if (m_bits == 0)
			return false;

#ifdef LOGGING
		m_log.resize(0);
#endif


		if (starting_address >= m_bits)
			return false;

		uint64_t ibit_idx = starting_address;

		m_exe_values.resize(0);
		m_exe_values_idx.resize(0);
		m_exe_N.resize(0);

		while (ibit_idx < m_bits)
		{
			Operations op;
			if (RD_operations(ibit_idx, op) == false)
				return false;

			log_operation(ibit_idx - 4, op);

			switch (op)
			{
			case OP_BeginScope:
			{
				if (exe_begin_new_scope() == false)
					return false;

				break;
			}

			case OP_EndScope:
			{
				int is_loop = 0;
				RD_data(is_loop, 1, ibit_idx);

				if (is_loop)
				{
					int delta;
					if (RD_data(delta, 32, ibit_idx) == false)
						return false;

					ASSERT(delta < 0);

					ibit_idx += delta;

					ASSERT(ibit_idx > 0);
				}

				if (exe_get_scope() == 0)
				{
					// Same as return
					update_param_values(params);
				}

				if (exe_end_scope() == false)
					return false;

				break;
			}


			case OP_Return:
			{
				while (exe_get_scope())
				{
					if (exe_end_scope() == false)
						return false;
				}

				update_param_values(params);

				if (exe_end_scope() == false)
					return false;

				break;
			}

			case OP_VariableDefinition:
			{
				if (define_variable(ibit_idx) == false)
					return false;

				int iscope = exe_get_scope();

				if (iscope == 0 && m_exe_N[iscope] <= params.size())
				{
					int iparam = m_exe_N[iscope] - 1;

					// Don't need to use m_exe_values_idx because the first N parameters will be the parameter values.
					// This assumption depends on how code is generated when defining the function.
					Value *param = params[iparam];
					//if (param->m_val_type == VT_barray)
					//{
					//	const vector<uint8_t> *v = &param->get_barray(0);
					//	short break_here = true;
					//}
					m_exe_values[iscope][iparam].AssignArray(*param);
				}

				break;
			}

			case OP_VariableAssignment:
			{
				uint64_t id;

				uint32_t immediate_data;
				if (RD_data(immediate_data, 1, ibit_idx) == false)
					return false;

				ASSERT(immediate_data == 0);

				if (RD_variable_id(id, ibit_idx) == false)
					return false;

				e_int iscope;
				int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, id, iscope);

				if (idx < 0)
					return false;

				if (assign_value(m_exe_values[iscope][idx], ibit_idx) == false)
					return false;

				break;
			}

			case OP_Function:
			{
				uint64_t addr;
				if (RD_data(addr, 64, ibit_idx) == false)
					return false;

				uint64_t nparams;
				if (RD_compressed_integer(nparams, ibit_idx) == false)
					return false;

				vector<uint64_t> params_var_id;
				params_var_id.resize(nparams);

				for (int iparam = 0; iparam < nparams; iparam++)
				{
					if (RD_variable_id(params_var_id[iparam], ibit_idx) == false)
						return false;
				}

				int has_expected_value = 0;
				if (RD_data(has_expected_value, 1, ibit_idx) == false)
					return false;

				uint8_t seed[SEED_SZ_BYTES];

				if (has_expected_value)
				{
					uint64_t id;
					if (RD_variable_id(id, ibit_idx) == false)
						return false;

					e_int iscope;
					int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, id, iscope);

					//Value expected_value, *ret_val = 0;
					//if (RD_value(expected_value, m_exe_values, m_exe_values_idx, m_exe_N, ibit_idx, &ret_val) == false)
					//	return false;

					create_seed(m_exe_values[iscope][idx], seed);
				}
				else
				{
					ZeroMemory(seed, sizeof(seed));
				}

				vector<Value *> params;
				params.resize(nparams);

				// Initialize the vector of parameters
				for (int iparam = 0; iparam < nparams; iparam++)
				{
					e_int iscope;
					int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, params_var_id[iparam], iscope);

					//params[iparam].m_val_type = m_exe_values[iscope][idx].m_val_type;
					//if (params[iparam].AssignArray(m_exe_values[iscope][idx]) == false)
					//	return false;

					params[iparam] = &m_exe_values[iscope][idx];
				}

				RunCompiledCode obj;
				obj.Initialize(m_cc, m_bits, seed);

				obj.Run(params, addr);

				// Update variables with returned parameter values
				//for (int iparam = 0; iparam < nparams; iparam++)
				//{
				//	int iscope;
				//	int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, params_var_id[iparam], iscope);
				//	params[iparam].m_val_type = m_exe_values[iscope][idx].m_val_type;
				//	if (m_exe_values[iscope][idx].AssignArray(params[iparam]) == false)
				//		return false;
				//}

#ifdef _DEBUG
				vector<string> report;
				ReportValues(params, report);
#endif

				break;
			}

			case OP_ExternalFunction:
			{
				uint64_t func_idx;
				if (RD_compressed_integer(func_idx, ibit_idx) == false)
					return false;

				uint64_t nparams;
				if (RD_compressed_integer(nparams, ibit_idx) == false)
					return false;

				// Read the variable ids of the parameters
				vector<uint64_t> params_var_id;
				params_var_id.resize(nparams);
				for (int iparam = 0; iparam < nparams; iparam++)
				{
					if (RD_variable_id(params_var_id[iparam], ibit_idx) == false)
						return false;
				}

				PtrSet<Value> func_params;
				for (int iparam = 0; iparam < nparams; iparam++)
				{
					e_int iscope;
					int idx = find_value_with_id(m_exe_values, m_exe_values_idx, m_exe_N, params_var_id[iparam], iscope);


					Value *param = &m_exe_values[iscope][idx];

					func_params.Append(param);
				}

				// Run the function		
				bool(*func)(PtrSet<Value> *) = m_ext_func[func_idx];
				bool ret = func(&func_params);
				//bool ret = m_ext_func[func_idx](&func_params);

				//bool ret = func->m_func(&func_params);

				// So that the values don't get deleted
				func_params.ExtractAll();

				if (ret == false)
					return false;

				break;
			}

			case OP_WhileCondition:
			case OP_IfCondition:
			{
				LogicalOperators lo;
				if (RD_logical_operator(lo, ibit_idx) == false)
					return false;

				Value tmp[2], *v_left = 0, *v_right = 0;

				if (RD_value(tmp[0], m_exe_values, m_exe_values_idx, m_exe_N, ibit_idx, &v_left) == false)
				{
					ASSERT(FALSE);
					return false;
				}

				if (v_left == 0)
				{
					v_right = &tmp[0];
					v_right->m_var_id = 0xFFFFFFFF; // so that we won't delete this later
				}

				if (RD_value(tmp[1], m_exe_values, m_exe_values_idx, m_exe_N, ibit_idx, &v_right) == false)
				{
					ASSERT(FALSE);
					return false;
				}

				if (v_right == 0)
				{
					v_right = &tmp[1];
					v_right->m_var_id = 0xFFFFFFFF; // so that we won't delete this later
				}

				bool result;
				if (perform_logical_operation(lo, v_left, v_right, result) == false)
					return false;

				// Delete immediate values which were instantiated inside RD_value()
				if (v_left && v_left->m_var_id == 0) delete v_left;
				if (v_right->m_var_id == 0) delete v_right;

				// read the skip forward number
				int delta;
				if (RD_data(delta, 32, ibit_idx) == false)
					return false;

				if (result == false)
					ibit_idx += delta; // jump by the skip forward number of bits

				// otherwise carry on into the new scope

				break;
			}

			case OP_Continue:
			{
				uint64_t nunwind = 0;
				if (RD_compressed_integer(nunwind, ibit_idx) == false)
					return false;

				exe_end_scope(nunwind);

				int delta;
				if (RD_data(delta, 32, ibit_idx) == false)
					return false;

				ibit_idx += delta;

				continue;
			}

			case OP_Break:
			{
				uint64_t nunwind = 0;
				if (RD_compressed_integer(nunwind, ibit_idx) == false)
					return false;

				exe_end_scope(nunwind);

				int delta;
				if (RD_data(delta, 32, ibit_idx) == false)
					return false;

				ibit_idx += delta;

				continue;
			}

			default:

				return false;

			}

			if (exe_get_scope() < 0)
				break; // we are done
		}

		return true;
	}

	inline void SetEncryptionBuffer(const EBuffer &ebuf)
	{
		m_e_bin = ebuf.GetEBin();
	}

private:

	inline void log_operation(uint64_t ibit_idx, Operations op)
	{
#ifdef LOGGING
		char msg[1024];
		string tmp;
		sprintf_s(msg, "[%llu] %s", ibit_idx, GetOpName(op, tmp));
		m_log.push_back(msg);
#endif
	}

#ifdef LOGGING
	vector<string> m_log;
#endif

	const vector<uint8_t> *m_cc = { 0 };			// compiled code
	uint64_t m_bits = { 0 };						// size of compiled code in bits

	const vector<uint8_t> *m_e_bin = 0;

	char m_msg[1024];

	int m_LineTypes[8];
	int m_VariableTypes[4];

	vector<vector<Value>> m_exe_values;		// The variables for each scope
	vector<vector<int>> m_exe_values_idx;	// Sort index for variables within each scope - need this so that we can quickly find a value by its var_id
	vector<int> m_exe_N;					// Number of variables for each scope

	//vector<Value> m_exe_pre_assignment_values;

	inline void update_param_values(vector<Value *> &params)
	{
		if (exe_get_scope() != 0)
			__debugbreak();

		for (int iparam = 0; iparam < params.size(); iparam++)
			if (params[iparam]->AssignArray(m_exe_values[0][iparam]) == false)
				__debugbreak();
	}

	inline int find_value_insert_pos(const vector<Value> &values, const vector<int> values_idx, const int N, const Value &value)
	{
		if (N == 0)
			return 0;

		int istart = 1;
		int iend = N - 1;

		int n = N;

		if (value < values[values_idx[0]])
			return 0;

		if (value > values[values_idx[n - 1]])
			return n;

		while (iend - istart > 1)
		{
			int i = (istart + iend) / 2;

			if (value < values[values_idx[i]])
				iend = i;
			else if (value > values[values_idx[i]])
				istart = i;
			else // v == vidx[i]
				return i + 1;
		}

		if (value < values[values_idx[istart]])
			return istart;

		if (value < values[values_idx[iend]])
			return iend;

		return iend + 1;
	}

	inline int find_value_with_id(const vector<vector<Value>> &values, const vector<vector<int>> &values_idx, const vector<int> &N, const uint64_t &id, e_int &iscope)
	{
		iscope = exe_get_scope();

		while (iscope >= 0)
		{
			Value token;
			token.m_var_id = id;

			int ipos = find_value_insert_pos(values[iscope], values_idx[iscope], N[iscope], token);
			ipos--;

			if (ipos < 0)
			{
				iscope = iscope - 1;
				continue;
			}

			if (ipos >= values[iscope].size())
			{
				iscope = iscope - 1;
				continue;
			}

			if (values[iscope][values_idx[iscope][ipos]].m_var_id == id)
				return ipos;

			iscope = iscope - 1;
		}

		ASSERT(false);
		return -1;
	}

	inline bool append_value(vector<Value> &values, vector<int> &values_idx, int &N, const Value &value)
	{
		int idx = find_value_insert_pos(values, values_idx, N, value);

		if (values.size() == N)
			values.resize(3 * (N / 2));

		if (value.m_var_id)
		{
			values[N].m_var_id = value.m_var_id;
			values[N].m_val_type = value.m_val_type;
			values[N].m_array_sz = value.m_array_sz;

			// Process pre_assignments if applicable
			//if (m_exe_pre_assignment_values.size())
			//{
			//	for (int i = 0; i < m_exe_pre_assignment_values.size(); i++)
			//	{
			//		if (m_exe_pre_assignment_values[i].m_var_id == values[N].m_var_id)
			//		{
			//			if (values[N].m_val_type != m_exe_pre_assignment_values[i].m_val_type)
			//				__debugbreak();

			//			string err_msg;
			//			m_exe_pre_assignment_values[i].m_var_id = 0; // make an immediate value so the function call will not fail
			//			if (values[N].AssignValue(m_exe_pre_assignment_values[i], err_msg) == false)
			//				__debugbreak();

			//			m_exe_pre_assignment_values.erase(m_exe_pre_assignment_values.begin() + i);
			//			break;
			//		}
			//	}
			//}

		}
		else
		{
			if (values[N].AssignValue(value) == false)
				return false;
		}

		values_idx[N] = idx;

		for (int i = 0; i < N; i++)
		{
			if (values_idx[i] >= idx)
				values_idx[i]++;
		}

		N++;

		return true;
	}

	inline bool exe_begin_new_scope(void)
	{
		const int initial_values_sz = 100;

		int N = m_exe_values.size();
		m_exe_values.resize(N + 1);
		m_exe_values[N].resize(initial_values_sz);
		m_exe_values_idx.resize(N + 1);
		m_exe_values_idx[N].resize(initial_values_sz);
		m_exe_N.resize(N + 1);

		return true;
	}

	inline bool exe_end_scope(int n = 1)
	{
		int N = m_exe_values.size();
		if (N - n < 0)
			return false;

		m_exe_values.resize(N - n);
		m_exe_values_idx.resize(N - n);
		m_exe_N.resize(N - n);

		return true;
	}

	inline int exe_get_scope(void) const
	{
		return m_exe_N.size() - 1;
	}

	inline uint8_t get_byte(uint64_t idx)
	{
		if (m_e_bin == 0)
			return (*m_cc)[idx];

		return ((*m_cc)[idx] ^ (*m_e_bin)[idx % (*m_e_bin).size()]);
	}

	inline bool RD_data(uint8_t *data, int nbits, uint64_t &ibit_idx)
	{
		int nbytes = nbits / 8;
		if (nbits % 8)
			nbytes++;

		memset(data, 0, nbytes);

		for (int ibit = 0; ibit < nbits; ibit++)
		{
			uint64_t idx = ibit_idx / 8;
			int offset = ibit_idx % 8;

			uint8_t u8 = get_byte(idx);
			if (u8 & (0x01 << offset))
			{
				int ibyte = ibit / 8;

				data[ibyte] |= 0x01 << (ibit % 8);
			}

			ibit_idx++;
		}

		return true;
	}

	inline bool RD_operations(uint64_t &ibit, Operations &op)
	{
		if (ibit > m_bits - 4)
			return false;

		return RD_data((int &)op, 4, ibit);
	}

	inline bool RD_data(int &data, const int &nbits, uint64_t &ibit_idx)
	{
		data = 0;
		return RD_data((uint8_t *)&data, nbits, ibit_idx);
	}

	inline bool RD_data(uint64_t &data, const int &nbits, uint64_t &ibit_idx)
	{
		data = 0;
		return RD_data((uint8_t *)&data, nbits, ibit_idx);
	}

	inline bool RD_data(uint32_t &data, const int &nbits, uint64_t &ibit_idx)
	{
		data = 0;
		return RD_data((uint8_t *)&data, nbits, ibit_idx);
	}

	inline bool RD_variable_type(ValTypes &itype, uint64_t &ibit)
	{
		if (ibit >= m_bits - 3)
			return false;

		memset(&itype, 0, sizeof(itype));
		return RD_data((int &)itype, 3, ibit);
	}

	inline bool RD_logical_operator(LogicalOperators &lo, uint64_t &ibit)
	{
		if (ibit >= m_bits - 3)
			return false;

		memset(&lo, 0, sizeof(lo));
		return RD_data((int &)lo, 3, ibit);
	}

	inline bool RD_value_immediate_data(Value &val, uint64_t &ibit_idx)
	{
		switch (val.m_val_type)
		{
		case ValTypes::VT_int:
			return RD_data((uint8_t *)val.get_data_writable(), sizeof(int) * 8, ibit_idx);

		case ValTypes::VT_double:
			return RD_data((uint8_t *)val.get_data_writable(), sizeof(double) * 8, ibit_idx);

		case ValTypes::VT_uint64:
			return RD_data((uint8_t *)val.get_data_writable(), 64, ibit_idx);

		case ValTypes::VT_uint32:
			return RD_data((uint8_t *)val.get_data_writable(), 32, ibit_idx);

		case ValTypes::VT_string:
		{
			int n = 0;

			char s[MAX_IMMEDIATE_STRING_LEN];
			while (n < sizeof(s))
			{
				RD_data((uint8_t*)&s[n], 8, ibit_idx);
				if (s[n] == 0)
					break;

				n++;
			}

			if (n == sizeof(s))
			{
				s[n - 1] = 0;
				ASSERT(FALSE); // truncation
			}

			vector<string> tmp;
			tmp.resize(1);
			tmp[0] = s;
			val.initialize_string(1, (const uint8_t *)&tmp);

			ASSERT(n < sizeof(s));
			return true;
		}
		//case ValTypes::VT_string:
		//	return RD_data

		default:
			return false;
		}
	}

	inline bool RD_variable_id(uint64_t &id, uint64_t &ibit)
	{
		bool status = RD_compressed_integer(id, ibit);
		if (status == false)
			return false;

		return true;
	}

	inline bool RD_array_sz(uint64_t &array_sz, uint64_t &ibit)
	{
		uint64_t is_array = 0;
		bool status = RD_data(is_array, 1, ibit);
		if (status == false)
			return false;

		if (is_array == 0)
		{
			array_sz = 1;
			return true;
		}

		status = RD_compressed_integer(array_sz, ibit);
		if (status == false)
			return false;

		return true;
	}

	inline bool RD_compressed_integer(uint64_t &id, uint64_t &ibit, int *ret_sign = 0)
	{
		int sign;
		if (RD_data(sign, 1, ibit) == false)
			return false;

		if (ret_sign)
			*ret_sign = sign;

		if (ret_sign == 0 && sign == 0) ASSERT(FALSE);

		int db_code;

		if (RD_data(db_code, 3, ibit) == false)
			return false;

		int data_bits;
		switch (db_code)
		{
		case 0: data_bits = 8; break;
		case 1: data_bits = 12; break;
		case 2: data_bits = 16; break;
		case 3: data_bits = 24;  break;
		case 4: data_bits = 32; break;
		case 5: data_bits = 48; break;
		case 6: data_bits = 56; break;
		default: data_bits = 63; break;
		}

		if (RD_data(id, data_bits, ibit) == false)
			return false;

		return true;
	}

	inline bool RD_immediate_value(Value &val, uint64_t &ibit_idx)
	{
		ValTypes type;
		if (RD_variable_type(type, ibit_idx) == false)
			return false;

		val.m_val_type = type;
		if (RD_value_immediate_data(val, ibit_idx) == false)
			return false;

		return true;
	}

	// This function is only to be used in situations where a value (immediate or otherwise) is expected 
	inline bool RD_value(Value &val, vector<vector<Value>> &values, const vector<vector<int>> values_idx, const vector<int> N, uint64_t &ibit_idx, Value **ret_value = 0)
	{
		int is_immediate_val;
		if (RD_data(is_immediate_val, 1, ibit_idx) == false)
			return false;

		if (is_immediate_val)
			return RD_immediate_value(val, ibit_idx);

		// is_variable == true

		if (ret_value == 0)
			return false;

		uint64_t id;
		if (RD_variable_id(id, ibit_idx) == false)
			return false;

		e_int iscope;
		int idx = find_value_with_id(values, values_idx, N, id, iscope);

		int is_array_element;
		RD_data(is_array_element, 1, ibit_idx);

		if (is_array_element)
		{
			if (RD_data(is_immediate_val, 1, ibit_idx) == false)
			{
				ASSERT(FALSE); return false;
			}

			uint64_t element_u64;

			if (is_immediate_val)
			{
				int sign;
				if (RD_compressed_integer(element_u64, ibit_idx, &sign) == false)
				{
					ASSERT(FALSE);
					return false;
				}

				ASSERT(sign); // Must be a positive number
			}
			else
			{
				uint64_t id_array_element;
				RD_variable_id(id_array_element, ibit_idx);

				e_int iscope_array_element;
				int idx_array_element = find_value_with_id(values, values_idx, N, id_array_element, iscope_array_element);

				Value v_array_element;
				v_array_element.m_val_type = VT_uint64;
				if (v_array_element.AssignValue(values[iscope_array_element][values_idx[iscope_array_element][idx_array_element]], 0, true) == false)
				{
					ASSERT(FALSE);
					return false;
				}

				element_u64 = v_array_element.get_uint64(0);
			}

			int vin_idx = values_idx[iscope][idx];
			const Value *vin = &values[iscope][vin_idx];

			ASSERT(element_u64 < vin->GetArraySize()); // array element is out of range

			*ret_value = new Value;
			(*ret_value)->m_val_type = vin->m_val_type;
			(*ret_value)->AssignValue(*vin, 0, true, 0, element_u64);
			return true;
		}

		*ret_value = &values[iscope][values_idx[iscope][idx]];

		return true;
	}

	// returns a value object
	//inline Value *RD_value_ex(Value &val, vector<vector<Value>> &values, const vector<vector<int>> values_idx, const vector<int> N, uint64_t &ibit_idx)
	//{
	//	int is_variable;
	//	if (RD_data(is_variable, 1, ibit_idx) == false)
	//		return 0;

	//	if (is_variable == false)
	//	{
	//		if (RD_immediate_value(val, ibit_idx) == false)
	//			return 0;

	//		return &val;
	//	}

	//	// is_variable == true

	//	uint64_t id;
	//	if (RD_variable_id(id, ibit_idx) == false)
	//		return false;

	//	int iscope;
	//	int idx = find_value_with_id(values, values_idx, N, id, iscope);

	//	return &values[iscope][values_idx[iscope][idx]];
	//}

	inline bool RD_math_operator(MathOperators &mop, uint64_t &ibit_idx)
	{
		return RD_data((int &)mop, 4, ibit_idx);
	}

	inline void seed_hash_function(const void *data, int data_sz, const ValTypes &vt, uint8_t *hash, int hash_sz)
	{
		int niter = SEED_HASH_NITER;
		CreateRandomBufferFromSeed((const uint8_t *)data, data_sz, vt, hash, hash_sz, niter);
	}

	inline bool create_seed(const Value expected_value, uint8_t *seed)
	{
		ZeroMemory(seed, SEED_SZ_BYTES);

		// No expected value, the seed is zero
		if (expected_value.m_val_type == VT_Undefined)
			return true;

		if (expected_value.m_val_type == VT_int)
		{
			const void *data = expected_value.get_data();
			seed_hash_function(data, sizeof(int), VT_int, seed, SEED_SZ_BYTES);
			return true;
		}
		else if (expected_value.m_val_type == VT_double)
		{
			const void *data = expected_value.get_data();
			seed_hash_function(data, sizeof(double), VT_double, seed, SEED_SZ_BYTES);
			return true;
		}
		else if (expected_value.m_val_type == VT_string)
		{
			int len = expected_value.get_string(0).length();
			const void *data = expected_value.get_string(0).c_str();
			seed_hash_function(data, len, VT_string, seed, SEED_SZ_BYTES);
			return true;
		}
		else if (expected_value.m_val_type == VT_barray)
		{
			int len = expected_value.get_barray(0).size();
			const void *data = &expected_value.get_barray(0)[0];
			seed_hash_function(data, len, VT_barray, seed, SEED_SZ_BYTES);
			return true;
		}
		else if (expected_value.m_val_type == VT_uint64)
		{
			const void *data = expected_value.get_data();
			seed_hash_function(data, sizeof(uint64_t), VT_uint64, seed, SEED_SZ_BYTES);
			return true;
		}

		return false;
	}

};

};

// List of encryption buffers used by RunCompiledCode class
set<EBuffer> RunCompiledCode::m_buffer_set;

// List of pointers to external functions used by RunCompiledCode class
vector<bool(*)(PtrSet<Value>*)> RunCompiledCode::m_ext_func;

set<EBuffer> g_buffer_set;

inline const EBuffer& GetEBufferForSeed(const uint8_t* seed)
{
	EBuffer token;
	token.InitializeAsToken(seed);

	auto ebuf = g_buffer_set.find(token);
	if (ebuf == g_buffer_set.end())
	{
		token.InitializeBuffer();
		g_buffer_set.emplace(token);
		ebuf = g_buffer_set.find(token);
	}

	return (*ebuf);
}

#if _DEBUG
int g_ext_function_call_log_idx = 0;
vector<ExternalFunctionCallLogElement> g_ext_function_call_log;
#endif

// Read the encrypted compiled code file into memory
inline bool ReadCompiledCodeFile(const char* file_name, vector<uint8_t>& compiled_code, STRING& err_msg)
{
	string s0;

	if (_access(file_name, 00))
	{
		err_msg = "File does not exist: ";
		err_msg += file_name;
		return false;
	}

	FILE* stream = 0;
	fopen_s(&stream, file_name, DecodeText("A090CBD310D2CB", s0)/*rb*/);
	if (stream == 0)
	{
		err_msg = DecodeText("704E0D59E05EE656DCF09DB450BE89CC6EF63933393AECBD63", s0)/*Can't read file: */;
		err_msg += file_name;
		return false;
	}

	int64_t len = _filelengthi64(_fileno(stream));
	if (len > 0x1000000)
	{
		fclose(stream);
		err_msg = DecodeText("B0BB9B4AC203811E5B8D5C0408F295A10FD2F6393E246A247E246A9B3E", s0)/*File is too big: */;
		err_msg += file_name;
		return false;
	}

	compiled_code.resize(len);

	size_t nread = fread(&compiled_code[0], 1, len, stream);
	if (nread != len)
	{
		fclose(stream);
		err_msg = DecodeText("1E8511B09A639B13E60E2F1B175D61E218FD43F1C8E2F27EF2066BC2D2C22FE2426BC27F", s0)/*Problem reading file: */;
		err_msg += file_name;
		return false;
	}

	fclose(stream);
	return true;
}
