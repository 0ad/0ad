/*
GUI util
by Gustav Larsson
gee@pyro.nu

--Overview--

	Contains help class GUI<>, which gives us templated
	 parameter to all functions within GUI.

--More info--

	http://gee.pyro.nu/wfg/GUI/

*/

#ifndef GUIutil_H
#define GUIutil_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

//--------------------------------------------------------
//  Help Classes/Structs for the GUI
//--------------------------------------------------------
// TEMP
struct CRect
{
	CRect() {}
	CRect(int _l, int _t, int _r, int _b) :
		left(_l),
		top(_t),
		right(_r),
		bottom(_b) {}
	int bottom, top, left, right;

	bool operator ==(const CRect &rect) const
	{
		return	(bottom==rect.bottom) &&
				(top==rect.top) &&
				(left==rect.left) &&
				(right==rect.right);
	}

	bool operator !=(const CRect &rect) const
	{
		return	!(*this==rect);
	}
};

/**
 * @author Gustav Larsson
 *
 * Client Area is a rectangle relative to a parent rectangle
 *
 * You can input the whole value of the Client Area by
 * string. Like used in the GUI.
 */
struct CClientArea
{
public:
	CClientArea();
	CClientArea(const CStr &Value);

	/// Pixel modifiers
	CRect pixel;

	/// Percent modifiers (I'll let this be integers, I don't think a greater precision is needed)
	CRect percent;

	/**
	 * Get client area rectangle when the parent is given
	 */
	CRect GetClientArea(const CRect &parent);

	/**
	 * The ClientArea can be set from a string looking like:
	 *
	 * @code
	 * "0 0 100% 100%"
	 * "50%-10 50%-10 50%+10 50%+10" @endcode
	 * 
	 * i.e. First percent modifier, then + or - and the pixel modifier.
	 * Although you can use just the percent or the pixel modifier. Notice
	 * though that the percent modifier must always be the first when
	 * both modifiers are inputted.
	 *
	 * @return true if success, false if failure. If false then the client area
	 *			will be unchanged.
	 */
	bool SetClientArea(const CStr &Value);
};


// TEMP
struct CColor
{
	float r, g, b, a;
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
 * it can't friend with GUI since it's templated.
 */
class CInternalCGUIAccessorBase
{
protected:
	/// Get object pointer
	static IGUIObject * GetObjectPointer(CGUI &GUIinstance, const CStr &Object);
	
	/// const version
	static const IGUIObject * GetObjectPointer(const CGUI &GUIinstance, const CStr &Object);
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

public:
	/**
	 * Retrieves a setting by name from object pointer
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PS_RESULT GetSetting(const IGUIObject *pObject, const CStr &Setting, T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		Value = *(T*)((size_t)pObject+pObject->GetSettingsInfo()[Setting].m_Offset);

		return PS_OK;
	}

	/**
	 * Sets a value by name using a real datatype as input
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 */
	static PS_RESULT SetSetting(IGUIObject *pObject, const CStr &Setting, const T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		// This better be the correct adress
		*(T*)((size_t)pObject+pObject->GetSettingsInfo()[Setting].m_Offset) = Value;

		pObject->CheckSettingsValidity();

		return PS_OK;
	}

	/**
	 * Retrieves a setting by settings name and object name
	 *
	 * @param GUI GUI Object const ref
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PS_RESULT GetSetting(
		const CGUI &GUIinstance, const CStr &Object, 
		const CStr &Setting, T &Value)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function
		const IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		return GetSetting(pObject, Setting, Value);
	}

	/**
	 * Sets a value by setting and object name using a real 
	 * datatype as input
	 *
	 * @param GUI GUI Object, reference since we'll be changing values
	 * @param Object Object name
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 */
	static PS_RESULT SetSetting(
		CGUI &GUIinstance, const CStr &Object, 
		const CStr &Setting, const T &Value)
	{
		if (!GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function

		// Important, we don't want to use this T, we want
		//  to use the standard T, since that will be the
		//  one with the friend relationship
		IGUIObject *pObject = GetObjectPointer(GUIinstance, Object);

		pObject->CheckSettingsValidity();

		return SetSetting(pObject, Setting, Value);
	}

	//--------------------------------------------------------
	//  This function returns the C++ structure of the
	//	 inputted string. For instance if you input 
	//	 "0 0 10 10" and request a CRect, it will give you
	//	 a CRect(0,0,10,10).
	//	This function is widely used within the GUI.
	//  Input:
	//    String					The Value in string format
	//	Return:
	//	  Returns the value in the structure T.						
	//--------------------------------------------------------
/*	static T GetStringValue(const CStr &String)
	{
		if (typeid(T) == typeid(int))
		{
			return atoi(String.c_str());
		}

		if (typeid(T) == typeid(float) ||
			typeid(T) == typeid(double))
		{
			return atof(String.c_str());
		}

		if (typeid(T) == typeid(CRect))
		{
			(CRect)return CRect();
		}

		if (typeid(T) == typeid(CColor))
		{
			return CColor();
		}

		switch(typeid(T))
		{
		case typeid(int):
			return atoi(String);

		case typeid(float):
		case typeid(double):
			return atof(String);

		case typeid(CRect):
			return CRect(0,0,0,0);

		case typeid(CColor):
			return CColor(0,0,0,0);

		default:
			// Repport error unrecognized
			return T();
		}

		// If this function is called T is unrecognized
		
		// TODO repport error
		
		return T();
	}
*/
/*
	static T<int> GetStringValue(const CStr &String)
	{
		return atoi(String.c_str());
	}
*/
	// int
/*	static int GetStringValue(const CStr &String)
	{
		// If this function is called T is unrecognized
		
		// TODO repport error
		
		return 10;
	}
*/

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
			if (pObject->GetBaseSettings().m_Hidden)
				return true;
		}
		if (RR & GUIRR_DISABLED)
		{
			if (pObject->GetBaseSettings().m_Enabled)
				return true;
		}

		// false means not restricted
		return false;
	}
};

#endif
