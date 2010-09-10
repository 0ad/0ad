/* Copyright (C) 2010 Wildfire Games.
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
 * Particle engine implementation
 */

#ifndef INCLUDED_PARTICLEENGINE
#define INCLUDED_PARTICLEENGINE

#include "ParticleEmitter.h"
#include "lib/tex/tex.h"
#include "lib/res/graphics/ogl_tex.h"
#include "graphics/Texture.h"

#include "ps/CLogger.h"
#include "ps/Loader.h"

#include "lib/ogl.h"
#include "renderer/Renderer.h"

// Different textures
enum PText { DEFAULTTEXT, MAX_TEXTURES };
// Different emitters
enum PEmit { DEFAULTEMIT, MAX_EMIT };

class CParticleEngine
{
public:
	virtual ~CParticleEngine(void);


	/// @return instance of the singleton class
	static CParticleEngine* GetInstance();

	/// delete the instance of the singleton class
	static void DeleteInstance();

	/// @return true on success, false on failure
	bool InitParticleSystem(void);

	/**
	 * add the emitter to the engine's list.
	 * @return indicator of success.
	 **/
	bool AddEmitter(CEmitter *emitter, int type = DEFAULTTEXT, int ID = DEFAULTEMIT);

	/// @return emitter with the given ID or 0 if not found.
	CEmitter* FindEmitter(int ID);

	/**
	 * Check if the emitters are ready to be deleted and removed.
	 * If not, call Update() on them.
	 **/
	void UpdateEmitters();

	/// render each emitter and their particles
	void RenderParticles();

	/**
	 * destroy all active emitters on screen.
	 * @param fade if true, allows emitters to fade out. if false,
	 * they disappear instantly.
	 **/
	void DestroyAllEmitters(bool fade = true);

	/// do cleanup that's not done in the destructor.
	void Cleanup();

	void EnterParticleContext(void);
	void LeaveParticleContext(void);

	int GetTotalParticles() { return totalParticles; }
	void SetTotalParticles(int particles) { totalParticles = particles; }
	void AddToTotalParticles(int addAmount) { totalParticles += addAmount; }
	void SubToTotalParticles(int subAmount) { totalParticles -= subAmount; }

private:
	CParticleEngine(void);
	static CParticleEngine*  m_pInstance;    // The singleton instance

	CTexturePtr idTexture[MAX_TEXTURES];
	int totalParticles;					// Total Amount of particles of all emitters.

	struct tEmitterNode
	{
		CEmitter *pEmitter;
		tEmitterNode *prev, *next;
		int ID;
	};

	tEmitterNode *m_pHead;
	friend class CEmitter;
};

#endif
