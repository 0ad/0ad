#include "CommandManager.h"
#include "RaiseElevationTool.h"
#include "RaiseElevationCommand.h"

#include "timer.h"

// default tool instance
CRaiseElevationTool CRaiseElevationTool::m_RaiseElevationTool;


CRaiseElevationTool::CRaiseElevationTool() : m_Speed(MAX_SPEED)
{
}

// callback for left button down event 
void CRaiseElevationTool::OnLButtonDown(u32 flags,int px,int py)
{
	// store trigger time
	m_LastTriggerTime=get_time();
	// give base class a shout to do some work
	CBrushTool::OnLButtonDown(flags,px,py);
}

// callback for left button up event 
void CRaiseElevationTool::OnLButtonUp(u32 flags,int px,int py)
{
	// force a trigger
	OnTriggerLeft();
	// give base class a shout to do some work
	CBrushTool::OnLButtonUp(flags,px,py);
}

// callback for right button down event 
void CRaiseElevationTool::OnRButtonDown(u32 flags,int px,int py)
{
	// store trigger time
	m_LastTriggerTime=get_time();
	// give base class a shout to do some work
	CBrushTool::OnRButtonDown(flags,px,py);
}

// callback for right button up event 
void CRaiseElevationTool::OnRButtonUp(u32 flags,int px,int py)
{
	// force a trigger
	OnTriggerRight();
	// give base class a shout to do some work
	CBrushTool::OnRButtonUp(flags,px,py);
}

void CRaiseElevationTool::AlterSelectionHeight(int32 amount)
{
	CRaiseElevationCommand* alterCmd=new CRaiseElevationCommand(amount,m_BrushSize,m_SelectionCentre);
	g_CmdMan.Execute(alterCmd);
}

int32 CRaiseElevationTool::CalcDistSinceLastTrigger()
{
	double curtime=get_time();
	double elapsed=curtime-m_LastTriggerTime;
	int32 dist=int32(elapsed*m_Speed);
	
	m_LastTriggerTime+=dist/m_Speed;

	return dist;
}

// tool triggered by left mouse button; raise selected terrain
void CRaiseElevationTool::OnTriggerLeft() 
{ 
	AlterSelectionHeight(CalcDistSinceLastTrigger()); 
}

// tool triggered by right mouse button; lower selected terrain
void CRaiseElevationTool::OnTriggerRight() 
{ 
	AlterSelectionHeight(-CalcDistSinceLastTrigger()); 
}

