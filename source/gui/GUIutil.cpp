/* Copyright (C) 2009 Wildfire Games.
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
GUI utilities
*/

#include "precompiled.h"
#include "GUI.h"
#include "ps/Parser.h"
#include "ps/i18n.h"

extern int g_yres;


template <>
bool __ParseString<bool>(const CStr& Value, bool &Output)
{
	if (Value == "true")
		Output = true;
	else
	if (Value == "false")
		Output = false;
	else 
		return false;

	return true;
}

template <>
bool __ParseString<int>(const CStr& Value, int &Output)
{
	Output = Value.ToInt();
	return true;
}

template <>
bool __ParseString<float>(const CStr& Value, float &Output)
{
	Output = Value.ToFloat();
	return true;
}

template <>
bool __ParseString<CRect>(const CStr& Value, CRect &Output)
{
	// Use the parser to parse the values
	CParser& parser (CParserCache::Get("_$value_$value_$value_$value_"));

	std::string str = Value;

	CParserLine line;
	line.ParseString(parser, str);
	if (!line.m_ParseOK)
	{
		// Parsing failed
		return false;
	}
	float values[4];
	for (int i=0; i<4; ++i)
	{
		if (!line.GetArgFloat(i, values[i]))
		{
			// Parsing failed
			return false;
		}
	}

	// Finally the rectangle values
	Output = CRect(values[0], values[1], values[2], values[3]);
	return true;
}

template <>
bool __ParseString<CClientArea>(const CStr& Value, CClientArea &Output)
{
	return Output.SetClientArea(Value);
}

template <>
bool GUI<int>::ParseColor(const CStr& Value, CColor &Output, float DefaultAlpha)
{
	// First, check our database in g_GUI for pre-defined colors
	//  If we find anything, we'll ignore DefaultAlpha
#ifdef g_GUI
	// If it fails, it won't do anything with Output
	if (g_GUI.GetPreDefinedColor(Value, Output))
		return true;

#endif // g_GUI

	return Output.ParseString(Value, DefaultAlpha);
}


template <>
bool __ParseString<CColor>(const CStr& Value, CColor &Output)
{
	// First, check our database in g_GUI for pre-defined colors
#ifdef g_GUI
	// If it fails, it won't do anything with Output
	if (g_GUI.GetPreDefinedColor(Value, Output))
		return true;

#endif // g_GUI

	return Output.ParseString(Value, 255.f);
}

template <>
bool __ParseString<CSize>(const CStr& Value, CSize &Output)
{
	// Use the parser to parse the values
	CParser& parser (CParserCache::Get("_$value_$value_"));

	std::string str = Value;

	CParserLine line;
	line.ParseString(parser, str);
	if (!line.m_ParseOK)
	{
		// Parsing failed
		return false;
	}

	float x, y;

	// x
	if (!line.GetArgFloat(0, x))
	{
		// TODO Gee: Parsing failed
		return false;
	}

	// y
	if (!line.GetArgFloat(1, y))
	{
		// TODO Gee: Parsing failed
		return false;
	}

	Output.cx = x;
	Output.cy = y;
	
	return true;
}

template <>
bool __ParseString<CPos>(const CStr& Value, CPos &Output)
{
	CSize temp;
	if (__ParseString<CSize>(Value, temp))
	{
		Output = CPos(temp);
		return true;
	}
	else
		return false;
}

template <>
bool __ParseString<EAlign>(const CStr& Value, EAlign &Output)
{
	if (Value == "left")
		Output = EAlign_Left;
	else
	if (Value == "center")
		Output = EAlign_Center;
	else
	if (Value == "right")
		Output = EAlign_Right;
	else
		return false;

	return true;
}

template <>
bool __ParseString<EVAlign>(const CStr& Value, EVAlign &Output)
{
	if (Value == "top")
		Output = EVAlign_Top;
	else
	if (Value == "center")
		Output = EVAlign_Center;
	else
	if (Value == "bottom")
		Output = EVAlign_Bottom;
	else
		return false;

	return true;
}

template <>
bool __ParseString<CGUIString>(const CStr& Value, CGUIString &Output)
{
	// Translate the Value and retrieve the localised string in
	//  Unicode.

	Output.SetValue(I18n::translate((CStrW)Value));
	return true;
}

template <>
bool __ParseString<CStr>(const CStr& Value, CStr&Output)
{
	// Do very little.
	Output = Value;
	return true;
}

template <>
bool __ParseString<CStrW>(const CStr& Value, CStrW& Output)
{
	// Translate the Value and retrieve the localised string in
	//  Unicode.

	Output = I18n::translate((CStrW)Value);
	return true;
}

template <>
bool __ParseString<CGUISpriteInstance>(const CStr& Value, CGUISpriteInstance &Output)
{
	Output = Value;
	return true;
}

template <>
bool __ParseString<CGUIList>(const CStr& UNUSED(Value), CGUIList& UNUSED(Output))
{
	return false;
}


//--------------------------------------------------------

void guiLoadIdentity()
{
	glLoadIdentity();
	glTranslatef(0.0f, (GLfloat)g_yres, -1000.0f);
	glScalef(1.0f, -1.f, 1.0f);
}

//--------------------------------------------------------
//  Utilities implementation
//--------------------------------------------------------
IGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(CGUI &GUIinstance, const CStr& Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}

const IGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(const CGUI &GUIinstance, const CStr& Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}

void CInternalCGUIAccessorBase::QueryResetting(IGUIObject *pObject)
{
	GUI<>::RecurseObject(0, pObject, &IGUIObject::ResetStates);
}

void CInternalCGUIAccessorBase::HandleMessage(IGUIObject *pObject, const SGUIMessage &message)
{
	pObject->HandleMessage(message);		
}



#ifndef NDEBUG
	#define TYPE(T) \
		template<> void CheckType<T>(const IGUIObject* obj, const CStr& setting) {	\
			std::map<CStr, SGUISetting>::const_iterator it = obj->m_Settings.find(setting);	\
			if (it == obj->m_Settings.end() || it->second.m_Type != GUIST_##T)	\
			{	\
				/* Abort now, to avoid corrupting everything by invalidly \
					casting pointers */ \
				DEBUG_DISPLAY_ERROR(L"FATAL ERROR: Inconsistent types in GUI");	\
			}	\
		}
	#include "GUItypes.h"
	#undef TYPE
#endif


//--------------------------------------------------------------------

template <typename T>
PS_RESULT GUI<T>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value)
{
	if (pObject == NULL)
		throw PSERROR_GUI_NullObjectProvided();

	std::map<CStr, SGUISetting>::const_iterator it = pObject->m_Settings.find(Setting);
	if (it == pObject->m_Settings.end())
		return PS_FAIL;

	if (it->second.m_pSetting == NULL)
		return PS_FAIL;

#ifndef NDEBUG
	CheckType<T>(pObject, Setting);
#endif

	// Get value
	Value = (T*)(it->second.m_pSetting);

	return PS_OK;
}

template <typename T>
PS_RESULT GUI<T>::GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value)
{
	T* v;
	PS_RESULT ret = GetSettingPointer(pObject, Setting, v);
	if (ret == PS_OK)
		Value = *v;
	return ret;
}

template <typename T>
PS_RESULT GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, 
							 const T &Value, const bool& SkipMessage)
{
	if (pObject == NULL)
		throw PSERROR_GUI_NullObjectProvided();

	if (!pObject->SettingExists(Setting))
		throw PSERROR_GUI_InvalidSetting();

#ifndef NDEBUG
	CheckType<T>(pObject, Setting);
#endif

	// Set value
	*(T*)pObject->m_Settings[Setting].m_pSetting = Value;

	//
	//	Some settings needs special attention at change
	//

	// If setting was "size", we need to re-cache itself and all children
	if (Setting == "size")
	{
		RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);
	}
	else
	if (Setting == "hidden")
	{
		// Hiding an object requires us to reset it and all children
		QueryResetting(pObject);
		//RecurseObject(0, pObject, IGUIObject::ResetStates);
	}

	if (!SkipMessage)
		HandleMessage(pObject, SGUIMessage(GUIM_SETTINGS_UPDATED, Setting));

	return PS_OK;
}

// Instantiate templated functions:
#define TYPE(T) \
	template PS_RESULT GUI<T>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value); \
	template PS_RESULT GUI<T>::GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value); \
	template PS_RESULT GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, const T &Value, const bool& SkipMessage);
#define GUITYPE_IGNORE_CGUISpriteInstance
#include "GUItypes.h"

// Don't instantiate GetSetting<CGUISpriteInstance> - this will cause linker errors if
// you attempt to retrieve a sprite using GetSetting, since that copies the sprite
// and will mess up the caching performed by DrawSprite. You have to use GetSettingPointer
// instead. (This is mainly useful to stop me accidentally using the wrong function.)
template PS_RESULT GUI<CGUISpriteInstance>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, CGUISpriteInstance* &Value);
template PS_RESULT GUI<CGUISpriteInstance>::SetSetting(IGUIObject *pObject, const CStr& Setting, const CGUISpriteInstance &Value, const bool& SkipMessage);
