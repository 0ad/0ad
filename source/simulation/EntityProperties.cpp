#include "EntityProperties.h"

CGenericProperty::CGenericProperty()
{
	m_type = PROP_INTEGER;
	m_integer = 0;
}

CGenericProperty::CGenericProperty( i32 value )
{
	m_type = PROP_INTEGER;
	m_integer = value;
}

CGenericProperty::CGenericProperty( float value )
{
	m_type = PROP_FLOAT;
	m_float = value;
}

CGenericProperty::CGenericProperty( CStr& value )
{
	m_type = PROP_STRING;
	m_string = new CStr( value );
}

CGenericProperty::CGenericProperty( CVector3D& value )
{
	m_type = PROP_VECTOR;
	m_vector = new CVector3D( value );
}

CGenericProperty::CGenericProperty( void* value )
{
	m_type = PROP_PTR;
	m_ptr = value;
}

CGenericProperty::CGenericProperty( i32* value )
{
	m_type = PROP_INTEGER_INTRINSIC;
	m_integerptr = value;
}

CGenericProperty::CGenericProperty( float* value )
{
	m_type = PROP_FLOAT_INTRINSIC;
	m_floatptr = value;
}

CGenericProperty::CGenericProperty( CStr* value )
{
	m_type = PROP_STRING_INTRINSIC;
	m_string = value;
}

CGenericProperty::CGenericProperty( CVector3D* value )
{
	m_type = PROP_VECTOR_INTRINSIC;
	m_vector = value;
}

CGenericProperty::~CGenericProperty()
{
	switch( m_type )
	{
	case PROP_STRING:
		delete( m_string ); break;
	case PROP_VECTOR:
		delete( m_vector ); break;
	default:
		break;
	}
}

CGenericProperty::operator CStr()
{
	char working[64];
	switch( m_type )
	{
	case PROP_STRING:
	case PROP_STRING_INTRINSIC:
		return( *m_string );
	case PROP_VECTOR:
	case PROP_VECTOR_INTRINSIC:
		snprintf( working, 63, "{ %f, %f, %f }", m_vector->X, m_vector->Y, m_vector->Z );
		working[63] = 0;
		return( CStr( working ) );
	case PROP_INTEGER:
		return( CStr( m_integer ) );
	case PROP_INTEGER_INTRINSIC:
		return( CStr( *m_integerptr ) );
	case PROP_FLOAT:
		return( CStr( m_float ) );
	case PROP_FLOAT_INTRINSIC:
		return( CStr( *m_floatptr ) );
	default:
		return( CStr() );
	}
}

CGenericProperty::operator CVector3D()
{
	switch( m_type )
	{
	case PROP_VECTOR:
		return( *m_vector );
	default:
		return( CVector3D() );
	}
}

CGenericProperty::operator i32()
{
	switch( m_type )
	{
	case PROP_INTEGER:
		return( m_integer );
	case PROP_INTEGER_INTRINSIC:
		return( *m_integerptr );
	case PROP_FLOAT:
		return( (i32)m_float );
	case PROP_FLOAT_INTRINSIC:
		return( (i32)*m_floatptr );
	case PROP_STRING:
		return( m_string->ToInt() );
	default:
		return( 0 );
	}
}

CGenericProperty::operator float()
{
	switch( m_type )
	{
	case PROP_INTEGER:
		return( (float)m_integer );
	case PROP_INTEGER_INTRINSIC:
		return( (float)*m_integerptr );
	case PROP_FLOAT:
		return( m_float );
	case PROP_FLOAT_INTRINSIC:
		return( *m_floatptr );
	case PROP_STRING:
		return( m_string->ToFloat() );
	default:
		return( 0.0f );
	}
}

CGenericProperty::operator void*()
{
	switch( m_type )
	{
	case PROP_PTR:
		return( m_ptr );
	default:
		return( NULL );
	}
}
