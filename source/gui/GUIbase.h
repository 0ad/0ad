/*
GUI Core, stuff that the whole GUI uses
by Gustav Larsson
gee@pyro.nu

--Overview--

	Contains defines, includes, types etc that the whole 
	 GUI should have included.

--More info--

	http://gee.pyro.nu/wfg/GUI/

*/

#ifndef GUIbase_H
#define GUIbase_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

//--------------------------------------------------------
//  Forward declarations
//--------------------------------------------------------
class IGUIObject;


//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

// Global CGUI
#define g_GUI CGUI::GetSingleton()

// Temp
#define CInput		nemInput
//#define CStr		std::string

// Example
//  GUI_ADD_OFFSET(CButton, SButtonSettings, m_Settings, "frozen", m_Frozen);
//
#define GUI_ADD_OFFSET(_class, _struct, name, type, str, var) \
	m_SettingsInfo[str].m_Offset = offsetof(_class, name) + offsetof(_struct, var);	\
	m_SettingsInfo[str].m_Type = type;

// Declares the static variable in IGUISettingsObject<>
#define DECLARE_SETTINGS_INFO(_struct) \
	map_Settings IGUISettingsObject<_struct>::m_SettingsInfo;

// Setup an object's ConstructObject function
#define GUI_OBJECT(obj)													\
public:																	\
	static IGUIObject *ConstructObject() { return new obj(); }


//--------------------------------------------------------
//  Types
//--------------------------------------------------------
/** 
 * @enum EGUIMessage
 * Message send to IGUIObject::HandleMessage() in order
 * to give life to Objects manually with
 * a derived HandleMessage().
 */
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

/**
 * Recurse restrictions, when we recurse, if an object
 * is hidden for instance, you might want it to skip
 * the children also
 * Notice these are flags! and we don't really need one
 * for no restrictions, because then you'll just enter 0
 */
enum
{
	GUIRR_HIDDEN=1,
	GUIRR_DISABLED=2
};

// Typedefs
typedef	std::map<CStr, IGUIObject*> map_pObjects;


//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------
DECLARE_ERROR(PS_NAME_TAKEN)
DECLARE_ERROR(PS_OBJECT_FAIL)
DECLARE_ERROR(PS_SETTING_FAIL)
DECLARE_ERROR(PS_VALUE_INVALID)
DECLARE_ERROR(PS_NEEDS_PGUI)
DECLARE_ERROR(PS_NAME_AMBIGUITY)
DECLARE_ERROR(PS_NEEDS_NAME)

DECLARE_ERROR(PS_LEXICAL_FAIL)
DECLARE_ERROR(PS_SYNTACTICAL_FAIL)

#endif