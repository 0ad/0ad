/* Copyright (c) 2011 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"

#include "Profiler2.h"

#include "lib/allocators/shared_ptr.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Profiler2GPU.h"
#include "third_party/mongoose/mongoose.h"

#include <iomanip>

CProfiler2 g_Profiler2;

// A human-recognisable pattern (for debugging) followed by random bytes (for uniqueness)
const u8 CProfiler2::RESYNC_MAGIC[8] = {0x11, 0x22, 0x33, 0x44, 0xf4, 0x93, 0xbe, 0x15};

CProfiler2::CProfiler2() :
	m_Initialised(false), m_FrameNumber(0), m_MgContext(NULL), m_GPU(NULL)
{
}

CProfiler2::~CProfiler2()
{
	if (m_Initialised)
		Shutdown();
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
			mg_printf(conn, "%s", header404);
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

void CProfiler2::InitialiseGPU()
{
	ENSURE(!m_GPU);
	m_GPU = new CProfiler2GPU(*this);
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

void CProfiler2::EnableGPU()
{
	ENSURE(m_Initialised);
	if (!m_GPU)
		InitialiseGPU();
}

void CProfiler2::ShutdownGPU()
{
	SAFE_DELETE(m_GPU);
}

void CProfiler2::Shutdown()
{
	ENSURE(m_Initialised);

	ENSURE(!m_GPU); // must shutdown GPU before profiler

	if (m_MgContext)
	{
		mg_stop(m_MgContext);
		m_MgContext = NULL;
	}

	// the destructor is not called for the main thread
	// we have to call it manually to avoid memory leaks
	ENSURE(ThreadUtil::IsMainThread());
	void * dataptr = pthread_getspecific(m_TLS);
	TLSDtor(dataptr);

	int err = pthread_key_delete(m_TLS);
	ENSURE(err == 0);
	m_Initialised = false;
}

void CProfiler2::RecordGPUFrameStart()
{
	if (m_GPU)
		m_GPU->FrameStart();
}

void CProfiler2::RecordGPUFrameEnd()
{
	if (m_GPU)
		m_GPU->FrameEnd();
}

void CProfiler2::RecordGPURegionEnter(const char* id)
{
	if (m_GPU)
		m_GPU->RegionEnter(id);
}

void CProfiler2::RecordGPURegionLeave(const char* id)
{
	if (m_GPU)
		m_GPU->RegionLeave(id);
}

/**
 * Called by pthreads when a registered thread is destroyed.
 */
void CProfiler2::TLSDtor(void* data)
{
	ThreadStorage* storage = (ThreadStorage*)data;

	storage->GetProfiler().RemoveThreadStorage(storage);

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

	AddThreadStorage(storage);
}

void CProfiler2::AddThreadStorage(ThreadStorage* storage)
{
	CScopeLock lock(m_Mutex);
	m_Threads.push_back(storage);
}

void CProfiler2::RemoveThreadStorage(ThreadStorage* storage)
{
	CScopeLock lock(m_Mutex);
	m_Threads.erase(std::find(m_Threads.begin(), m_Threads.end(), storage));
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
	int len = vsnprintf(buffer + 4, MAX_ATTRIBUTE_LENGTH - 1, fmt, argp); // subtract 1 from length to make MSVC vsnprintf safe
	// (Don't use vsprintf_s because it treats overflow as fatal)

	// Terminate the string if the printing was truncated
	if (len < 0 || len >= (int)MAX_ATTRIBUTE_LENGTH - 1)
	{
		strncpy(buffer + 4 + MAX_ATTRIBUTE_LENGTH - 4, "...", 4);
		len = MAX_ATTRIBUTE_LENGTH - 1; // excluding null terminator
	}

	// Store the length in the buffer
	memcpy(buffer, &len, sizeof(len));

	Write(ITEM_ATTRIBUTE, buffer, 4 + len);
}


void CProfiler2::ConstructJSONOverview(std::ostream& stream)
{
	TIMER(L"profile2 overview");

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
	TIMER(L"profile2 visitor");

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
			visitor.OnSync(lastTime);
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

	void OnSync(double UNUSED(time))
	{
		// Split the array of items into an array of array (arbitrarily splitting
		// around the sync points) to avoid array-too-large errors in JSON decoders
		m_Stream << "null], [\n";
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
	TIMER(L"profile2 query");

	std::string buffer;

	{
		TIMER(L"profile2 get buffer");

		CScopeLock lock(m_Mutex); // lock against changes to m_Threads or deletions of ThreadStorage

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

		stream << "[\n";
		buffer = storage->GetBuffer();
	}

	BufferVisitor_Dump visitor(stream);
	RunBufferVisitor(buffer, visitor);

	stream << "null]\n]}";

	return NULL;
}

void CProfiler2::SaveToFile()
{
	OsPath path = psLogDir()/"profile2.jsonp";
	std::ofstream stream(OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);
	ENSURE(stream.good());

	std::vector<ThreadStorage*> threads;

	{
		CScopeLock lock(m_Mutex);
		threads = m_Threads;
	}

	stream << "profileDataCB({\"threads\": [\n";
	for (size_t i = 0; i < threads.size(); ++i)
	{
		if (i != 0)
			stream << ",\n";
		stream << "{\"name\":\"" << CStr(threads[i]->GetName()).EscapeToPrintableASCII() << "\",\n";
		stream << "\"data\": ";
		ConstructJSONResponse(stream, threads[i]->GetName());
		stream << "\n}";
	}
	stream << "\n]});\n";
}
