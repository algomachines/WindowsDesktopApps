// Copyright (c) 2021 AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"
#include "Encryption.h"
#include "TextMessageAPI.h"
#include "TextMessageThread.h"
#include "SemaphoreObject.h"
#include "Clipboard.h"

namespace TextMessageAPI
{

	HFONT message_hfont = 0;
	HFONT timestamp_hfont = 0;

	COLORREF local_text_color = RGB(255, 255, 255);
	COLORREF remote_text_color = RGB(0, 0, 0);

	COLORREF timestamp_text_color = RGB(128, 128, 128);

	COLORREF local_bg_color = RGB(50, 130, 50);    // green
	COLORREF remote_bg_color = RGB(200, 200, 200); // light gray

	HBRUSH local_bg_hbrush = 0;
	HBRUSH remote_bg_hbrush = 0;

	static HANDLE s_semaphore = 0;

	const int TOP_MARGIN = 5; // top margin of rendering in the text message window

	class RenderedTextObj
	{
	public:
		inline RenderedTextObj(void)
		{
			ZERO(m_rect);
		}

		string m_text;
		RECT m_rect;
	};

	vector<RenderedTextObj> s_rendered_text;


#define GET_MESSAGE_THREAD_HANDLE_OBJ\
	if (s_message_threads.GetCount() == 0)\
	{\
		ERROR_LOCATION(err_msg);\
		err_msg += "no active message threads";\
		return false;\
	}\
		\
	if (h == 0 && s_message_threads.GetCount() != 1)\
	{\
		ERROR_LOCATION(err_msg);\
		err_msg += "expected number of active message threads to be 1";\
		return false;\
	}\
		\
	MessageThreadHandle* h_obj = 0;\
		\
	if (h == 0)\
	{\
		h_obj = s_message_threads.GetObjectA(0);\
	}\
	else\
	{\
		MessageThreadHandle token;\
		token.m_h = h;\
		\
		BOOL exists;\
		h_obj = s_message_threads.GetPtrByToken(&token, exists);\
		\
		if (exists == FALSE)\
		{\
			ERROR_LOCATION(err_msg);\
			err_msg += "No message thread associated with given handle";\
			return false;\
		}\
	}

	inline void CreateResourecesIfNecessary(void)
	{
		if (message_hfont)
			return;

		int ht = 20;
		int wt = 0;
		message_hfont = CreateFont(ht, wt, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Arial");

		ht = 12;
		timestamp_hfont = CreateFont(ht, wt, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Arial");

		local_bg_hbrush = CreateSolidBrush(local_bg_color);
		remote_bg_hbrush = CreateSolidBrush(remote_bg_color);
	}

	bool Initialize(string &err_msg)
	{
		CreateResourecesIfNecessary();
		return true;
	}

	HFONT GetTimestampFont(void)
	{
		return timestamp_hfont;
	}

	class MessageThreadHandle
	{
	public:

		inline MessageThreadHandle(void)
		{
			m_obj = 0;
			m_h = 0;
			m_stream = 0;
		}

		inline bool operator < (const MessageThreadHandle& obj) const
		{
			return m_h < obj.m_h;
		}

		inline bool operator > (const MessageThreadHandle& obj) const
		{
			return m_h > obj.m_h;
		}

		MessageThread* m_obj;
		FILE* m_stream;

		uint64_t m_h;

		vector<uint8_t> m_pwd_hash;

		static uint64_t m_next_h;

		inline void SetPwdHash(const uint8_t* pwd_hash)
		{
			m_pwd_hash.resize(32);
			memmove(&m_pwd_hash[0], pwd_hash, 32);
		}

		inline bool Close(void)
		{
			if (m_stream)
			{
				fclose(m_stream);
				m_stream = 0;
			}

			if (m_obj)
			{
				delete m_obj;
				m_obj = 0;
			}

			Zero(m_pwd_hash);

			return true;
		}
	};

	uint64_t MessageThreadHandle::m_next_h = 1;

	PtrSet<MessageThreadHandle> s_message_threads;

	// Returns a handle to an open TextMessageThread if successful, otherwise returns 0
	// directory - directory where the TextMessageThread database for the specified SenderID resides
	// my_hashed_id - my 32 byte hashed id
	// hashed_SenderID - hashed 32 byte sender ID for the thread
	// pwd_hash - hash of the password which is used to decrypt the database - 16 byte
	// create_if_missing - create and empty TextMessageThread database if none is present at the given directory
	// nickname - human assigned name for this database/thread only used on creation
	// url - URL for the server associated with this database, required when creating the database
	uint64_t Open(const char* directory, const uint8_t* my_hashed_id, const uint8_t* hashed_SenderID, const uint8_t* pwd_hash, string& err_msg, bool create_if_missing/*=false*/, const char* nickname/*=0*/, const char* url/*=0*/)
	{
		MessageThreadHandle* h_obj = 0;

		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		if (DoesFileExist(directory) == false)
		{
			if (create_if_missing == false)
			{
				err_msg = "Directory does not exist: ";
				err_msg += directory;
				return 0;
			}

			if (_mkdir(directory))
			{
				err_msg = "Unable to create directory: ";
				err_msg += directory;
				return 0;
			}
		}

		string fname;
		bin_to_ascii_char(hashed_SenderID, 32, fname);

		string file_name;
		file_name = directory;
		file_name += "\\";
		file_name += fname;


		FILE* stream = 0;
		if (DoesFileExist(file_name.c_str()) == false)
		{
			if (create_if_missing == false)
			{
				err_msg = "File does not exist: ";
				err_msg += file_name;
				return 0;
			}
			else
			{
				fopen_s(&stream, file_name.c_str(), "w+b");

				if (stream == 0)
				{
					err_msg = "Unable to create file: ";
					err_msg += file_name;
					return 0;
				}
			}
		}
		else
		{
			// backup the file before 
			fopen_s(&stream, file_name.c_str(), "r+b");
			if (stream == 0)
			{
				err_msg = "Unable to open file: ";
				err_msg += file_name;

				return 0;
			}
		}

		int sz = filelength(file_name.c_str());
		MessageThread* obj = new MessageThread;
		if (sz)
		{
			uint8_t* buf = new uint8_t[sz];

			int nread = fread(buf, 1, sz, stream);
			if (nread != sz)
			{
				delete[] buf;
				fclose(stream);
				stream = 0;
				err_msg = "Problem reading file: ";
				err_msg += file_name;
				return 0;
			}

			uint32_t thash = 0;
			uint32_t hash = 0;
			uint32_t p_hash = 0;

			MurmurHash3_x86_32(buf, sz, 0, &hash);
			MurmurHash3_x86_32(pwd_hash, 32, 0, &p_hash);

			// Decrypt the buffer
			symmetric_encryption(buf, sz, pwd_hash, 32);

			MurmurHash3_x86_32(buf, sz, 0, &thash);

			uint64_t check = 0;
			if (memcmp(buf, &check, sizeof(check)))
			{
				delete[] buf;
				delete obj;
				fclose(stream);
				stream = 0;
				err_msg = "Invalid password";
				return 0;
			}

			uint32_t n = obj->LoadFromBuffer(buf);
			//if (n != sz)
			//{
			//	delete[] buf;
			//	delete obj;
			//	fclose(stream);
			//	stream = 0;
			//	err_msg = "Problem loading text message db from file: ";
			//	err_msg += file_name;
			//	return 0;
			//}

			while (n == sz)
			{
				// Backup the file

				// first re-encrypt
				symmetric_encryption(buf, sz, pwd_hash, 32);

				STRING backup_file;
				backup_file = directory;
				backup_file.AppendSubDir("backups");
				if (DoesFileExist(backup_file) == false)
				{
					if (_mkdir(backup_file))
						break; // failed to make the backup directory
				}

				backup_file.AppendSubDir(fname.c_str());

				FILE* bk_stream = 0;
				fopen_s(&bk_stream, backup_file, "wb");

				if (bk_stream == 0)
					break;

				fwrite(&buf[0], 1, sz, bk_stream);

				fclose(bk_stream);

				break;
			}

			delete[] buf;
		}
		else
		{
			// New database, need to do some initialization
			if (nickname)
				obj->SetNickname(nickname);

			obj->Set_hashed_LocalID(my_hashed_id);
			obj->Set_hashed_RemoteID(hashed_SenderID);
			obj->SetURL(url);
		}

		h_obj = new MessageThreadHandle;
		h_obj->m_h = MessageThreadHandle::m_next_h++;

		if (s_message_threads.Exists(h_obj))
		{
			delete obj;
			delete h_obj;
			fclose(stream);
			__debugbreak();
			return 0; // should not happen
		}

		h_obj->m_stream = stream;
		h_obj->m_obj = obj;

		s_message_threads.Append(h_obj);

		h_obj->SetPwdHash(pwd_hash);

		return h_obj->m_h;
	}

	// Change the pwd for all conversations with a password matching old_pwd_hash to new_pwd_hash
	// directory - directory where the TextMessageThread database 
	// my_hashed_id - my 32 byte hashed id
	// old_pwd_hash - hash of the password which is used to attempt to decrypt each conversation - 16 byte
	// new_pwd_hash - hash of the new password which is used encrypt each successfully decrypted conversation - 16 byte
	// ntotal - total number of conversations found
	// nupdated - number of conversations which were successfully updated
	bool UpdatePasswordForAllConversations(const char* directory, const uint8_t* my_hashed_id, const uint8_t* old_pwd_hash, const uint8_t* new_pwd_hash, int& ntotal, int& nupdated, string& err_msg)
	{
		ntotal = 0;
		nupdated = 0;

		CloseAll(); // First close all open conversations

		STRING fspec;
		fspec.SetValidFileName(directory, "*");

		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(fspec, &data);

		while (h != INVALID_HANDLE_VALUE)
		{
			if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			vector<uint8_t> hashed_sender_id;
			ascii_char_to_bin(data.cFileName, hashed_sender_id);

			if (hashed_sender_id.size() != 32)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			ntotal++;

			uint64_t h_conv = Open(directory, my_hashed_id, &hashed_sender_id[0], old_pwd_hash, err_msg);
			if (h_conv == 0)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			if (Save(h_conv, new_pwd_hash, err_msg) == false)
			{
				Close(h_conv);

				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			nupdated++;

			if (FindNextFile(h, &data) == false)
				break;
		}

		FindClose(h);

		return true;
	}

	bool CloseAll(void)
	{
		string err_msg;
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		for (size_t i = 0; i < s_message_threads.size(); i++)
			s_message_threads[i].Close();

		s_message_threads.RemoveAll();

		return true;
	}

	bool Close(uint64_t h)
	{
		string err_msg;
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		for (size_t i = 0; i < s_message_threads.size(); i++)
		{
			if (s_message_threads[i].m_h == h)
			{
				s_message_threads[i].Close();
				s_message_threads.Remove(i);
				return true;
			}
		}

		return false;
	}

	// Change the url for all conversations with a password matching pwd_hash 
	// directory - directory where the TextMessageThread database 
	// my_hashed_id - my 32 byte hashed id
	// pwd_hash - hash of the  password which is used encrypt and decrypted each conversation - 16 byte
	// url - url to assing to each conversation
	// ntotal - total number of conversations found
	// nupdated - number of conversations which were successfully updated
	bool UpdateURLForAllConversations(const char* directory, const uint8_t* my_hashed_id, const uint8_t* pwd_hash, const char* url, int& ntotal, int& nupdated, string& err_msg)
	{
		ntotal = 0;
		nupdated = 0;

		CloseAll(); // First close all open conversations

		STRING fspec;
		fspec.SetValidFileName(directory, "*");

		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(fspec, &data);

		while (h != INVALID_HANDLE_VALUE)
		{
			if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			vector<uint8_t> hashed_sender_id;
			ascii_char_to_bin(data.cFileName, hashed_sender_id);

			if (hashed_sender_id.size() != 32)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			ntotal++;

			uint64_t h_conv = Open(directory, my_hashed_id, &hashed_sender_id[0], pwd_hash, err_msg);
			if (h_conv == 0)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			if (ChangeURL(h_conv, url, err_msg) == false)
			{
				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			if (Save(h_conv, pwd_hash, err_msg) == false)
			{
				Close(h_conv);

				if (FindNextFile(h, &data) == false)
					break;

				continue;
			}

			nupdated++;

			if (FindNextFile(h, &data) == false)
				break;
		}

		FindClose(h);

		return true;
	}

	bool ChangeURL(uint64_t h, const char* url, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		h_obj->m_obj->SetURL(url);

		return true;
	}

	// Returns true if there exists a thread database file for the given hashed_SenderID, otherwise returns false
	bool Exists(const char* directory, const uint8_t* hashed_SenderID)
	{
		string fname;
		bin_to_ascii_char(hashed_SenderID, 32, fname);

		STRING file_name;
		file_name.SetValidFileName(directory, fname.c_str());
		return DoesFileExist(file_name);
	}

	// Is the specified handle valid, if h==0, are there any open conversations?
	bool IsOpen(uint64_t h/*=0*/)
	{
		string err_msg;

		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return false;

		return true;
	}


	bool GetNumMessages(uint64_t h, uint32_t &n)
	{
		string err_msg;

		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		if (s_message_threads.size() == 0)
			return false;

		// If h==0 take the first message thread 
		if (h == 0)
			h = s_message_threads[0].m_h;

		MessageThreadHandle token;
		token.m_h = h;

		BOOL exists;
		MessageThreadHandle* h_obj = s_message_threads.GetPtrByToken(&token, exists);

		if (exists == FALSE)
			return false;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return false;

		n = msg_thread->GetNumMessages();

		return true;
	}

	const char* GetNickname(uint64_t h, string &nickname)
	{
		string err_msg;

		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return 0;

		if (s_message_threads.size() == 0)
			return 0;

		// If h==0 take the first message thread 
		if (h == 0)
			h = s_message_threads[0].m_h;

		MessageThreadHandle token;
		token.m_h = h;

		BOOL exists;
		MessageThreadHandle* h_obj = s_message_threads.GetPtrByToken(&token, exists);

		if (exists == FALSE)
			return 0;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return 0;

		nickname = msg_thread->GetNickname();

		return nickname.c_str();
	}

	bool SetNickname(uint64_t h, const char *nickname)
	{
		string err_msg;

		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		if (s_message_threads.size() == 0)
			return false;

		// If h==0 take the first message thread 
		if (h == 0)
			h = s_message_threads[0].m_h;

		MessageThreadHandle token;
		token.m_h = h;

		BOOL exists;
		MessageThreadHandle* h_obj = s_message_threads.GetPtrByToken(&token, exists);

		if (exists == FALSE)
			return false;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return false;

		msg_thread->SetNickname(nickname);

		return true;
	}

	bool GetURL(uint64_t h, string &URL, string &err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return false;

		URL = msg_thread->GetURL();
		return true;
	}

	bool ClearHistory(uint64_t h, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return false;

		return msg_thread->ClearHistory(err_msg);
	}

	bool GetHashedRemoteID(uint64_t h, vector<uint8_t> &hashed_id, string &err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		if (s_message_threads.size() == 0)
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		if (!msg_thread)
			return 0;

		hashed_id.resize(32);
		memmove(&hashed_id[0], msg_thread->Get_hashed_RemoteID(), 32);
		return &hashed_id[0];
	}

	// h - handle to the thread
	// sender - true if sender is being appended, false if receiver is being appended
	// text - ascii text of the message, may not be more than 256 characters
	bool Append(uint64_t h, bool sender, const char* text, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		if (h_obj->m_obj->AppendTextMessage(sender, text, err_msg) == false)
			return false;

		return true;
	}


	// h - handle to the thread - may be zero 
	// NOTE: it is assumed that the client is the receiver of this message
	bool AppendFromFile(uint64_t h, const char* file_name, const vector<uint8_t>& password_hash, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		int len = filelength(file_name);

		FILE* stream = 0;
		fopen_s(&stream, file_name, "rb");

		if (!stream)
		{
			ERROR_LOCATION(err_msg);
			err_msg += "Can't open file for reading: ";
			err_msg += file_name;
			return false;
		}

		vector<uint8_t> buf;
		buf.resize(len);

		if (fread(&buf[0], 1, buf.size(), stream) != buf.size())
		{
			ERROR_LOCATION(err_msg);
			err_msg += "Problem reading from file: ";
			err_msg += file_name;
			return false;
		}

		fclose(stream);

		// decrypt
		if (password_hash.size())
			symmetric_encryption(&buf[0], buf.size(), &password_hash[0], password_hash.size());

		int i = 0;

		uint64_t t_latest_ms = 0;

		while ((i + sizeof(uint64_t)) < buf.size())
		{
			uint64_t timestamp_ms = 0;
			memmove(&timestamp_ms, &buf[i], sizeof(timestamp_ms));

			uint64_t current_time = get_time_ms();

			if (timestamp_ms > current_time)
				timestamp_ms = current_time;

			if (t_latest_ms < timestamp_ms)
				t_latest_ms = timestamp_ms;

			if (timestamp_ms <= t_latest_ms)
				timestamp_ms = ++t_latest_ms; // make sure that timestamps are in increasing order

			len -= sizeof(timestamp_ms);

			i += sizeof(timestamp_ms);

			if (i >= buf.size())
				break;

			const char* msg = (const char*)&buf[i];

			int len = StrLen(msg);

			if (len > 256)
				break;

			i += len + 1;

			if (i > buf.size())
				break;

			bool sender = false;
			bool status = h_obj->m_obj->AppendTextMessage(sender, msg, err_msg, &timestamp_ms);

			if (status == false)
				return false;

			if (i == buf.size())
				break;
		}

		return true;
	}

	// Save the memory resident text message thread to file
	// pwd_hash - hash of the password which is used to encrypt the database - 16 byte
	// file_name - optional parameter, provided in case we need to reduce the file size (close, delete, save, reopen)
	bool Save(uint64_t h, const uint8_t* pwd_hash, string& err_msg, const char *file_name/*=0*/)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		if (pwd_hash == 0)
		{
			ASSERT(h_obj->m_pwd_hash.size() == 32);
			pwd_hash = (const uint8_t *)&h_obj->m_pwd_hash[0];
		}

		MessageThread* msg_thread = h_obj->m_obj;
		uint32_t sz = msg_thread->GetStorageSizeBytes();

		uint32_t hash[6], thash[6], p_hash[5];
		ZERO(hash);
		ZERO(p_hash);
		ZERO(thash);

		uint8_t* buf = new uint8_t[sz];
		ZeroMemory(buf, sz);

		uint32_t check = msg_thread->SaveToBuffer(buf);
		ASSERT(check == sz);

		int flen = _filelength(_fileno(h_obj->m_stream));
		if (flen > sz && file_name) // zero out the remainder of the file
		{
			fclose(h_obj->m_stream);

			if (DeleteFile(file_name) == false)
			{
				ERROR_LOCATION(err_msg);
				err_msg += "Problem trying to delete file: ";
				err_msg += file_name;
				return false;
			}

			fopen_s(&h_obj->m_stream, file_name, "wb");
			if (h_obj->m_stream == 0)
			{
				ERROR_LOCATION(err_msg);
				err_msg += "Problem trying to create file: ";
				err_msg += file_name;
				return false;
			}

			fclose(h_obj->m_stream);

			fopen_s(&h_obj->m_stream, file_name, "r+b"); // open zero size file
			if (h_obj->m_stream == 0)
			{
				ERROR_LOCATION(err_msg);
				err_msg += "Problem trying to open file for writing: ";
				err_msg += file_name;
				return false;
			}
		}

		MurmurHash3_x86_32(buf, sz, 0, &thash[0]);

		// Encrypt the buffer
		symmetric_encryption(buf, sz, pwd_hash, 32);

		MurmurHash3_x86_32(buf, sz, 0, &hash[0]);
		MurmurHash3_x86_32(pwd_hash, 32, 0, &p_hash[0]);

		int status = fseek(h_obj->m_stream, 0, SEEK_SET);

		int nwritten = fwrite(buf, 1, sz, h_obj->m_stream);

		if (nwritten != sz)
		{
			delete[] buf;
			char msg[256];
			sprintf_s(msg, sizeof(msg), "%s() : Problem writing to file associated with handle: %llu", __FUNCTION__, h);
			err_msg = msg;
			return false;
		}

#if 0
		// Check that we can readback and decrypt
		{
			fclose(h_obj->m_stream);
			h_obj->m_stream = 0;

			FILE* stream = 0;

			string fname;
			bin_to_ascii_char(h_obj->m_obj->Get_hashed_RemoteID(), 32, fname);

			const char* file_name = "D:\\AlgoMachines\\DOC\\Demo4_PrivateMessage\\MSG_DB\\ZfW7tezIRgvu4YvplPHQDBFYKVnHhw.xwQi6O~ea1k8";

			fopen_s(&stream, file_name, "r+b");
			ASSERT(stream);

			ASSERT(fread(buf, 1, sz, stream) == sz);

			MurmurHash3_x86_32(buf, sz, 0, &hash[2]); // encrypted hash
			MurmurHash3_x86_32(pwd_hash, 32, 0, &p_hash[2]); // password hash

			// Decrypt
			symmetric_encryption(buf, sz, pwd_hash, 32);

			MurmurHash3_x86_32(buf, sz, 0, &thash[3]); // decrypted hash
			MurmurHash3_x86_32(pwd_hash, 32, 0, &p_hash[3]); // password hash

			fclose(stream);
		}
#endif

		delete[] buf;

		// Flush written output to the file
		fflush(h_obj->m_stream);
		status =_commit(_fileno(h_obj->m_stream));
		DWORD err = GetLastError();

		return true;
	}

	// returns true if some part of the timestamp fits in the client rect, otherwise false
	// timestamp_ms - unix time (UTC) in ms for the message
	// sender - sender if true (left side) otherwise receiver (right side)
	// client_rect - the client area where drawing must take place
	// s_timestamp - timestamp text from timestamp_ms
	// rect_timestamp - rectangle of drawing, within the client_rect
	inline bool draw_timestamp(uint64_t timestamp_ms, bool sender, const RECT& client_rect, int iy_offset, HDC hdc, string& s_timestamp, RECT& rect_timestamp, bool suppress_rendering=false)
	{
		SetTextColor(hdc, timestamp_text_color);
		SelectObject(hdc, timestamp_hfont);

		const int margin = TOP_MARGIN;	// gap on the left and right side
		int width = (client_rect.right - client_rect.left);
		width -= margin * 2;

		get_local_time_as_string(timestamp_ms, s_timestamp);

		int nchar_fit_time;
		SIZE sz_time;
		GetTextExtentExPointA(hdc, s_timestamp.c_str(), s_timestamp.size(), width, &nchar_fit_time, 0, &sz_time);

		const int pad = sz_time.cy / 5;

		if (sender) // draw on the right side
		{
			rect_timestamp.right = client_rect.right - margin;
			rect_timestamp.left = rect_timestamp.right - sz_time.cx;

		}
		else // draw on the left side
		{
			rect_timestamp.left = client_rect.left + margin;
			rect_timestamp.right = rect_timestamp.left + sz_time.cx;
		}

		rect_timestamp.top = client_rect.top + margin + iy_offset + pad;
		rect_timestamp.bottom = rect_timestamp.top + sz_time.cy;

		if (has_overlap(rect_timestamp, client_rect) == false)
			return false;

		//const int ellipse = margin;
		//RoundRect(hdc, rect_timestamp.left, rect_timestamp.top, rect_timestamp.right, rect_timestamp.bottom, ellipse, ellipse);

		if (suppress_rendering == false)
			DrawTextEx(hdc, &s_timestamp[0], s_timestamp.size(), &rect_timestamp, DT_CENTER | DT_SINGLELINE | DT_BOTTOM, NULL);

		return true;
	}

	inline bool draw_message(const string& msg, bool sender, const RECT& client_rect, int iy_offset, HDC hdc, RECT& rect_msg, bool supress_rendering=false)
	{
		if (sender)
		{
			SetTextColor(hdc, local_text_color);
			SelectObject(hdc, local_bg_hbrush);
		}
		else
		{
			SetTextColor(hdc, remote_text_color);
			SelectObject(hdc, remote_bg_hbrush);

		}

		SelectObject(hdc, message_hfont);

		const int margin = 5;	// gap between the two sides and on the left and right
		int width = (client_rect.right - client_rect.left);
		width -= margin * 2;

		const int ellipse = margin;

		STRING s_line = msg.c_str();
		s_line.Replace("\r", "");

		int top = iy_offset + client_rect.top;

		{
			int nchar_fit = 0;
			vector<int> dx;
			dx.resize(msg.size());
			SIZE sz;

			vector<RECT> text_rect_per_line_break;
			vector<STRING> text_per_line_break;

			int iline_break = 0;
			int pad = 0;

			bool bDone = false;

			while (1)
			{
				int len = StrLen(s_line);
				GetTextExtentExPointA(hdc, s_line, len, width, &nchar_fit, &dx[0], &sz);

				if (pad == 0)
					pad = sz.cy / 5; // pad all the sides around the text

				// Check for a line break in the characters which fit
				STRING s_to_render = s_line;

				for (int ibreak = 0; ibreak < nchar_fit; ibreak++)
				{
					if (s_line[ibreak] == '\n')
					{
						s_to_render = "";
						if (ibreak)
							s_to_render.Insert(0, s_line, ibreak);
						else
							s_to_render = " ";

						s_line.Remove(0, ibreak+1); // remove the charcters which are about to be rendered from this buffer
						len = StrLen(s_to_render);

						int nfit;
						GetTextExtentExPointA(hdc, s_to_render, len, width, &nfit, &dx[0], &sz);

						goto ExpandRetangle;
					}
				}

				if (len > nchar_fit)
				{
					// look for a break point
					int ibreak = nchar_fit - 1;
					while (ibreak >= nchar_fit / 2)
					{
						if (s_line[ibreak] == ' ')
							break;
							
						ibreak--;
					}

					s_to_render = "";
					s_to_render.Insert(0, s_line, ibreak);
					s_line.Remove(0, ibreak); // remove the charcters which are about to be rendered from this buffer
					int i = 0;
					while (s_line[i++] == ' ') s_line.Remove(0, 1); // remove leading spaces
					len = StrLen(s_to_render);
					int nfit;
					GetTextExtentExPointA(hdc, s_to_render, len, width, &nfit, &dx[0], &sz);
				}
				else
					bDone = true;

ExpandRetangle:

				// expand the rectangle
				if (sender) // left side
				{
					if (iline_break == 0)
					{
						rect_msg.right = client_rect.right - margin;
						rect_msg.left = rect_msg.right - sz.cx - pad * 2;
					}
					else
					{
						rect_msg.left = min(rect_msg.left, rect_msg.right - sz.cx - pad * 2);
					}
				}
				else // left side
				{
					if (iline_break == 0)
					{
						rect_msg.left = client_rect.left + margin;
						rect_msg.right = rect_msg.left + sz.cx + pad * 2;
					}
					else
					{
						rect_msg.right = max(rect_msg.right, rect_msg.left + sz.cx + pad * 2);
					}
				}

				if (iline_break == 0)
				{
					rect_msg.top = top;
					rect_msg.bottom = rect_msg.top + sz.cy + pad * 2;
				}
				else
				{
					rect_msg.bottom += sz.cy;
				}

				top = rect_msg.bottom + 1;

				RECT text_render_rect = rect_msg;
				text_render_rect.left += pad;
				text_render_rect.right -= pad;
				text_render_rect.bottom = rect_msg.bottom;
				text_render_rect.top = rect_msg.bottom - sz.cy;

				text_rect_per_line_break.push_back(text_render_rect);
				text_per_line_break.push_back(s_to_render);

				if (bDone)
					break;

				iline_break++;
			}

			rect_msg.bottom += pad;

			if (supress_rendering == false)
			{
				RoundRect(hdc, rect_msg.left, rect_msg.top, rect_msg.right, rect_msg.bottom, ellipse, ellipse);

				for (int i = 0; i < text_rect_per_line_break.size(); i++)
				{
					RECT rect = text_rect_per_line_break[i];
					rect.left = rect_msg.left + pad;
					rect.right = rect_msg.right - pad;
					DrawTextEx(hdc, text_per_line_break[i].GetStringMutable(), -1, &rect, DT_SINGLELINE | DT_VCENTER, NULL);
				}
			}

			RenderedTextObj obj;
			obj.m_rect = rect_msg;
			obj.m_text = s_line.GetStringConst();

			s_rendered_text.push_back(obj);
		}

		return true;
	}

	// Not thread save, only call if semaphore protections is already taken care of above this level
	bool RenderByIndexAtTop_unprotected(uint64_t h, uint32_t idx_top, uint32_t &idx_bottom, const RECT& wnd_rect, HDC hdc, bool &unread_messages_appended, string& err_msg)
	{
		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		CreateResourecesIfNecessary();

		SetBkMode(hdc, TRANSPARENT);

		int iy_offset = TOP_MARGIN; // top margin

		s_rendered_text.resize(0);

		uint32_t idx = idx_top;

		while (1)
		{
			string message;
			uint64_t timestamp_ms;
			bool sender;
			if (msg_thread->GetMessageByIndex(idx, message, timestamp_ms, sender) == false)
				break;

			RECT rect_timestamp;
			string s_timestamp;
			if (draw_timestamp(timestamp_ms, sender, wnd_rect, iy_offset, hdc, s_timestamp, rect_timestamp) == false)
				break;

			iy_offset = rect_timestamp.bottom + 1;

			RECT rect_msg;
			if (draw_message(message, sender, wnd_rect, iy_offset, hdc, rect_msg) == false)
				break;

			iy_offset = rect_msg.bottom + 1;

			if (iy_offset >= wnd_rect.bottom)
				break;

			idx++;
		}

		idx_bottom = idx;

		if (unread_messages_appended && idx_bottom < msg_thread->GetNumMessages())
		{
			// Draw a solid red circle in the UL corner of the client area if there are unread messages
			int diam = 17;
			int left = (width(wnd_rect)-diam) / 2;
			int right = left + diam;
			int top = diam/5;
			int bottom = top + diam;

			HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
			SelectObject(hdc, hbrush);
			HPEN hpen = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
			SelectObject(hdc, hpen);

			Ellipse(hdc, left, top, right, bottom);

			DeleteObject(hbrush);
			DeleteObject(hpen);
		}
		else
		{
			unread_messages_appended = false; // clear the flag
		}

		return true;
	}

	// Render the text message thread starting at the specified message index
	// h - handle to the thread - may be zero
	// idx - top message index
	// wnd_rect - client window area
	// hdc - device context handle
	//bool RenderByIndexAtTop(uint64_t h, uint32_t idx, const RECT& wnd_rect, HDC hdc, string& err_msg)
	//{
	//	SemaphoreObject sem_obj(s_semaphore, 2000);
	//	if (sem_obj.Failed(err_msg))
	//		return false;

	//	uint32_t top_idx = idx;
	//	uint32_t bottom_idx = 0;
	//	return RenderByIndexAtTop_unprotected(h, top_idx, bottom_idx, wnd_rect, hdc, err_msg);
	//}

	inline int get_render_height_of_message(const string& msg, bool sender, uint64_t timestamp_ms, const RECT& client_rect, HDC hdc)
	{
		int iy_offset = 0;
		RECT rect_msg, rect_timestamp;
		bool suppress_rendering = true;

		string s_timestamp;
		draw_timestamp(timestamp_ms, sender, client_rect, iy_offset, hdc, s_timestamp, rect_timestamp, suppress_rendering);

		iy_offset = rect_timestamp.bottom + 1;

		draw_message(msg, sender, client_rect, iy_offset, hdc, rect_msg, suppress_rendering);

		iy_offset = rect_msg.bottom + 1;

		return iy_offset;
	}


	inline uint32_t compute_idx_top_given_idx_bottom(const MessageThread* msg_thread, uint32_t idx_bottom, const RECT& wnd_rect, HDC hdc)
	{
		int ht_total = height(wnd_rect) - TOP_MARGIN * 2;
		int idx;
		for (idx = idx_bottom; idx >= 0; idx--)
		{
			string msg;
			uint64_t timestamp_ms;
			bool sender;

			msg_thread->GetMessageByIndex(idx, msg, timestamp_ms, sender);

			int ht = get_render_height_of_message(msg, sender, timestamp_ms, wnd_rect, hdc);
			
			ht_total -= ht;
			if (ht_total < 0)
			{
				if (idx != idx_bottom)
					idx++;

				break;
			}
		}

		if (idx == -1)
			idx = 0;

		return idx;
	}

#if 0
	uint32_t compute_idx_top_given_idx_bottom(const MessageThread* msg_thread, uint32_t idx_bottom, const RECT& wnd_rect, HDC hdc)
	{
		uint32_t idx = idx_bottom - 1;
		uint32_t idx_top = idx;

		// compute available height
		int height = wnd_rect.bottom - wnd_rect.top;
		int width = wnd_rect.right - wnd_rect.left;

		SelectObject(hdc, message_hfont);
		int nchar_fit = 0;
		int dx;
		SIZE sz;
		GetTextExtentExPointA(hdc, "A", 1, width, &nchar_fit, &dx, &sz);

		int height_txt_line = sz.cy + 2 * (sz.cy / 5) + TOP_MARGIN;

		SelectObject(hdc, timestamp_hfont);
		GetTextExtentExPointA(hdc, "A", 1, width, &nchar_fit, &dx, &sz);

		int height_timestamp_line = sz.cy + (sz.cy / 5) + TOP_MARGIN;

		int render_height = TOP_MARGIN;

		// start adding up the height to render messages starting with the last message
		uint32_t n = msg_thread->GetNumMessages();
		if (n == 0)
			return -1; // no messages to render

		if (n == 1)
			return 0; // only one message, make sure it appears at the top

		while (idx >= 0)
		{
			string message;
			uint64_t timestamp_ms;
			bool sender;
			if (msg_thread->GetMessageByIndex(idx, message, timestamp_ms, sender) == false)
				break;

			STRING s_msg = message.c_str();
			int nlines = s_msg.Replace("\n", "\n") + 1;

			render_height += height_timestamp_line + height_txt_line * nlines;

			if (render_height > height)
				break;

			idx_top = idx;

			idx--;
		}

		return idx_top;
	}
#endif

	// Render the text message thread starting at the specified message index
	// h - handle to the thread - may be zero
	// idx_top - not used on input, on output this is the top line rendered
	// idx_bottom - index at the bottom after rendering, set to 0xFFFFFFFF to render the last index at the bottom, on return this is set to the last line rendered
	// wnd_rect - client window area
	// hdc - device context handle
	bool RenderByIndexAtBottom(uint64_t h, uint32_t &idx_top, uint32_t &idx_bottom, const RECT& wnd_rect, HDC hdc, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		CreateResourecesIfNecessary();
		
		if (idx_bottom == 0xFFFFFFFF)
		{
			idx_bottom = msg_thread->GetNumMessages();
			if (idx_bottom == 0)
				return true; // there are not messages to render

			idx_bottom--;
		}

		idx_top = compute_idx_top_given_idx_bottom(msg_thread, idx_bottom, wnd_rect, hdc);

		bool not_used = false;
		return RenderByIndexAtTop_unprotected(h, idx_top, idx_bottom, wnd_rect, hdc, not_used, err_msg);
	}

	// Render the text message thread starting at the specified message index
	// h - handle to the thread - may be zero
	// idx_top - top message index, returned value may be different if input value is invalid
	// idx_bottom - bottom message index on ouput, input value is ignored
	// wnd_rect - client window area
	// hdc - device context handle
	// unread_messaged_appended - if true on input and if the bottom line of the render does not reach the last message, then a symbol is rendered showing that
	//                            there are unread messages
	bool RenderByIndexAtTop(uint64_t h, uint32_t& idx_top, uint32_t& idx_bottom, const RECT& wnd_rect, HDC hdc, bool& unread_messages_appended, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		CreateResourecesIfNecessary();
		
		bool status = RenderByIndexAtTop_unprotected(h, idx_top, idx_bottom, wnd_rect, hdc, unread_messages_appended, err_msg);

		return status;
	}


	// Returns false if there is no message text at this point
	// h - handle to the thread, may be zero
	// pt - point in the client window rectangle where the message was rendered
	// msg_txt - text of the message at the specified point, only includes text from a single line rendered to the screen
	bool GetMessageTextAtPoint(uint64_t h, POINT pt, string& msg_txt)
	{
		for (int i = 0; i < s_rendered_text.size(); i++)
		{
			if (is_inside(pt, s_rendered_text[i].m_rect))
			{
				msg_txt = s_rendered_text[i].m_text;
				return true;
			}
		}

		return false;
	}

	inline void increase_buf_alloc_if_necessary(vector<char>& buf, uint32_t idx, size_t sz)
	{
		size_t required_size = idx + sz + 1;

		if (buf.size() >= required_size)
			return;

		size_t new_size = max((buf.size() * 3) / 2, required_size);

		buf.resize(new_size);
	}

	// Copy the text message history for the given handle to the Windows clipboard
	// h - handle to the thread, may be zero
	// hWnd - windows handle of the owner of the clipboard
	bool CopyHistory(uint64_t h, HWND hWnd, string& err_msg)
	{
		SemaphoreObject sem_obj(s_semaphore, 2000);
		if (sem_obj.Failed(err_msg))
			return false;

		GET_MESSAGE_THREAD_HANDLE_OBJ;

		MessageThread* msg_thread = h_obj->m_obj;

		uint32_t n = msg_thread->GetNumMessages();
		if (n == 0)
		{
			err_msg = "There are no messages to copy to the clipboard.";
			return false;
		}

		vector<char> buf;
		buf.resize(0x10000); // start with a 64K buffer
		uint32_t idx = 0;

		const char* c = &buf[0];

		string local_id, remote_id;
		bin_to_ascii_char(msg_thread->Get_hashed_LocalID(), 32, local_id);
		bin_to_ascii_char(msg_thread->Get_hashed_RemoteID(), 32, remote_id);

		char b[1024];
		ZERO(b);
		sprintf_s(b, sizeof(b), "LocalID: %s\nRemoteID: %s (%s)\n\n", local_id.c_str(), remote_id.c_str(), msg_thread->GetNickname());

		int len = strlen(b);
		increase_buf_alloc_if_necessary(buf, idx, len);
		memmove(&buf[idx], b, len);
		idx += len;
	
		const char* sender =   "[Local]\n";
		const char* receiver = "[Remote]\n";
		const char* cr = "\n";

		int len_sender = strlen(sender);
		int len_receiver = strlen(receiver);
		int len_cr = strlen(cr);

		for (uint32_t i = 0; i < n; i++)
		{
			string msg, s_timestamp;
			bool b_sender;
			uint64_t timestamp_ms;

			msg_thread->GetMessageByIndex(i, msg, timestamp_ms, b_sender);
			get_local_time_as_string(timestamp_ms, s_timestamp);

			increase_buf_alloc_if_necessary(buf, idx, s_timestamp.length()+1);
			memmove(&buf[idx], s_timestamp.c_str(), s_timestamp.length());
			idx += s_timestamp.length();

			buf[idx++] = ' ';

			// Append "Sender" or "Receiver" on the same line as the timestamp
			int len = b_sender ? len_sender : len_receiver;
			const char* sr = b_sender ? sender : receiver;

			increase_buf_alloc_if_necessary(buf, idx, len);
			memmove(&buf[idx], sr, len);
			idx += len;

			len = strlen(msg.c_str());
			increase_buf_alloc_if_necessary(buf, idx, len);
			memmove(&buf[idx], msg.c_str(), len);
			idx += len;

			increase_buf_alloc_if_necessary(buf, idx, len_cr);
			memmove(&buf[idx], cr, len_cr);
			idx += len_cr;
		}

		buf.resize(idx);

		if (CopyStringToClipboard(&buf[0], hWnd) == FALSE)
		{
			err_msg = "Problem copying data to the clipboard.";
			return false;
		}
		
		return true;
	}
};