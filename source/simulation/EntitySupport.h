// Supporting data types for CEntity and related

#ifndef ENTITY_SUPPORT_INCLUDED
#define ENTITY_SUPPORT_INCLUDED

#include "EntityHandles.h"
#include "ScriptObject.h"

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
	}
	static void ScriptingInit()
	{
		AddProperty<float>( L"crush", &CDamageType::m_Crush );
		AddProperty<float>( L"hack", &CDamageType::m_Hack );
		AddProperty<float>( L"pierce", &CDamageType::m_Pierce );
		AddProperty<float>( L"typeless", &CDamageType::m_Typeless );
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

struct SEntityAction
{
	float m_MaxRange;
	float m_MinRange;
	size_t m_Speed;
	SEntityAction() { m_MaxRange = m_MinRange = 0.0f; m_Speed = 1000; }
};

struct SClassSet
{
	SClassSet* m_Parent;

	typedef STL_HASH_SET<CStrW, CStrW_hash_compare> ClassSet;

	ClassSet m_Set;
	std::vector<CStrW> m_Added;
	std::vector<CStrW> m_Removed;

	inline SClassSet() { m_Parent = NULL; }

	inline bool IsMember( CStrW Test )
		{ return( m_Set.find( Test ) != m_Set.end() ); }

	inline void SetParent( SClassSet* Parent )
		{ m_Parent = Parent; Rebuild(); }

	void Rebuild()
	{
		if( m_Parent )
			m_Set = m_Parent->m_Set;
		else
			m_Set.clear();

		std::vector<CStrW>::iterator it;
		for( it = m_Removed.begin(); it != m_Removed.end(); it++ )
			m_Set.erase( *it );
		for( it = m_Added.begin(); it != m_Added.end(); it++ )
			m_Set.insert( *it );
	}
};

struct SAuraData
{
	enum Allegiance
	{
		SELF = 1,
		PLAYER = 2,
		GAIA = 4,
		ALLY = 8,
		ENEMY = 16,
	};	
	static const ssize_t DURATION_RADIUS = -1;
	static const ssize_t DURATION_PERMANENT = -2;
	SClassSet::ClassSet m_Affects;
	Allegiance m_Allegiance;
	float m_Radius;
	size_t m_Time;
	size_t m_Cooldown;
	float m_Hitpoints;
	size_t m_Duration;
	bool m_Cumulative;
	CScriptObject m_BeginEffect;
	CScriptObject m_EndEffect;
};

struct SAuraInstance
{
	HEntity m_Influenced;
	size_t m_EnteredRange;
	size_t m_LastInRange;
	size_t m_Applied;
	inline CEntity* GetEntity() const { return( m_Influenced ); }
};

struct SAura
{
	std::vector<SAuraInstance> m_Influenced;
	SAuraData m_Data;
	size_t m_Recharge;
};

#endif
