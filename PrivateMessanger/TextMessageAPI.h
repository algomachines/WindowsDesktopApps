// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"

#pragma once

// These functions are threadsafe

namespace TextMessageAPI
{

	// The TextMessageAPI is meant to facilitate:
	// 1) Storage and Retrieval of Text Messages 
	// 2) Searching of Text Messages 
	// 3) Rendering of Text Messages


	// Create resources which will be used later
	bool Initialize(string& err_msg);

	HFONT GetTimestampFont(void);

	// Returns a handle to an open TextMessageThread if successful, otherwise returns 0
	// directory - directory where the TextMessageThread database for the specified SenderID resides
	// my_hashed_ID - hashed InstallID
	// hashed_SenderID - hashed 32 byte sender ID for the thread
	// pwd_hash - hash of the password which is used to decrypt the database - 16 byte
	// create_if_missing - create and empty TextMessageThread database if none is present at the given directory
	// nickname - human assigned name for this database/tread only used on creation
	// url - URL for the server associated with this database, required when creating the dtabase
	uint64_t Open(const char* directory, const uint8_t* my_hashed_id, const uint8_t* hashed_SenderID, const uint8_t* pwd_hash, string& err_msg, bool create_if_missing = false, const char* nickname = 0, const char* url = 0);

	// Change the pwd for all conversations with a password matching old_pwd_hash to new_pwd_hash
	// directory - directory where the TextMessageThread database 
	// my_hashed_id - my 32 byte hashed id
	// old_pwd_hash - hash of the password which is used to attempt to decrypt each conversation - 16 byte
	// new_pwd_hash - hash of the new password which is used encrypt each successfully decrypted conversation - 16 byte
	// ntotal - total number of conversations found
	// nupdated - number of conversations which were successfully updated
	bool UpdatePasswordForAllConversations(const char* directory, const uint8_t* my_hashed_id, const uint8_t* old_pwd_hash, const uint8_t* new_pwd_hash, int& ntotal, int& nupdated, string& err_msg);

	// Change the url for all conversations with a password matching pwd_hash 
	// directory - directory where the TextMessageThread database 
	// my_hashed_id - my 32 byte hashed id
	// pwd_hash - hash of the  password which is used encrypt and decrypted each conversation - 16 byte
	// url - url to assing to each conversation
	// ntotal - total number of conversations found
	// nupdated - number of conversations which were successfully updated
	bool UpdateURLForAllConversations(const char* directory, const uint8_t* my_hashed_id, const uint8_t* pwd_hash, const char* url, int& ntotal, int& nupdated, string& err_msg);

	// Change the url for the specified conversation
	// h - handle to a conversation, may be zero
	bool ChangeURL(uint64_t h, const char* url, string& err_msg);

	const char* GetNickname(uint64_t h, string& nickname);

	bool SetNickname(uint64_t h, const char* nickname);

	// Each conversation database includes an embedded URL
	// h - may be zero 
	bool GetURL(uint64_t h, string &URL, string &err_msg);

	// Each conversation database includes the hashed ID of the remote client that you are talking to
	// h - may be zero 
	bool GetHashedRemoteID(uint64_t h, vector<uint8_t> &hashed_id, string &err_msg);

	// Close all message threads which are open
	bool CloseAll(void);

	// Clear all messages from the database for this conversation
	// h - may be zero
	bool ClearHistory(uint64_t h, string& err_msg);

	// Returns true if there exists a thread database file for the given hashed_SenderID, otherwise returns false
	bool Exists(const char* directory, const uint8_t* hashed_SenderID);

	// h Is the specified handle valid, if h==0, are there any open conversations?
	bool IsOpen(uint64_t h = 0);

	bool Close(uint64_t h);

	// h - handle to the thread
	// sender - true if sender is being appended, false if receiver is being appended
	// text - ascii text of the message, may not be more than 256 characters
	bool Append(uint64_t h, bool sender, const char* text, string& err_msg);

	// h - handle to the thread
	// file_name - name of the encrypted message file
	// password_hash - password hash to decrypt the file
	// 
	// NOTE: it is assumed that the client is the receiver of this message
	bool AppendFromFile(uint64_t h, const char* file_name, const vector<uint8_t>& password_hash, string& err_msg);

	// Save the memory resident text message thread to file
	// pwd_hash - hash of the password which is used to encrypt the database - 16 byte
	// file_name - optional parameter, provided in case we need to reduce the file size (close, delete, save, reopen)
	bool Save(uint64_t h, const uint8_t* pwd_hash, string& err_msg, const char *file_name=0);

	// h - handle to the thread - may be zero
	// n - number of messages in the thread
	bool GetNumMessages(uint64_t h, uint32_t& n);

	// Render the text message thread starting at the specified message index
	// h - handle to the thread - may be zero
	// idx_top - top message index, returned value may be different if input value is invalid
	// idx_bottom - bottom message index on ouput, input value is ignored
	// wnd_rect - client window area
	// hdc - device context handle
	// unread_messages - on input, true if there are new messages, on ouput true if the new messages have not been rendered
	bool RenderByIndexAtTop(uint64_t h, uint32_t &idx_top, uint32_t &idx_bottom, const RECT& wnd_rect, HDC hdc, bool& unread_messages, string& err_msg);

	// Render the text message thread starting at the specified message index
	// h - handle to the thread - may be zero
	// idx_top - not used on input, on output this is the top line rendered
	// idx_bottom - index at the bottom after rendering, set to 0xFFFFFFFF to render the last index at the bottom, on return this is set to the last line rendered
	// wnd_rect - client window area
	// hdc - device context handle
	bool RenderByIndexAtBottom(uint64_t h, uint32_t& idx_top, uint32_t& idx_bottom, const RECT& wnd_rect, HDC hdc, string& err_msg);

	// Render the text message thread starting at the specified message time (unix time in ms)
	// h - handle to the thread
	// t_ms - ms since Jan 1, 1970 00:00:00.000
	// wnd_rect - client window area
	// hdc - device context handle
	bool RenderByTimestamp(uint64_t h, uint64_t t_ms, const RECT& wnd_rect, HDC hdc);

	// Returns false if there is no message text at this point
	// h - handle to the thread, may be zero
	// pt - point in the client window rectangle where the message was rendered
	// msg_txt - text of the message at the specified point, only includes text from a single line rendered to the screen
	bool GetMessageTextAtPoint(uint64_t h, POINT pt, string& msg_txt);

	// Copy the text message history for the given handle to the Windows clipboard
	// h - handle to the thread, may be zero
	// hWnd - windows handle of the owner of the clipboard
	bool CopyHistory(uint64_t h, HWND hWnd, string& err_msg);
};
