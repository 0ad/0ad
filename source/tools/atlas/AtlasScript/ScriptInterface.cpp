/* Copyright (C) 2009 Wildfire Games.
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

#include "js/jsapi.h"

#ifdef _WIN32
# pragma warning(disable: 4996) // avoid complaints about deprecated localtime
#endif

#include "wx/wx.h"

#include "wxJS/common/main.h"
#include "wxJS/ext/wxjs_ext.h"
#include "wxJS/io/init.h"
#include "wxJS/gui/init.h"
#include "wxJS/gui/control/panel.h"
#include "wxJS/gui/misc/bitmap.h"
#include "wxJS/gui/event/jsevent.h"
#include "wxJS/gui/event/key.h"
#include "wxJS/gui/event/mouse.h"

#include "GameInterface/Shareable.h"
#include "GameInterface/Messages.h"


#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#ifdef USE_VALGRIND
# include <valgrind/valgrind.h>
#endif

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

	template<> struct FromJSVal<jsval>
	{
		static bool Convert(JSContext* WXUNUSED(cx), jsval v, jsval& out)
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
			jschar* ch = JS_GetStringChars(ret);
			out = std::wstring(ch, ch+JS_GetStringLength(ret));
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
			char* ch = JS_GetStringBytes(ret);
			out = std::string(ch);
			return true;
		}
	};

	template<> struct FromJSVal<wxString>
	{
		static bool Convert(JSContext* cx, jsval v, wxString& out)
		{
			JSString* ret = JS_ValueToString(cx, v);
			if (! ret)
				FAIL("Argument must be convertible to a string");
			jschar* ch = JS_GetStringChars(ret);
			out = wxString((const char*)ch, wxMBConvUTF16(), (size_t)(JS_GetStringLength(ret)*2));
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

	template<> struct ToJSVal<float>
	{
		static jsval Convert(JSContext* cx, const float& val)
		{
			jsval rval = JSVAL_VOID;
			JS_NewDoubleValue(cx, val, &rval); // ignore return value
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
		static jsval Convert(JSContext* WXUNUSED(cx), const size_t& val)
		{
			return INT_TO_JSVAL(val);
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

	////////////////////////////////////////////////////////////////
	// wxJS types:

	template<> struct ToJSVal<wxKeyEvent>
	{
		static jsval Convert(JSContext* cx, const wxKeyEvent& val)
		{
			wxKeyEvent& evt = const_cast<wxKeyEvent&>(val); // ugly, but needed for wxJS
			wxjs::gui::PrivKeyEvent *jsEvent = new wxjs::gui::PrivKeyEvent(evt);
			jsEvent->SetScoop(false); // (wxJS will clone the event now, and not modify the const version)
			return wxjs::gui::KeyEvent::CreateObject(cx, jsEvent);
		}
	};

	template<> struct ToJSVal<wxMouseEvent>
	{
		static jsval Convert(JSContext* cx, const wxMouseEvent& val)
		{
			wxMouseEvent& evt = const_cast<wxMouseEvent&>(val); // see comments above for KeyEvent
			wxjs::gui::PrivMouseEvent *jsEvent = new wxjs::gui::PrivMouseEvent(evt);
			jsEvent->SetScoop(false);
			return wxjs::gui::MouseEvent::CreateObject(cx, jsEvent);
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
			JS_AddRoot(cx, &obj);
			for (size_t i = 0; i < val.size(); ++i)
			{
				jsval el = ToJSVal<T>::Convert(cx, val[i]);
				JS_SetElement(cx, obj, (jsint)i, &el);
			}
			JS_RemoveRoot(cx, &obj);
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

	////////////////////////////////////////////////////////////////
	// AtlasMessage structures:

	template<> struct FromJSVal<AtlasMessage::Position>
	{
		static bool Convert(JSContext* cx, jsval v, AtlasMessage::Position& out)
		{
			JSObject* obj;
			if (! JS_ValueToObject(cx, v, &obj) || obj == NULL)
				FAIL("Argument must be an object");
			jsval val;

			float x, y, z;
			if (! JS_GetProperty(cx, obj, "x", &val))
				FAIL("Failed to get 'x'");
			if (! ScriptInterface::FromJSVal(cx, val, x))
				FAIL("Failed to convert 'x'");
			if (! JS_GetProperty(cx, obj, "y", &val))
				FAIL("Failed to get 'y'");
			if (! ScriptInterface::FromJSVal(cx, val, y))
				FAIL("Failed to convert 'y'");
			if (! JS_GetProperty(cx, obj, "z", &val))
				FAIL("Failed to get 'z'");
			if (! ScriptInterface::FromJSVal(cx, val, z))
				FAIL("Failed to convert 'z'");

			out = AtlasMessage::Position(x, y, z);
			return true;
		}
	};

	template<> struct ToJSVal<AtlasMessage::sTerrainGroupPreview>
	{
		static jsval Convert(JSContext* cx, const AtlasMessage::sTerrainGroupPreview& val)
		{
			JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
			if (! obj) return JSVAL_VOID;
			JS_AddRoot(cx, &obj);
			
			JS_DefineProperty(cx, obj, "name", ToJSVal<std::wstring>::Convert(cx, *val.name), NULL, NULL, JSPROP_ENUMERATE);
			
			unsigned char* buf = (unsigned char*)(malloc(val.imagedata.GetSize()));
			memcpy(buf, val.imagedata.GetBuffer(), val.imagedata.GetSize());
			jsval bmp = wxjs::gui::Bitmap::CreateObject(cx, new wxBitmap (wxImage(val.imagewidth, val.imageheight, buf)));
			JS_DefineProperty(cx, obj, "imagedata", bmp, NULL, NULL, JSPROP_ENUMERATE);
			
			JS_RemoveRoot(cx, &obj);
			
			return OBJECT_TO_JSVAL(obj);
		}
	};

	template<> struct ToJSVal<AtlasMessage::sObjectsListItem>
	{
		static jsval Convert(JSContext* cx, const AtlasMessage::sObjectsListItem& val)
		{
			JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
			if (! obj) return JSVAL_VOID;
			JS_AddRoot(cx, &obj);
			JS_DefineProperty(cx, obj, "id", ToJSVal<std::wstring>::Convert(cx, *val.id), NULL, NULL, JSPROP_ENUMERATE);
			JS_DefineProperty(cx, obj, "name", ToJSVal<std::wstring>::Convert(cx, *val.name), NULL, NULL, JSPROP_ENUMERATE);
			JS_DefineProperty(cx, obj, "type", ToJSVal<int>::Convert(cx, val.type), NULL, NULL, JSPROP_ENUMERATE);
			JS_RemoveRoot(cx, &obj);
			return OBJECT_TO_JSVAL(obj);
		}
	};

	template<> struct ToJSVal<AtlasMessage::sObjectSettings>
	{
		static jsval Convert(JSContext* cx, const AtlasMessage::sObjectSettings& val)
		{
			JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
			if (! obj) return JSVAL_VOID;
			JS_AddRoot(cx, &obj);
			JS_DefineProperty(cx, obj, "player", ToJSVal<size_t>::Convert(cx, val.player), NULL, NULL, JSPROP_ENUMERATE);
			JS_DefineProperty(cx, obj, "selections", ToJSVal<std::vector<std::wstring> >::Convert(cx, *val.selections), NULL, NULL, JSPROP_ENUMERATE);
			JS_DefineProperty(cx, obj, "variantgroups", ToJSVal<std::vector<std::vector<std::wstring> > >::Convert(cx, *val.variantgroups), NULL, NULL, JSPROP_ENUMERATE);
			JS_RemoveRoot(cx, &obj);
			return OBJECT_TO_JSVAL(obj);
		}
	};

	template<> struct FromJSVal<AtlasMessage::sObjectSettings>
	{
		static bool Convert(JSContext* cx, jsval v, AtlasMessage::sObjectSettings& out)
		{
			JSObject* obj;
			if (! JS_ValueToObject(cx, v, &obj) || obj == NULL)
				FAIL("Argument must be an object");
			jsval val;

			int player;
			if (! JS_GetProperty(cx, obj, "player", &val))
				FAIL("Failed to get 'player'");
			if (! ScriptInterface::FromJSVal(cx, val, player))
				FAIL("Failed to convert 'player'");
			out.player = player;

			std::vector<std::wstring> selections;
			if (! JS_GetProperty(cx, obj, "selections", &val))
				FAIL("Failed to get 'selections'");
			if (! ScriptInterface::FromJSVal(cx, val, selections))
				FAIL("Failed to convert 'selections'");
			out.selections = selections;
			
			// variantgroups is only used in engine-to-editor, so we don't
			// bother converting it here

			return true;
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
template bool ScriptInterface::FromJSVal<wxString>(JSContext*, jsval, wxString&);
template bool ScriptInterface::FromJSVal<float>(JSContext*, jsval, float&);
template bool ScriptInterface::FromJSVal<jsval>(JSContext*, jsval, jsval&);
template jsval ScriptInterface::ToJSVal<wxString>(JSContext*, wxString const&);
template jsval ScriptInterface::ToJSVal<wxKeyEvent>(JSContext*, wxKeyEvent const&);
template jsval ScriptInterface::ToJSVal<wxMouseEvent>(JSContext*, wxMouseEvent const&);
template jsval ScriptInterface::ToJSVal<int>(JSContext*, int const&);
template jsval ScriptInterface::ToJSVal<float>(JSContext*, float const&);
template jsval ScriptInterface::ToJSVal<std::vector<int> >(JSContext*, std::vector<int> const&);

////////////////////////////////////////////////////////////////

struct ScriptInterface_impl
{
	ScriptInterface_impl();
	~ScriptInterface_impl();
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
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
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
#ifdef USE_VALGRIND
		// When running under Valgrind, print more information in the error message
		VALGRIND_PRINTF_BACKTRACE("->");
#endif
		wxPrintf(_T("wxJS %s: %s\n--------\n"), isWarning ? _T("warning") : _T("error"), logMessage.c_str());
	}

	// Functions in the Atlas.* namespace:
	
	JSBool ForceGC(JSContext* cx, JSObject* WXUNUSED(obj), uintN WXUNUSED(argc), jsval* WXUNUSED(argv), jsval* WXUNUSED(rval))
	{
		JS_GC(cx);
		return JS_TRUE;
	}

	JSBool LoadScript(JSContext* cx, JSObject* WXUNUSED(obj), uintN WXUNUSED(argc), jsval* argv, jsval* rval)
	{
		if (! ( JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1]) ))
			return JS_FALSE;
		
		JSString* name = JSVAL_TO_STRING(argv[0]);
		JSString* code = JSVAL_TO_STRING(argv[1]);
		
		return ScriptInterface_impl::LoadScript(cx,
				JS_GetStringChars(code), (uintN)JS_GetStringLength(code),
				JS_GetStringBytes(name), rval);
	}
	
	// Functions in the global namespace:

	JSBool print(JSContext* cx, JSObject* WXUNUSED(obj), uintN argc, jsval* argv, jsval* WXUNUSED(rval))
	{
		for (uintN i = 0; i < argc; ++i)
		{
			std::string str;
			if (! ScriptInterface::FromJSVal(cx, argv[i], str))
				return JS_FALSE;
			printf("%s", str.c_str());
		}
		fflush(stdout);
		return JS_TRUE;
	}
}

ScriptInterface_impl::ScriptInterface_impl()
{
	JSBool ok;

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

	m_glob = JS_NewObject(m_cx, &global_class, NULL, NULL);
	ok = JS_InitStandardClasses(m_cx, m_glob);
	
	JS_DefineProperty(m_cx, m_glob, "global", OBJECT_TO_JSVAL(m_glob), NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	wxjs::gui::InitClass(m_cx, m_glob);
	wxjs::io::InitClass(m_cx, m_glob);
	wxjs::ext::InitClass(m_cx, m_glob);
	wxjs::ext::InitObject(m_cx, m_glob);

	JS_DefineFunction(m_cx, m_glob, "print", ::print, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
	m_atlas = JS_DefineObject(m_cx, m_glob, "Atlas", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "ForceGC", ::ForceGC, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "LoadScript", ::LoadScript, 2, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineObject(m_cx, m_atlas, "State", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
	RegisterMessages(m_atlas);
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	JS_EndRequest(m_cx);
	JS_DestroyContext(m_cx);
	JS_DestroyRuntime(m_rt);
}

JSBool ScriptInterface_impl::LoadScript(JSContext* cx, const jschar* chars, uintN length, const char* filename, jsval* rval)
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

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uintN nargs)
{
	JS_DefineFunction(m_cx, m_atlas, name, fptr, nargs, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
}


ScriptInterface::ScriptInterface(SubmitCommand submitCommand)
	: m(new ScriptInterface_impl())
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

bool ScriptInterface::AddRoot(void* ptr)
{
	return JS_AddRoot(m->m_cx, ptr) ? true : false;
}

bool ScriptInterface::RemoveRoot(void* ptr)
{
	return JS_RemoveRoot(m->m_cx, ptr) ? true : false;
}

ScriptInterface::LocalRootScope::LocalRootScope(ScriptInterface& scriptInterface)
	: m_ScriptInterface(scriptInterface)
{
	m_OK = JS_EnterLocalRootScope(m_ScriptInterface.m->m_cx) ? true : false;
}

ScriptInterface::LocalRootScope::~LocalRootScope()
{
	if (m_OK)
		JS_LeaveLocalRootScope(m_ScriptInterface.m->m_cx);
}

bool ScriptInterface::LocalRootScope::OK()
{
	return m_OK;
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
	const uintN argc = args.size();
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
	JSBool ok = JS_EvaluateScript(m->m_cx, m->m_glob, script.mb_str(), script.length(), NULL, 0, &rval);
	return ok ? true : false;
}

bool ScriptInterface::Eval_(const wxString& script, jsval& rval)
{
	JSBool ok = JS_EvaluateScript(m->m_cx, m->m_glob,
		script.mb_str(), script.length(), NULL, 0, &rval);
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

wxPanel* ScriptInterface::LoadScriptAsPanel(const wxString& name, wxWindow* parent)
{
	wxPanel* panel = new wxPanel(parent, -1);
	JSObject* jsWindow = JSVAL_TO_OBJECT(wxjs::gui::Panel::CreateObject(m->m_cx, panel));
	panel->SetClientObject(new wxjs::JavaScriptClientData(m->m_cx, jsWindow, true, false));
	
	jsval jsName = ToJSVal(m->m_cx, name);
	
	const uintN argc = 2;
	jsval argv[argc] = { jsName, OBJECT_TO_JSVAL(jsWindow) };
	
	jsval rval;
	JS_CallFunctionName(m->m_cx, m->m_glob, "loadScript", argc, argv, &rval); // TODO: error checking
	return panel;
}

// TODO: this is an ugly function to provide
std::pair<wxPanel*, wxPanel*> ScriptInterface::LoadScriptAsSidebar(const wxString& name, wxWindow* side, wxWindow* bottom)
{
	wxPanel* sidePanel = new wxPanel(side, -1);
	JSObject* jsSideWindow = JSVAL_TO_OBJECT(wxjs::gui::Panel::CreateObject(m->m_cx, sidePanel));
	sidePanel->SetClientObject(new wxjs::JavaScriptClientData(m->m_cx, jsSideWindow, true, false));
	
	wxPanel* bottomPanel = new wxPanel(bottom, -1);
	JSObject* jsBottomWindow = JSVAL_TO_OBJECT(wxjs::gui::Panel::CreateObject(m->m_cx, bottomPanel));
	bottomPanel->SetClientObject(new wxjs::JavaScriptClientData(m->m_cx, jsBottomWindow, true, false));

	jsval jsName = ToJSVal(m->m_cx, name);
	
	const uintN argc = 3;
	jsval argv[argc] = { jsName, OBJECT_TO_JSVAL(jsSideWindow), OBJECT_TO_JSVAL(jsBottomWindow) };
	
	jsval rval;
	JS_CallFunctionName(m->m_cx, m->m_glob, "loadScript", argc, argv, &rval); // TODO: error checking
	
	// TODO: This really need a better way to handle these two windows (of which one is optional)...
	if (bottomPanel->GetChildren().size() != 0)
		return std::make_pair(sidePanel, bottomPanel);
	else
	{
		bottomPanel->Destroy();
		return std::make_pair(sidePanel, static_cast<wxPanel*>(NULL));
	}
}

////////////////////////////////////////////////////////////////

#define TYPE(elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define NAME(elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define MAKE_STR_(s) #s
#define MAKE_STR(s) MAKE_STR_(s)

#define CONVERT_ARGS(r, data, i, elem) \
	TYPE(elem) a##i; \
	if (! ScriptInterface::FromJSVal< TYPE(elem) >(cx, argv[i], a##i)) \
		return JS_FALSE;
		
#define CONVERT_OUTPUTS(r, data, i, elem) \
	JS_DefineProperty(cx, ret, MAKE_STR(NAME(elem)), ScriptInterface::ToJSVal(cx, q.NAME(elem)), \
		NULL, NULL, JSPROP_ENUMERATE);
		
#define ARG_LIST(r, data, i, elem) BOOST_PP_COMMA_IF(i) a##i

#define MESSAGE(name, vals) \
	JSBool call_##name(JSContext* cx, JSObject* WXUNUSED(obj), uintN WXUNUSED(argc), jsval* argv, jsval* WXUNUSED(rval)) \
	{ \
		(void)cx; (void)argv; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, vals) \
		g_MessagePasser->Add(SHAREABLE_NEW(m##name, ( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, vals) ))); \
		return JS_TRUE; \
	}
	
#define COMMAND(name, merge, vals) \
	JSBool call_##name(JSContext* cx, JSObject* WXUNUSED(obj), uintN WXUNUSED(argc), jsval* argv, jsval* WXUNUSED(rval)) \
	{ \
		(void)cx; (void)argv; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, vals) \
		g_SubmitCommand(new AtlasMessage::m##name (AtlasMessage::d##name ( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, vals) ))); \
		return JS_TRUE; \
	}
	
#define QUERY(name, in_vals, out_vals) \
	JSBool call_##name(JSContext* cx, JSObject* WXUNUSED(obj), uintN WXUNUSED(argc), jsval* argv, jsval* rval) \
	{ \
		(void)argv; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_SEQ_FOR_EACH_I(CONVERT_ARGS, ~, in_vals) \
		q##name q = q##name( BOOST_PP_SEQ_FOR_EACH_I(ARG_LIST, ~, in_vals) ); \
		q.Post(); \
		JSObject* ret = JS_NewObject(cx, NULL, NULL, NULL); \
		if (! ret) return JS_FALSE; \
		*rval = OBJECT_TO_JSVAL(ret); \
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

void ScriptInterface_impl::RegisterMessages(JSObject* parent)
{
	using namespace AtlasMessage;
	
	JSFunction* ret;
	JSObject* obj = JS_DefineObject(m_cx, parent, "Message", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define MESSAGE(name, vals) \
	ret = JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define COMMAND(name, merge, vals) \
	ret = JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#define QUERY(name, in_vals, out_vals) \
	ret = JS_DefineFunction(m_cx, obj, #name, call_##name, BOOST_PP_SEQ_SIZE((~)in_vals)-1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

	#undef INCLUDED_MESSAGES
	 
	#include "GameInterface/Messages.h"

	#undef MESSAGE
	#undef COMMAND
	#undef QUERY
}

