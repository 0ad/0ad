/*
GUI Inclusion file
by Gustav Larsson
gee@pyro.nu

--Overview--

	Include this file and it will include the whole GUI

	Also includes global GUI functions that is used to
	 make global functions templated

--More info--

	http://gee.pyro.nu/wfg/GUI/

*/

#ifndef GUI_H
#define GUI_H

#ifdef WIN32
# pragma warning(disable:4786)
#endif // WIN32

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
// { temp TODO
///	#include "nemesis.h"
	#define DEFINE_ERROR(x, y)  PS_RESULT x=y; 
	#define DECLARE_ERROR(x)  extern PS_RESULT x; 
// } temp

// Includes used by the whole GUI
#include <map>
#include <string>
#include <stddef.h>


#include "ogl.h"

//--------------------------------------------------------
//  TODO name this section
//--------------------------------------------------------
class CGUIObject;

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------
// Temp
///#define CInput		nemInput
#define CStr		std::string

// Example
//  GUI_ADD_OFFSET(CButton, SButtonSettings, m_Settings, "frozen", m_Frozen);
//
#define GUI_ADD_OFFSET(_class, _struct, name, type, str, var) \
	m_SettingsInfo[str].m_Offset = offsetof(_class, name) + offsetof(_struct, var);	\
	m_SettingsInfo[str].m_Type = type;

// Declares the static variable in CGUISettingsObject<>
#define DECLARE_SETTINGS_INFO(_struct) \
	map_Settings CGUISettingsObject<_struct>::m_SettingsInfo;

// Setup an object's ConstructObject function
#define GUI_OBJECT(obj)													\
public:																	\
	static CGUIObject *ConstructObject() { return new obj(); }

//--------------------------------------------------------
//  Types
//--------------------------------------------------------
// Message send to HandleMessage in order
//  to give life to Objects manually with
//  a derived HandleMessage().
enum EGUIMessage
{
	GUIM_PREPROCESS,
	GUIM_POSTPROCESS,
	GUIM_MOUSE_OVER,
	GUIM_MOUSE_ENTER,
	GUIM_MOUSE_LEAVE,
	GUIM_MOUSE_PRESS_LEFT,
	GUIM_MOUSE_PRESS_RIGHT,
	GUIM_MOUSE_DOWN_LEFT,
	GUIM_MOUSE_DOWN_RIGHT,
	GUIM_MOUSE_RELEASE_LEFT,
	GUIM_MOUSE_RELEASE_RIGHT,
	GUIM_SETTINGS_UPDATED,
	GUIM_PRESSED
};

// Recurse restrictions, when we recurse, if an object
//  is hidden for instance, you might want it to skip
//	the children also
// Notice these are flags! and we don't really need one
//  for no restrictions, because then you'll just enter 0
enum
{
	GUIRR_HIDDEN=1,
	GUIRR_DISABLED=2
};

// Typedefs
typedef	std::map<CStr, CGUIObject*> map_pObjects;

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------
typedef const char * PS_RESULT;
DECLARE_ERROR(PS_FAIL)
DECLARE_ERROR(PS_OK)
DECLARE_ERROR(PS_NAME_TAKEN)
DECLARE_ERROR(PS_OBJECT_FAIL)
DECLARE_ERROR(PS_SETTING_FAIL)
DECLARE_ERROR(PS_VALUE_INVALID)
DECLARE_ERROR(PS_NEEDS_PGUI)
DECLARE_ERROR(PS_NAME_AMBIGUITY)
DECLARE_ERROR(PS_NEEDS_NAME)


DECLARE_ERROR(PS_LEXICAL_FAIL)
DECLARE_ERROR(PS_SYNTACTICAL_FAIL)

//--------------------------------------------------------
//  Includes static functions that needs one template
//	 argument.
//--------------------------------------------------------
// int is only to please functions that doesn't even use
//	T
template <typename T=int>
class GUI
{
	// Private functions further ahead
	friend class CGUI;
	friend class CGUIObject;

public:
	//--------------------------------------------------------
	//  Retrieves a setting by name
	//  Input:
	//    pObject					Object pointer
	//    Setting					Setting by name
	//  Output:
	//    Value						Stores value here
	//								 note type T!
	//--------------------------------------------------------
	static PS_RESULT GetSetting(CGUIObject *pObject, const CStr &Setting, T &Value)
	{
		if (pObject == NULL)
			return PS_OBJECT_FAIL;

		if (!pObject->SettingExists(Setting))
			return PS_SETTING_FAIL;

		// Set value
		Value = *(T*)((size_t)pObject+pObject->GetSettingsInfo()[Setting].m_Offset);

		return PS_OK;
	}

	//--------------------------------------------------------
	//  Sets a value by name using a real datatype as input
	//  Input:
	//    pObject					Object pointer
	//    Setting					Setting by name
	//    Value						Sets value to this
	//								 note type T!
	//--------------------------------------------------------
	static PS_RESULT SetSetting(CGUIObject *pObject, const CStr &Setting, const T &Value)
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

	//--------------------------------------------------------
	//  Retrieves a setting and object name
	//  Input:
	//	  GUI						GUI Object const ref
	//    Object					Object name
	//    Setting					Setting by name
	//  Output:
	//    Value						Stores value here
	//								 note type T!
	//--------------------------------------------------------
/*	static PS_RESULT GetSetting(
		const CGUI &GUIinstance, const CStr &Object, 
		const CStr &Setting, T &Value)
	{
		if (GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function
		CGUIObject *pObject = GUIinstance.m_pAllObjects[Object];

		return GetSetting(pObject, Setting, Value);
	}

	//--------------------------------------------------------
	//  Sets a value by setting and object name using a real 
	//	 datatype as input
	//  Input:
	//	  GUI						GUI Object const ref
	//    Object					Object name
	//    Setting					Setting by name
	//    Value						Sets value to this
	//								 note type T!
	//--------------------------------------------------------
	static PS_RESULT SetSetting(
		const CGUI &GUIinstance, const CStr &Object, 
		const CStr &Setting, const T &Value)
	{
		if (GUIinstance.ObjectExists(Object))
			return PS_OBJECT_FAIL;

		// Retrieve pointer and call sibling function
		CGUIObject *pObject = GUIinstance.m_pAllObjects[Object];

		return SetSetting(pObject, Setting, Value);
	}
*/
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
	typedef void (CGUIObject::*void_Object_pFunction_argT)(const T &arg);
	typedef void (CGUIObject::*void_Object_pFunction_argRefT)(T &arg);
	typedef void (CGUIObject::*void_Object_pFunction)();

	//--------------------------------------------------------
	//  Recurses an object calling a function on itself
	//	 and all children (and so forth)
	//  Input:
	//	  RR						Recurse Restrictions
	//	  pObject					Object to iterate
	//    pFunc						Function to recurse
	//    Argument					Argument of type T
	//--------------------------------------------------------
	static void RecurseObject(const int &RR, CGUIObject *pObject, void_Object_pFunction_argT pFunc, const T &Argument)
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

	//--------------------------------------------------------
	//  Same as above only with reference
	//--------------------------------------------------------
	static void RecurseObject(const int &RR, CGUIObject *pObject, void_Object_pFunction_argRefT pFunc, T &Argument)
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

	//--------------------------------------------------------
	//  Same as above only with no argument
	//--------------------------------------------------------
	static void RecurseObject(const int &RR, CGUIObject *pObject, void_Object_pFunction pFunc)
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
	// Sub functions
	static bool CheckIfRestricted(const int &RR, CGUIObject *pObject)
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

//--------------------------------------------------------
//  Post includes
//--------------------------------------------------------
#include "CGUIObject.h"
#include "CGUISettingsObject.h"
#include "CGUIButtonBehavior.h"
#include "CButton.h"
#include "CGUISprite.h"
#include "CGUI.h"

//--------------------------------------------------------
//  Prototypes
//--------------------------------------------------------


#endif