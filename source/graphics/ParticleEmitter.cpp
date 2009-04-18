/* Copyright (C) 2009 Wildfire Games.
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
 * Particle and Emitter base classes.
 */

#include "precompiled.h"
#include "ParticleEmitter.h"
#include "ParticleEngine.h"
#include "ps/XML/Xeromyces.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "particleSystem"

//forward declaration
void GetValueAndVariation(CXeromyces XeroFile, XMBElement parent, CStr& value, CStr& variation);

CEmitter::CEmitter(const int MAX_PARTICLES, const int lifetime, int UNUSED(textureID))
{
	m_particleCount = 0;
	// declare the pool of nodes
	m_maxParticles = MAX_PARTICLES;
	m_heap = new tParticle[m_maxParticles];
	m_emitterLife = lifetime;
	m_decrementLife = true;
	m_decrementAlpha = true;
	m_renderParticles = true;
	isFinished = false;
	m_texture = NULL;
	
	// init the used/open list
	m_usedList = NULL;
	m_openList = NULL;

	// link all the particles in the heap
	//	into one large open list
	for(int i = 0; i < m_maxParticles - 1; i++)
	{
		m_heap[i].next = &(m_heap[i + 1]);	 
	}
	m_openList = m_heap;
}

CEmitter::~CEmitter(void)
{
	delete [] m_heap;
}

bool CEmitter::LoadFromXML(const CStr& filename)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		// Fail
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	// Only the ones we can't load using normal methods.
	EL(Emitter);
	AT(Type);
	EL(Lifetime);
	EL(Particles);
	AT(MaxNumber);
	EL(EmitsPerFrame);
	EL(Texture);
	EL(Size);
	EL(Value);
	EL(Variation);
	EL(Color);
	EL(Start);
	EL(End);
	AT(r);
	AT(g);
	AT(b);
	EL(Alpha);
	AT(BlendMode);
	AT(Decrement);
	EL(Direction);
	EL(Yaw);
	EL(Pitch);
	EL(Speed);
	EL(Life);
	EL(Force);
	EL(X);
	EL(Y);
	EL(Z);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.GetRoot();

	if( root.GetNodeName() != el_Emitter )
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CEmitter::LoadEmitterXML: XML root was not \"Emitter\" in file %s. Load failed.", filename.c_str() );
		return( false );
	}

	m_tag = CStr(filename).AfterLast("/").BeforeLast(".xml");

	//TODO figure out if we need to use Type attribute to construct different emitter types, 
	// probably have to move some of this code into a static factory method or out into ParticleEngine class
	XMBAttributeList attributes = root.GetAttributes();
	CStr type = attributes.GetNamedItem(at_Type);
	
	CStr stringValue;
	XMBElementList children = root.GetChildNodes();
	for (int i = 0; i < children.Count; ++i)
	{
		XMBElement child = children.Item(i);
		int childName = child.GetNodeName();
		if( childName == el_Lifetime )
		{
			stringValue = child.GetText();
            m_emitterLife = stringValue.ToInt();
			if( m_emitterLife < 0 )
				m_emitterLife = -1;
		}
		else if( childName == el_Particles )
		{
			attributes = child.GetAttributes();
			stringValue = attributes.GetNamedItem(at_MaxNumber);
			m_maxParticles = stringValue.ToInt();

			XMBElementList particleSettings = child.GetChildNodes();
			for (int j = 0; j < particleSettings.Count; ++j)
			{
				XMBElement settingElement = particleSettings.Item(j);
				int settingName = settingElement.GetNodeName();
				if( settingName == el_EmitsPerFrame )
				{
					CStr value, variation;
					GetValueAndVariation(XeroFile, settingElement, value, variation);
					m_emitsPerFrame = value.ToInt();
					m_emitsVar = variation.ToInt();
				}
				else if( settingName == el_Texture )
				{
					stringValue = settingElement.GetText();
					m_texture = new CTexture(stringValue);
					u32 flags = 0;
					if(!(CRenderer::GetSingletonPtr()->LoadTexture(m_texture, flags)))
						return false;
				}
				else if( settingName == el_Size )
				{
					stringValue = settingElement.GetText();
					m_size = stringValue.ToFloat();
				}
				else if( settingName == el_Color )
				{
					XMBElementList colorElementList = settingElement.GetChildNodes();
					for (int k = 0; k < colorElementList.Count; ++k)
					{
						XMBElement colorElement = colorElementList.Item(k);
						int colorName = colorElement.GetNodeName();
						if( colorName == el_Start )
						{
							XMBElementList startColorElementList = colorElement.GetChildNodes();
							for (int m = 0; m < startColorElementList.Count; ++m)
							{
								XMBElement startColorElement = startColorElementList.Item(m);
								int startColorElementName = startColorElement.GetNodeName();
								if( startColorElementName == el_Value )
								{
									attributes = startColorElement.GetAttributes();
									stringValue = attributes.GetNamedItem(at_r);
									m_startColor.r = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_g);
									m_startColor.g = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_b);
									m_startColor.b = stringValue.ToInt();
								}
								else if( startColorElementName == el_Variation )
								{
									attributes = startColorElement.GetAttributes();
									stringValue = attributes.GetNamedItem(at_r);
									m_startColorVar.r = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_g);
									m_startColorVar.g = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_b);
									m_startColorVar.b = stringValue.ToInt();
								}			
							}
						}
						else if( colorName == el_End )
						{
							XMBElementList endColorElementList = colorElement.GetChildNodes();
							for (int m = 0; m < endColorElementList.Count; ++m)
							{
								XMBElement endColorElement = endColorElementList.Item(m);
								int endColorElementName = endColorElement.GetNodeName();
								if( endColorElementName == el_Value )
								{
									attributes = endColorElement.GetAttributes();
									stringValue = attributes.GetNamedItem(at_r);
									m_endColor.r = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_g);
									m_endColor.g = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_b);
									m_endColor.b = stringValue.ToInt();
								}
								else if( endColorElementName == el_Variation )
								{
									attributes = endColorElement.GetAttributes();
									stringValue = attributes.GetNamedItem(at_r);
									m_endColorVar.r = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_g);
									m_endColorVar.g = stringValue.ToInt();
									stringValue = attributes.GetNamedItem(at_b);
									m_endColorVar.b = stringValue.ToInt();
								}			
							}
						}
					}
				}
				else if( settingName == el_Alpha )
				{
					attributes = settingElement.GetAttributes();
					stringValue = attributes.GetNamedItem(at_BlendMode);
					m_blendMode = stringValue.ToInt();
					stringValue = attributes.GetNamedItem(at_Decrement);
					if (stringValue == "True" || stringValue == "true")
						m_decrementAlpha = true;
					else
						m_decrementAlpha = false;
					CStr value, variation;
					GetValueAndVariation(XeroFile, settingElement, value, variation);
					m_alpha = value.ToInt();
					m_alphaVar = value.ToInt();
				}
				else if( settingName == el_Direction )
				{
					XMBElementList directionElementList = settingElement.GetChildNodes();
					for (int k = 0; k < directionElementList.Count; ++k)
					{
						XMBElement directionElement = directionElementList.Item(k);
						int directionElementName = directionElement.GetNodeName();
						if( directionElementName == el_Yaw )
						{
							CStr value, variation;
							GetValueAndVariation(XeroFile, directionElement, value, variation);
							m_yaw = value.ToInt();
							m_yawVar = variation.ToInt();
						}
						else if( directionElementName == el_Pitch )
						{
							CStr value, variation;
							GetValueAndVariation(XeroFile, directionElement, value, variation);
							m_pitch = value.ToInt();
							m_pitchVar = variation.ToInt();
						}
						else if( directionElementName == el_Speed )
						{
							CStr value, variation;
							GetValueAndVariation(XeroFile, directionElement, value, variation);
							m_speed = value.ToFloat();
							m_speedVar = variation.ToFloat();
						}
					}
				}
				else if( settingName == el_Life )
				{
					attributes = settingElement.GetAttributes();
					stringValue = attributes.GetNamedItem(at_Decrement);
					if (stringValue == "True" || stringValue == "true")
						m_decrementLife = true;
					else
						m_decrementLife = false;
					CStr value, variation;
					GetValueAndVariation(XeroFile, settingElement, value, variation);
					m_life = value.ToInt();
					m_lifeVar = variation.ToInt();
				}
				else if( settingName == el_Force )
				{
					XMBElementList forceElementList = settingElement.GetChildNodes();
					for (int k = 0; k < forceElementList.Count; ++k)
					{
						XMBElement forceElement = forceElementList.Item(k);
						int forceElementName = forceElement.GetNodeName();
						if( forceElementName == el_X )
						{
							stringValue = forceElement.GetText();
							m_force.X = stringValue.ToFloat();
						}
						else if( forceElementName == el_Y )
						{
							stringValue = forceElement.GetText();
							m_force.Y = stringValue.ToFloat();
						}
						else if( forceElementName == el_Z )
						{
							stringValue = forceElement.GetText();
							m_force.Z = stringValue.ToFloat();
						}
					}
				}
			}
		}
	}
	return true;
}

bool CEmitter::AddParticle()
{
	tColor start, end;
	float fYaw, fPitch, fSpeed;

	if(!m_openList)
		return false;

	if(m_particleCount < m_maxParticles)
	{
		// get a particle from the open list
		tParticle *particle = m_openList;

		// set it's initial position to the emitter's position
		particle->pos.X = m_pos.X;
		particle->pos.Y = m_pos.Y;
		particle->pos.Z = m_pos.Z;

		// Calculate the starting direction vector
		fYaw = m_yaw + (m_yawVar * RandomNum());
		fPitch = m_pitch + (m_pitchVar * RandomNum());

		// Convert the rotations to a vector
		RotationToDirection(fPitch,fYaw,&particle->dir);

		// Multiply in the speed factor
		fSpeed = m_speed + (m_speedVar * RandomNum());
		particle->dir.X *= fSpeed;
		particle->dir.Y *= fSpeed;
		particle->dir.Z *= fSpeed;

		// Calculate the life span
		particle->life = m_life + (int)((float)m_lifeVar * RandomNum());

		// Calculate the colors
		start.r = m_startColor.r + (m_startColorVar.r * RandomChar());
		start.g = m_startColor.g + (m_startColorVar.g * RandomChar());
		start.b = m_startColor.b + (m_startColorVar.b * RandomChar());
		end.r = m_endColor.r + (m_endColorVar.r * RandomChar());
		end.g = m_endColor.g + (m_endColorVar.g * RandomChar());
		end.b = m_endColor.b + (m_endColorVar.b * RandomChar());

		// set the initial color of the particle
		particle->color.r = start.r;
		particle->color.g = start.g;
		particle->color.b = start.b;

		// Create the color delta
		particle->deltaColor.r = (end.r - start.r) / particle->life;
		particle->deltaColor.g = (end.g - start.g) / particle->life;
		particle->deltaColor.b = (end.b - start.b) / particle->life;

		//TODO: make this settable
		particle->alpha = 255.0f;
		particle->alphaDelta = particle->alpha / particle->life;

		particle->inPos = false;

		// Now, we pop a node from the open list and put it into the used list
		m_openList = particle->next;
		particle->next = m_usedList;
		m_usedList = particle;

		// update the length of the used list (particle Count)
		m_particleCount++;
		return true;
	}

	return false;
}

bool CEmitter::Render()
{
	if(m_renderParticles)
	{
		switch(m_blendMode)
		{
		case 1:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// Fire
			break;
		case 2:
			glBlendFunc(GL_SRC_COLOR, GL_ONE);						// Crappy Fire
			break;
		case 3:
			glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);		// Plain Particles
			break;
		case 4:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);		// Nice fade out effect
			break;
		}

		// Bind the texture. Use the texture assigned to this emitter.
		int unit = 0;
		g_Renderer.SetTexture(unit, m_texture);

		glBegin(GL_QUADS);
		{
			tParticle *tempParticle = m_usedList;
		
			while(tempParticle)
			{
				tColor *pColor = &(tempParticle->color);
				glColor4ub(pColor->r,pColor->g, pColor->b, (GLubyte)tempParticle->alpha);
				glTexCoord2d(0.0, 0.0);
				CVector3D *pPos = &(tempParticle->pos);
				glVertex3f(pPos->X - m_size, pPos->Y + m_size, pPos->Z);
				glTexCoord2d(0.0, 1.0);
				glVertex3f(pPos->X - m_size, pPos->Y - m_size, pPos->Z);
				glTexCoord2d(1.0, 1.0);
				glVertex3f(pPos->X + m_size, pPos->Y - m_size, pPos->Z);
				glTexCoord2d(1.0, 0.0);
				glVertex3f(pPos->X + m_size, pPos->Y + m_size, pPos->Z);
				tempParticle = tempParticle->next;
			}
		}
		glEnd();

		return true;
	}
	return false;
}

bool CEmitter::Update()
{
	int emits;
	// walk through the used list, and update each of the particles
	tParticle *tempParticle = m_usedList;			// start at the beginning of the used list
	tParticle *prev = m_usedList;					
	while(tempParticle)								// loop on a valid particle
	{
		// don't update if the particle is supposed to be dead
		if(tempParticle->life > 0)
		{
			// update the particle
			// Calculate the new pos
			tempParticle->pos.X += tempParticle->dir.X;
			tempParticle->pos.Y += tempParticle->dir.Y;
			tempParticle->pos.Z += tempParticle->dir.Z;

			// Add global force to direction
			tempParticle->dir.X += m_force.X;
			tempParticle->dir.Y += m_force.Y;
			tempParticle->dir.Z += m_force.Z;

			// Get the new color
			tempParticle->color.r += tempParticle->deltaColor.r;
			tempParticle->color.g += tempParticle->deltaColor.g;
			tempParticle->color.b += tempParticle->deltaColor.b;

			// fade it out
			if(m_decrementAlpha)
				tempParticle->alpha -= tempParticle->alphaDelta;

			// gets a little older
			if(m_decrementLife)
				tempParticle->life--;

			// move to the next particle in the list
			prev = tempParticle; 
			tempParticle = tempParticle->next;
		}
		else	// this means the particle lifetime is over
		{
			// if this is the first particle in usedList
			// then set the pointers to the next in the usedList
			// and open up the tempParticle
			if(tempParticle == m_usedList)
			{	
				m_usedList = tempParticle->next;
				tempParticle->next = m_openList;
				// set the open list head to the particle
				m_openList = tempParticle;
				prev = m_usedList;
				tempParticle = m_usedList;
			}
			else
			{

				//// We need to pull the particle out of the 
				//// used list and insert it into the open list
				
				// fix the previous node in the list to skip over the one we are pulling out
				prev->next = tempParticle->next;
				// set the particle to point to the head of the open list
				tempParticle->next = m_openList;
				// set the open list head to the particle
				m_openList = tempParticle;
				// move on to the next iteration
				tempParticle = prev->next;
			}
			// and there is one less
			m_particleCount--;
		}
	}	// end of while
	if(m_emitterLife > 0 || m_emitterLife == -1)
	{
		// Emit particles for this frame
		emits = m_emitsPerFrame + (int)((float)m_emitsVar * RandomNum());

		// if the emitter life is -1 that means it's infinite
		if(m_emitterLife != -1)
			m_emitterLife--;
		
		for(int i = 0; i < emits; i++)
			AddParticle();
		return true;
	}
	else
	{
		if(m_particleCount > 0)
		{	
			return true;
		}
		else
		{
			isFinished = true;
			return false;		// this will be checked for and then it will be deleted
		}
	}
}

void GetValueAndVariation(CXeromyces XeroFile, XMBElement parent, CStr& value, CStr& variation)
{
	int el_Value = XeroFile.GetElementID("value");
	int el_Variation = XeroFile.GetElementID("variation");

	XMBElementList elementList = parent.GetChildNodes();
	for (int i = 0; i < elementList.Count; ++i)
	{
		XMBElement child = elementList.Item(i);
		int childName = child.GetNodeName();
		if( childName == el_Value )
		{
			value = child.GetText();
		}
		else if( childName == el_Variation )
		{
			variation = child.GetText();
		}
	}
}
