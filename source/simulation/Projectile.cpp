#include "precompiled.h"

#include "Projectile.h"
#include "Entity.h"
#include "graphics/Model.h"
#include "graphics/Unit.h"
#include "maths/Matrix3D.h"
#include "ScriptObject.h"
#include "ps/Game.h"
#include "Collision.h"
#include "graphics/ObjectManager.h"
#include "ps/CLogger.h"

const double GRAVITY = 0.00001;
const double GRAVITY_2 = GRAVITY * 0.5;

CProjectile::CProjectile( const CModel* Actor, const CVector3D& Position, const CVector3D& Target, float Speed, CEntity* Originator, const CScriptObject& ImpactScript, const CScriptObject& MissScript )
{
	m_Actor = Actor->Clone();
	m_Position = m_Position_Previous = m_Position_Graphics = Position;
	m_Speed_H = Speed;
	m_Originator = Originator;
	m_ImpactEventHandler = ImpactScript;
	m_MissEventHandler = MissScript;

	AddHandler( EVENT_IMPACT, &m_ImpactEventHandler );
	AddHandler( EVENT_MISS, &m_MissEventHandler );

	// That was the easy stuff.
	// We want horizontal distance only:

	m_Axis = Target - Position;
	double s = m_Axis.length();
	m_Axis /= (float)s; 

	// Now vertical distance:
	double d_h = Target.Y - Position.Y;
	// Time of impact:
	double t = s / m_Speed_H;
	// Required vertical velocity at launch:
	m_Speed_V = (float)( d_h / t + GRAVITY_2 * t );
}

CProjectile::~CProjectile()
{
	std::vector<CProjectile*>::iterator it;
	for( it = g_ProjectileManager.m_Projectiles.begin(); it != g_ProjectileManager.m_Projectiles.end(); ++it )
		if( *it == this )
		{
			g_ProjectileManager.m_Projectiles.erase( it );
			break;
		}
	delete( m_Actor );
}

bool CProjectile::Update( size_t timestep_millis )
{
	m_Position_Previous = m_Position;
	m_Position.X += timestep_millis * m_Axis.x * m_Speed_H;
	m_Position.Z += timestep_millis * m_Axis.y * m_Speed_H;
	
	m_Position.Y += (float)( timestep_millis * ( m_Speed_V - timestep_millis * GRAVITY_2 ) );
	m_Speed_V -= (float)( timestep_millis * GRAVITY );

	float height = m_Position.Y - g_Game->GetWorld()->GetTerrain()->getExactGroundLevel( m_Position.X, m_Position.Z );
	
	if( height < 0.0f )
	{
		// We appear to have missed.
		CEventProjectileMiss evt( m_Originator, m_Position );
		DispatchEvent( &evt );
		// Not going to let this be cancelled.
		return( false );
	}
	RayIntersects& r = GetProjectileIntersection( m_Position_Previous, m_Axis, timestep_millis * m_Speed_H );
	RayIntersects::iterator it;
	for( it = r.begin(); it != r.end(); it++ )
	{
		// Hit something?
		if( *it != m_Originator ) /* That wouldn't be fair at all... */
		{
			// Low enough to hit it?
			if( height < (*it)->m_bounds->m_height )
			{
				CEventProjectileImpact evt( m_Originator, *it, m_Position );
				if( DispatchEvent( &evt ) )
					return( false );
			}
		}
	}
	return( true );
}

void CProjectile::Interpolate( size_t timestep_millis )
{
	m_Position_Graphics.X = m_Position_Previous.X + timestep_millis * m_Speed_H * m_Axis.x;
	m_Position_Graphics.Z = m_Position_Previous.Z + timestep_millis * m_Speed_H * m_Axis.y;
	m_Position_Graphics.Y = (float)( m_Position_Previous.Y + timestep_millis * ( m_Speed_V - timestep_millis * GRAVITY_2 ) );
	float dh_dt = (float)( m_Speed_V - timestep_millis * GRAVITY );
	float scale = 1 / sqrt( m_Speed_H * m_Speed_H + dh_dt * dh_dt );
	float scale2 = m_Speed_H * scale;
	
	float y = dh_dt * scale;
	CMatrix3D rotateInc;
	rotateInc.SetIdentity();
	rotateInc._22 = rotateInc._33 = y;
	rotateInc._23 = -( rotateInc._32 = scale2 );
	
	CMatrix3D rotateDir;
	rotateDir.SetIdentity();
	rotateDir._11 = rotateDir._33 = m_Axis.y;
	rotateDir._31 = -( rotateDir._13 = m_Axis.x );
	rotateInc.Concatenate( rotateDir );
	
	rotateInc._14 = m_Position_Graphics.X;
	rotateInc._24 = m_Position_Graphics.Y;
	rotateInc._34 = m_Position_Graphics.Z;

	m_Actor->SetTransform( rotateInc );
}

void CProjectile::ScriptingInit()
{
	CJSObject<CProjectile>::ScriptingInit( "Projectile", Construct, 4 );
}

JSBool CProjectile::Construct( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 4 );
	CStr ModelString;
	CVector3D Here, There;
	float Speed;
	CEntity* Temp, *Originator = NULL;
	CObjectEntry* oe = NULL;
	CModel* Model = NULL;
	CScriptObject Impact, Miss;
	const char* err = NULL;


	Temp = ToNative<CEntity>( argv[0] );
	if(Temp)
	{
		Model = Temp->m_actor->GetObject()->m_ProjectileModel;
		if( !Model )
		{
			err = "No projectile model is defined for that entity's actor.";
			goto fail;
		}
	}
	else if( !ToPrimitive<CStr>( cx, argv[0], ModelString ) || NULL == ( oe = g_ObjMan.FindObject( ModelString ) ) || NULL == ( Model = oe->m_Model ) )
	{
		err = "Invalid actor";
		goto fail;
	}

	Temp = ToNative<CEntity>( argv[1] );
	if(Temp)
	{
		// Use the position vector of this entity, add a bit (so the arrow doesn't appear out of the ground)
		// In future, find the appropriate position from the entity (location of a specific prop point?)
		Here = Temp->m_position;
		Here.Y = g_Game->GetWorld()->GetTerrain()->getExactGroundLevel( Here.X, Here.Z ) + 2.5f;
	}
	else if( !( ToPrimitive<CVector3D>( cx, argv[1], Here ) ) )
	{
		err = "Invalid vector";
		goto fail;
	}

	Temp = ToNative<CEntity>( argv[2] );
	if(Temp)
	{
		// Use the position vector of this entity.
		// TODO: Maybe: Correct for the movement of this entity.
		//       Then again, that doesn't belong here.
		There = Temp->m_position;
		There.Y = g_Game->GetWorld()->GetTerrain()->getExactGroundLevel( There.X, There.Z ) + 2.5f;
	}
	else if( !( ToPrimitive<CVector3D>( cx, argv[2], There ) ) )
	{
		err = "Invalid vector";
		goto fail;
	}

	Speed = ToPrimitive<float>( cx, argv[3] );
	if( Speed == 0.0f )
	{
		// Either wasn't specified, or was zero. In either case,
		// can't allow it: div/0 errors in the physics
		err = "Invalid speed";
		goto fail;
	}
	// Ignore errors in these last few and use the defaults if there's a problem.

	if( argc >= 5 )
		Originator = ToNative<CEntity>( argv[4] );
	if( argc >= 6 )
		Impact = argv[5]; // Script to run on impact with an entity.
	if( argc >= 7 ) 
		Miss = argv[6]; // Script to run on impact with the floor.

	{
		CProjectile* p = g_ProjectileManager.AddProjectile( Model, Here, There, Speed / 1000.0f, Originator, Impact, Miss );

		*rval = ToJSVal<CProjectile>( *p );
		return( JS_TRUE );
	}

fail:
	*rval = JSVAL_NULL;
	JS_ReportError( cx, err );
	return( JS_TRUE );
}

CEventProjectileImpact::CEventProjectileImpact( CEntity* Originator, CEntity* Impact, const CVector3D& Position ) : CScriptEvent( L"ProjectileImpact", EVENT_IMPACT )
{
	m_Originator = Originator;
	m_Impact = Impact;
	m_Position = Position;
	AddLocalProperty( L"originator", &m_Originator, true );
	AddLocalProperty( L"impacted", &m_Impact );
	AddLocalProperty( L"position", &m_Position );
}

CEventProjectileMiss::CEventProjectileMiss( CEntity* Originator, const CVector3D& Position ) : CScriptEvent( L"ProjectileMiss", EVENT_MISS )
{
	m_Originator = Originator;
	m_Position = Position;
	AddLocalProperty( L"originator", &m_Originator, true );
	AddLocalProperty( L"position", &m_Position );
}

CProjectileManager::CProjectileManager()
{
	m_LastTurnLength = 0;
	debug_printf( "CProjectileManager CREATED\n" );
}

CProjectileManager::~CProjectileManager()
{
	while( m_Projectiles.size() )
		delete( m_Projectiles[0] );
	debug_printf( "CProjectileManager DESTROYED\n" );
}
	
CProjectile* CProjectileManager::AddProjectile( const CModel* Actor, const CVector3D& Position, const CVector3D& Target, float Speed, CEntity* Originator, const CScriptObject& ImpactScript, const CScriptObject& MissScript )
{
	CProjectile* p = new CProjectile( Actor, Position, Target, Speed, Originator, ImpactScript, MissScript );
	m_Projectiles.push_back( p );
	return( p );
}

void CProjectileManager::DeleteProjectile( CProjectile* p )
{
	delete( p );
}

void CProjectileManager::UpdateAll( size_t timestep )
{
	uint i;
	for( i = 0; i < m_Projectiles.size(); i++ )
		if( !( m_Projectiles[i]->Update( timestep ) ) )
		{
			delete( m_Projectiles[i] );
			i--;
		}
}

void CProjectileManager::InterpolateAll( double relativeOffset )
{
	size_t absoluteOffset = (size_t)( (double)m_LastTurnLength * relativeOffset );
	std::vector<CProjectile*>::iterator it;
	for( it = m_Projectiles.begin(); it != m_Projectiles.end(); ++it )
		(*it)->Interpolate( absoluteOffset );
}
