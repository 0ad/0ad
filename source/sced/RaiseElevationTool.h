#ifndef _RAISEELEVATIONTOOL_H
#define _RAISEELEVATIONTOOL_H

#include <set>
#include "res/res.h"
#include "BrushTool.h"

class CRaiseElevationTool : public CBrushTool
{
public:
	enum { MAX_BRUSH_SIZE=8 };
	enum { MAX_SPEED=255 };

public:
	CRaiseElevationTool();

	// tool triggered by left mouse button; raise selected terrain
	void OnTriggerLeft(); 
	// tool triggered by right mouse button; lower selected terrain
	void OnTriggerRight();

	// callback for left button down event 
	void OnLButtonDown(u32 flags,int px,int py);
	// callback for left button up event 
	void OnLButtonUp(u32 flags,int px,int py);
	// callback for right button down event 
	void OnRButtonDown(u32 flags,int px,int py);
	// callback for right button up event 
	void OnRButtonUp(u32 flags,int px,int py);

	// set change in elevation on tool being triggered
	void SetSpeed(int delta) { m_Speed=delta; }
	// get change in elevation on tool being triggered
	int GetSpeed() const { return m_Speed; }

	// allow multiple triggers by click and drag
	bool SupportDragTrigger() { return true; }

	// get tool instance
	static CRaiseElevationTool* GetTool() { return &m_RaiseElevationTool; }

private:	
	// raise/lower the currently selected terrain tiles by given amount
	void AlterSelectionHeight(int32 amount);

	// calculate distance terrain has moved since last trigger; adjust last trigger
	// time appropriately to avoid rounding errors
	int32 CalcDistSinceLastTrigger();

	// number of units to raise/lower selected terrain tiles per second
	int m_Speed;
	
	// time of last trigger
	double m_LastTriggerTime;

	// default tool instance
	static CRaiseElevationTool m_RaiseElevationTool;
};

#endif
