#include "precompiled.h"

#include "EntityProperties.h"
#include "BaseEntityCollection.h"
#include "scripting/JSInterface_BaseEntity.h"

#undef new // to avoid confusing warnings

void CProperty::associate( IPropertyOwner* owner, const CStr& name )
{
	m_owner = owner;
	owner->m_properties[name] = this;
	m_updateFn = NULL;
}

void CProperty::associate( IPropertyOwner* owner, const CStr& name, void (IPropertyOwner::*updateFn)() )
{
	m_owner = owner;
	owner->m_properties[name] = this;
	m_updateFn = updateFn;
}

CProperty& CProperty::operator=( jsval value )
{
	set( value );
	return( *this );
}

CProperty_i32::CProperty_i32()
{
	modifier = NULL;
}

CProperty_i32::~CProperty_i32()
{
	if( modifier )
		delete( modifier );
}

CProperty_i32& CProperty_i32::operator =( i32 value )
{
	if( !modifier )
		modifier = new SProperty_NumericModifier();
	*modifier = (float)value;
	data = value;
	return( *this );
}

void CProperty_i32::set( jsval value )
{
	if( !modifier )
		modifier = new SProperty_NumericModifier();
	try
	{
		*modifier = (float)g_ScriptingHost.ValueToInt( value );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		*modifier = 0;
	}
}

bool CProperty_i32::rebuild( CProperty* parent, bool triggerFn )
{
	CProperty_i32* _parent = (CProperty_i32*)parent;
	i32 newvalue = 0;
	if( _parent )
		newvalue = *_parent;
	if( modifier )
	{
		newvalue = (i32)(newvalue * modifier->multiplicative);
		newvalue += (i32)modifier->additive;
	}
	if( data == newvalue )
		return( false );		// No change.
	data = newvalue;
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( true );
}

inline CProperty_i32::operator i32()
{
	return( data );
}

jsval CProperty_i32::tojsval()
{
	return( INT_TO_JSVAL( data ) );
}

CProperty_bool::CProperty_bool()
{
	modifier = NULL;
}

CProperty_bool::~CProperty_bool()
{
	if( modifier )
		delete( modifier );
}

CProperty_bool& CProperty_bool::operator=( const bool value )
{
	if( !modifier )
		modifier = new SProperty_BooleanModifier();
	*modifier = value;
	data = value;
	return( *this );
}

void CProperty_bool::set( jsval value )
{
	if( !modifier )
		modifier = new SProperty_BooleanModifier();
	try
	{
		*modifier = g_ScriptingHost.ValueToBool( value );
	}
	catch( ... )
	{
		*modifier = false;
	}
}

bool CProperty_bool::rebuild( CProperty* parent, bool triggerFn )
{
	CProperty_bool* _parent = (CProperty_bool*)parent;
	bool newvalue = false;
	if( _parent )
		newvalue = *_parent;
	if( modifier )
		newvalue = modifier->replacement;
	if( data == newvalue )
		return( false );		// No change.
	data = newvalue;
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( true );
}

inline CProperty_bool::operator bool()
{
	return( data );
}

jsval CProperty_bool::tojsval()
{
	return( BOOLEAN_TO_JSVAL( data ) );
}

CProperty_float::CProperty_float()
{
	modifier = NULL;
}

CProperty_float::~CProperty_float()
{
	if( modifier )
		delete modifier;
}

CProperty_float& CProperty_float::operator =( const float& value )
{
	if( !modifier )
		modifier = new SProperty_NumericModifier();
	*modifier = value;
	data = value;
	return( *this );
}

void CProperty_float::set( const jsval value )
{
	if( !modifier )
		modifier = new SProperty_NumericModifier();
	try
	{
		*modifier = (float)g_ScriptingHost.ValueToDouble( value );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		*modifier = 0.0f;
	}
}

bool CProperty_float::rebuild( CProperty* parent, bool triggerFn )
{
	CProperty_float* _parent = (CProperty_float*)parent;
	float newvalue = 0;
	if( _parent )
		newvalue = *_parent;
	if( modifier )
	{
		newvalue *= modifier->multiplicative;
		newvalue += modifier->additive;
	}
	if( data == newvalue )
		return( false );		// No change.
	data = newvalue;
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( true );
}

CProperty_float::operator float()
{
	return( data );
}

jsval CProperty_float::tojsval()
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), (jsdouble)data ) ) );
}

CProperty_float::operator bool()
{
	return( data != 0.0f);
}

float CProperty_float::operator+( float value )
{
	return( data + value );
}

float CProperty_float::operator-( float value )
{
	return( data - value );
}

float CProperty_float::operator*( float value )
{
	return( data * value );
}

float CProperty_float::operator/( float value )
{
	return( data / value );
}

bool CProperty_float::operator<( float value )
{
	return( data < value );
}

bool CProperty_float::operator>( float value )
{
	return( data > value );
}

bool CProperty_float::operator==( float value )
{
	return( data == value );
}

CProperty_CStr::CProperty_CStr()
{
	modifier = NULL;
}

CProperty_CStr::~CProperty_CStr()
{
	if( modifier )
		delete( modifier );
}

CProperty_CStr& CProperty_CStr::operator=( const CStr& value )
{
	if( !modifier )
		modifier = new SProperty_StringModifier();
	*modifier = value;
	m_String = value;
	return( *this );
}

void CProperty_CStr::set( jsval value )
{
	if( !modifier )
		modifier = new SProperty_StringModifier();
	try
	{
		*modifier = g_ScriptingHost.ValueToString( value );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		*modifier = CStr();
		m_String.clear();
	}
}

bool CProperty_CStr::rebuild( CProperty* parent, bool triggerFn )
{
	CProperty_CStr* _parent = (CProperty_CStr*)parent;
	CStr newvalue = "";
	if( _parent )
		newvalue = *_parent;
	if( modifier )
		newvalue = modifier->replacement;

	if( *this == newvalue )
		return( false );		// No change.
	m_String = newvalue;
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( true );
}

jsval CProperty_CStr::tojsval()
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.getContext(), m_String.c_str() ) ) );
}

CProperty_CVector3D& CProperty_CVector3D::operator =( const CVector3D& value )
{
	*( (CVector3D*)this ) = value;
	return( *this );
}

void CProperty_CVector3D::set( jsval value )
{
	JSObject* vector3d = JSVAL_TO_OBJECT( value );
	JSI_Vector3D::Vector3D_Info* v = NULL;
	if( JSVAL_IS_OBJECT( value ) && ( v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), vector3d, &JSI_Vector3D::JSI_class, NULL ) ) )
	{
		CVector3D* copy = v->vector;
		X = copy->X;
		Y = copy->Y;
		Z = copy->Z;
	}
	else
	{
		X = 0.0f; Y = 0.0f; Z = 0.0f;
	}
}

bool CProperty_CVector3D::rebuild( CProperty* parent, bool triggerFn )
{
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( false ); //	Vector properties aren't inheritable.
}

jsval CProperty_CVector3D::tojsval()
{
	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( this, m_owner, m_updateFn ) );
	return( OBJECT_TO_JSVAL( vector3d ) );
}

CProperty_CBaseEntityPtr& CProperty_CBaseEntityPtr::operator =( CBaseEntity* value )
{
	data = value;
	return( *this );
}

void CProperty_CBaseEntityPtr::set( jsval value )
{
	JSObject* baseEntity = JSVAL_TO_OBJECT( value );
	CBaseEntity* base = NULL;
	if( JSVAL_IS_OBJECT( value ) && ( base = (CBaseEntity*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), baseEntity, &JSI_BaseEntity::JSI_class, NULL ) ) )
	{
		data = base;
	}
	else
		JS_ReportError( g_ScriptingHost.getContext(), "[BaseEntity] Invalid reference" );

}

bool CProperty_CBaseEntityPtr::rebuild( CProperty* parent, bool triggerFn )
{
	if( triggerFn && m_updateFn ) (m_owner->*m_updateFn)();
	return( false ); // CBaseEntity* properties aren't inheritable.
}

jsval CProperty_CBaseEntityPtr::tojsval()
{
	JSObject* baseEntity = JS_NewObject( g_ScriptingHost.getContext(), &JSI_BaseEntity::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), baseEntity, data );
	return( OBJECT_TO_JSVAL( baseEntity ) );
}

CProperty_CBaseEntityPtr::operator bool()
{
	return( data != NULL );
}

CProperty_CBaseEntityPtr::operator CBaseEntity*()
{
	return( data );
}

CBaseEntity& CProperty_CBaseEntityPtr::operator *() const
{
	return( *data );
}

CBaseEntity* CProperty_CBaseEntityPtr::operator ->() const
{
	return( data );
}

void IPropertyOwner::rebuild( CStr propertyName )
{
	CProperty* thisProperty = m_properties[propertyName];
	CProperty* baseProperty = NULL;
	if( m_base )
	{
		if( m_base->m_properties.find( propertyName ) != m_base->m_properties.end() )
			baseProperty = m_base->m_properties[propertyName];
	}
	if( thisProperty->rebuild( baseProperty ) )
	{
		std::vector<IPropertyOwner*>::iterator it;
		for( it = m_inheritors.begin(); it != m_inheritors.end(); it++ )
			(*it)->rebuild( propertyName );
	}
}

void IPropertyOwner::rebuild()
{
	STL_HASH_MAP<CStr,CProperty*,CStr_hash_compare>::iterator property;
	if( m_base )
	{
		for( property = m_properties.begin(); property != m_properties.end(); property++ )
		{
			CProperty* baseProperty = NULL;
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

	std::vector<IPropertyOwner*>::iterator it;
	for( it = m_inheritors.begin(); it != m_inheritors.end(); it++ )
		(*it)->rebuild();

}
