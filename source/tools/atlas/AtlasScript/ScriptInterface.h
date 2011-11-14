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

#include <memory>
#include <vector>

#ifdef _WIN32
# define XP_WIN
# define WIN32 // SpiderMonkey expects this

// The jsval struct type causes crashes due to weird miscompilation
// issues in (at least) VC2008, so force it to be the less-type-safe
// non-struct type instead
# define JS_NO_JSVAL_JSID_STRUCT_TYPES

#else

# define XP_UNIX

// (See comment in scriptinterface/ScriptTypes.h)
# if defined(DEBUG) && defined(WITH_SYSTEM_MOZJS185)
#  define JS_NO_JSVAL_JSID_STRUCT_TYPES
# endif

#endif // (we don't support XP_OS2 or XP_BEOS)

#ifdef __GNUC__
# define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif

#ifdef _MSC_VER
# define MSC_VERSION _MSC_VER
#else
# define MSC_VERSION 0
#endif

// Ignore some harmless warnings triggered by jsapi.h
#if GCC_VERSION >= 402 // (older GCCs don't support this pragma)
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#if MSC_VERSION
# pragma warning(push)
# pragma warning(disable:4480) // "nonstandard extension used: specifying underlying type for enum"
# pragma warning(disable:4100) // "unreferenced formal parameter"
#endif

#include "js/jsapi.h"

#if MSC_VERSION
# pragma warning(pop)
#endif
#if GCC_VERSION >= 402
# pragma GCC diagnostic warning "-Wunused-parameter"
# pragma GCC diagnostic warning "-Wredundant-decls"
#endif

class wxString;

namespace AtlasMessage { struct mWorldCommand; }
typedef void (*SubmitCommand)(AtlasMessage::mWorldCommand* command);

struct AtlasScriptInterface_impl;
class ScriptInterface
{
public:
	ScriptInterface(SubmitCommand submitCommand);
	~ScriptInterface();
	void SetCallbackData(void* cbdata);
	static void* GetCallbackData(JSContext* cx);

	template <typename T> bool SetValue(const wxString& name, const T& val);
	template <typename T> bool GetValue(const wxString& name, T& ret);

	bool CallFunction(jsval val, const char* name);
	template <typename T, typename R>
		bool CallFunction(jsval val, const char* name, const T& a, R& ret);
	template <typename T, typename S, typename R>
		bool CallFunction(jsval val, const char* name, const T& a, const S& b, R& ret);

	bool Eval(const wxString& script);
	template <typename T> bool Eval(const wxString& script, T& ret);

	// Defined elsewhere:
	//     template <TR, T0..., TR (*fptr) (void* cbdata, T0...)>
	//     void RegisterFunction(const char* functionName);
	// (NOTE: The return type must be defined as a ToJSVal<TR> specialisation
	// in ScriptInterface.cpp, else you'll end up with linker errors.)

	void LoadScript(const wxString& filename, const wxString& code);

	// Convert a jsval to a C++ type. (This might trigger GC.)
	template <typename T> static bool FromJSVal(JSContext* cx, jsval val, T& ret);
	// Convert a C++ type to a jsval. (This might trigger GC. The return
	// value must be rooted if you don't want it to be collected.)
	template <typename T> static jsval ToJSVal(JSContext* cx, const T& val);

	bool AddRoot(jsval* ptr);
	bool RemoveRoot(jsval* ptr);

	JSContext* GetContext();

private:
	bool SetValue_(const wxString& name, jsval val);
	bool GetValue_(const wxString& name, jsval& ret);
	bool CallFunction_(jsval val, const char* name, std::vector<jsval>& args, jsval& ret);
	bool Eval_(const wxString& name, jsval& ret);

	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<AtlasScriptInterface_impl> m;

// The nasty macro/template bits are split into a separate file so you don't have to look at them
#include "NativeWrapper.inl"

template <typename T>
bool ScriptInterface::SetValue(const wxString& name, const T& val)
{
	return SetValue_(name, ToJSVal(GetContext(), val));
}

template <typename T>
bool ScriptInterface::GetValue(const wxString& name, T& ret)
{
	jsval jsRet;
	if (! GetValue_(name, jsRet)) return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template <typename T, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T& a, R& ret)
{
	jsval jsRet;
	std::vector<jsval> argv;
	argv.push_back(ToJSVal(GetContext(), a));
	bool ok = CallFunction_(val, name, argv, jsRet);
	if (! ok) return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template <typename T, typename S, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T& a, const S& b, R& ret)
{
	jsval jsRet;
	std::vector<jsval> argv;
	argv.push_back(ToJSVal(GetContext(), a));
	argv.push_back(ToJSVal(GetContext(), b));
	bool ok = CallFunction_(val, name, argv, jsRet);
	if (! ok) return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template <typename T>
bool ScriptInterface::Eval(const wxString& script, T& ret)
{
	jsval jsRet;
	if (! Eval_(script, jsRet)) return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

/**
 * A trivial wrapper around a jsval. Used to avoid template overload ambiguities
 * with jsval (which is just an integer), for any code that uses
 * ScriptInterface::ToJSVal or ScriptInterface::FromJSVal
 */
class CScriptVal
{
public:
	CScriptVal() : m_Val(JSVAL_VOID) { }
	CScriptVal(jsval val) : m_Val(val) { }

	jsval get() const { return m_Val; }

private:
	jsval m_Val;
};
