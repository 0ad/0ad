/*
GUI util
by Gustav Larsson
gee@pyro.nu

--Overview--

	Contains help class GUI<>, which gives us templated
	 parameter to all functions within GUI.

--More info--

	Check GUI.h

*/

#ifndef GUIutil_H
#define GUIutil_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "Parser.h"
// TODO Gee: New
#include "Overlay.h"

//--------------------------------------------------------
//  Help Classes/Structs for the GUI
//--------------------------------------------------------

class CClientArea;
class CGUIString;

template <typename T>
bool __ParseString(const CStr& Value, T &tOutput);

template <>
bool __ParseString<bool>(const CStr& Value, bool &Output);

template <>
bool __ParseString<int>(const CStr& Value, int &Output);

template <>
bool __ParseString<float>(const CStr& Value, float &Output);

template <>
bool __ParseString<CRect>(const CStr& Value, CRect &Output);

template <>
bool __ParseString<CClientArea>(const CStr& Value, CClientArea &Output);

template <>
bool __ParseString<CColor>(const CStr& Value, CColor &Output);

template <>
bool __ParseString<CSize>(const CStr& Value, CSize &Output);

template <>
bool __ParseString<EAlign>(const CStr& Value, EAlign &Output);

template <>
bool __ParseString<EVAlign>(const CStr& Value, EVAlign &Output);

template <>
bool __ParseString<CGUIString>(const CStr& Value, CGUIString &Output);


// Icon, you create them in the XML file with root element <setup>
//  you use them in text owned by different objects... Such as CText.
struct SGUIIcon
{
	// Texture name of icon
	CStr m_TextureName;

	// Size
	CSize m_Size;
};


/**
 * @author Gustav Larsson
 *
 * Client Area is a rectangle relative to a parent rectangle
 *
 * You can input the whole value of the Client Area by
 * string. Like used in the GUI.
 */
class CClientArea
{
public:
	CClientArea();
	CClientArea(const CStr& Value);

	/// Pixel modifiers
	CRect pixel;

	/// Percent modifiers
	CRect percent;

	/**
	 * Get client area rectangle when the parent is given
	 */
	CRect GetClientArea(const CRect &parent) const;

	/**
	 * The ClientArea can be set from a string looking like:
	 *
	 * "0 0 100% 100%"
	 * "50%-10 50%-10 50%+10 50%+10"
	 * 
	 * i.e. First percent modifier, then + or - and the pixel modifier.
	 * Although you can use just the percent or the pixel modifier. Notice
	 * though that the percent modifier must always be the first when
	 * both modifiers are inputted.
	 *
	 * @return true if success, false if failure. If false then the client area
	 *			will be unchanged.
	 */
	bool SetClientArea(const CStr& Value);
};

//--------------------------------------------------------
//  Forward declarations
//--------------------------------------------------------
class CGUI;
class IGUIObject;

/**
 * @author Gustav Larsson
 *
 * Base class to only the class GUI. This superclass is 
 * kind of a templateless extention of the class GUI.
 * Used for other functions to friend with, because it
 * it can't friend with GUI since it's templated (at least
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

	static void HandleMessage(IGUIObject *pObject, const SGUIMessage &message);
};


/**
 * @author Gustav Larsson
 *
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
	/**
	 * Retrieves a setting by name from object pointer
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PS_RESULT GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		if (!pObject->m_Settings.find(Setting)->second.m_pSetting)
			return PS_FAIL;

		// Set value
		Value = *(T*)pObject->m_Settings.find(Setting)->second.m_pSetting;
			
		return PS_OK;
	}

	/**
	 * Sets a value by name using a real datatype as input.
	 *
	 * This is the official way of setting a setting, no other
	 *  way should only causiously be used!
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 */
	static PS_RESULT SetSetting(IGUIObject *pObject, const CStr& Setting, const T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		*(T*)pObject->m_Settings[Setting].m_pSetting = Value;
		
		//
		//	Some settings needs special attention at change
		//

		// If setting was "size", we need to re-cache itself and all children
		if (Setting == CStr("size"))
		{
			RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);
		}
		else
		if (Setting == CStr("hidden"))
		{
			// Hiding an object requires us to reset it and all children
			QueryResetting(pObject);
			//RecurseObject(0, pObject, IGUIObject::ResetStates);
		}

		HandleMessage(pObject, SGUIMessage(GUIM_SETTINGS_UPDATED, Setting));

		return PS_OK;
	}

#ifdef g_GUI
	/**
	 * Adapter that uses the singleton g_GUI
	 * Can safely be removed.
	 */
	static PS_RESULT GetSetting(
		const CStr& Object, 
		const CStr& Setting, T &Value)
	{
		return GetSetting(g_GUI, Object, Setting, Value);
	}
#endif // g_GUI

	/**
	 * Retrieves a setting by settings name and object name
	 *
	 * @param GUI GUI Object const ref
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PS_RESULT GetSetting(
		const CGUI &GUIinstance, const CStr& Object, 
		const CStr& Setting, T &Value)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function
		const IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		return GetSetting(pObject, Setting, Value);
	}

#ifdef g_GUI
	/**
	 * Adapter that uses the singleton g_GUI
	 * Can safely be removed.
	 */
	static PS_RESULT SetSetting(
		const CStr& Object, const CStr& Setting, const T &Value)
	{
		return SetSetting(g_GUI, Object, Setting, Value);		
	}
#endif // g_GUI

	/**
	 * Sets a value by setting and object name using a real 
	 * datatype as input
	 *
	 * This is just a wrapper so that we can type the object name
	 *  and not input the actual pointer.
	 *
	 * @param GUI GUI Object, reference since we'll be changing values
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 */
	static PS_RESULT SetSetting(
		CGUI &GUIinstance, const CStr& Object, 
		const CStr& Setting, const T &Value)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function

		// Important, we don't want to use this T, we want
		//  to use the standard T, since that will be the
		//  one with the friend relationship
		IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		return SetSetting(pObject, Setting, Value);
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
	static CStr FallBackSprite(const CStr& prim, const CStr& sec)
	{
		// CStr() == empty string, null
		return ((prim!=CStr())?(prim):(sec));
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
	 * This is just a wrapper for _mem_ParseString() which really
	 * works the magic.
	 *
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 *
	 * @see _mem_ParseString()
	 */
	static bool ParseString(const CStr& Value, T &tOutput)
	{
		return __ParseString<T>(Value, tOutput);
	}

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
	 * @throws PS_RESULT Depends on what pFunc might throw. PS_RESULT is standard.
	 *			Itself doesn't throw anything.
	*/
	static void RecurseObject(const int &RR, IGUIObject *pObject, void_Object_pFunction_argT pFunc, const T &Argument)
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
	static void RecurseObject(const int &RR, IGUIObject *pObject, void_Object_pFunction_argRefT pFunc, T &Argument)
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
	static void RecurseObject(const int &RR, IGUIObject *pObject, void_Object_pFunction pFunc)
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
	static bool CheckIfRestricted(const int &RR, IGUIObject *pObject)
	{
		if (RR & GUIRR_HIDDEN)
		{
			bool hidden;
			GUI<bool>::GetSetting(pObject, "hidden", hidden);

			if (hidden)
				return true;
		}
		if (RR & GUIRR_DISABLED)
		{
			bool enabled;
			GUI<bool>::GetSetting(pObject, "enabled", enabled);

			if (!enabled)
				return true;
		}
		if (RR & GUIRR_GHOST)
		{
			bool ghost;
			GUI<bool>::GetSetting(pObject, "ghost", ghost);

			if (ghost)
				return true;
		}

		// false means not restricted
		return false;
	}
};

#endif
