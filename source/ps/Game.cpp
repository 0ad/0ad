#include "precompiled.h"

#include "Game.h"

void CGame::Initialize(CGameAttributes *pAttribs)
{
	m_World.Initialize(pAttribs);
	m_Simulation.Initialize(pAttribs);
	m_GameView.Initialize(pAttribs);
}

void CGame::Render()
{
	m_GameView.Render();
}

void CGame::Update(double deltaTime)
{
	m_Simulation.Update(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
}
