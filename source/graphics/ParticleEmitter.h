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

#ifndef INCLUDED_PARTICLEEMITTER
#define INCLUDED_PARTICLEEMITTER

#include "graphics/ModelAbstract.h"
#include "graphics/ParticleEmitterType.h"
#include "graphics/Texture.h"
#include "maths/Quaternion.h"
#include "renderer/VertexArray.h"

#include <map>

/**
 * Simulation state for a single particle.
 */
struct SParticle
{
	CVector3D pos;
	CVector3D velocity;
	float angle;
	float angleSpeed;
	float size;
	float sizeGrowthRate;
	SColor4ub color;
	float age;
	float maxAge;
};

typedef shared_ptr<CParticleEmitter> CParticleEmitterPtr;

/**
 * Particle emitter.
 *
 * Emitters store particle data in two forms:
 *  * m_Particles contains the raw data used for the CPU particle simulation.
 *  * m_VertexArray contains the data required for rendering.
 * Particles are rendered as billboard quads, so the vertex array contains four vertices
 * per particle with different UV coordinates. The billboard position computation is
 * performed by a vertex shader.
 *
 * The number of particles is a constant for the entire life of the emitter,
 * to simplify the updating and rendering.
 * m_Particles acts like a ring buffer, so we don't have to worry about dynamically
 * allocating particles. If particles have variable lifetimes, they'll exist in the
 * array with alpha=0 until they're overwritten by a new particle after the maximum
 * lifetime.
 *
 * (It's quite likely this could be made more efficient, if the overhead of any added
 * complexity is not high.)
 */
class CParticleEmitter
{
public:
	CParticleEmitter(const CParticleEmitterTypePtr& type);

	/**
	 * Set the position to be used for emission of new particles.
	 */
	void SetPosition(const CVector3D& pos)
	{
		m_Pos = pos;
	}

	CVector3D GetPosition() const
	{
		return m_Pos;
	}

	/**
	 * Set the rotation to be used for emission of new particles (note: depends on particles).
	 */
	void SetRotation(const CQuaternion& rot)
	{
		m_Rot = rot;
	}
	
	CQuaternion GetRotation() const
	{
		return m_Rot;
	}

	/**
	 * Get the bounding box of the center points of particles at their current positions.
	 */
	CBoundingBoxAligned GetParticleBounds() { return m_ParticleBounds; }

	/**
	 * Push a new particle onto the ring buffer. (May overwrite an old particle.)
	 */
	void AddParticle(const SParticle& particle);

	/**
	 * Update particle and vertex array data. Must be called before RenderArray.
	 *
	 * If frameNumber is the same as the previous call to UpdateArrayData,
	 * then the function will do no work and return immediately.
	 */
	void UpdateArrayData(int frameNumber);

	/**
	 * Bind rendering state (textures and blend modes).
	 */
	void Bind(const CShaderProgramPtr& shader);

	/**
	 * Draw the vertex array.
	 */
	void RenderArray(const CShaderProgramPtr& shader);

	/**
	 * Stop this emitter emitting new particles, and pass responsibility for rendering
	 * to the CParticleManager. This should be called before dropping the last shared_ptr
	 * to this object so that it will carry on rendering (until all particles have dissipated)
	 * even when it's no longer attached to a model.
	 * @param self the shared_ptr you're about to drop
	 */
	void Unattach(const CParticleEmitterPtr& self);

	void SetEntityVariable(const std::string& name, float value);

	CParticleEmitterTypePtr m_Type;

	/// Whether this emitter is still emitting new particles
	bool m_Active;

	CVector3D m_Pos;
	CQuaternion m_Rot;

	std::map<std::string, float> m_EntityVariables;

	std::vector<SParticle> m_Particles;
	size_t m_NextParticleIdx;

	float m_LastUpdateTime;
	float m_EmissionRoundingError;

private:
	/// Bounding box of the current particle center points
	CBoundingBoxAligned m_ParticleBounds;

	VertexIndexArray m_IndexArray;

	VertexArray m_VertexArray;
	VertexArray::Attribute m_AttributePos;
	VertexArray::Attribute m_AttributeAxis;
	VertexArray::Attribute m_AttributeUV;
	VertexArray::Attribute m_AttributeColor;

	int m_LastFrameNumber;
};

/**
 * Particle emitter model, for attaching emitters as props on other models.
 */
class CModelParticleEmitter : public CModelAbstract
{
public:
	CModelParticleEmitter(const CParticleEmitterTypePtr& type);
	~CModelParticleEmitter();

	/// Dynamic cast
	virtual CModelParticleEmitter* ToCModelParticleEmitter()
	{
		return this;
	}

	virtual CModelAbstract* Clone() const;

	virtual void SetDirtyRec(int dirtyflags)
	{
		SetDirty(dirtyflags);
	}

	virtual void SetTerrainDirty(ssize_t UNUSED(i0), ssize_t UNUSED(j0), ssize_t UNUSED(i1), ssize_t UNUSED(j1))
	{
	}

	virtual void SetEntityVariable(const std::string& name, float value);

	virtual void CalcBounds();
	virtual void ValidatePosition();
	virtual void InvalidatePosition();
	virtual void SetTransform(const CMatrix3D& transform);

	CParticleEmitterTypePtr m_Type;
	CParticleEmitterPtr m_Emitter;
};

#endif // INCLUDED_PARTICLEEMITTER
