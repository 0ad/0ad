/*
GUI utilities
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "Parser.h"
#include "i18n.h"

using namespace std;

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

	string str = (const TCHAR*)Value;

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
bool __ParseString<CColor>(const CStr& Value, CColor &Output)
{
	// Use the parser to parse the values
	CParser& parser (CParserCache::Get("_[-$arg(_minus)]$value_[-$arg(_minus)]$value_[-$arg(_minus)]$value_[[-$arg(_minus)]$value_]"));

	string str = Value;

	CParserLine line;
	line.ParseString(parser, str);
	if (!line.m_ParseOK)
	{
		// TODO Gee: Parsing failed
		return false;
	}
	float values[4];
	values[3] = 255.f; // default
	for (int i=0; i<(int)line.GetArgCount(); ++i)
	{
		if (!line.GetArgFloat(i, values[i]))
		{
			// Parsing failed
			return false;
		}
	}

	Output.r = values[0]/255.f;
	Output.g = values[1]/255.f;
	Output.b = values[2]/255.f;
	Output.a = values[3]/255.f;
	
	return true;
}

template <>
bool __ParseString<CSize>(const CStr& Value, CSize &Output)
{
	// Use the parser to parse the values
	CParser& parser (CParserCache::Get("_$value_$value_"));

	string str = (const TCHAR*)Value;

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
bool __ParseString<EAlign>(const CStr &Value, EAlign &Output)
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
bool __ParseString<EVAlign>(const CStr &Value, EVAlign &Output)
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

	Output.SetValue(translate((CStrW)Value));
	return true;
}

template <>
bool __ParseString<CStr>(const CStr& Value, CStr &Output)
{
	// Do very little.
	Output = Value;
	return true;
}

template <>
bool __ParseString<CStrW>(const CStr& Value, CStrW &Output)
{
	// Translate the Value and retrieve the localised string in
	//  Unicode.

	Output = translate((CStrW)Value);
	return true;
}

template <>
bool __ParseString<CGUISpriteInstance>(const CStr& Value, CGUISpriteInstance &Output)
{
	Output = Value;
	return true;
}

//--------------------------------------------------------
//  Help Classes/Structs for the GUI implementation
//--------------------------------------------------------

CClientArea::CClientArea() : pixel(0.f,0.f,0.f,0.f), percent(0.f,0.f,0.f,0.f)
{
}

CClientArea::CClientArea(const CStr& Value)
{
	SetClientArea(Value);
}

CRect CClientArea::GetClientArea(const CRect &parent) const
{
	// If it's a 0 0 100% 100% we need no calculations
	if (percent == CRect(0.f,0.f,100.f,100.f) && pixel == CRect(0.f,0.f,0.f,0.f))
		return parent;

	CRect client;

	// This should probably be cached and not calculated all the time for every object.
    client.left =	parent.left + (parent.right-parent.left)*percent.left/100.f + pixel.left;
	client.top =	parent.top + (parent.bottom-parent.top)*percent.top/100.f + pixel.top;
	client.right =	parent.left + (parent.right-parent.left)*percent.right/100.f + pixel.right;
	client.bottom =	parent.top + (parent.bottom-parent.top)*percent.bottom/100.f + pixel.bottom;

	return client;
}

bool CClientArea::SetClientArea(const CStr& Value)
{
	// Get value in STL string format
	string _Value = (const TCHAR*)Value;

	// This might lack incredible speed, but since all XML files
	//  are read at startup, reading 100 client areas will be
	//  negligible in the loading time.

	// Setup parser to parse the value

	// One of the four values:
	//  will give outputs like (in argument):
	//  (200) <== no percent, just the first $value
	//  (200) (percent) <== just the percent
	//  (200) (percent) (100) <== percent PLUS pixel
	//  (200) (percent) (-100) <== percent MINUS pixel
	//  (200) (percent) (100) (-100) <== Both PLUS and MINUS are used, INVALID
	/*
	string one_value = "_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]";
	string four_values = one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)_"; // it's easier to just end with another delimiter
	*/
	// Don't use the above strings, because they make this code go very slowly
	const char* four_values =
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_";
	CParser& parser (CParserCache::Get(four_values));

    CParserLine line;
	line.ParseString(parser, _Value);

	if (!line.m_ParseOK)
		return false;

	int arg_count[4]; // argument counts for the four values
	int arg_start[4] = {0,0,0,0}; // location of first argument, [0] is always 0

	// Divide into the four piles (delimiter is an argument named "delim")
	for (int i=0, valuenr=0; i<(int)line.GetArgCount(); ++i)
	{
		string str;
		line.GetArgString(i, str);
		if (str == "delim")
		{
			if (valuenr==0)
			{
				arg_count[0] = i;
				arg_start[1] = i+1;
			}
			else
			{
				if (valuenr!=3)
				{
					assert(valuenr <= 2);
					arg_start[valuenr+1] = i+1;
					arg_count[valuenr] = arg_start[valuenr+1] - arg_start[valuenr] - 1;
				}
				else
					arg_count[3] = (int)line.GetArgCount() - arg_start[valuenr] - 1;
			}

			++valuenr;
		}
	}

	// Iterate argument
	
	// This is the scheme:
	// 1 argument = Just pixel value
	// 2 arguments = Just percent value
	// 3 arguments = percent and pixel
	// 4 arguments = INVALID

  	// Default to 0
	float values[4][2] = {{0.f,0.f},{0.f,0.f},{0.f,0.f},{0.f,0.f}};
	for (int v=0; v<4; ++v)
	{
		if (arg_count[v] == 1)
		{
			string str;
			line.GetArgString(arg_start[v], str);

			if (!line.GetArgFloat(arg_start[v], values[v][1]))
				return false;
		}
		else
		if (arg_count[v] == 2)
		{
			if (!line.GetArgFloat(arg_start[v], values[v][0]))
				return false;
		}
		else
		if (arg_count[v] == 3)
		{
			if (!line.GetArgFloat(arg_start[v], values[v][0]) ||
				!line.GetArgFloat(arg_start[v]+2, values[v][1]))
				return false;

		}
		else return false;
	}

	// Now store the values[][] in the right place
	pixel.left =		values[0][1];
	pixel.top =			values[1][1];
	pixel.right =		values[2][1];
	pixel.bottom =		values[3][1];
	percent.left =		values[0][0];
	percent.top =		values[1][0];
	percent.right =		values[2][0];
	percent.bottom =	values[3][0];
	return true;
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
			if (((IGUIObject*)obj)->m_Settings[setting].m_Type != GUIST_##T)	\
			{	\
				debug_warn("EXCESSIVELY FATAL ERROR: Inconsistent types in GUI");	\
				throw "EXCESSIVELY FATAL ERROR: Inconsistent types in GUI";	/* TODO: better reporting */ \
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
		return PS_OBJECT_FAIL;

	if (!pObject->SettingExists(Setting))
		return PS_SETTING_FAIL;

	if (!pObject->m_Settings.find(Setting)->second.m_pSetting)
		return PS_FAIL;

#ifndef NDEBUG
	CheckType<T>(pObject, Setting);
#endif

	// Get value
	Value = (T*)pObject->m_Settings.find(Setting)->second.m_pSetting;

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
PS_RESULT GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, const T &Value)
{
	if (pObject == NULL)
		return PS_OBJECT_FAIL;

	if (!pObject->SettingExists(Setting))
		return PS_SETTING_FAIL;

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

	HandleMessage(pObject, SGUIMessage(GUIM_SETTINGS_UPDATED, Setting));

	return PS_OK;
}

// Instantiate templated functions:
#define TYPE(T) \
	template PS_RESULT GUI<T>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, T* &Value); \
	template PS_RESULT GUI<T>::GetSetting(const IGUIObject *pObject, const CStr& Setting, T &Value); \
	template PS_RESULT GUI<T>::SetSetting(IGUIObject *pObject, const CStr& Setting, const T &Value);
#define GUITYPE_IGNORE_CGUISpriteInstance
#include "GUItypes.h"

// Don't instantiate GetSetting<CGUISpriteInstance> - this will cause linker errors if
// you attempt to retrieve a sprite using GetSetting, since that copies the sprite
// and will mess up the caching performed by DrawSprite. You have to use GetSettingPointer
// instead. (This is mainly useful to stop me accidentally using the wrong function.)
template PS_RESULT GUI<CGUISpriteInstance>::GetSettingPointer(const IGUIObject *pObject, const CStr& Setting, CGUISpriteInstance* &Value);
template PS_RESULT GUI<CGUISpriteInstance>::SetSetting(IGUIObject *pObject, const CStr& Setting, const CGUISpriteInstance &Value);
