#include "precompiled.h"

/*
#include "EntityProperties.h"
#include "BaseEntityCollection.h"
#include "simulation/scripting/JSInterface_BaseEntity.h"

#undef new // to avoid confusing warnings

void IBoundProperty::associate( IBoundPropertyOwner* owner, const CStrW& name )
{
	m_owner = owner;
	owner->m_properties[name] = this;
	m_updateFn = NULL;
}

void IBoundProperty::associate( IBoundPropertyOwner* owner, const CStrW& name, void (IBoundPropertyOwner::*updateFn)() )
{
	m_owner = owner;
	owner->m_properties[name] = this;
	m_updateFn = updateFn;
}

void IBoundProperty::fromjsval( const jsval value )
{
	set( value );
	if( m_updateFn )
		(m_owner->*m_updateFn)();
}

// --- 



// --- 

void CBoundProperty<int>::set( const jsval value )
{
	try
	{
		m_data = ToPrimitive<int>( value );
		m_inherited = false;
	}
	catch( ... )
	{
	}
}

jsval CBoundProperty<int>::tojsval()
{
	return( INT_TO_JSVAL( m_data ) );
}

void CBoundProperty<bool>::set( const jsval value )
{
	try
	{
		m_data = ToPrimitive<bool>( value );
		m_inherited = false;
	}
	catch( ... )
	{
	}
}

jsval CBoundProperty<bool>::tojsval()
{
	return( BOOLEAN_TO_JSVAL( m_data ) );
}

void CBoundProperty<float>::set( const jsval value )
{
	try
	{
		m_data = ToPrimitive<float>( value );
		m_inherited = false;
	}
	catch( ... )
	{
	}
}

jsval CBoundProperty<float>::tojsval()
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), (jsdouble)m_data ) ) );
}

void CBoundObjectProperty<CStr>::set( const jsval value )
{
	try
	{
		m_String = g_ScriptingHost.ValueToString( value );
		m_inherited = false;
	}
	catch( ... )
	{
	}
}

jsval CBoundObjectProperty<CStr>::tojsval()
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.getContext(), m_String.c_str() ) ) );
}

void CBoundObjectProperty<CStrW>::set( const jsval value )
{
	try
	{
		m_String = g_ScriptingHost.ValueToUCString( value );
		m_inherited = false;
	}
	catch( ... )
	{
	}
}

jsval CBoundObjectProperty<CStrW>::tojsval()
{
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.getContext(), utf16().c_str() ) ) );
}

CBoundObjectProperty<CVector3D>::CBoundObjectProperty()
{
	m_inherited = false;
}

void CBoundObjectProperty<CVector3D>::set( const jsval value )
{
	JSObject* vector3d = JSVAL_TO_OBJECT( value );
	JSI_Vector3D::Vector3D_Info* v = NULL;
	if( JSVAL_IS_OBJECT( value ) && ( v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), vector3d, &JSI_Vector3D::JSI_class, NULL ) ) )
	{
		CVector3D* copy = v->vector;
		X = copy->X;
		Y = copy->Y;
		Z = copy->Z;
		m_inherited = false;
	}
	else
	{
		X = 0.0f; Y = 0.0f; Z = 0.0f;
	}
}

jsval CBoundObjectProperty<CVector3D>::tojsval()
{
	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( this, m_owner, m_updateFn ) );
	return( OBJECT_TO_JSVAL( vector3d ) );
}

bool CBoundObjectProperty<CVector3D>::rebuild( IBoundProperty* parent, bool triggerFn )
{
	return( false );
}

//--

void CBoundObjectProperty<CScriptObject>::set( const jsval value )
{
	switch( JS_TypeOfValue( g_ScriptingHost.GetContext(), value ) )
	{
	case JSTYPE_STRING:
		CompileScript( L"", g_ScriptingHost.ValueToUCString( value ) );
		m_inherited = false;
		break;
	case JSTYPE_FUNCTION:
		Function = JS_ValueToFunction( g_ScriptingHost.GetContext(), value );
		m_inherited = false;
		break;
	}
}

jsval CBoundObjectProperty<CScriptObject>::tojsval()
{
	if( Function )
		return( OBJECT_TO_JSVAL( JS_GetFunctionObject( Function ) ) );
	return( JSVAL_NULL );
}

//--

void CBoundProperty<CBaseEntity*>::set( const jsval value )
{
	JSObject* baseEntity = JSVAL_TO_OBJECT( value );
	CBaseEntity* base = NULL;

	if( JSVAL_IS_OBJECT( value ) && ( base = ToNative<CBaseEntity>( g_ScriptingHost.GetContext(), baseEntity ) ) )
	{
		m_data = base;
	}
	else
		JS_ReportError( g_ScriptingHost.getContext(), "[BaseEntity] Invalid reference" );
}

jsval CBoundProperty<CBaseEntity*>::tojsval()
{
	JSObject* baseEntity = m_data->GetScript();
	return( OBJECT_TO_JSVAL( baseEntity ) );
}



*/
