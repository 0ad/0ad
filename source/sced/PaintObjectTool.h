#ifndef _PAINTOBJECTTOOL_H
#define _PAINTOBJECTTOOL_H

#include <set>
#include "res/res.h"

#include "BrushTool.h"
#include "Vector3D.h"
#include "Matrix3D.h"
#include "Model.h"

class CObjectEntry;
class CPaintObjectCommand;

class CPaintObjectTool : public CBrushTool
{
public:
	CPaintObjectTool();

	// draw this tool
	void OnDraw();

	// callback for left button up event
	void OnLButtonUp(unsigned int flags,int px,int py);

	// tool triggered via left mouse button; paint current selection
	void OnTriggerLeft() { PaintSelection(); }

	// allow multiple triggers by click and hold - subsequent triggers rotate
	// the last applied object, rather than adding new objects
	bool SupportDragTrigger() { return true; }

	// get the default paint model instance
	static CPaintObjectTool* GetTool() { return &m_PaintObjectTool; }

private:
	// apply current object to current selection
	void PaintSelection();
	// build the m_ObjectTransform member from current tile selection
	void BuildTransform();
	// currently active command, or null if no object currently being applied
	CPaintObjectCommand* m_PaintCmd;
	// Y-rotation of object currently being applied
	float m_Rotation;
	// position of object when first dropped
	CVector3D m_Position;
	// time of last trigger
	double m_LastTriggerTime;
	// current transform of selected object
	CMatrix3D m_ObjectTransform;
	// model of current object 
	CModel* m_Model;
	// default tool instance
	static CPaintObjectTool m_PaintObjectTool;
};

#endif
