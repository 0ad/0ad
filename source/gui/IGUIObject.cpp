/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "GUI.h"

#include "gui/scripting/JSInterface_GUITypes.h"
#include "gui/scripting/JSInterface_IGUIObject.h"

#include "ps/GameSetup/Config.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

template<typename T>
void SGUISetting::Init(IGUIObject& pObject, const CStr& Name)
{
	m_pSetting = new T();

	m_FromJSVal = [Name, &pObject](JSContext* cx, JS::HandleValue v) {
		T value;
		if (!ScriptInterface::FromJSVal<T>(cx, v, value))
			return false;

		GUI<T>::SetSetting(&pObject, Name, value);
		return true;
	};

	m_ToJSVal = [Name, this](JSContext* cx, JS::MutableHandleValue v) {
		ScriptInterface::ToJSVal<T>(cx, v, *static_cast<T*>(m_pSetting));
	};
}

IGUIObject::IGUIObject()
	: m_pGUI(NULL), m_pParent(NULL), m_MouseHovering(false), m_LastClickTime()
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
}

IGUIObject::~IGUIObject()
{
	for (const std::pair<CStr, SGUISetting>& p : m_Settings)
		switch (p.second.m_Type)
		{
			// delete() needs to know the type of the variable - never delete a void*
#define TYPE(t) case GUIST_##t: delete (t*)p.second.m_pSetting; break;
#include "GUItypes.h"
#undef TYPE
		default:
			debug_warn(L"Invalid setting type");
		}

	if (m_pGUI)
		JS_RemoveExtraGCRootsTracer(m_pGUI->GetScriptInterface()->GetJSRuntime(), Trace, this);
}

//-------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------
void IGUIObject::SetGUI(CGUI* const& pGUI)
{
	if (!m_pGUI)
		JS_AddExtraGCRootsTracer(pGUI->GetScriptInterface()->GetJSRuntime(), Trace, this);
	m_pGUI = pGUI;
}

void IGUIObject::AddChild(IGUIObject* pChild)
{
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
			m_Children.erase(m_Children.end()-1);

			throw;
		}
	}
	// else do nothing
}

void IGUIObject::AddToPointersMap(map_pObjects& ObjectMap)
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

void IGUIObject::AddSetting(const EGUISettingType& Type, const CStr& Name)
{
	// Is name already taken?
	if (m_Settings.count(Name) >= 1)
		return;

	// Construct, and set type
	m_Settings[Name].m_Type = Type;

	switch (Type)
	{
#define TYPE(type) \
	case GUIST_##type: \
		m_Settings[Name].Init<type>(*this, Name);\
		break;

		// Construct the setting.
		#include "GUItypes.h"

#undef TYPE

	default:
		debug_warn(L"IGUIObject::AddSetting failed, type not recognized!");
		break;
	}
}


bool IGUIObject::MouseOver()
{
	if (!GetGUI())
		throw PSERROR_GUI_OperationNeedsGUIObject();

	return m_CachedActualSize.PointInside(GetMousePos());
}

bool IGUIObject::MouseOverIcon()
{
	return false;
}

CPos IGUIObject::GetMousePos() const
{
	if (GetGUI())
		return GetGUI()->m_MousePos;

	return CPos();
}

void IGUIObject::UpdateMouseOver(IGUIObject* const& pMouseOver)
{
	if (pMouseOver == this)
	{
		if (!m_MouseHovering)
			SendEvent(GUIM_MOUSE_ENTER, "mouseenter");

		m_MouseHovering = true;

		SendEvent(GUIM_MOUSE_OVER, "mousemove");
	}
	else
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

PSRETURN IGUIObject::SetSetting(const CStr& Setting, const CStrW& Value, const bool& SkipMessage)
{
	if (!SettingExists(Setting))
		return PSRETURN_GUI_InvalidSetting;

	SGUISetting set = m_Settings[Setting];

#define TYPE(type) \
	else if (set.m_Type == GUIST_##type) \
	{ \
		type _Value; \
		if (!GUI<type>::ParseString(Value, _Value)) \
			return PSRETURN_GUI_UnableToParse; \
		GUI<type>::SetSetting(this, Setting, _Value, SkipMessage); \
	}

	if (0)
		;
#include "GUItypes.h"
#undef TYPE
	else
	{
		// Why does it always fail?
		//return PS_FAIL;
		return LogInvalidSettings(Setting);
	}
	return PSRETURN_OK;
}



PSRETURN IGUIObject::GetSettingType(const CStr& Setting, EGUISettingType& Type) const
{
	if (!SettingExists(Setting))
		return LogInvalidSettings(Setting);

	if (m_Settings.find(Setting) == m_Settings.end())
		return LogInvalidSettings(Setting);

	Type = m_Settings.find(Setting)->second.m_Type;

	return PSRETURN_OK;
}


void IGUIObject::ChooseMouseOverAndClosest(IGUIObject*& pObject)
{
	if (!MouseOver())
		return;

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

IGUIObject* IGUIObject::GetParent() const
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

void IGUIObject::ResetStates()
{
	// Notify the gui that we aren't hovered anymore
	UpdateMouseOver(nullptr);
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
		m_CachedActualSize = ca.GetClientArea(CRect(0.f, 0.f, g_xres / g_GuiScale, g_yres / g_GuiScale));

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

void IGUIObject::LoadStyle(CGUI& GUIinstance, const CStr& StyleName)
{
	// Fetch style
	if (GUIinstance.m_Styles.count(StyleName) == 1)
	{
		LoadStyle(GUIinstance.m_Styles[StyleName]);
	}
	else
	{
		debug_warn(L"IGUIObject::LoadStyle failed");
	}
}

void IGUIObject::LoadStyle(const SGUIStyle& Style)
{
	// Iterate settings, it won't be able to set them all probably, but that doesn't matter
	for (const std::pair<CStr, CStrW>& p : Style.m_SettingsDefaults)
	{
		// Try set setting in object
		SetSetting(p.first, p.second);

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
	JS::RootedValue globalVal(cx, pGUI->GetGlobalObject());
	JS::RootedObject globalObj(cx, &globalVal.toObject());

	const int paramCount = 1;
	const char* paramNames[paramCount] = { "mouse" };

	// Location to report errors from
	CStr CodeName = GetName()+" "+Action;

	// Generate a unique name
	static int x = 0;
	char buf[64];
	sprintf_s(buf, ARRAY_SIZE(buf), "__eventhandler%d (%s)", x++, Action.c_str());

	JS::CompileOptions options(cx);
	options.setFileAndLine(CodeName.c_str(), 0);
	options.setCompileAndGo(true);

	JS::RootedFunction func(cx);
	JS::AutoObjectVector emptyScopeChain(cx);
	if (!JS::CompileFunction(cx, emptyScopeChain, options, buf, paramCount, paramNames, Code.c_str(), Code.length(), &func))
	{
		LOGERROR("RegisterScriptHandler: Failed to compile the script for %s", Action.c_str());
		return;
	}

	JS::RootedObject funcObj(cx, JS_GetFunctionObject(func));
	SetScriptHandler(Action, funcObj);
}

void IGUIObject::SetScriptHandler(const CStr& Action, JS::HandleObject Function)
{
	// m_ScriptHandlers is only rooted after SetGUI() has been called (which sets up the GC trace callbacks),
	// so we can't safely store objects in it if the GUI hasn't been set yet.
	ENSURE(m_pGUI && "A GUI must be associated with the GUIObject before adding ScriptHandlers!");
	m_ScriptHandlers[Action] = JS::Heap<JSObject*>(Function);
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
	std::map<CStr, JS::Heap<JSObject*> >::iterator it = m_ScriptHandlers.find(Action);
	if (it == m_ScriptHandlers.end())
		return;

	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);

	// Set up the 'mouse' parameter
	JS::RootedValue mouse(cx);

	m_pGUI->GetScriptInterface()->CreateObject(
		&mouse,
		"x", m_pGUI->m_MousePos.x,
		"y", m_pGUI->m_MousePos.y,
		"buttons", m_pGUI->m_MouseButtons);

	JS::AutoValueVector paramData(cx);
	paramData.append(mouse);
	JS::RootedObject obj(cx, GetJSObject());
	JS::RootedValue handlerVal(cx, JS::ObjectValue(*it->second));
	JS::RootedValue result(cx);
	bool ok = JS_CallFunctionValue(cx, obj, handlerVal, paramData, &result);
	if (!ok)
	{
		// We have no way to propagate the script exception, so just ignore it
		// and hope the caller checks JS_IsExceptionPending
	}
}

void IGUIObject::ScriptEvent(const CStr& Action, JS::HandleValueArray paramData)
{
	std::map<CStr, JS::Heap<JSObject*> >::iterator it = m_ScriptHandlers.find(Action);
	if (it == m_ScriptHandlers.end())
		return;

	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, GetJSObject());
	JS::RootedValue handlerVal(cx, JS::ObjectValue(*it->second));
	JS::RootedValue result(cx);

	if (!JS_CallFunctionValue(cx, obj, handlerVal, paramData, &result))
		JS_ReportError(cx, "Errors executing script action \"%s\"", Action.c_str());
}

JSObject* IGUIObject::GetJSObject()
{
	JSContext* cx = m_pGUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	// Cache the object when somebody first asks for it, because otherwise
	// we end up doing far too much object allocation. TODO: Would be nice to
	// not have these objects hang around forever using up memory, though.
	if (!m_JSObject.initialized())
	{
		m_JSObject.init(cx, m_pGUI->GetScriptInterface()->CreateCustomObject("GUIObject"));
		JS_SetPrivate(m_JSObject.get(), this);
	}
	return m_JSObject.get();
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
	return GetGUI() != 0 && m_pParent == GetGUI()->m_BaseObject;
}

void IGUIObject::TraceMember(JSTracer* trc)
{
	for (std::pair<const CStr, JS::Heap<JSObject*>>& handler : m_ScriptHandlers)
		JS_CallObjectTracer(trc, &handler.second, "IGUIObject::m_ScriptHandlers");
}

PSRETURN IGUIObject::LogInvalidSettings(const CStr8& Setting) const
{
	LOGWARNING("IGUIObject: setting %s was not found on an object", Setting.c_str());
	return PSRETURN_GUI_InvalidSetting;
}
