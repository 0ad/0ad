/*
CGUIObject
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"
#include "cgui.h"
///#include "Parser/parser.h"
#include <assert.h>

using namespace std;

// Offsets
map_Settings CGUIObject::m_SettingsInfo;

//-------------------------------------------------------------------
//  Implementation Macros
//-------------------------------------------------------------------
#define _GUI_ADD_OFFSET(type, str, var) \
	SettingsInfo[str].m_Offset = offsetof(CGUIObject, m_BaseSettings) + offsetof(SGUIBaseSettings,var); \
	SettingsInfo[str].m_Type = type;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CGUIObject::CGUIObject() : 
	m_pGUI(NULL), 
	m_pParent(NULL),
	m_MouseHovering(false)
{
	// Default values of base settings !
	m_BaseSettings.m_Enabled =		true;
	m_BaseSettings.m_Hidden =		false;
	m_BaseSettings.m_Style =		"null";
	m_BaseSettings.m_Z =			0.f;

	// Static! Only done once
	if (m_SettingsInfo.empty())
	{
		SetupBaseSettingsInfo(m_SettingsInfo);
	}
}

CGUIObject::~CGUIObject()
{
}

//-------------------------------------------------------------------
//  Change the base settings
//	Input:
//	  Set						Setting struct
//-------------------------------------------------------------------
void CGUIObject::SetBaseSettings(const SGUIBaseSettings &Set)
{ 
	m_BaseSettings = Set; 
	CheckSettingsValidity(); 
}

//-------------------------------------------------------------------
//  Adds a child
//	Notice nothing will be returned or thrown if the child hasn't
//	 been inputted into the GUI yet. This is because that's were
//	 all is checked. Now we're just linking two objects, but
//	 it's when we're inputting them into the GUI we'll check
//	 validity! Notice also when adding it to the GUI this function
//	 will inevitably have been called by CGUI::AddObject which
//	 will catch the throw and return the error code.
//	i.e. The user will never put in the situation wherein a throw
//	 must be caught, the GUI's internal error handling will be
//	 completely transparent to the interfacially sequential model.
//	Input:
//	  pChild				Child to add
//-------------------------------------------------------------------
void CGUIObject::AddChild(CGUIObject *pChild)
{
	// 
//	assert(pChild);

	pChild->SetParent(this);

	m_Children.push_back(pChild);

	// If this (not the child) object is already attached
	//  to a CGUI, it pGUI pointer will be non-null.
	//  This will mean we'll have to check if we're using
	//  names already used.
	if (pChild->GetGUI())
	{
		try
		{
			// Atomic function, if it fails it won't
			//  have changed anything
			//UpdateObjects();
			pChild->GetGUI()->UpdateObjects();
		}
		catch (PS_RESULT e)
		{
			// If anything went wrong, reverse what we did and throw
			//  an exception telling it never added a child
			m_Children.erase( m_Children.end()-1 );

			// We'll throw the same exception for easier
			//  error handling
			throw e;
		}
	}
	// else do nothing
}

//-------------------------------------------------------------------
//  Adds object and its children to the map, it's name being the
//	 first part, and the second being itself.
//	Input:
//	  ObjectMap				Checks to see if the name's already taken
//	Output:
//	  ObjectMap				Fills it with more
//	Throws:
//	  PS_NAME_AMBIGUITY
//-------------------------------------------------------------------
void CGUIObject::AddToPointersMap(map_pObjects &ObjectMap)
{
	// Just don't do anything about the top node
	if (m_pParent == NULL)
		return;

	// Now actually add this one
	//  notice we won't add it if it's doesn't have any parent
	//  (i.e. being the base object)
	if (m_Name == string())
	{
		throw PS_NEEDS_NAME;
	}
	if (ObjectMap.count(m_Name) > 0)
	{
		throw PS_NAME_AMBIGUITY;
	}
	else
	{
		ObjectMap[m_Name] = this;
	}
}

//-------------------------------------------------------------------
//  Destroys all children and the current object too
//-------------------------------------------------------------------
void CGUIObject::Destroy()
{
	// Is there anything besides the children to destroy?
}

//-------------------------------------------------------------------
//  Sets up a map_size_t to include the variables in m_BaseSettings
//  Input:
//    p						Pointers that should be filled with base
//							 variables
//-------------------------------------------------------------------
void CGUIObject::SetupBaseSettingsInfo(map_Settings &SettingsInfo)
{
	_GUI_ADD_OFFSET("bool",		"enabled",		m_Enabled)
	_GUI_ADD_OFFSET("bool",		"hidden",		m_Hidden)
	_GUI_ADD_OFFSET("rect",		"size1024",		m_Size)
	_GUI_ADD_OFFSET("string",	"style",		m_Style)
	_GUI_ADD_OFFSET("float",	"z",			m_Z)
	_GUI_ADD_OFFSET("string",	"caption",		m_Caption)
}




//-------------------------------------------------------------------
//  Checks if mouse is over and returns result
//  mouse_x, mouse_y defined in CGUI
//-------------------------------------------------------------------
bool CGUIObject::MouseOver()
{
	CGUI* gui = GetGUI();
	if(!gui)
		throw PS_NEEDS_PGUI;

	return (gui->mouse_x >= m_BaseSettings.m_Size.left &&
			gui->mouse_x <= m_BaseSettings.m_Size.right &&
			gui->mouse_y >= m_BaseSettings.m_Size.bottom &&
			gui->mouse_y <= m_BaseSettings.m_Size.top);
}

//-------------------------------------------------------------------
//  Inputes the object that is currently hovered, this function
//	 updates this object accordingly (i.e. if it's the object
//	 being inputted one thing happens, and not, another).
//	Input:
//	  pMouseOver				Object that is currently hovered
//								 can OF COURSE be NULL too!
//-------------------------------------------------------------------
void CGUIObject::UpdateMouseOver(CGUIObject * const &pMouseOver)
{
	// Check if this is the object being hovered.
	if (pMouseOver == this)
	{
		if (!m_MouseHovering)
		{
			// It wasn't hovering, so that must mean it just entered
			HandleMessage(GUIM_MOUSE_ENTER);
		}

		// Either way, set to true
		m_MouseHovering = true;

		// call mouse over
		HandleMessage(GUIM_MOUSE_OVER);
	}
	else // Some other object (or none) is hovered
	{
		if (m_MouseHovering)
		{
			m_MouseHovering = false;
			HandleMessage(GUIM_MOUSE_LEAVE);
		}
	}
}

//-------------------------------------------------------------------
//  Check if setting exists by name
//  Input:
//    Setting				Setting by name
//-------------------------------------------------------------------
bool CGUIObject::SettingExists(const CStr &Setting) const
{
	// Because GetOffsets will direct dynamically defined
	//  classes with polymorifsm to respective m_SettingsInfo
	//  we need to make no further updates on this function
	//  in derived classes.
	return (GetSettingsInfo().count(Setting) == 1)?true:false;
}


//-------------------------------------------------------------------
//  Set a setting by string, regardless of what type it is...
//	 example a CRect(10,10,20,20) would be "10 10 20 20"
//  Input:
//    Setting				Setting by name
//    Value					Value
//-------------------------------------------------------------------
void CGUIObject::SetSetting(const CStr &Setting, const CStr &Value)
{
	if (!SettingExists(Setting))
	{
		throw PS_FAIL;
	}

	// Get setting
	SGUISetting set = GetSettingsInfo()[Setting];

	if (set.m_Type == "string")
	{
		GUI<string>::SetSetting(this, Setting, Value);
	}
	else
	if (set.m_Type == "float")
	{
		// Use the parser to parse the values
/*		CParser parser;
		parser.InputTaskType("", "_$value_");

		CParserLine line;
		line.ParseString(parser, Value);
		if (!line.m_ParseOK)
		{
			// ERROR!
			throw PS_FAIL;
		}

		float value;
		if (!line.GetArgFloat(0, value))
		{
			// ERROR!
			throw PS_FAIL;
		}

		// Finally the rectangle values
		GUI<float>::SetSetting(this, Setting, value);
*/
		GUI<float>::SetSetting(this, Setting, (float)atof(Value.c_str()) );
	}
	else
	if (set.m_Type == "rect")
	{
		// TEMP
		GUI<CRect>::SetSetting(this, Setting, CRect(100,100,200,200));

		// Use the parser to parse the values
/*		CParser parser;
		parser.InputTaskType("", "_$value_$value_$value_$value_");

		CParserLine line;
		line.ParseString(parser, Value);
		if (!line.m_ParseOK)
		{
			// ERROR!
			throw PS_FAIL;
		}
		int values[4];
		for (int i=0; i<4; ++i)
		{
			if (!line.GetArgInt(i, values[i]))
			{
				// ERROR!
				throw PS_FAIL;
			}
		}

		// Finally the rectangle values
		CRect rect(values[0], values[1], values[2], values[3]);
		GUI<CRect>::SetSetting(this, Setting, rect);
*/	}
	else
	{
		throw PS_FAIL;
	}
}


//-------------------------------------------------------------------
//  Inputs a reference pointer, checks if the new inputted object
//	 if hovered, if so, then check if (this)'s Z value is greater
//	 than the inputted object... If so then the object is closer
//	 and we'll replace the pointer with (this)
//	Also Notice input can be NULL, which means the Z value demand
//	 is out. NOTICE you can't input NULL as const so you'll have
//	 to set an object to NULL.
//  Input:
//    pObject				Object pointer
//  Input:
//    pObject				Object pointer, either old or (this)
//-------------------------------------------------------------------
void CGUIObject::ChooseMouseOverAndClosest(CGUIObject* &pObject)
{
	if (MouseOver())
	{
		// Check if we've got competition at all
		if (pObject == NULL)
		{
			pObject = this;
			return;
		}

		// Or if it's closer
		if (GetBaseSettings().m_Z >= pObject->GetBaseSettings().m_Z)
		{
			pObject = this;
			return;
		}
	}
}

//-------------------------------------------------------------------
//  Get Object's parent, notice that if the parent is the top-node
//	 then, we'll return NULL, because we don't want the top-node
//   taken into account.
//	Return:
//	  The Parent
//-------------------------------------------------------------------
CGUIObject *CGUIObject::GetParent()
{
	// Important, we're not using GetParent() for these
	//  checks, that could screw it up
	if (m_pParent)
	{
		if (m_pParent->m_pParent == NULL)
			return NULL;
	}

	return m_pParent;
}

//-------------------------------------------------------------------
//  Called every time settings are change, this is where you check
//	 validity (not syntactical, that's already check) of your values.
//	 perhaps you can't have Z being below 0. Anyway this is where
//	 all is checked, and if you wanbt to add more in a derived object
//	 do that in GUIM_SETTINGS_UPDATED
//-------------------------------------------------------------------
void CGUIObject::CheckSettingsValidity()
{
	// If we hide an object, reset many of its parts
	if (GetBaseSettings().m_Hidden)
	{
		// Simulate that no object is hovered for this object and all its children
		//  why? because it's 
		try
		{
			GUI<CGUIObject*>::RecurseObject(0, this, &CGUIObject::UpdateMouseOver, NULL);
		}
		catch (...) {}
	}

	try
	{
		// Send message to myself
		HandleMessage(GUIM_SETTINGS_UPDATED);
	}
	catch (...)
	{
	}
}