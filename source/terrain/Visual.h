#ifndef _VISUAL_H
#define _VISUAL_H

#include "RenderableObject.h"

class CModel;

class CVisual : public CRenderableObject
{
public:
	CVisual() : m_Model(0) {}

	void CalcBounds();

	CModel* m_Model;
};

#endif