#ifndef _RENDERABLEOBJECT_H
#define _RENDERABLEOBJECT_H

#include "res/res.h"
#include "types.h"
#include "terrain/Bound.h"
#include "terrain/Matrix3D.h"


// dirty flags
#define RENDERDATA_UPDATE_VERTICES		(1<<1)
#define RENDERDATA_UPDATE_INDICES		(1<<2)

class CRenderData 
{
public:
	CRenderData() : m_UpdateFlags(0) {}
	virtual ~CRenderData() {}

	u32 m_UpdateFlags;
};

class CRenderableObject
{
public:
	CRenderableObject() : m_RenderData(0) {
		m_Transform.SetIdentity();
	}
	virtual ~CRenderableObject() { delete m_RenderData; }

	void SetTransform(const CMatrix3D& transform) {
		m_Transform=transform;
		CalcBounds();
	}
	const CMatrix3D& GetTransform() const { return m_Transform; }


	// CalcBounds: calculate (and store in m_Bounds) the world space bounds of this object
	virtual void CalcBounds() = 0;  
	const CBound& GetBounds() const { return m_Bounds; }

	// object renderdata
	CRenderData* m_RenderData;

protected:
	// object bounds
	CBound m_Bounds;
	// local->world space transform
	CMatrix3D m_Transform;
};

#endif
