#include "precompiled.h"

#include "AttributeMap.h"

namespace AttributeMap_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CAttributeMap *pInstance=(CAttributeMap *)(intptr_t)JS_GetPrivate(cx, obj);

		return pInstance->GetJSProperty(id, vp);
	}

	JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CAttributeMap *pInstance=(CAttributeMap *)(intptr_t)JS_GetPrivate(cx, obj);
		return pInstance->SetJSProperty(id, *vp);
	}

	JSClass Class = {
		"AttributeMap", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}
};

JSBool CAttributeMap::SetJSProperty(jsval id, jsval v)
{
	CStr propName = g_ScriptingHost.ValueToString(id);
	CStrW value = g_ScriptingHost.ValueToUCString(v);
	MapType::iterator it=m_Attributes.find(propName);
	if (it != m_Attributes.end())
	{
		SetValue(propName, value);
		return JS_TRUE;
	}
	else
		return JS_FALSE;
}

JSBool CAttributeMap::GetJSProperty(jsval id, jsval *vp)
{
	CStr propName = g_ScriptingHost.ValueToString(id);

	MapType::const_iterator it=m_Attributes.find(propName);
	if (it == m_Attributes.end())
		return JS_FALSE;
	else
	{
		CStrW value=it->second;
		*vp=g_ScriptingHost.UCStringToValue(value.utf16());
		return JS_TRUE;
	}
}

void CAttributeMap::CreateJSObject()
{
	ONCE(
		g_ScriptingHost.DefineCustomObjectType(&AttributeMap_JS::Class, AttributeMap_JS::Construct, 0, NULL, NULL, NULL, NULL);
	);
	m_JSObject=g_ScriptingHost.CreateCustomObject("AttributeMap");
	JS_SetPrivate(g_ScriptingHost.getContext(), m_JSObject, (void *)this);
}

void CAttributeMap::CallJSCallback(CStrW script)
{
	g_ScriptingHost.ExecuteScript(script, L"AttributeMap Callback Script", m_JSObject);
}

void CAttributeMap::AddValue(CStr name, CStrW value)
{
	m_Attributes[name]=value;
	m_JSCallbacks[name]=L"";
}

// Default implementation: just set the value
void CAttributeMap::SetValue(CStr name, CStrW value)
{
	MapType::iterator it=m_Attributes.find(name);
	assert(it != m_Attributes.end());
	it->second=value;

	if (m_UpdateCB)
		m_UpdateCB(name, value, m_UpdateCBUserData);
	JSCallbackMapType::const_iterator cbit=m_JSCallbacks.find(name);
	if (cbit->second.Length())
		CallJSCallback(cbit->second);
}

CStrW CAttributeMap::GetValue(CStr name) const
{
	MapType::const_iterator it=m_Attributes.find(name);
	assert(it != m_Attributes.end());
	return it->second;
}

CAttributeMap::CAttributeMap():
	m_UpdateCB(NULL),
	m_UpdateCBUserData(NULL),
	m_JSObject(NULL)
{
}

CAttributeMap::~CAttributeMap()
{}
