/////////////////////////////////////////////////////////////////////////////////////
// navicam.h
//
//	- header containing NaviCam class, used to produce MAX style navigation controls
/////////////////////////////////////////////////////////////////////////////////////


#ifndef _NAVICAM_H
#define _NAVICAM_H

#include "res/res.h"
#include "Camera.h"

/////////////////////////////////////////////////////////////////////////////////////
// CNaviCam: MAX style navigation controller
class CNaviCam
{
public:
	// constructor
	CNaviCam();

	// handler for wheel scroll event - dir is positive for 
	// upward scroll (away from user), or negative for downward scroll (towards
	// user)
	void OnMouseWheelScroll(u32 flags,int px,int py,float dir);
	// handler for middle button down event 
	void OnMButtonDown(u32 flags,int px,int py);
	// handler for middle button up event 
	void OnMButtonUp(u32 flags,int px,int py);
	// handler for mouse move (only called when middle button down)
	void OnMouseMove(u32 flags,int px,int py);

	// return the regular camera object
	CCamera& GetCamera() { return m_Camera; }

private:
	// the regular camera object that defines FOV, orientation, etc
	CCamera m_Camera;
	// current zoom of camera
	float m_CameraZoom;
};

extern CNaviCam g_NaviCam;


#endif
