#include "Visual.h"
#include "Model.h"

void CVisual::CalcBounds()
{
	m_Bounds.SetEmpty();

	for (int i=0; i<m_Model->GetModelDef()->GetNumVertices(); i++)
	{
		SModelVertex *pVertex = &m_Model->GetModelDef()->GetVertices()[i];
		CVector3D coord1,coord2;
		if (pVertex->m_Bone!=-1) {
			coord1=m_Model->GetBonePoses()[pVertex->m_Bone].Transform(pVertex->m_Coords);
		} else {
			coord1=pVertex->m_Coords;
		}
		m_Bounds+=m_Transform.Transform(coord1);
	}
}
