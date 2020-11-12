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

#ifndef INCLUDED_SCRIPTRUNTIME
#define INCLUDED_SCRIPTRUNTIME

#include "ScriptTypes.h"
#include "ScriptExtraHeaders.h"

#include <sstream>

#define STACK_CHUNK_SIZE 8192

/**
 * Abstraction around a SpiderMonkey JSRuntime.
 * Each ScriptRuntime can be used to initialize several ScriptInterface
 * contexts which can then share data, but a single ScriptRuntime should
 * only be used on a single thread.
 *
 * (One means to share data between threads and runtimes is to create
 * a ScriptInterface::StructuredClone.)
 */

class ScriptRuntime
{
public:
	ScriptRuntime(shared_ptr<ScriptRuntime> parentRuntime, int runtimeSize, int heapGrowthBytesGCTrigger);
	~ScriptRuntime();

	/**
	 * MaybeIncrementalRuntimeGC tries to determine whether a runtime-wide garbage collection would free up enough memory to
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

	void RegisterContext(JSContext* cx);
	void UnRegisterContext(JSContext* cx);

	JSRuntime* m_rt;

private:

	void PrepareContextsForIncrementalGC();

	std::list<JSContext*> m_Contexts;

	int m_RuntimeSize;
	int m_HeapGrowthBytesGCTrigger;
	int m_LastGCBytes;
	double m_LastGCCheck;
};

#endif // INCLUDED_SCRIPTRUNTIME
