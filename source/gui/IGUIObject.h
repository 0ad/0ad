/* Copyright (C) 2019 Wildfire Games.
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
 * The base class of an object.
 * All objects are derived from this class.
 * It's an abstract data type, so it can't be used per se.
 * Also contains a Dummy object which is used for completely blank objects.
 */

#ifndef INCLUDED_IGUIOBJECT
#define INCLUDED_IGUIOBJECT

#include "GUIbase.h"
#include "GUItext.h"

#include "gui/scripting/JSInterface_IGUIObject.h"
#include "lib/input.h" // just for IN_PASS
#include "ps/XML/Xeromyces.h"

#include <string>
#include <vector>

struct SGUISetting;
struct SGUIStyle;
class CGUI;

class JSObject;

ERROR_TYPE(GUI, UnableToParse);

/**
 * Setting Type
 * @see SGUISetting
 *
 * For use of later macros, all names should be GUIST_ followed
 * by the code name (case sensitive!).
 */
#define TYPE(T) GUIST_##T,
enum EGUISettingType
{
	#include "GUItypes.h"
};
#undef TYPE

/**
 * A GUI Setting is anything that can be inputted from XML as
 * \<object\>-attributes (with exceptions). For instance:
 * \<object style="null"\>
 *
 * "style" will be a SGUISetting.
 */
struct SGUISetting
{
	SGUISetting() : m_pSetting(NULL) {}

	void				*m_pSetting;
	EGUISettingType		m_Type;
};

/**
 * GUI object such as a button or an input-box.
 * Abstract data type !
 */
class IGUIObject
{
	friend class CGUI;
	friend class CInternalCGUIAccessorBase;
	friend class IGUIScrollBar;
	friend class GUITooltip;

	// Allow getProperty to access things like GetParent()
	friend bool JSI_IGUIObject::getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp);
	friend bool JSI_IGUIObject::setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool UNUSED(strict), JS::MutableHandleValue vp);
	friend bool JSI_IGUIObject::getComputedSize(JSContext* cx, uint argc, JS::Value* vp);
	friend bool JSI_IGUIObject::getTextSize(JSContext* cx, uint argc, JS::Value* vp);

public:
	IGUIObject();
	virtual ~IGUIObject();

	/**
	 * Checks if mouse is hovering this object.
     * The mouse position is cached in CGUI.
	 *
	 * This function checks if the mouse is hovering the
	 * rectangle that the base setting "size" makes.
	 * Although it is virtual, so one could derive
	 * an object from CButton, which changes only this
	 * to checking the circle that "size" makes.
	 *
	 * @return true if mouse is hovering
	 */
	virtual bool MouseOver();

	/**
	 * Test if mouse position is over an icon
	 */
	virtual bool MouseOverIcon();

	//--------------------------------------------------------
	/** @name Leaf Functions */
	//--------------------------------------------------------
	//@{

	/// Get object name, name is unique
	const CStr& GetName() const { return m_Name; }

	/// Get object name
	void SetName(const CStr& Name) { m_Name = Name; }

	// Get Presentable name.
	//  Will change all internally set names to something like "<unnamed object>"
	CStr GetPresentableName() const;

	/**
	 * Adds object and its children to the map, it's name being the
	 * first part, and the second being itself.
	 *
	 * @param ObjectMap Adds this to the map_pObjects.
	 *
	 * @throws PSERROR_GUI_ObjectNeedsName Name is missing
	 * @throws PSERROR_GUI_NameAmbiguity Name is already taken
	 */
	void AddToPointersMap(map_pObjects& ObjectMap);

	/**
	 * Notice nothing will be returned or thrown if the child hasn't
	 * been inputted into the GUI yet. This is because that's were
	 * all is checked. Now we're just linking two objects, but
	 * it's when we're inputting them into the GUI we'll check
	 * validity! Notice also when adding it to the GUI this function
	 * will inevitably have been called by CGUI::AddObject which
	 * will catch the throw and return the error code.
	 * i.e. The user will never put in the situation wherein a throw
	 * must be caught, the GUI's internal error handling will be
	 * completely transparent to the interfacially sequential model.
	 *
	 * @param pChild Child to add
	 *
	 * @throws PSERROR_GUI from CGUI::UpdateObjects().
	 */
	void AddChild(IGUIObject* pChild);

	//@}
	//--------------------------------------------------------
	/** @name Iterate
	 * Used to iterate over all children of this object.
	 */
	//--------------------------------------------------------
	//@{

	vector_pObjects::iterator begin() { return m_Children.begin(); }
	vector_pObjects::iterator end() { return m_Children.end(); }

	//@}
	//--------------------------------------------------------
	/** @name Settings Management */
	//--------------------------------------------------------
	//@{

	/**
	 * Checks if settings exists, only available for derived
	 * classes that has this set up, that's why the base
	 * class just returns false
	 *
	 * @param Setting setting name
	 * @return True if settings exist.
	 */
	bool SettingExists(const CStr& Setting) const;

	/**
	 * All sizes are relative to resolution, and the calculation
	 * is not wanted in real time, therefore it is cached, update
	 * the cached size with this function.
	 */
	virtual void UpdateCachedSize();

	/**
	 * Set a setting by string, regardless of what type it is.
	 *
	 * example a CRect(10,10,20,20) would be "10 10 20 20"
	 *
	 * @param Setting Setting by name
	 * @param Value Value to set to
	 * @param SkipMessage Does not send a GUIM_SETTINGS_UPDATED if true
	 *
	 * @return PSRETURN (PSRETURN_OK if successful)
	 */
	PSRETURN SetSetting(const CStr& Setting, const CStrW& Value, const bool& SkipMessage = false);

	/**
	 * Retrieves the type of a named setting.
	 *
	 * @param Setting Setting by name
	 * @param Type Stores an EGUISettingType
	 * @return PSRETURN (PSRETURN_OK if successful)
	 */
	PSRETURN GetSettingType(const CStr& Setting, EGUISettingType& Type) const;

	/**
	 * Set the script handler for a particular object-specific action
	 *
	 * @param Action Name of action
	 * @param Code Javascript code to execute when the action occurs
	 * @param pGUI GUI instance to associate the script with
	 */
	void RegisterScriptHandler(const CStr& Action, const CStr& Code, CGUI* pGUI);

	/**
	 * Retrieves the JSObject representing this GUI object.
	 */
	JSObject* GetJSObject();

	//@}
protected:
	//--------------------------------------------------------
	/** @name Called by CGUI and friends
	 *
	 * Methods that the CGUI will call using
	 * its friendship, these should not
	 * be called by user.
	 * These functions' security are a lot
	 * what constitutes the GUI's
	 */
	//--------------------------------------------------------
	//@{

	/**
	 * Add a setting to m_Settings
	 *
	 * @param Type Setting type
	 * @param Name Setting reference name
	 */
	void AddSetting(const EGUISettingType& Type, const CStr& Name);

	/**
	 * Calls Destroy on all children, and deallocates all memory.
	 * MEGA TODO Should it destroy it's children?
	 */
	virtual void Destroy();

public:
	/**
     * This function is called with different messages
	 * for instance when the mouse enters the object.
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(SGUIMessage& UNUSED(Message)) {}

protected:
	/**
	 * Draws the object.
	 *
	 * @throws	PSERROR if any. But this will mostlikely be
	 *			very rare since if an object is drawn unsuccessfully
	 *			it'll probably only output in the Error log, and not
	 *			disrupt the whole GUI drawing.
	 */
	virtual void Draw() = 0;

	/**
	 * Some objects need to handle the SDL_Event_ manually.
	 * For instance the input box.
	 *
	 * Only the object with focus will have this function called.
	 *
	 * Returns either IN_PASS or IN_HANDLED. If IN_HANDLED, then
	 * the key won't be passed on and processed by other handlers.
	 * This is used for keys that the GUI uses.
	 */
	virtual InReaction ManuallyHandleEvent(const SDL_Event_* UNUSED(ev)) { return IN_PASS; }

	/**
	 * Loads a style.
	 *
	 * @param GUIinstance Reference to the GUI
	 * @param StyleName Style by name
	 */
	void LoadStyle(CGUI& GUIinstance, const CStr& StyleName);

	/**
	 * Loads a style.
	 *
	 * @param Style The style object.
	 */
	void LoadStyle(const SGUIStyle& Style);

	/**
	 * Returns not the Z value, but the actual buffered Z value, i.e. if it's
	 * defined relative, then it will check its parent's Z value and add
	 * the relativity.
	 *
	 * @return Actual Z value on the screen.
	 */
	virtual float GetBufferedZ() const;

	void SetGUI(CGUI* const& pGUI);

	/**
	 * Set parent of this object
	 */
	void SetParent(IGUIObject* pParent) { m_pParent = pParent; }

	/**
	 * Reset internal state of this object
	 */
	virtual void ResetStates()
	{
		// Notify the gui that we aren't hovered anymore
		UpdateMouseOver(NULL);
	}

public:
	CGUI* GetGUI() { return m_pGUI; }
	const CGUI* GetGUI() const { return m_pGUI; }

	/**
	 * Take focus!
	 */
	void SetFocus();

protected:
	/**
	 * Check if object is focused.
	 */
	bool IsFocused() const;

	/**
	 * <b>NOTE!</b> This will not just return m_pParent, when that is
	 * need use it! There is one exception to it, when the parent is
	 * the top-node (the object that isn't a real object), this
	 * will return NULL, so that the top-node's children are
	 * seemingly parentless.
	 *
	 * @return Pointer to parent
	 */
	IGUIObject* GetParent() const;

	/**
	 * Get Mouse from CGUI.
	 */
	CPos GetMousePos() const;

	/**
	 * Handle additional children to the \<object\>-tag. In IGUIObject, this function does
	 * nothing. In CList and CDropDown, it handles the \<item\>, used to build the data.
	 *
	 * Returning false means the object doesn't recognize the child. Should be reported.
	 * Notice 'false' is default, because an object not using this function, should not
	 * have any additional children (and this function should never be called).
	 */
	virtual bool HandleAdditionalChildren(const XMBElement& UNUSED(child), CXeromyces* UNUSED(pFile))
	{
		return false;
	}

	/**
	 * Cached size, real size m_Size is actually dependent on resolution
	 * and can have different *real* outcomes, this is the real outcome
	 * cached to avoid slow calculations in real time.
	 */
	CRect m_CachedActualSize;

	/**
	 * Send event to this GUI object (HandleMessage and ScriptEvent)
	 *
	 * @param type Type of GUI message to be handled
	 * @param EventName String representation of event name
	 * @return IN_HANDLED if event was handled, or IN_PASS if skipped
	 */
	InReaction SendEvent(EGUIMessageType type, const CStr& EventName);

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 * The mouse coordinates will be passed as the first argument.
	 *
	 * @param Action Name of action
	 */
	void ScriptEvent(const CStr& Action);

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 *
	 * @param Action Name of action
	 * @param Argument Argument to pass to action
	 */
	void ScriptEvent(const CStr& Action, JS::HandleValue Argument);

	void SetScriptHandler(const CStr& Action, JS::HandleObject Function);

	/**
	 * Inputes the object that is currently hovered, this function
	 * updates this object accordingly (i.e. if it's the object
	 * being inputted one thing happens, and not, another).
	 *
	 * @param pMouseOver	Object that is currently hovered,
	 *						can OF COURSE be NULL too!
	 */
	void UpdateMouseOver(IGUIObject* const& pMouseOver);

	//@}
private:
	//--------------------------------------------------------
	/** @name Internal functions */
	//--------------------------------------------------------
	//@{

	/**
	 * Inputs a reference pointer, checks if the new inputted object
	 * if hovered, if so, then check if this's Z value is greater
	 * than the inputted object... If so then the object is closer
	 * and we'll replace the pointer with this.
	 * Also Notice input can be NULL, which means the Z value demand
	 *  is out. NOTICE you can't input NULL as const so you'll have
	 * to set an object to NULL.
	 *
	 * @param pObject	Object pointer, can be either the old one, or
	 *					the new one.
	 */
	void ChooseMouseOverAndClosest(IGUIObject*& pObject);

	// Is the object a Root object, in philosophy, this means it
	//  has got no parent, and technically, it's got the m_BaseObject
	//  as parent.
	bool IsRootObject() const;

	/**
	 * Logs an invalid setting search and returns the correct return result
	 *
	 * @return the error result
	 */
	PSRETURN LogInvalidSettings(const CStr8& Setting) const;

	static void Trace(JSTracer* trc, void* data)
	{
		reinterpret_cast<IGUIObject*>(data)->TraceMember(trc);
	}

	void TraceMember(JSTracer* trc);

	// Variables

protected:
	// Name of object
	CStr									m_Name;

	// Constructed on the heap, will be destroyed along with the the object
	// TODO Gee: really the above?
	vector_pObjects							m_Children;

	// Pointer to parent
	IGUIObject								*m_pParent;

	//This represents the last click time for each mouse button
	double m_LastClickTime[6];

	/**
	 * This is an array of true or false, each element is associated with
	 * a string representing a setting. Number of elements is equal to
	 * number of settings.
	 *
	 * A true means the setting has been manually set in the file when
	 * read. This is important to know because I don't want to force
	 * the user to include its \<styles\>-XML-files first, so somehow
	 * the GUI needs to know which settings were set, and which is meant
	 * to.
	 */

	// More variables

	// Is mouse hovering the object? used with the function MouseOver()
	bool									m_MouseHovering;

	/**
	 * Settings pool, all an object's settings are located here
	 * If a derived object has got more settings that the base
	 * settings, it's because they have a new version of the
	 * function SetupSettings().
	 *
	 * @see SetupSettings()
	 */
	public:
	std::map<CStr, SGUISetting>				m_Settings;

private:
	// An object can't function stand alone
	CGUI									*m_pGUI;

	// Internal storage for registered script handlers.
	std::map<CStr, JS::Heap<JSObject*> >	m_ScriptHandlers;

	// Cached JSObject representing this GUI object
	JS::PersistentRootedObject				m_JSObject;
};


/**
 * Dummy object used primarily for the root object
 * or objects of type 'empty'
 */
class CGUIDummyObject : public IGUIObject
{
	GUI_OBJECT(CGUIDummyObject)

public:

	virtual void Draw() {}
	// Empty can never be hovered. It is only a category.
	virtual bool MouseOver() { return false; }
};

#endif // INCLUDED_IGUIOBJECT
