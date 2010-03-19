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
#else
# define XP_UNIX
#endif // (we don't support XP_OS2 or XP_BEOS)

#include "js/jspubtd.h"

class wxWindow;
class wxString;
class wxPanel;

namespace AtlasMessage { struct mWorldCommand; }
typedef void (*SubmitCommand)(AtlasMessage::mWorldCommand* command);

struct ScriptInterface_impl;
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
	wxPanel* LoadScriptAsPanel(const wxString& name, wxWindow* parent);
	std::pair<wxPanel*, wxPanel*> LoadScriptAsSidebar(const wxString& name, wxWindow* side, wxWindow* bottom);

	// Convert a jsval to a C++ type. (This might trigger GC.)
	template <typename T> static bool FromJSVal(JSContext* cx, jsval val, T& ret);
	// Convert a C++ type to a jsval. (This might trigger GC. The return
	// value must be rooted if you don't want it to be collected.)
	template <typename T> static jsval ToJSVal(JSContext* cx, const T& val);

	bool AddRoot(void* ptr);
	bool RemoveRoot(void* ptr);

	// Helper class for automatically rooting values
	class LocalRootScope
	{
		ScriptInterface& m_ScriptInterface;
		bool m_OK;
	public:
		// Tries to enter local root scope, so newly created
		// values won't be GCed. This might fail, so check OK()
		LocalRootScope(ScriptInterface& scriptInterface);
		// Returns true if the local root scope was successfully entered
		bool OK();
		// Leaves the local root scope
		~LocalRootScope();
	private:
		LocalRootScope& operator=(const LocalRootScope&);
	};
	#define LOCAL_ROOT_SCOPE LocalRootScope scope(*this); if (! scope.OK()) return false

private:
	JSContext* GetContext();
	bool SetValue_(const wxString& name, jsval val);
	bool GetValue_(const wxString& name, jsval& ret);
	bool CallFunction_(jsval val, const char* name, std::vector<jsval>& args, jsval& ret);
	bool Eval_(const wxString& name, jsval& ret);

	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<ScriptInterface_impl> m;

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
	LOCAL_ROOT_SCOPE;
	jsval jsRet;
	if (! GetValue_(name, jsRet)) return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template <typename T, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T& a, R& ret)
{
	LOCAL_ROOT_SCOPE;
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
	LOCAL_ROOT_SCOPE;
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
	LOCAL_ROOT_SCOPE;
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
	CScriptVal() : m_Val(0) { }
	CScriptVal(jsval val) : m_Val(val) { }

	jsval get() const { return m_Val; }

private:
	jsval m_Val;
};
