#ifndef _ps_scripting_JSMap_H
#define _ps_scripting_JSMap_H

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

#endif // _ps_scripting_JSMap_H
