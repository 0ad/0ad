#include "precompiled.h"

#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "renderer/TimeManager.h"


CTimeManager::CTimeManager()
{
	m_frameDelta = 0.0;
	m_globalTime = 0.0;
}

double CTimeManager::GetFrameDelta()
{
	return m_frameDelta;
}

double CTimeManager::GetGlobalTime()
{
	return m_globalTime;
}

void CTimeManager::Update(double delta)
{
	m_frameDelta = delta;
	m_globalTime += delta;
}
