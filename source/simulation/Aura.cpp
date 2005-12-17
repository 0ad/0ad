#include "precompiled.h"
#include "Aura.h"
#include "EntityManager.h"
#include <algorithm>

using namespace std;

CAura::CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, JSObject* handler )
		: m_cx(cx), m_source(source), m_name(name), m_radius(radius), m_handler(handler)
{
	JS_AddRoot( m_cx, m_handler );	// don't GC it so we can call it later
}

CAura::~CAura()
{
	JS_RemoveRoot( m_cx, m_handler );
}

void CAura::Update( size_t UNUSED(timestep) )
{
	vector<CEntity*> inRange;
	CVector3D pos = m_source->m_position;
	g_EntityManager.GetInRange( pos.X, pos.Z, m_radius, inRange );

	vector<CEntity*> prevInfluenced, curInfluenced, entered, exited;

	for( vector<HEntity>::iterator it = m_influenced.begin(); it != m_influenced.end(); it++ )
	{
		CEntity* ent = *it;
		if( ent->m_extant )
		{
			prevInfluenced.push_back(ent);
		}
	}

	m_influenced.clear();

	for( vector<CEntity*>::iterator it = inRange.begin(); it != inRange.end(); it++ )
	{
		CEntity* ent = *it;
		if(ent != m_source)
		{
			curInfluenced.push_back(ent);
			m_influenced.push_back( HEntity(ent->me) );
		}
	}

	sort( prevInfluenced.begin(), prevInfluenced.end() );
	sort( curInfluenced.begin(), curInfluenced.end() );

	jsval rval;
	jsval argv[1];

	CStrW enterName = L"onEnter";
	jsval enterFunction;
	utf16string enterName16 = enterName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, enterName16.c_str(), enterName16.length(), &enterFunction ) )
	{
		back_insert_iterator<vector<CEntity*> > ins( entered );
		set_difference( curInfluenced.begin(), curInfluenced.end(), 
			prevInfluenced.begin(), prevInfluenced.end(), 
			ins );
		for( vector<CEntity*>::iterator it = entered.begin(); it != entered.end(); it++ )
		{
			argv[0] = OBJECT_TO_JSVAL( (*it)->GetScript() );
			JS_CallFunctionValue( m_cx, m_handler, enterFunction, 1, argv, &rval );
			(*it)->m_aurasInfluencingMe.insert( this );
		}
	}
	
	CStrW exitName = L"onExit";
	jsval exitFunction;
	utf16string exitName16 = exitName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction ) )
	{
		back_insert_iterator<vector<CEntity*> > ins( exited );
		set_difference( prevInfluenced.begin(), prevInfluenced.end(), 
			curInfluenced.begin(), curInfluenced.end(), 
			ins );
		for( vector<CEntity*>::iterator it = exited.begin(); it != exited.end(); it++ )
		{
			argv[0] = OBJECT_TO_JSVAL( (*it)->GetScript() );
			JS_CallFunctionValue( m_cx, m_handler, exitFunction, 1, argv, &rval );
			(*it)->m_aurasInfluencingMe.erase( this );
		}
	}
}

void CAura::RemoveAll()
{
	jsval rval;
	jsval argv[1];
	CStrW exitName = L"onExit";
	jsval exitFunction;
	utf16string exitName16 = exitName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction ) )
	{
		for( vector<HEntity>::iterator it = m_influenced.begin(); it != m_influenced.end(); it++ )
		{
			CEntity* ent = *it;
			if( ent->m_extant )
			{
				argv[0] = OBJECT_TO_JSVAL( ent->GetScript() );
				JS_CallFunctionValue( m_cx, m_handler, exitFunction, 1, argv, &rval );
				(*it)->m_aurasInfluencingMe.erase( this );
			}
		}
	}
	m_influenced.clear();
}

void CAura::Remove( CEntity* ent )
{
	jsval rval;
	jsval argv[1];
	CStrW exitName = L"onExit";
	jsval exitFunction;
	utf16string exitName16 = exitName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction ) )
	{
		argv[0] = OBJECT_TO_JSVAL( ent->GetScript() );
		JS_CallFunctionValue( m_cx, m_handler, exitFunction, 1, argv, &rval );
		for( size_t i=0; i < m_influenced.size(); i++ )
		{
			if( ((CEntity*) m_influenced[i]) == ent )
			{
				m_influenced.erase( m_influenced.begin() + i );
				break;
			}
		}
	}
}
