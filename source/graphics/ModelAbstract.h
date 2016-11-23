/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_MODELABSTRACT
#define INCLUDED_MODELABSTRACT

#include "maths/BoundingBoxOriented.h"
#include "graphics/RenderableObject.h"
#include "ps/Shapes.h"
#include "simulation2/helpers/Player.h"

class CModel;
class CModelDecal;
class CModelParticleEmitter;

/**
 * Abstract base class for graphical objects that are used by units,
 * or as props attached to other CModelAbstract objects.
 * This includes meshes, terrain decals, and sprites.
 * These objects exist in a tree hierarchy.
 */
class CModelAbstract : public CRenderableObject
{
	NONCOPYABLE(CModelAbstract);

public:

	/**
	 * Describes a custom selection shape to be used for a model's selection box instead of the default
	 * recursive bounding boxes.
	 */
	struct CustomSelectionShape
	{
		enum EType {
			/// The selection shape is determined by an oriented box of custom, user-specified size.
			BOX,
			/// The selection shape is determined by a cylinder of custom, user-specified size.
			CYLINDER
		};

		EType m_Type; ///< Type of shape.
		float m_Size0; ///< Box width if @ref BOX, or radius if @ref CYLINDER
		float m_Size1; ///< Box depth if @ref BOX, or radius if @ref CYLINDER
		float m_Height; ///< Box height if @ref BOX, cylinder height if @ref CYLINDER
	};

public:

	CModelAbstract()
		: m_Parent(NULL), m_PositionValid(false), m_ShadingColor(1, 1, 1, 1), m_PlayerID(INVALID_PLAYER),
		  m_SelectionBoxValid(false), m_CustomSelectionShape(NULL)
	{ }

	~CModelAbstract()
	{
		delete m_CustomSelectionShape; // allocated and set externally by CCmpVisualActor, but our responsibility to clean up
	}

	virtual CModelAbstract* Clone() const = 0;

	/// Dynamic cast
	virtual CModel* ToCModel() { return NULL; }

	/// Dynamic cast
	virtual CModelDecal* ToCModelDecal() { return NULL; }

	/// Dynamic cast
	virtual CModelParticleEmitter* ToCModelParticleEmitter() { return NULL; }

	// (This dynamic casting is a bit ugly, but we won't have many subclasses
	// and this seems the easiest way to integrate with other code that wants
	// type-specific processing)

	/// Calls SetDirty on this model and all child objects.
	virtual void SetDirtyRec(int dirtyflags) = 0;

	/// Returns world space bounds of this object and all child objects.
	virtual const CBoundingBoxAligned GetWorldBoundsRec() { return GetWorldBounds(); } // default implementation

	/**
	 * Returns the world-space selection box of this model. Used primarily for hittesting against against a selection ray. The
	 * returned selection box may be empty to indicate that it does not wish to participate in the selection process.
	 */
	virtual const CBoundingBoxOriented& GetSelectionBox();

	virtual void InvalidateBounds()
	{
		m_BoundsValid = false;
		// a call to this method usually means that the model's transform has changed, i.e. it has moved or rotated, so we'll also
		// want to update the selection box accordingly regardless of the shape it is built from.
		m_SelectionBoxValid = false;
	}

	/// Sets a custom selection shape as described by a @p descriptor. Argument may be NULL
	/// if you wish to keep the default behaviour of using the recursively-calculated bounding boxes.
	void SetCustomSelectionShape(CustomSelectionShape* descriptor)
	{
		if (m_CustomSelectionShape != descriptor)
		{
			m_CustomSelectionShape = descriptor;
			m_SelectionBoxValid = false; // update the selection box when it is next requested
		}
	}

	/**
	 * Returns the (object-space) bounds that should be used to construct a selection box for this model and its children.
	 * May return an empty bound to indicate that this model and its children should not be selectable themselves, or should
	 * not be included in its parent model's selection box. This method is used for constructing the default selection boxes,
	 * as opposed to any boxes of custom shape specified by @ref m_CustomSelectionShape.
	 *
	 * If you wish your model type to be included in selection boxes, override this method and have it return the object-space
	 * bounds of itself, augmented recursively (via this method) with the object-space selection bounds of its children.
	 */
	virtual const CBoundingBoxAligned GetObjectSelectionBoundsRec() { return CBoundingBoxAligned::EMPTY; }

	/**
	 * Called when terrain has changed in the given inclusive bounds.
	 * Might call SetDirty if the change affects this model.
	 */
	virtual void SetTerrainDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1) = 0;

	/**
	 * Called when the entity tries to set some variable to affect the display of this model
	 * and/or its child objects.
	 */
	virtual void SetEntityVariable(const std::string& UNUSED(name), float UNUSED(value)) { }

	/**
	 * Ensure that both the transformation and the bone matrices are correct for this model and all its props.
	 */
	virtual void ValidatePosition() = 0;

	/**
	 * Mark this model's position and bone matrices, and all props' positions as invalid.
	 */
	virtual void InvalidatePosition() = 0;

	virtual void SetPlayerID(player_id_t id) { m_PlayerID = id; }

	// get the model's player ID; initial default is INVALID_PLAYER
	virtual player_id_t GetPlayerID() const { return m_PlayerID; }

	virtual void SetShadingColor(const CColor& color) { m_ShadingColor = color; }
	virtual CColor GetShadingColor() const { return m_ShadingColor; }

protected:
	void CalcSelectionBox();

public:
	/// If non-null, points to the model that we are attached to.
	CModelAbstract* m_Parent;

	/// True if both transform and and bone matrices are valid.
	bool m_PositionValid;

	player_id_t m_PlayerID;

	/// Modulating color
	CColor m_ShadingColor;

protected:

	/// Selection box for this model.
	CBoundingBoxOriented m_SelectionBox;

	/// Is the current selection box valid?
	bool m_SelectionBoxValid;

	/// Pointer to a descriptor for a custom-defined selection box shape. If no custom selection box is required, this is NULL
	/// and the standard recursive-bounding-box-based selection box is used. Otherwise, a custom selection box described by this
	/// field will be used.
	/// @see SetCustomSelectionShape
	CustomSelectionShape* m_CustomSelectionShape;

};

#endif // INCLUDED_MODELABSTRACT
