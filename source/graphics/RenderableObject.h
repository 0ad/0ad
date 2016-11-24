/* Copyright (C) 2012 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Base class for renderable objects
 */

#ifndef INCLUDED_RENDERABLEOBJECT
#define INCLUDED_RENDERABLEOBJECT


#include "maths/BoundingBoxAligned.h"
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
	NONCOPYABLE(CRenderableObject);

public:
	// constructor
	CRenderableObject() : m_RenderData(0), m_BoundsValid(false)
	{
		m_Transform.SetIdentity();
	}
	// destructor
	virtual ~CRenderableObject() { delete m_RenderData; }

	// set object transform
	virtual void SetTransform(const CMatrix3D& transform)
	{
		if (m_Transform == transform)
			return;
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
	void SetDirty(u32 dirtyflags)
	{
		if (m_RenderData)
			m_RenderData->m_UpdateFlags |= dirtyflags;
	}

	/**
	 * (Re)calculates and stores any bounds or bound-dependent data for this object. At this abstraction level, this is only the world-space
	 * bounds stored in @ref m_WorldBounds; subclasses may use this method to (re)compute additional bounds if necessary, or any data that
	 * depends on the bounds. Whenever bound-dependent data is requested through a public interface, @ref RecalculateBoundsIfNecessary should
	 * be called first to ensure bound correctness, which will in turn call this method if it turns out that they're outdated.
	 *
	 * @see m_BoundsValid
	 * @see RecalculateBoundsIfNecessary
	 */
	virtual void CalcBounds() = 0;

	/// Returns the world-space axis-aligned bounds of this object.
	const CBoundingBoxAligned& GetWorldBounds()
	{
		RecalculateBoundsIfNecessary();
		return m_WorldBounds;
	}

	/**
	 * Marks the bounds as invalid. This will trigger @ref RecalculateBoundsIfNecessary to recompute any bound-related data the next time
	 * any bound-related data is requested through a public interface -- at least, if you've made sure to call it before returning the
	 * stored data.
	 */
	virtual void InvalidateBounds() { m_BoundsValid = false; }

	// Set the object renderdata and free previous renderdata, if any.
	void SetRenderData(CRenderData* renderdata)
	{
		delete m_RenderData;
		m_RenderData = renderdata;
	}

	/// Return object renderdata - can be null if renderer hasn't yet created the renderdata
	CRenderData* GetRenderData() { return m_RenderData; }

protected:
	/// Factored out so subclasses don't need to repeat this if they want to add additional getters for bounds-related methods
	/// (since they'll have to make sure to recalc the bounds if necessary before they return it).
	void RecalculateBoundsIfNecessary()
	{
		if (!m_BoundsValid) {
			CalcBounds();
			m_BoundsValid = true;
		}
	}

protected:
	/// World-space bounds of this object
	CBoundingBoxAligned m_WorldBounds;
	// local->world space transform
	CMatrix3D m_Transform;
	// world->local space transform
	CMatrix3D m_InvTransform;
	// object renderdata
	CRenderData* m_RenderData;

	/**
	 * Remembers whether any bounds need to be recalculated. Subclasses that add any data that depends on the bounds should
	 * take care to consider the validity of the bounds and recalculate their data when necessary -- overriding @ref CalcBounds
	 * to do so would be a good idea, since it's already set up to be called by @ref RecalculateBoundsIfNecessary whenever the
	 * bounds are marked as invalid. The latter should then be called before returning any bounds or bounds-derived data through
	 * a public interface (see the implementation of @ref GetWorldBounds for an example).
	 *
	 * @see CalcBounds
	 * @see InvalidateBounds
	 * @see RecalculateBoundsIfNecessary
	 */
	bool m_BoundsValid;
};

#endif
