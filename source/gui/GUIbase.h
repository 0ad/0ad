/*
GUI Core, stuff that the whole GUI uses
by Gustav Larsson
gee@pyro.nu

--Overview--

	Contains defines, includes, types etc that the whole 
	 GUI should have included.

--More info--

	Check GUI.h

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

// Object settings setups

// Setup an object's ConstructObject function
#define GUI_OBJECT(obj)													\
public:																	\
	static IGUIObject *ConstructObject() { return new obj(); }


//--------------------------------------------------------
//  Types
//--------------------------------------------------------
/** 
 * @enum EGUIMessage
 * Message types
 *
 * @see SGUIMessage
 */
enum EGUIMessageType
{
	GUIM_PREPROCESS,		// questionable
	GUIM_POSTPROCESS,		// questionable
	GUIM_MOUSE_OVER,
	GUIM_MOUSE_ENTER,
	GUIM_MOUSE_LEAVE,
	GUIM_MOUSE_PRESS_LEFT,
	GUIM_MOUSE_PRESS_RIGHT,
	GUIM_MOUSE_DOWN_LEFT,
	GUIM_MOUSE_DOWN_RIGHT,
	GUIM_MOUSE_RELEASE_LEFT,
	GUIM_MOUSE_RELEASE_RIGHT,
	GUIM_MOUSE_WHEEL_UP,
	GUIM_MOUSE_WHEEL_DOWN,
	GUIM_SETTINGS_UPDATED,	// SGUIMessage.m_Value = name of setting
	GUIM_PRESSED,
	GUIM_MOUSE_MOTION,
	GUIM_LOAD				// Called when an object is added to the GUI.
};

/**
 * @author Gustav Larsson
 *
 * Message send to IGUIObject::HandleMessage() in order
 * to give life to Objects manually with
 * a derived HandleMessage().
 */
struct SGUIMessage
{
	SGUIMessage() {}
	SGUIMessage(const EGUIMessageType &_type) : type(_type) {}
	SGUIMessage(const EGUIMessageType &_type, const CStr &_value) : type(_type), value(_value) {}
	~SGUIMessage() {}

	/**
	 * Describes what the message regards
	 */
	EGUIMessageType type;

	/**
	 * Optional data
	 */
	CStr value;
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
	GUIRR_HIDDEN		= 0x00000001,
	GUIRR_DISABLED		= 0x00000010,
	GUIRR_GHOST			= 0x00000100
};

/**
 * @enum EGUISettingsStruct
 * 
 * Stored in SGUISetting, tells us in which struct
 * the setting is located, that way we can query
 * for the structs address.
 */
enum EGUISettingsStruct
{
	GUISS_BASE,
	GUISS_EXTENDED
};

// Typedefs
typedef	std::map<CStr, IGUIObject*> map_pObjects;
typedef std::vector<IGUIObject*> vector_pObjects;

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
