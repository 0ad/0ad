#include "precompiled.h"

#include "CommandManager.h"
#include "Unit.h"
#include "Model.h"
#include "ObjectEntry.h"
#include "Terrain.h"
#include "Renderer.h"
#include "UnitManager.h"
#include "SelectObjectTool.h"
#include <algorithm>

// default tool instance
CSelectObjectTool CSelectObjectTool::m_SelectObjectTool;

CSelectObjectTool::CSelectObjectTool() 
{
	m_BrushSize=0;
}

void CSelectObjectTool::OnDraw()
{
	glColor3f(1,0,0);

	for (uint i=0;i<m_SelectedUnits.size();i++) {
		RenderUnitBounds(m_SelectedUnits[i]);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// SelectObject: try and select the object under the cursor
// TODO, RC - add support for CTRL, SHIFT + ALT modifiers
// TODO, RC - move selected objects to another tool? visual of selection
// currently disappears when tool changes
void CSelectObjectTool::SelectObject(unsigned int flags,int px,int py)
{
	// modifiers:
	//	CTRL - add to selection
	//	ALT - remove from selection

	// build camera ray
	CVector3D rayorigin,raydir;
	BuildCameraRay(px,py,rayorigin,raydir);

	// try and pick object with it
	CUnit* hit=g_UnitMan.PickUnit(rayorigin,raydir);
	if (hit) {
		if (flags & TOOL_MOUSEFLAG_CTRLDOWN) {
			// add unit to selection if not already there
			if (std::find(m_SelectedUnits.begin(),m_SelectedUnits.end(),hit)==m_SelectedUnits.end()) {
				m_SelectedUnits.push_back(hit);
			}
		} else if (flags & TOOL_MOUSEFLAG_ALTDOWN) {
			// add unit to selection if not already there
			typedef std::vector<CUnit*>::iterator Iter; 
			Iter iter=std::find(m_SelectedUnits.begin(),m_SelectedUnits.end(),hit);
			if (iter!=m_SelectedUnits.end()) {
				m_SelectedUnits.erase(iter);
			}
		} else {
			// just set hit as sole selection
			m_SelectedUnits.clear();
			m_SelectedUnits.push_back(hit);
		}
	} else {
		// clear selection unless some modifier begin applied
		if (!(flags & TOOL_MOUSEFLAG_CTRLDOWN) && !(flags & TOOL_MOUSEFLAG_ALTDOWN)) {
			m_SelectedUnits.clear();
		}
	}
}
	
/////////////////////////////////////////////////////////////////////////////////////////////////
// RenderUnitBounds: render a bounding box round given unit
void CSelectObjectTool::RenderUnitBounds(CUnit* unit)
{
	glPushMatrix();
	const CMatrix3D& transform=unit->GetModel()->GetTransform();
	glMultMatrixf(&transform._11);	

	const CBound& bounds=unit->GetModel()->GetObjectBounds();

	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[0].X,bounds[0].Y,bounds[0].Z);
	glVertex3f(bounds[0].X,bounds[0].Y,bounds[1].Z);
	glVertex3f(bounds[0].X,bounds[1].Y,bounds[1].Z);
	glVertex3f(bounds[0].X,bounds[1].Y,bounds[0].Z);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[1].X,bounds[0].Y,bounds[0].Z);
	glVertex3f(bounds[1].X,bounds[0].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[1].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[1].Y,bounds[0].Z);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[0].X,bounds[0].Y,bounds[0].Z);
	glVertex3f(bounds[0].X,bounds[0].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[0].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[0].Y,bounds[0].Z);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[0].X,bounds[1].Y,bounds[0].Z);
	glVertex3f(bounds[0].X,bounds[1].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[1].Y,bounds[1].Z);
	glVertex3f(bounds[1].X,bounds[1].Y,bounds[0].Z);
	glEnd();

	glPopMatrix();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// GetFirstEntity: return the entity of the first selected object
CEntity* CSelectObjectTool::GetFirstEntity()
{
	if (m_SelectedUnits.size() == 0)
		return NULL;
	return m_SelectedUnits[0]->GetEntity();
}