/*
Object with settings
by Gustav Larsson
gee@pyro.nu

--Overview--

	Generic object that stores a struct with settings

--Usage--

	If an object wants settings with a standard,
	it will use this as a middle step instead of being
	directly derived from CGUIObject

--Examples--

	instead of:
	
		class CButton : public CGUIObject

	you go:

		class CButton : public CGUISettingsObject<SButtonSettings>

	and SButtonSettings will be included as m_Settings with
	all gets and sets set up

--More info--

	Check GUI.h

*/

#ifndef CGUISettingsObject_H
#define CGUISettingsObject_H

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
 * Appends more settings to the <code>CGUIObject</code>.
 * Can be used with multiple inheritance.
 *
 * @see CGUIObject
 */
template <typename SETTINGS>
class CGUISettingsObject : virtual public CGUIObject
{
public:
	CGUISettingsObject() {}
	virtual ~CGUISettingsObject() {}

	/**
	 * Get Offsets, <b>important</b> to include so it returns this 
	 * <code>m_Offsets</code> and not <code>CGUIObject::m_SettingsInfo</code>
	 *
	 * @return Settings infos
	 */
	virtual map_Settings GetSettingsInfo() const { return m_SettingsInfo; }

	/**
	 * @return Returns a copy of <code>m_Settings</code>
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
	/// Settings struct
	SETTINGS								m_Settings;

	/**
	 * <b>Offset database</b><br>
	 * tells us where a variable by a string name is
	 * located hardcoded, in order to acquire a pointer
	 * for that variable... Say "frozen" gives
	 * the offset from <code>CGUIObject</code> to <code>m_Frozen</code>.
	 *
	 * <b>note!</b> _NOT_ from <code>SGUIBaseSettings</code> to <code>m_Frozen</code>!
	 *
	 * Note that it's imperative that this <code>m_SettingsInfo</code> includes
	 * all offsets of <code>m_BaseSettings</code> too, because when
	 * using this class, this m_SettingsInfo will be the only
	 * one used.
	 */
	static map_Settings						m_SettingsInfo;
};

#endif