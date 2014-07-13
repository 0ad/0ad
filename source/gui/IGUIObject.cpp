/* Copyright (C) 2012 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
IGUIObject
*/

#include "precompiled.h"
#include "GUI.h"

#include "gui/scripting/JSInterface_IGUIObject.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "scriptinterface/ScriptInterface.h"

#include "ps/CLogger.h"

extern int g_xres, g_yres;


//-------------------------------------------------------------------
//  Implementation Macros
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIObject::IGUIObject() : 
	m_pGUI(NULL), 
	m_pParent(NULL),
	m_MouseHovering(false)
{
	AddSetting(GUIST_bool,			"enabled");
	AddSetting(GUIST_bool,			"hidden");
	AddSetting(GUIST_CClientArea,	"size");
	AddSetting(GUIST_CStr,			"style");
	AddSetting(GUIST_CStr,			"hotkey");
	AddSetting(GUIST_float,			"z");
	AddSetting(GUIST_bool,			"absolute");
	AddSetting(GUIST_bool,			"ghost");
	AddSetting(GUIST_float,			"aspectratio");
	AddSetting(GUIST_CStrW,			"tooltip");
	AddSetting(GUIST_CStr,			"tooltip_style");

	// Setup important defaults
	GUI<bool>::SetSetting(this, "hidden", false);
	GUI<bool>::SetSetting(this, "ghost", false);
	GUI<bool>::SetSetting(this, "enabled", true);
	GUI<bool>::SetSetting(this, "absolute", true);


	for (int i=0; i<6; i++)
		m_LastClickTime[i]=0;
}

IGUIObject::~IGUIObject()
{
	{
		std::map<CStr, SGUISetting>::iterator it;
		for (it = m_Settings.begin(); it != m_Settings.end(); ++it)
		{
			switch (it->second.m_Type)
			{
				// delete() needs to know the type of the variable - never delete a void*
#define TYPE(t) case GUIST_##t: delete (t*)it->second.m_pSetting; break;
#include "GUItypes.h"
#undef TYPE
		default:
			debug_warn(L"Invalid setting type");
			}
		}
	}
}

//-------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------
void IGUIObject::AddChild(IGUIObject *pChild)
{
	// 
//	ENSURE(pChild);

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
		catch (PSERROR_GUI&)
		{
			// If anything went wrong, reverse what we did and throw
			//  an exception telling it never added a child
			m_Children.erase( m_Children.end()-1 );
			
			throw;
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
	if (m_Name.empty())
	{
		throw PSERROR_GUI_ObjectNeedsName();
	}
	if (ObjectMap.count(m_Name) > 0)
	{
		throw PSERROR_GUI_NameAmbiguity(m_Name.c_str());
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

// Notice if using this, the naming convention of GUIST_ should be strict.
#define TYPE(type)									\
	case GUIST_##type:								\
		m_Settings[Name].m_pSetting = new type();	\
		break;

void IGUIObject::AddSetting(const EGUISettingType &Type, const CStr& Name)
{
	// Is name already taken?
	if (m_Settings.count(Name) >= 1)
		return;

	// Construct, and set type
	m_Settings[Name].m_Type = Type;

	switch (Type)
	{
		// Construct the setting.
		#include "GUItypes.h"

	default:
		debug_warn(L"IGUIObject::AddSetting failed, type not recognized!");
		break;
	}
}
#undef TYPE


bool IGUIObject::MouseOver()
{
	if(!GetGUI())
		throw PSERROR_GUI_OperationNeedsGUIObject();

	return m_CachedActualSize.PointInside(GetMousePos());
}

bool IGUIObject::MouseOverIcon()
{
	return false;
}

CPos IGUIObject::GetMousePos() const
{ 
	return ((GetGUI())?(GetGUI()->m_MousePos):CPos()); 
}

void IGUIObject::UpdateMouseOver(IGUIObject * const &pMouseOver)
{
	// Check if this is the object being hovered.
	if (pMouseOver == this)
	{
		if (!m_MouseHovering)
		{
			// It wasn't hovering, so that must mean it just entered
			SendEvent(GUIM_MOUSE_ENTER, "mouseenter");
		}

		// Either way, set to true
		m_MouseHovering = true;

		// call mouse over
		SendEvent(GUIM_MOUSE_OVER, "mousemove");
	}
	else // Some other object (or none) is hovered
	{
		if (m_MouseHovering)
		{
			m_MouseHovering = false;
			SendEvent(GUIM_MOUSE_LEAVE, "mouseleave");
		}
	}
}

bool IGUIObject::SettingExists(const CStr& Setting) const
{
	// Because GetOffsets will direct dynamically defined
	//  classes with polymorphism to respective ms_SettingsInfo
	//  we need to make no further updates on this function
	//  in derived classes.
	//return (GetSettingsInfo().count(Setting) >= 1);
	return (m_Settings.count(Setting) >= 1);
}

#define TYPE(type)										\
	else												\
	if (set.m_Type == GUIST_##type)						\
	{													\
		type _Value;									\
		if (!GUI<type>::ParseString(Value, _Value))		\
			return PSRETURN_GUI_UnableToParse;			\
														\
		GUI<type>::SetSetting(this, Setting, _Value, SkipMessage);	\
	}

PSRETURN IGUIObject::SetSetting(const CStr& Setting, const CStrW& Value, const bool& SkipMessage)
{
	if (!SettingExists(Setting))
	{
		return PSRETURN_GUI_InvalidSetting;
	}

	// Get setting
	SGUISetting set = m_Settings[Setting];

	if (0);
	// else...
#include "GUItypes.h"
	else
	{
		// Why does it always fail?
		//return PS_FAIL;
		return LogInvalidSettings(Setting);
	}
	return PSRETURN_OK;
}

#undef TYPE


PSRETURN IGUIObject::GetSettingType(const CStr& Setting, EGUISettingType &Type) const
{
	if (!SettingExists(Setting))
	{
		return LogInvalidSettings(Setting);
	}

	if (m_Settings.find(Setting) == m_Settings.end())
	{
		return LogInvalidSettings(Setting);
	}

	Type = m_Settings.find(Setting)->second.m_Type;

	return PSRETURN_OK;
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
		if (GetBufferedZ() >= pObject->GetBufferedZ())
		{
			pObject = this;
			return;
		}
	}
}

IGUIObject *IGUIObject::GetParent() const
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
	bool absolute;
	GUI<bool>::GetSetting(this, "absolute", absolute);

	float aspectratio = 0.f;
	GUI<float>::GetSetting(this, "aspectratio", aspectratio);

	CClientArea ca;
	GUI<CClientArea>::GetSetting(this, "size", ca);
	
	// If absolute="false" and the object has got a parent,
	//  use its cached size instead of the screen. Notice
	//  it must have just been cached for it to work.
	if (absolute == false && m_pParent && !IsRootObject())
		m_CachedActualSize = ca.GetClientArea(m_pParent->m_CachedActualSize);
	else
		m_CachedActualSize = ca.GetClientArea(CRect(0.f, 0.f, (float)g_xres, (float)g_yres));

	// In a few cases, GUI objects have to resize to fill the screen
	// but maintain a constant aspect ratio.
	// Adjust the size to be the max possible, centered in the original size:
	if (aspectratio)
	{
		if (m_CachedActualSize.GetWidth() > m_CachedActualSize.GetHeight()*aspectratio)
		{
			float delta = m_CachedActualSize.GetWidth() - m_CachedActualSize.GetHeight()*aspectratio;
			m_CachedActualSize.left += delta/2.f;
			m_CachedActualSize.right -= delta/2.f;
		}
		else
		{
			float delta = m_CachedActualSize.GetHeight() - m_CachedActualSize.GetWidth()/aspectratio;
			m_CachedActualSize.bottom -= delta/2.f;
			m_CachedActualSize.top += delta/2.f;
		}
	}
}

void IGUIObject::LoadStyle(CGUI &GUIinstance, const CStr& StyleName)
{
	// Fetch style
	if (GUIinstance.m_Styles.count(StyleName)==1)
	{
		LoadStyle(GUIinstance.m_Styles[StyleName]);
	}
	else
	{
		debug_warn(L"IGUIObject::LoadStyle failed");
	}
}

void IGUIObject::LoadStyle(const SGUIStyle &Style)
{
	// Iterate settings, it won't be able to set them all probably, but that doesn't matter
	std::map<CStr, CStrW>::const_iterator cit;
	for (cit = Style.m_SettingsDefaults.begin(); cit != Style.m_SettingsDefaults.end(); ++cit)
	{
		// Try set setting in object
		SetSetting(cit->first, cit->second);

		// It doesn't matter if it fail, it's not suppose to be able to set every setting.
		//  since it's generic.

		// The beauty with styles is that it can contain more settings
		//  than exists for the objects using it. So if the SetSetting
		//  fails, don't care.
	}
}

float IGUIObject::GetBufferedZ() const
{
	bool absolute;
	GUI<bool>::GetSetting(this, "absolute", absolute);

	float Z;
	GUI<float>::GetSetting(this, "z", Z);

	if (absolute)
		return Z;
	else
	{
		if (GetParent())
			return GetParent()->GetBufferedZ() + Z;
		else
		{
			// In philosophy, a parentless object shouldn't be able to have a relative sizing,
			//  but we'll accept it so that absolute can be used as default without a complaint. 
			//  Also, you could consider those objects children to the screen resolution.
			return Z;
		}
	}
}

void IGUIObject::RegisterScriptHandler(const CStr& Action, const CStr& Code, CGUI* pGUI)
{
	if(!GetGUI())
		throw PSERROR_GUI_OperationNeedsGUIObject();
		
	JSContext* cx = pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	
	const int paramCount = 1;
	const char* paramNames[paramCount] = { "mouse" };

	// Location to report errors from
	CStr CodeName = GetName()+" "+Action;

	// Generate a unique name
	static int x=0;
	char buf[64];
	sprintf_s(buf, ARRAY_SIZE(buf), "__eventhandler%d (%s)", x++, Action.c_str());

	JSFunction* func = JS_CompileFunction(cx, JSVAL_TO_OBJECT(pGUI->GetGlobalObject()),
		buf, paramCount, paramNames, Code.c_str(), Code.length(), CodeName.c_str(), 0);

	if (!func)
		return; // JS will report an error message

	SetScriptHandler(Action, JS_GetFunctionObject(func));
}

void IGUIObject::SetScriptHandler(const CStr& Action, JSObject* Function)
{
	m_ScriptHandlers[Action] = CScriptValRooted(m_pGUI->GetScriptInterface()->GetContext(), JS::ObjectValue(*Function));
}

InReaction IGUIObject::SendEvent(EGUIMessageType type, const CStr& EventName)
{
	PROFILE2_EVENT("gui event");
	PROFILE2_ATTR("type: %s", EventName.c_str());
	PROFILE2_ATTR("object: %s", m_Name.c_str());

	SGUIMessage msg(type);
	HandleMessage(msg);

	ScriptEvent(EventName);

	return (msg.skipped ? IN_PASS : IN_HANDLED);
}

void IGUIObject::ScriptEvent(const CStr& Action)
{
	std::map<CStr, CScriptValRooted>::iterator it = m_ScriptHandlers.find(Action);
	if (it == m_ScriptHandlers.end())
		return;

	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);

	// Set up the 'mouse' parameter
	CScriptVal mouse;
	m_pGUI->GetScriptInterface()->Eval("({})", mouse);
	m_pGUI->GetScriptInterface()->SetProperty(mouse.get(), "x", m_pGUI->m_MousePos.x, false);
	m_pGUI->GetScriptInterface()->SetProperty(mouse.get(), "y", m_pGUI->m_MousePos.y, false);
	m_pGUI->GetScriptInterface()->SetProperty(mouse.get(), "buttons", m_pGUI->m_MouseButtons, false);

	jsval paramData[] = { mouse.get() };

	jsval result;
	bool ok = JS_CallFunctionValue(cx, GetJSObject(), (*it).second.get(), ARRAY_SIZE(paramData), paramData, &result);
	if (!ok)
	{
		// We have no way to propagate the script exception, so just ignore it
		// and hope the caller checks JS_IsExceptionPending
	}
}

void IGUIObject::ScriptEvent(const CStr& Action, const CScriptValRooted& Argument)
{
	std::map<CStr, CScriptValRooted>::iterator it = m_ScriptHandlers.find(Action);
	if (it == m_ScriptHandlers.end())
		return;

	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);

	JSObject* object = GetJSObject();

	jsval arg = Argument.get();

	jsval result;
	bool ok = JS_CallFunctionValue(cx, object, (*it).second.get(), 1, &arg, &result);
	if (!ok)
	{
		JS_ReportError(cx, "Errors executing script action \"%s\"", Action.c_str());
	}
}

JSObject* IGUIObject::GetJSObject()
{
	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	// Cache the object when somebody first asks for it, because otherwise
	// we end up doing far too much object allocation. TODO: Would be nice to
	// not have these objects hang around forever using up memory, though.
	if (m_JSObject.uninitialised())
	{
		JS::RootedObject obj(cx, m_pGUI->GetScriptInterface()->CreateCustomObject("GUIObject"));
		m_JSObject = CScriptValRooted(cx, JS::ObjectValue(*obj));
		JS_SetPrivate(obj, this);
	}
	return &m_JSObject.get().toObject();
}

CStr IGUIObject::GetPresentableName() const
{
	// __internal(), must be at least 13 letters to be able to be
	//  an internal name
	if (m_Name.length() <= 12)
		return m_Name;

	if (m_Name.substr(0, 10) == "__internal")
		return CStr("[unnamed object]");
	else
		return m_Name;
}

void IGUIObject::SetFocus()
{
	GetGUI()->m_FocusedObject = this; 
}

bool IGUIObject::IsFocused() const
{
	return GetGUI()->m_FocusedObject == this; 
}

bool IGUIObject::IsRootObject() const
{
	return (GetGUI() != 0 && m_pParent == GetGUI()->m_BaseObject);
}

PSRETURN IGUIObject::LogInvalidSettings(const CStr8 &Setting) const
{
	LOGWARNING(L"IGUIObject: setting %hs was not found on an object", 
		Setting.c_str());
	return PSRETURN_GUI_InvalidSetting;
}
