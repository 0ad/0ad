/////////////////////////////////////////////////////
//	File Name:	ParticleEngine.h
//	Date:		6/29/05
//	Author:		Will Dull
//	Purpose:	The particle engine system.
//				controls and maintain particles
//				through emitters that are passed
//				into each of the main functions.
/////////////////////////////////////////////////////

#ifndef _PARTICLEENGINE_H_
#define _PARTICLEENGINE_H_

#include "ParticleEmitter.h"
#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "ps/CLogger.h"
#include "ps/Loader.h"

#include "ogl.h"
#include "Renderer.h"

// Different textures
enum PText { DEFAULTTEXT, MAX_TEXTURES };
// Different emitters
enum PEmit { DEFAULTEMIT, MAX_EMIT };

class CParticleEngine
{
public:
	virtual ~CParticleEngine(void);


	//////////////////////////////////////////////////////////////
	//Func Name: GetInstance
	//Date: 8/1/05
	//Author: Will Dull
	//Purpose: returns the instance of the singleton class
	//////////////////////////////////////////////////////////////
	static CParticleEngine* GetInstance();

	//////////////////////////////////////////////////////////////
	//Func Name: DeleteInstance
	//Date: 8/1/05
	//Author: Will Dull
	//Purpose: deletes the instance of the singleton class
	//////////////////////////////////////////////////////////////
	static void DeleteInstance();

	//////////////////////////////////////////////////////////////
	//Func Name: initParticleSystem
	//Date: 6/29/05
	//Author: Will Dull
	//Out: True if particle system initialized correctly, 
	//	   false otherwise
	//Purpose: inits particle system.
	//////////////////////////////////////////////////////////////
	bool initParticleSystem(void);

	//////////////////////////////////////////////////////////////
	//Func Name: addEmitter
	//Date: 7/20/05
	//Author: Will Dull
	//In: emitter to add, texture type, id of emitter
	//Out: True if emitter added correctly, 
	//	   false otherwise
	//Purpose: adds the emitter to the engines list
	//////////////////////////////////////////////////////////////
	bool addEmitter(CEmitter *emitter, int type = DEFAULTTEXT, int ID = DEFAULTEMIT);

	//////////////////////////////////////////////////////////////
	//Func Name: findEmitter
	//Date: 7/21/05
	//Author: Will Dull
	//In: id of emitter to find
	//Out: the emitter if found, 
	//	   NULL otherwise
	//Purpose: finds the emitter using its ID
	//////////////////////////////////////////////////////////////
	CEmitter* findEmitter(int ID);

	//////////////////////////////////////////////////////////////
	//Func Name: updateEmitters
	//Date: 7/20/05
	//Author: Will Dull
	//Purpose: Checks if the emitter is ready to be deleted
	//		and removed. If not it calls the emitters update
	//		function.
	//////////////////////////////////////////////////////////////
	void updateEmitters();

	//////////////////////////////////////////////////////////////
	//Func Name: renderParticles
	//Date: 7/20/05
	//Author: Will Dull
	//Purpose: Renders the emitter and all it's particles
	//////////////////////////////////////////////////////////////
	void renderParticles();

	//////////////////////////////////////////////////////////////
	//Func Name: destroyAllEmitters
	//Date: 8/1/05
	//Author: Will Dull
	//In: fade - if true, will allow the emitter to fade itself out
	//	if false, emitter and particles will disappear instantly
	//Purpose: Destroys every active emitter on screen.
	//////////////////////////////////////////////////////////////
	void destroyAllEmitters(bool fade = true);

	//////////////////////////////////////////////////////////////
	//Func Name: cleanup
	//Date: 8/3/05
	//Author: Will Dull
	//Purpose: Any cleanup not done in the destructor.
	//////////////////////////////////////////////////////////////
	void cleanup();

	void EnterParticleContext(void);
	void LeaveParticleContext(void);

	int getTotalParticles() { return totalParticles; }
	void SetTotalParticles(int particles) { totalParticles = particles; }
	void AddToTotalParticles(int addAmount) { totalParticles += addAmount; }
	void SubToTotalParticles(int subAmount) { totalParticles -= subAmount; }

private:
	CParticleEngine(void);
	static CParticleEngine*  m_pInstance;    // The singleton instance

	CTexture idTexture[MAX_TEXTURES];
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
