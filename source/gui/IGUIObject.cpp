/*
IGUIObject
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"

///// janwas: again, including etiquette?
#include "../ps/Parser.h"
#include <assert.h>
/////

using namespace std;

// Offsets
map_Settings IGUIObject::m_SettingsInfo;

//-------------------------------------------------------------------
//  Implementation Macros
//-------------------------------------------------------------------
#define _GUI_ADD_OFFSET(type, str, var) \
	SettingsInfo[str].m_Offset = offsetof(IGUIObject, m_BaseSettings) + offsetof(SGUIBaseSettings,var); \
	SettingsInfo[str].m_Type = type;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIObject::IGUIObject() : 
	m_pGUI(NULL), 
	m_pParent(NULL),
	m_MouseHovering(false)
{
	// Default values of base settings !
	m_BaseSettings.m_Enabled =		true;
	m_BaseSettings.m_Hidden =		false;
	m_BaseSettings.m_Style =		"null";
	m_BaseSettings.m_Z =			0.f;
	m_BaseSettings.m_Absolute =		true;

	// Static! Only done once
	if (m_SettingsInfo.empty())
	{
		SetupBaseSettingsInfo(m_SettingsInfo);
	}
}

IGUIObject::~IGUIObject()
{
}

//-------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------
void IGUIObject::AddChild(IGUIObject *pChild)
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

void IGUIObject::AddToPointersMap(map_pObjects &ObjectMap)
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

void IGUIObject::Destroy()
{
	// Is there anything besides the children to destroy?
}

void IGUIObject::SetupBaseSettingsInfo(map_Settings &SettingsInfo)
{
	_GUI_ADD_OFFSET("bool",			"enabled",		m_Enabled)
	_GUI_ADD_OFFSET("bool",			"hidden",		m_Hidden)
	_GUI_ADD_OFFSET("client area",	"size",			m_Size)
	_GUI_ADD_OFFSET("string",		"style",		m_Style)
	_GUI_ADD_OFFSET("float",		"z",			m_Z)
	_GUI_ADD_OFFSET("string",		"caption",		m_Caption)
	_GUI_ADD_OFFSET("bool",			"absolute",		m_Absolute)
}

bool IGUIObject::MouseOver()
{
	if(!GetGUI())
		throw PS_NEEDS_PGUI;

	u16 mouse_x = GetMouseX(),
		mouse_y = GetMouseY();

	return (mouse_x >= m_CachedActualSize.left &&
			mouse_x <= m_CachedActualSize.right &&
			mouse_y >= m_CachedActualSize.top &&
			mouse_y <= m_CachedActualSize.bottom);
}

u16 IGUIObject::GetMouseX() const
{ 
	return ((GetGUI())?(GetGUI()->m_MouseX):0); 
}

u16 IGUIObject::GetMouseY() const 
{ 
	return ((GetGUI())?(GetGUI()->m_MouseY):0); 
}

void IGUIObject::UpdateMouseOver(IGUIObject * const &pMouseOver)
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

bool IGUIObject::SettingExists(const CStr &Setting) const
{
	// Because GetOffsets will direct dynamically defined
	//  classes with polymorifsm to respective m_SettingsInfo
	//  we need to make no further updates on this function
	//  in derived classes.
	return (GetSettingsInfo().count(Setting) == 1)?true:false;
}

void IGUIObject::SetSetting(const CStr &Setting, const CStr &Value)
{
	if (!SettingExists(Setting))
	{
		throw PS_FAIL;
	}

	// Get setting
	SGUISetting set = GetSettingsInfo()[Setting];

	if (set.m_Type == CStr(_T("string")))
	{
		GUI<CStr>::SetSetting(this, Setting, Value);
	}
	else
	if (set.m_Type == CStr(_T("bool")))
	{
		bool bValue;

		if (Value == CStr(_T("true")))
			bValue = true;
		else
		if (Value == CStr(_T("false")))
			bValue = false;
		else 
			throw PS_FAIL;

		GUI<bool>::SetSetting(this, Setting, bValue);
	}
	else
	if (set.m_Type == CStr(_T("float")))
	{
		// GeeTODO Okay float value!?
		GUI<float>::SetSetting(this, Setting, Value.ToFloat() );
	}
	else
	if (set.m_Type == CStr(_T("rect")))
	{
		// Use the parser to parse the values
		CParser parser;
		parser.InputTaskType("", "_$value_$value_$value_$value_");

		// GeeTODO  string really?
		string str = (const TCHAR*)Value;

		CParserLine line;
		line.ParseString(parser, str);
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
	}
	else
	if (set.m_Type == CStr(_T("client area")))
	{
		// Get Client Area
		CClientArea ca;

		if (!ca.SetClientArea(Value))
		{
			// GeeTODO REPORT
		}

		// Check if valid!

		GUI<CClientArea>::SetSetting(this, Setting, ca);
	}
	else
	{
		throw PS_FAIL;
	}
}

void IGUIObject::ChooseMouseOverAndClosest(IGUIObject* &pObject)
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

IGUIObject *IGUIObject::GetParent()
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

void IGUIObject::UpdateCachedSize()
{
	// If absolute="false" and the object has got a parent,
	//  use its cached size instead of the screen. Notice
	//  it must have just been cached for it to work.
	if (m_BaseSettings.m_Absolute == false && m_pParent)
		m_CachedActualSize = m_BaseSettings.m_Size.GetClientArea( m_pParent->m_CachedActualSize );
	else
		m_CachedActualSize = m_BaseSettings.m_Size.GetClientArea( CRect(0, 0, g_xres, g_yres) );
}

// GeeTODO keep this function and all???
void IGUIObject::CheckSettingsValidity()
{
	// If we hide an object, reset many of its parts
	if (GetBaseSettings().m_Hidden)
	{
		// Simulate that no object is hovered for this object and all its children
		//  why? because it's 
		try
		{
			GUI<IGUIObject*>::RecurseObject(0, this, &IGUIObject::UpdateMouseOver, NULL);
		}
		catch (...) {}
	}

	try
	{
		// Send message to itself
		HandleMessage(GUIM_SETTINGS_UPDATED);
	}
	catch (...)
	{
	}
}
