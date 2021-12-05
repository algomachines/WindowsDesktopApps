// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "OS.h"

#include "SimpleDB.hpp"

#pragma once

class TextMessageHdr
{
public:

	inline TextMessageHdr(void)
	{
		m_data = 0;
	}

	inline bool operator < (const TextMessageHdr& obj) const
	{
		return m_Timestamp_ms < obj.m_Timestamp_ms;
	}

	inline bool operator > (const TextMessageHdr& obj) const
	{
		return m_Timestamp_ms > obj.m_Timestamp_ms;
	}

	static inline uint32_t GetSizeBytes(void)
	{
		return sizeof(TextMessageHdr);
	}

	inline void Initialize(bool sender, uint8_t nchar)
	{
		m_is_sender = sender ? 1 : 0;
		m_nchar = nchar;
		m_Timestamp_ms = get_time_ms();
	}

	// Used only to update nchar
	inline bool Update(const TextMessageHdr& rec, string& err_msg)
	{
		if (m_nchar != rec.m_nchar)
			m_nchar = rec.m_nchar;

		return true;
	}

	inline bool HasSameData(const TextMessageHdr& rec) const
	{
		if (m_nchar != rec.m_nchar)
			return false;

		if (m_is_sender != rec.m_is_sender)
			return false;

		return true;
	}

	inline bool Assign(const TextMessageHdr& rec, string& err_msg)
	{
		m_data = rec.m_data;
		return true;
	}

	inline uint64_t GetTimestamp_ms(void) const
	{
		return m_Timestamp_ms;
	}

	inline void SetTimestamp_ms(uint64_t t)
	{
		m_Timestamp_ms = t;
	}

	inline uint8_t GetNChar(void) const
	{
		return (uint8_t)m_nchar;
	}

	inline bool IsSender(void) const
	{
		return (bool)m_is_sender;
	}

private:

	union
	{
		uint64_t m_data;
		struct
		{
			uint64_t m_Timestamp_ms : 48; // good for 8500 years
			uint64_t m_is_sender : 1; // Message is from sender or receiver
			uint64_t m_nchar : 8; // Up to 256 characters in the messsage
			uint64_t m_reserved : 7; // reserved space
		};
	};
};

class MessageThread
{
public:

	inline MessageThread(void)
	{
		m_reserved = 0;
		ZERO(m_nickname);
		ZERO(m_hashed_LocalID);
		ZERO(m_hashed_RemoteID);
	}

	uint32_t GetStorageSizeBytes(void) const
	{
		uint32_t n = sizeof(m_reserved);
		n += GSSv(m_URL);
		n += GSSv(m_nickname);
		n += sizeof(m_hashed_LocalID);
		n += sizeof(m_hashed_RemoteID);
		n += m_header_db.GetStorageSizeBytes();
		n += GSSv(m_index);
		n += GSSv(m_message_data);
		return n;
	}

	uint32_t SaveToBuffer(void* b) const
	{
		uint32_t i = 0;

		STB(m_reserved);
		STBv(m_URL);
		STBv(m_nickname);
		STB(m_hashed_LocalID);
		STB(m_hashed_RemoteID);

		i += m_header_db.SaveToBuffer((uint8_t*)b + i);

		STBv(m_index);
		STBv(m_message_data);

		return i;
	}

	uint32_t LoadFromBuffer(const void* b)
	{
		uint32_t i = 0;

		LFB(m_reserved);
		LFBv(m_URL);
		LFBv(m_nickname);
		LFB(m_hashed_LocalID);
		LFB(m_hashed_RemoteID);

		i += m_header_db.LoadFromBuffer((const uint8_t*)b + i);

		LFBv(m_index);
		LFBv(m_message_data);

		return i;
	}

	uint32_t GetNumMessages(void) const
	{
		return m_header_db.GetNumRecords();
	}

	inline bool AppendTextMessage(bool sender, const char* text, string& err_msg, uint64_t *timestamp_ms=0)
	{
		int msg_len = StrLen(text);
		ASSERT(msg_len <= 256);
		if (msg_len > 256)
		{
			ERROR_LOCATION(err_msg);
			err_msg = "Can't add text message to the database because it contains more than 256 characters.";
			return false;
		}

		TextMessageHdr rec;
		rec.Initialize(sender, msg_len);
		if (timestamp_ms)
			rec.SetTimestamp_ms(*timestamp_ms);

		uint32_t nrec = m_header_db.GetNumRecords();
		if (nrec && rec.GetTimestamp_ms() < m_header_db.GetRecordByIndex(nrec - 1)->GetTimestamp_ms())
		{
			ERROR_LOCATION(err_msg);
			err_msg = "Can't add text message to the database because its timestamp is earlier than the last text message.";
			return false;
		}

		bool changes_made;
		if (m_header_db.UpdateRecord(rec, changes_made, err_msg) == false)
			return false;

		if (changes_made == false)
			return true; // should not happen, but no harm if it does

		// It is possible the the record which was just updated was not at the end.
		// If so, the index and message data may not simply be appended
		bool exists;
		uint32_t rec_idx = m_header_db.GetRecordIndex(rec, exists);
		ASSERT(exists);

		if (rec_idx == m_header_db.GetNumRecords() - 1)
		{
			// Record is at the end, simply append
			uint32_t old_sz = m_message_data.size();
			m_index.push_back(old_sz);
			m_message_data.resize(m_message_data.size() + msg_len);

			memmove(&m_message_data[old_sz], text, msg_len);
		}
		else
		{
			ERROR_LOCATION(err_msg);
			err_msg = "Unknown error.";
			return false; // should not happen - trouble if it does

			//// Record is not at the end, must insert
			//uint32_t n = m_index[rec_idx];
			//m_index.insert(m_index.begin() + rec_idx, n); // insert
			//for (int i = rec_idx + 1; i < m_index.size(); i++) m_index[i] += msg_len; // adjust trailing values

			//// Insert the text in the appropriate location
			//m_message_data.insert(m_message_data.begin() + n, msg_len, 0);
			//for (int i = n; i < n + msg_len; i++)
			//	m_message_data[i] = text[i - n];
			
		}

		return true;
	}

	inline void SetNickname(const char* s)
	{
		int len = StrLen(s);
		len = min(len, 32);
		m_nickname.resize(len + 1);
		memmove(&m_nickname[0], s, len);
		m_nickname[len] = 0;
	}

	inline const char* GetNickname(void) const
	{
		return m_nickname.c_str();
	}

	inline void SetURL(const char* s)
	{
		int len = StrLen(s);
		len = min(len, 32);
		m_URL.resize(len + 1);
		memmove(&m_URL[0], s, len);
	}

	inline const char* GetURL(void) const
	{
		return m_URL.c_str();
	}

	inline const uint8_t* Get_hashed_RemoteID(void) const
	{
		return m_hashed_RemoteID;
	}

	inline const uint8_t* Get_hashed_LocalID(void) const
	{
		return m_hashed_LocalID;
	}

	inline void Set_hashed_LocalID(const uint8_t* id)
	{
		memmove(m_hashed_LocalID, id, sizeof(m_hashed_LocalID));
	}

	inline void Set_hashed_RemoteID(const uint8_t* id)
	{
		memmove(m_hashed_RemoteID, id, sizeof(m_hashed_RemoteID));
	}

	inline bool GetMessageByIndex(uint32_t idx, string& message, uint64_t &timestamp_ms, bool &sender) const
	{
		if (idx >= m_index.size())
			return false;

		const TextMessageHdr* hdr = m_header_db.GetRecordByIndex(idx);
		if (hdr == 0)
			return false;

		timestamp_ms = hdr->GetTimestamp_ms();

		sender = hdr->IsSender();

		uint8_t nchar = hdr->GetNChar();

		message.resize((int)(nchar+1));

		memmove(&message[0], &m_message_data[m_index[idx]], nchar);

		return true;
	}

	inline bool ClearHistory(string &err_msg)
	{
		if (m_index.size())
			memset(&m_index[0], 0, m_index.size() * sizeof(m_index[0]));
		
		if (m_message_data.size())
			memset(&m_message_data[0], 0, m_message_data.size() * sizeof(m_message_data[0]));

		m_header_db.RemoveAllRecords();
		m_index.resize(0);
		m_message_data.resize(0);

		return true;
	}

private:

	uint64_t m_reserved; 

	string m_URL;
	string m_nickname;
	uint8_t m_hashed_LocalID[32];
	uint8_t m_hashed_RemoteID[32];

	SimpleDB<TextMessageHdr> m_header_db;

	vector<uint32_t> m_index;			// index for each text message in the buffer
	vector<uint8_t>  m_message_data;	// raw text data
};