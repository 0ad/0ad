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

// Generic object that stores a struct with settings

template <typename SETTINGS>
class CGUISettingsObject : virtual public CGUIObject
{
public:
	CGUISettingsObject() {}
	virtual ~CGUISettingsObject() {}

	// Get Offsets
	//  important so it returns this m_Offsets and not CGUIObject::m_SettingsInfo
	virtual map_Settings GetSettingsInfo() const { return m_SettingsInfo; }

	// GetSettings()
	// returns a copy of m_Settings
	SETTINGS GetSettings() const { return m_Settings; }
	
	// SetSettings
	// Sets m_Settings to _set
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
	// Settings
	SETTINGS								m_Settings;

	// Offset database
	//  tells us where a variable by a string name is
	//  located hardcoded, in order to acquire a pointer
	//  for that variable... Say "frozen" gives
	//  the offset from CGUIObject to m_Frozen
	// note! _NOT_ from SGUIBaseSettings to m_Frozen!
	//
	// Note that it's imperative that this m_SettingsInfo includes
	//  all offsets of m_BaseSettings too, because when
	//  using this class, this m_SettingsInfo will be the only
	//  one used.
	static map_Settings						m_SettingsInfo;
};

#endif