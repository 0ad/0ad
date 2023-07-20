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

#include "precompiled.h"
#include "FSM.h"


CFsmEvent::CFsmEvent(unsigned int type)
{
	m_Type = type;
	m_Param = nullptr;
}

CFsmEvent::~CFsmEvent()
{
	m_Param = nullptr;
}

void CFsmEvent::SetParamRef(void* pParam)
{
	m_Param = pParam;
}

CFsmTransition::CFsmTransition(const unsigned int state, const CallbackFunction action)
	: m_CurrState{state},
	m_Action{action}
{}

void CFsmTransition::SetEvent(CFsmEvent* pEvent)
{
	m_Event = pEvent;
}

void CFsmTransition::SetNextState(unsigned int nextState)
{
	m_NextState = nextState;
}

bool CFsmTransition::RunAction() const
{
	return !m_Action.pFunction ||
		reinterpret_cast<Action*>(m_Action.pFunction)(m_Action.pContext, m_Event);
}

CFsm::CFsm()
{
	m_Done = false;
	m_FirstState = FSM_INVALID_STATE;
	m_CurrState = FSM_INVALID_STATE;
	m_NextState = FSM_INVALID_STATE;
}

CFsm::~CFsm()
{
	Shutdown();
}

void CFsm::Setup()
{
	// Does nothing by default
}

void CFsm::Shutdown()
{
	// Release transitions
	TransitionList::iterator itTransition = m_Transitions.begin();
	for (; itTransition < m_Transitions.end(); ++itTransition)
		delete *itTransition;

	// Release events
	EventMap::iterator itEvent = m_Events.begin();
	for (; itEvent != m_Events.end(); ++itEvent)
		delete itEvent->second;

	m_States.clear();
	m_Events.clear();
	m_Transitions.clear();

	m_Done = false;
	m_FirstState = FSM_INVALID_STATE;
	m_CurrState = FSM_INVALID_STATE;
	m_NextState = FSM_INVALID_STATE;
}

void CFsm::AddState(unsigned int state)
{
	m_States.insert(state);
}

CFsmEvent* CFsm::AddEvent(unsigned int eventType)
{
	CFsmEvent* pEvent = nullptr;

	// Lookup event by type
	EventMap::iterator it = m_Events.find(eventType);
	if (it != m_Events.end())
	{
		pEvent = it->second;
	}
	else
	{
		pEvent = new CFsmEvent(eventType);

		// Store new event into internal map
		m_Events[eventType] = pEvent;
	}

	return pEvent;
}

CFsmTransition* CFsm::AddTransition(unsigned int state, unsigned int eventType, unsigned int nextState,
	void* pAction /* = nullptr */, void* pContext /* = nullptr*/)
{
	// Make sure we store the current state
	AddState(state);

	// Make sure we store the next state
	AddState(nextState);

	// Make sure we store the event
	CFsmEvent* pEvent = AddEvent(eventType);
	if (!pEvent)
		return nullptr;

	// Create new transition
	CFsmTransition* pNewTransition = new CFsmTransition(state, {pAction, pContext});

	// Setup new transition
	pNewTransition->SetEvent(pEvent);
	pNewTransition->SetNextState(nextState);

	// Store new transition
	m_Transitions.push_back(pNewTransition);

	return pNewTransition;
}

CFsmTransition* CFsm::GetTransition(unsigned int state, unsigned int eventType) const
{
	if (!IsValidState(state))
		return nullptr;

	if (!IsValidEvent(eventType))
		return nullptr;

	TransitionList::const_iterator it = m_Transitions.begin();
	for (; it != m_Transitions.end(); ++it)
	{
		CFsmTransition* pCurrTransition = *it;
		if (!pCurrTransition)
			continue;

		CFsmEvent* pCurrEvent = pCurrTransition->GetEvent();
		if (!pCurrEvent)
			continue;

		// Is it our transition?
		if (pCurrTransition->GetCurrState() == state &&  pCurrEvent->GetType() == eventType)
			return pCurrTransition;
	}

	// No transition found
	return nullptr;
}

void CFsm::SetFirstState(unsigned int firstState)
{
	m_FirstState = firstState;
}

void CFsm::SetCurrState(unsigned int state)
{
	m_CurrState = state;
}

bool CFsm::IsFirstTime() const
{
	return (m_CurrState == FSM_INVALID_STATE);
}

bool CFsm::Update(unsigned int eventType, void* pEventParam)
{
	if (!IsValidEvent(eventType))
		return false;

	if (IsFirstTime())
		m_CurrState = m_FirstState;

	// Lookup transition
	CFsmTransition* pTransition = GetTransition(m_CurrState, eventType);
	if (!pTransition)
		return false;

	// Setup event parameter
	EventMap::iterator it = m_Events.find(eventType);
	if (it != m_Events.end())
	{
		CFsmEvent* pEvent = it->second;
		if (pEvent)
			pEvent->SetParamRef(pEventParam);
	}

	// Save the default state transition (actions might call SetNextState
	// to override this)
	SetNextState(pTransition->GetNextState());

	if (!pTransition->RunAction())
		return false;

	SetCurrState(GetNextState());

	// Reset the next state since it's no longer valid
	SetNextState(FSM_INVALID_STATE);

	return true;
}

bool CFsm::IsDone() const
{
	// By default the internal flag m_Done is tested
	return m_Done;
}

bool CFsm::IsValidState(unsigned int state) const
{
	StateSet::const_iterator it = m_States.find(state);
	if (it == m_States.end())
		return false;

	return true;
}

bool CFsm::IsValidEvent(unsigned int eventType) const
{
	EventMap::const_iterator it = m_Events.find(eventType);
	if (it == m_Events.end())
		return false;

	return true;
}
