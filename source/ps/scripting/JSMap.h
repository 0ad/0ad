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

#ifndef INCLUDED_JSMAP
#define INCLUDED_JSMAP

/*
	A simple read-only JS wrapper for map types (STL associative containers).
	Has been tested with integer keys. Writeability shouldn't be all that hard
	to add, it's just not needed by the current users of the class.
*/
template <typename T, typename KeyType = typename T::key_type>
class CJSMap
{
	T *m_pInstance;
	JSObject *m_JSObject;
	
	typedef typename T::iterator iterator;

	static JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSMap<T, KeyType> *object=ToNative<CJSMap<T, KeyType> >(cx, obj);
		T *pInstance = object->m_pInstance;
		KeyType key=ToPrimitive<KeyType>(id);
	
		iterator it;
		it=pInstance->find(key);
		if (it == pInstance->end())
			return JS_FALSE;
		
		*vp=OBJECT_TO_JSVAL(ToScript(it->second));
		return JS_TRUE;
	}

	static JSBool SetProperty( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* UNUSED(vp) )
	{
		return JS_FALSE;
	}

	void CreateScriptObject()
	{
		if( !m_JSObject )
		{
			m_JSObject = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
			JS_AddRoot( g_ScriptingHost.GetContext(), (void*)&m_JSObject );		
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JSObject, this );
		}
	}
	
	void ReleaseScriptObject()
	{
		if( m_JSObject )
		{
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JSObject, NULL );
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &m_JSObject );
			m_JSObject = NULL;
		}
	}

public:
	CJSMap(T* pInstance):
		m_pInstance(pInstance),
		m_JSObject(NULL)
	{
	}
	
	~CJSMap()
	{
		ReleaseScriptObject();
	}

	JSObject *GetScript()
	{
		if (!m_JSObject)
			CreateScriptObject();
		return m_JSObject;
	}
	
	static void ScriptingInit(const char *className)
	{
		JSI_class.name=className;
		g_ScriptingHost.DefineCustomObjectType(&JSI_class,
			NULL, 0, NULL, NULL, NULL, NULL);
	}

	// Needs to be public for ToScript/ToNative
	static JSClass JSI_class;
};

template<typename T, typename KeyType>
JSClass CJSMap<T, KeyType>::JSI_class =
{
	NULL, JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	GetProperty, SetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL
};

#endif // INCLUDED_JSMAP
