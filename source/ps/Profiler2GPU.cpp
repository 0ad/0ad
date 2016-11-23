/* Copyright (c) 2014 Wildfire Games
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

#include "Profiler2GPU.h"

#include "lib/ogl.h"
#include "lib/allocators/shared_ptr.h"
#include "ps/ConfigDB.h"
#include "ps/Profiler2.h"

#if !CONFIG2_GLES

class CProfiler2GPU_base
{
	NONCOPYABLE(CProfiler2GPU_base);

protected:
	CProfiler2GPU_base(CProfiler2& profiler, const char* name) :
		m_Profiler(profiler), m_Storage(profiler, name)
	{
		m_Storage.RecordSyncMarker(m_Profiler.GetTime());
		m_Storage.Record(CProfiler2::ITEM_EVENT, m_Profiler.GetTime(), "thread start");

		m_Profiler.AddThreadStorage(&m_Storage);
	}

	~CProfiler2GPU_base()
	{
		m_Profiler.RemoveThreadStorage(&m_Storage);
	}

	CProfiler2& m_Profiler;
	CProfiler2::ThreadStorage m_Storage;
};

//////////////////////////////////////////////////////////////////////////

// Base class for ARB_timer_query, EXT_timer_query
class CProfiler2GPU_timer_query : public CProfiler2GPU_base
{
protected:
	CProfiler2GPU_timer_query(CProfiler2& profiler, const char* name) :
		CProfiler2GPU_base(profiler, name)
	{
	}

	~CProfiler2GPU_timer_query()
	{
		if (!m_FreeQueries.empty())
			pglDeleteQueriesARB(m_FreeQueries.size(), &m_FreeQueries[0]);
		ogl_WarnIfError();
	}

	// Returns a new GL query object (or a recycled old one)
	GLuint NewQuery()
	{
		if (m_FreeQueries.empty())
		{
			// Generate a batch of new queries
			m_FreeQueries.resize(8);
			pglGenQueriesARB(m_FreeQueries.size(), &m_FreeQueries[0]);
			ogl_WarnIfError();
		}

		GLuint query = m_FreeQueries.back();
		m_FreeQueries.pop_back();
		return query;
	}

	std::vector<GLuint> m_FreeQueries; // query objects that are allocated but not currently in used
};

//////////////////////////////////////////////////////////////////////////

/*
 * GL_ARB_timer_query supports sync and async queries for absolute GPU
 * timestamps, which lets us time regions of code relative to the CPU.
 * At the start of a frame, we record the CPU time and sync GPU timestamp,
 * giving the time-vs-timestamp offset.
 * At each enter/leave-region event, we do an async GPU timestamp query.
 * When all the queries for a frame have their results available,
 * we convert their GPU timestamps into CPU times and record the data.
 */
class CProfiler2GPU_ARB_timer_query : public CProfiler2GPU_timer_query
{
	struct SEvent
	{
		const char* id;
		GLuint query;
		bool isEnter; // true if entering region; false if leaving
	};

	struct SFrame
	{
		u32 num;

		double syncTimeStart; // CPU time at start of maybe this frame or a recent one
		GLint64 syncTimestampStart; // GL timestamp corresponding to timeStart

		std::vector<SEvent> events;
	};

	std::deque<SFrame> m_Frames;

public:
	static bool IsSupported()
	{
		return ogl_HaveExtension("GL_ARB_timer_query");
	}

	CProfiler2GPU_ARB_timer_query(CProfiler2& profiler) :
		CProfiler2GPU_timer_query(profiler, "gpu_arb")
	{
		// TODO: maybe we should check QUERY_COUNTER_BITS to ensure it's
		// high enough (but apparently it might trigger GL errors on ATI)
	}

	~CProfiler2GPU_ARB_timer_query()
	{
		// Pop frames to return queries to the free list
		while (!m_Frames.empty())
			PopFrontFrame();
	}

	void FrameStart()
	{
		ProcessFrames();

		SFrame frame;
		frame.num = m_Profiler.GetFrameNumber();

		// On (at least) some NVIDIA Windows drivers, when GPU-bound, or when
		// vsync enabled and not CPU-bound, the first glGet* call at the start
		// of a frame appears to trigger a wait (to stop the GPU getting too
		// far behind, or to wait for the vsync period).
		// That will be this GL_TIMESTAMP get, which potentially distorts the
		// reported results. So we'll only do it fairly rarely, and for most
		// frames we'll just assume the clocks don't drift much
		
		const double RESYNC_PERIOD = 1.0; // seconds

		double now = m_Profiler.GetTime();

		if (m_Frames.empty() || now > m_Frames.back().syncTimeStart + RESYNC_PERIOD)
		{
			PROFILE2("profile timestamp resync");

			pglGetInteger64v(GL_TIMESTAMP, &frame.syncTimestampStart);
			ogl_WarnIfError();

			frame.syncTimeStart = m_Profiler.GetTime();
			// (Have to do GetTime again after GL_TIMESTAMP, because GL_TIMESTAMP
			// might wait a while before returning its now-current timestamp)
		}
		else
		{
			// Reuse the previous frame's sync data
			frame.syncTimeStart = m_Frames[m_Frames.size()-1].syncTimeStart;
			frame.syncTimestampStart = m_Frames[m_Frames.size()-1].syncTimestampStart;
		}

		m_Frames.push_back(frame);

		RegionEnter("frame");
	}

	void FrameEnd()
	{
		RegionLeave("frame");
	}

	void RecordRegion(const char* id, bool isEnter)
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.back();

		SEvent event;
		event.id = id;
		event.query = NewQuery();
		event.isEnter = isEnter;

		pglQueryCounter(event.query, GL_TIMESTAMP);
		ogl_WarnIfError();

		frame.events.push_back(event);
	}

	void RegionEnter(const char* id)
	{
		RecordRegion(id, true);
	}

	void RegionLeave(const char* id)
	{
		RecordRegion(id, false);
	}

private:

	void ProcessFrames()
	{
		while (!m_Frames.empty())
		{
			SFrame& frame = m_Frames.front();

			// Queries become available in order so we only need to check the last one
			GLint available = 0;
			pglGetQueryObjectivARB(frame.events.back().query, GL_QUERY_RESULT_AVAILABLE, &available);
			ogl_WarnIfError();
			if (!available)
				break;

			// The frame's queries are now available, so retrieve and record all their results:

			for (size_t i = 0; i < frame.events.size(); ++i)
			{
				GLuint64 queryTimestamp = 0;
				pglGetQueryObjectui64v(frame.events[i].query, GL_QUERY_RESULT, &queryTimestamp);
					// (use the non-suffixed function here, as defined by GL_ARB_timer_query)
				ogl_WarnIfError();

				// Convert to absolute CPU-clock time
				double t = frame.syncTimeStart + (double)(queryTimestamp - frame.syncTimestampStart) / 1e9;

				// Record a frame-start for syncing
				if (i == 0)
					m_Storage.RecordFrameStart(t);

				if (frame.events[i].isEnter)
					m_Storage.Record(CProfiler2::ITEM_ENTER, t, frame.events[i].id);
				else
					m_Storage.RecordLeave(t);

				// Associate the frame number with the "frame" region
				if (i == 0)
					m_Storage.RecordAttributePrintf("%u", frame.num);
			}

			PopFrontFrame();
		}
	}

	void PopFrontFrame()
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.front();
		for (size_t i = 0; i < frame.events.size(); ++i)
			m_FreeQueries.push_back(frame.events[i].query);
		m_Frames.pop_front();
	}
};

//////////////////////////////////////////////////////////////////////////

/*
 * GL_EXT_timer_query only supports async queries for elapsed time,
 * and only a single simultaneous query.
 * We can't correctly convert it to absolute time, so we just pretend
 * each GPU frame starts the same time as the CPU for that frame.
 * We do a query for elapsed time between every adjacent enter/leave-region event.
 * When all the queries for a frame have their results available,
 * we sum the elapsed times to calculate when each event occurs within the
 * frame, and record the data.
 */
class CProfiler2GPU_EXT_timer_query : public CProfiler2GPU_timer_query
{
	struct SEvent
	{
		const char* id;
		GLuint query; // query for time elapsed from this event until the next, or 0 for final event
		bool isEnter; // true if entering region; false if leaving
	};

	struct SFrame
	{
		u32 num;
		double timeStart; // CPU time at frame start
		std::vector<SEvent> events;
	};

	std::deque<SFrame> m_Frames;

public:
	static bool IsSupported()
	{
		return ogl_HaveExtension("GL_EXT_timer_query");
	}

	CProfiler2GPU_EXT_timer_query(CProfiler2& profiler) :
		CProfiler2GPU_timer_query(profiler, "gpu_ext")
	{
	}

	~CProfiler2GPU_EXT_timer_query()
	{
		// Pop frames to return queries to the free list
		while (!m_Frames.empty())
			PopFrontFrame();
	}

	void FrameStart()
	{
		ProcessFrames();

		SFrame frame;
		frame.num = m_Profiler.GetFrameNumber();
		frame.timeStart = m_Profiler.GetTime();

		m_Frames.push_back(frame);

		RegionEnter("frame");
	}

	void FrameEnd()
	{
		RegionLeave("frame");

		pglEndQueryARB(GL_TIME_ELAPSED);
		ogl_WarnIfError();
	}

	void RecordRegion(const char* id, bool isEnter)
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.back();

		// Must call glEndQuery before calling glGenQueries (via NewQuery),
		// for compatibility with the GL_EXT_timer_query spec (which says
		// GL_INVALID_OPERATION if a query of any target is active; the ARB
		// spec and OpenGL specs don't appear to say that, but the AMD drivers
		// implement that error (see Trac #1033))

		if (!frame.events.empty())
		{
			pglEndQueryARB(GL_TIME_ELAPSED);
			ogl_WarnIfError();
		}

		SEvent event;
		event.id = id;
		event.query = NewQuery();
		event.isEnter = isEnter;

		pglBeginQueryARB(GL_TIME_ELAPSED, event.query);
		ogl_WarnIfError();

		frame.events.push_back(event);
	}

	void RegionEnter(const char* id)
	{
		RecordRegion(id, true);
	}

	void RegionLeave(const char* id)
	{
		RecordRegion(id, false);
	}

private:
	void ProcessFrames()
	{
		while (!m_Frames.empty())
		{
			SFrame& frame = m_Frames.front();

			// Queries become available in order so we only need to check the last one
			GLint available = 0;
			pglGetQueryObjectivARB(frame.events.back().query, GL_QUERY_RESULT_AVAILABLE, &available);
			ogl_WarnIfError();
			if (!available)
				break;

			// The frame's queries are now available, so retrieve and record all their results:

			double t = frame.timeStart;
			m_Storage.RecordFrameStart(t);

			for (size_t i = 0; i < frame.events.size(); ++i)
			{
				if (frame.events[i].isEnter)
					m_Storage.Record(CProfiler2::ITEM_ENTER, t, frame.events[i].id);
				else
					m_Storage.RecordLeave(t);

				// Associate the frame number with the "frame" region
				if (i == 0)
					m_Storage.RecordAttributePrintf("%u", frame.num);

				// Advance by the elapsed time to the next event
				GLuint64 queryElapsed = 0;
				pglGetQueryObjectui64vEXT(frame.events[i].query, GL_QUERY_RESULT, &queryElapsed);
					// (use the EXT-suffixed function here, as defined by GL_EXT_timer_query)
				ogl_WarnIfError();
				t += (double)queryElapsed / 1e9;
			}

			PopFrontFrame();
		}
	}

	void PopFrontFrame()
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.front();
		for (size_t i = 0; i < frame.events.size(); ++i)
			m_FreeQueries.push_back(frame.events[i].query);
		m_Frames.pop_front();
	}
};

//////////////////////////////////////////////////////////////////////////

/*
 * GL_INTEL_performance_queries is not officially documented
 * (see http://zaynar.co.uk/docs/gl-intel-performance-queries.html)
 * but it's potentially useful so we'll support it anyway.
 * It supports async queries giving elapsed time plus a load of other
 * counters that we'd like to use, and supports many simultaneous queries
 * (unlike GL_EXT_timer_query).
 * There are multiple query types (typically 2), each with its own set of
 * multiple counters.
 * On each enter-region event, we start a new set of queries.
 * On each leave-region event, we end the corresponding set of queries.
 * We can't tell the offsets between the enter events of nested regions,
 * so we pretend they all got entered at the same time.
 */
class CProfiler2GPU_INTEL_performance_queries : public CProfiler2GPU_base
{
	struct SEvent
	{
		const char* id;
		bool isEnter;
		std::vector<GLuint> queries; // if isEnter, one per SPerfQueryType; else empty
	};

	struct SFrame
	{
		u32 num;
		double timeStart; // CPU time at frame start
		std::vector<SEvent> events;
		std::vector<size_t> activeRegions; // stack of indexes into events
	};

	std::deque<SFrame> m_Frames;

	// Counters listed by the graphics driver for a particular query type
	struct SPerfCounter
	{
		std::string name;
		std::string desc;
		GLuint offset;
		GLuint size;
		GLuint type;
	};

	// Query types listed by the graphics driver
	struct SPerfQueryType
	{
		GLuint queryTypeId;
		std::string name;
		GLuint counterBufferSize;
		std::vector<SPerfCounter> counters;

		std::vector<GLuint> freeQueries; // query objects that are allocated but not currently in use
	};

	std::vector<SPerfQueryType> m_QueryTypes;

	#define INTEL_PERFQUERIES_NONBLOCK             0x83FA
	#define INTEL_PERFQUERIES_BLOCK	               0x83FB
	#define INTEL_PERFQUERIES_TYPE_UNSIGNED_INT    0x9402
	#define INTEL_PERFQUERIES_TYPE_UNSIGNED_INT64  0x9403
	#define INTEL_PERFQUERIES_TYPE_FLOAT           0x9404
	#define INTEL_PERFQUERIES_TYPE_BOOL            0x9406

public:
	static bool IsSupported()
	{
		return ogl_HaveExtension("GL_INTEL_performance_queries");
	}

	CProfiler2GPU_INTEL_performance_queries(CProfiler2& profiler) :
		 CProfiler2GPU_base(profiler, "gpu_intel")
	{
		LoadPerfCounters();
	}

	~CProfiler2GPU_INTEL_performance_queries()
	{
		// Pop frames to return queries to the free list
		while (!m_Frames.empty())
			PopFrontFrame();

		for (size_t i = 0; i < m_QueryTypes.size(); ++i)
			for (size_t j = 0; j < m_QueryTypes[i].freeQueries.size(); ++j)
				pglDeletePerfQueryINTEL(m_QueryTypes[i].freeQueries[j]);

		ogl_WarnIfError();
	}

	void FrameStart()
	{
		ProcessFrames();

		SFrame frame;
		frame.num = m_Profiler.GetFrameNumber();
		frame.timeStart = m_Profiler.GetTime();

		m_Frames.push_back(frame);

		RegionEnter("frame");
	}

	void FrameEnd()
	{
		RegionLeave("frame");
	}

	void RegionEnter(const char* id)
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.back();

		SEvent event;
		event.id = id;
		event.isEnter = true;

		for (size_t i = 0; i < m_QueryTypes.size(); ++i)
		{
			GLuint id = NewQuery(i);
			pglBeginPerfQueryINTEL(id);
			ogl_WarnIfError();
			event.queries.push_back(id);
		}

		frame.activeRegions.push_back(frame.events.size());

		frame.events.push_back(event);
	}

	void RegionLeave(const char* id)
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.back();

		ENSURE(!frame.activeRegions.empty());
		SEvent& activeEvent = frame.events[frame.activeRegions.back()];

		for (size_t i = 0; i < m_QueryTypes.size(); ++i)
		{
			pglEndPerfQueryINTEL(activeEvent.queries[i]);
			ogl_WarnIfError();
		}

		frame.activeRegions.pop_back();

		SEvent event;
		event.id = id;
		event.isEnter = false;
		frame.events.push_back(event);
	}

private:
	GLuint NewQuery(size_t queryIdx)
	{
		ENSURE(queryIdx < m_QueryTypes.size());

		if (m_QueryTypes[queryIdx].freeQueries.empty())
		{
			GLuint id;
			pglCreatePerfQueryINTEL(m_QueryTypes[queryIdx].queryTypeId, &id);
			ogl_WarnIfError();
			return id;
		}

		GLuint id = m_QueryTypes[queryIdx].freeQueries.back();
		m_QueryTypes[queryIdx].freeQueries.pop_back();
		return id;
	}

	void ProcessFrames()
	{
		while (!m_Frames.empty())
		{
			SFrame& frame = m_Frames.front();

			// Queries don't become available in order, so check them all before
			// trying to read the results from any
			for (size_t j = 0; j < m_QueryTypes.size(); ++j)
			{
				size_t size = m_QueryTypes[j].counterBufferSize;
				shared_ptr<char> buf(new char[size], ArrayDeleter());

				for (size_t i = 0; i < frame.events.size(); ++i)
				{
					if (!frame.events[i].isEnter)
						continue;

					GLuint length = 0;
					pglGetPerfQueryDataINTEL(frame.events[i].queries[j], INTEL_PERFQUERIES_NONBLOCK, size, buf.get(), &length);
					ogl_WarnIfError();
					if (length == 0)
						return;
				}
			}

			double lastTime = frame.timeStart;
			std::stack<double> endTimes;

			m_Storage.RecordFrameStart(frame.timeStart);

			for (size_t i = 0; i < frame.events.size(); ++i)
			{
				if (frame.events[i].isEnter)
				{
					m_Storage.Record(CProfiler2::ITEM_ENTER, lastTime, frame.events[i].id);

					if (i == 0)
						m_Storage.RecordAttributePrintf("%u", frame.num);

					double elapsed = 0.0;

					for (size_t j = 0; j < m_QueryTypes.size(); ++j)
					{
						GLuint length;
						char* buf = new char[m_QueryTypes[j].counterBufferSize];
						pglGetPerfQueryDataINTEL(frame.events[i].queries[j], INTEL_PERFQUERIES_BLOCK, m_QueryTypes[j].counterBufferSize, buf, &length);
						ogl_WarnIfError();
						ENSURE(length == m_QueryTypes[j].counterBufferSize);

						m_Storage.RecordAttributePrintf("-- %s --", m_QueryTypes[j].name.c_str());

						for (size_t k = 0; k < m_QueryTypes[j].counters.size(); ++k)
						{
							SPerfCounter& counter = m_QueryTypes[j].counters[k];

							if (counter.type == INTEL_PERFQUERIES_TYPE_UNSIGNED_INT)
							{
								ENSURE(counter.size == 4);
								GLuint value = 0;
								memcpy(&value, buf + counter.offset, counter.size);
								m_Storage.RecordAttributePrintf("%s: %u", counter.name.c_str(), value);
							}
							else if (counter.type == INTEL_PERFQUERIES_TYPE_UNSIGNED_INT64)
							{
								ENSURE(counter.size == 8);
								GLuint64 value = 0;
								memcpy(&value, buf + counter.offset, counter.size);
								m_Storage.RecordAttributePrintf("%s: %.0f", counter.name.c_str(), (double)value);

								if (counter.name == "TotalTime")
									elapsed = (double)value / 1e6;
							}
							else if (counter.type == INTEL_PERFQUERIES_TYPE_FLOAT)
							{
								ENSURE(counter.size == 4);
								GLfloat value = 0;
								memcpy(&value, buf + counter.offset, counter.size);
								m_Storage.RecordAttributePrintf("%s: %f", counter.name.c_str(), value);
							}
							else if (counter.type == INTEL_PERFQUERIES_TYPE_BOOL)
							{
								ENSURE(counter.size == 4);
								GLuint value = 0;
								memcpy(&value, buf + counter.offset, counter.size);
								ENSURE(value == 0 || value == 1);
								m_Storage.RecordAttributePrintf("%s: %u", counter.name.c_str(), value);
							}
							else
							{
								//debug_warn(L"unrecognised Intel performance counter type");
							}
						}

						delete[] buf;
					}

					endTimes.push(lastTime + elapsed);
				}
				else
				{
					lastTime = endTimes.top();
					endTimes.pop();
					m_Storage.RecordLeave(lastTime);
				}
			}

			PopFrontFrame();
		}
	}

	void PopFrontFrame()
	{
		ENSURE(!m_Frames.empty());
		SFrame& frame = m_Frames.front();
		for (size_t i = 0; i < frame.events.size(); ++i)
			if (frame.events[i].isEnter)
				for (size_t j = 0; j < m_QueryTypes.size(); ++j)
					m_QueryTypes[j].freeQueries.push_back(frame.events[i].queries[j]);
		m_Frames.pop_front();
	}

	void LoadPerfCounters()
	{
		GLuint queryTypeId;
		pglGetFirstPerfQueryIdINTEL(&queryTypeId);
		ogl_WarnIfError();
		do
		{
			char queryName[256];
			GLuint counterBufferSize, numCounters, maxQueries, unknown;
			pglGetPerfQueryInfoINTEL(queryTypeId, ARRAY_SIZE(queryName), queryName, &counterBufferSize, &numCounters, &maxQueries, &unknown);
			ogl_WarnIfError();
			ENSURE(unknown == 1);

			SPerfQueryType query;
			query.queryTypeId = queryTypeId;
			query.name = queryName;
			query.counterBufferSize = counterBufferSize;

			for (GLuint counterId = 1; counterId <= numCounters; ++counterId)
			{
				char counterName[256];
				char counterDesc[2048];
				GLuint counterOffset, counterSize, counterUsage, counterType;
				GLuint64 unknown2;
				pglGetPerfCounterInfoINTEL(queryTypeId, counterId, ARRAY_SIZE(counterName), counterName, ARRAY_SIZE(counterDesc), counterDesc, &counterOffset, &counterSize, &counterUsage, &counterType, &unknown2);
				ogl_WarnIfError();
				ENSURE(unknown2 == 0 || unknown2 == 1);

				SPerfCounter counter;
				counter.name = counterName;
				counter.desc = counterDesc;
				counter.offset = counterOffset;
				counter.size = counterSize;
				counter.type = counterType;
				query.counters.push_back(counter);
			}

			m_QueryTypes.push_back(query);

			pglGetNextPerfQueryIdINTEL(queryTypeId, &queryTypeId);
			ogl_WarnIfError();

		} while (queryTypeId);
	}
};

//////////////////////////////////////////////////////////////////////////

CProfiler2GPU::CProfiler2GPU(CProfiler2& profiler) :
	m_Profiler(profiler), m_ProfilerARB(NULL), m_ProfilerEXT(NULL), m_ProfilerINTEL(NULL)
{
	bool enabledARB = false;
	bool enabledEXT = false;
	bool enabledINTEL = false;
	CFG_GET_VAL("profiler2.gpu.arb.enable", enabledARB);
	CFG_GET_VAL("profiler2.gpu.ext.enable", enabledEXT);
	CFG_GET_VAL("profiler2.gpu.intel.enable", enabledINTEL);

	// Only enable either ARB or EXT, not both, because they are redundant
	// (EXT is only needed for compatibility with older systems), and because
	// using both triggers GL_INVALID_OPERATION on AMD drivers (see comment
	// in CProfiler2GPU_EXT_timer_query::RecordRegion)
	if (enabledARB && CProfiler2GPU_ARB_timer_query::IsSupported())
	{
		m_ProfilerARB = new CProfiler2GPU_ARB_timer_query(m_Profiler);
	}
	else if (enabledEXT && CProfiler2GPU_EXT_timer_query::IsSupported())
	{
		m_ProfilerEXT = new CProfiler2GPU_EXT_timer_query(m_Profiler);
	}

	// The INTEL mode should be compatible with ARB/EXT (though no current
	// drivers support both), and provides complementary data, so enable it
	// when possible
	if (enabledINTEL && CProfiler2GPU_INTEL_performance_queries::IsSupported())
	{
		m_ProfilerINTEL = new CProfiler2GPU_INTEL_performance_queries(m_Profiler);
	}
}

CProfiler2GPU::~CProfiler2GPU()
{
	SAFE_DELETE(m_ProfilerARB);
	SAFE_DELETE(m_ProfilerEXT);
	SAFE_DELETE(m_ProfilerINTEL);
}

void CProfiler2GPU::FrameStart()
{
	if (m_ProfilerARB)
		m_ProfilerARB->FrameStart();

	if (m_ProfilerEXT)
		m_ProfilerEXT->FrameStart();

	if (m_ProfilerINTEL)
		m_ProfilerINTEL->FrameStart();
}

void CProfiler2GPU::FrameEnd()
{
	if (m_ProfilerARB)
		m_ProfilerARB->FrameEnd();

	if (m_ProfilerEXT)
		m_ProfilerEXT->FrameEnd();

	if (m_ProfilerINTEL)
		m_ProfilerINTEL->FrameEnd();
}

void CProfiler2GPU::RegionEnter(const char* id)
{
	if (m_ProfilerARB)
		m_ProfilerARB->RegionEnter(id);

	if (m_ProfilerEXT)
		m_ProfilerEXT->RegionEnter(id);

	if (m_ProfilerINTEL)
		m_ProfilerINTEL->RegionEnter(id);
}

void CProfiler2GPU::RegionLeave(const char* id)
{
	if (m_ProfilerARB)
		m_ProfilerARB->RegionLeave(id);

	if (m_ProfilerEXT)
		m_ProfilerEXT->RegionLeave(id);

	if (m_ProfilerINTEL)
		m_ProfilerINTEL->RegionLeave(id);
}

#else // CONFIG2_GLES

CProfiler2GPU::CProfiler2GPU(CProfiler2& profiler) :
	m_Profiler(profiler), m_ProfilerARB(NULL), m_ProfilerEXT(NULL), m_ProfilerINTEL(NULL)
{
}

CProfiler2GPU::~CProfiler2GPU() { }

void CProfiler2GPU::FrameStart() { }
void CProfiler2GPU::FrameEnd() { }
void CProfiler2GPU::RegionEnter(const char* UNUSED(id)) { }
void CProfiler2GPU::RegionLeave(const char* UNUSED(id)) { }

#endif
