#ifndef _TOOL_H
#define _TOOL_H

// flags for mouse events
#define TOOL_MOUSEFLAG_ALTDOWN		0x04
#define TOOL_MOUSEFLAG_CTRLDOWN		0x08
#define TOOL_MOUSEFLAG_SHIFTDOWN	0x10

class CTool
{
public:
	// virtual destructor
	virtual ~CTool() {}

	// draw the visual representation of this tool
	virtual void OnDraw() {}
	// callback for left button down event 
	virtual void OnLButtonDown(unsigned int flags,int px,int py) {}
	// callback for left button up event 
	virtual void OnLButtonUp(unsigned int flags,int px,int py) {}
	// callback for right button down event 
	virtual void OnRButtonDown(unsigned int flags,int px,int py) {}
	// callback for right button up event 
	virtual void OnRButtonUp(unsigned int flags,int px,int py) {}
	// callback for mouse move event 
	virtual void OnMouseMove(unsigned int flags,int px,int py) {}
};


#endif
