#include "precompiled.h"

#include "EntityProperties.h"
#include "BaseEntityCollection.h"
#include "scripting/JSInterface_BaseEntity.h"

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
		m_data = g_ScriptingHost.ValueToInt( value );
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
		m_data = g_ScriptingHost.ValueToBool( value );
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
		m_data = (float)g_ScriptingHost.ValueToDouble( value );
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
		*this = g_ScriptingHost.ValueToUCString( value );
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

void CBoundProperty<CBaseEntity*>::set( const jsval value )
{
	JSObject* baseEntity = JSVAL_TO_OBJECT( value );
	CBaseEntity* base = NULL;
	if( JSVAL_IS_OBJECT( value ) && ( base = (CBaseEntity*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), baseEntity, &JSI_BaseEntity::JSI_class, NULL ) ) )
	{
		m_data = base;
	}
	else
		JS_ReportError( g_ScriptingHost.getContext(), "[BaseEntity] Invalid reference" );
}

jsval CBoundProperty<CBaseEntity*>::tojsval()
{
	JSObject* baseEntity = JS_NewObject( g_ScriptingHost.getContext(), &JSI_BaseEntity::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), baseEntity, m_data );
	return( OBJECT_TO_JSVAL( baseEntity ) );
}


void IBoundPropertyOwner::rebuild( CStrW propertyName )
{
	IBoundProperty* thisProperty = m_properties[propertyName];
	IBoundProperty* baseProperty = NULL;
	if( m_base )
	{
		if( m_base->m_properties.find( propertyName ) != m_base->m_properties.end() )
			baseProperty = m_base->m_properties[propertyName];
	}
	if( thisProperty->rebuild( baseProperty ) )
	{
		std::vector<IBoundPropertyOwner*>::iterator it;
		for( it = m_inheritors.begin(); it != m_inheritors.end(); it++ )
			(*it)->rebuild( propertyName );
	}
}

void IBoundPropertyOwner::rebuild()
{
	STL_HASH_MAP<CStrW,IBoundProperty*,CStrW_hash_compare>::iterator property;
	if( m_base )
	{
		for( property = m_properties.begin(); property != m_properties.end(); property++ )
		{
			IBoundProperty* baseProperty = NULL;
			if( m_base->m_properties.find( property->first ) != m_base->m_properties.end() )
				baseProperty = m_base->m_properties[property->first];
			(property->second)->rebuild( baseProperty, false );
		}
	}
	else
	{
		for( property = m_properties.begin(); property != m_properties.end(); property++ )
			(property->second)->rebuild( NULL, false );
	}

	std::vector<IBoundPropertyOwner*>::iterator it;
	for( it = m_inheritors.begin(); it != m_inheritors.end(); it++ )
		(*it)->rebuild();

}
