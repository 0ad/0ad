///////////////////////////////////////////////////////////////////////////////
//
// Name:		RenderableObject.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RENDERABLEOBJECT_H
#define _RENDERABLEOBJECT_H

#include <assert.h>
#include "res/res.h"
#include "terrain/Bound.h"
#include "terrain/Matrix3D.h"


// dirty flags - used as notification to the renderer that some bit of data
// need updating
#define RENDERDATA_UPDATE_VERTICES		(1<<1)
#define RENDERDATA_UPDATE_INDICES		(1<<2)


///////////////////////////////////////////////////////////////////////////////
// CRenderData: base class of all the renderer's renderdata classes - the 
// derived class stores necessary information for rendering an object of a 
// particular type
class CRenderData 
{
public:
	CRenderData() : m_UpdateFlags(0) {}
	virtual ~CRenderData() {}

	u32 m_UpdateFlags;
};

///////////////////////////////////////////////////////////////////////////////
// CRenderableObject: base class of all renderable objects - patches, models,
// sprites, etc; stores position and bound information, and a pointer to 
// some renderdata necessary for the renderer to actually render it
class CRenderableObject
{
public:
	// constructor
	CRenderableObject() : m_RenderData(0) {
		m_Transform.SetIdentity();
	}
	// destructor
	virtual ~CRenderableObject() { delete m_RenderData; }

	// set object transform
	void SetTransform(const CMatrix3D& transform) {
		m_Transform=transform;
		CalcBounds();
	}
	// get object transform
	const CMatrix3D& GetTransform() const { return m_Transform; }

	// mark some part of the renderdata as dirty, and requiring
	// an update on next render
	void SetDirty(u32 dirtyflags) {
		if (m_RenderData) m_RenderData->m_UpdateFlags|=dirtyflags;
	}

	// calculate (and store in m_Bounds) the world space bounds of this object
	// - must be implemented by all concrete subclasses
	virtual void CalcBounds() = 0;  

	// return world space bounds of this object
	const CBound& GetBounds() const { return m_Bounds; }

	// set the object renderdata 
	// TODO,RC 10/04/04 - need to delete existing renderdata here, or can we
	// assume the renderer won't set renderdata when an object already has it?
	// - just assert we've no renderdata at the minute
	void SetRenderData(CRenderData* renderdata) { 
		assert(m_RenderData==0);
		m_RenderData=renderdata; 
	}

	// return object renderdata - can be null if renderer hasn't yet
	// created the renderdata
	CRenderData* GetRenderData() { return m_RenderData; }

protected:
	// object bounds
	CBound m_Bounds;
	// local->world space transform
	CMatrix3D m_Transform;
	// object renderdata
	CRenderData* m_RenderData;
};

#endif
