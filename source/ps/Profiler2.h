/* Copyright (c) 2016 Wildfire Games
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

/**
 * @file
 * New profiler (complementing the older CProfileManager)
 *
 * The profiler is designed for analysing framerate fluctuations or glitches,
 * and temporal relationships between threads.
 * This contrasts with CProfilerManager and most external profiling tools,
 * which are designed more for measuring average throughput over a number of
 * frames.
 *
 * To view the profiler output, press F11 to enable the HTTP output mode
 * and then open source/tools/profiler2/profiler2.html in a web browser.
 *
 * There is a single global CProfiler2 instance (g_Profiler2),
 * providing the API used by the rest of the game.
 * The game can record the entry/exit timings of a region of code
 * using the PROFILE2 macro, and can record other events using
 * PROFILE2_EVENT.
 * Regions and events can be annotated with arbitrary string attributes,
 * specified with printf-style format strings, using PROFILE2_ATTR
 * (e.g. PROFILE2_ATTR("frame: %d", m_FrameNum) ).
 *
 * This is designed for relatively coarse-grained profiling, or for rare events.
 * Don't use it for regions that are typically less than ~0.1msecs, or that are
 * called hundreds of times per frame. (The old CProfilerManager is better
 * for that.)
 *
 * New threads must call g_Profiler2.RegisterCurrentThread before any other
 * profiler functions.
 *
 * The main thread should call g_Profiler2.RecordFrameStart at the start of
 * each frame.
 * Other threads should call g_Profiler2.RecordSyncMarker occasionally,
 * especially if it's been a long time since their last call to the profiler,
 * or if they've made thousands of calls since the last sync marker.
 *
 * The profiler is implemented with thread-local fixed-size ring buffers,
 * which store a sequence of variable-length items indicating the time
 * of the event and associated data (pointers to names, attribute strings, etc).
 * An HTTP server provides access to the data: when requested, it will make
 * a copy of a thread's buffer, then parse the items and return them in JSON
 * format. The profiler2.html requests and processes and visualises this data.
 *
 * The RecordSyncMarker calls are necessary to correct for time drift and to
 * let the buffer parser accurately detect the start of an item in the byte stream.
 *
 * This design aims to minimise the performance overhead of recording data,
 * and to simplify the visualisation of the data by doing it externally in an
 * environment with better UI tools (i.e. HTML) instead of within the game engine.
 *
 * The initial setup of g_Profiler2 must happen in the game's main thread.
 * RegisterCurrentThread and the Record functions may be called from any thread.
 * The HTTP server runs its own threads, which may call the ConstructJSON functions.
 */

#ifndef INCLUDED_PROFILER2
#define INCLUDED_PROFILER2

#include "lib/timer.h"
#include "ps/ThreadUtil.h"

struct mg_context;

// Note: Lots of functions are defined inline, to hypothetically
// minimise performance overhead.

class CProfiler2GPU;

class CProfiler2
{
	friend class CProfiler2GPU_base;
	friend class CProfile2SpikeRegion;
	friend class CProfile2AggregatedRegion;
public:
	// Items stored in the buffers:

	/// Item type identifiers
	enum EItem
	{
		ITEM_NOP = 0,
		ITEM_SYNC = 1, // magic value used for parse syncing
		ITEM_EVENT = 2, // single event
		ITEM_ENTER = 3, // entering a region
		ITEM_LEAVE = 4, // leaving a region (must be correctly nested)
		ITEM_ATTRIBUTE = 5, // arbitrary string associated with current region, or latest event (if the previous item was an event)
	};

	static const size_t MAX_ATTRIBUTE_LENGTH = 256; // includes null terminator, which isn't stored

	/// An arbitrary number to help resyncing with the item stream when parsing.
	static const u8 RESYNC_MAGIC[8];

	/**
	 * An item with a relative time and an ID string pointer.
	 */
	struct SItem_dt_id
	{
		float dt; // time relative to last event
		const char* id;
	};

private:
	// TODO: what's a good size?
	// TODO: different threads might want different sizes
	static const size_t BUFFER_SIZE = 4*1024*1024;
	static const size_t HOLD_BUFFER_SIZE = 128 * 1024;

	/**
	 * Class instantiated in every registered thread.
	 */
	class ThreadStorage
	{
		NONCOPYABLE(ThreadStorage);
	public:
		ThreadStorage(CProfiler2& profiler, const std::string& name);
		~ThreadStorage();

		enum { BUFFER_NORMAL, BUFFER_SPIKE, BUFFER_AGGREGATE };

		void RecordSyncMarker(double t)
		{
			// Store the magic string followed by the absolute time
			// (to correct for drift caused by the precision of relative
			// times stored in other items)
			u8 buffer[sizeof(RESYNC_MAGIC) + sizeof(t)];
			memcpy(buffer, &RESYNC_MAGIC, sizeof(RESYNC_MAGIC));
			memcpy(buffer + sizeof(RESYNC_MAGIC), &t, sizeof(t));
			Write(ITEM_SYNC, buffer, ARRAY_SIZE(buffer));
			m_LastTime = t;
		}

		void Record(EItem type, double t, const char* id)
		{
			// Store a relative time instead of absolute, so we can use floats
			// (to save memory) without suffering from precision problems
			SItem_dt_id item = { (float)(t - m_LastTime), id };
			Write(type, &item, sizeof(item));
		}

		void RecordFrameStart(double t)
		{
			RecordSyncMarker(t);
			Record(ITEM_EVENT, t, "__framestart"); // magic string recognised by the visualiser
		}

		void RecordLeave(double t)
		{
			float time = (float)(t - m_LastTime);
			Write(ITEM_LEAVE, &time, sizeof(float));
		}

		void RecordAttribute(const char* fmt, va_list argp) VPRINTF_ARGS(2);

		void RecordAttributePrintf(const char* fmt, ...) PRINTF_ARGS(2)
		{
			va_list argp;
			va_start(argp, fmt);
			RecordAttribute(fmt, argp);
			va_end(argp);
		}

		size_t HoldLevel();
		u8 HoldType();
		void PutOnHold(u8 type);
		void HoldToBuffer(bool condensed);
		void ThrowawayHoldBuffer();

		CProfiler2& GetProfiler()
		{
			return m_Profiler;
		}

		const std::string& GetName()
		{
			return m_Name;
		}

		/**
		 * Returns a copy of a subset of the thread's buffer.
		 * Not guaranteed to start on an item boundary.
		 * May be called by any thread.
		 */
		std::string GetBuffer();

	private:
		/**
		 * Store an item into the buffer.
		 */
		void Write(EItem type, const void* item, u32 itemSize);

		void WriteHold(EItem type, const void* item, u32 itemSize);

		CProfiler2& m_Profiler;
		std::string m_Name;

		double m_LastTime; // used for computing relative times

		u8* m_Buffer;

		struct HoldBuffer
		{
			friend class ThreadStorage;
		public:
			HoldBuffer()
			{
				buffer = new u8[HOLD_BUFFER_SIZE];
				memset(buffer, ITEM_NOP, HOLD_BUFFER_SIZE);
				pos = 0;
			}
			~HoldBuffer()
			{
				delete[] buffer;
			}
			void clear()
			{
				pos = 0;
			}
			void setType(u8 newType)
			{
				type = newType;
			}
			u8* buffer;
			u32 pos;
			u8 type;
		};

		HoldBuffer m_HoldBuffers[8];
		size_t m_HeldDepth;

		// To allow hopefully-safe reading of the buffer from a separate thread,
		// without any expensive synchronisation in the recording thread,
		// two copies of the current buffer write position are stored.
		// BufferPos0 is updated before writing; BufferPos1 is updated after writing.
		// GetBuffer can read Pos1, copy the buffer, read Pos0, then assume any bytes
		// outside the range Pos1 <= x < Pos0 are safe to use. (Any in that range might
		// be half-written and corrupted.) (All ranges are modulo BUFFER_SIZE.)
		// Outside of Write(), these will always be equal.
		//
		// TODO: does this attempt at synchronisation (plus use of COMPILER_FENCE etc)
		// actually work in practice?
		u32 m_BufferPos0;
		u32 m_BufferPos1;
	};

public:
	CProfiler2();
	~CProfiler2();

	/**
	 * Call in main thread to set up the profiler,
	 * before calling any other profiler functions.
	 */
	void Initialise();

	/**
	 * Call in main thread to enable the HTTP server.
	 * (Disabled by default for security and performance
	 * and to avoid annoying a firewall.)
	 */
	void EnableHTTP();

	/**
	 * Call in main thread to enable the GPU profiling support,
	 * after OpenGL has been initialised.
	 */
	void EnableGPU();

	/**
	 * Call in main thread to shut down the GPU profiling support,
	 * before shutting down OpenGL.
	 */
	void ShutdownGPU();

	/**
	 * Call in main thread to shut down the profiler's HTTP server
	 */
	void ShutDownHTTP();

	/**
	 * Call in main thread to enable/disable the profiler
	 */
	void Toggle();

	/**
	 * Call in main thread to shut everything down.
	 * All other profiled threads should have been terminated already.
	 */
	void Shutdown();

	/**
	 * Call in any thread to enable the profiler in that thread.
	 * @p name should be unique, and is used by the visualiser to identify
	 * this thread.
	 */
	void RegisterCurrentThread(const std::string& name);

	/**
	 * Non-main threads should call this occasionally,
	 * especially if it's been a long time since their last call to the profiler,
	 * or if they've made thousands of calls since the last sync marker.
	 */
	void RecordSyncMarker()
	{
		GetThreadStorage().RecordSyncMarker(GetTime());
	}

	/**
	 * Call in main thread at the start of a frame.
	 */
	void RecordFrameStart()
	{
		ENSURE(ThreadUtil::IsMainThread());
		GetThreadStorage().RecordFrameStart(GetTime());
	}

	void RecordEvent(const char* id)
	{
		GetThreadStorage().Record(ITEM_EVENT, GetTime(), id);
	}

	void RecordRegionEnter(const char* id)
	{
		GetThreadStorage().Record(ITEM_ENTER, GetTime(), id);
	}

	void RecordRegionEnter(const char* id, double time)
	{
		GetThreadStorage().Record(ITEM_ENTER, time, id);
	}

	void RecordRegionLeave()
	{
		GetThreadStorage().RecordLeave(GetTime());
	}

	void RecordAttribute(const char* fmt, ...) PRINTF_ARGS(2)
	{
		va_list argp;
		va_start(argp, fmt);
		GetThreadStorage().RecordAttribute(fmt, argp);
		va_end(argp);
	}

	void RecordGPUFrameStart();
	void RecordGPUFrameEnd();
	void RecordGPURegionEnter(const char* id);
	void RecordGPURegionLeave(const char* id);

	/**
	* Hold onto messages until a call to release or write the held messages.
	*/
	size_t HoldLevel()
	{
		return GetThreadStorage().HoldLevel();
	}

	u8 HoldType()
	{
		return GetThreadStorage().HoldType();
	}

	void HoldMessages(u8 type)
	{
		GetThreadStorage().PutOnHold(type);
	}

	void StopHoldingMessages(bool writeToBuffer, bool condensed = false)
	{
		if (writeToBuffer)
			GetThreadStorage().HoldToBuffer(condensed);
		else
			GetThreadStorage().ThrowawayHoldBuffer();
	}

	/**
	 * Call in any thread to produce a JSON representation of the general
	 * state of the application.
	 */
	void ConstructJSONOverview(std::ostream& stream);

	/**
	 * Call in any thread to produce a JSON representation of the buffer
	 * for a given thread.
	 * Returns NULL on success, or an error string.
	 */
	const char* ConstructJSONResponse(std::ostream& stream, const std::string& thread);

	/**
	 * Call in any thread to save a JSONP representation of the buffers
	 * for all threads, to a file named profile2.jsonp in the logs directory.
	 */
	void SaveToFile();

	double GetTime()
	{
		return timer_Time();
	}

	int GetFrameNumber()
	{
		return m_FrameNumber;
	}

	void IncrementFrameNumber()
	{
		++m_FrameNumber;
	}

	void AddThreadStorage(ThreadStorage* storage);
	void RemoveThreadStorage(ThreadStorage* storage);

private:
	void InitialiseGPU();

	static void TLSDtor(void* data);

	ThreadStorage& GetThreadStorage()
	{
		ThreadStorage* storage = (ThreadStorage*)pthread_getspecific(m_TLS);
		ASSERT(storage);
		return *storage;
	}

	bool m_Initialised;

	int m_FrameNumber;

	mg_context* m_MgContext;

	pthread_key_t m_TLS;

	CProfiler2GPU* m_GPU;

	CMutex m_Mutex;
	std::vector<ThreadStorage*> m_Threads; // thread-safe; protected by m_Mutex
};

extern CProfiler2 g_Profiler2;

/**
 * Scope-based enter/leave helper.
 */
class CProfile2Region
{
public:
	CProfile2Region(const char* name) : m_Name(name)
	{
		g_Profiler2.RecordRegionEnter(m_Name);
	}
	~CProfile2Region()
	{
		g_Profiler2.RecordRegionLeave();
	}
protected:
	const char* m_Name;
};

/**
* Scope-based enter/leave helper.
*/
class CProfile2SpikeRegion
{
public:
	CProfile2SpikeRegion(const char* name, double spikeLimit);
	~CProfile2SpikeRegion();
private:
	const char* m_Name;
	double m_Limit;
	double m_StartTime;
	bool m_PushedHold;
};

/**
* Scope-based enter/leave helper.
*/
class CProfile2AggregatedRegion
{
public:
	CProfile2AggregatedRegion(const char* name, double spikeLimit);
	~CProfile2AggregatedRegion();
private:
	const char* m_Name;
	double m_Limit;
	double m_StartTime;
	bool m_PushedHold;
};

/**
 * Scope-based GPU enter/leave helper.
 */
class CProfile2GPURegion
{
public:
	CProfile2GPURegion(const char* name) : m_Name(name)
	{
		g_Profiler2.RecordGPURegionEnter(m_Name);
	}
	~CProfile2GPURegion()
	{
		g_Profiler2.RecordGPURegionLeave(m_Name);
	}
private:
	const char* m_Name;
};

/**
 * Starts timing from now until the end of the current scope.
 * @p region is the name to associate with this region (should be
 * a constant string literal; the pointer must remain valid forever).
 * Regions may be nested, but preferably shouldn't be nested deeply since
 * it hurts the visualisation.
 */
#define PROFILE2(region) CProfile2Region profile2__(region)

#define PROFILE2_IFSPIKE(region, limit) CProfile2SpikeRegion profile2__(region, limit)

#define PROFILE2_AGGREGATED(region, limit) CProfile2AggregatedRegion profile2__(region, limit)

#define PROFILE2_GPU(region) CProfile2GPURegion profile2gpu__(region)

/**
 * Record the named event at the current time.
 */
#define PROFILE2_EVENT(name) g_Profiler2.RecordEvent(name)

/**
 * Associates a string (with printf-style formatting) with the current
 * region or event.
 * (If the last profiler call was PROFILE2_EVENT, it associates with that
 * event; otherwise it associates with the currently-active region.)
 */
#define PROFILE2_ATTR g_Profiler2.RecordAttribute

#endif // INCLUDED_PROFILER2
