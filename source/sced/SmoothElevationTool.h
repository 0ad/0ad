#ifndef _SMOOTHELEVATIONTOOL_H
#define _SMOOTHELEVATIONTOOL_H

#include <set>
#include "res/res.h"
#include "BrushTool.h"

class CSmoothElevationTool : public CBrushTool
{
public:
	enum { MAX_BRUSH_SIZE=8 };
	static const float MAX_SMOOTH_POWER;

public:
	CSmoothElevationTool();

	// tool triggered by left mouse button; smooth selected terrain
	void OnTriggerLeft() { SmoothSelection(); }

	// callback for left button down event 
	void OnLButtonDown(unsigned int flags,int px,int py);
	// callback for left button up event 
	void OnLButtonUp(unsigned int flags,int px,int py);

	// set smoothing power
	void SetSmoothPower(float power) { m_SmoothPower=power; }
	// get smoothing power
	float GetSmoothPower() const { return m_SmoothPower; }

	// allow multiple triggers by click and drag
	bool SupportDragTrigger() { return true; }

	// get tool instance
	static CSmoothElevationTool* GetTool() { return &m_SmoothElevationTool; }

private:	
	// smooth the currently selected terrain tiles
	void SmoothSelection();

	// time of last trigger
	double m_LastTriggerTime;

	// amount to smooth selected terrain tiles
	float m_SmoothPower;

	// default tool instance
	static CSmoothElevationTool m_SmoothElevationTool;
};

#endif
