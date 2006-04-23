#include "precompiled.h"

#include "NaviCam.h"
#include "EditorData.h"
#include <stdarg.h>

CNaviCam g_NaviCam;

///////////////////////////////////////////////////////////////////////////////
// CNaviCam constructor
CNaviCam::CNaviCam() : m_CameraZoom(10)
{
}

///////////////////////////////////////////////////////////////////////////////
// OnMouseWheelScroll: handler for wheel scroll event - dir is positive for 
// upward scroll (away from user), or negative for downward scroll (towards
// user)
void CNaviCam::OnMouseWheelScroll(u32 flags,int px,int py,float dir)
{
	CVector3D forward=m_Camera.m_Orientation.GetIn();
	float factor=dir*dir;
	if (dir<0) factor=-factor;
	
//	// check we're not going to zoom into the terrain, or too far out into space
//	float h=m_Camera.m_Orientation.GetTranslation().Y+forward.Y*factor*m_CameraZoom;
//	float minh=65536*HEIGHT_SCALE*1.05f;
//	
//	if (h<minh || h>1500) {
//		// yup, we will; don't move anywhere (do clamped move instead, at some point)
//	} else {
//		// do a full move
//		m_CameraZoom-=(factor)*0.1f;
//		if (m_CameraZoom<0.1f) m_CameraZoom=0.01f;
//		m_Camera.m_Orientation.Translate(forward*(factor*m_CameraZoom));
//	}

	m_Camera.m_Orientation.Translate(forward*(factor*2.5f));

	g_EditorData.OnCameraChanged();
}

///////////////////////////////////////////////////////////////////////////////
// OnMButtonDown: handler for middle button down event 
void CNaviCam::OnMButtonDown(u32 flags,int px,int py)
{
}

///////////////////////////////////////////////////////////////////////////////
// OnMButtonUp: handler for middle button up event 
void CNaviCam::OnMButtonUp(u32 flags,int px,int py)
{
}

///////////////////////////////////////////////////////////////////////////////
// OnMouseMove: handler for mouse move (only called when middle button down)
void CNaviCam::OnMouseMove(u32 flags,int px,int py)
{
}
