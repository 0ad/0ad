/*
GUI Object Base - Setting Extension
by Gustav Larsson
gee@pyro.nu

--Overview--

	Generic object that stores a struct with settings

--Usage--

	If an object wants settings with a standard,
	it will use this as a middle step instead of being
	directly derived from IGUIObject

--Examples--

	instead of:
	
		class CButton : public IGUIObject

	you go:

		class CButton : public IGUISettingsObject<SButtonSettings>

	and SButtonSettings will be included as m_Settings with
	all gets and sets set up

--More info--

	Check GUI.h

*/

#ifndef IGUISettingsObject_H
#define IGUISettingsObject_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Gustav Larsson
 *
 * Appends more settings to the IGUIObject.
 * Can be used with multiple inheritance.
 *
 * @see IGUIObject
 */
template <typename SETTINGS>
class IGUISettingsObject : virtual public IGUIObject
{
public:
	IGUISettingsObject() {}
	virtual ~IGUISettingsObject() {}

	/**
	 * Get Offsets, <b>important</b> to include so it returns this 
	 * m_Offsets and not IGUIObject::m_SettingsInfo
	 *
	 * @return Settings infos
	 */
	virtual map_Settings GetSettingsInfo() const { return m_SettingsInfo; }

	/**
	 * @return Returns a copy of m_Settings
	 */
	SETTINGS GetSettings() const { return m_Settings; }
	
	/// Sets settings
	void SetSettings(const SETTINGS &Set) 
	{ 
		m_Settings = Set; 

		//CheckSettingsValidity();
		// Since that function out-commented above really
		//  does just update the base settings, we'll call
		//  the message immediately instead
		try
		{
			HandleMessage(GUIM_SETTINGS_UPDATED);
		}
		catch (...) { }
	}

protected:
	/**
	 * You input the setting struct you want, and it will return a pointer to
	 * the struct.
	 *
	 * @param SettingsStruct tells us which pointer to return
	 */
	virtual void *GetStructPointer(const EGUISettingsStruct &SettingsStruct) const
	{
		switch (SettingsStruct)
		{
		case GUISS_BASE:
			return (void*)&m_BaseSettings;

		case GUISS_EXTENDED:
			return (void*)&m_Settings;

		default:
			// TODO Gee: Report error
			return NULL;
		}
	}

	/// Settings struct
	SETTINGS								m_Settings;

	/**
	 * <b>Offset database</b>\n
	 * tells us where a variable by a string name is
	 * located hardcoded, in order to acquire a pointer
	 * for that variable... Say "frozen" gives
	 * the offset from IGUIObject to m_Frozen.
	 *
	 * <b>note!</b> _NOT_ from SGUIBaseSettings to m_Frozen!
	 *
	 * Note that it's imperative that this m_SettingsInfo includes
	 * all offsets of m_BaseSettings too, because when
	 * using this class, this m_SettingsInfo will be the only
	 * one used.
	 */
	static map_Settings						m_SettingsInfo;
};

#endif
