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
#include "soundmanager/ISoundManager.h"

IGUIObject::IGUIObject(CGUI& pGUI)
	: m_pGUI(pGUI), m_pParent(NULL), m_MouseHovering(false), m_LastClickTime()
{
	AddSetting<bool>("enabled");
	AddSetting<bool>("hidden");
	AddSetting<CClientArea>("size");
	AddSetting<CStr>("style");
	AddSetting<CStr>("hotkey");
	AddSetting<float>("z");
	AddSetting<bool>("absolute");
	AddSetting<bool>("ghost");
	AddSetting<float>("aspectratio");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");

	// Setup important defaults
	GUI<bool>::SetSetting(this, "hidden", false);
	GUI<bool>::SetSetting(this, "ghost", false);
	GUI<bool>::SetSetting(this, "enabled", true);
	GUI<bool>::SetSetting(this, "absolute", true);
}

IGUIObject::~IGUIObject()
{
	for (const std::pair<CStr, IGUISetting*>& p : m_Settings)
		delete p.second;

	if (!m_ScriptHandlers.empty())
		JS_RemoveExtraGCRootsTracer(m_pGUI.GetScriptInterface()->GetJSRuntime(), Trace, this);
}

//-------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------

void IGUIObject::AddChild(IGUIObject* pChild)
{
//	ENSURE(pChild);

	pChild->SetParent(this);

	m_Children.push_back(pChild);

	{
		try
		{
			// Atomic function, if it fails it won't
			//  have changed anything
			//UpdateObjects();
			pChild->GetGUI().UpdateObjects();
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

template<typename T>
void IGUIObject::AddSetting(const CStr& Name)
{
	// This can happen due to inheritance
	if (SettingExists(Name))
		return;

	m_Settings[Name] = new CGUISetting<T>(*this, Name);
}

bool IGUIObject::IsMouseOver() const
{
	return m_CachedActualSize.PointInside(m_pGUI.GetMousePos());
}

bool IGUIObject::MouseOverIcon()
{
	return false;
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
	return m_Settings.count(Setting) == 1;
}

PSRETURN IGUIObject::SetSetting(const CStr& Setting, const CStrW& Value, const bool& SkipMessage)
{
	if (!SettingExists(Setting))
		return PSRETURN_GUI_InvalidSetting;

	if (!m_Settings[Setting]->FromString(Value, SkipMessage))
		return PSRETURN_GUI_UnableToParse;

	return PSRETURN_OK;
}

void IGUIObject::ChooseMouseOverAndClosest(IGUIObject*& pObject)
{
	if (!IsMouseOver())
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
	const CClientArea& ca = GUI<CClientArea>::GetSetting(this, "size");
	const float aspectratio = GUI<float>::GetSetting(this, "aspectratio");

	// If absolute="false" and the object has got a parent,
	//  use its cached size instead of the screen. Notice
	//  it must have just been cached for it to work.
	if (!GUI<bool>::GetSetting(this, "absolute") && m_pParent && !IsRootObject())
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

void IGUIObject::LoadStyle(CGUI& pGUI, const CStr& StyleName)
{
	if (pGUI.HasStyle(StyleName))
		LoadStyle(pGUI.GetStyle(StyleName));
	else
		debug_warn(L"IGUIObject::LoadStyle failed");
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
	const float Z = GUI<float>::GetSetting(this, "z");

	if (GUI<bool>::GetSetting(this, "absolute"))
		return Z;

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

void IGUIObject::RegisterScriptHandler(const CStr& Action, const CStr& Code, CGUI& pGUI)
{
	JSContext* cx = pGUI.GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue globalVal(cx, pGUI.GetGlobalObject());
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
	options.setIsRunOnce(false);

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
	if (m_ScriptHandlers.empty())
		JS_AddExtraGCRootsTracer(m_pGUI.GetScriptInterface()->GetJSRuntime(), Trace, this);

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

	JSContext* cx = m_pGUI.GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);

	// Set up the 'mouse' parameter
	JS::RootedValue mouse(cx);

	const CPos& mousePos = m_pGUI.GetMousePos();

	m_pGUI.GetScriptInterface()->CreateObject(
		&mouse,
		"x", mousePos.x,
		"y", mousePos.y,
		"buttons", m_pGUI.GetMouseButtons());

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

void IGUIObject::ScriptEvent(const CStr& Action, const JS::HandleValueArray& paramData)
{
	std::map<CStr, JS::Heap<JSObject*> >::iterator it = m_ScriptHandlers.find(Action);
	if (it == m_ScriptHandlers.end())
		return;

	JSContext* cx = m_pGUI.GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, GetJSObject());
	JS::RootedValue handlerVal(cx, JS::ObjectValue(*it->second));
	JS::RootedValue result(cx);

	if (!JS_CallFunctionValue(cx, obj, handlerVal, paramData, &result))
		JS_ReportError(cx, "Errors executing script action \"%s\"", Action.c_str());
}

void IGUIObject::CreateJSObject()
{
	JSContext* cx = m_pGUI.GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);

	m_JSObject.init(cx, m_pGUI.GetScriptInterface()->CreateCustomObject("GUIObject"));
	JS_SetPrivate(m_JSObject.get(), this);
}

JSObject* IGUIObject::GetJSObject()
{
	// Cache the object when somebody first asks for it, because otherwise
	// we end up doing far too much object allocation.
	if (!m_JSObject.initialized())
		CreateJSObject();

	return m_JSObject.get();
}

bool IGUIObject::IsHidden() const
{
	// Statically initialise some strings, so we don't have to do
	// lots of allocation every time this function is called
	static const CStr strHidden("hidden");
	return GUI<bool>::GetSetting(this, strHidden);
}

bool IGUIObject::IsHiddenOrGhost() const
{
	static const CStr strGhost("ghost");
	return IsHidden() || GUI<bool>::GetSetting(this, strGhost);
}

void IGUIObject::PlaySound(const CStr& settingName) const
{
	if (!g_SoundManager)
		return;

	const CStrW& soundPath = GUI<CStrW>::GetSetting(this, settingName);

	if (!soundPath.empty())
		g_SoundManager->PlayAsUI(soundPath.c_str(), false);
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
	m_pGUI.SetFocusedObject(this);
}

bool IGUIObject::IsFocused() const
{
	return m_pGUI.GetFocusedObject() == this;
}

bool IGUIObject::IsRootObject() const
{
	return m_pParent == m_pGUI.GetBaseObject();
}

void IGUIObject::TraceMember(JSTracer* trc)
{
	// Please ensure to adapt the Tracer enabling and disabling in accordance with the GC things traced!

	for (std::pair<const CStr, JS::Heap<JSObject*>>& handler : m_ScriptHandlers)
		JS_CallObjectTracer(trc, &handler.second, "IGUIObject::m_ScriptHandlers");
}

// Instantiate templated functions:
#define TYPE(T) template void IGUIObject::AddSetting<T>(const CStr& Name);
#include "GUItypes.h"
#undef TYPE
