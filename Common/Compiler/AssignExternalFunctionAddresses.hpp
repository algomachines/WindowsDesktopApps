#pragma once

void AssignExternalFunctionAddresses(RunCompiledCode& obj)
{
	// Add the addresses of the External Functions to be called
	// *****  Order here must match order when compiled
	obj.SetExtFunc(EF_barray_size);
	obj.SetExtFunc(EF_barray_is_equal);
	obj.SetExtFunc(EF_barray_resize);
	obj.SetExtFunc(EF_barray_assign);
	obj.SetExtFunc(EF_barray_cut);
	obj.SetExtFunc(EF_barray_insert);
	obj.SetExtFunc(EF_barray_append_byte);
	obj.SetExtFunc(EF_barray_append_uint32);
	obj.SetExtFunc(EF_barray_append_uint64);
	obj.SetExtFunc(EF_barray_append_hex_string);
	obj.SetExtFunc(EF_barray_append_string);
	obj.SetExtFunc(EF_string_append);
	obj.SetExtFunc(EF_string_from_barray);
	obj.SetExtFunc(EF_string_len);
	obj.SetExtFunc(EF_file_exists);
	obj.SetExtFunc(EF_hash_to_uint64);
	obj.SetExtFunc(EF_hash_string_to_uint64);
	obj.SetExtFunc(EF_uint32_array_assign);
	obj.SetExtFunc(EF_uint32_assign_hex_string);
	obj.SetExtFunc(EF_uint32_array_assign_u8);
	obj.SetExtFunc(EF_uint32_array_set_u8);
	obj.SetExtFunc(EF_barray_assign_u32);
	obj.SetExtFunc(EF_u32_assign_barray);
	obj.SetExtFunc(EF_uint64_array_assign_u8);
	obj.SetExtFunc(EF_log);
	obj.SetExtFunc(EF_exp);
	obj.SetExtFunc(EF_encrypt_string_to_barray);
	obj.SetExtFunc(EF_decrypt_barray_to_string);
	obj.SetExtFunc(EF_get_unix_time_e);
	obj.SetExtFunc(EF_get_time_ms);
	obj.SetExtFunc(EF_symmetric_encryption_e_uint64);
	obj.SetExtFunc(EF_symmetric_encryption);
	obj.SetExtFunc(EF_MurmurHash3_x64_128);
	obj.SetExtFunc(EF_MurmurHash3_x86_32);
	obj.SetExtFunc(EF_assign_uint64_from_barray);
	obj.SetExtFunc(EF_get_e_int);
	obj.SetExtFunc(EF_hash_64);
	obj.SetExtFunc(EF_random_64);
	obj.SetExtFunc(EF_hash_program_id);
	obj.SetExtFunc(EF_append_string_array_to_file);
	obj.SetExtFunc(EF_get_data_dir);
	obj.SetExtFunc(EF_generate_random_data);
	obj.SetExtFunc(EF_compress_file);
	obj.SetExtFunc(EF_get_file_size);
	obj.SetExtFunc(EF_delete_file);
	obj.SetExtFunc(EF_does_file_exist);
	obj.SetExtFunc(EF_read_file);
	obj.SetExtFunc(EF_write_file);
	obj.SetExtFunc(EF_send_data_to_server);
	
	obj.SetExtFunc(EF_prompt);
	obj.SetExtFunc(EF_to_stdout);

#ifdef _DEBUG
	// Debug functions - must be the last
	obj.SetExtFunc(EF_uint32_info);
	obj.SetExtFunc(EF_string_info);
	obj.SetExtFunc(EF_barray_info);
	obj.SetExtFunc(EF_double_info);
	obj.SetExtFunc(EF_uint64_info);
	obj.SetExtFunc(EF_int_info);
	obj.SetExtFunc(EF_barray_save_to_file);
#endif

	// Game functions
//obj.SetExtFunc(EF_GetSystemTimeMS);
//obj.SetExtFunc(EF_Sleep);
////obj.SetExtFunc(EF_ProcessSleepSession);
//obj.SetExtFunc(EF_ProcessKeyUpActions);
//obj.SetExtFunc(EF_IsKeyPushed);
//obj.SetExtFunc(EF_GetActionThreadStopRequest);
//obj.SetExtFunc(EF_GameOverValidation);
//obj.SetExtFunc(EF_GetNextVisibleInvaderName);
//obj.SetExtFunc(EF_ProcessInvaderWipeout);
//obj.SetExtFunc(EF_InvaderHitsBottom);
//obj.SetExtFunc(EF_ProcessInvaderReverseDirection);
//obj.SetExtFunc(EF_NextLevelValidation);
//obj.SetExtFunc(EF_SwitchCannonIfNecessary);
//obj.SetExtFunc(EF_ProcessCannonMovement);
//obj.SetExtFunc(EF_ProcessBulletActions);
//obj.SetExtFunc(EF_ProcessMotherShipActions);
//obj.SetExtFunc(EF_ProcessPaintActions);
//obj.SetExtFunc(EF_IsKeyPushed);
//obj.SetExtFunc(EF_SynchronizeGlobalClockCycle);
//obj.SetExtFunc(EF_GetCannonName);
//obj.SetExtFunc(EF_ValidateTime);
//obj.SetExtFunc(EF_SendScoreToServer);
}

#ifdef COMPILER
bool AddExternalFunctions(string& err_msg, bool add_game_functions/* = true*/)
{
	// Need to have an analog to this in the run only module - (BTCTimeLockDecryption)
	if (AddExtFunc(0, "barray_size(uint32,barray,uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_is_equal(barray,barray,int,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_resize(barray,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_assign(barray,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_cut(barray,int,int,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_insert(barray,barray,int,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_append_byte(barray,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_append_uint32(barray,uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_append_uint64(barray,uint64,int,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_append_hex_string(barray,STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_append_string(barray,STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "string_append(STRING,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "string_from_barray(STRING,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "string_len(STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "file_exists(STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "hash_to_uint64(uint64,barray,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "hash_string_to_uint64(uint64,STRING,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint32_array_assign(uint32,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint32_assign_hex_string(uint32,STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint32_array_assign_u8(uint32, barray, uint32, uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint32_array_set_u8(uint32, int, uint32, uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_asssign_u32(barray, uint32, uint32, uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "u32_asssign_barray(uint32,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint64_array_assign_u8(uint64, barray, uint32, uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "log(double,double)", err_msg) == false) return false;
	if (AddExtFunc(0, "exp(double,double)", err_msg) == false) return false;
	if (AddExtFunc(0, "encrypt_string_to_barray(uint64,STRING,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "decrypt_barray_to_string(uint64,barray,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "get_unix_time_e(barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "get_time_ms(uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "symmetric_encryption_e_uint64(barray,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "symmetric_encryption(barray,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "MurmurHash3_x64_128(barray,uint32,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "MurmurHash3_x86_32(barray,uint32,uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "assign_uint64_from_barray(uint64,barray,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "get_e_int(int,barray,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "hash_64(uint64,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "random_64(uint64,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "hash_program_id(barray,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "append_string_array_to_file(STRING,STRING,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "get_data_dir(STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "generate_random_data(barray,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "compress_file(STRING,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "get_file_size(STRING,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "delete_file(STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "does_file_exist(STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "read_file(STRING,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "write_file(STRING,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "send_data_to_server(STRING,barray,barray)", err_msg) == false) return false;

	// Info functions
	if (AddExtFunc(0, "prompt(STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "to_stdout(STRING)", err_msg) == false) return false;


#ifdef _DEBUG
	if (AddExtFunc(0, "uint32_info(STRING,uint32)", err_msg) == false) return false;
	if (AddExtFunc(0, "string_info(STRING,STRING)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_info(STRING,barray)", err_msg) == false) return false;
	if (AddExtFunc(0, "double_info(STRING,double)", err_msg) == false) return false;
	if (AddExtFunc(0, "uint64_info(STRING,uint64)", err_msg) == false) return false;
	if (AddExtFunc(0, "int_info(STRING,int)", err_msg) == false) return false;
	if (AddExtFunc(0, "barray_save_to_file(STRING,STRING,barray)", err_msg) == false) return false;
#endif

	if (add_game_functions)
	{
		// DRM Game 
		if (AddExtFunc(0, "GetSystemTimeMS(uint64)", err_msg) == false) return false;
		if (AddExtFunc(0, "Sleep(int)", err_msg) == false) return false;
		//if (AddExtFunc(0, "ProcessSleepSession(int,uint64,uint64,uint64,uint64)", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessKeyUpActions()", err_msg) == false) return false;
		if (AddExtFunc(0, "EF_IsKeyPushed(int,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "GetActionThreadStopRequest(int)", err_msg) == false) return false;
		if (AddExtFunc(0, "GameOverValidation(double,double,barray,barray,uint64)", err_msg) == false) return false;
		if (AddExtFunc(0, "GetNextVisibleInvaderName(int,STRING)", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessInvaderWipeout(STRING,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "InvaderHitsBottom(STRING,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessInvaderReverseDirection(STRING,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "NextLevelValidation(double,double)", err_msg) == false) return false;
		if (AddExtFunc(0, "SwitchCannonIfNecessary(STRING,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessCannonMovement(STRING,int,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessBulletActions()", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessMotherShipActions()", err_msg) == false) return false;
		if (AddExtFunc(0, "ProcessPaintActions(uint64)", err_msg) == false) return false;
		if (AddExtFunc(0, "IsKeyPushed(int,int)", err_msg) == false) return false;
		if (AddExtFunc(0, "SynchronizeGlobalClockCycle(uint64)", err_msg) == false) return false;
		if (AddExtFunc(0, "GetCannonName(STRING)", err_msg) == false) return false;
		if (AddExtFunc(0, "ValidateTime(barray, barray)", err_msg) == false) return false;
		if (AddExtFunc(0, "SendScoreToServer(uint64,uint64,uint64)", err_msg) == false) return false;
	}

	// BTC.hpp
#ifdef _DEBUG
	//if (AddExtFunc(0, "confirm_hash_meets_target(barray,uint64,uint64,double)", err_msg) == false) return false;
	//if (AddExtFunc(0, "BRSHA256(barray,barray)", err_msg) == false) return false;
	//if (AddExtFunc(0, "BRSHA256Compress(uint32,uint32)", err_msg) == false) return false;
#endif
	return true;
}
#endif

