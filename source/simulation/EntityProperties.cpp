#include "EntityProperties.h"

CGenericProperty::CGenericProperty()
{
	m_type = PROP_INTEGER;
	m_integer = 0;
}

CGenericProperty::~CGenericProperty()
{
	releaseData();
}

void CGenericProperty::releaseData()
{
	switch( m_type & ~PROP_TYPELOCKED )
	{
	case PROP_STRING:
		delete( m_string ); break;
	case PROP_VECTOR:
		delete( m_vector ); break;
	default:
		break;
	}
}

CGenericProperty::operator i32()
{
	return( toInteger() );
}

CGenericProperty::operator float()
{
	return( toFloat() );
}

CGenericProperty::operator CStr()
{
	return( toString() );
}

CGenericProperty::operator CVector3D()
{
	return( toVector() );
}

CGenericProperty::operator void *()
{
	return( toVoid() );
}

CGenericProperty& CGenericProperty::operator=( int32_t value )
{
	if( m_type & PROP_TYPELOCKED )
	{
		fromInteger( value );
	}
	else
	{
		releaseData();
		m_type = PROP_INTEGER;
		m_integer = value;
	}
	return( *this );
}

CGenericProperty& CGenericProperty::operator=( float value )
{
	if( m_type & PROP_TYPELOCKED )
	{
		fromFloat( value );
	}
	else
	{
		releaseData();
		m_type = PROP_FLOAT;
		m_float = value;
	}
	return( *this );
}

CGenericProperty& CGenericProperty::operator=( CStr& value )
{
	if( m_type & PROP_TYPELOCKED )
	{
		fromString( value );
	}
	else
	{
		releaseData();
		m_type = PROP_STRING;
		m_string = new CStr( value );
	}
	return( *this );
}

CGenericProperty& CGenericProperty::operator=( CVector3D& value )
{
	if( m_type & PROP_TYPELOCKED )
	{
		fromVector( value );
	}
	else
	{
		releaseData();
		m_type = PROP_VECTOR;
		m_vector = new CVector3D( value );
	}
	return( *this );
}

CGenericProperty& CGenericProperty::operator =( void* value )
{
	if( m_type & PROP_TYPELOCKED )
	{
		fromVoid( value );
	}
	else
	{
		releaseData();
		m_type = PROP_PTR;
		m_ptr = value;
	}
	return( *this );
}

void CGenericProperty::associate( i32* value )
{
	i32 current = toInteger();
	releaseData();
	m_type = (EPropTypes)( PROP_INTEGER | PROP_INTRINSIC | PROP_TYPELOCKED );
	m_integerptr = value;
	//*m_integerptr = current;
}

void CGenericProperty::associate( float* value )
{
	float current = toFloat();
	releaseData();
	m_type = (EPropTypes)( PROP_FLOAT | PROP_INTRINSIC | PROP_TYPELOCKED );
	m_floatptr = value;
	//*m_floatptr = current;
}

void CGenericProperty::associate( CStr* value )
{
	CStr current = toString();
	releaseData();
	m_type = (EPropTypes)( PROP_STRING | PROP_VECTOR | PROP_TYPELOCKED );
	m_string = value;
	//*m_string = current;
}

void CGenericProperty::associate( CVector3D* value )
{
	CVector3D current = toVector();
	releaseData();
	m_type = (EPropTypes)( PROP_VECTOR | PROP_INTRINSIC | PROP_TYPELOCKED );
	m_vector = value;
	//*value = current;
}

void CGenericProperty::typelock( EPropTypes type )
{
	if( m_type & PROP_INTRINSIC ) return;
	switch( type )
	{
	case PROP_INTEGER:
		{
			i32 current = toInteger();
			releaseData();
			m_integer = current;
		}
		break;
	case PROP_FLOAT:
		{
			float current = toFloat();
			releaseData();
			m_float = current;
		}
		break;
	case PROP_STRING:
		{
			CStr* current = new CStr( toString() );
			releaseData();
			m_string = current;
		}
		break;
	case PROP_VECTOR:
		{
			CVector3D* current = new CVector3D( toVector() );
			releaseData();
			m_vector = current;
		}
		break;
	case PROP_PTR:
		{
			void* current = toVoid();
			releaseData();
			m_ptr = current;
		}
		break;
	default:
		return;
	}
	m_type = (EPropTypes)( type | PROP_TYPELOCKED );
}

void CGenericProperty::typeloose()
{
	if( m_type & PROP_INTRINSIC ) return;
	m_type = (EPropTypes)( m_type & ~PROP_TYPELOCKED );
}

i32& CGenericProperty::asInteger()
{
	assert( ( m_type & PROP_STRIPFLAGS ) == PROP_INTEGER );
	if( m_type & PROP_INTRINSIC )
		return( *m_integerptr );
	return( m_integer );
}

float& CGenericProperty::asFloat()
{
	assert( ( m_type & PROP_STRIPFLAGS ) == PROP_FLOAT );
	if( m_type & PROP_INTRINSIC )
		return( *m_floatptr );
	return( m_float );
}

CStr& CGenericProperty::asString()
{
	assert( ( m_type & PROP_STRIPFLAGS ) == PROP_STRING );
	return( *m_string );
}

CVector3D& CGenericProperty::asVector()
{
	assert( ( m_type & PROP_STRIPFLAGS ) == PROP_VECTOR );
	return( *m_vector );
}

i32 CGenericProperty::toInteger()
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		return( asInteger() );
	case PROP_FLOAT:
		return( (i32)asFloat() );
	case PROP_STRING:
	case PROP_STRING_INTRINSIC:
		return( (i32)( asString().ToInt() ) );
	case PROP_VECTOR:
	case PROP_VECTOR_INTRINSIC:
	case PROP_PTR:
		return( 0 );
	default:
		assert( 0 && "Invalid property type" );
	}
	return( 0 );
}

float CGenericProperty::toFloat()
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		return( (float)asInteger() );
	case PROP_FLOAT:
		return( asFloat() );
	case PROP_STRING:
	case PROP_STRING_INTRINSIC:
		return( asString().ToFloat() );
	case PROP_VECTOR:
	case PROP_VECTOR_INTRINSIC:
	case PROP_PTR:
		return( 0.0f );
	default:
		assert( 0 && "Invalid property type" );
	}
	return( 0.0f );
}

CStr CGenericProperty::toString()
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		return( CStr( asInteger() ) );
	case PROP_FLOAT:
		return( CStr( asFloat() ) );
	case PROP_STRING:
		return( CStr( asString() ) );
	case PROP_VECTOR:
		{
			char buffer[256];
			snprintf( buffer, 250, "{ %f, %f, %f }", asVector().X, asVector().Y, asVector().Z );
			return( CStr( buffer ) );
		}
	case PROP_PTR:
		return( CStr() );
	default:
		assert( 0 && "Invalid property type" );
	}
	return( CStr() );
}

CVector3D CGenericProperty::toVector()
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_VECTOR:
		return( CVector3D( asVector() ) );
	case PROP_INTEGER:
	case PROP_FLOAT:
	case PROP_STRING:
	case PROP_PTR:
		return( CVector3D() );
	default:
		assert( 0 && "Invalid property type" );
	}
	return( CVector3D() );
}

void* CGenericProperty::toVoid()
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_PTR:
		return( m_ptr );
	case PROP_INTEGER:
	case PROP_INTEGER_INTRINSIC:
	case PROP_FLOAT:
	case PROP_FLOAT_INTRINSIC:
	case PROP_STRING:
	case PROP_STRING_INTRINSIC:
	case PROP_VECTOR:
	case PROP_VECTOR_INTRINSIC:
		return( NULL );
	default:
		assert( 0 && "Invalid property type" );
	}
	return( NULL );
}

void CGenericProperty::fromInteger( i32 value )
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		asInteger() = value; return;
	case PROP_FLOAT:
		asFloat() = (float)value; return;
	case PROP_STRING:
		asString() = value; return;
	case PROP_VECTOR:
		asVector() = CVector3D(); return;
	case PROP_PTR:
		m_ptr = NULL; return;
	default:
		assert( 0 && "Invalid property type" );
	}
}

void CGenericProperty::fromFloat( float value )
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		asInteger() = (i32)value; return;
	case PROP_FLOAT:
		asFloat() = value; return;
	case PROP_STRING:
		asString() = value; return;
	case PROP_VECTOR:
		asVector() = CVector3D(); return;
	case PROP_PTR:
		m_ptr = NULL; return;
	default:
		assert( 0 && "Invalid property type" );
	}
}

void CGenericProperty::fromString( CStr& value )
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		asInteger() = value.ToInt(); return;
	case PROP_FLOAT:
		asFloat() = value.ToFloat(); return;
	case PROP_STRING:
		asString() = value; return;
	case PROP_VECTOR:
		asVector() = CVector3D(); return;
	case PROP_PTR:
		m_ptr = NULL; return;
	default:
		assert( 0 && "Invalid property type" );
	}
}

void CGenericProperty::fromVector( CVector3D& value )
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		asInteger() = 0; return;
	case PROP_FLOAT:
		asFloat() = 0.0f; return;
	case PROP_STRING:
		{
			char buffer[256];
			snprintf( buffer, 250, "{ %f, %f, %f }", value.X, value.Y, value.Z );
			asString() = CStr( buffer );
		}
		return;
	case PROP_VECTOR:
		asVector() = CVector3D( value ); return;
	case PROP_PTR:
		m_ptr = NULL; return;
	default:
		assert( 0 && "Invalid property type" );
	}
}

void CGenericProperty::fromVoid( void* value )
{
	switch( m_type & PROP_STRIPFLAGS )
	{
	case PROP_INTEGER:
		asInteger() = 0; return;
	case PROP_FLOAT:
		asFloat() = 0.0f; return;
	case PROP_STRING:
		asString() = CStr(); return;
	case PROP_VECTOR:
		asVector() = CVector3D(); return;
	case PROP_PTR:
		m_ptr = value; return;
	default:
		assert( 0 && "Invalid property type" );
	}
}






	
	

/*

Here lies the old version of CGenericProperty. Will remove it when I know the new one works.

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

CGenericProperty::operator CStr&()
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
*/
