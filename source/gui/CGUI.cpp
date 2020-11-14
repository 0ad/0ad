/* Copyright (C) 2020 Wildfire Games.
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

#include "gui/GUIObjectTypes.h"
#include "gui/IGUIScrollBar.h"
#include "gui/Scripting/ScriptFunctions.h"
#include "i18n/L10n.h"
#include "lib/bits.h"
#include "lib/input.h"
#include "lib/sysdep/sysdep.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>
#include <unordered_set>

extern int g_yres;

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

CGUI::CGUI(const shared_ptr<ScriptContext>& context)
	: m_BaseObject(*this),
	  m_FocusedObject(nullptr),
	  m_InternalNameNumber(0),
	  m_MouseButtons(0)
{
	m_ScriptInterface.reset(new ScriptInterface("Engine", "GUIPage", context));
	m_ScriptInterface->SetCallbackData(this);

	GuiScriptingInit(*m_ScriptInterface);
	m_ScriptInterface->LoadGlobalScripts();
}

CGUI::~CGUI()
{
	for (const std::pair<CStr, IGUIObject*>& p : m_pAllObjects)
		delete p.second;

	for (const std::pair<CStr, const CGUISprite*>& p : m_Sprites)
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

			ScriptInterface::Request rq(m_ScriptInterface);
			JS::RootedObject globalObj(rq.cx, rq.glob);
			JS::RootedValue result(rq.cx);
			JS_CallFunctionValue(rq.cx, globalObj, m_GlobalHotkeys[hotkey][eventName], JS::HandleValueArray::empty(), &result);
		}

		std::map<CStr, std::vector<IGUIObject*> >::iterator it = m_HotkeyObjects.find(hotkey);
		if (it != m_HotkeyObjects.end())
			for (IGUIObject* const& obj : it->second)
			{
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
		m_MousePos = CPos((float)ev->ev.motion.x / g_GuiScale, (float)ev->ev.motion.y / g_GuiScale);

		SGUIMessage msg(GUIM_MOUSE_MOTION);
		m_BaseObject.RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::HandleMessage, msg);
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
	CPos oldMousePos = m_MousePos;
	if (ev->ev.type == SDL_MOUSEBUTTONDOWN || ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		m_MousePos = CPos((float)ev->ev.button.x / g_GuiScale, (float)ev->ev.button.y / g_GuiScale);
	}

	// Only one object can be hovered
	IGUIObject* pNearest = nullptr;

	// TODO Gee: (2004-09-08) Big TODO, don't do the below if the SDL_Event is something like a keypress!
	{
		PROFILE("mouse events");
		// TODO Gee: Optimizations needed!
		//  these two recursive function are quite overhead heavy.

		// pNearest will after this point at the hovered object, possibly nullptr
		pNearest = FindObjectUnderMouse();

		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		m_BaseObject.RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::UpdateMouseOver, static_cast<IGUIObject* const&>(pNearest));

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
			m_BaseObject.RecurseObject(&IGUIObject::IsHidden, &IGUIObject::ResetStates);

			// Since the hover state will have been reset, we reload it.
			m_BaseObject.RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::UpdateMouseOver, static_cast<IGUIObject* const&>(pNearest));
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

	// Handle keys for input boxes
	if (GetFocusedObject())
	{
		if ((ev->ev.type == SDL_KEYDOWN &&
		     ev->ev.key.keysym.sym != SDLK_ESCAPE &&
		     !g_keys[SDLK_LCTRL] && !g_keys[SDLK_RCTRL] &&
		     !g_keys[SDLK_LALT] && !g_keys[SDLK_RALT]) ||
		    ev->ev.type == SDL_HOTKEYDOWN ||
		    ev->ev.type == SDL_TEXTINPUT ||
		    ev->ev.type == SDL_TEXTEDITING)
		{
			ret = GetFocusedObject()->ManuallyHandleEvent(ev);
		}
		// else will return IN_PASS because we never used the button.
	}

	return ret;
}

void CGUI::TickObjects()
{
	SendEventToAll(EventNameTick);
	m_Tooltip.Update(FindObjectUnderMouse(), m_MousePos, *this);
}

void CGUI::SendEventToAll(const CStr& eventName)
{
	m_BaseObject.RecurseObject(nullptr, &IGUIObject::ScriptEvent, eventName);
}

void CGUI::SendEventToAll(const CStr& eventName, const JS::HandleValueArray& paramData)
{
	m_BaseObject.RecurseObject(nullptr, &IGUIObject::ScriptEvent, eventName, paramData);
}

void CGUI::Draw()
{
	// Clear the depth buffer, so the GUI is
	// drawn on top of everything else
	glClear(GL_DEPTH_BUFFER_BIT);

	m_BaseObject.RecurseObject(&IGUIObject::IsHidden, &IGUIObject::Draw);
}

void CGUI::DrawSprite(const CGUISpriteInstance& Sprite, int CellID, const float& Z, const CRect& Rect, const CRect& UNUSED(Clipping))
{
	// If the sprite doesn't exist (name == ""), don't bother drawing anything
	if (!Sprite)
		return;

	// TODO: Clipping?

	Sprite.Draw(*this, Rect, CellID, m_Sprites, Z);
}

void CGUI::UpdateResolution()
{
	m_BaseObject.RecurseObject(nullptr, &IGUIObject::UpdateCachedSize);
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
	parent.AddChild(child);
	return true;
}

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
	m_BaseObject.RecurseObject(&IGUIObject::IsHiddenOrGhost, &IGUIObject::ChooseMouseOverAndClosest, pNearest);
	return pNearest;
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
	ScriptInterface::Request rq(*m_ScriptInterface);

	if (hotkeyTag.empty())
	{
		JS_ReportError(rq.cx, "Cannot assign a function to an empty hotkey identifier!");
		return;
	}

	// Only support "Press", "Keydown" and "Release" events.
	if (eventName != EventNamePress && eventName != EventNameKeyDown && eventName != EventNameRelease)
	{
		JS_ReportError(rq.cx, "Cannot assign a function to an unsupported event!");
		return;
	}

	if (!function.isObject() || !JS_ObjectIsFunction(rq.cx, &function.toObject()))
	{
		JS_ReportError(rq.cx, "Cannot assign non-function value to global hotkey '%s'", hotkeyTag.c_str());
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

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, Filename, "gui") != PSRETURN_OK)
		return;

	XMBElement node = XeroFile.GetRoot();
	CStr root_name(XeroFile.GetElementString(node.GetNodeName()));

	if (root_name == "objects")
		Xeromyces_ReadRootObjects(node, &XeroFile, Paths);
	else if (root_name == "sprites")
		Xeromyces_ReadRootSprites(node, &XeroFile);
	else if (root_name == "styles")
		Xeromyces_ReadRootStyles(node, &XeroFile);
	else if (root_name == "setup")
		Xeromyces_ReadRootSetup(node, &XeroFile);
	else
		LOGERROR("CGUI::LoadXmlFile encountered an unknown XML root node type: %s", root_name.c_str());
}

void CGUI::LoadedXmlFiles()
{
	m_BaseObject.RecurseObject(nullptr, &IGUIObject::UpdateCachedSize);

	SGUIMessage msg(GUIM_LOAD);
	m_BaseObject.RecurseObject(nullptr, &IGUIObject::HandleMessage, msg);

	SendEventToAll(EventNameLoad);
}

//===================================================================
//	XML Reading Xeromyces Specific Sub-Routines
//===================================================================

void CGUI::Xeromyces_ReadRootObjects(XMBElement Element, CXeromyces* pFile, std::unordered_set<VfsPath>& Paths)
{
	int el_script = pFile->GetElementID("script");

	std::vector<std::pair<CStr, CStr> > subst;

	// Iterate main children
	//  they should all be <object> or <script> elements
	for (XMBElement child : Element.GetChildNodes())
	{
		if (child.GetNodeName() == el_script)
			// Execute the inline script
			Xeromyces_ReadScript(child, pFile, Paths);
		else
			// Read in this whole object into the GUI
			Xeromyces_ReadObject(child, pFile, &m_BaseObject, subst, Paths, 0);
	}
}

void CGUI::Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
		Xeromyces_ReadSprite(child, pFile);
}

void CGUI::Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
		Xeromyces_ReadStyle(child, pFile);
}

void CGUI::Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
	{
		CStr name(pFile->GetElementString(child.GetNodeName()));

		if (name == "scrollbar")
			Xeromyces_ReadScrollBarStyle(child, pFile);
		else if (name == "icon")
			Xeromyces_ReadIcon(child, pFile);
		else if (name == "tooltip")
			Xeromyces_ReadTooltip(child, pFile);
		else if (name == "color")
			Xeromyces_ReadColor(child, pFile);
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}
}

void CGUI::Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	ENSURE(pParent);

	XMBAttributeList attributes = Element.GetAttributes();

	CStr type(attributes.GetNamedItem(pFile->GetAttributeID("type")));
	if (type.empty())
		type = "empty";

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	IGUIObject* object = ConstructObject(type);

	if (!object)
	{
		LOGERROR("GUI: Unrecognized object type \"%s\"", type.c_str());
		return;
	}

	// Cache some IDs for element attribute names, to avoid string comparisons
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
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
	CStr argStyle(attributes.GetNamedItem(attr_style));

	if (m_Styles.find("default") != m_Styles.end())
		object->LoadStyle("default");

	if (!argStyle.empty())
	{
		if (m_Styles.find(argStyle) == m_Styles.end())
			LOGERROR("GUI: Trying to use style '%s' that doesn't exist.", argStyle.c_str());
		else
			object->LoadStyle(argStyle);
	}

	bool NameSet = false;
	bool ManuallySetZ = false;

	CStrW inclusionPath;

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

			for (const std::pair<CStr, CStr>& sub : NameSubst)
				name.Replace(sub.first, sub.second);

			object->SetName(name);
			NameSet = true;
			continue;
		}

		if (attr.Name == attr_z)
			ManuallySetZ = true;

		object->SetSettingFromString(pFile->GetAttributeString(attr.Name), attr.Value.FromUTF8(), false);
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName("__internal(" + CStr::FromInt(m_InternalNameNumber) + ")");
		++m_InternalNameNumber;
	}

	CStrW caption(Element.GetText().FromUTF8());
	if (!caption.empty())
		object->SetSettingFromString("caption", caption, false);

	for (XMBElement child : Element.GetChildNodes())
	{
		// Check what name the elements got
		int element_name = child.GetNodeName();

		if (element_name == elmt_object)
		{
			// Call this function on the child
			Xeromyces_ReadObject(child, pFile, object, NameSubst, Paths, nesting_depth);
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
			else // It’s pure JavaScript code.
				// Read the inline code (concatenating to the file code, if both are specified)
				code += CStr(child.GetText());

			CStr eventName = child.GetAttributes().GetNamedItem(attr_on);
			object->RegisterScriptHandler(eventName, code, *this);
		}
		else if (child.GetNodeName() == elmt_script)
		{
			Xeromyces_ReadScript(child, pFile, Paths);
		}
		else if (element_name == elmt_repeat)
		{
			Xeromyces_ReadRepeat(child, pFile, object, NameSubst, Paths, nesting_depth);
		}
		else if (element_name == elmt_translatableAttribute)
		{
			// This is an element in the form “<translatableAttribute id="attributeName">attributeValue</translatableAttribute>”.
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: ‘translatableAttribute’ XML element with empty ‘id’ XML attribute found. (object: %s)", object->GetPresentableName().c_str());
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
			// This is an element in the form “<attribute id="attributeName"><keep>Don’t translate this part
			// </keep><translate>but translate this one.</translate></attribute>”.
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: ‘attribute’ XML element with empty ‘id’ XML attribute found. (object: %s)", object->GetPresentableName().c_str());
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

				CXeromyces XeroIncluded;
				if (XeroIncluded.Load(g_VFS, filename, "gui") != PSRETURN_OK)
				{
					LOGERROR("GUI: Error reading included XML: '%s'", utf8_from_wstring(filename));
					continue;
				}

				XMBElement node = XeroIncluded.GetRoot();
				if (node.GetNodeName() != XeroIncluded.GetElementID("object"))
				{
					LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", utf8_from_wstring(filename));
					continue;
				}

				if (nesting_depth+1 >= MAX_OBJECT_DEPTH)
				{
					LOGERROR("GUI: Too many nested GUI includes. Probably caused by a recursive include attribute. Abort rendering '%s'.", utf8_from_wstring(filename));
					continue;
				}

				Xeromyces_ReadObject(node, &XeroIncluded, object, NameSubst, Paths, nesting_depth+1);
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
					CXeromyces XeroIncluded;
					if (XeroIncluded.Load(g_VFS, path, "gui") != PSRETURN_OK)
					{
						LOGERROR("GUI: Error reading included XML: '%s'", path.string8());
						continue;
					}

					XMBElement node = XeroIncluded.GetRoot();
					if (node.GetNodeName() != XeroIncluded.GetElementID("object"))
					{
						LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", path.string8());
						continue;
					}
					Xeromyces_ReadObject(node, &XeroIncluded, object, NameSubst, Paths, nesting_depth+1);
				}

			}
			else
				LOGERROR("GUI: 'include' XML element must have valid 'file' or 'directory' attribute found. (object %s)", object->GetPresentableName().c_str());
		}
		else
		{
			// Try making the object read the tag.
			if (!object->HandleAdditionalChildren(child, pFile))
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
			object->SetSetting<float>("z", pParent->GetBufferedZ() + 10.f, false);
		else
			// If the object is relative, then we'll just store Z as "10"
			object->SetSetting<float>("z", 10.f, false);
	}

	if (!AddObject(*pParent, *object))
		delete object;
}

void CGUI::Xeromyces_ReadRepeat(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, std::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(object);
	ATTR(count);
	ATTR(var);

	XMBAttributeList attributes = Element.GetAttributes();

	int count = CStr(attributes.GetNamedItem(attr_count)).ToInt();
	CStr var("["+attributes.GetNamedItem(attr_var)+"]");
	if (var.size() < 3)
		var = "[n]";

	for (int n = 0; n < count; ++n)
	{
		NameSubst.emplace_back(var, "[" + CStr::FromInt(n) + "]");

		XERO_ITER_EL(Element, child)
		{
			if (child.GetNodeName() == elmt_object)
				Xeromyces_ReadObject(child, pFile, pParent, NameSubst, Paths, nesting_depth);
		}
		NameSubst.pop_back();
	}
}

void CGUI::Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile, std::unordered_set<VfsPath>& Paths)
{
	// Check for a 'file' parameter
	CStrW file(Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("file")).FromUTF8());

	// If there is a file specified, open and execute it
	if (!file.empty())
	{
		if (!VfsPath(file).IsDirectory())
		{
			Paths.insert(file);
			m_ScriptInterface->LoadGlobalScriptFile(file);
		}
		else
			LOGERROR("GUI: Script path %s is not a file path", file.ToUTF8().c_str());
	}

	// If it has a directory attribute, read all JS files in that directory
	CStrW directory(Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("directory")).FromUTF8());
	if (!directory.empty())
	{
		if (VfsPath(directory).IsDirectory())
		{
			VfsPaths pathnames;
			vfs::GetPathnames(g_VFS, directory, L"*.js", pathnames);
			for (const VfsPath& path : pathnames)
			{
				// Only load new files (so when the insert succeeds)
				if (Paths.insert(path).second)
					m_ScriptInterface->LoadGlobalScriptFile(path);
			}
		}
		else
			LOGERROR("GUI: Script path %s is not a directory path", directory.ToUTF8().c_str());
	}

	CStr code(Element.GetText());
	if (!code.empty())
		m_ScriptInterface->LoadGlobalScript(L"Some XML file", code.FromUTF8());
}

void CGUI::Xeromyces_ReadSprite(XMBElement Element, CXeromyces* pFile)
{
	CGUISprite* Sprite = new CGUISprite;

	// Get name, we know it exists because of DTD requirements
	CStr name = Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("name"));

	if (m_Sprites.find(name) != m_Sprites.end())
		LOGWARNING("GUI sprite name '%s' used more than once; first definition will be discarded", name.c_str());

	// shared_ptr to link the effect to every image, faster than copy.
	std::shared_ptr<SGUIImageEffects> effects;

	for (XMBElement child : Element.GetChildNodes())
	{
		CStr ElementName(pFile->GetElementString(child.GetNodeName()));

		if (ElementName == "image")
			Xeromyces_ReadImage(child, pFile, *Sprite);
		else if (ElementName == "effect")
		{
			if (effects)
				LOGERROR("GUI <sprite> must not have more than one <effect>");
			else
			{
				effects = std::make_shared<SGUIImageEffects>();
				Xeromyces_ReadEffects(child, pFile, *effects);
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

void CGUI::Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite& parent)
{
	SGUIImage* Image = new SGUIImage();

	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
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
		else if (attr_name == "cell_size")
		{
			CSize size;
			if (!ParseString<CSize>(this, attr_value, size))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_CellSize = size;
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
		else if (attr_name == "z_level")
		{
			float z_level;
			if (!ParseString<float>(this, attr_value, z_level))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_DeltaZ = z_level/100.f;
		}
		else if (attr_name == "backcolor")
		{
			if (!ParseString<CGUIColor>(this, attr_value, Image->m_BackColor))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
		}
		else if (attr_name == "bordercolor")
		{
			if (!ParseString<CGUIColor>(this, attr_value, Image->m_BorderColor))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
		}
		else if (attr_name == "border")
		{
			bool b;
			if (!ParseString<bool>(this, attr_value, b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_Border = b;
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	// Look for effects
	for (XMBElement child : Element.GetChildNodes())
	{
		CStr ElementName(pFile->GetElementString(child.GetNodeName()));
		if (ElementName == "effect")
		{
			if (Image->m_Effects)
				LOGERROR("GUI <image> must not have more than one <effect>");
			else
			{
				Image->m_Effects = std::make_shared<SGUIImageEffects>();
				Xeromyces_ReadEffects(child, pFile, *Image->m_Effects);
			}
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	parent.AddImage(Image);
}

void CGUI::Xeromyces_ReadEffects(XMBElement Element, CXeromyces* pFile, SGUIImageEffects& effects)
{
	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));

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

void CGUI::Xeromyces_ReadStyle(XMBElement Element, CXeromyces* pFile)
{
	SGUIStyle style;
	CStr name;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));

		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == "name")
			name = attr.Value;
		else
			style.m_SettingsDefaults.emplace(attr_name, attr.Value.FromUTF8());
	}

	m_Styles.erase(name);
	m_Styles.emplace(name, std::move(style));
}

void CGUI::Xeromyces_ReadScrollBarStyle(XMBElement Element, CXeromyces* pFile)
{
	SGUIScrollBarStyle scrollbar;
	CStr name;

	// Setup some defaults.
	scrollbar.m_MinimumBarSize = 0.f;
	// Using 1.0e10 as a substitute for infinity
	scrollbar.m_MaximumBarSize = 1.0e10;
	scrollbar.m_UseEdgeButtons = false;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name = pFile->GetAttributeString(attr.Name);
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

void CGUI::Xeromyces_ReadIcon(XMBElement Element, CXeromyces* pFile)
{
	SGUIIcon icon;
	CStr name;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else if (attr_name == "sprite")
			icon.m_SpriteName = attr_value;
		else if (attr_name == "size")
		{
			CSize size;
			if (!ParseString<CSize>(this, attr_value.FromUTF8(), size))
				LOGERROR("Error parsing '%s' (\"%s\") inside <icon>.", attr_name, attr_value);
			else
				icon.m_Size = size;
		}
		else if (attr_name == "cell_id")
		{
			int cell_id;
			if (!ParseString<int>(this, attr_value.FromUTF8(), cell_id))
				LOGERROR("GUI: Error parsing '%s' (\"%s\") inside <icon>.", attr_name, attr_value);
			else
				icon.m_CellID = cell_id;
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	m_Icons.erase(name);
	m_Icons.emplace(name, std::move(icon));
}

void CGUI::Xeromyces_ReadTooltip(XMBElement Element, CXeromyces* pFile)
{
	IGUIObject* object = new CTooltip(*this);

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_name == "name")
			object->SetName("__tooltip_" + attr_value);
		else
			object->SetSettingFromString(attr_name, attr_value.FromUTF8(), true);
	}

	if (!AddObject(m_BaseObject, *object))
		delete object;
}

void CGUI::Xeromyces_ReadColor(XMBElement Element, CXeromyces* pFile)
{
	XMBAttributeList attributes = Element.GetAttributes();
	CStr name = attributes.GetNamedItem(pFile->GetAttributeID("name"));

	// Try parsing value
	CStr value(Element.GetText());
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
