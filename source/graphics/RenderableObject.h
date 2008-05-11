/**
 * =========================================================================
 * File        : RenderableObject.h
 * Project     : 0 A.D.
 * Description : Base class for renderable objects
 * =========================================================================
 */

#ifndef INCLUDED_RENDERABLEOBJECT
#define INCLUDED_RENDERABLEOBJECT


#include "maths/Bound.h"
#include "maths/Matrix3D.h"


// dirty flags - used as notification to the renderer that some bit of data
// need updating
#define RENDERDATA_UPDATE_VERTICES		(1<<1)
#define RENDERDATA_UPDATE_INDICES		(1<<2)
#define RENDERDATA_UPDATE_COLOR			(1<<4)


///////////////////////////////////////////////////////////////////////////////
// CRenderData: base class of all the renderer's renderdata classes - the
// derived class stores necessary information for rendering an object of a
// particular type
class CRenderData
{
public:
	CRenderData() : m_UpdateFlags(0) {}
	virtual ~CRenderData() {}

	int m_UpdateFlags;
};

///////////////////////////////////////////////////////////////////////////////
// CRenderableObject: base class of all renderable objects - patches, models,
// sprites, etc; stores position and bound information, and a pointer to
// some renderdata necessary for the renderer to actually render it
class CRenderableObject
{
public:
	// constructor
	CRenderableObject() : m_RenderData(0), m_BoundsValid(false) {
		m_Transform.SetIdentity();
	}
	// destructor
	virtual ~CRenderableObject() { delete m_RenderData; }

	// set object transform
	virtual void SetTransform(const CMatrix3D& transform) {
		// store transform, calculate inverse
		m_Transform=transform;
		m_Transform.GetInverse(m_InvTransform);
		// normal recalculation likely required on transform change; flag it
		SetDirty(RENDERDATA_UPDATE_VERTICES);
		// need to rebuild world space bounds
		InvalidateBounds();
	}
	// get object to world space transform
	const CMatrix3D& GetTransform() const { return m_Transform; }
	// get world to object space transform
	const CMatrix3D& GetInvTransform() const { return m_InvTransform; }

	// mark some part of the renderdata as dirty, and requiring
	// an update on next render
	void SetDirty(u32 dirtyflags) {
		if (m_RenderData) m_RenderData->m_UpdateFlags|=dirtyflags;
	}

	// calculate (and store in m_Bounds) the world space bounds of this object
	// - must be implemented by all concrete subclasses
	virtual void CalcBounds() = 0;

	// return world space bounds of this object
	const CBound& GetBounds() {
		if (! m_BoundsValid) {
			CalcBounds();
			m_BoundsValid = true;
		}
		return m_Bounds;
	}

	void InvalidateBounds() { m_BoundsValid = false; }

	// Set the object renderdata and free previous renderdata, if any.
	void SetRenderData(CRenderData* renderdata) {
		delete m_RenderData;
		m_RenderData = renderdata;
	}

	// return object renderdata - can be null if renderer hasn't yet
	// created the renderdata
	CRenderData* GetRenderData() { return m_RenderData; }

protected:
	// object bounds
	CBound m_Bounds;
	// local->world space transform
	CMatrix3D m_Transform;
	// world->local space transform
	CMatrix3D m_InvTransform;
	// object renderdata
	CRenderData* m_RenderData;

private:
	// remembers whether m_bounds needs to be recalculated
	bool m_BoundsValid;
};

#endif
