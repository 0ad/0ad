// Supporting data types for CEntity and related

class CEntityManager;

class CDamageType : public CJSObject<CDamageType>
{
public:
	float m_Crush;
	float m_Hack;
	float m_Pierce;
	float m_Typeless;
	CDamageType()
	{
		Init( 0.0f, 0.0f, 0.0f, 0.0f );
	}
	virtual ~CDamageType() {}
	CDamageType( float Crush, float Hack, float Pierce )
	{
		Init( Crush, Hack, Pierce, 0.0f );
	}
	CDamageType( float Typeless )
	{
		Init( 0.0f, 0.0f, 0.0f, Typeless );
	}
	CDamageType( float Crush, float Hack, float Pierce, float Typeless )
	{
		Init( Crush, Hack, Pierce, Typeless );
	}
	void Init( float Crush, float Hack, float Pierce, float Typeless )
	{
		m_Crush = Crush;
		m_Hack = Hack;
		m_Pierce = Pierce;
		m_Typeless = Typeless;
		AddProperty<float>( L"crush", &m_Crush );
		AddProperty<float>( L"hack", &m_Hack );
		AddProperty<float>( L"pierce", &m_Pierce );
		AddProperty<float>( L"typeless", &m_Typeless );
	}
	static void ScriptingInit()
	{
		CJSObject<CDamageType>::ScriptingInit( "DamageType", Construct, 3 );
	}
	static JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		CDamageType* dt;

		if( argc == 0 )
			dt = new CDamageType();
		else if( argc == 1 )
			dt = new CDamageType( ToPrimitive<float>( argv[0] ) );
		else if( argc == 3 )
			dt = new CDamageType( ToPrimitive<float>( argv[0] ),
								  ToPrimitive<float>( argv[1] ),
								  ToPrimitive<float>( argv[2] ) );
		else if( argc == 4 )
			dt = new CDamageType( ToPrimitive<float>( argv[0] ),
								  ToPrimitive<float>( argv[1] ),
								  ToPrimitive<float>( argv[2] ),
								  ToPrimitive<float>( argv[3] ) );
		else
			return( JS_FALSE );

		dt->m_EngineOwned = false; // Let this object be deallocated when JS GCs it.
	
		*rval = OBJECT_TO_JSVAL( dt->GetScript() );

		return( JS_TRUE );
	}
};
