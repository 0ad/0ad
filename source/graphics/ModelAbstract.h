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

#include "graphics/RenderableObject.h"
#include "ps/Overlay.h"
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
	CModelAbstract() :
		m_Parent(NULL), m_PositionValid(false),
		m_ShadingColor(1, 1, 1, 1), m_PlayerID(INVALID_PLAYER)
	{
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

	/**
	 * Calls SetDirty on this model and all child objects.
	 */
	virtual void SetDirtyRec(int dirtyflags) = 0;

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
	 * Ensure that both the transformation and the bone
	 * matrices are correct for this model and all its props.
	 */
	virtual void ValidatePosition() = 0;

	/**
	 * Mark this model's position and bone matrices,
	 * and all props' positions as invalid.
	 */
	virtual void InvalidatePosition() = 0;

	virtual void SetPlayerID(player_id_t id) { m_PlayerID = id; }

	// get the model's player ID; initial default is INVALID_PLAYER
	virtual player_id_t GetPlayerID() const { return m_PlayerID; }

	virtual void SetShadingColor(const CColor& colour) { m_ShadingColor = colour; }
	virtual CColor GetShadingColor() const { return m_ShadingColor; }

	/// If non-null points to the model that we are attached to.
	CModelAbstract* m_Parent;

	/// True if both transform and and bone matrices are valid.
	bool m_PositionValid;

	player_id_t m_PlayerID;

	// modulating color
	CColor m_ShadingColor;
};

#endif // INCLUDED_MODELABSTRACT
