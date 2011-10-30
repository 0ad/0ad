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

#include "precompiled.h"

#include "ParticleManager.h"

#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

static Status ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CParticleManager*>(param)->ReloadChangedFile(path);
}

CParticleManager::CParticleManager() :
	m_CurrentTime(0.f)
{
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CParticleManager::~CParticleManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

CParticleEmitterTypePtr CParticleManager::LoadEmitterType(const VfsPath& path)
{
	boost::unordered_map<VfsPath, CParticleEmitterTypePtr>::iterator it = m_EmitterTypes.find(path);
	if (it != m_EmitterTypes.end())
		return it->second;

	CParticleEmitterTypePtr emitterType(new CParticleEmitterType(path, *this));
	m_EmitterTypes[path] = emitterType;
	return emitterType;
}

void CParticleManager::AddUnattachedEmitter(const CParticleEmitterPtr& emitter)
{
	m_UnattachedEmitters.push_back(emitter);
}

void CParticleManager::ClearUnattachedEmitters()
{
	m_UnattachedEmitters.clear();
}

void CParticleManager::Interpolate(float frameLength)
{
	m_CurrentTime += frameLength;
}

struct EmitterHasNoParticles
{
	bool operator()(const CParticleEmitterPtr& emitterPtr)
	{
		CParticleEmitter& emitter = *emitterPtr.get();
		for (size_t i = 0; i < emitter.m_Particles.size(); ++i)
		{
			SParticle& p = emitter.m_Particles[i];
			if (p.age < p.maxAge)
				return false;
		}
		return true;
	}
};

void CParticleManager::RenderSubmit(SceneCollector& collector, const CFrustum& UNUSED(frustum))
{
	PROFILE("submit unattached particles");

	// Delete any unattached emitters that have no particles left
	m_UnattachedEmitters.remove_if(EmitterHasNoParticles());

	// TODO: should do some frustum culling

	for (std::list<CParticleEmitterPtr>::iterator it = m_UnattachedEmitters.begin(); it != m_UnattachedEmitters.end(); ++it)
		collector.Submit(it->get());
}

Status CParticleManager::ReloadChangedFile(const VfsPath& path)
{
	m_EmitterTypes.erase(path);
	return INFO::OK;
}
