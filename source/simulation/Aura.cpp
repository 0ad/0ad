/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Aura.h"

#include "EntityManager.h"
#include "Entity.h"

#include "scripting/ScriptableComplex.inl"

#include <algorithm>

CAura::CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, int tickRate, const CVector4D& color, JSObject* handler )
		: m_cx(cx), m_source(source), m_name(name), m_radius(radius), m_handler(handler),
		m_tickRate(tickRate), m_tickCyclePos(0), m_color(color)
{
	JS_AddRoot( m_cx, &m_handler );	// don't GC it so we can call it later
}

CAura::~CAura()
{
	JS_RemoveRoot( m_cx, &m_handler );
}

void CAura::Update( int timestep )
{
	std::vector<CEntity*> inRange;
	CVector3D pos = m_source->m_position;
	g_EntityManager.GetInRange( pos.X, pos.Z, m_radius, inRange );

	std::vector<CEntity*> prevInfluenced, curInfluenced, entered, exited;

	prevInfluenced.reserve(m_influenced.size());
	curInfluenced.reserve(m_influenced.size());

	for( std::vector<HEntity>::iterator it = m_influenced.begin(); it != m_influenced.end(); it++ )
	{
		CEntity* ent = *it;
		if( ent && ent->m_extant )
		{
			prevInfluenced.push_back(ent);
		}
	}

	m_influenced.clear();

	for( std::vector<CEntity*>::iterator it = inRange.begin(); it != inRange.end(); it++ )
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
	jsval enterFunction;
	if( JS_GetProperty( m_cx, m_handler, "onEnter", &enterFunction )
		&& enterFunction != JSVAL_VOID)
	{
		std::back_insert_iterator<std::vector<CEntity*> > ins( entered );
		set_difference( curInfluenced.begin(), curInfluenced.end(), 
			prevInfluenced.begin(), prevInfluenced.end(), 
			ins );
		for( std::vector<CEntity*>::iterator it = entered.begin(); it != entered.end(); it++ )
		{
			argv[0] = OBJECT_TO_JSVAL( (*it)->GetScript() );
			JS_CallFunctionValue( m_cx, m_handler, enterFunction, 1, argv, &rval );
			(*it)->m_aurasInfluencingMe.insert( this );
		}
	}
	
	// Call onExit on any unit that has exited the aura
	jsval exitFunction;
	if( JS_GetProperty( m_cx, m_handler, "onExit", &exitFunction )
		&& exitFunction != JSVAL_VOID )
	{
		std::back_insert_iterator<std::vector<CEntity*> > ins( exited );
		set_difference( prevInfluenced.begin(), prevInfluenced.end(), 
			curInfluenced.begin(), curInfluenced.end(), 
			ins );
		for( std::vector<CEntity*>::iterator it = exited.begin(); it != exited.end(); it++ )
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
		jsval tickFunction;
		if( JS_GetProperty( m_cx, m_handler, "onTick", &tickFunction )
			&& tickFunction != JSVAL_VOID )
		{
			for( std::vector<CEntity*>::iterator it = curInfluenced.begin(); it != curInfluenced.end(); it++ )
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
	jsval exitFunction;
	if( JS_GetProperty( m_cx, m_handler, "onExit", &exitFunction )
		&& exitFunction != JSVAL_VOID )
	{
		// Call the exit function on everything in our influence
		for( std::vector<HEntity>::iterator it = m_influenced.begin(); it != m_influenced.end(); it++ )
		{
			CEntity* ent = *it;
			if( ent && ent->m_extant )
			{
				argv[0] = OBJECT_TO_JSVAL( ent->GetScript() );
				JS_CallFunctionValue( m_cx, m_handler, exitFunction, 1, argv, &rval );
				(*it)->m_aurasInfluencingMe.erase( this );
			}
		}
	}
	m_influenced.clear();
}

// Remove an entity from the aura, but does not remove the aura from its
// m_aurasInfluencingMe. (Used when the entity is asking to be removed from
// the aura and will clear its own m_aurasInfluencingMe list afterwards).
void CAura::Remove( CEntity* ent )
{
	jsval rval;
	jsval argv[1];
	jsval exitFunction;
	if( JS_GetProperty( m_cx, m_handler, "onExit", &exitFunction )
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
