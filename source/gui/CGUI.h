/* Copyright (C) 2022 Wildfire Games.
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

/*
 * This is the top class of the whole GUI, all objects
 * and settings are stored within this class.
 */

#ifndef INCLUDED_CGUI
#define INCLUDED_CGUI

#include "gui/GUITooltip.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SGUIIcon.h"
#include "gui/SGUIMessage.h"
#include "gui/SGUIStyle.h"
#include "lib/input.h"
#include "maths/Rect.h"
#include "maths/Size2D.h"
#include "maths/Vector2D.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptForward.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern const double SELECT_DBLCLICK_RATE;

class CCanvas2D;
class CGUISpriteInstance;
class CGUISprite;
class IGUIObject;
struct SGUIImageEffects;
struct SGUIScrollBarStyle;

class GUIProxyProps;

using map_pObjects = std::map<CStr, IGUIObject*>;

/**
 * The main object that represents a whole GUI page.
 */
class CGUI
{
	NONCOPYABLE(CGUI);

private:
	// Private typedefs
	using ConstructObjectFunction = IGUIObject* (*)(CGUI&);

public:
	CGUI(const std::shared_ptr<ScriptContext>& context);
	~CGUI();

	/**
	 * Informs the GUI page which GUI object types may be constructed from XML.
	 */
	void AddObjectTypes();

	/**
	 * Performs processing that should happen every frame
	 * (including sending the "Tick" event to scripts)
	 */
	void TickObjects();

	/**
	 * Sends a specified script event to every object
	 *
	 * @param eventName String representation of event name
	 */
	void SendEventToAll(const CStr& eventName);

	/**
	 * Sends a specified script event to every object
	 *
	 * @param eventName String representation of event name
	 * @param paramData JS::HandleValueArray storing the arguments passed to the event handler.
	 */
	void SendEventToAll(const CStr& eventName, const JS::HandleValueArray& paramData);

	/**
	 * Displays the whole GUI
	 */
	void Draw(CCanvas2D& canvas);

	/**
	 * Draw GUI Sprite
	 *
	 * @param Sprite Object referring to the sprite (which also caches
	 *        calculations for faster rendering)
	 * @param Canvas Canvas to draw on
	 * @param Rect Position and Size
	 * @param Clipping The sprite shouldn't be drawn outside this rectangle
	 */
	void DrawSprite(const CGUISpriteInstance& Sprite, CCanvas2D& canvas, const CRect& Rect, const CRect& Clipping = CRect());

	/**
	 * The replacement of Process(), handles an SDL_Event_
	 *
	 * @param ev SDL Event, like mouse/keyboard input
	 */
	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * Load a GUI XML file into the GUI.
	 *
	 * <b>VERY IMPORTANT!</b> All \<styles\>-files must be read before
	 * everything else!
	 *
	 * @param Filename Name of file
	 * @param Paths Set of paths; all XML and JS files loaded will be added to this
	 */
	void LoadXmlFile(const VfsPath& Filename, std::unordered_set<VfsPath>& Paths);

	/**
	 * Called after all XML files linked in the page file were loaded.
	 */
	void LoadedXmlFiles();

	/**
	 * Allows the JS side to modify the hotkey setting assigned to a GUI object.
	 */
	void SetObjectHotkey(IGUIObject* pObject, const CStr& hotkeyTag);
	void UnsetObjectHotkey(IGUIObject* pObject, const CStr& hotkeyTag);

	/**
	 * Allows the JS side to modify the style setting assigned to a GUI object.
	 */
	void SetObjectStyle(IGUIObject* pObject, const CStr& styleName);
	void UnsetObjectStyle(IGUIObject* pObject);

	/**
	 * Allows the JS side to add or remove global hotkeys.
	 */
	void SetGlobalHotkey(const CStr& hotkeyTag, const CStr& eventName, JS::HandleValue function);
	void UnsetGlobalHotkey(const CStr& hotkeyTag, const CStr& eventName);

	/**
	 * Return the object which is an ancestor of every other GUI object.
	 */
	IGUIObject* GetBaseObject();

	/**
	 * Checks if object exists and return true or false accordingly
	 *
	 * @param Name String name of object
	 * @return true if object exists
	 */
	bool ObjectExists(const CStr& Name) const;

	/**
	 * Returns the GUI object with the desired name, or nullptr
	 * if no match is found,
	 *
	 * @param Name String name of object
	 * @return Matching object, or nullptr
	 */
	IGUIObject* FindObjectByName(const CStr& Name) const;

	/**
	 * Returns the GUI object under the mouse, or nullptr if none.
	 */
	IGUIObject* FindObjectUnderMouse();

	/**
	 * Returns the current screen coordinates of the cursor.
	 */
	const CVector2D& GetMousePos() const { return m_MousePos; }

	/**
	 * Returns the currently pressed mouse buttons.
	 */
	const unsigned int& GetMouseButtons() { return m_MouseButtons; }

	const SGUIScrollBarStyle* GetScrollBarStyle(const CStr& style) const;

	/**
	 * Returns the current GUI window size.
	 */
	CSize2D GetWindowSize() const;

	/**
	 * The GUI needs to have all object types inputted and
	 * their constructors. Also it needs to associate a type
	 * by a string name of the type.
	 *
	 * To add a type:
	 * @code
	 * AddObjectType("button", &CButton::ConstructObject);
	 * @endcode
	 *
	 * @param str Reference name of object type
	 * @param pFunc Pointer of function ConstuctObject() in the object
	 *
	 * @see CGUI#ConstructObject()
	 */
	void AddObjectType(const CStr& str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

	/**
	 * Update Resolution, should be called every time the resolution
	 * of the OpenGL screen has been changed, this is because it needs
	 * to re-cache all its actual sizes
	 *
	 * Needs no input since screen resolution is global.
	 *
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateResolution();

	/**
	 * Check if an icon exists
	 */
	bool HasIcon(const CStr& name) const { return (m_Icons.find(name) != m_Icons.end()); }

	/**
	 * Get Icon (a const reference, can never be changed)
	 */
	const SGUIIcon& GetIcon(const CStr& name) const { return m_Icons.at(name); }

	/**
	 * Check if a style exists
	 */
	bool HasStyle(const CStr& name) const { return (m_Styles.find(name) != m_Styles.end()); }

	/**
	 * Get Style if it exists, otherwise throws an exception.
	 */
	const SGUIStyle& GetStyle(const CStr& name) const { return m_Styles.at(name); }

	/**
	 * Check if a predefined color of that name exists.
	 */
	bool HasPreDefinedColor(const CStr& name) const { return (m_PreDefinedColors.find(name) != m_PreDefinedColors.end()); }

	/**
	 * Resolve the predefined color if it exists, otherwise throws an exception.
	 */
	const CGUIColor& GetPreDefinedColor(const CStr& name) const { return m_PreDefinedColors.at(name); }

	GUIProxyProps* GetProxyData(const js::BaseProxyHandler* ptr) { return m_ProxyData.at(ptr).get(); }

	std::shared_ptr<ScriptInterface> GetScriptInterface() { return m_ScriptInterface; };

private:
	/**
	 * The CGUI takes ownership of the child object and links the parent with the child.
	 * Returns false on failure to take over ownership of the child object.
	 */
	bool AddObject(IGUIObject& parent, IGUIObject& child);

	/**
	 * You input the name of the object type, and let's
	 * say you input "button", then it will construct a
	 * CGUIObjet* as a CButton.
	 *
	 * @param str Name of object type
	 * @return Newly constructed IGUIObject (but constructed as a subclass)
	 */
	IGUIObject* ConstructObject(const CStr& str);

public:
	/**
	 * Get Focused Object.
	 */
	IGUIObject* GetFocusedObject() { return m_FocusedObject; }

	/**
	 * Change focus to new object.
	 * Will send LOST_FOCUS/GOT_FOCUS messages as appropriate.
	 * pObject can be nullptr to remove all focus.
	 */
	void SetFocusedObject(IGUIObject* pObject);

	/**
	 * Alert the focussed object of this GUIPage that the focus of the page has changed.
	 */
	void SendFocusMessage(EGUIMessageType msg);

	/**
	 * Reads a string value and modifies the given value of type T if successful.
	 * Does not change the value upon conversion failure.
	 *
	 * @param pGUI The GUI page which may contain data relevant to the parsing
	 *             (for example predefined colors).
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 */
	template <typename T>
	static bool ParseString(const CGUI* pGUI, const CStrW& Value, T& tOutput);

private:

	//--------------------------------------------------------
	/** @name XML Reading Xeromyces specific subroutines
	 *
	 * These does not throw!
	 * Because when reading in XML files, it won't be fatal
	 * if an error occurs, perhaps one particular object
	 * fails, but it'll still continue reading in the next.
	 * All Error are reported with ReportParseError
	 */
	//--------------------------------------------------------

	/*
		Xeromyces_* functions tree
		<objects> (ReadRootObjects)
		 |
		 +-<script> (ReadScript)
		 |
		 +-<object> (ReadObject)
			|
			+-<action>
			|
			+-Optional Type Extensions (IGUIObject::ReadExtendedElement) TODO
			|
			+-<<object>> *recursive*


		<styles> (ReadRootStyles)
		 |
		 +-<style> (ReadStyle)


		<sprites> (ReadRootSprites)
		 |
		 +-<sprite> (ReadSprite)
			|
			+-<image> (ReadImage)


		<setup> (ReadRootSetup)
		 |
		 +-<tooltip> (ReadToolTip)
		 |
		 +-<scrollbar> (ReadScrollBar)
		 |
		 +-<icon> (ReadIcon)
		 |
		 +-<color> (ReadColor)
	*/
	//@{

	// Read Roots

	/**
	 * Reads in the root element \<objects\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the objects-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param Paths		Collects the set of all XML/JS files that are loaded
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootObjects(const XMBData& xmb, XMBElement element, std::unordered_set<VfsPath>& Paths);

	/**
	 * Reads in the root element \<sprites\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprites-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootSprites(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the root element \<styles\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the styles-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootStyles(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the root element \<setup\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the setup-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootSetup(const XMBData& xmb, XMBElement element);

	// Read Subs

	/**
	 * Notice! Recursive function!
	 *
	 * Read in an \<object\> (the XMBElement) and stores it
	 * as a child in the pParent.
	 *
	 * It will also check the object's children and call this function
	 * on them too. Also it will call all other functions that reads
	 * in other stuff that can be found within an object. Check the
	 * tree in the beginning of this class' Xeromyces_* section.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the object-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param pParent	Parent to add this object as child in.
	 * @param NameSubst	A set of substitution strings that will be
	 * 					applied to all object names within this object.
	 * @param Paths		Output set of file paths that this GUI object
	 * 					relies on.
	 *
	 * @see LoadXmlFile()
	 */
	IGUIObject* Xeromyces_ReadObject(const XMBData& xmb, XMBElement element, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth);

	/**
	 * Reads in the element \<repeat\>, which repeats its child \<object\>s
	 * 'count' times, replacing the string "[n]" (or the value of the attribute
	 * 'var' enclosed in square brackets) in its descendants' names with "[0]",
	 * "[1]", etc.
	 */
	void Xeromyces_ReadRepeat(const XMBData& xmb, XMBElement element, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth);

	/**
	 * Reads in the element \<script\> (the XMBElement) and executes
	 * the script's code.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the script-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param Paths		Output set of file paths that this script is loaded from.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadScript(const XMBData& xmb, XMBElement element, std::unordered_set<VfsPath>& Paths);

	/**
	 * Reads in the element \<sprite\> (the XMBElement) and stores the
	 * result in a new CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprite-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadSprite(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the element \<image\> (the XMBElement) and stores the
	 * result within the CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the image-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param parent	Parent sprite.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadImage(const XMBData& xmb, XMBElement element, CGUISprite& parent);

	/**
	 * Reads in the element \<effect\> (the XMBElement) and stores the
	 * result within the SGUIImageEffects.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the image-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param effects	Effects object to add this effect to.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadEffects(const XMBData& xmb, XMBElement element, SGUIImageEffects& effects);

	/**
	 * Reads in the element \<style\> (the XMBElement) and stores the
	 * result in m_Styles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the style-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadStyle(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the element \<scrollbar\> (the XMBElement) and stores the
	 * result in m_ScrollBarStyles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadScrollBarStyle(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the element \<icon\> (the XMBElement) and stores the
	 * result in m_Icons.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadIcon(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the element \<tooltip\> (the XMBElement) and stores the
	 * result as an object with the name __tooltip_#.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadTooltip(const XMBData& xmb, XMBElement element);

	/**
	 * Reads in the element \<color\> (the XMBElement) and stores the
	 * result in m_PreDefinedColors
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadColor(const XMBData& xmb, XMBElement element);

	//@}

private:

	// Variables

	//--------------------------------------------------------
	/** @name Miscellaneous */
	//--------------------------------------------------------
	//@{

	std::shared_ptr<ScriptInterface> m_ScriptInterface;

	/**
	 * don't want to pass this around with the
	 * ChooseMouseOverAndClosest broadcast -
	 * we'd need to pack this and pNearest in a struct
	 */
	CVector2D m_MousePos;

	/**
	 * Indicates which buttons are pressed (bit 0 = LMB,
	 * bit 1 = RMB, bit 2 = MMB)
	 */
	unsigned int m_MouseButtons;

	// Tooltip
	GUITooltip m_Tooltip;

	//@}
	//--------------------------------------------------------
	/** @name Objects */
	//--------------------------------------------------------
	//@{

	/**
	 * Base Object, all its children are considered parentless
	 * because this is not a real object per se.
	 */
	std::unique_ptr<IGUIObject> m_BaseObject;

	/**
	 * Focused object!
	 * Say an input box that is selected. That one is focused.
	 * There can only be one focused object.
	 */
	IGUIObject* m_FocusedObject;

	/**
	 * Just pointers for fast name access, each object
	 * is really constructed within its parent for easy
	 * recursive management.
	 * Notice m_BaseObject won't belong here since it's
	 * not considered a real object.
	 */
	map_pObjects m_pAllObjects;

	/**
	 * Number of object that has been given name automatically.
	 * the name given will be '__internal(#)', the number (#)
	 * being this variable. When an object's name has been set
	 * as followed, the value will increment.
	 */
	int m_InternalNameNumber;

	/**
	 * Function pointers to functions that constructs
	 * IGUIObjects by name... For instance m_ObjectTypes["button"]
	 * is filled with a function that will "return new CButton();"
	 */
	std::map<CStr, ConstructObjectFunction> m_ObjectTypes;

	/**
	 * This is intended to store the JSFunction when accessing certain properties.
	 * The problem is that these functions are per-scriptInterface, and proxy handlers aren't.
	 * So we know what we want to store, but we don't really have anywhere to store it.
	 * It would be simpler to recreate the functions on every JS call, but that is slower
	 * (this may or may not matter now and in the future).
	 * It's not a great solution, but I can't find a better one at the moment.
	 * An alternative would be to store these on the proxy's prototype,
	 * but that embarks a lot of un-necessary code.
	 */
	std::unordered_map<const js::BaseProxyHandler*, std::unique_ptr<GUIProxyProps>> m_ProxyData;

	/**
	 * Map from hotkey names to objects that listen to the hotkey.
	 * (This is an optimisation to avoid recursing over the whole GUI
	 * tree every time a hotkey is pressed).
	 */
	std::map<CStr, std::vector<IGUIObject*> > m_HotkeyObjects;

	/**
	 * Map from hotkey names to maps of eventNames to functions that are triggered
	 * when the hotkey goes through the event. Contrary to object hotkeys, this
	 * allows for only one global function per hotkey name per event type.
	 */
	std::map<CStr, std::map<CStr, JS::PersistentRootedValue>> m_GlobalHotkeys;

	/**
	 * XML and JS can subscribe handlers to events identified by these names.
	 * Store in static const members to avoid string copies, gain compile errors when misspelling and
	 * to allow reuse in other classes.
	 */
	static const CStr EventNameLoad;
	static const CStr EventNameTick;
	static const CStr EventNamePress;
	static const CStr EventNameKeyDown;
	static const CStr EventNameRelease;
	static const CStr EventNameMouseRightPress;
	static const CStr EventNameMouseLeftPress;
	static const CStr EventNameMouseWheelDown;
	static const CStr EventNameMouseWheelUp;
	static const CStr EventNameMouseLeftDoubleClick;
	static const CStr EventNameMouseLeftRelease;
	static const CStr EventNameMouseRightDoubleClick;
	static const CStr EventNameMouseRightRelease;

	//--------------------------------------------------------
	//	Databases
	//	These are loaded from XML files and marked as noncopyable and const to
	//	rule out unintentional modification and copy, especially during Draw calls.
	//--------------------------------------------------------

	// Colors
	std::map<CStr, const CGUIColor> m_PreDefinedColors;

	// Sprites
	std::map<CStr, std::unique_ptr<const CGUISprite>> m_Sprites;

	// Styles
	std::map<CStr, const SGUIStyle> m_Styles;

	// Scroll-bar styles
	std::map<CStr, const SGUIScrollBarStyle> m_ScrollBarStyles;

	// Icons
	std::map<CStr, const SGUIIcon> m_Icons;

public:
	/**
	 * Map from event names to object which listen to a given event.
	 */
	std::unordered_map<CStr, std::vector<IGUIObject*>> m_EventObjects;
};

#endif // INCLUDED_CGUI
