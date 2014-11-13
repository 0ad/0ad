/* Copyright (C) 2014 Wildfire Games.
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

#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/flyweight/no_locking.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include "ScriptTypes.h"
#include "ScriptExtraHeaders.h"

#define STACK_CHUNK_SIZE 8192

class AutoGCRooter;

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
	ScriptRuntime(int runtimeSize, int heapGrowthBytesGCTrigger);
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
	
	void RegisterContext(JSContext* cx);
	void UnRegisterContext(JSContext* cx);
	
	JSRuntime* m_rt;
	AutoGCRooter* m_rooter;

private:
	
	void PrepareContextsForIncrementalGC();
	
	// Workaround for: https://bugzilla.mozilla.org/show_bug.cgi?id=890243
	JSContext* m_dummyContext;
	
	std::list<JSContext*> m_Contexts;
	
	int m_RuntimeSize;
	int m_HeapGrowthBytesGCTrigger;
	int m_LastGCBytes;
	double m_LastGCCheck;

	static void* jshook_script(JSContext* UNUSED(cx), JSAbstractFramePtr UNUSED(fp), 
		bool UNUSED(isConstructing), JSBool before, 
		JSBool* UNUSED(ok), void* closure);

	static void* jshook_function(JSContext* cx, JSAbstractFramePtr fp, 
		bool UNUSED(isConstructing), JSBool before, 
		JSBool* UNUSED(ok), void* closure);

	static void jshook_trace(JSTracer* trc, void* data);

	// To profile scripts usefully, we use a call hook that's called on every enter/exit,
	// and need to find the function name. But most functions are anonymous so we make do
	// with filename plus line number instead.
	// Computing the names is fairly expensive, and we need to return an interned char*
	// for the profiler to hold a copy of, so we use boost::flyweight to construct interned
	// strings per call location.

	// Identifies a location in a script
	struct ScriptLocation
	{
		JSContext* cx;
		JSScript* script;
		jsbytecode* pc;

		bool operator==(const ScriptLocation& b) const
		{
			return cx == b.cx && script == b.script && pc == b.pc;
		}

		friend std::size_t hash_value(const ScriptLocation& loc)
		{
			std::size_t seed = 0;
			boost::hash_combine(seed, loc.cx);
			boost::hash_combine(seed, loc.script);
			boost::hash_combine(seed, loc.pc);
			return seed;
		}
	};

	// Computes and stores the name of a location in a script
	struct ScriptLocationName
	{
		ScriptLocationName(const ScriptLocation& loc)
		{
			JSContext* cx = loc.cx;
			JSScript* script = loc.script;
			jsbytecode* pc = loc.pc;

			std::string filename = JS_GetScriptFilename(cx, script);
			size_t slash = filename.rfind('/');
			if (slash != filename.npos)
				filename = filename.substr(slash+1);

			uint line = JS_PCToLineNumber(cx, script, pc);

			std::stringstream ss;
			ss << "(" << filename << ":" << line << ")";
			name = ss.str();
		}

		std::string name;
	};

	// Flyweight types (with no_locking because the call hooks are only used in the
	// main thread, and no_tracking because we mustn't delete values the profiler is
	// using and it's not going to waste much memory)
	typedef boost::flyweight<
		std::string,
		boost::flyweights::no_tracking,
		boost::flyweights::no_locking
	> StringFlyweight;
	typedef boost::flyweight<
		boost::flyweights::key_value<ScriptLocation, ScriptLocationName>,
		boost::flyweights::no_tracking,
		boost::flyweights::no_locking
	> LocFlyweight;

};

#endif // INCLUDED_SCRIPTRUNTIME