/* Copyright (C) 2016 Wildfire Games.
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

#include "ScriptRuntime.h"

#include "ps/GameSetup/Config.h"
#include "ps/Profile.h"


void GCSliceCallbackHook(JSRuntime* UNUSED(rt), JS::GCProgress progress, const JS::GCDescription& UNUSED(desc))
{
	/*
	 * During non-incremental GC, the GC is bracketed by JSGC_CYCLE_BEGIN/END
	 * callbacks. During an incremental GC, the sequence of callbacks is as
	 * follows:
	 *   JSGC_CYCLE_BEGIN, JSGC_SLICE_END  (first slice)
	 *   JSGC_SLICE_BEGIN, JSGC_SLICE_END  (second slice)
	 *   ...
	 *   JSGC_SLICE_BEGIN, JSGC_CYCLE_END  (last slice)
	*/


	if (progress == JS::GC_SLICE_BEGIN)
	{
		if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
			g_Profiler.Start("GCSlice");
		g_Profiler2.RecordRegionEnter("GCSlice");
	}
	else if (progress == JS::GC_SLICE_END)
	{
		if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
			g_Profiler.Stop();
    	g_Profiler2.RecordRegionLeave();
	}
	else if (progress == JS::GC_CYCLE_BEGIN)
	{
		if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
			g_Profiler.Start("GCSlice");
		g_Profiler2.RecordRegionEnter("GCSlice");
	}
	else if (progress == JS::GC_CYCLE_END)
	{
		if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
			g_Profiler.Stop();
    	g_Profiler2.RecordRegionLeave();
	}

	// The following code can be used to print some information aobut garbage collection
	// Search for "Nonincremental reason" if there are problems running GC incrementally.
	#if 0
	if (progress == JS::GCProgress::GC_CYCLE_BEGIN)
		printf("starting cycle ===========================================\n");

	const char16_t* str = desc.formatMessage(rt);
	int len = 0;
	
	for(int i = 0; i < 10000; i++)
	{
		len++;
		if(!str[i])
			break;
	}
	
	wchar_t outstring[len];
	
	for(int i = 0; i < len; i++)
	{
		outstring[i] = (wchar_t)str[i];
	}
	
	printf("---------------------------------------\n: %ls \n---------------------------------------\n", outstring);
	#endif
}

void ScriptRuntime::GCCallback(JSRuntime* UNUSED(rt), JSGCStatus status, void *data)
{
	if (status == JSGC_END)
		reinterpret_cast<ScriptRuntime*>(data)->GCCallbackMember();
}

void ScriptRuntime::GCCallbackMember()
{
	m_FinalizationListObjectIdCache.clear();
}

void ScriptRuntime::AddDeferredFinalizationObject(const std::shared_ptr<void>& obj)
{
	m_FinalizationListObjectIdCache.push_back(obj);
}

bool ScriptRuntime::m_Initialized = false;

ScriptRuntime::ScriptRuntime(shared_ptr<ScriptRuntime> parentRuntime, int runtimeSize, int heapGrowthBytesGCTrigger):
	m_LastGCBytes(0),
	m_LastGCCheck(0.0f),
	m_HeapGrowthBytesGCTrigger(heapGrowthBytesGCTrigger),
	m_RuntimeSize(runtimeSize)
{
	if (!m_Initialized)
	{
		ENSURE(JS_Init());
		m_Initialized = true;
	}

	JSRuntime* parentJSRuntime = parentRuntime ? parentRuntime->m_rt : nullptr;
	m_rt = JS_NewRuntime(runtimeSize, JS_USE_HELPER_THREADS, parentJSRuntime);
	ENSURE(m_rt); // TODO: error handling

	JS::SetGCSliceCallback(m_rt, GCSliceCallbackHook);
	JS_SetGCCallback(m_rt, ScriptRuntime::GCCallback, this);

	JS_SetGCParameter(m_rt, JSGC_MAX_MALLOC_BYTES, m_RuntimeSize);
	JS_SetGCParameter(m_rt, JSGC_MAX_BYTES, m_RuntimeSize);
	JS_SetGCParameter(m_rt, JSGC_MODE, JSGC_MODE_INCREMENTAL);

	// The whole heap-growth mechanism seems to work only for non-incremental GCs.
	// We disable it to make it more clear if full GCs happen triggered by this JSAPI internal mechanism.
	JS_SetGCParameter(m_rt, JSGC_DYNAMIC_HEAP_GROWTH, false);
	
	m_dummyContext = JS_NewContext(m_rt, STACK_CHUNK_SIZE);
	ENSURE(m_dummyContext);
}

ScriptRuntime::~ScriptRuntime()
{
	JS_DestroyContext(m_dummyContext);
	JS_SetGCCallback(m_rt, nullptr, nullptr);
	JS_DestroyRuntime(m_rt);
	ENSURE(m_FinalizationListObjectIdCache.empty() && "Leak: Removing callback while some objects still aren't finalized!");
}

void ScriptRuntime::RegisterContext(JSContext* cx)
{
	m_Contexts.push_back(cx);
}

void ScriptRuntime::UnRegisterContext(JSContext* cx)
{
	m_Contexts.remove(cx);
}

#define GC_DEBUG_PRINT 0
void ScriptRuntime::MaybeIncrementalGC(double delay)
{
	PROFILE2("MaybeIncrementalGC");
	
	if (JS::IsIncrementalGCEnabled(m_rt))
	{
		// The idea is to get the heap size after a completed GC and trigger the next GC when the heap size has
		// reached m_LastGCBytes + X. 
		// In practice it doesn't quite work like that. When the incremental marking is completed, the sweeping kicks in.
		// The sweeping actually frees memory and it does this in a background thread (if JS_USE_HELPER_THREADS is set).
		// While the sweeping is happening we already run scripts again and produce new garbage.

		const int GCSliceTimeBudget = 30; // Milliseconds an incremental slice is allowed to run
		
		// Have a minimum time in seconds to wait between GC slices and before starting a new GC to distribute the GC 
		// load and to hopefully make it unnoticeable for the player. This value should be high enough to distribute 
		// the load well enough and low enough to make sure we don't run out of memory before we can start with the 
		// sweeping.
		if (timer_Time() - m_LastGCCheck < delay)
			return;
		
		m_LastGCCheck = timer_Time();
		
		int gcBytes = JS_GetGCParameter(m_rt, JSGC_BYTES);
		
#if GC_DEBUG_PRINT
			std::cout << "gcBytes: " << gcBytes / 1024 << " KB" << std::endl;
#endif
		
		if (m_LastGCBytes > gcBytes || m_LastGCBytes == 0)
		{
#if GC_DEBUG_PRINT
			printf("Setting m_LastGCBytes: %d KB \n", gcBytes / 1024);
#endif
			m_LastGCBytes = gcBytes;
		}

		// Run an additional incremental GC slice if the currently running incremental GC isn't over yet 
		// ... or
		// start a new incremental GC if the JS heap size has grown enough for a GC to make sense
		if (JS::IsIncrementalGCInProgress(m_rt) || (gcBytes - m_LastGCBytes > m_HeapGrowthBytesGCTrigger))
		{				
#if GC_DEBUG_PRINT
			if (JS::IsIncrementalGCInProgress(m_rt))
				printf("An incremental GC cycle is in progress. \n");
			else
				printf("GC needed because JSGC_BYTES - m_LastGCBytes > m_HeapGrowthBytesGCTrigger \n"
					"    JSGC_BYTES: %d KB \n    m_LastGCBytes: %d KB \n    m_HeapGrowthBytesGCTrigger: %d KB \n", 
					gcBytes / 1024, 
					m_LastGCBytes / 1024, 
					m_HeapGrowthBytesGCTrigger / 1024);
#endif
			
			// A hack to make sure we never exceed the runtime size because we can't collect the memory
			// fast enough.
			if (gcBytes > m_RuntimeSize / 2)
			{
				if (JS::IsIncrementalGCInProgress(m_rt))
				{
#if GC_DEBUG_PRINT
					printf("Finishing incremental GC because gcBytes > m_RuntimeSize / 2. \n");
#endif
					PrepareContextsForIncrementalGC();
					JS::FinishIncrementalGC(m_rt, JS::gcreason::REFRESH_FRAME);
				}
				else
				{
					if (gcBytes > m_RuntimeSize * 0.75)
					{
						ShrinkingGC();
#if GC_DEBUG_PRINT
						printf("Running shrinking GC because gcBytes > m_RuntimeSize * 0.75. \n");
#endif
					}
					else
					{
#if GC_DEBUG_PRINT
						printf("Running full GC because gcBytes > m_RuntimeSize / 2. \n");
#endif
						JS_GC(m_rt);
					}
				}
			}
			else
			{
#if GC_DEBUG_PRINT
				if (!JS::IsIncrementalGCInProgress(m_rt))
					printf("Starting incremental GC \n");
				else
					printf("Running incremental GC slice \n");
#endif
				PrepareContextsForIncrementalGC();
				JS::IncrementalGC(m_rt, JS::gcreason::REFRESH_FRAME, GCSliceTimeBudget);
			}
			m_LastGCBytes = gcBytes;
		}
	}
}

void ScriptRuntime::ShrinkingGC()
{
	JS_SetGCParameter(m_rt, JSGC_MODE, JSGC_MODE_COMPARTMENT);
	JS::PrepareForFullGC(m_rt);
	JS::ShrinkingGC(m_rt, JS::gcreason::REFRESH_FRAME);
	JS_SetGCParameter(m_rt, JSGC_MODE, JSGC_MODE_INCREMENTAL);
}

void ScriptRuntime::PrepareContextsForIncrementalGC()
{
	for (JSContext* const& ctx : m_Contexts)
		JS::PrepareZoneForGC(js::GetCompartmentZone(js::GetContextCompartment(ctx)));
}
