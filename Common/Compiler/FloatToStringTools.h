#pragma once

inline bool decimal_point_exists(const char *s_num)
{
	int i = 0;

	while (s_num[i])
	{
		if (s_num[i] == '.')
			return true;

		i++;
	}

	return false;
}

inline void eliminate_redundant_characters_from_number(char *s_num)
{
	bool exponent_found = false;

	// if last character is a decimal point, add 1 trailing zero
	{
		int n = strlen(s_num);
		if (s_num[n - 1] == '.')
		{
			s_num[n] = '0';
			s_num[n] = 0;
		}
	}

	// Eliminate leading zeros in the exponent
	int i = 1;
	while (1)
	{
		if (s_num[i] == 'e' || s_num[i] == 'E')
		{
			exponent_found = true;
			i++;
			if (s_num[i] == '-') i++; else
				if (s_num[i] == '+') i++;
			int i0 = i;
			while (s_num[i] == '0') i++;
			while (s_num[i])
			{
				s_num[i0++] = s_num[i++];
			}
			s_num[i0] = 0;

			break;
		}
		i++;
		if (!s_num[i]) break;
	}

	// Eliminate decimal point if possible
	i = 1;
	while (s_num[i])
	{
		if (s_num[i] == 'e' || s_num[i] == 'E')
		{
			if (s_num[i - 1] == '.')
			{
				while (s_num[i])
				{
					s_num[i - 1] = s_num[i];
					i++;
				}
				s_num[i - 1] = 0;
				break;
			}
		}
		i++;
	}

	// Eliminate trailing zeros
	if (!exponent_found && decimal_point_exists(s_num))
	{
		int len = strlen(s_num);
		while (s_num[len - 1] == '0')
		{
			if (!len) break;
			len--;
			if (s_num[len - 1] != '.')
				s_num[len] = 0;
		}
	}
}

inline void __fail_proof_double_to_string
(
char sign,
double input_variable,
char *output_var_str,
int max_output_str_len,
int sig_digits
)
{
	char s_tmp[128];

	_gcvt_s(s_tmp, sizeof(s_tmp), input_variable, max(sig_digits, (short)10));

	eliminate_redundant_characters_from_number(s_tmp);

	if ((short)strlen(s_tmp) <= max_output_str_len)
	{
		memmove(output_var_str, s_tmp, strlen(s_tmp) + 1);
		return;
	}

	// look for the exponent
	int i = 0;
	int iexp = -1;
	while (s_tmp[i] != 'e' && s_tmp[i] != 'E' && s_tmp[i]) i++;

	if (s_tmp[i])
		iexp = i;

	// no exponent, okay to cut off the number
	strncpy_s(output_var_str, max_output_str_len + 1, s_tmp, min(strlen(s_tmp) + 1, (int)max_output_str_len));

	if (iexp != -1)
	{
		// exponent exists
		int exp_slen = strlen(&s_tmp[iexp]);
		i = max_output_str_len - exp_slen;
		if (i > 2) // Add the exponent to the end if there is room
		{
			strcpy_s(&output_var_str[i], max_output_str_len + 1 - i, &s_tmp[iexp]);
			iexp = i;
		}
	}

	if (sign == '-')
	{
		if (iexp == -1)
		{
			for (i = max_output_str_len - 1; i>0; i--)
				output_var_str[i] = output_var_str[i - 1];
		}
		else
		{
			for (i = iexp - 1; i>0; i--)
			{
				output_var_str[i] = output_var_str[i - 1];
			}
		}

		output_var_str[0] = '-';
	}

	output_var_str[max_output_str_len] = 0;
}

inline bool EngineeringNotation
(
	double input_variable,
	char *output_var_str,
	int max_output_str_len,
	int sig_digits
)
{
	char sign = 0;
	int i, j;

	for (i = 0; i<max_output_str_len; i++)
		output_var_str[i] = 0;

	// Determine the sign of the input variable.
	if (input_variable < 0.0)
	{
		input_variable = -input_variable;
		sign = '-';
	}
	else
		sign = '+';

	double mantessa, exponent;
	int iexponent;
	_exception _ex;

	short junk = 0;

	if (input_variable == 0.0)
	{
		mantessa = 0.0;
		iexponent = 0;
	}
	else
	{
		double lg = log10(input_variable);
		if (!_matherr(&_ex))
		{
			__fail_proof_double_to_string(sign, input_variable, output_var_str, max_output_str_len, sig_digits);
			return false;
		}

		double tmp = modf(lg, &exponent);
		if (!_matherr(&_ex))
		{
			__fail_proof_double_to_string(sign, input_variable, output_var_str, max_output_str_len, sig_digits);
			return false;
		}

		mantessa = pow(10.0, tmp);
		if (!_matherr(&_ex))
		{
			__fail_proof_double_to_string(sign, input_variable, output_var_str, max_output_str_len, sig_digits);
			return false;
		}

		iexponent = (short)exponent;

		while (mantessa < 1.0)
		{
			if (iexponent < -100 || iexponent > 100)
			{
				__fail_proof_double_to_string(sign, input_variable, output_var_str, max_output_str_len, sig_digits);
				return false;
			}

			mantessa *= 10;
			iexponent--;
		}
	}

	short decimal_location = 1;

	while (abs(iexponent) % 3)
	{
		iexponent--;
		decimal_location++;
	}

	int junk0 = 0, junk1 = 0;
	char s_number[128];
	_fcvt_s(s_number, sizeof(s_number), mantessa, sig_digits, &junk0, &junk1);

	i = 0;
	if (i<max_output_str_len)
		output_var_str[i++] = sign;

	j = 0;
	while (i<max_output_str_len && j < decimal_location)
	{
		output_var_str[i++] = s_number[j++];
	}

	if (i<max_output_str_len)
		output_var_str[i++] = '.';

	while (i<max_output_str_len && j < sig_digits)
		output_var_str[i++] = s_number[j++];

	char s_exponent[10];
	sprintf_s(s_exponent, sizeof(s_exponent), "%+03d", iexponent);

	if (i<max_output_str_len)
		output_var_str[i++] = 'E';

	j = 0;
	while (i<max_output_str_len && s_exponent[j])
		output_var_str[i++] = s_exponent[j++];

	eliminate_redundant_characters_from_number(output_var_str);

	if (i == max_output_str_len)
		return (false); // Error

	return (true);
}

inline bool DoubleToString(double d, char *s_ret, int max_len)
{
	if (max_len < 10) return false;

	if (!d)
	{
		s_ret[0] = '0';
		s_ret[1] = 0;
		return true;
	}

	char *s = (char *)alloca(max_len + 10);

	if (!d)
	{
		s_ret[0] = '0';
		s_ret[1] = 0;
		return true;
	}

	while (1)
	{
		if (d < 0.0F)
		{
			if (d < -1.0e6)
				break;

			if (d > -1.0e-6)
				break;

			if (d == (int64_t)d) // Integer valued
			{
				sprintf_s(s, max_len + 10, "%lld", (int64_t)d);
				strncpy_s(s_ret, max_len, s, max_len + 1);
				return true;
			}

			short sig_dig = max_len - 1;

			_gcvt_s(s, max_len + 10, (double)d, sig_dig);
			eliminate_redundant_characters_from_number(s);
			if (strlen(s) > max_len)
			{
				sig_dig -= strlen(s) - max_len;
				if (sig_dig < 1)
				{
					strncpy_s(s_ret, max_len, s, max_len + 1);
					return FALSE;
				}

				if (sig_dig < 6)
					sig_dig = 6;

				while (1)
				{
					_gcvt_s(s, max_len + 10, (double)d, sig_dig);
					eliminate_redundant_characters_from_number(s);

					if (strlen(s) > max_len)
					{
						sig_dig--;
						continue;
					}

					break;
				}

				if (strlen(s) >= max_len)
					__debugbreak();
			}

			strncpy_s(s_ret, max_len, s, max_len + 1);
			return true;
		}

		if (d > 1.0e6)
			break;

		if (d < 1.0e-6)
			break;

		if (d == (int64_t)d) // Integer valued
		{
			sprintf_s(s, max_len + 1, "%lld", (int64_t)d);
			strncpy_s(s_ret, max_len, s, max_len + 1);
			return true;
		}

		short sig_dig = max_len - 1;

		_gcvt_s(s, max_len + 10, (double)d, sig_dig);
		eliminate_redundant_characters_from_number(s);
		if (strlen(s) > max_len)
		{
			sig_dig -= strlen(s) - max_len;
			if (sig_dig < 1)
			{
				strncpy_s(s_ret, max_len, s, max_len + 1);
				return false;
			}

			while (1)
			{
				_gcvt_s(s, max_len + 10, (double)d, sig_dig);
				eliminate_redundant_characters_from_number(s);

				if (strlen(s) > max_len)
				{
					sig_dig--;
					continue;
				}

				break;
			}

			if (strlen(s) > max_len)
				__debugbreak();
		}

		strncpy_s(s_ret, max_len, s, max_len + 1);
		return TRUE;
	}

	bool iret = EngineeringNotation(d, s, max_len, min(max_len - 8, 12));

	strncpy_s(s_ret, max_len, s, max_len + 1);

	return iret;
}

inline bool nibble_to_ascii_hex(uint8_t n, char &h)
{
	if (n < 10)
	{
		h = '0' + n;
		return true;
	}

	if (n < 16)
	{
		h = 'A' + n - 10;
		return true;
	}

	h = '?';
	return false;
}

inline bool byte_to_ascii_hex(uint8_t b, char *hex)
{
	int upper = (0x0F & (b >> 4));
	int lower = (0x0F & b);

	nibble_to_ascii_hex(upper, hex[0]);
	nibble_to_ascii_hex(lower, hex[1]);
	hex[2] = 0;

	return true;
}