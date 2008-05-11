// Functions for (de)serialization of jsvals

// WIP, not yet functional

#include "network/Serialization.h"
#include "JSConversions.h"
#include "ps/CStr.h"

class jsval_ser : public ISerializable
{
	enum
	{
		TAG_BOOLEAN_FALSE,
		TAG_BOOLEAN_TRUE,
		TAG_INT,
		TAG_DOUBLE,
		TAG_STRING,
		TAG_NOT_SERIALIZABLE = -1
	} m_tag; 
	jsval m_data;
public:
	jsval_ser() : m_tag( TAG_NOT_SERIALIZABLE )
	{
	}
	jsval_ser( jsval data ) : m_data( data )
	{
		if( m_data == JSVAL_FALSE )
			m_tag = TAG_BOOLEAN_FALSE;
		if( m_data == JSVAL_TRUE )
			m_tag = TAG_BOOLEAN_TRUE;
		if( JSVAL_IS_INT( m_data ) )
			m_tag = TAG_INT;
		if( JSVAL_IS_DOUBLE( m_data ) )
			m_tag = TAG_DOUBLE;
		if( JSVAL_IS_STRING( m_data ) )
			m_tag = TAG_STRING;
		m_tag = TAG_NOT_SERIALIZABLE;
	}
	operator jsval() const
	{
		return( m_data );
	}
	operator CStr() const
	{
		return( ToPrimitive<CStrW>( m_data ) );
	}
	size_t GetSerializedLength() const
	{
		switch( m_tag )
		{
		case TAG_BOOLEAN_FALSE:
		case TAG_BOOLEAN_TRUE:
			return( 1 );
		case TAG_INT:
			return( 5 );
		case TAG_DOUBLE:
			return( 9 );
		case TAG_STRING:
			return( 1 + (ToPrimitive<CStrW>(m_data)).GetSerializedLength() );
		default:
			debug_warn("An attempt was made to serialize a jsval other than a number, boolean or string." );
			return( 1 );
		}
	}
	u8* Serialize( u8* buffer ) const
	{
		Serialize_int_1( buffer, m_tag );
		switch( m_tag )
		{
		case TAG_BOOLEAN_FALSE:
		case TAG_BOOLEAN_TRUE:
			break;
		case TAG_INT:
			{
				u32 ival = JSVAL_TO_INT( m_data );
				Serialize_int_4( buffer, ival );
			}
			break;
		case TAG_DOUBLE:
			{
				union {
					u64 ival;
					double dval;
				} val;
				cassert(sizeof(u64) == sizeof(double));
				val.dval = *JSVAL_TO_DOUBLE( m_data );
				Serialize_int_8( buffer, val.ival );
			}
			break;
		case TAG_STRING:
			buffer = ( ToPrimitive<CStrW>( m_data ) ).Serialize( buffer );
			break;
		default:
			debug_warn("An attempt was made to serialize a jsval other than a number, boolean or string." );
			break;
		}
		return( buffer );
	}
	const u8* Deserialize( const u8* buffer, const u8* end )
	{
		Deserialize_int_1( buffer, (u8&)m_tag );
		switch( m_tag )
		{
		case TAG_BOOLEAN_FALSE:
			m_data = JSVAL_FALSE;
			break;
		case TAG_BOOLEAN_TRUE:
			m_data = JSVAL_TRUE;
			break;
		case TAG_INT:
			{
				u32 ival;
				Deserialize_int_4( buffer, ival );
				m_data = INT_TO_JSVAL( ival );
			}
			break;
		case TAG_DOUBLE:
			{
				union {
					u64 ival;
					double dval;
				} val;
				cassert(sizeof(u64) == sizeof(double));
				Deserialize_int_8( buffer, val.ival );
				JS_NewDoubleValue( g_ScriptingHost.GetContext(), val.dval, &m_data );
			}
			break;
		case TAG_STRING:
			{
				CStrW ival;
				buffer = ival.Deserialize( buffer, end );
				m_data = ToJSVal<CStrW>( ival );
			}
			break;
		default:
			debug_warn("An attempt was made to deserialize a jsval other than a number, boolean or string." );
			break;
		}
		return( buffer );
	}
};
