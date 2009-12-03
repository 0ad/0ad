#ifndef INCLUDED_GUIMANAGER
#define INCLUDED_GUIMANAGER

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "scripting/SpiderMonkey.h"

class CGUI;
struct JSObject;
class IGUIObject;
struct CColor;
struct SGUIIcon;

/**
 * External interface to the GUI system.
 *
 * The GUI consists of a set of pages. Each page is constructed from a
 * series of XML files, and is independent from any other page.
 * Only one page is active at a time. All events and render requests etc
 * will go to the active page. This lets the GUI switch between pre-game menu
 * and in-game UI.
 */
class CGUIManager
{
public:
	CGUIManager();
	~CGUIManager();

	// Load a new GUI page and make it active, All current pages will be destroyed.
	void SwitchPage(const CStrW& name, jsval initData);

	// Load a new GUI page and make it active. All current pages will be retained,
	// and will still be drawn and receive tick events, but will not receive
	// user inputs.
	void PushPage(const CStrW& name, jsval initData);

	// Unload the currently active GUI page, and make the previous page active.
	// (There must be at least two pages when you call this.)
	void PopPage();

	// Hotload pages when their .xml files have changed
	LibError ReloadChangedFiles(const VfsPath& path);

	// These functions are all equivalent to the CGUI functions of the same
	// name, applied to the currently active GUI page:

	bool GetPreDefinedColor(const CStr& name, CColor& output);
	bool IconExists(const CStr& str) const;
	SGUIIcon GetIcon(const CStr& str) const;

	IGUIObject* FindObjectByName(const CStr& name) const;

	void SendEventToAll(const CStr& eventName);
	void TickObjects();
	void Draw();
	void UpdateResolution();

	JSObject* GetScriptObject();

	InReaction HandleEvent(const SDL_Event_* ev);

private:
	struct SGUIPage
	{
		SGUIPage();
		SGUIPage(const SGUIPage&);
		~SGUIPage();
		CStrW name;
		std::set<VfsPath> inputs; // for hotloading
		jsval initData;
		shared_ptr<CGUI> gui;
	};

	void LoadPage(SGUIPage& page);

	shared_ptr<CGUI> top() const;

	typedef std::vector<SGUIPage> PageStackType;
	PageStackType m_PageStack;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
