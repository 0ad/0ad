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

#ifdef _WIN32
# define XP_WIN
#else
# define XP_UNIX
#endif // (we don't support XP_OS2 or XP_BEOS)

// NOTE: This requires a threadsafe SpiderMonkey - make sure you compile it with
// the right options, else it'll fail when linking (which might not be until runtime,
// and then you'll wish you saw this comment before spending so much time investigating
// the problem).
// ((There are a few places where SM really needs to be told to be threadsafe, even
// though we're not actually sharing runtimes between threads.))
#define JS_THREADSAFE

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

	template <typename T> void SetValue(const wxString& name, const T& val);
	template <typename T> bool GetValue(const wxString& name, T& ret);
	void Eval(const wxString& script);
	template <typename T> bool Eval(const wxString& script, T& ret);

	// Defined elsewhere:
	//     template <TR, T0..., TR (*fptr) (void* cbdata, T0...)>
	//     void RegisterFunction(const char* functionName);
	// (NOTE: The return type must be defined as a ToJSVal<TR> specialisation
	// in ScriptInterface.cpp, else you'll end up with linker errors.)

	void LoadScript(const wxString& filename, const wxString& code);
	wxPanel* LoadScriptAsPanel(const wxString& name, wxWindow* parent);
	std::pair<wxPanel*, wxPanel*> LoadScriptAsSidebar(const wxString& name, wxWindow* side, wxWindow* bottom);

	template <typename T> static bool FromJSVal(JSContext* cx, jsval val, T& ret);
	template <typename T> static jsval ToJSVal(JSContext* cx, const T& val);
private:
	JSContext* GetContext();
	void AddRoot(void* ptr);
	void RemoveRoot(void* ptr);
	void SetValue_(const wxString& name, jsval val);
	bool GetValue_(const wxString& name, jsval& ret);
	bool Eval_(const wxString& name, jsval& ret);

	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<ScriptInterface_impl> m;

// The nasty macro/template bits are split into a separate file so you don't have to look at them
#include "NativeWrapper.inl"

template <typename T>
void ScriptInterface::SetValue(const wxString& name, const T& val)
{
	return SetValue_(name, ToJSVal(GetContext(), val));
}

template <typename T>
bool ScriptInterface::GetValue(const wxString& name, T& ret)
{
	jsval jsRet;
	if (! GetValue_(name, jsRet)) return false;
	AddRoot(&jsRet); // root it while we do some more work (TODO: is this really necessary?)
	bool ok = FromJSVal(GetContext(), jsRet, ret);
	RemoveRoot(&jsRet);
	return ok;
}

template <typename T>
bool ScriptInterface::Eval(const wxString& script, T& ret)
{
	jsval jsRet;
	if (! Eval_(script, jsRet)) return false;
	AddRoot(&jsRet); // root it while we do some more work
	bool ok = FromJSVal(GetContext(), jsRet, ret);
	RemoveRoot(&jsRet);
	return ok;
}
