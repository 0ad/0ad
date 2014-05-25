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

/*
GUI util

--Overview--

	Contains help class GUI<>, which gives us templated
	 parameter to all functions within GUI.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_GUIUTIL
#define INCLUDED_GUIUTIL


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUIbase.h"
// TODO Gee: New
#include "ps/Overlay.h"
#include "CGUI.h"
#include "CGUISprite.h"
#include "IGUIObject.h"

//--------------------------------------------------------
//  Help Classes/Structs for the GUI
//--------------------------------------------------------

class CClientArea;
class CGUIString;
class CMatrix3D;

template <typename T>
bool __ParseString(const CStrW& Value, T &tOutput);

// Model-view-projection matrix with (0,0) in top-left of screen
CMatrix3D GetDefaultGuiMatrix();

//--------------------------------------------------------
//  Forward declarations
//--------------------------------------------------------
struct SGUIMessage;

/**
 * Base class to only the class GUI. This superclass is 
 * kind of a templateless extention of the class GUI.
 * Used for other functions to friend with, because it
 * can't friend with GUI since it's templated (at least
 * not on all compilers we're using).
 */
class CInternalCGUIAccessorBase
{
protected:
	/// Get object pointer
	static IGUIObject * GetObjectPointer(CGUI &GUIinstance, const CStr& Object);
	
	/// const version
	static const IGUIObject * GetObjectPointer(const CGUI &GUIinstance, const CStr& Object);

	/// Wrapper for ResetStates
	static void QueryResetting(IGUIObject *pObject);

	static void HandleMessage(IGUIObject *pObject, SGUIMessage &message);
};


#ifndef NDEBUG
// Used to ensure type-safety, sort of 
template<typename T> void CheckType(const IGUIObject* obj, const CStr& setting);
#endif


/**
 * Includes static functions that needs one template
 * argument.
 *
 * int is only to please functions that doesn't even use T
 * and are only within this class because it's convenient
 */
template <typename T=int>
class GUI : public CInternalCGUIAccessorBase
{
	// Private functions further ahead
	friend class CGUI;
	friend class IGUIObject;
	friend class CInternalCGUIAccessorBase;

public:

	// Like GetSetting (below), but doesn't make a copy of the value
	// (so it can be modified later)
	static PSRETURN GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value);

	/**
	 * Retrieves a setting by name from object pointer
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PSRETURN GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value);

	/**
	 * Sets a value by name using a real datatype as input.
	 *
	 * This is the official way of setting a setting, no other
	 *  way should only cautiously be used!
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 * @param SkipMessage Does not send a GUIM_SETTINGS_UPDATED if true
	 */
	static PSRETURN SetSetting(IGUIObject *pObject, const CStr& Setting, 
								const T &Value, const bool &SkipMessage=false);

	/**
	 * Retrieves a setting by settings name and object name
	 *
	 * @param GUIinstance GUI Object const ref
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PSRETURN GetSetting(
		const CGUI &GUIinstance, const CStr& Object, 
		const CStr& Setting, T &Value)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PSRETURN_GUI_NullObjectProvided;

		// Retrieve pointer and call sibling function
		const IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		return GetSetting(pObject, Setting, Value);
	}

	/**
	 * Sets a value by setting and object name using a real 
	 * datatype as input
	 *
	 * This is just a wrapper so that we can type the object name
	 *  and not input the actual pointer.
	 *
	 * @param GUIinstance GUI Object, reference since we'll be changing values
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 * @param SkipMessage Does not send a GUIM_SETTINGS_UPDATED if true
	 */
	static PSRETURN SetSetting(
		CGUI &GUIinstance, const CStr& Object, 
		const CStr& Setting, const T &Value,
		const bool& SkipMessage=false)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PSRETURN_GUI_NullObjectProvided;

		// Retrieve pointer and call sibling function

		// Important, we don't want to use this T, we want
		//  to use the standard T, since that will be the
		//  one with the friend relationship
		IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		return SetSetting(pObject, Setting, Value, SkipMessage);
	}
	
	/**
	 * This will return the value of the first sprite if it's not null,
	 * if it is null, it will return the value of the second sprite, if
	 * that one is null, then null it is.
	 *
	 * @param prim Primary sprite that should be used
	 * @param sec Secondary sprite if Primary should fail
	 * @return Resulting string
	 */
	static const CGUISpriteInstance& FallBackSprite(
									const CGUISpriteInstance& prim,
									const CGUISpriteInstance& sec)
	{
		return (prim.IsEmpty() ? sec : prim);
	}

	/**
	 * Same principle as FallBackSprite
	 *
	 * @param prim Primary color that should be used
	 * @param sec Secondary color if Primary should fail
	 * @return Resulting color
	 * @see FallBackSprite
	 */
	static CColor FallBackColor(const CColor &prim, const CColor &sec)
	{
		// CColor() == null.
		return ((prim!=CColor())?(prim):(sec));
	}

	/**
	 * Sets a value by setting and object name using a real 
	 * datatype as input.
	 *
	 * This is just a wrapper for __ParseString() which really
	 * works the magic.
	 *
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 *
	 * @see __ParseString()
	 */
	static bool ParseString(const CStrW& Value, T &tOutput)
	{
		return __ParseString<T>(Value, tOutput);
	}

	static bool ParseColor(const CStrW& Value, CColor &tOutput, float DefaultAlpha);

private:

	// templated typedef of function pointer
	typedef void (IGUIObject::*void_Object_pFunction_argT)(const T &arg);
	typedef void (IGUIObject::*void_Object_pFunction_argRefT)(T &arg);
	typedef void (IGUIObject::*void_Object_pFunction)();

	/**
	 * If you want to call a IGUIObject-function
	 * on not just an object, but also on ALL of their children
	 * you want to use this recursion system.
	 * It recurses an object calling a function on itself
	 * and all children (and so forth).
	 *
	 * <b>Restrictions:</b>\n
	 * You can also set restrictions, so that if the recursion
	 * reaches an objects with certain setup, it just doesn't
	 * call the function on the object, nor it's children for
	 * that matter. i.e. it cuts that object off from the
	 * recursion tree. What setups that can cause restrictions
	 * are hardcoded and specific. Check out the defines
	 * GUIRR_* for all different setups.
	 *
	 * Error reports are either logged or thrown out of RecurseObject.
	 * Always use it with try/catch!
	 *
	 * @param RR Recurse Restrictions, set to 0 if no restrictions
	 * @param pObject Top object, this is where the iteration starts
	 * @param pFunc Function to recurse
	 * @param Argument Argument for pFunc of type T
	 * @throws PSERROR Depends on what pFunc might throw. PSERROR is standard.
	 *			Itself doesn't throw anything.
	*/
	static void RecurseObject(int RR, IGUIObject *pObject, void_Object_pFunction_argT pFunc, const T &Argument)
	{
		// TODO Gee: Don't run this for the base object.
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)(Argument);
		
		// Iterate children
		vector_pObjects::iterator it;
		for (it = pObject->ChildrenItBegin(); it != pObject->ChildrenItEnd(); ++it)
		{
			RecurseObject(RR, *it, pFunc, Argument);
		}
	}

	/**
	 * Argument is reference.
	 *
	 * @see RecurseObject()
	 */
	static void RecurseObject(int RR, IGUIObject *pObject, void_Object_pFunction_argRefT pFunc, T &Argument)
	{
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)(Argument);
		
		// Iterate children
		vector_pObjects::iterator it;
		for (it = pObject->ChildrenItBegin(); it != pObject->ChildrenItEnd(); ++it)
		{
			RecurseObject(RR, *it, pFunc, Argument);
		}
	}

	/**
	 * With no argument.
	 *
	 * @see RecurseObject()
	 */
	static void RecurseObject(int RR, IGUIObject *pObject, void_Object_pFunction pFunc)
	{
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)();

		// Iterate children
		vector_pObjects::iterator it;
		for (it = pObject->ChildrenItBegin(); it != pObject->ChildrenItEnd(); ++it)
		{
			RecurseObject(RR, *it, pFunc);
		}
	}

private:
	/**
	 * Checks restrictions for the iteration, for instance if
	 * you tell the recursor to avoid all hidden objects, it
	 * will, and this function checks a certain object's
	 * restriction values.
	 *
	 * @param RR What kind of restriction, for instance hidden or disabled
	 * @param pObject Object
	 * @return true if restricted
	 */
	static bool CheckIfRestricted(int RR, IGUIObject *pObject)
	{
		// Statically initialise some strings, so we don't have to do
		// lots of allocation every time this function is called
		static CStr strHidden("hidden");
		static CStr strEnabled("enabled");
		static CStr strGhost("ghost");

		if (RR & GUIRR_HIDDEN)
		{
			bool hidden = true;
			GUI<bool>::GetSetting(pObject, strHidden, hidden);

			if (hidden)
				return true;
		}
		if (RR & GUIRR_DISABLED)
		{
			bool enabled = false;
			GUI<bool>::GetSetting(pObject, strEnabled, enabled);

			if (!enabled)
				return true;
		}
		if (RR & GUIRR_GHOST)
		{
			bool ghost = true;
			GUI<bool>::GetSetting(pObject, strGhost, ghost);

			if (ghost)
				return true;
		}

		// false means not restricted
		return false;
	}
};

#endif
