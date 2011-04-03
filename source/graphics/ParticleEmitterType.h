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

#ifndef INCLUDED_PARTICLEEMITTERTYPE
#define INCLUDED_PARTICLEEMITTERTYPE

#include "graphics/Texture.h"
#include "lib/ogl.h"
#include "lib/file/vfs/vfs_path.h"

class CParticleEmitter;
class CParticleManager;
class IParticleVar;

/**
 * Particle emitter type - stores the common state data for all emitters of that
 * type, and uses that data to update the emitter states.
 *
 * The data is initialised from XML files.
 *
 * Most of the emitter type data is represented as subclasses of IParticleVar,
 * which will typically return a constant value or a random value each time it's
 * evaluated. New subclasses can be added to support different random distributions,
 * etc.
 */
class CParticleEmitterType
{
	NONCOPYABLE(CParticleEmitterType);	// reference member
public:
	CParticleEmitterType(const VfsPath& path, CParticleManager& manager);

private:
	friend class CParticleEmitter;
	friend class CParticleVarConstant;
	friend class CParticleVarUniform;
	friend class CParticleVarCopy;

	enum
	{
		VAR_EMISSIONRATE,
		VAR_LIFETIME,
		VAR_ANGLE,
		VAR_VELOCITY_X,
		VAR_VELOCITY_Y,
		VAR_VELOCITY_Z,
		VAR_VELOCITY_ANGLE,
		VAR_SIZE,
		VAR_COLOR_R,
		VAR_COLOR_G,
		VAR_COLOR_B,
		VAR__MAX
	};

	int GetVariableID(const std::string& name);

	bool LoadXML(const VfsPath& path);

	void UpdateEmitter(CParticleEmitter& emitter, float dt);

	CTexturePtr m_Texture;

	GLenum m_BlendEquation;
	GLenum m_BlendFuncSrc;
	GLenum m_BlendFuncDst;

	size_t m_MaxParticles;

	typedef shared_ptr<IParticleVar> IParticleVarPtr;
	std::vector<IParticleVarPtr> m_Variables;

	CParticleManager& m_Manager;
};

typedef shared_ptr<CParticleEmitterType> CParticleEmitterTypePtr;

#endif // INCLUDED_PARTICLEEMITTERTYPE
