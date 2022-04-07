/* Copyright (C) 2022 Wildfire Games.
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
#include "ps/ConfigDB.h"
#include "ps/Profiler2.h"

#include <deque>
#include <stack>
#include <vector>

#if !CONFIG2_GLES

/*
 * GL_ARB_timer_query supports sync and async queries for absolute GPU
 * timestamps, which lets us time regions of code relative to the CPU.
 * At the start of a frame, we record the CPU time and sync GPU timestamp,
 * giving the time-vs-timestamp offset.
 * At each enter/leave-region event, we do an async GPU timestamp query.
 * When all the queries for a frame have their results available,
 * we convert their GPU timestamps into CPU times and record the data.
 */
class CProfiler2GPUARB
{
	NONCOPYABLE(CProfiler2GPUARB);

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

	CProfiler2GPUARB(CProfiler2& profiler)
		: m_Profiler(profiler), m_Storage(*new CProfiler2::ThreadStorage(profiler, "gpu_arb"))
	{
		// TODO: maybe we should check QUERY_COUNTER_BITS to ensure it's
		// high enough (but apparently it might trigger GL errors on ATI)

		m_Storage.RecordSyncMarker(m_Profiler.GetTime());
		m_Storage.Record(CProfiler2::ITEM_EVENT, m_Profiler.GetTime(), "thread start");

		m_Profiler.AddThreadStorage(&m_Storage);
	}

	~CProfiler2GPUARB()
	{
		// Pop frames to return queries to the free list
		while (!m_Frames.empty())
			PopFrontFrame();

		if (!m_FreeQueries.empty())
			glDeleteQueriesARB(m_FreeQueries.size(), &m_FreeQueries[0]);
		ogl_WarnIfError();

		m_Profiler.RemoveThreadStorage(&m_Storage);
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

			glGetInteger64v(GL_TIMESTAMP, &frame.syncTimestampStart);
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

		glQueryCounter(event.query, GL_TIMESTAMP);
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
			glGetQueryObjectivARB(frame.events.back().query, GL_QUERY_RESULT_AVAILABLE, &available);
			ogl_WarnIfError();
			if (!available)
				break;

			// The frame's queries are now available, so retrieve and record all their results:

			for (size_t i = 0; i < frame.events.size(); ++i)
			{
				GLuint64 queryTimestamp = 0;
				glGetQueryObjectui64v(frame.events[i].query, GL_QUERY_RESULT, &queryTimestamp);
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

	// Returns a new GL query object (or a recycled old one)
	GLuint NewQuery()
	{
		if (m_FreeQueries.empty())
		{
			// Generate a batch of new queries
			m_FreeQueries.resize(8);
			glGenQueriesARB(m_FreeQueries.size(), &m_FreeQueries[0]);
			ogl_WarnIfError();
		}

		GLuint query = m_FreeQueries.back();
		m_FreeQueries.pop_back();
		return query;
	}

	CProfiler2& m_Profiler;
	CProfiler2::ThreadStorage& m_Storage;

	std::vector<GLuint> m_FreeQueries; // query objects that are allocated but not currently in used
};

CProfiler2GPU::CProfiler2GPU(CProfiler2& profiler) :
	m_Profiler(profiler)
{
	bool enabledARB = false;
	CFG_GET_VAL("profiler2.gpu.arb.enable", enabledARB);

	if (enabledARB && CProfiler2GPUARB::IsSupported())
	{
		m_ProfilerARB = std::make_unique<CProfiler2GPUARB>(m_Profiler);
	}
}

CProfiler2GPU::~CProfiler2GPU() = default;

void CProfiler2GPU::FrameStart()
{
	if (m_ProfilerARB)
		m_ProfilerARB->FrameStart();
}

void CProfiler2GPU::FrameEnd()
{
	if (m_ProfilerARB)
		m_ProfilerARB->FrameEnd();
}

void CProfiler2GPU::RegionEnter(const char* id)
{
	if (m_ProfilerARB)
		m_ProfilerARB->RegionEnter(id);
}

void CProfiler2GPU::RegionLeave(const char* id)
{
	if (m_ProfilerARB)
		m_ProfilerARB->RegionLeave(id);
}

#else // CONFIG2_GLES

CProfiler2GPU::CProfiler2GPU(CProfiler2& profiler) :
	m_Profiler(profiler)
{
}

CProfiler2GPU::~CProfiler2GPU() = default;

void CProfiler2GPU::FrameStart() { }
void CProfiler2GPU::FrameEnd() { }
void CProfiler2GPU::RegionEnter(const char* UNUSED(id)) { }
void CProfiler2GPU::RegionLeave(const char* UNUSED(id)) { }

#endif
