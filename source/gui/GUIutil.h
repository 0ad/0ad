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

// TEMP
struct CColor
{
	float r, g, b, a;
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
	CRect GetClientArea(const CRect &parent) const;

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

	/// Wrapper for ResetStates
	static void QueryResetting(IGUIObject *pObject);
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
	static PS_RESULT GetSetting(const IGUIObject *pObject, const CStr &Setting, T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		Value = 
			*(T*)((size_t)pObject->GetStructPointer(pObject->GetSettingsInfo()[Setting].m_SettingsStruct) +
				  pObject->GetSettingsInfo()[Setting].m_Offset);

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
	static PS_RESULT SetSetting(IGUIObject *pObject, const CStr &Setting, const T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		*(T*)((size_t)pObject->GetStructPointer(pObject->GetSettingsInfo()[Setting].m_SettingsStruct) +
			   pObject->GetSettingsInfo()[Setting].m_Offset) = Value;

		
		//
		//	Some settings needs special attention at change
		//

		// If setting was "size", we need to re-cache itself and all children
		if (Setting == CStr(_T("size")))
		{
			RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);
		}
		else
		if (Setting == CStr(_T("hidden")))
		{
			// Hiding an object requires us to reset it and all children
			QueryResetting(pObject);
			//RecurseObject(0, pObject, IGUIObject::ResetStates);
		}

		pObject->HandleMessage(SGUIMessage(GUIM_SETTINGS_UPDATED, Setting));

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
	 * This is just a wrapper so that we can type the object name
	 *  and not input the actual pointer.
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

		return SetSetting(pObject, Setting, Value);
	}

		
	/**
	 * Sets a value by setting and object name using a real 
	 * datatype as input.
	 *
	 * This is just a wrapper for _mem_ParseString() which really
	 * works the magic.
	 *
	 * @param Type type in string, like "float" or "client area"
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 *
	 * @see _mem_ParseString()
	 */
	static bool ParseString(const CStr &Value, T &tOutput)
	{
		void *mem = NULL;

		if (!_mem_ParseString(Value, mem))
			return false;
	
		// Copy from memory
		tOutput = *(T*)mem;

		delete [] mem;

		// TODO Gee: Undefined type - maybe report in log
		return true;
	}

private:
	/**
	 * Input a value in string form, and it will output the result in
	 * Memory with type T.
	 *
	 * @param Type type in string, like "float" or "client area"
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param Memory Should be NULL, will be constructed within the function.
	 * @return True at success.
	 */
	static bool _mem_ParseString(const CStr &Value, void *&Memory)
	{
/*		if (typeid(T) == typeid(CStr))
		{
			tOutput = Value;
			return true;
		}
		else
*/		if (typeid(T) == typeid(bool))
		{
			bool _Value;

			if (Value == CStr(_T("true")))
				_Value = true;
			else
			if (Value == CStr(_T("false")))
				_Value = false;
			else 
				return false;

			Memory = malloc(sizeof(bool));
			memcpy(Memory, (const void*)&_Value, sizeof(bool));
			return true;
		}
		else
		if (typeid(T) == typeid(float))
		{
			float _Value = Value.ToFloat();
			// TODO Gee: Okay float value!?
			Memory = malloc(sizeof(float));
			memcpy(Memory, (const void*)&_Value, sizeof(float));
			return true;
		}
		else
		if (typeid(T) == typeid(int))
		{
			int _Value = Value.ToInt();
			// TODO Gee: Okay float value!?
			Memory = malloc(sizeof(int));
			memcpy(Memory, (const void*)&_Value, sizeof(int));
			return true;
		}
		else
		if (typeid(T) == typeid(CRect))
		{
			// Use the parser to parse the values
			CParser parser;
			parser.InputTaskType("", "_$value_$value_$value_$value_");

			string str = (const TCHAR*)Value;

			CParserLine line;
			line.ParseString(parser, str);
			if (!line.m_ParseOK)
			{
				// Parsing failed
				return false;
			}
			int values[4];
			for (int i=0; i<4; ++i)
			{
				if (!line.GetArgInt(i, values[i]))
				{
					// Parsing failed
					return false;
				}
			}

			// Finally the rectangle values
			CRect _Value(values[0], values[1], values[2], values[3]);
			
			Memory = malloc(sizeof(CRect));
			memcpy(Memory, (const void*)&_Value, sizeof(CRect));
			return true;
		}
		else
		if (typeid(T) == typeid(CClientArea))
		{
			// Get Client Area
			CClientArea _Value;

			// Check if valid!
			if (!_Value.SetClientArea(Value))
			{
				return false;
			}

			Memory = malloc(sizeof(CClientArea));
			memcpy(Memory, (const void*)&_Value, sizeof(CClientArea));
			return true;
		}
		else
		if (typeid(T) == typeid(CColor))
		{
			// Use the parser to parse the values
			CParser parser;
			parser.InputTaskType("", "_$value_$value_$value_[$value_]");

			string str = (const TCHAR*)Value;

			CParserLine line;
			line.ParseString(parser, str);
			if (!line.m_ParseOK)
			{
				// TODO Gee: Parsing failed
				return false;
			}
			float values[4];
			values[3] = 255.f; // default
			for (int i=0; i<line.GetArgCount(); ++i)
			{
				if (!line.GetArgFloat(i, values[i]))
				{
					// TODO Gee: Parsing failed
					return false;
				}
			}

			// Finally the rectangle values
			CColor _Value;
			// TODO Gee: Done better when CColor is sweeter
			_Value.r = values[0]/255.f;
			_Value.g = values[1]/255.f;
			_Value.b = values[2]/255.f;
			_Value.a = values[3]/255.f;
			
			Memory = malloc(sizeof(CColor));
			memcpy(Memory, (const void*)&_Value, sizeof(CColor));
			return true;
		}

		// TODO Gee: Undefined type - maybe report in log
		return false;
	}

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
			if (pObject->GetBaseSettings().m_Hidden)
				return true;
		}
		if (RR & GUIRR_DISABLED)
		{
			if (pObject->GetBaseSettings().m_Enabled)
				return true;
		}
		if (RR & GUIRR_GHOST)
		{
			if (pObject->GetBaseSettings().m_Ghost)
				return true;
		}

		// false means not restricted
		return false;
	}
};

#endif
