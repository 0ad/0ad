#ifndef _BRUSHTOOL_H
#define _BRUSHTOOL_H

#include "res/res.h"
#include "Tool.h"
#include "Vector3D.h"

class CPatch;
class CMiniPatch;

class CBrushTool : public CTool
{
public:
	CBrushTool();

	// draw the visual representation of this tool
	virtual void OnDraw();
	// callback for left button down event 
	virtual void OnLButtonDown(unsigned int flags,int px,int py);
	// callback for left button up event 
	virtual void OnLButtonUp(unsigned int flags,int px,int py);
	// callback for right button down event 
	virtual void OnRButtonDown(unsigned int flags,int px,int py);
	// callback for right button up event 
	virtual void OnRButtonUp(unsigned int flags,int px,int py);
	// callback for mouse move event 
	virtual void OnMouseMove(unsigned int flags,int px,int py);

	// action to take when tool is triggered via left mouse, or left mouse + drag
	virtual void OnTriggerLeft() {};
	// action to take when tool is triggered via right mouse, or right mouse + drag
	virtual void OnTriggerRight() {};

	// set current brush size
	void SetBrushSize(int size) { m_BrushSize=size; }
	// get current brush size
	int GetBrushSize() { return m_BrushSize; }

	// virtual function: allow multiple triggers by click and drag? - else requires individual clicks 
	// to invoke trigger .. default to off
	virtual bool SupportDragTrigger() { return false; }

protected:
	// build camera ray through screen point (px,py)
	void BuildCameraRay(int px,int py,CVector3D& origin,CVector3D& dir);
	// return true if given tile is on border of selection, false otherwise
	bool IsBorderSelection(int gx,int gz);
	// return true if given tile is a neighbour of the a border of selection, false otherwise
	bool IsNeighbourSelection(int gx,int gz);
	// build a selection of the given radius around the given minipatch
	void BuildSelection(int radius,CPatch* patch,CMiniPatch* minipatch);	
	// left mouse button currently down?
	bool m_LButtonDown;
	// right mouse button currently down?
	bool m_RButtonDown;
	// current tool brush size
	int m_BrushSize;
	// centre of current selection
	int m_SelectionCentre[2];
	// world space "projection" of mouse point - somewhere in the m_SelectionCentre tile
	CVector3D m_SelectionPoint;
};

#endif
