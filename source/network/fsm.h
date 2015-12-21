/* Copyright (C) 2015 Wildfire Games.
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

#ifndef FSM_H
#define	FSM_H

// INCLUDES
#include <vector>
#include <set>
#include <map>

// DECLARATIONS
#define	FSM_INVALID_STATE	( unsigned int )( ~0 )

class CFsmEvent;
class CFsmTransition;
class CFsm;

typedef bool ( *CONDITION )	( void* pContext );
typedef bool ( *ACTION )	( void* pContext, const CFsmEvent* pEvent );

typedef struct
{
	void*	pFunction;
	void*	pContext;

} CallbackFunction;

typedef std::set< unsigned int >				StateSet;
typedef std::map< unsigned int, CFsmEvent* >	EventMap;
typedef std::vector< CFsmTransition* >			TransitionList;
typedef std::vector< CallbackFunction >			CallbackList;

/**
 * Represents a signal in the state machine that a change has occurred.
 * The CFsmEvent objects are under the control of CFsm so
 * they are created and deleted via CFsm.
 */
class CFsmEvent
{
	NONCOPYABLE(CFsmEvent);
public:

	CFsmEvent( unsigned int type );
	~CFsmEvent( void );

	unsigned int	GetType		( void ) const { return m_Type; }
	void*			GetParamRef	( void ) { return m_Param; }
	void			SetParamRef	( void* pParam );

private:
	unsigned int	m_Type;				// Event type
	void*			m_Param;			// Event paramater
};


/**
 * An association of event, condition, action and next state.
 */
class CFsmTransition
{
	NONCOPYABLE(CFsmTransition);
public:

	CFsmTransition( unsigned int state );
	~CFsmTransition( void );

	void				 RegisterAction		(
											 void* pAction,
											 void* pContext );
	void				 RegisterCondition	(
											 void* pCondition,
											 void* pContext );
	void				 SetEvent			( CFsmEvent* pEvent );
	CFsmEvent*			 GetEvent			( void ) const	{ return m_Event; }
	void				 SetNextState		( unsigned int nextState );
	unsigned int		 GetNextState		( void ) const	{ return m_NextState; }
	unsigned int		 GetCurrState		( void ) const	{ return m_CurrState; }
	const CallbackList&	 GetActions			( void ) const	{ return m_Actions; }
	const CallbackList&	 GetConditions		( void ) const	{ return m_Conditions; }
	bool				 ApplyConditions	( void ) const;
	bool				 RunActions			( void ) const;

private:
	unsigned int	m_CurrState;		// Current state
	unsigned int	m_NextState;		// Next state
	CFsmEvent*		m_Event;			// Transition event
	CallbackList	m_Actions;			// List of actions for transition
	CallbackList	m_Conditions;		// List of conditions for transition
};

/**
 * Manages states, events, actions and transitions
 * between states. It provides an interface for advertising
 * events and track the current state. The implementation is
 * a Mealy state machine, so the system respond to events
 * and execute some action.
 *
 * A Mealy state machine has behaviour associated with state
 * transitions; Mealy machines are event driven where an
 * event triggers a state transition
 */
class CFsm
{
	NONCOPYABLE(CFsm);
public:

	CFsm( void );
	virtual ~CFsm( void );

	/**
	 * Constructs the state machine. This method must be overriden so that
	 * connections are constructed for the particular state machine implemented
	 */
	virtual void Setup( void );

	/**
	 * Clear event, action and condition lists and reset state machine
	 */
	void Shutdown( void );

	void			AddState			( unsigned int state );
	CFsmEvent*		AddEvent			( unsigned int eventType );
	CFsmTransition*	AddTransition		(
										 unsigned int state,
										 unsigned int eventType,
										 unsigned int nextState );
	CFsmTransition* AddTransition		(
										 unsigned int state,
										 unsigned int eventType,
										 unsigned int nextState,
										 void* pAction,
										 void* pContext );
	CFsmTransition*	GetTransition		(
										 unsigned int state,
										 unsigned int eventType ) const;
	CFsmTransition*	GetEventTransition	( unsigned int eventType ) const;
	void			SetFirstState		( unsigned int firstState );
	void			SetCurrState		( unsigned int state );
	unsigned int	GetCurrState		( void ) const		{ return m_CurrState; }
	void			SetNextState		( unsigned int nextState )	{ m_NextState = nextState; }
	unsigned int	GetNextState		( void ) const	{ return m_NextState; }
	const StateSet&	GetStates			( void ) const		{ return m_States; }
	const EventMap&	GetEvents			( void ) const		{ return m_Events; }
	const TransitionList&	GetTransitions		( void ) const		{ return m_Transitions; }
	bool			Update				( unsigned int eventType, void* pEventData );
	bool			IsValidState		( unsigned int state ) const;
	bool			IsValidEvent		( unsigned int eventType ) const;
	virtual bool	IsDone				( void ) const;

private:
	bool			IsFirstTime			( void ) const;

	bool			m_Done;				// FSM work is done
	unsigned int	m_FirstState;		// Initial state
	unsigned int	m_CurrState;		// Current state
	unsigned int	m_NextState;		// Next state
	StateSet		m_States;			// List of states
	EventMap		m_Events;			// List of events
	TransitionList	m_Transitions;		// State transitions
};

#endif	// FSM_H

