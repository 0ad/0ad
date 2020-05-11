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

#include "gui/Scripting/JSInterface_IGUIObject.h"
#include "gui/SettingTypes/CGUISize.h"
#include "gui/SGUIMessage.h"
#include "lib/input.h" // just for IN_PASS
#include "ps/XML/Xeromyces.h"

#include <map>
#include <string>
#include <vector>

class CGUI;
class IGUIObject;
class IGUISetting;

using map_pObjects = std::map<CStr, IGUIObject*>;

#define GUI_OBJECT(obj) \
public: \
	static IGUIObject* ConstructObject(CGUI& pGUI) \
		{ return new obj(pGUI); }

/**
 * GUI object such as a button or an input-box.
 * Abstract data type !
 */
class IGUIObject
{
	friend class CGUI;

	// Allow getProperty to access things like GetParent()
	friend bool JSI_IGUIObject::getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp);
	friend bool JSI_IGUIObject::setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp, JS::ObjectOpResult& result);
	friend bool JSI_IGUIObject::deleteProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result);
	friend bool JSI_IGUIObject::getComputedSize(JSContext* cx, uint argc, JS::Value* vp);

public:
	NONCOPYABLE(IGUIObject);

	IGUIObject(CGUI& pGUI);
	virtual ~IGUIObject();

	/**
	 * This function checks if the mouse is hovering the
	 * rectangle that the base setting "size" makes.
	 * Although it is virtual, so one could derive
	 * an object from CButton, which changes only this
	 * to checking the circle that "size" makes.
	 *
	 * This function also returns true if there is a different
	 * GUI object shown on top of this one.
	 */
	virtual bool IsMouseOver() const;

	/**
	 * This function returns true if the mouse is hovering
	 * over this GUI object and if this GUI object is the
	 * topmost object in that screen location.
	 * For example when hovering dropdown list items, the
	 * buttons beneath the list won't return true here.
	 */
	virtual bool IsMouseHovering() const { return m_MouseHovering; }

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
	 * Builds the object hierarchy with references.
	 */
	void AddChild(IGUIObject& pChild);

	/**
	 * Return all child objects of the current object.
	 */
	const std::vector<IGUIObject*>& GetChildren() const { return m_Children; }

	//@}
	//--------------------------------------------------------
	/** @name Settings Management */
	//--------------------------------------------------------
	//@{

	/**
	 * Registers the given setting variables with the GUI object.
	 * Enable XML and JS to modify the given variable.
	 *
	 * @param Type Setting type
	 * @param Name Setting reference name
	 */
	template<typename T>
	void RegisterSetting(const CStr& Name, T& Value);

	/**
	 * Returns whether there is a setting with the given name registered.
	 *
	 * @param Setting setting name
	 * @return True if settings exist.
	 */
	bool SettingExists(const CStr& Setting) const;

	/**
	 * Get a mutable reference to the setting.
	 * If no such setting exists, an exception of type std::out_of_range is thrown.
	 * If the value is modified, there is no GUIM_SETTINGS_UPDATED message sent.
	 */
	template <typename T>
	T& GetSetting(const CStr& Setting);

	template <typename T>
	const T& GetSetting(const CStr& Setting) const;

	/**
	 * Set a setting by string, regardless of what type it is.
	 * Used to parse setting values from XML files.
	 * For example a CRect(10,10,20,20) is created from "10 10 20 20".
	 * Returns false if the conversion fails, otherwise true.
	 */
	bool SetSettingFromString(const CStr& Setting, const CStrW& Value, const bool SendMessage);

	/**
	 * Assigns the given value to the setting identified by the given name.
	 * Uses move semantics, so do not read from Value after this call.
	 *
	 * @param SendMessage If true, a GUIM_SETTINGS_UPDATED message will be broadcasted to all GUI objects.
	 */
	template <typename T>
	void SetSetting(const CStr& Setting, T& Value, const bool SendMessage);

	/**
	 * This variant will copy the value.
	 */
	template <typename T>
	void SetSetting(const CStr& Setting, const T& Value, const bool SendMessage);

	/**
	 * Returns whether this object is set to be hidden or ghost.
	 */
	bool IsEnabled() const;

	/**
	 * Returns whether this is object is set to be hidden.
	 */
	bool IsHidden() const;

	/**
	 * Returns whether this object is set to be hidden or ghost.
	 */
	bool IsHiddenOrGhost() const;

	/**
	 * Retrieves the configured sound filename from the given setting name and plays that once.
	 */
	void PlaySound(const CStrW& soundPath) const;

	/**
	 * Send event to this GUI object (HandleMessage and ScriptEvent)
	 *
	 * @param type Type of GUI message to be handled
	 * @param eventName String representation of event name
	 * @return IN_HANDLED if event was handled, or IN_PASS if skipped
	 */
	InReaction SendEvent(EGUIMessageType type, const CStr& eventName);

	/**
	 * Same as SendEvent, but passes mouse coordinates and button state as an argument.
	 */
	InReaction SendMouseEvent(EGUIMessageType type, const CStr& eventName);

	/**
	 * All sizes are relative to resolution, and the calculation
	 * is not wanted in real time, therefore it is cached, update
	 * the cached size with this function.
	 */
	virtual void UpdateCachedSize();

	/**
	 * Reset internal state of this object.
	 */
	virtual void ResetStates();

	/**
	 * Set the script handler for a particular object-specific action
	 *
	 * @param eventName Name of action
	 * @param Code Javascript code to execute when the action occurs
	 * @param pGUI GUI instance to associate the script with
	 */
	void RegisterScriptHandler(const CStr& eventName, const CStr& Code, CGUI& pGUI);

	/**
	 * Inheriting classes may append JS functions to the JS object representing this class.
	 */
	virtual void RegisterScriptFunctions() {}

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

public:
	/**
     * This function is called with different messages
	 * for instance when the mouse enters the object.
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(SGUIMessage& UNUSED(Message)) {}

	/**
	 * Calls an IGUIObject member function recursively on this object and its children.
	 * Aborts recursion at IGUIObjects that have the isRestricted function return true.
	 * The arguments of the callback function must be references.
	*/
	template<typename... Args>
	void RecurseObject(bool(IGUIObject::*isRestricted)() const, void(IGUIObject::*callbackFunction)(Args... args), Args&&... args)
	{
		if (!IsBaseObject())
		{
			if (isRestricted && (this->*isRestricted)())
				return;

			(this->*callbackFunction)(args...);
		}

		for (IGUIObject* const& obj : m_Children)
			obj->RecurseObject(isRestricted, callbackFunction, args...);
	}

protected:
	/**
	 * Draws the object.
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
	 */
	void LoadStyle(const CStr& StyleName);

	/**
	 * Returns not the Z value, but the actual buffered Z value, i.e. if it's
	 * defined relative, then it will check its parent's Z value and add
	 * the relativity.
	 *
	 * @return Actual Z value on the screen.
	 */
	virtual float GetBufferedZ() const;

	/**
	 * Set parent of this object
	 */
	void SetParent(IGUIObject* pParent) { m_pParent = pParent; }

public:

	CGUI& GetGUI() { return m_pGUI; }
	const CGUI& GetGUI() const { return m_pGUI; }

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
	 * will return nullptr, so that the top-node's children are
	 * seemingly parentless.
	 *
	 * @return Pointer to parent
	 */
	IGUIObject* GetParent() const;

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
	 * Allow the GUI object to process after all child items were handled.
	 * Useful to avoid iterator invalidation with push_back calls.
	 */
	virtual void AdditionalChildrenHandled() {}

	/**
	 * Cached size, real size m_Size is actually dependent on resolution
	 * and can have different *real* outcomes, this is the real outcome
	 * cached to avoid slow calculations in real time.
	 */
	CRect m_CachedActualSize;

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 * The mouse coordinates will be passed as the first argument.
	 *
	 * @param eventName Name of action
	 */
	void ScriptEvent(const CStr& eventName);

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 * The mouse coordinates will be passed as the first argument.
	 *
	 * @param eventName Name of action
	 *
	 * @return True if the script returned something truthy.
	 */
	bool ScriptEventWithReturn(const CStr& eventName);

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 *
	 * @param eventName Name of action
	 * @param paramData JS::HandleValueArray arguments to pass to the event.
	 */
	void ScriptEvent(const CStr& eventName, const JS::HandleValueArray& paramData);

	/**
	 * Execute the script for a particular action.
	 * Does nothing if no script has been registered for that action.
	 *
	 * @param eventName Name of action
	 * @param paramData JS::HandleValueArray arguments to pass to the event.
	 *
	 * @return True if the script returned something truthy.
	 */
	bool ScriptEventWithReturn(const CStr& eventName, const JS::HandleValueArray& paramData);

	/**
	 * Assigns a JS function to the event name.
	 */
	void SetScriptHandler(const CStr& eventName, JS::HandleObject Function);

	/**
	 * Deletes an event handler assigned to the given name, if such a handler exists.
	 */
	void UnsetScriptHandler(const CStr& eventName);

	/**
	 * Inputes the object that is currently hovered, this function
	 * updates this object accordingly (i.e. if it's the object
	 * being inputted one thing happens, and not, another).
	 *
	 * @param pMouseOver Object that is currently hovered, can be nullptr too!
	 */
	void UpdateMouseOver(IGUIObject* const& pMouseOver);

	//@}
private:
	//--------------------------------------------------------
	/** @name Internal functions */
	//--------------------------------------------------------
	//@{

	/**
	 * Creates the JS object representing this page upon first use.
	 */
	void CreateJSObject();

	/**
	 * Updates some internal data depending on the setting changed.
	 */
	void PreSettingChange(const CStr& Setting);
	void SettingChanged(const CStr& Setting, const bool SendMessage);

	/**
	 * Inputs a reference pointer, checks if the new inputted object
	 * if hovered, if so, then check if this's Z value is greater
	 * than the inputted object... If so then the object is closer
	 * and we'll replace the pointer with this.
	 * Also Notice input can be nullptr, which means the Z value demand
	 *  is out. NOTICE you can't input nullptr as const so you'll have
	 * to set an object to nullptr.
	 *
	 * @param pObject	Object pointer, can be either the old one, or
	 *					the new one.
	 */
	void ChooseMouseOverAndClosest(IGUIObject*& pObject);

	/**
	 * Returns whether this is the object all other objects are descendants of.
	 */
	bool IsBaseObject() const;

	/**
	 * Returns whether this object is a child of the base object.
	 */
	bool IsRootObject() const;

	static void Trace(JSTracer* trc, void* data)
	{
		reinterpret_cast<IGUIObject*>(data)->TraceMember(trc);
	}

	void TraceMember(JSTracer* trc);

// Variables
protected:
	static const CStr EventNameMouseEnter;
	static const CStr EventNameMouseMove;
	static const CStr EventNameMouseLeave;

	// Name of object
	CStr m_Name;

	// Constructed on the heap, will be destroyed along with the the CGUI
	std::vector<IGUIObject*> m_Children;

	// Pointer to parent
	IGUIObject* m_pParent;

	//This represents the last click time for each mouse button
	double m_LastClickTime[6];

	/**
	 * This variable is true if the mouse is hovering this object and
	 * this object is the topmost object shown in this location.
	 */
	bool m_MouseHovering;

	/**
	 * Settings pool, all an object's settings are located here
	 */
	std::map<CStr, IGUISetting*> m_Settings;

	// An object can't function stand alone
	CGUI& m_pGUI;

	// Internal storage for registered script handlers.
	std::map<CStr, JS::Heap<JSObject*> > m_ScriptHandlers;

	// Cached JSObject representing this GUI object
	JS::PersistentRootedObject m_JSObject;

	// Cache references to settings for performance
	bool m_Enabled;
	bool m_Hidden;
	CGUISize m_Size;
	CStr m_Style;
	CStr m_Hotkey;
	float m_Z;
	bool m_Absolute;
	bool m_Ghost;
	float m_AspectRatio;
	CStrW m_Tooltip;
	CStr m_TooltipStyle;
};

#endif // INCLUDED_IGUIOBJECT
