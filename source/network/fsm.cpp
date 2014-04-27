/* Copyright (C) 2013 Wildfire Games.
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

// INCLUDES
#include "precompiled.h"
#include "fsm.h"

// DECLARATIONS

//-----------------------------------------------------------------------------
// Name: CFsmEvent()
// Desc: Constructor
//-----------------------------------------------------------------------------
CFsmEvent::CFsmEvent( unsigned int type )
{
	m_Type	= type;
	m_Param	= NULL;
}

//-----------------------------------------------------------------------------
// Name; ~CFsmEvent()
// Desc: Destructor
//-----------------------------------------------------------------------------
CFsmEvent::~CFsmEvent( void )
{
	m_Param = NULL;
}

//-----------------------------------------------------------------------------
// Name: SetParamRef()
// Desc: Sets the parameter for the event
//-----------------------------------------------------------------------------
void CFsmEvent::SetParamRef( void* pParam )
{
	m_Param = pParam;
}

//-----------------------------------------------------------------------------
// Name: CFsmTransition()
// Desc: Constructor
//-----------------------------------------------------------------------------
CFsmTransition::CFsmTransition( unsigned int state )
{
	m_CurrState	= state;
}

//-----------------------------------------------------------------------------
// Name: ~CFsmTransition()
// Desc: Destructor
//-----------------------------------------------------------------------------
CFsmTransition::~CFsmTransition( void )
{
	m_Actions.clear();
	m_Conditions.clear();
}

//-----------------------------------------------------------------------------
// Name: RegisterAction()
// Desc: Adds action that will be executed when the transition will occur
//-----------------------------------------------------------------------------
void CFsmTransition::RegisterAction( void* pAction, void* pContext )
{
	CallbackFunction callback;

	// Add action at the end of actions list
	callback.pFunction	= pAction;
	callback.pContext	= pContext;

	m_Actions.push_back( callback );
}

//-----------------------------------------------------------------------------
// Name: AddCondition()
// Desc: Adds condition which will be evaluated when the transition will occurs
//-----------------------------------------------------------------------------
void CFsmTransition::RegisterCondition( void* pCondition, void* pContext )
{
	CallbackFunction callback;

	// Add condition at the end of conditions list
	callback.pFunction	= pCondition;
	callback.pContext	= pContext;

	m_Conditions.push_back( callback );
}

//-----------------------------------------------------------------------------
// Name: SetEvent()
// Desc: Set event for which transition will occur
//-----------------------------------------------------------------------------
void CFsmTransition::SetEvent( CFsmEvent* pEvent )
{
	m_Event = pEvent;
}

//-----------------------------------------------------------------------------
// Name: SetNextState()
// Desc: Set next state the transition will switch the system to
//-----------------------------------------------------------------------------
void CFsmTransition::SetNextState( unsigned int nextState )
{
	m_NextState = nextState;
}

//-----------------------------------------------------------------------------
// Name: ApplyConditions()
// Desc: Evaluate conditions for the transition
// Note: If there are no conditions, assume true
//-----------------------------------------------------------------------------
bool CFsmTransition::ApplyConditions( void ) const
{
    bool eval = true;

	CallbackList::const_iterator it = m_Conditions.begin();
    for( ; it != m_Conditions.end(); ++it )
	{
		if ( it->pFunction )
		{
			// Evaluate condition
			CONDITION Condition = ( CONDITION )it->pFunction;
			eval &= Condition( it->pContext );
		}
    }

    return eval;
}

//-----------------------------------------------------------------------------
// Name: RunActions()
// Desc: Execute actions for the transition
// Note: If there are no actions, assume true
//-----------------------------------------------------------------------------
bool CFsmTransition::RunActions( void ) const
{
	bool result = true;

	CallbackList::const_iterator it = m_Actions.begin();
	for( ; it != m_Actions.end(); ++it )
	{
		if ( it->pFunction )
		{
			// Run action
			ACTION Action = ( ACTION )it->pFunction;
			result &= Action( it->pContext, m_Event );
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// Name: CFsm()
// Desc: Constructor
//-----------------------------------------------------------------------------
CFsm::CFsm( void )
{
	m_Done		 = false;
	m_FirstState = FSM_INVALID_STATE;
	m_CurrState	 = FSM_INVALID_STATE;
	m_NextState  = FSM_INVALID_STATE;
}

//-----------------------------------------------------------------------------
// Name: ~CFsm()
// Desc: Destructor
//-----------------------------------------------------------------------------
CFsm::~CFsm( void )
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Name: Setup()
// Desc: Setup events, actions and state transitions
//-----------------------------------------------------------------------------
void CFsm::Setup( void )
{
	// Does nothing by default
}

//-----------------------------------------------------------------------------
// Name: Reset()
// Desc: Shuts down the state machine and releases any resources
//-----------------------------------------------------------------------------
void CFsm::Shutdown( void )
{
	// Release transitions
	TransitionList::iterator itTransition = m_Transitions.begin();
	for ( ; itTransition < m_Transitions.end(); ++itTransition )
	{
		CFsmTransition* pCurrTransition = *itTransition;
		if ( !pCurrTransition ) continue;

		delete pCurrTransition;
	}

	// Release events
	EventMap::iterator itEvent = m_Events.begin();
	for( ; itEvent != m_Events.end(); ++itEvent )
	{
		CFsmEvent* pCurrEvent = itEvent->second;
		if ( !pCurrEvent ) continue;

		delete pCurrEvent;
	}

	m_States.clear();
	m_Events.clear();
	m_Transitions.clear();

	m_Done		 = false;
	m_FirstState = FSM_INVALID_STATE;
	m_CurrState	 = FSM_INVALID_STATE;
	m_NextState  = FSM_INVALID_STATE;
}

//-----------------------------------------------------------------------------
// Name: AddState()
// Desc: Adds the specified state to the internal list of states
// Note: If a state with the specified ID exists, the state is not added
//-----------------------------------------------------------------------------
void CFsm::AddState( unsigned int state )
{
	m_States.insert( state );
}

//-----------------------------------------------------------------------------
// Name: AddEvent()
// Desc: Adds the specified event to the internal list of events
// Note: If an eveny with the specified ID exists, the event is not added
//-----------------------------------------------------------------------------
CFsmEvent* CFsm::AddEvent( unsigned int eventType )
{
	CFsmEvent* pEvent = NULL;

	// Lookup event by type
	EventMap::iterator it = m_Events.find( eventType );
	if ( it != m_Events.end() )
	{
		pEvent = it->second;
	}
	else
	{
		pEvent = new CFsmEvent( eventType );
		if ( !pEvent ) return NULL;

		// Store new event into internal map
		m_Events[ eventType ] = pEvent;
	}

	return pEvent;
}

//-----------------------------------------------------------------------------
// Name: AddTransition()
// Desc: Adds a new transistion to the state machine
//-----------------------------------------------------------------------------
CFsmTransition* CFsm::AddTransition( 
									unsigned int state,
									unsigned int eventType,
									unsigned int nextState )
{
	// Make sure we store the current state
	AddState( state );

	// Make sure we store the next state
	AddState( nextState );

	// Make sure we store the event
	CFsmEvent* pEvent = AddEvent( eventType );
	if ( !pEvent ) return NULL;

	// Create new transition
	CFsmTransition* pNewTransition = new CFsmTransition( state );
	if ( !pNewTransition )
	{
		delete pEvent;
		return NULL;
	}

	// Setup new transition
	pNewTransition->SetEvent( pEvent );
	pNewTransition->SetNextState( nextState );

	// Store new transition
	m_Transitions.push_back( pNewTransition );

	return pNewTransition;
}

//-----------------------------------------------------------------------------
// Name: AddTransition()
// Desc: Adds a new transition to the state machine
//-----------------------------------------------------------------------------
CFsmTransition* CFsm::AddTransition( 
									unsigned int state,
									unsigned int eventType,
									unsigned int nextState,
									void* pAction,
									void* pContext )
{
	CFsmTransition* pTransition = AddTransition( state, eventType, nextState );
	if ( !pTransition ) return NULL;
	
	// If action specified, register it
	if ( pAction )
		pTransition->RegisterAction( pAction, pContext );

	return pTransition;
}

//-----------------------------------------------------------------------------
// Name: GetTransition()
// Desc: Lookup transition given the state, event and next state to transition
//-----------------------------------------------------------------------------
CFsmTransition* CFsm::GetTransition(
									unsigned int state,
									unsigned int eventType ) const
{
	// Valid state?
	if ( !IsValidState( state ) ) return NULL;

	// Valid event?
	if ( !IsValidEvent( eventType ) ) return NULL;

	// Loop through the list of transitions
	TransitionList::const_iterator it = m_Transitions.begin();
	for ( ; it != m_Transitions.end(); ++it )
	{
		CFsmTransition* pCurrTransition = *it;
		if ( !pCurrTransition ) continue;

		CFsmEvent* pCurrEvent = pCurrTransition->GetEvent();
		if ( !pCurrEvent ) continue;

		// Is it our transition?
		if ( pCurrTransition->GetCurrState() == state &&
			 pCurrEvent->GetType() == eventType )
		{
			return pCurrTransition;
		}
	}

	// No transition found
	return NULL;
}

//-----------------------------------------------------------------------------
// Name: SetFirstState()
// Desc: Set initial state for FSM
//-----------------------------------------------------------------------------
void CFsm::SetFirstState( unsigned int firstState )
{
	m_FirstState = firstState;
}

//-----------------------------------------------------------------------------
// Name: SetCurrState()
// Desc: Set current state and update last state to current state
//-----------------------------------------------------------------------------
void CFsm::SetCurrState( unsigned int state )
{
	m_CurrState = state;
}

//-----------------------------------------------------------------------------
// Name: IsFirstTime()
// Desc: Verifies if the state machine has been already updated
//-----------------------------------------------------------------------------
bool CFsm::IsFirstTime( void ) const
{
	return ( m_CurrState == FSM_INVALID_STATE );
}

//-----------------------------------------------------------------------------
// Name: Update()
// Desc: Updates state machine and retrieves next state
//-----------------------------------------------------------------------------
bool CFsm::Update( unsigned int eventType, void* pEventParam )
{
	// Valid event?
	if ( !IsValidEvent( eventType ) )
		return false;

	// First time update?
	if ( IsFirstTime() )
		m_CurrState = m_FirstState;

	// Lookup transition
	CFsmTransition* pTransition = GetTransition( m_CurrState, eventType );
	if ( !pTransition ) 
		return false;

	// Setup event parameter
	EventMap::iterator it = m_Events.find( eventType );
	if ( it != m_Events.end() )
	{
		CFsmEvent* pEvent = it->second;
		if ( pEvent ) pEvent->SetParamRef( pEventParam );
	}

	// Valid transition?
	if ( !pTransition->ApplyConditions() ) 
		return false;

	// Save the default state transition (actions might call SetNextState
	// to override this)
	SetNextState( pTransition->GetNextState() );

	// Run transition actions
	if ( !pTransition->RunActions() ) 
		return false;

	// Switch state
	SetCurrState( GetNextState() );

	// Reset the next state since it's no longer valid
	SetNextState( FSM_INVALID_STATE );
	
	return true;
}

//-----------------------------------------------------------------------------
// Name: IsDone()
// Desc: Tests whether the state machine has finished its work
// Note: This is state machine specific
//-----------------------------------------------------------------------------
bool CFsm::IsDone( void ) const
{
	// By default the internal flag m_Done is tested
	return m_Done;
}

//-----------------------------------------------------------------------------
// Name: IsValidState()
// Desc: Verifies whether the specified state is managed by FSM
//-----------------------------------------------------------------------------
bool CFsm::IsValidState( unsigned int state ) const
{
	StateSet::const_iterator it = m_States.find( state );
	if ( it == m_States.end() ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: IsValidEvent()
// Desc: Verifies whether the specified event is managed by FSM
//-----------------------------------------------------------------------------
bool CFsm::IsValidEvent( unsigned int eventType ) const
{
	EventMap::const_iterator it = m_Events.find( eventType );
	if ( it == m_Events.end() ) return false;

	return true;
}
