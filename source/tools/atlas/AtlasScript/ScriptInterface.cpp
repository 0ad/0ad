#include "ScriptInterface.h"

#include <cassert>

#include "js/jsapi.h"

#ifdef _WIN32
# pragma warning(disable: 4996) // avoid complaints about deprecated localtime
#endif

#include "wx/wx.h"

#include "wxJS/common/main.h"
#include "wxJS/ext/jsmembuf.h"
#include "wxJS/io/init.h"
#include "wxJS/gui/init.h"
#include "wxJS/gui/control/panel.h"

#include "GameInterface/Shareable.h"
#include "GameInterface/Messages.h"

// We want to include Messages.h again below, with some different definitions,
// so cheat and undefine its include-guard
#undef INCLUDED_MESSAGES

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define FAIL(msg) do { JS_ReportError(cx, msg); return false; } while (false)

const int RUNTIME_SIZE = 1024*1024; // TODO: how much memory is needed?
const int STACK_CHUNK_SIZE = 8192;

////////////////////////////////////////////////////////////////

namespace
{
	// Use templated structs instead of functions, so that we can use partial specialisation:
	
	template<typename T> struct FromJSVal
	{
		static bool Convert(JSContext* cx, jsval WXUNUSED(v), T& WXUNUSED(out))
		{
			JS_ReportError(cx, "Unrecognised argument type");
			// TODO: SetPendingException turns the error into a JS-catchable exception,
			// but the error report doesn't say anything useful like the line number,
			// so I'm just using ReportError instead for now (and failures are uncatchable
			// and will terminate the whole script)
			//JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Unrecognised argument type")));
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
			if (! JS_ValueToInt32(cx, v, &ret)) return false;
			out = ret;
			return true;
		}
	};

	template<> struct FromJSVal<std::wstring>
	{
		static bool Convert(JSContext* cx, jsval v, std::wstring& out)
		{
			JSString* ret = JS_ValueToString(cx, v); // never returns NULL
			jschar* ch = JS_GetStringChars(ret);
			out = std::wstring(ch, ch+JS_GetStringLength(ret));
			return true;
		}
	};

	template<> struct FromJSVal<std::string>
	{
		static bool Convert(JSContext* cx, jsval v, std::string& out)
		{
			JSString* ret = JS_ValueToString(cx, v); // never returns NULL
			char* ch = JS_GetStringBytes(ret);
			out = std::string(ch);
			return true;
		}
	};

	template<> struct FromJSVal<wxString>
	{
		static bool Convert(JSContext* cx, jsval v, wxString& out)
		{
			JSString* ret = JS_ValueToString(cx, v); // never returns NULL
			jschar* ch = JS_GetStringChars(ret);
			out = wxString((const char*)ch, wxMBConvUTF16(), JS_GetStringLength(ret)*2);
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
			for (jsint i = 0; i < length; ++i)
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
}

template<typename T> bool ScriptInterface::FromJSVal(JSContext* cx, jsval v, T& out)
{
	return ::FromJSVal<T>::Convert(cx, v, out);
}

// Explicit instantiation of functions that would otherwise be unused in this file
// but are required for linking with other files
template bool ScriptInterface::FromJSVal<wxString>(JSContext*, jsval, wxString&);


template<> jsval ScriptInterface::ToJSVal<float>(JSContext* cx, const float& val)
{
	jsval rval = JSVAL_VOID;
	JS_NewDoubleValue(cx, val, &rval); // ignore return value
	return rval;
}

template<> jsval ScriptInterface::ToJSVal<int>(JSContext* WXUNUSED(cx), const int& val)
{
	return INT_TO_JSVAL(val);
}

template<> jsval ScriptInterface::ToJSVal<wxString>(JSContext* cx, const wxString& val)
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
		"global", 0,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
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

	JS_BeginRequest(m_cx); // if you get linker errors, see the comment in the .h about JS_THREADSAFE

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
	wxjs::ext::MemoryBuffer::JSInit(m_cx, m_glob);

	JS_DefineFunction(m_cx, m_glob, "print", ::print, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
	m_atlas = JS_DefineObject(m_cx, m_glob, "Atlas", NULL, NULL, JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "ForceGC", ::ForceGC, 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_atlas, "LoadScript", ::LoadScript, 2, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
	
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


ScriptInterface::ScriptInterface()
	: m(new ScriptInterface_impl)
{
}

ScriptInterface::~ScriptInterface()
{
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs)
{
	m->Register(name, fptr, (uintN)nargs);
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

////////////////////////////////////////////////////////////////

struct MessageWrapper
{
	#define NUMBERED_LIST(z, i, data) , data##i
	#define NUMBERED_LIST2(z, i, data) BOOST_PP_COMMA_IF(i) data##i
	#define CONVERT_ARGS(z, i, data) T##i a##i; if (! ScriptInterface::FromJSVal<T##i>(cx, argv[i], a##i)) return JS_FALSE;
	#define OVERLOADS(z, i, data) \
		template <typename Message BOOST_PP_REPEAT_##z (i, NUMBERED_LIST, typename T) > \
		static JSNative call(Message* (*WXUNUSED(fptr)) ( BOOST_PP_REPEAT_##z(i, NUMBERED_LIST2, T) )) { \
			return &_call<Message BOOST_PP_REPEAT_##z(i, NUMBERED_LIST, T) >; \
		} \
		template <typename Message BOOST_PP_REPEAT_##z (i, NUMBERED_LIST, typename T) > \
		static uintN nargs(Message* (*WXUNUSED(fptr)) ( BOOST_PP_REPEAT_##z(i, NUMBERED_LIST2, T) )) { \
			return i; \
		} \
		template <typename Message BOOST_PP_REPEAT_##z (i, NUMBERED_LIST, typename T) > \
		static JSBool _call(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* WXUNUSED(rval)) \
		{ \
			(void)cx; (void)obj; (void)argc; (void)argv; /* avoid 'unused parameter' warnings */ \
			BOOST_PP_REPEAT_##z (i, CONVERT_ARGS, ~) \
			AtlasMessage::g_MessagePasser->Add(SHAREABLE_NEW(Message, ( BOOST_PP_REPEAT_##z(i, NUMBERED_LIST2, a) ))); \
			return JS_TRUE; \
		}
	BOOST_PP_REPEAT(7, OVERLOADS, ~)
};

void ScriptInterface_impl::RegisterMessages(JSObject* parent)
{
	using namespace AtlasMessage;
	
	JSFunction* ret;
	JSObject* obj = JS_DefineObject(m_cx, parent, "Message", NULL, NULL, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

#define MESSAGE(name, vals) \
	ret = JS_DefineFunction(m_cx, obj, #name, MessageWrapper::call(&m##name::CtorType), MessageWrapper::nargs(&m##name::CtorType), JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

#define QUERY(name, in_vals, out_vals) /* TODO \
	extern void f##name##_wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##_wrapper));*/

#define COMMAND(name, merge, vals) /* TODO \
	extern cmdHandler c##name##_create(); \
	GetCmdHandlers().insert(std::pair<std::string, cmdHandler>("c"#name, c##name##_create()));*/

#undef SHAREABLE_STRUCT
#define SHAREABLE_STRUCT(name)

	#define MESSAGES_SKIP_SETUP
	#include "GameInterface/Messages.h"
}
