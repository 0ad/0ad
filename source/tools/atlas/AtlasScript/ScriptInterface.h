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

struct ScriptInterface_impl;
class ScriptInterface
{
public:
	ScriptInterface();
	~ScriptInterface();

	// Defined elsewhere:
	//     template <TR, T0..., TR (*fptr) (T0...)>
	//     void RegisterFunction(const wxString& functionName);
	// (NOTE: The return type must be defined as a ToJSVal<TR> specialisation
	// in ScriptInterface.cpp, else you'll end up with linker errors.)

	void LoadScript(const wxString& filename, const wxString& code);
	wxPanel* LoadScriptAsPanel(const wxString& name, wxWindow* parent);

	template <typename T> static bool FromJSVal(JSContext* cx, jsval val, T& ret);
	template <typename T> static jsval ToJSVal(JSContext* cx, const T& val);
private:
	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<ScriptInterface_impl> m;

// The nasty macro/template bits are split into a separate file so you don't have to look at them
#include "NativeWrapper.inl"
