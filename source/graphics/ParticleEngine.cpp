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

#include "precompiled.h"

#include "ParticleEngine.h"

#include "graphics/TextureManager.h"

CParticleEngine *CParticleEngine::m_pInstance = 0;
CParticleEngine::CParticleEngine(void)
{
	m_pHead = NULL;
	totalParticles = 0;
}

CParticleEngine::~CParticleEngine(void)
{
}

void CParticleEngine::Cleanup()
{
	tEmitterNode *temp = m_pHead;
	totalParticles = 0;
	while(temp)
	{
		tEmitterNode *pTemp = temp->next;
		if(!temp->prev)
			m_pHead = temp->next;
		else
			temp->prev->next = temp->next;

		if(pTemp)
			temp->next->prev = temp->prev;

		delete temp->pEmitter;
		delete temp;
		temp = pTemp;
	}

	DeleteInstance();
}

CParticleEngine *CParticleEngine::GetInstance(void)
{
	// Check to see if one hasn't been made yet.
	if (m_pInstance == 0)
		m_pInstance = new CParticleEngine;

	// Return the address of the instance.
	return m_pInstance;
}

void CParticleEngine::DeleteInstance()
{
	if (m_pInstance)
		delete m_pInstance;
	m_pInstance = 0;
}

bool CParticleEngine::InitParticleSystem()
{
	// Texture Loading
	CTextureProperties textureProps(L"art/textures/particles/sprite.tga");
	CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);

	idTexture[DEFAULTTEXT] = texture;

	return true;
}

bool CParticleEngine::AddEmitter(CEmitter *emitter, int type, int ID)
{
	emitter->SetTexture(idTexture[type]);
	if(m_pHead == NULL)
	{
		tEmitterNode *temp = new tEmitterNode;
		temp->pEmitter = emitter;
		temp->prev = NULL;
		temp->next = NULL;
		temp->ID = ID;
		m_pHead = temp;
		return true;
	}
	else
	{
		tEmitterNode *temp = new tEmitterNode;
		temp->pEmitter = emitter;
		temp->next = m_pHead;
		temp->prev = NULL;
		temp->ID = ID;
		m_pHead->prev = temp;
		m_pHead = temp;
		return true;
	}
}

CEmitter* CParticleEngine::FindEmitter(int ID)
{
	tEmitterNode *temp = m_pHead;
	while(temp)
	{
		if(temp->ID < 0 || temp->ID > MAX_EMIT)
			continue;

		// NOTE: In the event that there are two different
		// emitters with the same ID, this will only
		// return the first one that it finds. So
		// make sure your emitter has a unique ID
		// if you want to use this function.
		if(temp->ID == ID)
			return temp->pEmitter;

		temp = temp->next;
	}

	// NOTE: Just in case this happens, it's VERY important
	// that you wrap this function in a if statement
	// to check for this condition because you could
	// end up with a crash if you try to change an
	// emitter when you couldn't find it and returned
	// NULL instead.
	return NULL;
}

void CParticleEngine::UpdateEmitters()
{
	tEmitterNode *temp = m_pHead;
	totalParticles = 0;
	while(temp)
	{
		// are we ready for deletion?
		if(temp->pEmitter->IsFinished())
		{
			// store a pointer to the next node
			tEmitterNode *pTemp = temp->next;

			// check for the head
			if(!temp->prev)
				m_pHead = pTemp;
			else
				// forward the previous's pointer
				temp->prev->next = pTemp;

			// if there is any next one,
			if(pTemp)
				// fix the backwards pointer
				pTemp->prev = temp->prev;

			delete temp->pEmitter;
			delete temp;
			temp = pTemp;
		}
		else
		{
			temp->pEmitter->Update();

			// Add current emitter to particle count
			totalParticles += temp->pEmitter->GetParticleCount();
			temp = temp->next;
		}
	}
}

void CParticleEngine::RenderParticles()
{
	EnterParticleContext();

	tEmitterNode *temp = m_pHead;
	while(temp)
	{
		temp->pEmitter->Render();
		temp = temp->next;
	}

	LeaveParticleContext();
}

void CParticleEngine::DestroyAllEmitters(bool fade)
{
	tEmitterNode *temp = m_pHead;
	while(temp)
	{
		if(fade)
		{
			temp->pEmitter->SetEmitterLife(0);
			temp = temp->next;
		}
		else
		{
			// store a pointer to the next node
			tEmitterNode *pTemp = temp->next;

			// check for the head
			if(!temp->prev)
				m_pHead = pTemp;
			else
				// forward the previous's pointer
				temp->prev->next = pTemp;

			// if there is any next one,
			if(pTemp)
				// fix the backwards pointer
				pTemp->prev = temp->prev;

			delete temp->pEmitter;
			delete temp;
			temp = pTemp;
		}
	}
	m_pHead = NULL;
	UpdateEmitters();
}

void CParticleEngine::EnterParticleContext(void)
{
	//glEnable(GL_DEPTH_TEST);               // Enable depth testing for hidden surface removal.
	glDepthMask(false);
	//glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);               // Enable texture mapping.
	glPushMatrix();
	glEnable(GL_BLEND);
}

void CParticleEngine::LeaveParticleContext(void)
{
	//glDisable(GL_DEPTH_TEST);
	glDepthMask(true);
	//glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glDisable(GL_BLEND);
}
