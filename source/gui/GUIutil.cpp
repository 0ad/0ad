/* Copyright (C) 2014 Wildfire Games.
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
#include "GUIManager.h"
#include "maths/Matrix3D.h"

extern int g_xres, g_yres;

#include "ps/CLogger.h"


template <>
bool __ParseString<bool>(const CStrW& Value, bool &Output)
{
	if (Value == L"true")
		Output = true;
	else
	if (Value == L"false")
		Output = false;
	else 
		return false;

	return true;
}

template <>
bool __ParseString<int>(const CStrW& Value, int &Output)
{
	Output = Value.ToInt();
	return true;
}

template <>
bool __ParseString<float>(const CStrW& Value, float &Output)
{
	Output = Value.ToFloat();
	return true;
}

template <>
bool __ParseString<CRect>(const CStrW& Value, CRect &Output)
{
	const unsigned int NUM_COORDS = 4;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; i++)
	{
		if (stream.eof())
		{
			LOGWARNING(L"Too few CRect parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING(L"Unable to parse CRect parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	if (!stream.eof())
	{
		LOGWARNING(L"Too many CRect parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	// Finally the rectangle values
	Output = CRect(coords[0], coords[1], coords[2], coords[3]);

	return true;
}

template <>
bool __ParseString<CClientArea>(const CStrW& Value, CClientArea &Output)
{
	return Output.SetClientArea(Value.ToUTF8());
}

template <>
bool GUI<int>::ParseColor(const CStrW& Value, CColor &Output, int DefaultAlpha)
{
	// First, check our database in g_GUI for pre-defined colors
	//  If we find anything, we'll ignore DefaultAlpha
	// If it fails, it won't do anything with Output
	if (g_GUI->GetPreDefinedColor(Value.ToUTF8(), Output))
		return true;

	return Output.ParseString(Value.ToUTF8(), DefaultAlpha);
}


template <>
bool __ParseString<CColor>(const CStrW& Value, CColor &Output)
{
	// First, check our database in g_GUI for pre-defined colors
	// If it fails, it won't do anything with Output
	if (g_GUI->GetPreDefinedColor(Value.ToUTF8(), Output))
		return true;

	return Output.ParseString(Value.ToUTF8());
}

template <>
bool __ParseString<CSize>(const CStrW& Value, CSize &Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; i++)
	{
		if (stream.eof())
		{
			LOGWARNING(L"Too few CSize parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING(L"Unable to parse CSize parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.cx = coords[0];
	Output.cy = coords[1];

	if (!stream.eof())
	{
		LOGWARNING(L"Too many CSize parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool __ParseString<CPos>(const CStrW& Value, CPos &Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; i++)
	{
		if (stream.eof())
		{
			LOGWARNING(L"Too few CPos parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING(L"Unable to parse CPos parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.x = coords[0];
	Output.y = coords[1];

	if (!stream.eof())
	{
		LOGWARNING(L"Too many CPos parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool __ParseString<EAlign>(const CStrW& Value, EAlign &Output)
{
	if (Value == L"left")
		Output = EAlign_Left;
	else
	if (Value == L"center")
		Output = EAlign_Center;
	else
	if (Value == L"right")
		Output = EAlign_Right;
	else
		return false;

	return true;
}

template <>
bool __ParseString<EVAlign>(const CStrW& Value, EVAlign &Output)
{
	if (Value == L"top")
		Output = EVAlign_Top;
	else
	if (Value == L"center")
		Output = EVAlign_Center;
	else
	if (Value == L"bottom")
		Output = EVAlign_Bottom;
	else
		return false;

	return true;
}

template <>
bool __ParseString<CGUIString>(const CStrW& Value, CGUIString &Output)
{
	// TODO: i18n: Might want to translate the Value perhaps

	Output.SetValue(Value);
	return true;
}

template <>
bool __ParseString<CStr>(const CStrW& Value, CStr& Output)
{
	// Do very little.
	Output = Value.ToUTF8();
	return true;
}

template <>
bool __ParseString<CStrW>(const CStrW& Value, CStrW& Output)
{
	// TODO: i18n: Might want to translate the Value perhaps

	Output = Value;
	return true;
}

template <>
bool __ParseString<CGUISpriteInstance>(const CStrW& Value, CGUISpriteInstance &Output)
{
	Output = CGUISpriteInstance(Value.ToUTF8());
	return true;
}

template <>
bool __ParseString<CGUIList>(const CStrW& UNUSED(Value), CGUIList& UNUSED(Output))
{
	return false;
}


//--------------------------------------------------------

CMatrix3D GetDefaultGuiMatrix()
{
	CMatrix3D m;
	m.SetIdentity();
	m.Scale(1.0f, -1.f, 1.0f);
	m.Translate(0.0f, (float)g_yres, -1000.0f);

	CMatrix3D proj;
	proj.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	m = proj * m;

	return m;
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

void CInternalCGUIAccessorBase::HandleMessage(IGUIObject *pObject, SGUIMessage &message)
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
PSRETURN GUI<T>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value)
{
	ENSURE(pObject != NULL);

	std::map<CStr, SGUISetting>::const_iterator it = pObject->m_Settings.find(Setting);
	if (it == pObject->m_Settings.end())
	{
		LOGWARNING(L"setting %hs was not found on object %hs", 
			Setting.c_str(),
			pObject->GetPresentableName().c_str());
		return PSRETURN_GUI_InvalidSetting;
	}

	if (it->second.m_pSetting == NULL)
		return PSRETURN_GUI_InvalidSetting;

#ifndef NDEBUG
	CheckType<T>(pObject, Setting);
#endif

	// Get value
	Value = (T*)(it->second.m_pSetting);

	return PSRETURN_OK;
}

template <typename T>
PSRETURN GUI<T>::GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value)
{
	T* v = NULL;
	PSRETURN ret = GetSettingPointer(pObject, Setting, v);
	if (ret == PSRETURN_OK)
		Value = *v;
	return ret;
}

// Helper function for SetSetting
template <typename T>
bool IsBoolTrue(const T&)
{
	return false;
}
template <>
bool IsBoolTrue<bool>(const bool& v)
{
	return v;
}

template <typename T>
PSRETURN GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, 
							 const T &Value, const bool& SkipMessage)
{
	ENSURE(pObject != NULL);

	if (!pObject->SettingExists(Setting))
	{
		LOGWARNING(L"setting %hs was not found on object %hs", 
			Setting.c_str(),
			pObject->GetPresentableName().c_str());
		return PSRETURN_GUI_InvalidSetting;
	}

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
		if (IsBoolTrue(Value))
			QueryResetting(pObject);
	}

	if (!SkipMessage)
	{
		SGUIMessage msg(GUIM_SETTINGS_UPDATED, Setting);
		HandleMessage(pObject, msg);
	}

	return PSRETURN_OK;
}

// Instantiate templated functions:
#define TYPE(T) \
	template PSRETURN GUI<T>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value); \
	template PSRETURN GUI<T>::GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value); \
	template PSRETURN GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, const T &Value, const bool& SkipMessage);
#define GUITYPE_IGNORE_CGUISpriteInstance
#include "GUItypes.h"
#undef GUITYPE_IGNORE_CGUISpriteInstance
#undef TYPE

// Don't instantiate GetSetting<CGUISpriteInstance> - this will cause linker errors if
// you attempt to retrieve a sprite using GetSetting, since that copies the sprite
// and will mess up the caching performed by DrawSprite. You have to use GetSettingPointer
// instead. (This is mainly useful to stop me accidentally using the wrong function.)
template PSRETURN GUI<CGUISpriteInstance>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, CGUISpriteInstance* &Value);
template PSRETURN GUI<CGUISpriteInstance>::SetSetting(IGUIObject *pObject, const CStr& Setting, const CGUISpriteInstance &Value, const bool& SkipMessage);
