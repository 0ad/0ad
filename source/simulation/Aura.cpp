#include "precompiled.h"
#include "Aura.h"
#include "EntityManager.h"
#include <algorithm>

using namespace std;

CAura::CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, size_t tickRate, JSObject* handler )
		: m_cx(cx), m_source(source), m_name(name), m_radius(radius), m_handler(handler),
		m_tickRate(tickRate), m_tickCyclePos(0)
{
	JS_AddRoot( m_cx, &m_handler );	// don't GC it so we can call it later
}

CAura::~CAura()
{
	JS_RemoveRoot( m_cx, &m_handler );
}

void CAura::Update( size_t timestep )
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

	// Call onEnter on any new unit that has entered the aura
	CStrW enterName = L"onEnter";
	jsval enterFunction;
	utf16string enterName16 = enterName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, enterName16.c_str(), enterName16.length(), &enterFunction )
		&& enterFunction != JSVAL_VOID)
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
	
	// Call onExit on any unit that has exited the aura
	CStrW exitName = L"onExit";
	jsval exitFunction;
	utf16string exitName16 = exitName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction )
		&& exitFunction != JSVAL_VOID )
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

	m_tickCyclePos += timestep;

	if( m_tickRate > 0 && m_tickCyclePos > m_tickRate )
	{
		// It's time to tick; call OnTick on any unit that is in the aura
		CStrW tickName = L"onTick";
		jsval tickFunction;
		utf16string tickName16 = tickName.utf16();
		if( JS_GetUCProperty( m_cx, m_handler, tickName16.c_str(), tickName16.length(), &tickFunction )
			&& tickFunction != JSVAL_VOID )
		{
			for( vector<CEntity*>::iterator it = curInfluenced.begin(); it != curInfluenced.end(); it++ )
			{
				argv[0] = OBJECT_TO_JSVAL( (*it)->GetScript() );
				JS_CallFunctionValue( m_cx, m_handler, tickFunction, 1, argv, &rval );
			}
		}

		// Reset cycle pos
		m_tickCyclePos %= m_tickRate;
	}
}

void CAura::RemoveAll()
{
	jsval rval;
	jsval argv[1];
	CStrW exitName = L"onExit";
	jsval exitFunction;
	utf16string exitName16 = exitName.utf16();
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction )
		&& exitFunction != JSVAL_VOID )
	{
		// Call the exit function on everything in our influence
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
	if( JS_GetUCProperty( m_cx, m_handler, exitName16.c_str(), exitName16.length(), &exitFunction )
		&& exitFunction != JSVAL_VOID )
	{
		// Call the exit function on it
		argv[0] = OBJECT_TO_JSVAL( ent->GetScript() );
		JS_CallFunctionValue( m_cx, m_handler, exitFunction, 1, argv, &rval );

		// Remove it from the m_influenced array
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
