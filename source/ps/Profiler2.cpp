/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Profiler2.h"

#include "lib/allocators/shared_ptr.h"
#include "ps/CLogger.h"
#include "third_party/mongoose/mongoose.h"

CProfiler2 g_Profiler2;

// A human-recognisable pattern (for debugging) followed by random bytes (for uniqueness)
const u8 CProfiler2::RESYNC_MAGIC[8] = {0x11, 0x22, 0x33, 0x44, 0xf4, 0x93, 0xbe, 0x15};

CProfiler2::CProfiler2() :
	m_Initialised(false), m_MgContext(NULL)
{
}

/**
 * Mongoose callback. Run in an arbitrary thread (possibly concurrently with other requests).
 */
static void* MgCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info)
{
	CProfiler2* profiler = (CProfiler2*)request_info->user_data;
	ENSURE(profiler);

	void* handled = (void*)""; // arbitrary non-NULL pointer to indicate successful handling

	const char* header200 =
		"HTTP/1.1 200 OK\r\n"
		"Access-Control-Allow-Origin: *\r\n" // TODO: not great for security
		"Content-Type: text/plain; charset=utf-8\r\n\r\n";

	const char* header404 =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Unrecognised URI";

	const char* header400 =
		"HTTP/1.1 400 Bad Request\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Invalid request";

	switch (event)
	{
	case MG_NEW_REQUEST:
	{
		std::stringstream stream;

		std::string uri = request_info->uri;
		if (uri == "/overview")
		{
			profiler->ConstructJSONOverview(stream);
		}
		else if (uri == "/query")
		{
			if (!request_info->query_string)
			{
				mg_printf(conn, "%s (no query string)", header400);
				return handled;
			}

			// Identify the requested thread
			char buf[256];
			int len = mg_get_var(request_info->query_string, strlen(request_info->query_string), "thread", buf, ARRAY_SIZE(buf));
			if (len < 0)
			{
				mg_printf(conn, "%s (no 'thread')", header400);
				return handled;
			}
			std::string thread(buf);

			const char* err = profiler->ConstructJSONResponse(stream, thread);
			if (err)
			{
				mg_printf(conn, "%s (%s)", header400, err);
				return handled;
			}
		}
		else
		{
			mg_printf(conn, header404);
			return handled;
		}

		mg_printf(conn, "%s", header200);
		std::string str = stream.str();
		mg_write(conn, str.c_str(), str.length());
		return handled;
	}

	case MG_HTTP_ERROR:
		return NULL;

	case MG_EVENT_LOG:
		// Called by Mongoose's cry()
		LOGERROR(L"Mongoose error: %hs", request_info->log_message);
		return NULL;

	case MG_INIT_SSL:
		return NULL;

	default:
		debug_warn(L"Invalid Mongoose event type");
		return NULL;
	}
};

void CProfiler2::Initialise()
{
	ENSURE(!m_Initialised);
	int err = pthread_key_create(&m_TLS, &CProfiler2::TLSDtor);
	ENSURE(err == 0);
	m_Initialised = true;

	RegisterCurrentThread("main");
}

void CProfiler2::EnableHTTP()
{
	ENSURE(m_Initialised);

	// Ignore multiple enablings
	if (m_MgContext)
		return;

	const char *options[] = {
		"listening_ports", "127.0.0.1:8000", // bind to localhost for security
		"num_threads", "6", // enough for the browser's parallel connection limit
		NULL
	};
	m_MgContext = mg_start(MgCallback, this, options);
	ENSURE(m_MgContext);
}

void CProfiler2::Shutdown()
{
	ENSURE(m_Initialised);

	if (m_MgContext)
	{
		mg_stop(m_MgContext);
		m_MgContext = NULL;
	}

	// TODO: free non-NULL keys, instead of leaking them

	int err = pthread_key_delete(m_TLS);
	ENSURE(err == 0);
	m_Initialised = false;
}

/**
 * Called by pthreads when a registered thread is destroyed.
 */
void CProfiler2::TLSDtor(void* data)
{
	ThreadStorage* storage = (ThreadStorage*)data;

	CProfiler2& profiler = storage->GetProfiler();

	{
		CScopeLock lock(profiler.m_Mutex);
		profiler.m_Threads.erase(std::find(profiler.m_Threads.begin(), profiler.m_Threads.end(), storage));
	}

	delete (ThreadStorage*)data;
}

void CProfiler2::RegisterCurrentThread(const std::string& name)
{
	ENSURE(m_Initialised);

	ENSURE(pthread_getspecific(m_TLS) == NULL); // mustn't register a thread more than once

	ThreadStorage* storage = new ThreadStorage(*this, name);
	int err = pthread_setspecific(m_TLS, storage);
	ENSURE(err == 0);

	RecordSyncMarker();
	RecordEvent("thread start");

	CScopeLock lock(m_Mutex);
	m_Threads.push_back(storage);
}

CProfiler2::ThreadStorage::ThreadStorage(CProfiler2& profiler, const std::string& name) :
	m_Profiler(profiler), m_Name(name), m_BufferPos0(0), m_BufferPos1(0), m_LastTime(timer_Time())
{
	m_Buffer = new u8[BUFFER_SIZE];
	memset(m_Buffer, ITEM_NOP, BUFFER_SIZE);
}

CProfiler2::ThreadStorage::~ThreadStorage()
{
	delete[] m_Buffer;
}

std::string CProfiler2::ThreadStorage::GetBuffer()
{
	// Called from an arbitrary thread (not the one writing to the buffer).
	// 
	// See comments on m_BufferPos0 etc.

	shared_ptr<u8> buffer(new u8[BUFFER_SIZE], ArrayDeleter());

	u32 pos1 = m_BufferPos1;
	COMPILER_FENCE; // must read m_BufferPos1 before m_Buffer

	memcpy(buffer.get(), m_Buffer, BUFFER_SIZE);

	COMPILER_FENCE; // must read m_BufferPos0 after m_Buffer
	u32 pos0 = m_BufferPos0;

	// The range [pos1, pos0) modulo BUFFER_SIZE is invalid, so concatenate the rest of the buffer

	if (pos1 <= pos0) // invalid range is in the middle of the buffer
		return std::string(buffer.get()+pos0, buffer.get()+BUFFER_SIZE) + std::string(buffer.get(), buffer.get()+pos1);
	else // invalid wrap is wrapped around the end/start buffer
		return std::string(buffer.get()+pos0, buffer.get()+pos1);
}

void CProfiler2::ThreadStorage::RecordAttribute(const char* fmt, va_list argp)
{
	char buffer[MAX_ATTRIBUTE_LENGTH + 4] = {0}; // first 4 bytes are used for storing length
	i32 len = vsprintf_s(buffer + 4, ARRAY_SIZE(buffer) - 4, fmt, argp);
	if (len < 0)
	{
		debug_warn("Profiler attribute sprintf failed");
		return;
	}

	// Store the length in the buffer
	memcpy(buffer, &len, sizeof(len)); 

	Write(ITEM_ATTRIBUTE, buffer, 4 + len);
}


void CProfiler2::ConstructJSONOverview(std::ostream& stream)
{
	CScopeLock lock(m_Mutex);

	stream << "{\"threads\":[";
	for (size_t i = 0; i < m_Threads.size(); ++i)
	{
		if (i != 0)
			stream << ",";
		stream << "{\"name\":\"" << CStr(m_Threads[i]->GetName()).EscapeToPrintableASCII() << "\"}";
	}
	stream << "]}";
}

/**
 * Given a buffer and a visitor class (with functions OnEvent, OnEnter, OnLeave, OnAttribute),
 * calls the visitor for every item in the buffer.
 */
template<typename V>
void RunBufferVisitor(const std::string& buffer, V& visitor)
{
	// The buffer doesn't necessarily start at the beginning of an item
	// (we just grabbed it from some arbitrary point in the middle),
	// so scan forwards until we find a sync marker.
	// (This is probably pretty inefficient.)

	u32 realStart = (u32)-1; // the start point decided by the scan algorithm

	for (u32 start = 0; start + 1 + sizeof(CProfiler2::RESYNC_MAGIC) <= buffer.length(); ++start)
	{
		if (buffer[start] == CProfiler2::ITEM_SYNC
			&& memcmp(buffer.c_str() + start + 1, &CProfiler2::RESYNC_MAGIC, sizeof(CProfiler2::RESYNC_MAGIC)) == 0)
		{
			realStart = start;
			break;
		}
	}

	ENSURE(realStart != (u32)-1); // we should have found a sync point somewhere in the buffer

	u32 pos = realStart; // the position as we step through the buffer

	double lastTime = -1;
		// set to non-negative by EVENT_SYNC; we ignore all items before that
		// since we can't compute their absolute times

	while (pos < buffer.length())
	{
		u8 type = buffer[pos];
		++pos;

		switch (type)
		{
		case CProfiler2::ITEM_NOP:
		{
			// ignore
			break;
		}
		case CProfiler2::ITEM_SYNC:
		{
			u8 magic[sizeof(CProfiler2::RESYNC_MAGIC)];
			double t;
			memcpy(magic, buffer.c_str()+pos, ARRAY_SIZE(magic));
			ENSURE(memcmp(magic, &CProfiler2::RESYNC_MAGIC, sizeof(CProfiler2::RESYNC_MAGIC)) == 0);
			pos += sizeof(CProfiler2::RESYNC_MAGIC);
			memcpy(&t, buffer.c_str()+pos, sizeof(t));
			pos += sizeof(t);
			lastTime = t;
			break;
		}
		case CProfiler2::ITEM_EVENT:
		{
			CProfiler2::SItem_dt_id item;
			memcpy(&item, buffer.c_str()+pos, sizeof(item));
			pos += sizeof(item);
			if (lastTime >= 0)
			{
				lastTime = lastTime + (double)item.dt;
				visitor.OnEvent(lastTime, item.id);
			}
			break;
		}
		case CProfiler2::ITEM_ENTER:
		{
			CProfiler2::SItem_dt_id item;
			memcpy(&item, buffer.c_str()+pos, sizeof(item));
			pos += sizeof(item);
			if (lastTime >= 0)
			{
				lastTime = lastTime + (double)item.dt;
				visitor.OnEnter(lastTime, item.id);
			}
			break;
		}
		case CProfiler2::ITEM_LEAVE:
		{
			CProfiler2::SItem_dt_id item;
			memcpy(&item, buffer.c_str()+pos, sizeof(item));
			pos += sizeof(item);
			if (lastTime >= 0)
			{
				lastTime = lastTime + (double)item.dt;
				visitor.OnLeave(lastTime, item.id);
			}
			break;
		}
		case CProfiler2::ITEM_ATTRIBUTE:
		{
			u32 len;
			memcpy(&len, buffer.c_str()+pos, sizeof(len));
			ENSURE(len <= CProfiler2::MAX_ATTRIBUTE_LENGTH);
			pos += sizeof(len);
			std::string attribute(buffer.c_str()+pos, buffer.c_str()+pos+len);
			pos += len;
			if (lastTime >= 0)
			{
				visitor.OnAttribute(attribute);
			}
			break;
		}
		default:
			debug_warn(L"Invalid profiler item when parsing buffer");
			return;
		}
	}
};

/**
 * Visitor class that dumps events as JSON.
 * TODO: this is pretty inefficient (in implementation and in output format).
 */
struct BufferVisitor_Dump
{
	NONCOPYABLE(BufferVisitor_Dump);
public:
	BufferVisitor_Dump(std::ostream& stream) : m_Stream(stream)
	{
	}

	void OnEvent(double time, const char* id)
	{
		m_Stream << "[1," << std::fixed << std::setprecision(9) << time;
		m_Stream << ",\"" << CStr(id).EscapeToPrintableASCII() << "\"],\n";
	}

	void OnEnter(double time, const char* id)
	{
		m_Stream << "[2," << std::fixed << std::setprecision(9) << time;
		m_Stream << ",\"" << CStr(id).EscapeToPrintableASCII() << "\"],\n";
	}

	void OnLeave(double time, const char* id)
	{
		m_Stream << "[3," << std::fixed << std::setprecision(9) << time;
		m_Stream << ",\"" << CStr(id).EscapeToPrintableASCII() << "\"],\n";
	}

	void OnAttribute(const std::string& attr)
	{
		m_Stream << "[4,\"" << CStr(attr).EscapeToPrintableASCII() << "\"],\n";
	}

	std::ostream& m_Stream;
};

const char* CProfiler2::ConstructJSONResponse(std::ostream& stream, const std::string& thread)
{
	CScopeLock lock(m_Mutex);

	ThreadStorage* storage = NULL;
	for (size_t i = 0; i < m_Threads.size(); ++i)
	{
		if (m_Threads[i]->GetName() == thread)
		{
			storage = m_Threads[i];
			break;
		}
	}

	if (!storage)
		return "cannot find named thread";

	stream << "{\"events\":[\n";

	std::string buffer = storage->GetBuffer();
	BufferVisitor_Dump visitor(stream);
	RunBufferVisitor(buffer, visitor);

	stream << "null\n]}";

	return NULL;
}
