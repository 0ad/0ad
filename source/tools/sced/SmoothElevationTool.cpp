#include "precompiled.h"

#include "timer.h"
#include "CommandManager.h"
#include "SmoothElevationTool.h"
#include "SmoothElevationCommand.h"

// default tool instance
CSmoothElevationTool CSmoothElevationTool::m_SmoothElevationTool;

// maximum smoothing power
const float CSmoothElevationTool::MAX_SMOOTH_POWER=32.0f;

CSmoothElevationTool::CSmoothElevationTool() : m_SmoothPower(16.0f)
{
}


void CSmoothElevationTool::SmoothSelection()
{
	double curtime=1000*get_time();
	double elapsed=(curtime-m_LastTriggerTime);
	
	if (elapsed > 1000.0) elapsed = 1000.0;
	while (elapsed>=m_SmoothPower) {
		CSmoothElevationCommand* smoothCmd=new CSmoothElevationCommand(MAX_SMOOTH_POWER,m_BrushSize,m_SelectionCentre);
		g_CmdMan.Execute(smoothCmd);
		elapsed-=m_SmoothPower;
	}
	m_LastTriggerTime=curtime-elapsed;
}


// callback for left button down event 
void CSmoothElevationTool::OnLButtonDown(unsigned int flags,int px,int py)
{
	// store trigger time
	m_LastTriggerTime=1000*get_time();
	// give base class a shout to do some work
	CBrushTool::OnLButtonDown(flags,px,py);
}

// callback for left button up event 
void CSmoothElevationTool::OnLButtonUp(unsigned int flags,int px,int py)
{
	// force a trigger
	OnTriggerLeft();
	// give base class a shout to do some work
	CBrushTool::OnLButtonUp(flags,px,py);
}


