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

#include "precompiled.h"

#include "Sprite.h"

#include "graphics/TextureManager.h"
#include "renderer/Renderer.h"
#include "lib/ogl.h"
#include "lib/res/graphics/ogl_tex.h"

CSprite::CSprite()
{
	// default scale 1:1
	m_scale.X = m_scale.Y = m_scale.Z = 1.0f;
	
	// default position (0.0f, 0.0f, 0.0f)
	m_translation.X = m_translation.Y = m_translation.Z = 0.0f;

	// default size 1.0 x 1.0
	SetSize(1.0f, 1.0f);

	// default colour, white
	m_colour[0] = m_colour[1] = m_colour[2] = m_colour[3] = 1.0f;
}

CSprite::~CSprite() 
{
}

void CSprite::Render() 
{	
	glPushMatrix();
	glTranslatef(m_translation.X, m_translation.Y, m_translation.Z);
	glScalef(m_scale.X, m_scale.Y, m_scale.Z);
	BeginBillboard();
	glDisable(GL_CULL_FACE);

	if (m_texture)
		m_texture->Bind();

	glColor4fv(m_colour);

	glBegin(GL_TRIANGLE_STRIP);
		// bottom left
		glTexCoord2f(0.0f, 0.0f);
		glVertex3fv(m_coords[0].GetFloatArray());
	
		// top left
		glTexCoord2f(0.0f, 1.0f);
		glVertex3fv(m_coords[1].GetFloatArray());

		// bottom right
		glTexCoord2f(1.0f, 0.0f);
		glVertex3fv(m_coords[2].GetFloatArray());

		// top left
		glTexCoord2f(1.0f, 1.0f);
		glVertex3fv(m_coords[3].GetFloatArray());
	glEnd();

	glEnable(GL_CULL_FACE);

	EndBillboard();
}

void CSprite::SetTexture(const CTexturePtr& texture)
{
	m_texture = texture;
}

void CSprite::SetSize(float width, float height) 
{
	m_width = width;
	m_height = height;

	float xOffset = m_width / 2;
	float yOffset = m_height / 2;

	// bottom left
	m_coords[0].X = - (xOffset);
	m_coords[0].Y = - (yOffset);
	m_coords[0].Z = 0.0f;

	// top left
	m_coords[1].X = - (xOffset);
	m_coords[1].Y = yOffset;
	m_coords[1].Z = 0.0f;

	// bottom right
	m_coords[2].X = xOffset;
	m_coords[2].Y = - (yOffset);
	m_coords[2].Z = 0.0f;

	// top right
	m_coords[3].X = xOffset;
	m_coords[3].Y = yOffset;
	m_coords[3].Z = 0.0f;
}

float CSprite::GetWidth() 
{
	return m_width;
}

void CSprite::SetWidth(float width) 
{
	SetSize(width, m_height);	
}

float CSprite::GetHeight() 
{
	return m_height;
}

void CSprite::SetHeight(float height) 
{
	SetSize(m_width, height);
}

CVector3D CSprite::GetTranslation() 
{
	return m_translation;
}

void CSprite::SetTranslation(CVector3D trans) 
{
	m_translation = trans;
}

void CSprite::SetTranslation(float x, float y, float z) 
{
	m_translation.X = x;
	m_translation.Y = y;
	m_translation.Z = z;
}

CVector3D CSprite::GetScale() 
{
	return m_scale;
}

void CSprite::SetScale(CVector3D scale) 
{
	m_scale = scale;
}

void CSprite::SetScale(float x, float y, float z) 
{
	m_scale.X = x;
	m_scale.Y = y;
	m_scale.Z = z;
}

void CSprite::SetColour(float * colour) 
{
	m_colour[0] = colour[0];
	m_colour[1] = colour[1];
	m_colour[2] = colour[2];
	m_colour[3] = colour[3];
}

void CSprite::SetColour(float r, float g, float b, float a)
{
	m_colour[0] = r;
	m_colour[1] = g;
	m_colour[2] = b;
	m_colour[3] = a;
}

//Must call glPushMatrix() before this. Should be called before any other gl calls
void CSprite::BeginBillboard() 
{
	float newMatrix[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 
							0.0f, 1.0f, 0.0f, 0.0f, 
							0.0f, 0.0f, 1.0f, 0.0f, 
							0.0f, 0.0f, 0.0f, 1.0f };
    float currentMatrix[16];

	glGetFloatv(GL_MODELVIEW_MATRIX, currentMatrix);
	newMatrix[0] = currentMatrix[0];
	newMatrix[1] = currentMatrix[4];
	newMatrix[2] = currentMatrix[8];
	newMatrix[4] = currentMatrix[1];
	newMatrix[5] = currentMatrix[5];
	newMatrix[6] = currentMatrix[9];
	newMatrix[8] = currentMatrix[2];
	newMatrix[9] = currentMatrix[6];
	newMatrix[10] = currentMatrix[10];

	glMultMatrixf(newMatrix);
}

void CSprite::EndBillboard() 
{
	glPopMatrix();
}
