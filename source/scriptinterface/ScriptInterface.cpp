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

#include "ScriptInterface.h"
#include "ScriptStats.h"
#include "AutoRooters.h"

#include "lib/debug.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

#include <cassert>
#include <map>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/flyweight/no_locking.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include "valgrind.h"

#define STACK_CHUNK_SIZE 8192

#if ENABLE_SCRIPT_PROFILING
# define signbit std::signbit
# include "js/jsdbgapi.h"
# undef signbit
#endif

////////////////////////////////////////////////////////////////

class ScriptRuntime
{
public:
	ScriptRuntime(int runtimeSize) :
		m_rooter(NULL)
	{
		m_rt = JS_NewRuntime(runtimeSize);
		ENSURE(m_rt); // TODO: error handling

#if ENABLE_SCRIPT_PROFILING
		// Profiler isn't thread-safe, so only enable this on the main thread
		if (ThreadUtil::IsMainThread())
		{
			if (CProfileManager::IsInitialised())
			{
				JS_SetExecuteHook(m_rt, jshook_script, this);
				JS_SetCallHook(m_rt, jshook_function, this);
			}
		}
#endif

		JS_SetExtraGCRoots(m_rt, jshook_trace, this);
	}

	~ScriptRuntime()
	{
		JS_DestroyRuntime(m_rt);
	}

	JSRuntime* m_rt;
	AutoGCRooter* m_rooter;

private:

#if ENABLE_SCRIPT_PROFILING
	static void* jshook_script(JSContext* UNUSED(cx), JSStackFrame* UNUSED(fp), JSBool before, JSBool* UNUSED(ok), void* closure)
	{
		if (before)
			g_Profiler.StartScript("script invocation");
		else
			g_Profiler.Stop();

		return closure;
	}

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

			uintN line = JS_PCToLineNumber(cx, script, pc);

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

	static void* jshook_function(JSContext* cx, JSStackFrame* fp, JSBool before, JSBool* UNUSED(ok), void* closure)
	{
		if (!before)
		{
			g_Profiler.Stop();
			return closure;
		}

		JSFunction* fn = JS_GetFrameFunction(cx, fp);
		if (!fn)
		{
			g_Profiler.StartScript("(function)");
			return closure;
		}

		// Try to get the name of non-anonymous functions
		JSString* name = JS_GetFunctionId(fn);
		if (name)
		{
			char* chars = JS_EncodeString(cx, name);
			if (chars)
			{
				g_Profiler.StartScript(StringFlyweight(chars).get().c_str());
				JS_free(cx, chars);
				return closure;
			}
		}

		// No name - compute from the location instead
		ScriptLocation loc = { cx, JS_GetFrameScript(cx, fp), JS_GetFramePC(cx, fp) };
		g_Profiler.StartScript(LocFlyweight(loc).get().name.c_str());

		return closure;
	}
#endif

	static void jshook_trace(JSTracer* trc, void* data)
	{
		ScriptRuntime* m = static_cast<ScriptRuntime*>(data);

		if (m->m_rooter)
			m->m_rooter->Trace(trc);
	}
};

shared_ptr<ScriptRuntime> ScriptInterface::CreateRuntime(int runtimeSize)
{
	return shared_ptr<ScriptRuntime>(new ScriptRuntime(runtimeSize));
}

////////////////////////////////////////////////////////////////

struct ScriptInterface_impl
{
	ScriptInterface_impl(const char* nativeScopeName, const shared_ptr<ScriptRuntime>& runtime);
	~ScriptInterface_impl();
	void Register(const char* name, JSNative fptr, uintN nargs);

	shared_ptr<ScriptRuntime> m_runtime;
	JSContext* m_cx;
	JSObject* m_glob; // global scope object
	JSObject* m_nativeScope; // native function scope object
};

namespace
{

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

void ErrorReporter(JSContext* cx, const char* message, JSErrorReport* report)
{
	// XXX Ugly hack: we want to compile code with 'use strict' and with JSOPTION_STRICT,
	// but the latter causes the former to be reported as a useless expression, so
	// ignore that specific warning here
	if (report->flags == 5 && report->lineno == 0 && report->errorNumber == 163)
		return;

	std::stringstream msg;
	bool isWarning = JSREPORT_IS_WARNING(report->flags);
	msg << (isWarning ? "JavaScript warning: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}

	msg << message;

	// If there is an exception, then print its stack trace
	jsval excn;
	if (JS_GetPendingException(cx, &excn) && JSVAL_IS_OBJECT(excn))
	{
		// TODO: this violates the docs ("The error reporter callback must not reenter the JSAPI.")

		jsval rval;
		const char dumpStack[] = "this.stack.trimRight().replace(/^/mg, '  ')"; // indent each line
		if (JS_EvaluateScript(cx, JSVAL_TO_OBJECT(excn), dumpStack, ARRAY_SIZE(dumpStack)-1, "(eval)", 1, &rval))
		{
			std::string stackTrace;
			if (ScriptInterface::FromJSVal(cx, rval, stackTrace))
				msg << "\n" << stackTrace;
		}
	}

	if (isWarning)
		LOGWARNING(L"%hs", msg.str().c_str());
	else
		LOGERROR(L"%hs", msg.str().c_str());

	// When running under Valgrind, print more information in the error message
//	VALGRIND_PRINTF_BACKTRACE("->");
}

// Functions in the global namespace:

JSBool print(JSContext* cx, uintN argc, jsval* vp)
{
	for (uintN i = 0; i < argc; ++i)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[i], str))
			return JS_FALSE;
		printf("%s", str.c_str());
	}
	fflush(stdout);
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool logmsg(JSContext* cx, uintN argc, jsval* vp)
{
	if (argc < 1)
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
		return JS_FALSE;
	LOGMESSAGE(L"%ls", str.c_str());
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool warn(JSContext* cx, uintN argc, jsval* vp)
{
	if (argc < 1)
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
		return JS_FALSE;
	LOGWARNING(L"%ls", str.c_str());
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool error(JSContext* cx, uintN argc, jsval* vp)
{
	if (argc < 1)
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
		return JS_FALSE;
	LOGERROR(L"%ls", str.c_str());
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool ProfileStart(JSContext* cx, uintN argc, jsval* vp)
{
	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
	{
		const char* name = "(ProfileStart)";

		if (argc >= 1)
		{
			std::string str;
			if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
				return JS_FALSE;

			typedef boost::flyweight<
				std::string,
				boost::flyweights::no_tracking,
				boost::flyweights::no_locking
			> StringFlyweight;

			name = StringFlyweight(str).get().c_str();
		}

		g_Profiler.StartScript(name);
	}

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool ProfileStop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* vp)
{
	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
	{
		g_Profiler.Stop();
	}

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

// Math override functions:

JSBool Math_random(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
	// Grab the RNG that was hidden in our slot
	jsval rngp;
	if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), 0, &rngp))
		return JS_FALSE;
	boost::rand48* rng = static_cast<boost::rand48*>(JSVAL_TO_PRIVATE(rngp));

	// TODO: is the double generation sufficiently deterministic for us?
	boost::uniform_real<double> dist;
	double r = dist(*rng);

	jsval rv;
	if (!JS_NewNumberValue(cx, r, &rv))
		return JS_FALSE;
	JS_SET_RVAL(cx, vp, rv);
	return JS_TRUE;
}

} // anonymous namespace

ScriptInterface_impl::ScriptInterface_impl(const char* nativeScopeName, const shared_ptr<ScriptRuntime>& runtime) :
	m_runtime(runtime)
{
	JSBool ok;

	m_cx = JS_NewContext(m_runtime->m_rt, STACK_CHUNK_SIZE);
	ENSURE(m_cx);

	// For GC debugging:
	// JS_SetGCZeal(m_cx, 2);

	JS_SetContextPrivate(m_cx, NULL);

	JS_SetErrorReporter(m_cx, ErrorReporter);

	uint32 options = 0;
	options |= JSOPTION_STRICT; // "warn on dubious practice"
	options |= JSOPTION_XML; // "ECMAScript for XML support: parse <!-- --> as a token"
	options |= JSOPTION_VAROBJFIX; // "recommended" (fixes variable scoping)

	// Enable method JIT, unless script profiling is enabled (since profiling
	// hooks are incompatible with the JIT)
#if !ENABLE_SCRIPT_PROFILING
	options |= JSOPTION_METHODJIT;

	// Some other JIT flags to experiment with:
	options |= JSOPTION_JIT;
	options |= JSOPTION_PROFILING;
#endif

	JS_SetOptions(m_cx, options);

	JS_SetVersion(m_cx, JSVERSION_LATEST);

	// Threadsafe SpiderMonkey requires that we have a request before doing anything much
	JS_BeginRequest(m_cx);

	m_glob = JS_NewGlobalObject(m_cx, &global_class);
	ok = JS_InitStandardClasses(m_cx, m_glob);
	ENSURE(ok);

	JS_DefineProperty(m_cx, m_glob, "global", OBJECT_TO_JSVAL(m_glob), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, m_glob, "print", ::print,  0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "log",   ::logmsg, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "warn",  ::warn,   1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "error", ::error,  1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	Register("ProfileStart", ::ProfileStart, 1);
	Register("ProfileStop", ::ProfileStop, 0);
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	JS_EndRequest(m_cx);
	JS_DestroyContext(m_cx);
}

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uintN nargs)
{
	JS_DefineFunction(m_cx, m_nativeScope, name, fptr, nargs, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
}

ScriptInterface::ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptRuntime>& runtime) :
	m(new ScriptInterface_impl(nativeScopeName, runtime))
{
	// Profiler stats table isn't thread-safe, so only enable this on the main thread
	if (ThreadUtil::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Add(this, debugName);
	}
}

ScriptInterface::~ScriptInterface()
{
	if (ThreadUtil::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Remove(this);
	}
}

void ScriptInterface::ShutDown()
{
	JS_ShutDown();
}

void ScriptInterface::SetCallbackData(void* cbdata)
{
	JS_SetContextPrivate(m->m_cx, cbdata);
}

void* ScriptInterface::GetCallbackData(JSContext* cx)
{
	return JS_GetContextPrivate(cx);
}

void ScriptInterface::ReplaceNondeterministicFunctions(boost::rand48& rng)
{
	jsval math;
	if (!JS_GetProperty(m->m_cx, m->m_glob, "Math", &math) || !JSVAL_IS_OBJECT(math))
	{
		LOGERROR(L"ReplaceNondeterministicFunctions: failed to get Math");
		return;
	}

	JSFunction* random = JS_DefineFunction(m->m_cx, JSVAL_TO_OBJECT(math), "random", Math_random, 0,
			JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	if (!random)
	{
		LOGERROR(L"ReplaceNondeterministicFunctions: failed to replace Math.random");
		return;
	}
	// Store the RNG in a slot which is sort-of-guaranteed to be unused by the JS engine
	JS_SetReservedSlot(m->m_cx, JS_GetFunctionObject(random), 0, PRIVATE_TO_JSVAL(&rng));
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs)
{
	m->Register(name, fptr, (uintN)nargs);
}

JSContext* ScriptInterface::GetContext() const
{
	return m->m_cx;
}

JSRuntime* ScriptInterface::GetRuntime() const
{
	return m->m_runtime->m_rt;
}

AutoGCRooter* ScriptInterface::ReplaceAutoGCRooter(AutoGCRooter* rooter)
{
	AutoGCRooter* ret = m->m_runtime->m_rooter;
	m->m_runtime->m_rooter = rooter;
	return ret;
}


jsval ScriptInterface::CallConstructor(jsval ctor, jsval arg)
{
	if (!JSVAL_IS_OBJECT(ctor))
	{
		LOGERROR(L"CallConstructor: ctor is not an object");
		return JSVAL_VOID;
	}

	return OBJECT_TO_JSVAL(JS_New(m->m_cx, JSVAL_TO_OBJECT(ctor), 1, &arg));
}

jsval ScriptInterface::NewObjectFromConstructor(jsval ctor)
{
	// Get the constructor's prototype
	// (Can't use JS_GetPrototype, since we want .prototype not .__proto__)
	jsval protoVal;
	if (!JS_GetProperty(m->m_cx, JSVAL_TO_OBJECT(ctor), "prototype", &protoVal))
	{
		LOGERROR(L"NewObjectFromConstructor: can't get prototype");
		return JSVAL_VOID;
	}

	if (!JSVAL_IS_OBJECT(protoVal))
	{
		LOGERROR(L"NewObjectFromConstructor: prototype is not an object");
		return JSVAL_VOID;
	}

	JSObject* proto = JSVAL_TO_OBJECT(protoVal);
	JSObject* parent = JS_GetParent(m->m_cx, JSVAL_TO_OBJECT(ctor));
	// TODO: rooting?
	if (!proto || !parent)
	{
		LOGERROR(L"NewObjectFromConstructor: null proto/parent");
		return JSVAL_VOID;
	}

	JSObject* obj = JS_NewObject(m->m_cx, NULL, proto, parent);
	if (!obj)
	{
		LOGERROR(L"NewObjectFromConstructor: object creation failed");
		return JSVAL_VOID;
	}

	return OBJECT_TO_JSVAL(obj);
}

bool ScriptInterface::CallFunctionVoid(jsval val, const char* name)
{
	jsval jsRet;
	return CallFunction_(val, name, 0, NULL, jsRet);
}

bool ScriptInterface::CallFunction_(jsval val, const char* name, size_t argc, jsval* argv, jsval& ret)
{
	JSObject* obj;
	if (!JS_ValueToObject(m->m_cx, val, &obj) || obj == NULL)
		return false;

	// Check that the named function actually exists, to avoid ugly JS error reports
	// when calling an undefined value
	JSBool found;
	if (!JS_HasProperty(m->m_cx, obj, name, &found) || !found)
		return false;

	JSBool ok = JS_CallFunctionName(m->m_cx, obj, name, (uintN)argc, argv, &ret);

	return ok ? true : false;
}

jsval ScriptInterface::GetGlobalObject()
{
	return OBJECT_TO_JSVAL(JS_GetGlobalObject(m->m_cx));
}

JSClass* ScriptInterface::GetGlobalClass()
{
	return &global_class;
}

bool ScriptInterface::SetGlobal_(const char* name, jsval value, bool replace)
{
	if (!replace)
	{
		JSBool found;
		if (!JS_HasProperty(m->m_cx, m->m_glob, name, &found))
			return false;
		if (found)
		{
			JS_ReportError(m->m_cx, "SetGlobal \"%s\" called multiple times", name);
			return false;
		}
	}

	JSBool ok = JS_DefineProperty(m->m_cx, m->m_glob, name, value, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);
	return ok ? true : false;
}

bool ScriptInterface::SetProperty_(jsval obj, const char* name, jsval value, bool constant, bool enumerate)
{
	uintN attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	if (! JS_DefineProperty(m->m_cx, object, name, value, NULL, NULL, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetPropertyInt_(jsval obj, int name, jsval value, bool constant, bool enumerate)
{
	uintN attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	if (! JS_DefinePropertyById(m->m_cx, object, INT_TO_JSID(name), value, NULL, NULL, attrs))
		return false;
	return true;
}

bool ScriptInterface::GetProperty_(jsval obj, const char* name, jsval& out)
{
	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	if (!JS_GetProperty(m->m_cx, object, name, &out))
		return false;
	return true;
}

bool ScriptInterface::HasProperty(jsval obj, const char* name)
{
	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	JSBool found;
	if (!JS_HasProperty(m->m_cx, object, name, &found))
		return false;
	return (found != JS_FALSE);
}

bool ScriptInterface::EnumeratePropertyNamesWithPrefix(jsval obj, const char* prefix, std::vector<std::string>& out)
{
	utf16string prefix16 (prefix, prefix+strlen(prefix));

	if (! JSVAL_IS_OBJECT(obj))
		return false; // TODO: log error messages

	JSObject* it = JS_NewPropertyIterator(m->m_cx, JSVAL_TO_OBJECT(obj));
	if (!it)
		return false;

	while (true)
	{
		jsid idp;
		jsval val;
		if (! JS_NextProperty(m->m_cx, it, &idp) || ! JS_IdToValue(m->m_cx, idp, &val))
			return false;
		if (val == JSVAL_VOID)
			break; // end of iteration
		if (! JSVAL_IS_STRING(val))
			continue; // ignore integer properties

		JSString* name = JSVAL_TO_STRING(val);
		size_t len = JS_GetStringLength(name);
		jschar* chars = JS_GetStringChars(name);
		if (len >= prefix16.size() && memcmp(chars, prefix16.c_str(), prefix16.size()*2) == 0)
			out.push_back(std::string(chars, chars+len)); // handles Unicode poorly
	}

	// Recurse up the prototype chain
	JSObject* prototype = JS_GetPrototype(m->m_cx, JSVAL_TO_OBJECT(obj));
	if (prototype)
	{
		if (! EnumeratePropertyNamesWithPrefix(OBJECT_TO_JSVAL(prototype), prefix, out))
			return false;
	}

	return true;
}

bool ScriptInterface::SetPrototype(jsval obj, jsval proto)
{
	if (!JSVAL_IS_OBJECT(obj) || !JSVAL_IS_OBJECT(proto))
		return false;
	return JS_SetPrototype(m->m_cx, JSVAL_TO_OBJECT(obj), JSVAL_TO_OBJECT(proto)) ? true : false;
}

bool ScriptInterface::FreezeObject(jsval obj, bool deep)
{
	if (!JSVAL_IS_OBJECT(obj))
		return false;

	if (deep)
		return JS_DeepFreezeObject(m->m_cx, JSVAL_TO_OBJECT(obj)) ? true : false;
	else
		return JS_FreezeObject(m->m_cx, JSVAL_TO_OBJECT(obj)) ? true : false;
}

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::wstring& code)
{
	// Compile the code in strict mode, to encourage better coding practices and
	// to possibly help SpiderMonkey with optimisations
	std::wstring codeStrict = L"\"use strict\";\n" + code;
	utf16string codeUtf16(codeStrict.begin(), codeStrict.end());
	uintN lineNo = 0; // put the automatic 'use strict' on line 0, so the real code starts at line 1

	JSFunction* func = JS_CompileUCFunction(m->m_cx, NULL, NULL, 0, NULL,
			reinterpret_cast<const jschar*> (codeUtf16.c_str()), (uintN)(codeUtf16.length()),
			utf8_from_wstring(filename.string()).c_str(), lineNo);

	if (!func)
		return false;

	jsval scriptRval;
	JSBool ok = JS_CallFunction(m->m_cx, NULL, func, 0, NULL, &scriptRval);

	return ok ? true : false;
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::wstring& code)
{
	// Compile the code in strict mode, to encourage better coding practices and
	// to possibly help SpiderMonkey with optimisations
	std::wstring codeStrict = L"\"use strict\";\n" + code;
	utf16string codeUtf16(codeStrict.begin(), codeStrict.end());
	uintN lineNo = 0; // put the automatic 'use strict' on line 0, so the real code starts at line 1

	jsval rval;
	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob,
			reinterpret_cast<const jschar*> (codeUtf16.c_str()), (uintN)(codeUtf16.length()),
			utf8_from_wstring(filename.string()).c_str(), lineNo, &rval);

	return ok ? true : false;
}

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path)
{
	if (!VfsFileExists(g_VFS, path))
	{
		LOGERROR(L"File '%ls' does not exist", path.string().c_str());
		return false;
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, path);

	if (ret != PSRETURN_OK)
	{
		LOGERROR(L"Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
		return false;
	}

	std::string content(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize());
	std::wstring code = wstring_from_utf8(content);

	// Compile the code in strict mode, to encourage better coding practices and
	// to possibly help SpiderMonkey with optimisations
	std::wstring codeStrict = L"\"use strict\";\n" + code;
	utf16string codeUtf16(codeStrict.begin(), codeStrict.end());
	uintN lineNo = 0; // put the automatic 'use strict' on line 0, so the real code starts at line 1

	jsval rval;
	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob,
			reinterpret_cast<const jschar*> (codeUtf16.c_str()), (uintN)(codeUtf16.length()),
			utf8_from_wstring(path.string()).c_str(), lineNo, &rval);

	return ok ? true : false;
}


bool ScriptInterface::Eval(const char* code)
{
	jsval rval;
	return Eval_(code, rval);
}

bool ScriptInterface::Eval_(const char* code, jsval& rval)
{
	utf16string codeUtf16(code, code+strlen(code));

	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob, (const jschar*)codeUtf16.c_str(), (uintN)codeUtf16.length(), "(eval)", 1, &rval);
	return ok ? true : false;
}

bool ScriptInterface::Eval_(const wchar_t* code, jsval& rval)
{
	utf16string codeUtf16(code, code+wcslen(code));

	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob, (const jschar*)codeUtf16.c_str(), (uintN)codeUtf16.length(), "(eval)", 1, &rval);
	return ok ? true : false;
}

CScriptValRooted ScriptInterface::ParseJSON(const std::string& string_utf8)
{
	std::wstring attrsW = wstring_from_utf8(string_utf8);
	utf16string string(attrsW.begin(), attrsW.end());

	jsval vp;
	JSONParser* parser = JS_BeginJSONParse(m->m_cx, &vp);
	if (!parser)
	{
		LOGERROR(L"ParseJSON failed to begin");
		return CScriptValRooted();
	}

	if (!JS_ConsumeJSONText(m->m_cx, parser, reinterpret_cast<const jschar*>(string.c_str()), (uint32)string.size()))
	{
		LOGERROR(L"ParseJSON failed to consume");
		return CScriptValRooted();
	}

	if (!JS_FinishJSONParse(m->m_cx, parser, JSVAL_NULL))
	{
		LOGERROR(L"ParseJSON failed to finish");
		return CScriptValRooted();
	}

	return CScriptValRooted(m->m_cx, vp);
}

CScriptValRooted ScriptInterface::ReadJSONFile(const VfsPath& path)
{
	if (!VfsFileExists(g_VFS, path))
	{
		LOGERROR(L"File '%ls' does not exist", path.string().c_str());
		return CScriptValRooted();
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, path);

	if (ret != PSRETURN_OK)
	{
		LOGERROR(L"Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
		return CScriptValRooted();
	}

	std::string content(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize()); // assume it's UTF-8

	return ParseJSON(content);
}

struct Stringifier
{
	static JSBool callback(const jschar* buf, uint32 len, void* data)
	{
		utf16string str(buf, buf+len);
		std::wstring strw(str.begin(), str.end());

		Status err; // ignore Unicode errors
		static_cast<Stringifier*>(data)->stream << utf8_from_wstring(strw, &err);
		return JS_TRUE;
	}

	std::stringstream stream;
};

struct StringifierW
{
	static JSBool callback(const jschar* buf, uint32 len, void* data)
	{
		utf16string str(buf, buf+len);
		static_cast<StringifierW*>(data)->stream << std::wstring(str.begin(), str.end());
		return JS_TRUE;
	}

	std::wstringstream stream;
};

std::string ScriptInterface::StringifyJSON(jsval obj, bool indent)
{
	Stringifier str;
	if (!JS_Stringify(m->m_cx, &obj, NULL, indent ? INT_TO_JSVAL(2) : JSVAL_VOID, &Stringifier::callback, &str))
	{
		LOGERROR(L"StringifyJSON failed");
		return "";
	}

	return str.stream.str();
}

std::wstring ScriptInterface::ToString(jsval obj, bool pretty)
{
	if (JSVAL_IS_VOID(obj))
		return L"(void 0)";

	// Try to stringify as JSON if possible
	// (TODO: this is maybe a bad idea since it'll drop 'undefined' values silently)
	if (pretty)
	{
		StringifierW str;

		// Temporary disable the error reporter, so we don't print complaints about cyclic values
		JSErrorReporter er = JS_SetErrorReporter(m->m_cx, NULL);

		bool ok = JS_Stringify(m->m_cx, &obj, NULL, INT_TO_JSVAL(2), &StringifierW::callback, &str) == JS_TRUE;

		// Restore error reporter
		JS_SetErrorReporter(m->m_cx, er);

		if (ok)
			return str.stream.str();

		// Clear the exception set when Stringify failed
		JS_ClearPendingException(m->m_cx);
	}

	// Caller didn't want pretty output, or JSON conversion failed (e.g. due to cycles),
	// so fall back to obj.toSource()

	std::wstring source = L"(error)";
	CallFunction(obj, "toSource", source);
	return source;
}

void ScriptInterface::ReportError(const char* msg)
{
	// JS_ReportError by itself doesn't seem to set a JS-style exception, and so
	// script callers will be unable to catch anything. So use JS_SetPendingException
	// to make sure there really is a script-level exception. But just set it to undefined
	// because there's not much value yet in throwing a real exception object.
	JS_SetPendingException(m->m_cx, JSVAL_VOID);
	// And report the actual error
	JS_ReportError(m->m_cx, "%s", msg);

	// TODO: Why doesn't JS_ReportPendingException(m->m_cx); work?
}

bool ScriptInterface::IsExceptionPending(JSContext* cx)
{
	return JS_IsExceptionPending(cx) ? true : false;
}

JSClass* ScriptInterface::GetClass(JSContext* cx, JSObject* obj)
{
	UNUSED2(cx); // unused if not JS_THREADSAFE

	return JS_GET_CLASS(cx, obj);
}

void* ScriptInterface::GetPrivate(JSContext* cx, JSObject* obj)
{
	// TODO: use JS_GetInstancePrivate
	return JS_GetPrivate(cx, obj);
}

void ScriptInterface::DumpHeap()
{
#ifdef DEBUG
	JS_DumpHeap(m->m_cx, stderr, NULL, 0, NULL, (size_t)-1, NULL);
#endif
	fprintf(stderr, "# Bytes allocated: %d\n", JS_GetGCParameter(GetRuntime(), JSGC_BYTES));
	JS_GC(m->m_cx);
	fprintf(stderr, "# Bytes allocated after GC: %d\n", JS_GetGCParameter(GetRuntime(), JSGC_BYTES));
}

void ScriptInterface::MaybeGC()
{
	JS_MaybeGC(m->m_cx);
}

class ValueCloner
{
public:
	ValueCloner(ScriptInterface& from, ScriptInterface& to) :
		scriptInterfaceFrom(from), cxFrom(from.GetContext()), cxTo(to.GetContext()), m_RooterFrom(from), m_RooterTo(to)
	{
	}

	// Return the cloned object (or an already-computed object if we've cloned val before)
	jsval GetOrClone(jsval val)
	{
		if (!JSVAL_IS_GCTHING(val) || JSVAL_IS_NULL(val))
			return val;

		std::map<void*, jsval>::iterator it = m_Mapping.find(JSVAL_TO_GCTHING(val));
		if (it != m_Mapping.end())
			return it->second;

		m_RooterFrom.Push(val); // root it so our mapping doesn't get invalidated

		return Clone(val);
	}

private:

#define CLONE_REQUIRE(expr, msg) if (!(expr)) { debug_warn(L"Internal error in CloneValueFromOtherContext: " msg); return JSVAL_VOID; }

	// Clone a new value (and root it and add it to the mapping)
	jsval Clone(jsval val)
	{
		if (JSVAL_IS_DOUBLE(val))
		{
			jsval rval;
			CLONE_REQUIRE(JS_NewNumberValue(cxTo, JSVAL_TO_DOUBLE(val), &rval), L"JS_NewNumberValue");
			m_RooterTo.Push(rval);
			return rval;
		}

		if (JSVAL_IS_STRING(val))
		{
			JSString* str = JS_NewUCStringCopyN(cxTo, JS_GetStringChars(JSVAL_TO_STRING(val)), JS_GetStringLength(JSVAL_TO_STRING(val)));
			CLONE_REQUIRE(str, L"JS_NewUCStringCopyN");
			jsval rval = STRING_TO_JSVAL(str);
			m_Mapping[JSVAL_TO_GCTHING(val)] = rval;
			m_RooterTo.Push(rval);
			return rval;
		}

		ENSURE(JSVAL_IS_OBJECT(val));

		JSObject* newObj;
		if (JS_IsArrayObject(cxFrom, JSVAL_TO_OBJECT(val)))
		{
			jsuint length;
			CLONE_REQUIRE(JS_GetArrayLength(cxFrom, JSVAL_TO_OBJECT(val), &length), L"JS_GetArrayLength");
			newObj = JS_NewArrayObject(cxTo, length, NULL);
			CLONE_REQUIRE(newObj, L"JS_NewArrayObject");
		}
		else
		{
			newObj = JS_NewObject(cxTo, NULL, NULL, NULL);
			CLONE_REQUIRE(newObj, L"JS_NewObject");
		}

		m_Mapping[JSVAL_TO_GCTHING(val)] = OBJECT_TO_JSVAL(newObj);
		m_RooterTo.Push(newObj);

		AutoJSIdArray ida (cxFrom, JS_Enumerate(cxFrom, JSVAL_TO_OBJECT(val)));
		CLONE_REQUIRE(ida.get(), L"JS_Enumerate");

		AutoGCRooter idaRooter(scriptInterfaceFrom);
		idaRooter.Push(ida.get());

		for (size_t i = 0; i < ida.length(); ++i)
		{
			jsid id = ida[i];
			jsval idval, propval;
			CLONE_REQUIRE(JS_IdToValue(cxFrom, id, &idval), L"JS_IdToValue");
			CLONE_REQUIRE(JS_GetPropertyById(cxFrom, JSVAL_TO_OBJECT(val), id, &propval), L"JS_GetPropertyById");
			jsval newPropval = GetOrClone(propval);

			if (JSVAL_IS_INT(idval))
			{
				// int jsids are portable across runtimes
				CLONE_REQUIRE(JS_SetPropertyById(cxTo, newObj, id, &newPropval), L"JS_SetPropertyById");
			}
			else if (JSVAL_IS_STRING(idval))
			{
				// string jsids are runtime-specific, so we need to copy the string content
				JSString* idstr = JS_ValueToString(cxFrom, idval);
				CLONE_REQUIRE(idstr, L"JS_ValueToString (id)");
				CLONE_REQUIRE(JS_SetUCProperty(cxTo, newObj, JS_GetStringChars(idstr), JS_GetStringLength(idstr), &newPropval), L"JS_SetUCProperty");
			}
			else
			{
				// this apparently could be an XML object; ignore it
			}
		}

		return OBJECT_TO_JSVAL(newObj);
	}

	ScriptInterface& scriptInterfaceFrom;
	JSContext* cxFrom;
	JSContext* cxTo;
	std::map<void*, jsval> m_Mapping;
	AutoGCRooter m_RooterFrom;
	AutoGCRooter m_RooterTo;
};

jsval ScriptInterface::CloneValueFromOtherContext(ScriptInterface& otherContext, jsval val)
{
	PROFILE("CloneValueFromOtherContext");

	ValueCloner cloner(otherContext, *this);
	return cloner.GetOrClone(val);
}

ScriptInterface::StructuredClone::StructuredClone() :
	m_Context(NULL), m_Data(NULL), m_Size(0)
{
}

ScriptInterface::StructuredClone::~StructuredClone()
{
	if (m_Data)
		JS_free(m_Context, m_Data);
}

shared_ptr<ScriptInterface::StructuredClone> ScriptInterface::WriteStructuredClone(jsval v)
{
	uint64* data = NULL;
	size_t nbytes = 0;
	if (!JS_WriteStructuredClone(m->m_cx, v, &data, &nbytes))
		return shared_ptr<StructuredClone>();
	// TODO: should we have better error handling?
	// Currently we'll probably continue and then crash in ReadStructuredClone

	shared_ptr<StructuredClone> ret (new StructuredClone);
	ret->m_Context = m->m_cx;
	ret->m_Data = data;
	ret->m_Size = nbytes;
	return ret;
}

jsval ScriptInterface::ReadStructuredClone(const shared_ptr<ScriptInterface::StructuredClone>& ptr)
{
	jsval ret = JSVAL_VOID;
	JS_ReadStructuredClone(m->m_cx, ptr->m_Data, ptr->m_Size, &ret);
	return ret;
}
