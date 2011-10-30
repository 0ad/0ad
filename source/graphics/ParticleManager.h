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

#ifndef INCLUDED_PARTICLEMANAGER
#define INCLUDED_PARTICLEMANAGER

#include "graphics/ParticleEmitter.h"
#include "graphics/ParticleEmitterType.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/unordered_map.hpp>

class SceneCollector;

class CParticleManager
{
public:
	CParticleManager();
	~CParticleManager();

	CParticleEmitterTypePtr LoadEmitterType(const VfsPath& path);

	/**
	 * Tell the manager to handle rendering of an emitter that is no longer
	 * attached to a unit.
	 */
	void AddUnattachedEmitter(const CParticleEmitterPtr& emitter);

	/**
	 * Delete unattached emitters if we don't wish to see them anymore (like in actor viewer)
	 */
	void ClearUnattachedEmitters();

	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum);

	void Interpolate(float frameLength);

	float GetCurrentTime() const { return m_CurrentTime; }

	Status ReloadChangedFile(const VfsPath& path);

	/// Random number generator shared between all particle emitters.
	boost::mt19937 m_RNG;

private:
	float m_CurrentTime;

	std::list<CParticleEmitterPtr> m_UnattachedEmitters;

	boost::unordered_map<VfsPath, CParticleEmitterTypePtr> m_EmitterTypes;
};

#endif // INCLUDED_PARTICLEMANAGER
