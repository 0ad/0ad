/* Copyright (C) 2021 Wildfire Games.
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

#include "CGUI.h"

#include "graphics/Canvas2D.h"
#include "gui/IGUIScrollBar.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectTypes/CGUIDummyObject.h"
#include "gui/ObjectTypes/CTooltip.h"
#include "gui/Scripting/ScriptFunctions.h"
#include "gui/Scripting/JSInterface_GUIProxy.h"
#include "i18n/L10n.h"
#include "lib/allocators/DynamicArena.h"
#include "lib/allocators/STLAllocators.h"
#include "lib/bits.h"
#include "lib/input.h"
#include "lib/sysdep/sysdep.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/Size2D.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptContext.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

const double SELECT_DBLCLICK_RATE = 0.5;
const u32 MAX_OBJECT_DEPTH = 100; // Max number of nesting for GUI includes. Used to detect recursive inclusion

const CStr CGUI::EventNameLoad = "Load";
const CStr CGUI::EventNameTick = "Tick";
const CStr CGUI::EventNamePress = "Press";
const CStr CGUI::EventNameKeyDown = "KeyDown";
const CStr CGUI::EventNameRelease = "Release";
const CStr CGUI::EventNameMouseRightPress = "MouseRightPress";
const CStr CGUI::EventNameMouseLeftPress = "MouseLeftPress";
const CStr CGUI::EventNameMouseWheelDown = "MouseWheelDown";
const CStr CGUI::EventNameMouseWheelUp = "MouseWheelUp";
const CStr CGUI::EventNameMouseLeftDoubleClick = "MouseLeftDoubleClick";
const CStr CGUI::EventNameMouseLeftRelease = "MouseLeftRelease";
const CStr CGUI::EventNameMouseRightDoubleClick = "MouseRightDoubleClick";
const CStr CGUI::EventNameMouseRightRelease = "MouseRightRelease";

namespace
{

struct VisibleObject
{
	IGUIObject* object;
	// Index of the object in a depth-first search inside GUI tree.
	size_t index;
	// Cached value of GetBufferedZ to avoid recursive calls in a deep hierarchy.
	float bufferedZ;
};

template<class Container>
void CollectVisibleObjectsRecursively(const std::vector<IGUIObject*>& objects, Container* visibleObjects)
{
	for (IGUIObject* const& object : objects)
	{
		if (!object->IsHidden())
		{
			visibleObjects->emplace_back(VisibleObject{object, visibleObjects->size(), 0.0f});
			CollectVisibleObjectsRecursively(object->GetChildren(), visibleObjects);
		}
	}
}

} // anonynous namespace

CGUI::CGUI(const std::shared_ptr<ScriptContext>& context)
	: m_BaseObject(std::make_unique<CGUIDummyObject>(*this)),
	  m_FocusedObject(nullptr),
	  m_InternalNameNumber(0),
	  m_MouseButtons(0)
{
	m_ScriptInterface = std::make_shared<ScriptInterface>("Engine", "GUIPage", context);
	m_ScriptInterface->SetCallbackData(this);

	GuiScriptingInit(*m_ScriptInterface);
	m_ScriptInterface->LoadGlobalScripts();
}

CGUI::~CGUI()
{
	for (const std::pair<const CStr, IGUIObject*>& p : m_pAllObjects)
		delete p.second;

	for (const std::pair<const CStr, const CGUISprite*>& p : m_Sprites)
		delete p.second;
}

InReaction CGUI::HandleEvent(const SDL_Event_* ev)
{
	InReaction ret = IN_PASS;

	if (ev->ev.type == SDL_HOTKEYDOWN || ev->ev.type == SDL_HOTKEYPRESS || ev->ev.type == SDL_HOTKEYUP)
	{
		const char* hotkey = static_cast<const char*>(ev->ev.user.data1);

		const CStr& eventName = ev->ev.type == SDL_HOTKEYPRESS ? EventNamePress : ev->ev.type == SDL_HOTKEYDOWN ? EventNameKeyDown : EventNameRelease;

		if (m_GlobalHotkeys.find(hotkey) != m_GlobalHotkeys.end() && m_GlobalHotkeys[hotkey].find(eventName) != m_GlobalHotkeys[hotkey].end())
		{
			ret = IN_HANDLED;

			ScriptRequest rq(m_ScriptInterface);
			JS::RootedObject globalObj(rq.cx, rq.glob);
			JS::RootedValue result(rq.cx);
			if (!JS_CallFunctionValue(rq.cx, globalObj, m_GlobalHotkeys[hotkey][eventName], JS::HandleValueArray::empty(), &result))
				ScriptException::CatchPending(rq);
		}

		std::map<CStr, std::vector<IGUIObject*> >::iterator it = m_HotkeyObjects.find(hotkey);
		if (it != m_HotkeyObjects.end())
			for (IGUIObject* const& obj : it->second)
			{
				if (!obj->IsEnabled())
					continue;
				if (ev->ev.type == SDL_HOTKEYPRESS)
					ret = obj->SendEvent(GUIM_PRESSED, EventNamePress);
				else if (ev->ev.type == SDL_HOTKEYDOWN)
					ret = obj->SendEvent(GUIM_KEYDOWN, EventNameKeyDown);
				else
					ret = obj->SendEvent(GUIM_RELEASED, EventNameRelease);
			}
	}

	else if (ev->ev.type == SDL_MOUSEMOTION)
	{
		// Yes the mouse position is stored as float to avoid
		//  constant conversions when operating in a
		//  float-based environment.
		m_MousePos = CVector2D((float)ev->ev.motion.x / g_GuiScale, (float)ev->ev.motion.y / g_GuiScale);

		SGUIMessage msg(GUIM_MOUSE_MOTION);
		m_BaseObject->RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::HandleMessage, msg);
	}

	// Update m_MouseButtons. (BUTTONUP is handled later.)
	else if (ev->ev.type == SDL_MOUSEBUTTONDOWN)
	{
		switch (ev->ev.button.button)
		{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		case SDL_BUTTON_MIDDLE:
			m_MouseButtons |= Bit<unsigned int>(ev->ev.button.button);
			break;
		default:
			break;
		}
	}

	// Update m_MousePos (for delayed mouse button events)
	CVector2D oldMousePos = m_MousePos;
	if (ev->ev.type == SDL_MOUSEBUTTONDOWN || ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		m_MousePos = CVector2D((float)ev->ev.button.x / g_GuiScale, (float)ev->ev.button.y / g_GuiScale);
	}

	// Allow the focused object to pre-empt regular GUI events.
	if (GetFocusedObject())
		ret = GetFocusedObject()->PreemptEvent(ev);

	// Only one object can be hovered
	// pNearest will after this point at the hovered object, possibly nullptr
	IGUIObject* pNearest = FindObjectUnderMouse();

	if (ret == IN_PASS)
	{
		// Now we'll call UpdateMouseOver on *all* objects,
		// we'll input the one hovered, and they will each
		// update their own data and send messages accordingly
		m_BaseObject->RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::UpdateMouseOver, static_cast<IGUIObject* const&>(pNearest));

		if (ev->ev.type == SDL_MOUSEBUTTONDOWN)
		{
			switch (ev->ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				// Focus the clicked object (or focus none if nothing clicked on)
				SetFocusedObject(pNearest);

				if (pNearest)
					ret = pNearest->SendMouseEvent(GUIM_MOUSE_PRESS_LEFT, EventNameMouseLeftPress);
				break;

			case SDL_BUTTON_RIGHT:
				if (pNearest)
					ret = pNearest->SendMouseEvent(GUIM_MOUSE_PRESS_RIGHT, EventNameMouseRightPress);
				break;

			default:
				break;
			}
		}
		else if (ev->ev.type == SDL_MOUSEWHEEL && pNearest)
		{
			if (ev->ev.wheel.y < 0)
				ret = pNearest->SendMouseEvent(GUIM_MOUSE_WHEEL_DOWN, EventNameMouseWheelDown);
			else if (ev->ev.wheel.y > 0)
				ret = pNearest->SendMouseEvent(GUIM_MOUSE_WHEEL_UP, EventNameMouseWheelUp);
		}
		else if (ev->ev.type == SDL_MOUSEBUTTONUP)
		{
			switch (ev->ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_LEFT];
					pNearest->m_LastClickTime[SDL_BUTTON_LEFT] = timer_Time();
					if (timeElapsed < SELECT_DBLCLICK_RATE)
						ret = pNearest->SendMouseEvent(GUIM_MOUSE_DBLCLICK_LEFT, EventNameMouseLeftDoubleClick);
					else
						ret = pNearest->SendMouseEvent(GUIM_MOUSE_RELEASE_LEFT, EventNameMouseLeftRelease);
				}
				break;
			case SDL_BUTTON_RIGHT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_RIGHT];
					pNearest->m_LastClickTime[SDL_BUTTON_RIGHT] = timer_Time();
					if (timeElapsed < SELECT_DBLCLICK_RATE)
						ret = pNearest->SendMouseEvent(GUIM_MOUSE_DBLCLICK_RIGHT, EventNameMouseRightDoubleClick);
					else
						ret = pNearest->SendMouseEvent(GUIM_MOUSE_RELEASE_RIGHT, EventNameMouseRightRelease);
				}
				break;
			}

			// Reset all states on all visible objects
			m_BaseObject->RecurseObject(&IGUIObject::IsHidden, &IGUIObject::ResetStates);

			// Since the hover state will have been reset, we reload it.
			m_BaseObject->RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::UpdateMouseOver, static_cast<IGUIObject* const&>(pNearest));
		}
	}

	// BUTTONUP's effect on m_MouseButtons is handled after
	// everything else, so that e.g. 'press' handlers (activated
	// on button up) see which mouse button had been pressed.
	if (ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		switch (ev->ev.button.button)
		{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		case SDL_BUTTON_MIDDLE:
			m_MouseButtons &= ~Bit<unsigned int>(ev->ev.button.button);
			break;
		default:
			break;
		}
	}

	// Restore m_MousePos (for delayed mouse button events)
	if (ev->ev.type == SDL_MOUSEBUTTONDOWN || ev->ev.type == SDL_MOUSEBUTTONUP)
		m_MousePos = oldMousePos;

	// Let GUI items handle keys after everything else, e.g. for input boxes.
	if (ret == IN_PASS && GetFocusedObject())
	{
		if (ev->ev.type == SDL_KEYUP || ev->ev.type == SDL_KEYDOWN ||
			ev->ev.type == SDL_HOTKEYUP || ev->ev.type == SDL_HOTKEYDOWN ||
			ev->ev.type == SDL_TEXTINPUT || ev->ev.type == SDL_TEXTEDITING)
			ret = GetFocusedObject()->ManuallyHandleKeys(ev);
		// else will return IN_PASS because we never used the button.
	}

	return ret;
}

void CGUI::TickObjects()
{
	m_BaseObject->RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::Tick);
	SendEventToAll(EventNameTick);
	m_Tooltip.Update(FindObjectUnderMouse(), m_MousePos, *this);
}

void CGUI::SendEventToAll(const CStr& eventName)
{
	std::unordered_map<CStr, std::vector<IGUIObject*>>::iterator it = m_EventObjects.find(eventName);
	if (it == m_EventObjects.end())
		return;

	std::vector<IGUIObject*> copy = it->second;
	for (IGUIObject* object : copy)
		object->ScriptEvent(eventName);
}

void CGUI::SendEventToAll(const CStr& eventName, const JS::HandleValueArray& paramData)
{
	std::unordered_map<CStr, std::vector<IGUIObject*>>::iterator it = m_EventObjects.find(eventName);
	if (it == m_EventObjects.end())
		return;

	std::vector<IGUIObject*> copy = it->second;
	for (IGUIObject* object : copy)
		object->ScriptEvent(eventName, paramData);
}

void CGUI::Draw()
{
	using Arena = Allocators::DynamicArena<128 * KiB>;
	using ObjectListAllocator = ProxyAllocator<VisibleObject, Arena>;
	Arena arena;

	std::vector<VisibleObject, ObjectListAllocator> visibleObjects((ObjectListAllocator(arena)));
	CollectVisibleObjectsRecursively(m_BaseObject->GetChildren(), &visibleObjects);
	for (VisibleObject& visibleObject : visibleObjects)
		visibleObject.bufferedZ = visibleObject.object->GetBufferedZ();

	std::sort(visibleObjects.begin(), visibleObjects.end(), [](const VisibleObject& visibleObject1, const VisibleObject& visibleObject2) -> bool {
		if (visibleObject1.bufferedZ != visibleObject2.bufferedZ)
			return visibleObject1.bufferedZ < visibleObject2.bufferedZ;
		return visibleObject1.index < visibleObject2.index;
	});

	CCanvas2D canvas;
	for (const VisibleObject& visibleObject : visibleObjects)
		visibleObject.object->Draw(canvas);
}

void CGUI::DrawSprite(const CGUISpriteInstance& Sprite, CCanvas2D& canvas, const CRect& Rect, const CRect& UNUSED(Clipping))
{
	// If the sprite doesn't exist (name == ""), don't bother drawing anything
	if (!Sprite)
		return;

	// TODO: Clipping?

	Sprite.Draw(*this, canvas, Rect, m_Sprites);
}

void CGUI::UpdateResolution()
{
	m_BaseObject->RecurseObject(nullptr, &IGUIObject::UpdateCachedSize);
}

IGUIObject* CGUI::ConstructObject(const CStr& str)
{
	std::map<CStr, ConstructObjectFunction>::iterator it = m_ObjectTypes.find(str);

	if (it == m_ObjectTypes.end())
		return nullptr;

	return (*it->second)(*this);
}

bool CGUI::AddObject(IGUIObject& parent, IGUIObject& child)
{
	if (child.m_Name.empty())
	{
		LOGERROR("Can't register an object without name!");
		return false;
	}

	if (m_pAllObjects.find(child.m_Name) != m_pAllObjects.end())
	{
		LOGERROR("Can't register more than one object of the name %s", child.m_Name.c_str());
		return false;
	}

	m_pAllObjects[child.m_Name] = &child;
	parent.RegisterChild(&child);
	return true;
}

IGUIObject* CGUI::GetBaseObject()
{
	return m_BaseObject.get();
};

bool CGUI::ObjectExists(const CStr& Name) const
{
	return m_pAllObjects.find(Name) != m_pAllObjects.end();
}

IGUIObject* CGUI::FindObjectByName(const CStr& Name) const
{
	map_pObjects::const_iterator it = m_pAllObjects.find(Name);

	if (it == m_pAllObjects.end())
		return nullptr;

	return it->second;
}

IGUIObject* CGUI::FindObjectUnderMouse()
{
	IGUIObject* pNearest = nullptr;
	m_BaseObject->RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::ChooseMouseOverAndClosest, pNearest);
	return pNearest;
}

CSize2D CGUI::GetWindowSize() const
{
	return CSize2D{static_cast<float>(g_xres) / g_GuiScale, static_cast<float>(g_yres) / g_GuiScale};
}

void CGUI::SetFocusedObject(IGUIObject* pObject)
{
	if (pObject == m_FocusedObject)
		return;

	if (m_FocusedObject)
	{
		SGUIMessage msg(GUIM_LOST_FOCUS);
		m_FocusedObject->HandleMessage(msg);
	}

	m_FocusedObject = pObject;

	if (m_FocusedObject)
	{
		SGUIMessage msg(GUIM_GOT_FOCUS);
		m_FocusedObject->HandleMessage(msg);
	}
}

void CGUI::SetObjectStyle(IGUIObject* pObject, const CStr& styleName)
{
	// If the style is not recognised (or an empty string) then ApplyStyle will
	// emit an error message. Thus we don't need to handle it here.
	pObject->ApplyStyle(styleName);
}

void CGUI::UnsetObjectStyle(IGUIObject* pObject)
{
	SetObjectStyle(pObject, "default");
}

void CGUI::SetObjectHotkey(IGUIObject* pObject, const CStr& hotkeyTag)
{
	if (!hotkeyTag.empty())
		m_HotkeyObjects[hotkeyTag].push_back(pObject);
}

void CGUI::UnsetObjectHotkey(IGUIObject* pObject, const CStr& hotkeyTag)
{
	if (hotkeyTag.empty())
		return;

	std::vector<IGUIObject*>& assignment = m_HotkeyObjects[hotkeyTag];

	assignment.erase(
		std::remove_if(
			assignment.begin(),
			assignment.end(),
			[&pObject](const IGUIObject* hotkeyObject)
				{ return pObject == hotkeyObject; }),
		assignment.end());
}

void CGUI::SetGlobalHotkey(const CStr& hotkeyTag, const CStr& eventName, JS::HandleValue function)
{
	ScriptRequest rq(*m_ScriptInterface);

	if (hotkeyTag.empty())
	{
		ScriptException::Raise(rq, "Cannot assign a function to an empty hotkey identifier!");
		return;
	}

	// Only support "Press", "Keydown" and "Release" events.
	if (eventName != EventNamePress && eventName != EventNameKeyDown && eventName != EventNameRelease)
	{
		ScriptException::Raise(rq, "Cannot assign a function to an unsupported event!");
		return;
	}

	if (!function.isObject() || !JS_ObjectIsFunction(&function.toObject()))
	{
		ScriptException::Raise(rq, "Cannot assign non-function value to global hotkey '%s'", hotkeyTag.c_str());
		return;
	}

	UnsetGlobalHotkey(hotkeyTag, eventName);
	m_GlobalHotkeys[hotkeyTag][eventName].init(rq.cx, function);
}

void CGUI::UnsetGlobalHotkey(const CStr& hotkeyTag, const CStr& eventName)
{
	std::map<CStr, std::map<CStr, JS::PersistentRootedValue>>::iterator it = m_GlobalHotkeys.find(hotkeyTag);
	if (it == m_GlobalHotkeys.end())
		return;

	m_GlobalHotkeys[hotkeyTag].erase(eventName);

	if (m_GlobalHotkeys.count(hotkeyTag) == 0)
		m_GlobalHotkeys.erase(it);
}

const SGUIScrollBarStyle* CGUI::GetScrollBarStyle(const CStr& style) const
{
	std::map<CStr, const SGUIScrollBarStyle>::const_iterator it = m_ScrollBarStyles.find(style);
	if (it == m_ScrollBarStyles.end())
		return nullptr;

 	return &it->second;
}

/**
 * @callgraph
 */
void CGUI::LoadXmlFile(const VfsPath& Filename, std::unordered_set<VfsPath>& Paths)
{
	Paths.insert(Filename);

	CXeromyces xeroFile;
	if (xeroFile.Load(g_VFS, Filename, "gui") != PSRETURN_OK)
		return;

	XMBElement node = xeroFile.GetRoot();
	std::string_view root_name(xeroFile.GetElementStringView(node.GetNodeName()));

	if (root_name == "objects")
		Xeromyces_ReadRootObjects(xeroFile, node, Paths);
	else if (root_name == "sprites")
		Xeromyces_ReadRootSprites(xeroFile, node);
	else if (root_name == "styles")
		Xeromyces_ReadRootStyles(xeroFile, node);
	else if (root_name == "setup")
		Xeromyces_ReadRootSetup(xeroFile, node);
	else
		LOGERROR("CGUI::LoadXmlFile encountered an unknown XML root node type: %s", root_name.data());
}

void CGUI::LoadedXmlFiles()
{
	m_BaseObject->RecurseObject(nullptr, &IGUIObject::UpdateCachedSize);

	SGUIMessage msg(GUIM_LOAD);
	m_BaseObject->RecurseObject(nullptr, &IGUIObject::HandleMessage, msg);

	SendEventToAll(EventNameLoad);
}

//===================================================================
//	XML Reading Xeromyces Specific Sub-Routines
//===================================================================

void CGUI::Xeromyces_ReadRootObjects(const XMBData& xmb, XMBElement element, std::unordered_set<VfsPath>& Paths)
{
	int el_script = xmb.GetElementID("script");

	std::vector<std::pair<CStr, CStr> > subst;

	// Iterate main children
	//  they should all be <object> or <script> elements
	for (XMBElement child : element.GetChildNodes())
	{
		if (child.GetNodeName() == el_script)
			// Execute the inline script
			Xeromyces_ReadScript(xmb, child, Paths);
		else
			// Read in this whole object into the GUI
			Xeromyces_ReadObject(xmb, child, m_BaseObject.get(), subst, Paths, 0);
	}
}

void CGUI::Xeromyces_ReadRootSprites(const XMBData& xmb, XMBElement element)
{
	for (XMBElement child : element.GetChildNodes())
		Xeromyces_ReadSprite(xmb, child);
}

void CGUI::Xeromyces_ReadRootStyles(const XMBData& xmb, XMBElement element)
{
	for (XMBElement child : element.GetChildNodes())
		Xeromyces_ReadStyle(xmb, child);
}

void CGUI::Xeromyces_ReadRootSetup(const XMBData& xmb, XMBElement element)
{
	for (XMBElement child : element.GetChildNodes())
	{
		std::string_view name(xmb.GetElementStringView(child.GetNodeName()));
		if (name == "scrollbar")
			Xeromyces_ReadScrollBarStyle(xmb, child);
		else if (name == "icon")
			Xeromyces_ReadIcon(xmb, child);
		else if (name == "tooltip")
			Xeromyces_ReadTooltip(xmb, child);
		else if (name == "color")
			Xeromyces_ReadColor(xmb, child);
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}
}

IGUIObject* CGUI::Xeromyces_ReadObject(const XMBData& xmb, XMBElement element, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	ENSURE(pParent);

	XMBAttributeList attributes = element.GetAttributes();

	CStr type(attributes.GetNamedItem(xmb.GetAttributeID("type")));
	if (type.empty())
		type = "empty";

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	IGUIObject* object = ConstructObject(type);

	if (!object)
	{
		LOGERROR("GUI: Unrecognized object type \"%s\"", type.c_str());
		return nullptr;
	}

	// Cache some IDs for element attribute names, to avoid string comparisons
	#define ELMT(x) int elmt_##x = xmb.GetElementID(#x)
	#define ATTR(x) int attr_##x = xmb.GetAttributeID(#x)
	ELMT(object);
	ELMT(action);
	ELMT(script);
	ELMT(repeat);
	ELMT(translatableAttribute);
	ELMT(translate);
	ELMT(attribute);
	ELMT(keep);
	ELMT(include);
	ATTR(style);
	ATTR(type);
	ATTR(name);
	ATTR(z);
	ATTR(on);
	ATTR(file);
	ATTR(directory);
	ATTR(id);
	ATTR(context);

	//
	//	Read Style and set defaults
	//
	//	If the setting "style" is set, try loading that setting.
	//
	//	Always load default (if it's available) first!
	//
	SetObjectStyle(object, "default");

	CStr argStyle(attributes.GetNamedItem(attr_style));
	if (!argStyle.empty())
		SetObjectStyle(object, argStyle);

	bool NameSet = false;
	bool ManuallySetZ = false;

	for (XMBAttribute attr : attributes)
	{
		// If value is "null", then it is equivalent as never being entered
		if (attr.Value == "null")
			continue;

		// Ignore "type" and "style", we've already checked it
		if (attr.Name == attr_type || attr.Name == attr_style)
			continue;

		if (attr.Name == attr_name)
		{
			CStr name(attr.Value);

			if (name.Left(2) == "__")
			{
				LOGERROR("GUI: Names starting with '__' are reserved for the engine (object: %s)", name.c_str());
				continue;
			}

			for (const std::pair<CStr, CStr>& sub : NameSubst)
				name.Replace(sub.first, sub.second);

			object->SetName(name);
			NameSet = true;
			continue;
		}

		if (attr.Name == attr_z)
			ManuallySetZ = true;

		object->SetSettingFromString(xmb.GetAttributeString(attr.Name), attr.Value.FromUTF8(), false);
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName("__internal(" + CStr::FromInt(m_InternalNameNumber) + ")");
		++m_InternalNameNumber;
	}

	CStrW caption(element.GetText().FromUTF8());
	if (!caption.empty())
		object->SetSettingFromString("caption", caption, false);

	for (XMBElement child : element.GetChildNodes())
	{
		// Check what name the elements got
		int element_name = child.GetNodeName();

		if (element_name == elmt_object)
		{
			// Call this function on the child
			Xeromyces_ReadObject(xmb, child, object, NameSubst, Paths, nesting_depth);
		}
		else if (element_name == elmt_action)
		{
			// Scripted <action> element

			// Check for a 'file' parameter
			CStrW filename(child.GetAttributes().GetNamedItem(attr_file).FromUTF8());

			CStr code;

			// If there is a file, open it and use it as the code
			if (!filename.empty())
			{
				Paths.insert(filename);
				CVFSFile scriptfile;
				if (scriptfile.Load(g_VFS, filename) != PSRETURN_OK)
				{
					LOGERROR("Error opening GUI script action file '%s'", utf8_from_wstring(filename));
					continue;
				}

				code = scriptfile.DecodeUTF8(); // assume it's UTF-8
			}

			XMBElementList grandchildren = child.GetChildNodes();
			if (!grandchildren.empty()) // The <action> element contains <keep> and <translate> tags.
				for (XMBElement grandchild : grandchildren)
				{
					if (grandchild.GetNodeName() == elmt_translate)
						code += g_L10n.Translate(grandchild.GetText());
					else if (grandchild.GetNodeName() == elmt_keep)
						code += grandchild.GetText();
				}
			else // It's pure JavaScript code.
				// Read the inline code (concatenating to the file code, if both are specified)
				code += CStr(child.GetText());

			CStr eventName = child.GetAttributes().GetNamedItem(attr_on);
			object->RegisterScriptHandler(eventName, code, *this);
		}
		else if (child.GetNodeName() == elmt_script)
		{
			Xeromyces_ReadScript(xmb, child, Paths);
		}
		else if (element_name == elmt_repeat)
		{
			Xeromyces_ReadRepeat(xmb, child, object, NameSubst, Paths, nesting_depth);
		}
		else if (element_name == elmt_translatableAttribute)
		{
			// This is an element in the form "<translatableAttribute id="attributeName">attributeValue</translatableAttribute>".
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: 'translatableAttribute' XML element with empty 'id' XML attribute found. (object: %s)", object->GetPresentableName().c_str());
				continue;
			}

			CStr value(child.GetText());
			if (value.empty())
				continue;

			CStr context(child.GetAttributes().GetNamedItem(attr_context)); // Read the context if any.

			CStr translatedValue = context.empty() ?
				g_L10n.Translate(value) :
				g_L10n.TranslateWithContext(context, value);

			object->SetSettingFromString(attributeName, translatedValue.FromUTF8(), false);
		}
		else if (element_name == elmt_attribute)
		{
			// This is an element in the form "<attribute id="attributeName"><keep>Don't translate this part
			// </keep><translate>but translate this one.</translate></attribute>".
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: 'attribute' XML element with empty 'id' XML attribute found. (object: %s)", object->GetPresentableName().c_str());
				continue;
			}

			CStr translatedValue;

			for (XMBElement grandchild : child.GetChildNodes())
			{
				if (grandchild.GetNodeName() == elmt_translate)
					translatedValue += g_L10n.Translate(grandchild.GetText());
				else if (grandchild.GetNodeName() == elmt_keep)
					translatedValue += grandchild.GetText();
			}
			object->SetSettingFromString(attributeName, translatedValue.FromUTF8(), false);
		}
		else if (element_name == elmt_include)
		{
			CStrW filename(child.GetAttributes().GetNamedItem(attr_file).FromUTF8());
			CStrW directory(child.GetAttributes().GetNamedItem(attr_directory).FromUTF8());
			if (!filename.empty())
			{
				if (!directory.empty())
					LOGWARNING("GUI: Include element found with file name (%s) and directory name (%s). Only the file will be processed.", utf8_from_wstring(filename), utf8_from_wstring(directory));

				Paths.insert(filename);

				CXeromyces xeroIncluded;
				if (xeroIncluded.Load(g_VFS, filename, "gui") != PSRETURN_OK)
				{
					LOGERROR("GUI: Error reading included XML: '%s'", utf8_from_wstring(filename));
					continue;
				}

				XMBElement node = xeroIncluded.GetRoot();
				if (node.GetNodeName() != xeroIncluded.GetElementID("object"))
				{
					LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", utf8_from_wstring(filename));
					continue;
				}

				if (nesting_depth+1 >= MAX_OBJECT_DEPTH)
				{
					LOGERROR("GUI: Too many nested GUI includes. Probably caused by a recursive include attribute. Abort rendering '%s'.", utf8_from_wstring(filename));
					continue;
				}

				Xeromyces_ReadObject(xeroIncluded, node, object, NameSubst, Paths, nesting_depth+1);
			}
			else if (!directory.empty())
			{
				if (nesting_depth+1 >= MAX_OBJECT_DEPTH)
				{
					LOGERROR("GUI: Too many nested GUI includes. Probably caused by a recursive include attribute. Abort rendering '%s'.", utf8_from_wstring(directory));
					continue;
				}

				VfsPaths pathnames;
				vfs::GetPathnames(g_VFS, directory, L"*.xml", pathnames);
				for (const VfsPath& path : pathnames)
				{
					// as opposed to loading scripts, don't care if it's loaded before
					// one might use the same parts of the GUI in different situations
					Paths.insert(path);
					CXeromyces xeroIncluded;
					if (xeroIncluded.Load(g_VFS, path, "gui") != PSRETURN_OK)
					{
						LOGERROR("GUI: Error reading included XML: '%s'", path.string8());
						continue;
					}

					XMBElement node = xeroIncluded.GetRoot();
					if (node.GetNodeName() != xeroIncluded.GetElementID("object"))
					{
						LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", path.string8());
						continue;
					}
					Xeromyces_ReadObject(xeroIncluded, node, object, NameSubst, Paths, nesting_depth+1);
				}

			}
			else
				LOGERROR("GUI: 'include' XML element must have valid 'file' or 'directory' attribute found. (object %s)", object->GetPresentableName().c_str());
		}
		else
		{
			// Try making the object read the tag.
			if (!object->HandleAdditionalChildren(xmb, child))
				LOGERROR("GUI: (object: %s) Reading unknown children for its type", object->GetPresentableName().c_str());
		}
	}

	object->AdditionalChildrenHandled();

	if (!ManuallySetZ)
	{
		// Set it automatically to 10 plus its parents
		if (object->m_Absolute)
			// If the object is absolute, we'll have to get the parent's Z buffered,
			// and add to that!
			object->m_Z.Set(pParent->GetBufferedZ() + 10.f, false);
		else
			// If the object is relative, then we'll just store Z as "10"
			object->m_Z.Set(10.f, false);
	}

	if (!AddObject(*pParent, *object))
	{
		delete object;
		return nullptr;
	}
	return object;
}

void CGUI::Xeromyces_ReadRepeat(const XMBData& xmb, XMBElement element, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	#define ELMT(x) int elmt_##x = xmb.GetElementID(#x)
	#define ATTR(x) int attr_##x = xmb.GetAttributeID(#x)
	ELMT(object);
	ATTR(count);
	ATTR(var);

	XMBAttributeList attributes = element.GetAttributes();

	int count = CStr(attributes.GetNamedItem(attr_count)).ToInt();
	CStr var("["+attributes.GetNamedItem(attr_var)+"]");
	if (var.size() < 3)
		var = "[n]";

	for (int n = 0; n < count; ++n)
	{
		NameSubst.emplace_back(var, "[" + CStr::FromInt(n) + "]");

		XERO_ITER_EL(element, child)
		{
			if (child.GetNodeName() == elmt_object)
				Xeromyces_ReadObject(xmb, child, pParent, NameSubst, Paths, nesting_depth);
		}
		NameSubst.pop_back();
	}
}

void CGUI::Xeromyces_ReadScript(const XMBData& xmb, XMBElement element, std::unordered_set<VfsPath>& Paths)
{
	// Check for a 'file' parameter
	CStrW fileAttr(element.GetAttributes().GetNamedItem(xmb.GetAttributeID("file")).FromUTF8());

	// If there is a file specified, open and execute it
	if (!fileAttr.empty())
	{
		if (!VfsPath(fileAttr).IsDirectory())
		{
			Paths.insert(fileAttr);
			m_ScriptInterface->LoadGlobalScriptFile(fileAttr);
		}
		else
			LOGERROR("GUI: Script path %s is not a file path", fileAttr.ToUTF8().c_str());
	}

	// If it has a directory attribute, read all JS files in that directory
	CStrW directoryAttr(element.GetAttributes().GetNamedItem(xmb.GetAttributeID("directory")).FromUTF8());
	if (!directoryAttr.empty())
	{
		if (VfsPath(directoryAttr).IsDirectory())
		{
			VfsPaths pathnames;
			vfs::GetPathnames(g_VFS, directoryAttr, L"*.js", pathnames);
			for (const VfsPath& path : pathnames)
			{
				// Only load new files (so when the insert succeeds)
				if (Paths.insert(path).second)
					m_ScriptInterface->LoadGlobalScriptFile(path);
			}
		}
		else
			LOGERROR("GUI: Script path %s is not a directory path", directoryAttr.ToUTF8().c_str());
	}

	CStr code(element.GetText());
	if (!code.empty())
		m_ScriptInterface->LoadGlobalScript(L"Some XML file", code);
}

void CGUI::Xeromyces_ReadSprite(const XMBData& xmb, XMBElement element)
{
	CGUISprite* Sprite = new CGUISprite;

	// Get name, we know it exists because of DTD requirements
	CStr name = element.GetAttributes().GetNamedItem(xmb.GetAttributeID("name"));

	if (m_Sprites.find(name) != m_Sprites.end())
		LOGWARNING("GUI sprite name '%s' used more than once; first definition will be discarded", name.c_str());

	// shared_ptr to link the effect to every image, faster than copy.
	std::shared_ptr<SGUIImageEffects> effects;

	for (XMBElement child : element.GetChildNodes())
	{
		std::string_view ElementName(xmb.GetElementStringView(child.GetNodeName()));
		if (ElementName == "image")
			Xeromyces_ReadImage(xmb, child, *Sprite);
		else if (ElementName == "effect")
		{
			if (effects)
				LOGERROR("GUI <sprite> must not have more than one <effect>");
			else
			{
				effects = std::make_shared<SGUIImageEffects>();
				Xeromyces_ReadEffects(xmb, child, *effects);
			}
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	// Apply the effects to every image (unless the image overrides it with
	// different effects)
	if (effects)
		for (SGUIImage* const& img : Sprite->m_Images)
			if (!img->m_Effects)
				img->m_Effects = effects;

	m_Sprites.erase(name);
	m_Sprites.emplace(name, Sprite);
}

void CGUI::Xeromyces_ReadImage(const XMBData& xmb, XMBElement element, CGUISprite& parent)
{
	SGUIImage* Image = new SGUIImage();

	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		CStrW attr_value(attr.Value.FromUTF8());

		if (attr_name == "texture")
		{
			Image->m_TextureName = VfsPath("art/textures/ui") / attr_value;
		}
		else if (attr_name == "size")
		{
			Image->m_Size.FromString(attr.Value);
		}
		else if (attr_name == "texture_size")
		{
			Image->m_TextureSize.FromString(attr.Value);
		}
		else if (attr_name == "real_texture_placement")
		{
			CRect rect;
			if (!ParseString<CRect>(this, attr_value, rect))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_TexturePlacementInFile = rect;
		}
		else if (attr_name == "fixed_h_aspect_ratio")
		{
			float val;
			if (!ParseString<float>(this, attr_value, val))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_FixedHAspectRatio = val;
		}
		else if (attr_name == "round_coordinates")
		{
			bool b;
			if (!ParseString<bool>(this, attr_value, b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_RoundCoordinates = b;
		}
		else if (attr_name == "wrap_mode")
		{
			if (attr_value == L"repeat")
				Image->m_WrapMode = GL_REPEAT;
			else if (attr_value == L"mirrored_repeat")
				Image->m_WrapMode = GL_MIRRORED_REPEAT;
			else if (attr_value == L"clamp_to_edge")
				Image->m_WrapMode = GL_CLAMP_TO_EDGE;
			else
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
		}
		else if (attr_name == "backcolor")
		{
			if (!ParseString<CGUIColor>(this, attr_value, Image->m_BackColor))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	// Look for effects
	for (XMBElement child : element.GetChildNodes())
	{
		std::string_view ElementName(xmb.GetElementStringView(child.GetNodeName()));
		if (ElementName == "effect")
		{
			if (Image->m_Effects)
				LOGERROR("GUI <image> must not have more than one <effect>");
			else
			{
				Image->m_Effects = std::make_shared<SGUIImageEffects>();
				Xeromyces_ReadEffects(xmb, child, *Image->m_Effects);
			}
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	parent.AddImage(Image);
}

void CGUI::Xeromyces_ReadEffects(const XMBData& xmb, XMBElement element, SGUIImageEffects& effects)
{
	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		if (attr_name == "add_color")
		{
			if (!effects.m_AddColor.ParseString(*this, attr.Value, 0))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr.Value);
		}
		else if (attr_name == "grayscale")
			effects.m_Greyscale = true;
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}
}

void CGUI::Xeromyces_ReadStyle(const XMBData& xmb, XMBElement element)
{
	SGUIStyle style;
	CStr name;

	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == "name")
			name = attr.Value;
		else
			style.m_SettingsDefaults.emplace(std::string(attr_name), attr.Value.FromUTF8());
	}

	m_Styles.erase(name);
	m_Styles.emplace(name, std::move(style));
}

void CGUI::Xeromyces_ReadScrollBarStyle(const XMBData& xmb, XMBElement element)
{
	SGUIScrollBarStyle scrollbar;
	CStr name;

	// Setup some defaults.
	scrollbar.m_MinimumBarSize = 0.f;
	// Using 1.0e10 as a substitute for infinity
	scrollbar.m_MaximumBarSize = 1.0e10;
	scrollbar.m_UseEdgeButtons = false;

	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else if (attr_name == "show_edge_buttons")
		{
			bool b;
			if (!ParseString<bool>(this, attr_value.FromUTF8(), b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_UseEdgeButtons = b;
		}
		else if (attr_name == "width")
		{
			float f;
			if (!ParseString<float>(this, attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_Width = f;
		}
		else if (attr_name == "minimum_bar_size")
		{
			float f;
			if (!ParseString<float>(this, attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_MinimumBarSize = f;
		}
		else if (attr_name == "maximum_bar_size")
		{
			float f;
			if (!ParseString<float>(this, attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_MaximumBarSize = f;
		}
		else if (attr_name == "sprite_button_top")
			scrollbar.m_SpriteButtonTop = attr_value;
		else if (attr_name == "sprite_button_top_pressed")
			scrollbar.m_SpriteButtonTopPressed = attr_value;
		else if (attr_name == "sprite_button_top_disabled")
			scrollbar.m_SpriteButtonTopDisabled = attr_value;
		else if (attr_name == "sprite_button_top_over")
			scrollbar.m_SpriteButtonTopOver = attr_value;
		else if (attr_name == "sprite_button_bottom")
			scrollbar.m_SpriteButtonBottom = attr_value;
		else if (attr_name == "sprite_button_bottom_pressed")
			scrollbar.m_SpriteButtonBottomPressed = attr_value;
		else if (attr_name == "sprite_button_bottom_disabled")
			scrollbar.m_SpriteButtonBottomDisabled = attr_value;
		else if (attr_name == "sprite_button_bottom_over")
			scrollbar.m_SpriteButtonBottomOver = attr_value;
		else if (attr_name == "sprite_back_vertical")
			scrollbar.m_SpriteBackVertical = attr_value;
		else if (attr_name == "sprite_bar_vertical")
			scrollbar.m_SpriteBarVertical = attr_value;
		else if (attr_name == "sprite_bar_vertical_over")
			scrollbar.m_SpriteBarVerticalOver = attr_value;
		else if (attr_name == "sprite_bar_vertical_pressed")
			scrollbar.m_SpriteBarVerticalPressed = attr_value;
	}

	m_ScrollBarStyles.erase(name);
	m_ScrollBarStyles.emplace(name, std::move(scrollbar));
}

void CGUI::Xeromyces_ReadIcon(const XMBData& xmb, XMBElement element)
{
	SGUIIcon icon;
	CStr name;

	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else if (attr_name == "sprite")
			icon.m_SpriteName = attr_value;
		else if (attr_name == "size")
		{
			CSize2D size;
			if (!ParseString<CSize2D>(this, attr_value.FromUTF8(), size))
				LOGERROR("Error parsing '%s' (\"%s\") inside <icon>.", attr_name, attr_value);
			else
				icon.m_Size = size;
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	m_Icons.erase(name);
	m_Icons.emplace(name, std::move(icon));
}

void CGUI::Xeromyces_ReadTooltip(const XMBData& xmb, XMBElement element)
{
	IGUIObject* object = new CTooltip(*this);

	for (XMBAttribute attr : element.GetAttributes())
	{
		std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_name == "name")
			object->SetName("__tooltip_" + attr_value);
		else
			object->SetSettingFromString(std::string(attr_name), attr_value.FromUTF8(), true);
	}

	if (!AddObject(*m_BaseObject, *object))
		delete object;
}

void CGUI::Xeromyces_ReadColor(const XMBData& xmb, XMBElement element)
{
	XMBAttributeList attributes = element.GetAttributes();
	CStr name = attributes.GetNamedItem(xmb.GetAttributeID("name"));

	// Try parsing value
	CStr value(element.GetText());
	if (value.empty())
		return;

	CColor color;
	if (color.ParseString(value))
	{
		m_PreDefinedColors.erase(name);
		m_PreDefinedColors.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(color.r, color.g, color.b, color.a));
	}
	else
		LOGERROR("GUI: Unable to create custom color '%s'. Invalid color syntax.", name.c_str());
}
