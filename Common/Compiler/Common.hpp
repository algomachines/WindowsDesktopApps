#include "ParamTools.hpp"
#include "SendDataToServer.h"

#pragma once

// Define times to simplify
typedef vector<uint8_t> barray;
typedef PtrSet<Value> Params;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
inline uint32_t GET_UINT32(const Params* params, int iparam, int idx = 0)
{
	return params->GetObjectConst(iparam)->get_uint32(idx);
}

inline void SET_UINT32(Params* params, int iparam, uint32_t ivalue, int idx = 0)
{
	params->GetObjectA(iparam)->set_uint32(idx, ivalue);
}

inline uint64_t GET_UINT64(const Params* params, int iparam, int idx = 0)
{
	return params->GetObjectConst(iparam)->get_uint64(idx);
}

inline void SET_UINT64(Params* params, int iparam, uint64_t ivalue, int idx = 0)
{
	params->GetObjectA(iparam)->set_uint64(idx, ivalue);
}


inline int GET_INT(const Params* params, int iparam, int idx = 0)
{
	return params->GetObjectConst(iparam)->get_int(idx);
}

inline void SET_INT(Params* params, int iparam, int ivalue, int idx = 0)
{
	params->GetObjectA(iparam)->set_int(idx, ivalue);
}

inline const barray* GET_BARRAY(Params* params, int iparam, int idx = 0)
{
	return &params->GetObjectConst(iparam)->get_barray(idx);
}

inline barray* GET_BARRAY_WRITABLE(Params* params, int iparam, int idx = 0)
{
	return &params->GetObjectA(iparam)->get_barray_writable(idx);
}

inline const char* GET_STRING(Params* params, int iparam, int idx = 0)
{
	return params->GetObjectConst(iparam)->get_string(idx).c_str();
}

inline uint32_t GET_ARRAY_SIZE(Params* params, int iparam)
{
	return params->GetObjectConst(iparam)->GetArraySize();
}
////////////////////////////////////////////////////////////////////////////////////////////////////


// barray_is_equal(barray data0, barray data1, int sz, int status)
// sz - number of bytes to compare starting at the beginning of both arrays
// status - 1 if they are equal, otherwise 0
bool EF_barray_is_equal(Params* params)
{
	const barray* data0 = GET_BARRAY(params, 0);
	const barray* data1 = GET_BARRAY(params, 1);
	int sz = GET_INT(params, 2);

	SET_INT(params, 3, 0); // initially set status to 0

	if (data0->size() < sz || data1->size() < sz)
		return true;

	if (memcmp(&(*data0)[0], &(*data1)[0], sz))
		return true;

	SET_INT(params, 3, 1); // set status to 1

	return true;
}


// barray_resize(barray, int);
bool EF_barray_resize(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_INT(1);

	int sz = params->GetObjectA(1)->get_int(0);
	vector<vector<uint8_t>> *barray = params->GetObjectA(0)->initialize_barray(1);

	(*barray)[0].resize(sz);

	return true;
}

// barray_append_byte(barray data, int b)
// Append one byte to the barray
bool EF_barray_append_byte(Params* params)
{
	barray* data = GET_BARRAY_WRITABLE(params, 0);
	int b = GET_INT(params, 1);

	if (b < 0 || b > 255)
		return false;

	data->push_back((uint8_t)b);

	return true;
}

// barray_assign(barray out, barray in);
bool EF_barray_assign(Params* params)
{
	barray *out = GET_BARRAY_WRITABLE(params, 0);
	const barray *in = GET_BARRAY(params, 1);

	out->resize(in->size());
	memmove(&(*out)[0], &(*in)[0], in->size());

	return true;
}

/////////////////////////////////////////////////////////////////////////////////
// Remove some data from a barray, return the data removed in another barray
// source - barray from which data is removed
// ipos - starting position from which data is removed
// sz - number of bytes to remove
// cut - barray to receive the removed data
// barray_cut(barray source, int ipos, int sz, barray cut)
bool EF_barray_cut(Params* params)
{
	barray* source = GET_BARRAY_WRITABLE(params,0);
	int ipos = GET_INT(params, 1);
	int sz = GET_INT(params, 2);
	barray* cut = GET_BARRAY_WRITABLE(params, 3);

	if (sz < 0 || sz > source->size())
		return false;

	cut->resize(sz);

	for (int i = ipos; i < ipos + sz; i++)
		(*cut)[i - ipos] = (*source)[i];

	source->erase(source->begin() + ipos, source->begin() + ipos + sz);

	return true;
}

// sz  : size of the barray data in bytes
// v   : the barray variable
// idx : index of the barray variable, i.e. v[idx]
// barray_size(uint32 sz, barray v, uint32 idx)
bool EF_barray_size(Params *params)
{
	SET_UINT32(params, 0, GET_BARRAY(params, 1, GET_UINT32(params, 2))->size());
	return true;
}

// bin_e_int - contains data for e_int[] array
// v_ret is the returned int value
// get_eint(int v_ret, barray bin_e_int, int idx)
bool EF_get_e_int(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_INT(0);
	VERIFY_PARAM_IS_BARRAY(1);
	VERIFY_PARAM_IS_INT(2);

	const vector<uint8_t> *bin_e_int = &params->GetObjectConst(1)->get_barray(0);
	e_int idx;
	idx = params->GetObjectConst(2)->get_int(0);
	idx = idx * sizeof(e_int);

	if (bin_e_int->size() <= idx)
	{
		ASSERT(FALSE);
		return false;
	}

	e_int v_ret;
	memmove(&v_ret, &(*bin_e_int)[idx], sizeof(e_int));

	params->GetObjectA(0)->set_int(0,v_ret);

	return true;
}

//uint32_array_set_u8(uint32 data_out, int val_u8, uint32 offset_u8, uint32 nbytes);
bool EF_uint32_array_set_u8(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(4);
	VERIFY_PARAM_IS_UINT32(0);
	VERIFY_PARAM_IS_INT(1);
	VERIFY_PARAM_IS_UINT32(2);
	VERIFY_PARAM_IS_UINT32(3);

	uint32_t offset_u8 = params->GetObjectConst(2)->get_uint32(0);
	uint32_t nbytes = params->GetObjectConst(3)->get_uint32(0);
	int val_u8 = params->GetObjectConst(1)->get_int(0);
	uint32_t out_sz_bytes = params->GetObjectConst(0)->GetArraySize();
	out_sz_bytes *= 4;

	// Make sure that data_out_u32 is large enough to receive the data
	if (out_sz_bytes < nbytes)
	{
		ASSERT(FALSE);
		return false;
	}

	uint8_t *data_out_u8 = (uint8_t *)params->GetObjectA(0)->get_data_writable();

	memset((void *)(data_out_u8+offset_u8), val_u8, nbytes);

	return true;
}

bool EF_encrypt_string_to_barray(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT64(0); // seed
	VERIFY_PARAM_IS_STRING(1); // string to encrypt
	VERIFY_PARAM_IS_BARRAY(2); // encrypted string

	uint64_t seed = params->GetObjectConst(0)->get_uint64(0);
	int len = params->GetObjectConst(1)->get_string(0).length();
	const char *msg = params->GetObjectConst(1)->get_string(0).c_str();

	uint8_t hash[16];
	MurmurHash3_x64_128(&seed, sizeof(seed), 0, hash);
	memmove(&seed, hash, sizeof(seed));

	size_t e_sz = 100000000 + (seed & 0x8FFFFFF); // ~ 220 million
	e_sz /= 16;
	e_sz *= 16;  // must be a factor of 16

	uint8_t *e = new uint8_t[e_sz];

	create_random_buffer(e, e_sz, hash);

	vector<uint8_t> *output = &params->GetObjectA(2)->get_barray_writable(0);
	output->resize(len);
	memmove((void *)&(*output)[0], msg, len);

	const uint8_t* b = &(*output)[0];

	__encrypt__(&(*output)[0], len, hash, sizeof(hash), e, (uint32_t)e_sz);

	delete[] e;

	return true;
}

// t - unix time in resolution ms
// get_time_ms(uint64 t)
bool EF_get_time_ms(Params *params)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	uint64_t t;
	SystemTimeToFileTime(&st, (FILETIME*)&t);

	// t is in 100 ns increments
	t /= 10000; // convert to ms

	uint64_t WindowsToUnixEpoc_ms = 11644473600000;
	if (t < WindowsToUnixEpoc_ms)
		return false;

	t -= WindowsToUnixEpoc_ms;

	SET_UINT64(params, 0, t);

	return true;
}

bool EF_get_unix_time_e(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(1);
	VERIFY_PARAM_IS_BARRAY(0); // encrypted time

	e_uint64 t;
	t = (uint64_t)_time64(NULL);

	params->GetObjectA(0)->get_barray_writable(0).resize(sizeof(t));
	memmove(&params->GetObjectA(0)->get_barray_writable(0)[0], &t, sizeof(t));

	return true;
}

bool EF_hash_64(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_UINT64(0); // 64 bit hash value
	VERIFY_PARAM_IS_UINT64(1); // data to hash

	uint32_t sz = params->GetObjectConst(1)->GetArraySize();

	uint32_t seed = 0;
	MurmurHash3_x86_32(params->GetObjectConst(1)->get_data(), seed, sz * sizeof(uint64_t), &seed);

	uint64_t hash[2];
	MurmurHash3_x64_128(params->GetObjectConst(1)->get_data(), sz * sizeof(uint64_t), seed, hash);

	params->GetObjectA(0)->set_uint64(0,hash[1]);

	return true;
}

inline void _hash_id(uint8_t* ID)
{
	const int sz = 32;
	
	uint8_t ID_hashed[sz];

	uint32_t seed;
	memmove(&seed, ID, sizeof(seed));
	MurmurHash3_x86_128(ID, sz, seed, &ID_hashed[0]);
	memmove(&seed, ID_hashed, sizeof(seed));
	MurmurHash3_x86_128(ID, sz, seed, &ID_hashed[16]);

	memmove(ID, ID_hashed, sz);
}

// hash - returned hash value - 32 bytes
// config_data - config data - begins with the 32 byte program id
// hash_program_id(barray hash, barray config_data)
bool EF_hash_program_id(Params* params)
{
	barray* hash = GET_BARRAY_WRITABLE(params, 0);
	const barray* config_data = GET_BARRAY(params, 1);

	if (config_data->size() < 32)
		return 0;

	const uint8_t* b = &(*config_data)[0];

	hash->resize(32);
	memmove(&(*hash)[0], &(*config_data)[0], 32);

	const uint8_t *h0 = &(*hash)[0];

	_hash_id(&(*hash)[0]);
	_hash_id(&(*hash)[0]);

	const uint8_t *h1 = &(*hash)[0];

	return true;
}

bool EF_random_64(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_UINT64(0); // array to receive random data
	VERIFY_PARAM_IS_UINT64(1); // the seed

	uint32_t sz = params->GetObjectConst(0)->GetArraySize();
	sz *= sizeof(uint64_t);

	uint8_t* data = (uint8_t *)params->GetObjectA(0)->get_data_writable();
	if (data == 0)
	{
		params->GetObjectA(0)->initialize_uint64(sz);
		data = (uint8_t*)params->GetObjectA(0)->get_data_writable();
	}
	else
		ZeroMemory(data, sz);

	uint64_t seed = params->GetObjectConst(1)->get_uint64(0);

	uint8_t* key = new uint8_t[sz];
	memmove(key, data, sz);

	CreateRandomBufferFromSeed(key, sz, 0, data, sz, (seed % 1000) + 1);

	delete[] key;

	return true;
}

bool EF_MurmurHash3_x64_128(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_BARRAY(0); // data to hash
	VERIFY_PARAM_IS_UINT32(1); // seed
	VERIFY_PARAM_IS_BARRAY(2); // 16 byte hash value

	const vector<uint8_t>* data = &params->GetObjectA(0)->get_barray(0);
	e_uint32 seed;
	seed = params->GetObjectA(1)->get_uint32(0);

	vector<uint8_t>* hash = &params->GetObjectA(2)->get_barray_writable(0);
	hash->resize(16);

	MurmurHash3_x64_128(&(*data)[0], data->size(), seed, &(*hash)[0]);

	return true;
}

// MurmurHash3_x86_32(barray key, uint32 seed, uint32 hash)
bool EF_MurmurHash3_x86_32(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_BARRAY(0); // key
	VERIFY_PARAM_IS_UINT32(1); // seed
	VERIFY_PARAM_IS_UINT32(2); // hash

	const vector<uint8_t>* key = &params->GetObjectConst(0)->get_barray(0);
	uint32_t seed = params->GetObjectConst(1)->get_uint32(0);
	uint32_t hash;

	MurmurHash3_x86_32(&(*key)[0], key->size(), seed, &hash);

	params->GetObjectA(2)->set_int(0, hash);
	return true;
}

// symmetric_encryption(barray data, barray guid)
// data - data to encrypt / decrypt
// guid - must have size = 16 i.e. sizeof(GUID)
bool EF_symmetric_encryption(Params* params)
{
	barray* buf = GET_BARRAY_WRITABLE(params, 0);
	const barray* v_guid = GET_BARRAY(params, 1);

	if (v_guid->size() != sizeof(GUID))
		return false;

	const GUID* guid = (const GUID*)&(*v_guid)[0];

	//string s0,s1;
	//const uint8_t* b = &(*buf)[0];
	//bin_to_hex_char(b, buf->size(), s0);

	symmetric_encryption(&(*buf)[0], buf->size(), *guid);

	//b = &(*buf)[0];
	//bin_to_hex_char(b, buf->size(), s1);

	return true;
}

bool EF_symmetric_encryption_e_uint64(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_BARRAY(0); // encrypted uint64
	VERIFY_PARAM_IS_STRING(1); // encrypted encoded guid

	// Get the encrypted uint64 value
	e_uint64 e_u64;
	memmove(&e_u64, &params->GetObjectA(0)->get_barray_writable(0)[0], sizeof(e_u64));

	// Decode the guid from the encoded text
	string s;
	DecodeText(params->GetObjectA(1)->get_string(0).c_str(), s);

	vector<uint8_t> buf;
	if (hex_char_to_bin(s, buf) == 0)
	{
		Zero(s);
		return false;
	}

	if (buf.size() != sizeof(GUID))
		return false;

	Zero(s);

	// decrypt the uint64 value
	uint64_t u64;
	u64 = e_u64;

	// access the decoded guid value
	const GUID *guid  = (const GUID *)&buf[0];
	// run the standard symmetric encryption algo modifying the contents of u64
	symmetric_encryption((uint8_t *)&u64, sizeof(u64), *guid);

	Zero(buf);

	// resize the first parameter to receive the symmetrically encrypted value of u64
	params->GetObjectA(0)->get_barray_writable(0).resize(sizeof(u64));

	// Move the contents of u64 into the newly resized first parameter
	memmove(&params->GetObjectA(0)->get_barray_writable(0)[0], &u64, sizeof(u64));

	ZERO(u64);

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Inserts into the target at the given position, the given number of bytes.
// Bytes are taken starting from the beginning of source
// 
// target - destination buffer for the inserted bytes
// source - source buffer for the inserted bytes (starting from the beginning)
// insert_pos - starting insert position in target for the insert
// count - number of bytes to insert from source into target
// 
// barray_insert(barray target, barray source, int insert_pos, int count)
bool EF_barray_insert(Params* params)
{
	VERIFY_PARAM_COUNT(4);
	VERIFY_PARAM_IS_BARRAY(0); // target barray
	VERIFY_PARAM_IS_BARRAY(1); // source barray
	VERIFY_PARAM_IS_INT(2);    // insert_pos
	VERIFY_PARAM_IS_INT(3);    // count

	barray* target = GET_BARRAY_WRITABLE(params,0);
	const barray* source = GET_BARRAY(params,1);
	int insert_pos = GET_INT(params, 2);
	int count = GET_INT(params, 3);

	if (count <= 0)
		return false;

	if (insert_pos < 0)
		return false;

	if (source->size() < count)
		return false;

	if (target->size() < insert_pos)
		return false;

	target->insert(target->begin() + insert_pos, source->begin(), source->begin() + count);

	return true;
}

bool EF_decrypt_barray_to_string(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT64(0); // seed
	VERIFY_PARAM_IS_BARRAY(1); // encrypted barray
	VERIFY_PARAM_IS_STRING(2); // decrypted string

	uint64_t seed = params->GetObjectConst(0)->get_uint64(0);
	int len = params->GetObjectConst(1)->get_barray(0).size();
	const uint8_t *encrypted_data = &params->GetObjectConst(1)->get_barray(0)[0];

	uint8_t hash[16];
	MurmurHash3_x64_128(&seed, sizeof(seed), 0, hash);
	memmove(&seed, hash, sizeof(seed));

	size_t e_sz = 100000000 + (seed & 0x8FFFFFF); // ~ 220 million
	e_sz /= 16;
	e_sz *= 16;  // must be a factor of 16

	uint8_t *e = new uint8_t[e_sz];

	create_random_buffer(e, e_sz, hash);

	uint8_t *output = new uint8_t[len+1];
	memmove(output, encrypted_data, len);
	output[len] = 0;

	__encrypt__(output, len, hash, sizeof(hash), e, (uint32_t)e_sz);

	delete[] e;

	//MessageBox(NULL, (const char *)output, "", MB_OK);

	params->GetObjectA(2)->set_string(0, (const char *)output);

	return true;
}

bool EF_exp(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_DOUBLE(0);
	VERIFY_PARAM_IS_DOUBLE(1);

	double vin = params->GetObjectConst(1)->get_double(0);

	return params->GetObjectA(0)->set_double(0, exp(vin));
}

bool EF_log(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_DOUBLE(0);
	VERIFY_PARAM_IS_DOUBLE(1);

	double vin = params->GetObjectConst(1)->get_double(0);

	return params->GetObjectA(0)->set_double(0, log(vin));
}

// barray_asssign_u32(barray data_out_u8, uint32 data_in_u32, uint32 offset_u32, uint32 num_u32)
bool EF_barray_assign_u32(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(4);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_UINT32(1);
	VERIFY_PARAM_IS_UINT32(2);
	VERIFY_PARAM_IS_UINT32(3);

	uint32_t offset_u32 = params->GetObjectConst(2)->get_uint32(0);
	uint32_t num_u32 = params->GetObjectConst(3)->get_uint32(0);
	const uint32_t *data_in_u32 = (const uint32_t *)params->GetObjectConst(1)->get_data();
	vector<uint8_t> *data_out_u8 = &params->GetObjectA(0)->get_barray_writable(0);

	// Make sure that data_in_u32 matches the given parameters
	if (params->GetObjectConst(1)->GetArraySize() < offset_u32 + num_u32)
	{
		ASSERT(FALSE);
		return false;
	}

	// Make sure that data_out_u8 has enough space allocated
	if (num_u32 * 4 > data_out_u8->size())
	{
		ASSERT(FALSE);
		return false;
	}

	memmove(&(*data_out_u8)[0], (void *)(data_in_u32 + offset_u32), num_u32 * 4);

	return true;
}

// data_out_u32 is zero padded of barray is not a multiple of 4 byrtes
// u32_assign_barray(uint32 data_out_u32, barray data_in_u8)
bool EF_u32_assign_barray(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_UINT32(0);
	VERIFY_PARAM_IS_BARRAY(1);

	const vector<uint8_t>* data_in_u8 = &params->GetObjectConst(1)->get_barray(0);

	int u32_array_sz = data_in_u8->size() / 4;
	if (data_in_u8->size() % 4)
		u32_array_sz++;

	uint32_t u32_current_array_sz = params->GetObjectConst(0)->GetArraySize();

	if (u32_current_array_sz < u32_array_sz)
		return false;

	memmove(params->GetObjectA(0)->get_data_writable(), &(*data_in_u8)[0], data_in_u8->size());

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy into uint32 array the contents of the barray
// data_out - the uint32 array
// data_in - the barray with data to be copied
// offset_u8 - the offset wrt the beginning of the barray where copying starts
// nbytes - the number of bytes to transfer - must be a multiple of 4
// 
// uint32_array_assign_u8(uint32 data_out, barray data_in, uint32 offset_u8, uint32 nbytes);
// 
bool EF_uint32_array_assign_u8(Params *params)
{
	const barray* data_in = GET_BARRAY(params, 1);
	uint32_t offset_u8 = GET_UINT32(params, 2);
	uint32_t nbytes = GET_UINT32(params, 3);
	
	uint32_t out_sz_bytes = params->GetObjectConst(0)->GetArraySize();
	out_sz_bytes *= sizeof(uint32_t);

	// Make sure that data_in_u8 is large enough to provide the data
	if (data_in->size() < offset_u8 + nbytes)
	{
		ASSERT(FALSE);
		return false;
	}

	// Make sure that data_out_u32 is large enough to receive the data
	if (out_sz_bytes < nbytes)
	{
		ASSERT(FALSE);
		return false;
	}

	uint8_t *data_out_u8 = (uint8_t *)params->GetObjectA(0)->get_data_writable();

	memmove((void *)data_out_u8, (const void *)&(*data_in)[offset_u8], nbytes);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy into uint64 array the contents of the barray
// data_out - the uint64 array
// data_in - the barray with data to be copied
// offset_u8 - the offset wrt the beginning of the barray where copying starts
// nbytes - the number of bytes to transfer - must be a multiple of 4
// 
// uint64_array_assign_u8(uint64 data_out, barray data_in, uint32 offset_u8, uint32 nbytes);
// 
bool EF_uint64_array_assign_u8(Params* params)
{
	const barray* data_in = GET_BARRAY(params, 1);
	uint32_t offset_u8 = GET_UINT32(params, 2);
	uint32_t nbytes = GET_UINT32(params, 3);

	uint32_t out_sz_bytes = params->GetObjectConst(0)->GetArraySize();
	out_sz_bytes *= sizeof(uint64_t);

	// Make sure that data_in_u8 is large enough to provide the data
	if (data_in->size() < offset_u8 + nbytes)
	{
		ASSERT(FALSE);
		return false;
	}

	// Make sure that data_out_u32 is large enough to receive the data
	if (out_sz_bytes < nbytes)
	{
		ASSERT(FALSE);
		return false;
	}

	uint8_t* data_out_u8 = (uint8_t*)params->GetObjectA(0)->get_data_writable();

	memmove((void*)data_out_u8, (const void*)&(*data_in)[offset_u8], nbytes);

	return true;
}

//barray_append_string(barray target, STRING txt, int pad_to_size)
bool EF_barray_append_string(Params* params)
{
	barray* target = GET_BARRAY_WRITABLE(params, 0);
	const char* s = GET_STRING(params, 1);
	int pad_to_size = GET_INT(params, 2);

	int n=0;
	while (s[n])
	{
		n++;
		if (n > pad_to_size)
			break;
	}

	if (n > pad_to_size)
		return false;

	int n0 = target->size();

	target->resize(n0 + pad_to_size);
	
	memmove(&(*target)[n0], &s[0], n);

	return true;
}

//barray_append_hex_string(barray target, STRING txt, int reverse_byte_order);
bool EF_barray_append_hex_string(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_STRING(1);
	VERIFY_PARAM_IS_INT(2);

	const string *s_hex = &params->GetObjectConst(1)->get_string(0);

	e_int len;
	len = s_hex->length();

	vector<uint8_t> *target = &params->GetObjectA(0)->get_barray_writable(0);

	int i0 = target->size();
	target->resize(i0 + len / 2);

	e_int reverse_byte_order;
	reverse_byte_order = params->GetObjectA(2)->get_int(0);

	e_int sz;
	sz = target->size();

	e_int i;
	for (i = 0; i < len; i = i + 2)
	{
		e_int h0;
		h0 = (*s_hex)[i];
		e_int h1;
		h1 = (*s_hex)[i + 1];

		if (h0 >= '0' && h0 <= '9')
			h0 = h0 - '0';
		else if (h0 >= 'A' && h0 <= 'F')
			h0 = 10 + h0 - 'A';
		else if (h0 >= 'a' && h0 <= 'f')
			h0 = 10 + h0 - 'a';
		else
			ASSERT(FALSE);

		if (h1 >= '0' && h1 <= '9')
			h1 = h1 - '0';
		else if (h1 >= 'A' && h1 <= 'F')
			h1 = 10 + h1 - 'A';
		else if (h1 >= 'a' && h1 <= 'f')
			h1 = 10 + h1 - 'a';
		else
			ASSERT(FALSE);

		e_int b;
		b = h1 + (h0 << 4);

		if (reverse_byte_order == 0)
			(*target)[i0 + i / 2] = (uint8_t)b;
		else
			(*target)[(sz - 1) - (i / 2)] = (uint8_t)b;
	}

	const uint8_t *check = &(*target)[0];

	return true;
}

// barray_append_uint64(barray target, uint64 data, int nbytes, int reverse_byte_order)
bool EF_barray_append_uint64(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(4);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_UINT64(1);
	VERIFY_PARAM_IS_INT(2);
	VERIFY_PARAM_IS_INT(3)

	int nbytes = params->GetObjectConst(2)->get_int(0);

	ASSERT(nbytes > 0 && nbytes <= 8);

	union
	{
		uint64_t data_u64;
		uint8_t data_u8[8];
	};

	data_u64 = params->GetObjectConst(1)->get_uint64(0);

	vector<uint8_t> *v = &params->GetObjectA(0)->get_barray_writable(0);
	ASSERT(v);

	int i0 = v->size();
	v->resize(i0 + nbytes);

	int reverse_byte_order = params->GetObjectA(3)->get_int(0);

	int sz = v->size();
	for (int i = i0; i < v->size(); i++)
	{
		if (reverse_byte_order == 0)
			(*v)[i] = data_u8[i - i0];
		else
			(*v)[i] = data_u8[(sz - 1) - (i - i0)];
	}

	return true;
}

// barray_append_uint32(barray data, uint32 u32)
bool EF_barray_append_uint32(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_UINT32(1);

	vector<uint8_t>* data = &params->GetObjectA(0)->get_barray_writable(0);

	uint32_t u32_array_sz = params->GetObjectConst(1)->GetArraySize();


	uint32_t insert_pos = data->size();
	uint32_t n = insert_pos + u32_array_sz * 4;
	data->resize(n);

	for (uint32_t idx = 0; idx < u32_array_sz; idx++)
	{
		uint32_t u32 = params->GetObjectConst(1)->get_uint32(idx);

		memmove(&(*data)[insert_pos], &u32, 4);
		insert_pos += 4;
	}

	return true;
}

bool EF_uint32_array_assign(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_UINT32(0);
	VERIFY_PARAM_IS_BARRAY(1);

	vector<uint32_t> *array = (vector<uint32_t> *)params->GetObjectA(0)->get_data();
	const vector<uint8_t> *input = (const vector<uint8_t> *)&params->GetObjectConst(1)->get_barray(0);

	if (array->size() != 8 * input->size())
		__debugbreak();

	memmove(&(array[0]), &(*input)[0], input->size());

	return true;
}

#define be32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))

bool EF_uint32_assign_hex_string(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT32(0);
	VERIFY_PARAM_IS_STRING(1);
	VERIFY_PARAM_IS_INT(2);

	int idx = params->GetObjectConst(2)->get_int(0);
	const string *s_hex = &params->GetObjectConst(1)->get_string(0);
	int len = s_hex->length();

	uint32_t sz = params->GetObjectConst(0)->GetArraySize();
	uint32_t *target_u32 = (uint32_t *)params->GetObjectA(0)->get_data_writable();

	if (idx > sz + len / 2)
	{
		ASSERT(FALSE);
		return false;
	}

	uint8_t *target = (uint8_t *)&target_u32[idx];

	for (int i = 0; i < len; i += 2)
	{
		int h0 = (*s_hex)[i];
		int h1 = (*s_hex)[i + 1];

		if (h0 >= '0' && h0 <= '9')
			h0 = h0 - '0';
		else if (h0 >= 'A' && h0 <= 'F')
			h0 = 10 + h0 - 'A';
		else if (h0 >= 'a' && h0 <= 'f')
			h0 = 10 + h0 - 'a';
		else
			ASSERT(FALSE);

		if (h1 >= '0' && h1 <= '9')
			h1 = h1 - '0';
		else if (h1 >= 'A' && h1 <= 'F')
			h1 = 10 + h1 - 'A';
		else if (h1 >= 'a' && h1 <= 'f')
			h1 = 10 + h1 - 'a';
		else
			ASSERT(FALSE);

		int b = h1 + (h0 << 4);

		target[i / 2] = (uint8_t)b;
	}

	int n = len / 8;
	for (int i = idx; i < idx+n; i++)
	{
		ASSERT(i < sz);
		target_u32[i] = be32(target_u32[i]); // Reverse the bytes
	}

	return true;
}

#include "MurmurHash3.h"
// hash_to_uint(uint64 hash, barray data, uint64 niter)
bool EF_hash_to_uint64(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT64(0);
	VERIFY_PARAM_IS_BARRAY(1);
	VERIFY_PARAM_IS_UINT64(2);

	const vector<uint8_t> *data = &params->GetObjectConst(1)->get_barray(0);
	uint64_t niter = params->GetObjectConst(2)->get_uint64(0);

	uint8_t out_128[16], in_128[16];
	
	MurmurHash3_x64_128(&(*data)[0], data->size(), (uint32_t)niter, out_128);

	for (uint64_t iter = 1; iter < niter; iter++)
	{
		memmove(in_128, out_128, sizeof(in_128));
		MurmurHash3_x64_128(in_128, sizeof(in_128), (uint32_t)iter, out_128);
	}

	uint64_t *v = (uint64_t *)params->GetObjectA(0)->get_data_writable();

	memmove(v, out_128, sizeof(uint64_t));

	return true;
}

//hash_string_to_uint64(uint64 hash, STRING data, uint64_niter)
bool EF_hash_string_to_uint64(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT64(0);
	VERIFY_PARAM_IS_STRING(1);
	VERIFY_PARAM_IS_UINT64(2);

	const string* data = &params->GetObjectConst(1)->get_string(0);
	uint64_t niter = params->GetObjectConst(2)->get_uint64(0);

	uint8_t out_128[16], in_128[16];

	MurmurHash3_x64_128(&(*data)[0], data->size(), (uint32_t)niter, out_128);

	for (uint64_t iter = 1; iter < niter; iter++)
	{
		memmove(in_128, out_128, sizeof(in_128));
		MurmurHash3_x64_128(in_128, sizeof(in_128), (uint32_t)iter, out_128);
	}

	uint64_t* v = (uint64_t*)params->GetObjectA(0)->get_data_writable();

	memmove(v, out_128, sizeof(uint64_t));

	return true;
}

bool EF_assign_uint64_from_barray(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_UINT64(0);
	VERIFY_PARAM_IS_BARRAY(1);
	VERIFY_PARAM_IS_INT(2);

	const vector<uint8_t>* data = &params->GetObjectConst(1)->get_barray(0);
	int offset = params->GetObjectConst(2)->get_int(0);

	if (data->size() < offset + sizeof(uint64_t))
		return false;

	if (offset < 0)
		return false;

	uint64_t v;
	memmove(&v, &(*data)[0 + offset], sizeof(uint64_t));

	params->GetObjectA(0)->set_uint64(0,v);

	return true;
}

// to_stdout(STRING)
bool EF_to_stdout(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(1);
	VERIFY_PARAM_IS_STRING(0);

	e_uint32 array_sz;
	array_sz = params->GetObjectConst(0)->GetArraySize();
	e_uint32 i;
	for (i = 0; i < array_sz; i = i + 1)
		printf_s("%s", params->GetObjectConst(0)->get_string(i).c_str());

	return true;
}

// string_from_barray(STRING dest, barray source)
bool EF_string_from_barray(Params* params)
{
	const barray* source = GET_BARRAY(params, 1);

	int n = 0;
	string s;
	for (int i = 0; i < source->size(); i++)
	{
		if (isascii((*source)[i]))
			s.push_back((char)(*source)[i]);
	}

	if (params->GetObjectA(0)->get_data() == 0)
	{
		params->GetObjectA(0)->initialize_string(1);
	}

	params->GetObjectA(0)->set_string(0, s.c_str());

	return true;

}

// string_append(STRING target, STRING s_to_append);
bool EF_string_append(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_STRING(1);

	const Value *s_to_append = params->GetObjectConst(1);

	string s_append;
	s_to_append->GetValueAsString(s_append);

	Value *s_target = params->GetObject(0);

	s_target->AppendString(s_append);

	return true;
}

bool EF_string_len(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_INT(1);

	if (params->GetObjectConst(0)->GetData() == 0)
	{
		params->GetObjectA(1)->set_int(0, 0);
		return true;
	}

	int len = params->GetObjectConst(0)->get_string(0).length();

	params->GetObjectA(1)->set_int(0, len);

	return true;
}

bool EF_file_exists(PtrSet<Value> *params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_INT(1);

	const Value *val = params->GetObjectConst(0);

	string file_name;
	val->GetValueAsString(file_name);

	int exists = _access(file_name.c_str(), 00)==0 ? 1 : 0;

	memmove(params->GetObjectA(1)->get_data_writable(), &exists, sizeof(exists));

	return true;
}

bool EF_get_data_dir(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(1);
	VERIFY_PARAM_IS_STRING(0);

	params->GetObjectA(0)->set_string(0, s_data_dir);

	return true;
}

bool EF_prompt(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(1);
	VERIFY_PARAM_IS_STRING(0);

	uint32_t sz = params->GetObjectConst(0)->GetArraySize();

	sz = min(sz, 20);

	STRING s;
	for (uint32_t i = 0; i < sz; i++)
	{
		if (i) s += "\n";

		if (params->GetObjectConst(0)->GetData())
			s += params->GetObjectConst(0)->get_string(i).c_str();
	}

	MessageBox(NULL, s, "prompt", MB_OK);

	return true;
}

// append_string_array_to_file(STRING file_name, STRING data, STRING err_msg)
bool EF_append_string_array_to_file(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_STRING(1);
	VERIFY_PARAM_IS_STRING(2);

	string file_name = params->GetObjectConst(0)->get_string(0);

	uint32_t array_sz = params->GetObjectConst(1)->GetArraySize();

	FILE* stream = 0;
	if (_access(file_name.c_str(), 00) == 0)
	{
		fopen_s(&stream, file_name.c_str(), "a");
		if (!stream)
		{
			STRING msg;
			string tmp;
			msg = DecodeText("6CBCEB948C2B26E08D6E8D865C8C5EEF069BE7826F41610F6017228774D2948A7E8AF2F606F606F6E0E2F606E0E05BE88A", tmp)/*Unable to open existing file: */;
			msg += file_name.c_str();
			params->GetObjectA(2)->set_string(0, msg);
			return true;
		}
	}
	else
	{
		fopen_s(&stream, file_name.c_str(), "w");
		if (!stream)
		{
			STRING msg;
			string tmp;
			msg = DecodeText("6C411FF7F88CB5A640D6914B1B679D92B56050439274F222E9F27E459522E945", tmp)/*Unable to create file: */;
			msg += file_name.c_str();
			params->GetObjectA(2)->set_string(0, msg);
			return true;
		}
	}

	for (uint32_t i = 0; i < array_sz; i++)
		fprintf_s(stream, "%s\n", params->GetObjectConst(1)->get_string(i).c_str());

	fclose(stream);

	return true;
}

// generate_random_data(barray buf, int sz) 
bool EF_generate_random_data(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_BARRAY(0);
	VERIFY_PARAM_IS_INT(1);

	PROCESS_MEMORY_COUNTERS_EX data;
	BOOL status = GetProcessMemoryInfo
	(
		GetCurrentProcess(),
		(PROCESS_MEMORY_COUNTERS *)&data,
		sizeof(data)
	);

	SYSTEMTIME st;
	GetSystemTime(&st);

	uint64_t t;
	SystemTimeToFileTime(&st, (FILETIME*)&t);
	uint32_t seed;
	MurmurHash3_x86_32(&st, sizeof(st), t, &seed);

	int sz = params->GetObjectConst(1)->get_int(0);
	vector<uint8_t>* buf = &params->GetObjectA(0)->get_barray_writable(0);
	buf->resize(sz);

	int niter = 10 + (seed % 99); // number between 10 and 99

	return CreateRandomBufferFromSeed((const uint8_t *)&data, sizeof(data), seed, &(*buf)[0], sz, niter);
}

// get_file_size(STING file_name, uint64 sz)
bool EF_get_file_size(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_UINT64(1);

	const char* file_in = params->GetObjectConst(0)->get_string(0).c_str();
	if (DoesFileExist(file_in) == false)
		return false;

	if (params->GetObjectA(1)->set_uint64(0, filelength_64(file_in)) == false)
		return false;

	return true;
}

// delete_file(STRING file_name)
bool EF_delete_file(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(1);
	VERIFY_PARAM_IS_STRING(0);

	if (DeleteFile(params->GetObjectConst(0)->get_string(0).c_str()) == FALSE)
		return false;

	return true;
}

// does_file_exist(STRING file_name, int state)
bool EF_does_file_exist(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_INT(1);

	if (DoesFileExist(params->GetObjectConst(0)->get_string(0).c_str()) == true)
		params->GetObjectA(1)->set_int(0, 1);
	else
		params->GetObjectA(1)->set_int(0, 0);

	return true;
}

inline int remove_port_from_url(STRING& url)
{
	int len = StrLen(url);

	const char* s = url;

	for (int i = len-1; i > 0; i--)
	{
		if (s[i] == ':')
		{
			int port = 0;
			if (sscanf_s(&s[i+1], "%ld", &port) != 1)
				return 0;

			url.Remove(i, len - i);
			return port;
		}
	}

	return 80; // default port
}

// send_data_to_server(STRING url, barray send_data, barray receive_data)
bool EF_send_data_to_server(PtrSet<Value>* params)
{
	VERIFY_PARAM_COUNT(3);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_BARRAY(1);
	VERIFY_PARAM_IS_BARRAY(2);

	string send_buf;
	bin_to_hex_char(&params->GetObjectConst(1)->get_barray(0)[0], params->GetObjectConst(1)->get_barray(0).size(), send_buf);

	const char* _url = params->GetObjectConst(0)->get_string(0).c_str();

	const char *component = strchr(_url, '/');

	STRING url;
	url.Insert(0, _url, component - _url);

	int port = remove_port_from_url(url);

	STRING err_msg;
	string receive_buf;
	if (SendBufferToServer(url, port, component, send_buf, receive_buf, err_msg) == false)
		return false;

	hex_char_to_bin(receive_buf, params->GetObjectA(2)->get_barray_writable(0));

	return true;
}


// file_name may be empty and uninitialized, in that case a file name is created
// 
// write_file(STRING file_name, barray data)
bool EF_write_file(Params* params)
{
	// If the first parameter, file name has no data,
	// create a temporary file name and initialize the 
	// variable with that name.
	if (params->GetObjectConst(0)->get_data() == 0)
	{
		string file_name;
		if (make_unique_filename(file_name, "tmp") == false)
			return false;

		params->GetObjectA(0)->initialize_string();
		params->GetObjectA(0)->set_string(0, file_name.c_str());
	}

	const char* file_name = GET_STRING(params, 0);
	const barray* data = GET_BARRAY(params, 1);

	return WriteBufferToFile(file_name, *data);
}

// read_file(STRING file_name, barray data)
bool EF_read_file(Params* params)
{
	return ReadFileToBuffer(GET_STRING(params,0), *GET_BARRAY_WRITABLE(params,1));
}

// compress_file(file_in, file_out) 
// file_in - must exist and available to open for reading
// file_out - will be created if name is not specified
bool EF_compress_file(PtrSet<Value>* params)
{
#ifdef EXCLUDE_compress_file
	MessageBox(NULL, "compress_file() function is not supported with this version.", "Error", MB_ICONERROR);
	return true;
#else
	VERIFY_PARAM_COUNT(2);
	VERIFY_PARAM_IS_STRING(0);
	VERIFY_PARAM_IS_STRING(1);

	const char* file_in = params->GetObjectConst(0)->get_string(0).c_str();
	if (DoesFileExist(file_in) == false)
		return false;

	if (params->GetObjectConst(1)->get_data() == 0)
	{
		string file_name;
		if (make_unique_filename(file_name, file_in) == false)
			return false;

		params->GetObjectA(1)->initialize_string();
		params->GetObjectA(1)->set_string(0, file_name.c_str());
	}

	const char* file_out = params->GetObjectConst(1)->get_string(0).c_str();

	string s0, s1, s2;
	const char *exe = DecodeText("79EE607A41E31F33D2F71F866F01", s0)/*7z.exe*/;

	char dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);

	string s_format;
	DecodeText("F50FD0D33DBD19F0163DD59F39F70643FF9F", s_format)/*a "%s" "%s"*/;

	char s_params[1024];
	sprintf_s(s_params, sizeof(s_params), s_format.c_str(), file_out, file_in);

	STRING target_file;
	target_file.LoadFileName(file_in, TRUE, TRUE, FALSE);
	target_file += DecodeText("B5B6215AA633F6112FD59B2F11", s2)/*.cout*/;
	Zero(s2);

	STRING err_msg;
	if (RunProcess(exe, dir, s_params, target_file, err_msg) == false)
		return false;
	
	// Read the target file into memory and then delete the file
	vector<uint8_t> buf;
	if (ReadFileToBuffer(target_file, buf) == false)
		return false;

	if (DeleteFile(target_file) == false)
		return false;

	// Validate the contents of the target buffer
	int idx = find_in_buffer(DecodeText("93BE4C9C8E59F1139127F12BF65EF2F694D2CCE78BCCF4", s1)/*Archive size: */, buf);
	Zero(s1);

	if (idx < 0)
		return false;

	int archive_size;
	if (sscanf_s((const char*)&buf[idx + 14], "%ld", &archive_size) != 1)
		return false;

	idx = find_in_buffer(DecodeText("7CA45C7456F4BA6F9C90194691D3B858D2E2F7F68DC880F3", s1)/*Everything is Ok*/, buf);
	Zero(s1);

	if (idx < 0)
		return false;

	if (filelength(file_out) != archive_size)
		return false;

	return true;
#endif
}