/* Copyright (C) 2023 Wildfire Games.
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
#define FSM_H

#include <limits>
#include <map>
#include <set>
#include <vector>


constexpr unsigned int FSM_INVALID_STATE{std::numeric_limits<unsigned int>::max()};

class CFsmEvent;
class CFsmTransition;
class CFsm;

using Condition = bool(void* pContext);
using Action = bool(void* pContext, const CFsmEvent* pEvent);

struct CallbackFunction
{
	void* pFunction;
	void* pContext;
};

using StateSet = std::set<unsigned int>;
using EventMap = std::map<unsigned int, CFsmEvent*>;
using TransitionList = std::vector<CFsmTransition*>;
using CallbackList = std::vector<CallbackFunction>;

/**
 * Represents a signal in the state machine that a change has occurred.
 * The CFsmEvent objects are under the control of CFsm so
 * they are created and deleted via CFsm.
 */
class CFsmEvent
{
	NONCOPYABLE(CFsmEvent);
public:

	CFsmEvent(unsigned int type);
	~CFsmEvent();

	unsigned int GetType() const
	{
		return m_Type;
	}

	void* GetParamRef()
	{
		return m_Param;
	}

	void SetParamRef(void* pParam);

private:
	unsigned int m_Type; // Event type
	void* m_Param; // Event paramater
};


/**
 * An association of event, condition, action and next state.
 */
class CFsmTransition
{
	NONCOPYABLE(CFsmTransition);
public:

	CFsmTransition(unsigned int state);
	~CFsmTransition();

	/**
	 * Registers an action that will be executed when the transition occurs.
	 * @param pAction the function which will be executed.
	 * @param pContext data passed to the function.
	 */
	void RegisterAction(void* pAction, void* pContext);

	/**
	 * Registers a condition which will be evaluated when the transition occurs.
	 * @param pCondition the predicate which will be executed.
	 * @param pContext data passed to the predicate.
	 */
	void RegisterCondition(void* pCondition, void* pContext);

	/**
	 * Set event for which transition will occur.
	 */
	void SetEvent(CFsmEvent* pEvent);
	CFsmEvent* GetEvent() const
	{
		return m_Event;
	}

	/**
	 * Set next state the transition will switch the system to.
	 */
	void SetNextState(unsigned int nextState);
	unsigned int GetNextState() const
	{
		return m_NextState;
	}

	unsigned int GetCurrState() const
	{
		return m_CurrState;
	}

	const CallbackList& GetActions() const
	{
		return m_Actions;
	}

	const CallbackList& GetConditions() const
	{
		return m_Conditions;
	}

	/**
	 * Evaluates conditions for the transition.
	 * @return whether all the conditions are true.
	 */
	bool ApplyConditions() const;

	/**
	 * Executes actions for the transition.
	 * @note If there are no actions, assume true.
	 * @return whether all the actions returned true.
	 */
	bool RunActions() const;

private:
	unsigned int m_CurrState;
	unsigned int m_NextState;
	CFsmEvent* m_Event;
	CallbackList m_Actions;
	CallbackList m_Conditions;
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
 * event triggers a state transition.
 */
class CFsm
{
	NONCOPYABLE(CFsm);
public:

	CFsm();
	virtual ~CFsm();

	/**
	 * Constructs the state machine. This method must be overriden so that
	 * connections are constructed for the particular state machine implemented.
	 */
	virtual void Setup();

	/**
	 * Clear event, action and condition lists and reset state machine.
	 */
	void Shutdown();

	/**
	 * Adds the specified state to the internal list of states.
	 * @note If a state with the specified ID exists, the state is not added.
	 */
	void AddState(unsigned int state);

	/**
	 * Adds the specified event to the internal list of events.
	 * @note If an eveny with the specified ID exists, the event is not added.
	 * @return a pointer to the new event.
	 */
	CFsmEvent* AddEvent(unsigned int eventType);

	/**
	 * Adds a new transistion to the state machine.
	 * @return a pointer to the new transition.
	 */
	CFsmTransition* AddTransition(unsigned int state, unsigned int eventType, unsigned int nextState );

	/**
	 * Adds a new transition to the state machine.
	 * @return a pointer to the new transition.
	 */
	CFsmTransition* AddTransition(unsigned int state, unsigned int eventType, unsigned int nextState,
		 void* pAction, void* pContext);

	/**
	 * Looks up the transition given the state, event and next state to transition to.
	 */
	CFsmTransition* GetTransition(unsigned int state, unsigned int eventType) const;
	CFsmTransition* GetEventTransition (unsigned int eventType) const;

	/**
	 * Sets the initial state for FSM.
	 */
	void SetFirstState(unsigned int firstState);

	/**
	 * Sets the current state and update the last state to the current state.
	 */
	void SetCurrState(unsigned int state);
	unsigned int GetCurrState() const
	{
		return m_CurrState;
	}

	void SetNextState(unsigned int nextState)
	{
		m_NextState = nextState;
	}

	unsigned int GetNextState() const
	{
		return m_NextState;
	}

	const StateSet& GetStates() const
	{
		return m_States;
	}

	const EventMap& GetEvents() const
	{
		return m_Events;
	}

	const TransitionList& GetTransitions() const
	{
		return m_Transitions;
	}

	/**
	 * Updates the FSM and retrieves next state.
	 * @return whether the state was changed.
	 */
	bool Update(unsigned int eventType, void* pEventData);

	/**
	 * Verifies whether the specified state is managed by the FSM.
	 */
	bool IsValidState(unsigned int state) const;

	/**
	 * Verifies whether the specified event is managed by the FSM.
	 */
	bool IsValidEvent(unsigned int eventType) const;

	/**
	 * Tests whether the state machine has finished its work.
	 * @note This is state machine specific.
	 */
	virtual bool IsDone() const;

private:
	/**
	 * Verifies whether state machine has already been updated.
	 */
	bool IsFirstTime() const;

	bool m_Done;
	unsigned int m_FirstState;
	unsigned int m_CurrState;
	unsigned int m_NextState;
	StateSet m_States;
	EventMap m_Events;
	TransitionList m_Transitions;
};

#endif // FSM_H
