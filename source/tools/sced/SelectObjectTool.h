#ifndef _SELECTOBJECTTOOL_H
#define _SELECTOBJECTTOOL_H

#include <set>
#include "res/res.h"

#include "BrushTool.h"
#include "Vector3D.h"
#include "Matrix3D.h"
#include "Model.h"

class CUnit;
class CEntity;

class CSelectObjectTool : public CBrushTool
{
public:
	CSelectObjectTool();

	void OnDraw();

	// tool triggered via left mouse button; paint current selection
	void OnLButtonDown(unsigned int flags,int px,int py) { SelectObject(flags,px,py); }

	// return the entity of the first selected object, or null if it can't
	// (TODO: less hackiness, for the whole player-selection system)
	CEntity* GetFirstEntity();

	void DeleteSelected();

	// get the default select object instance
	static CSelectObjectTool* GetTool() { return &m_SelectObjectTool; }

private:
	// try and select the object under the cursor
	void SelectObject(unsigned int flags,int px,int py);
	
	// render bounding box round given unit
	void RenderUnitBounds(CUnit* unit);

	// list of currently selected units
	std::vector<CUnit*> m_SelectedUnits;

	// default tool instance
	static CSelectObjectTool m_SelectObjectTool;
};

#endif
