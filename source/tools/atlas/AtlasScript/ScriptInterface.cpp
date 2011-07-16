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

#include "ScriptInterface.h"

#include <cassert>
#ifndef _WIN32
# include <typeinfo>
# include <cxxabi.h>
#endif

#include "wx/wx.h"

#include "GameInterface/Shareable.h"
#include "GameInterface/Messages.h"


#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#include "valgrind.h"

#define FAIL(msg) do { JS_ReportError(cx, msg); return false; } while (false)

const int RUNTIME_SIZE = 4*1024*1024; // TODO: how much memory is needed?
const int STACK_CHUNK_SIZE = 8192;

SubmitCommand g_SubmitCommand; // TODO: globals are ugly

////////////////////////////////////////////////////////////////

namespace
{
	template<typename T>
	void ReportError(JSContext* cx, const char* title)
	{
		// TODO: SetPendingException turns the error into a JS-catchable exception,
		// but the error report doesn't say anything useful like the line number,
		// so I'm just using ReportError instead for now (and failures are uncatchable
		// and will terminate the whole script)
		//JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "%s: Unhandled type", title)));
#ifdef _WIN32
		JS_ReportError(cx, "%s: Unhandled type", title);
#else
		// Give a more informative message on GCC
		int status;
		char* name = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
		JS_ReportError(cx, "%s: Unhandled type '%s'", title, name);
		free(name);
#endif
	}

	// Report runtime errors for unhandled types, so we don't have to bother
	// defining all the types in advance. (TODO: at some point we should
	// define all the types and then remove this bit so the errors are found
	// at link-time.)
	template<typename T> struct FromJSVal
	{
		static bool Convert(JSContext* cx, jsval WXUNUSED(v), T& WXUNUSED(out))
		{
			ReportError<T>(cx, "FromJSVal");
			return false;
		}
	};
	
	template<> struct FromJSVal<bool>
	{
		static bool Convert(JSContext* cx, jsval v, bool& out)
		{
			JSBool ret;
			if (! JS_ValueToBoolean(cx, v, &ret)) return false;
			out = (ret ? true : false);
			return true;
		}
	};

	template<> struct FromJSVal<float>
	{
		static bool Convert(JSContext* cx, jsval v, float& out)
		{
			jsdouble ret;
			if (! JS_ValueToNumber(cx, v, &ret)) return false;
			out = ret;
			return true;
		}
	};

	template<> struct FromJSVal<int>
	{
		static bool Convert(JSContext* cx, jsval v, int& out)
		{
			int32 ret;
			if (! JS_ValueToECMAInt32(cx, v, &ret)) return false;
			out = ret;
			return true;
		}
	};

	template<> struct FromJSVal<size_t>
	{
		static bool Convert(JSContext* cx, jsval v, size_t& out)
		{
			uint32 ret;
			if (! JS_ValueToECMAUint32(cx, v, &ret)) return false;
			out = ret;
			return true;
		}
	};

	template<> struct FromJSVal<CScriptVal>
	{
		static bool Convert(JSContext* WXUNUSED(cx), jsval v, CScriptVal& out)
		{
			out = v;
			return true;
		}
	};

	template<> struct FromJSVal<std::wstring>
	{
		static bool Convert(JSContext* cx, jsval v, std::wstring& out)
		{
			JSString* ret = JS_ValueToString(cx, v);
			if (! ret)
				FAIL("Argument must be convertible to a string");
			size_t len;
			const jschar* ch = JS_GetStringCharsAndLength(cx, ret, &len);
			if (! ch)
				FAIL("JS_GetStringsCharsAndLength failed"); // probably out of memory
			out = std::wstring(ch, ch + len);
			return true;
		}
	};

	template<> struct FromJSVal<std::string>
	{
		static bool Convert(JSContext* cx, jsval v, std::string& out)
		{
			JSString* ret = JS_ValueToString(cx, v);
			if (! ret)
				FAIL("Argument must be convertible to a string");
			size_t len = JS_GetStringEncodingLength(cx, ret);
			if (len == (size_t)-1)
				FAIL("JS_GetStringEncodingLength failed");
			char* ch = JS_EncodeString(cx, ret); // chops off high byte of each jschar
			if (! ch)
				FAIL("JS_EncodeString failed"); // probably out of memory
			out = std::string(ch, ch + len);
			JS_free(cx, ch);
			return true;
		}
	};

	template<> struct FromJSVal<wxString>
	{
		static bool Convert(JSContext* cx, jsval v, wxString& out)
		{
			JSString* ret = JS_ValueToString(cx, v);
			size_t len;
			if (! ret)
				FAIL("Argument must be convertible to a string");
			const jschar* ch = JS_GetStringCharsAndLength(cx, ret, &len);
			if (! ch)
				FAIL("JS_GetStringsCharsAndLength failed"); // probably out of memory
			out = wxString((const char*)ch, wxMBConvUTF16(), len*2);
			return true;
		}
	};

	template<typename T> struct FromJSVal<std::vector<T> >
	{
		static bool Convert(JSContext* cx, jsval v, std::vector<T>& out)
		{
			JSObject* obj;
			if (! JS_ValueToObject(cx, v, &obj) || obj == NULL || !JS_IsArrayObject(cx, obj))
				FAIL("Argument must be an array");
			jsuint length;
			if (! JS_GetArrayLength(cx, obj, &length))
				FAIL("Failed to get array length");
			out.reserve(length);
			for (jsuint i = 0; i < length; ++i)
			{
				jsval el;
				if (! JS_GetElement(cx, obj, i, &el))
					FAIL("Failed to read array element");
				T el2;
				if (! FromJSVal<T>::Convert(cx, el, el2))
					return false;
				out.push_back(el2);
			}
			return true;
		}
	};

	////////////////////////////////////////////////////////////////

	// Report runtime errors for unhandled types, so we don't have to bother
	// defining all the types in advance. (TODO: at some point we should
	// define all the types and then remove this bit so the errors are found
	// at link-time.)
	template<typename T> struct ToJSVal
	{
		static jsval Convert(JSContext* cx, const T& WXUNUSED(val))
		{
			ReportError<T>(cx, "ToJSVal");
			return JSVAL_VOID;
		}
	};

	////////////////////////////////////////////////////////////////
	// Primitive types:

	template<> struct ToJSVal<bool>
	{
		static jsval Convert(JSContext* WXUNUSED(cx), const bool& val)
		{
			return val ? JSVAL_TRUE : JSVAL_FALSE;
		}
	};

	template<> struct ToJSVal<float>
	{
		static jsval Convert(JSContext* cx, const float& val)
		{
			jsval rval = JSVAL_VOID;
			JS_NewNumberValue(cx, val, &rval); // ignore return value
			return rval;
		}
	};

	template<> struct ToJSVal<int>
	{
		static jsval Convert(JSContext* WXUNUSED(cx), const int& val)
		{
			return INT_TO_JSVAL(val);
		}
	};

	template<> struct ToJSVal<size_t>
	{
		static jsval Convert(JSContext* cx, const size_t& val)
		{
			if (val <= JSVAL_INT_MAX)
				return INT_TO_JSVAL(val);
			jsval rval = JSVAL_VOID;
			JS_NewNumberValue(cx, val, &rval); // ignore return value
			return rval;
		}
	};

	template<> struct ToJSVal<wxString>
	{
		static jsval Convert(JSContext* cx, const wxString& val)
		{
			wxMBConvUTF16 conv;
			size_t length;
			wxCharBuffer utf16 = conv.cWC2MB(val.c_str(), val.length()+1, &length);
			JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<jschar*>(utf16.data()), length/2);
			if (str)
				return STRING_TO_JSVAL(str);
			else
				return JSVAL_VOID;
		}
	};

	template<> struct ToJSVal<std::wstring>
	{
		static jsval Convert(JSContext* cx, const std::wstring& val)
		{
			wxMBConvUTF16 conv;
			size_t length;
			wxCharBuffer utf16 = conv.cWC2MB(val.c_str(), val.length()+1, &length);
			JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<jschar*>(utf16.data()), length/2);
			if (str)
				return STRING_TO_JSVAL(str);
			else
				return JSVAL_VOID;
		}
	};

	template<> struct ToJSVal<std::string>
	{
		static jsval Convert(JSContext* cx, const std::string& val)
		{
			JSString* str = JS_NewStringCopyN(cx, val.c_str(), val.length());
			if (str)
				return STRING_TO_JSVAL(str);
			return JSVAL_VOID;
		}
	};

	////////////////////////////////////////////////////////////////
	// Compound types:

	template<typename T> struct ToJSVal<std::vector<T> >
	{
		static jsval Convert(JSContext* cx, const std::vector<T>& val)
		{
			JSObject* obj = JS_NewArrayObject(cx, 0, NULL);
			if (! obj) return JSVAL_VOID;
			for (size_t i = 0; i < val.size(); ++i)
			{
				jsval el = ToJSVal<T>::Convert(cx, val[i]);
				JS_SetElement(cx, obj, (jsint)i, &el);
			}
			return OBJECT_TO_JSVAL(obj);
		}
	};
	
	template<typename T> struct ToJSVal<AtlasMessage::Shareable<T> >
	{
		static jsval Convert(JSContext* cx, const AtlasMessage::Shareable<T>& val)
		{
			return ToJSVal<T>::Convert(cx, val._Unwrap());
		}
	};
}

template<typename T> bool ScriptInterface::FromJSVal(JSContext* cx, jsval v, T& out)
{
	return ::FromJSVal<T>::Convert(cx, v, out);
}

template<typename T> jsval ScriptInterface::ToJSVal(JSContext* cx, const T& v)
{
	return ::ToJSVal<T>::Convert(cx, v);
}

// Explicit instantiation of functions that would otherwise be unused in this file
// but are required for linking with other files
template bool ScriptInterface::FromJSVal<std::string>(JSContext*, jsval, std::string&);
template bool ScriptInterface::FromJSVal<wxString>(JSContext*, jsval, wxString&);
template bool ScriptInterface::FromJSVal<bool>(JSContext*, jsval, bool&);
template bool ScriptInterface::FromJSVal<float>(JSContext*, jsval, float&);
template bool ScriptInterface::FromJSVal<CScriptVal>(JSContext*, jsval, CScriptVal&);
template jsval ScriptInterface::ToJSVal<wxString>(JSContext*, wxString const&);
template jsval ScriptInterface::ToJSVal<int>(JSContext*, int const&);
template jsval ScriptInterface::ToJSVal<float>(JSContext*, float const&);
template jsval ScriptInterface::ToJSVal<std::vector<int> >(JSContext*, std::vector<int> const&);
template jsval ScriptInterface::ToJSVal<size_t>(JSContext*, size_t const&);
template jsval ScriptInterface::ToJSVal<std::vector<wxString> >(JSContext*, std::vector<wxString> const&);

////////////////////////////////////////////////////////////////

struct AtlasScriptInterface_impl
{
	AtlasScriptInterface_impl();
	~AtlasScriptInterface_impl();
	static JSBool LoadScript(JSContext* cx, const jschar* chars, uintN length, const char* filename, jsval* rval);
	void RegisterMessages(JSObject* parent);
	void Register(const char* name, JSNative fptr, uintN nargs);

	JSRuntime* m_rt;
	JSContext* m_cx;
	JSObject* m_glob; // global scope object
	JSObject* m_atlas; // Atlas scope object
};

namespace
{
	JSClass global_class = {
		"global", JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL
	};
	
	void ErrorReporter(JSContext* WXUNUSED(cx), const char* message, JSErrorReport* report)
	{
		bool isWarning = JSREPORT_IS_WARNING(report->flags);
		wxString logMessage(isWarning ? _T("JavaScript warning: ") : _T("JavaScript error: "));
		if (report->filename)
		{
			logMessage << wxString::FromAscii(report->filename);
			logMessage << _T(" line ") << report->lineno << _T("\n");
		}
		logMessage << wxString::FromAscii(message);
		if (isWarning)
			wxLogWarning(_T("%s"), logMessage.c_str());
		else
			wxLogError(_T("%s"), logMessage.c_str());
		// When running under Valgrind, print more information in the error message
		VALGRIND_PRINTF_BACKTRACE("->");
		wxPrintf(_T("wxJS %s: %s\n--------\n"), isWarning ? _T("warning") : _T("error"), logMessage.c_str());
	}

	// Functions in the Atlas.* namespace:
	
	JSBool ForceGC(JSContext* cx, uintN WXUNUSED(argc), jsval* vp)
	{
		JS_GC(cx);
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}

	JSBool LoadScript(JSContext* cx, uintN argc, jsval* vp)
	{
		if (argc < 2 || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[1]))
			return JS_FALSE;
		
		std::string name;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], name))
			return JS_FALSE;

		JSString* code = JSVAL_TO_STRING(JS_ARGV(cx, vp)[1]);
		
		size_t len;
		const jschar* ch = JS_GetStringCharsAndLength(cx, code, &len);
		if (!ch)
			return JS_FALSE;

		jsval rval = JSVAL_VOID;
		if (!AtlasScriptInterface_impl::LoadScript(cx,
				ch, (uintN)len,
				name.c_str(), &rval))
			return JS_FALSE;

		JS_SET_RVAL(cx, vp, rval);
		return JS_TRUE;
	}
	
	// Functions in the global namespace:

	JSBool print(JSContext* cx, uintN argc, jsval* vp)
	{
		for (uintN i = 0; i < argc; ++i)
		{
			std::string str;
			if (! ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[i], str))
				return JS_FALSE;
			printf("%s", str.c_str());
		}
		fflush(stdout);
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}
}

AtlasScriptInterface_impl::AtlasScriptInterface_impl()
{
	m_rt = JS_NewRuntime(RUNTIME_SIZE);
	assert(m_rt); // TODO: error handling

	m_cx = JS_NewContext(m_rt, STACK_CHUNK_SIZE);
	assert(m_cx);

	JS_BeginRequest(m_cx); // if you get linker errors, see the comment in ScriptInterface.h about JS_THREADSAFE
	// (TODO: are we using requests correctly? (Probably not; how much does it matter?))

	JS_SetContextPrivate(m_cx, NULL);

	JS_SetErrorReporter(m_cx, ErrorReporter);

	JS_SetOptions(m_cx,
		  JSOPTION_STRICT // "warn on dubious practice"
		| JSOPTION_XML // "ECMAScript for XML support: parse <!-- --> as a token"
		);

	JS_SetVersion(m_cx, JSVERSION_LATEST);

	m_glob = JS_NewCompartmentAndGlobalObject(m_cx, &global_class, NULL);
	JS_InitStandardClasses(m_cx, m_glob);
	
	JS_DefineProperty(m_cx, m_glob, "global", OBJECT_TO_JSVAL(m_glob), NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, m_glob, "print", ::print, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
	m_atlas = JS_DefineObject(m_cx, m_glob, "Atlas", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "ForceGC", ::ForceGC, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "LoadScript", ::LoadScript, 2, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineObject(m_cx, m_atlas, "State", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
	RegisterMessages(m_atlas);
}

AtlasScriptInterface_impl::~AtlasScriptInterface_impl()
{
	JS_EndRequest(m_cx);
	JS_DestroyContext(m_cx);
	JS_DestroyRuntime(m_rt);
}

JSBool AtlasScriptInterface_impl::LoadScript(JSContext* cx, const jschar* chars, uintN length, const char* filename, jsval* rval)
{
	JSObject* scriptObj = JS_NewObject(cx, NULL, NULL, NULL);
	if (! scriptObj)
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(scriptObj);
		
	jsval scriptRval;
	JSBool ok = JS_EvaluateUCScript(cx, scriptObj, chars, length, filename, 1, &scriptRval);
	if (! ok)
		return JS_FALSE;
		
	return JS_TRUE;
}

void AtlasScriptInterface_impl::Register(const char* name, JSNative fptr, uintN nargs)
{
	JS_DefineFunction(m_cx, m_atlas, name, fptr, nargs, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
}


ScriptInterface::ScriptInterface(SubmitCommand submitCommand)
	: m(new AtlasScriptInterface_impl())
{
	g_SubmitCommand = submitCommand;
}

ScriptInterface::~ScriptInterface()
{
}

void ScriptInterface::SetCallbackData(void* cbdata)
{
	JS_SetContextPrivate(m->m_cx, cbdata);
}

void* ScriptInterface::GetCallbackData(JSContext* cx)
{
	return JS_GetContextPrivate(cx);
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs)
{
	m->Register(name, fptr, (uintN)nargs);
}

JSContext* ScriptInterface::GetContext()
{
	return m->m_cx;
}

bool ScriptInterface::AddRoot(jsval* ptr)
{
	return JS_AddValueRoot(m->m_cx, ptr) ? true : false;
}

bool ScriptInterface::RemoveRoot(jsval* ptr)
{
	return JS_RemoveValueRoot(m->m_cx, ptr) ? true : false;
}


bool ScriptInterface::SetValue_(const wxString& name, jsval val)
{
	jsval jsName = ToJSVal(m->m_cx, name);

	const uintN argc = 2;
	jsval argv[argc] = { jsName, val };
	jsval rval;
	JSBool ok = JS_CallFunctionName(m->m_cx, m->m_glob, "setValue", argc, argv, &rval);
	return ok ? true : false;
}

bool ScriptInterface::GetValue_(const wxString& name, jsval& ret)
{
	jsval jsName = ToJSVal(m->m_cx, name);

	const uintN argc = 1;
	jsval argv[argc] = { jsName };
	JSBool ok = JS_CallFunctionName(m->m_cx, m->m_glob, "getValue", argc, argv, &ret);
	return ok ? true : false;
}

bool ScriptInterface::CallFunction(jsval val, const char* name)
{
	jsval jsRet;
	std::vector<jsval> argv;
	return CallFunction_(val, name, argv, jsRet);
}

bool ScriptInterface::CallFunction_(jsval val, const char* name, std::vector<jsval>& args, jsval& ret)
{
	const uintN argc = (uintN)args.size();
	jsval* argv = NULL;
	if (argc)
		argv = &args[0];
	wxCHECK(JSVAL_IS_OBJECT(val), false);
	JSBool found;
	wxCHECK(JS_HasProperty(m->m_cx, JSVAL_TO_OBJECT(val), name, &found), false);
	if (! found)
		return false;
	JSBool ok = JS_CallFunctionName(m->m_cx, JSVAL_TO_OBJECT(val), name, argc, argv, &ret);
	return ok ? true : false;
}

bool ScriptInterface::Eval(const wxString& script)
{
	jsval rval;
	JSBool ok = JS_EvaluateScript(m->m_cx, m->m_glob, script.mb_str(), (uintN)script.length(), NULL, 0, &rval);
	return ok ? true : false;
}

bool ScriptInterface::Eval_(const wxString& script, jsval& rval)
{
	JSBool ok = JS_EvaluateScript(m->m_cx, m->m_glob, script.mb_str(), (uintN)script.length(), NULL, 0, &rval);
	return ok ? true : false;
}

void ScriptInterface::LoadScript(const wxString& filename, const wxString& code)
{
	size_t codeLength;
	wxMBConvUTF16 conv;
	wxCharBuffer codeUTF16 = conv.cWC2MB(code.c_str(), code.length()+1, &codeLength);
	jsval rval;
	m->LoadScript(m->m_cx, reinterpret_cast<jschar*>(codeUTF16.data()), (uintN)(codeLength/2), filename.ToAscii(), &rval);
}

////////////////////////////////////////////////////////////////

#define TYPE(elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define NAME(elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define MAKE_STR_(s) #s
#define MAKE_STR(s) MAKE_STR_(s)

#define CONVERT_ARGS(r, data, i, elem) \
	TYPE(elem) a##i; \
	if (! ScriptInterface::FromJSVal< TYPE(elem) >(cx, i < argc ? JS_ARGV(cx, vp)[i] : JSVAL_VOID, a##i)) \
		return JS_FALSE;
		
#define CONVERT_OUTPUTS(r, data, i, elem) \
	JS_DefineProperty(cx, ret, MAKE_STR(NAME(elem)), ScriptInterface::ToJSVal(cx, q.NAME(elem)), \
		NULL, NULL, JSPROP_ENUMERATE);
		
#define ARG_LIST(r, data, i, elem) BOOST_PP_COMMA_IF(i) a##i

#define MESSAGE(name, vals) \
	JSBool call_##name(JSContext* cx, uintN argc, jsval* vp) \
	{ \
		(void)cx; (void)argc; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, vals) \
		g_MessagePasser->Add(SHAREABLE_NEW(m##name, ( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, vals) ))); \
		JS_SET_RVAL(cx, vp, JSVAL_VOID); \
		return JS_TRUE; \
	}
	
#define COMMAND(name, merge, vals) \
	JSBool call_##name(JSContext* cx, uintN argc, jsval* vp) \
	{ \
		(void)cx; (void)argc; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, vals) \
		g_SubmitCommand(new AtlasMessage::m##name (AtlasMessage::d##name ( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, vals) ))); \
		JS_SET_RVAL(cx, vp, JSVAL_VOID); \
		return JS_TRUE; \
	}
	
#define QUERY(name, in_vals, out_vals) \
	JSBool call_##name(JSContext* cx, uintN argc, jsval* vp) \
	{ \
		(void)cx; (void)argc; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, in_vals) \
		q##name q = q##name( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, in_vals) ); \
		q.Post(); \
		JSObject* ret = JS_NewObject(cx, NULL, NULL, NULL); \
		if (! ret) return JS_FALSE; \
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ret)); \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_OUTPUTS, ~, out_vals) \
		return JS_TRUE; \
	}

#define MESSAGES_SKIP_SETUP
#define MESSAGES_SKIP_STRUCTS

// We want to include Messages.h again, with some different definitions,
// so cheat and undefine its include-guard
#undef INCLUDED_MESSAGES

namespace
{
	using namespace AtlasMessage;
	#include "GameInterface/Messages.h"
}

#undef MESSAGE
#undef COMMAND
#undef QUERY

void AtlasScriptInterface_impl::RegisterMessages(JSObject* parent)
{
	using namespace AtlasMessage;
	
	JSObject* obj = JS_DefineObject(m_cx, parent, "Message", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define MESSAGE(name, vals) \
	JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define COMMAND(name, merge, vals) \
	JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define QUERY(name, in_vals, out_vals) \
	JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)in_vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#undef INCLUDED_MESSAGES
	 
	#include "GameInterface/Messages.h"

	#undef MESSAGE
	#undef COMMAND
	#undef QUERY
}
