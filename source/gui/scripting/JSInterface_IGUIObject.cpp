#include "precompiled.h"

#include "JSInterface_IGUIObject.h"
#include "JSInterface_GUITypes.h"

#include "gui/IGUIObject.h"
#include "gui/CGUI.h"
#include "gui/CList.h"

#include "ps/CLogger.h"

#include "ps/StringConvert.h"

JSClass JSI_IGUIObject::JSI_class = {
	"GUIObject", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_IGUIObject::getProperty, JSI_IGUIObject::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, JSI_IGUIObject::construct
};

JSPropertySpec JSI_IGUIObject::JSI_props[] = 
{
	{ 0 }
};

JSFunctionSpec JSI_IGUIObject::JSI_methods[] = 
{
	{ "toString", JSI_IGUIObject::toString, 0, 0, 0 },
	{ "getByName", JSI_IGUIObject::getByName, 1, 0, 0 },
	{ 0 }
};

JSBool JSI_IGUIObject::getProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp)
{
	CStr propName = JS_GetStringBytes(JS_ValueToString(cx, id));

	// Skip some things which are known to be functions rather than properties.
	// ("constructor" *must* be here, else it'll try to GetSettingType before
	// the private IGUIObject* has been set (and thus crash). The others are
	// partly for efficiency, and also to allow correct reporting of attempts to
	// access nonexistent properties.)
	if (propName == "constructor" ||
		propName == "prototype"   ||
		propName == "toString"    ||
		propName == "getByName"
	   )
		return JS_TRUE;

	IGUIObject* e = (IGUIObject*)JS_GetPrivate(cx, obj);

	// Use onWhatever to access event handlers
	if (propName.Left(2) == "on")
	{
		CStr eventName (CStr(propName.substr(2)).LowerCase());
		std::map<CStr, JSObject**>::iterator it = e->m_ScriptHandlers.find(eventName);
		if (it == e->m_ScriptHandlers.end())
			*vp = JSVAL_NULL;
		else
			*vp = OBJECT_TO_JSVAL(*(it->second));
		return JS_TRUE;
	}


	// Handle the "parent" property specially
	if (propName == "parent")
	{
		IGUIObject* parent = e->GetParent();
		if (parent)
		{
			// If the object isn't parentless, return a new object
			JSObject* entity = JS_NewObject(cx, &JSI_IGUIObject::JSI_class, NULL, NULL);
			JS_SetPrivate(cx, entity, parent);
			*vp = OBJECT_TO_JSVAL(entity);
		}
		else
		{
			// Return null if there's no parent
			*vp = JSVAL_NULL;
		}
		return JS_TRUE;
	}
	// Also handle "name" specially
	else if (propName == "name")
	{
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, e->GetName()));
		return JS_TRUE;
	}
	// Handle all other properties
	else
	{
		// Retrieve the setting's type (and make sure it actually exists)
		EGUISettingType Type;
		if (e->GetSettingType(propName, Type) != PS_OK)
		{
			JS_ReportError(cx, "Invalid GUIObject property '%s'", propName.c_str());
			return JS_FALSE;
		}

		// (All the cases are in {...} to avoid scoping problems)
		switch (Type)
		{
		case GUIST_bool:
			{
				bool value;
				GUI<bool>::GetSetting(e, propName, value);
				*vp = value ? JSVAL_TRUE : JSVAL_FALSE;
				break;
			}

		case GUIST_int:
			{
				int value;
				GUI<int>::GetSetting(e, propName, value);
				*vp = INT_TO_JSVAL(value);
				break;
			}

		case GUIST_float:
			{
				float value;
				GUI<float>::GetSetting(e, propName, value);
				// Create a garbage-collectable double
				*vp = DOUBLE_TO_JSVAL(JS_NewDouble(cx, value) );
				break;
			}

		case GUIST_CColor:
			{
				CColor colour;
				GUI<CColor>::GetSetting(e, propName, colour);
				JSObject* obj = JS_NewObject(cx, &JSI_GUIColor::JSI_class, NULL, NULL);
				*vp = OBJECT_TO_JSVAL(obj); // root it

				// Attempt to minimise ugliness through macrosity
				#define P(x) jsval x = DOUBLE_TO_JSVAL(JS_NewDouble(cx, colour.x)); JS_SetProperty(cx, obj, #x, &x)
					P(r);
					P(g);
					P(b);
					P(a);
				#undef P

				break;
			}

		case GUIST_CClientArea:
			{
				CClientArea area;
				GUI<CClientArea>::GetSetting(e, propName, area);
				JSObject* obj = JS_NewObject(cx, &JSI_GUISize::JSI_class, NULL, NULL);
				*vp = OBJECT_TO_JSVAL(obj); // root it
				try
				{
				#define P(x, y, z) g_ScriptingHost.SetObjectProperty_Double(obj, #z, area.x.y)
					P(pixel,	left,	left);
					P(pixel,	top,	top);
					P(pixel,	right,	right);
					P(pixel,	bottom,	bottom);
					P(percent,	left,	rleft);
					P(percent,	top,	rtop);
					P(percent,	right,	rright);
					P(percent,	bottom,	rbottom);
				#undef P
				}
				catch (PSERROR_Scripting_ConversionFailed)
				{
					debug_warn("Error creating size object!");
					break;
				}

				break;
			}

		case GUIST_CGUIString:
			{
				CGUIString value;
				GUI<CGUIString>::GetSetting(e, propName, value);
				JSString* s = StringConvert::wchars_to_jsstring(cx, value.GetRawString().c_str());
				*vp = STRING_TO_JSVAL(s);
				break;
			}

		case GUIST_CStr:
			{
				CStr value;
				GUI<CStr>::GetSetting(e, propName, value);
				*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, value));
				break;
			}

		case GUIST_CStrW:
			{
				CStrW value;
				GUI<CStrW>::GetSetting(e, propName, value);
				*vp = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, value.utf16().c_str()));
				break;
			}

		case GUIST_CGUISpriteInstance:
			{
				CGUISpriteInstance *value;
				GUI<CGUISpriteInstance>::GetSettingPointer(e, propName, value);
				*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, value->GetName()));
				break;
			}

		case GUIST_EAlign:
			{
				EAlign value;
				GUI<EAlign>::GetSetting(e, propName, value);
				CStr word;
				switch (value)
				{
				case EAlign_Left: word = "left"; break;
				case EAlign_Right: word = "right"; break;
				case EAlign_Center: word = "center"; break;
				default: debug_warn("Invalid EAlign!"); word = "error"; break;
				}
				*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, word));
				break;
			}

		case GUIST_EVAlign:
			{
				EVAlign value;
				GUI<EVAlign>::GetSetting(e, propName, value);
				CStr word;
				switch (value)
				{
				case EVAlign_Top: word = "top"; break;
				case EVAlign_Bottom: word = "bottom"; break;
				case EVAlign_Center: word = "center"; break;
				default: debug_warn("Invalid EVAlign!"); word = "error"; break;
				}
				*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, word));
				break;
			}

		case GUIST_CGUIList:
			{
				CGUIList value;
				GUI<CGUIList>::GetSetting(e, propName, value);

				JSObject *obj = JS_NewArrayObject(cx, 0, NULL);
				*vp = OBJECT_TO_JSVAL(obj); // root it
				
				for (size_t i = 0; i < value.m_Items.size(); ++i)
				{
					JSString* s = StringConvert::wchars_to_jsstring(cx, value.m_Items[i].GetRawString().c_str());
					jsval val = STRING_TO_JSVAL(s);
					JS_SetElement(cx, obj, (jsint)i, &val);
				}

				break;
			}

		default:
			JS_ReportError(cx, "Setting '%s' uses an unimplemented type", propName.c_str());
			debug_warn("This shouldn't happen");
			return JS_FALSE;
		}

		return JS_TRUE;
	}
}

JSBool JSI_IGUIObject::setProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp)
{
	IGUIObject* e = (IGUIObject*)JS_GetPrivate(cx, obj);
	CStr propName = g_ScriptingHost.ValueToString(id);
	if (propName == "name")
	{
		CStr propValue = JS_GetStringBytes(JS_ValueToString(cx, *vp));
		e->SetName(propValue);

		return JS_TRUE;
	}

	// Use onWhatever to set event handlers
	if (propName.Left(2) == "on")
	{
		if (!JSVAL_IS_OBJECT(*vp) || !JS_ValueToFunction(cx, *vp))
		{
			JS_ReportError(cx, "on- event-handlers must be functions");
			return JS_FALSE;
		}

		CStr eventName (CStr(propName.substr(2)).LowerCase());
		e->SetScriptHandler(eventName, JSVAL_TO_OBJECT(*vp));

		return JS_TRUE;
	}

	// Retrieve the setting's type (and make sure it actually exists)
	EGUISettingType Type;
	if (e->GetSettingType(propName, Type) != PS_OK)
	{
		JS_ReportError(cx, "Invalid setting '%s'", propName.c_str());
		return JS_TRUE;
	}

	switch (Type)
	{

	case GUIST_CStr:
		{
			CStr value (JS_GetStringBytes(JS_ValueToString(cx, *vp)));
			GUI<CStr>::SetSetting(e, propName, value);
			break;
		}

	case GUIST_CStrW:
		{
			utf16string value (JS_GetStringChars(JS_ValueToString(cx, *vp)));
			GUI<CStrW>::SetSetting(e, propName, value);
			break;
		}

	case GUIST_CGUISpriteInstance:
		{
			CStr value (JS_GetStringBytes(JS_ValueToString(cx, *vp)));
			GUI<CGUISpriteInstance>::SetSetting(e, propName, value);
			break;
		}

	case GUIST_CGUIString:
		{
			std::wstring value;
			StringConvert::jsstring_to_wstring(JS_ValueToString(cx, *vp), value);

			CGUIString str;
			str.SetValue(value);
			GUI<CGUIString>::SetSetting(e, propName, str);
			break;
		}

	case GUIST_EAlign:
		{
			CStr value (JS_GetStringBytes(JS_ValueToString(cx, *vp)));
			EAlign a;
			if (value == "left") a = EAlign_Left;
			else if (value == "right") a = EAlign_Right;
			else if (value == "center" || value == "centre") a = EAlign_Center;
			else
			{
				JS_ReportError(cx, "Invalid alignment (should be 'left', 'right' or 'center')");
				return JS_FALSE;
			}
			GUI<EAlign>::SetSetting(e, propName, a);
			break;
		}

	case GUIST_EVAlign:
		{
			CStr value (JS_GetStringBytes(JS_ValueToString(cx, *vp)));
			EVAlign a;
			if (value == "top") a = EVAlign_Top;
			else if (value == "bottom") a = EVAlign_Bottom;
			else if (value == "center" || value == "centre") a = EVAlign_Center;
			else
			{
				JS_ReportError(cx, "Invalid alignment (should be 'top', 'bottom' or 'center')");
				return JS_FALSE;
			}
			GUI<EVAlign>::SetSetting(e, propName, a);
			break;
		}

	case GUIST_int:
		{
			int32 value;
			if (JS_ValueToInt32(cx, *vp, &value) == JS_TRUE)
				GUI<int>::SetSetting(e, propName, value);
			else
			{
				JS_ReportError(cx, "Cannot convert value to int");
				return JS_FALSE;
			}
			break;
		}

	case GUIST_float:
		{
			jsdouble value;
			if (JS_ValueToNumber(cx, *vp, &value) == JS_TRUE)
				GUI<float>::SetSetting(e, propName, (float)value);
			else
			{
				JS_ReportError(cx, "Cannot convert value to float");
				return JS_FALSE;
			}
			break;
		}

	case GUIST_bool:
		{
			JSBool value;
			if (JS_ValueToBoolean(cx, *vp, &value) == JS_TRUE)
				GUI<bool>::SetSetting(e, propName, value||0); // ||0 to avoid int-to-bool compiler warnings
			else
			{
				JS_ReportError(cx, "Cannot convert value to bool");
				return JS_FALSE;
			}
			break;
		}

	case GUIST_CClientArea:
		{
			if (JSVAL_IS_STRING(*vp))
			{
				if (e->SetSetting(propName, JS_GetStringBytes(JS_ValueToString(cx, *vp))) != PS_OK)
				{
					JS_ReportError(cx, "Invalid value for setting '%s'", propName.c_str());
					return JS_FALSE;
				}
			}
			else if (JSVAL_IS_OBJECT(*vp) && JS_GetClass(cx, JSVAL_TO_OBJECT(*vp)) == &JSI_GUISize::JSI_class)
			{
				CClientArea area;
				GUI<CClientArea>::GetSetting(e, propName, area);

				JSObject* obj = JSVAL_TO_OBJECT(*vp);
				#define P(x, y, z) area.x.y = (float)g_ScriptingHost.GetObjectProperty_Double(obj, #z)
					P(pixel,	left,	left);
					P(pixel,	top,	top);
					P(pixel,	right,	right);
					P(pixel,	bottom,	bottom);
					P(percent,	left,	rleft);
					P(percent,	top,	rtop);
					P(percent,	right,	rright);
					P(percent,	bottom,	rbottom);
				#undef P

				GUI<CClientArea>::SetSetting(e, propName, area);
			}
			else
			{
				JS_ReportError(cx, "Size only accepts strings or GUISize objects");
				return JS_FALSE;
			}
			break;
		}

	case GUIST_CColor:
		{
			if (JSVAL_IS_STRING(*vp))
			{
				if (e->SetSetting(propName, JS_GetStringBytes(JS_ValueToString(cx, *vp))) != PS_OK)
				{
					JS_ReportError(cx, "Invalid value for setting '%s'", propName.c_str());
					return JS_FALSE;
				}
			}
			else if (JSVAL_IS_OBJECT(*vp) && JS_GetClass(cx, JSVAL_TO_OBJECT(*vp)) == &JSI_GUIColor::JSI_class)
			{
				CColor colour;
				JSObject* obj = JSVAL_TO_OBJECT(*vp);
				jsval t; double s;
				#define PROP(x) JS_GetProperty(cx, obj, #x, &t); \
								JS_ValueToNumber(cx, t, &s); \
								colour.x = (float)s
				PROP(r); PROP(g); PROP(b); PROP(a);
				#undef PROP

				GUI<CColor>::SetSetting(e, propName, colour);
			}
			else
			{
				JS_ReportError(cx, "Color only accepts strings or GUIColor objects");
				return JS_FALSE;
			}
			break;
		}

	case GUIST_CGUIList:
		{
			JSObject* obj = JSVAL_TO_OBJECT(*vp);
			jsuint length;
			if (JSVAL_IS_OBJECT(*vp) && JS_GetArrayLength(cx, obj, &length) == JS_TRUE)
			{
				CGUIList list;

				for (int i=0; i<(int)length; ++i)
				{
					jsval element;
					if (! JS_GetElement(cx, obj, i, &element))
					{
						JS_ReportError(cx, "Failed to get list element");
						return JS_FALSE;
					}

					std::wstring value;
					JSString* string = JS_ValueToString(cx, element);
					StringConvert::jsstring_to_wstring(string, value);

					CGUIString str;
					str.SetValue(value);
					
					list.m_Items.push_back(str);
				}

				GUI<CGUIList>::SetSetting(e, propName, list);
			}
			else
			{
				JS_ReportError(cx, "List only accepts a GUIList object");
				return JS_FALSE;
			}
			break;
		}

		// TODO Gee: (2004-09-01) EAlign and EVAlign too.

	default:
		JS_ReportError(cx, "Setting '%s' uses an unimplemented type", propName.c_str());
		break;
	}

	return JS_TRUE;
}


JSBool JSI_IGUIObject::construct(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* UNUSED(rval))
{
	if (argc == 0)
	{
		JS_ReportError(cx, "GUIObject has no default constructor");
		return JS_FALSE;
	}

	debug_assert(argc == 1);

	// Store the IGUIObject in the JS object's 'private' area
	IGUIObject* guiObject = (IGUIObject*)JSVAL_TO_PRIVATE(argv[0]);
	JS_SetPrivate(cx, obj, guiObject);

	return JS_TRUE;
}


JSBool JSI_IGUIObject::getByName(JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval)
{
	debug_assert(argc == 1);

	CStr objectName = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));

	IGUIObject* guiObject = g_GUI.FindObjectByName(objectName);

	if (!guiObject)
	{
		// Not found - return null
		*rval = JSVAL_NULL;
		return JS_TRUE;
	}

	*rval = OBJECT_TO_JSVAL(guiObject->GetJSObject());
	return JS_TRUE;
}


void JSI_IGUIObject::init()
{
	g_ScriptingHost.DefineCustomObjectType(&JSI_class, construct, 1, JSI_props, JSI_methods, NULL, NULL);
}

JSBool JSI_IGUIObject::toString(JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval)
{
	IGUIObject* e = (IGUIObject*)JS_GetPrivate( cx, obj );

	char buffer[256];
	snprintf(buffer, 256, "[GUIObject: %s]", e->GetName().c_str());
	buffer[255] = 0;
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer));
	return JS_TRUE;
}
