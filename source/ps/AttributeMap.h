#ifndef _ps_AttributeMap_H
#define _ps_AttributeMap_H

#include "scripting/ScriptingHost.h"
// Needed for STL_HASH_MAP detection
#include "EntityProperties.h"

namespace AttributeMap_JS
{
	extern JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	extern JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	extern JSClass Class;
	JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
};

class CAttributeMap
{
public:
	typedef void (UpdateCallback)(CStr name, CStrW newValue, void *data);
	typedef STL_HASH_MAP<CStr, CStrW, CStr_hash_compare> MapType;

private:
	typedef STL_HASH_MAP<CStr, CStrW, CStr_hash_compare> JSCallbackMapType;
	JSCallbackMapType m_JSCallbacks;

	UpdateCallback *m_UpdateCB;
	void *m_UpdateCBUserData;

	void CallJSCallback(CStrW script);

protected:
	MapType m_Attributes;
	JSObject *m_JSObject;

	virtual void CreateJSObject();
	void AddValue(CStr name, CStrW value);

public:
	CAttributeMap();
	virtual ~CAttributeMap();

	CStrW GetValue(CStr name) const;
	void SetValue(CStr name, CStrW value);

	virtual JSBool GetJSProperty(jsval id, jsval *ret);
	virtual JSBool SetJSProperty(jsval id, jsval value);

	inline MapType &GetInternalValueMap()
	{	return m_Attributes; }

	inline void SetUpdateCallback(UpdateCallback *cb, void *userdata)
	{	
		m_UpdateCB=cb;
		m_UpdateCBUserData=userdata;
	}

	inline JSObject *GetJSObject()
	{
		if (!m_JSObject)
			CreateJSObject();
		return m_JSObject;
	}
};

#endif
