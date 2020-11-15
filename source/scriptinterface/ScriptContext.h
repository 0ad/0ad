/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTCONTEXT
#define INCLUDED_SCRIPTCONTEXT

#include "ScriptTypes.h"
#include "ScriptExtraHeaders.h"

#include <sstream>

constexpr int STACK_CHUNK_SIZE = 8192;

// Those are minimal defaults. The runtime for the main game is larger and GCs upon a larger growth.
constexpr int DEFAULT_CONTEXT_SIZE = 16 * 1024 * 1024;
constexpr int DEFAULT_HEAP_GROWTH_BYTES_GCTRIGGER = 2 * 1024 * 1024;

/**
 * Abstraction around a SpiderMonkey JSRuntime/JSContext.
 *
 * A single ScriptContext, with the associated runtime and context,
 * should only be used on a single thread.
 *
 * (One means to share data between threads and contexts is to create
 * a ScriptInterface::StructuredClone.)
 */

class ScriptContext
{
public:
	ScriptContext(int contextSize, int heapGrowthBytesGCTrigger);
	~ScriptContext();

	/**
	 * Returns a context, in which any number of ScriptInterfaces compartments can live.
	 * Each context should only ever be used on a single thread.
	 * @param parentContext Parent context from the parent thread, with which we share some thread-safe data
	 * @param contextSize Maximum size in bytes of the new context
	 * @param heapGrowthBytesGCTrigger Size in bytes of cumulated allocations after which a GC will be triggered
	 */
	static shared_ptr<ScriptContext> CreateContext(
		int contextSize = DEFAULT_CONTEXT_SIZE,
		int heapGrowthBytesGCTrigger = DEFAULT_HEAP_GROWTH_BYTES_GCTRIGGER);

	/**
	 * MaybeIncrementalGC tries to determine whether a context-wide garbage collection would free up enough memory to
	 * be worth the amount of time it would take. It does this with our own logic and NOT some predefined JSAPI logic because
	 * such functionality currently isn't available out of the box.
	 * It does incremental GC which means it will collect one slice each time it's called until the garbage collection is done.
	 * This can and should be called quite regularly. The delay parameter allows you to specify a minimum time since the last GC
	 * in seconds (the delay should be a fraction of a second in most cases though).
	 * It will only start a new incremental GC or another GC slice if this time is exceeded. The user of this function is
	 * responsible for ensuring that GC can run with a small enough delay to get done with the work.
	 */
	void MaybeIncrementalGC(double delay);
	void ShrinkingGC();

	/**
	 * This is used to keep track of compartments which should be prepared for a GC.
	 */
	void RegisterCompartment(JSCompartment* cmpt);
	void UnRegisterCompartment(JSCompartment* cmpt);

	JSRuntime* GetJSRuntime() const { return m_rt; }

	/**
	 * GetGeneralJSContext returns the context without starting a GC request and without
	 * entering any compartment. It should only be used in specific situations, such as
	 * creating a new compartment, or as an unsafe alternative to GetJSRuntime.
	 * If you need the compartmented context of a ScriptInterface, you should create a
	 * ScriptInterface::Request and use the context from that.
	 */
	JSContext* GetGeneralJSContext() const { return m_cx; }

private:

	JSRuntime* m_rt;
	JSContext* m_cx;

	void PrepareCompartmentsForIncrementalGC() const;
	std::list<JSCompartment*> m_Compartments;

	int m_ContextSize;
	int m_HeapGrowthBytesGCTrigger;
	int m_LastGCBytes;
	double m_LastGCCheck;
};

#endif // INCLUDED_SCRIPTCONTEXT
