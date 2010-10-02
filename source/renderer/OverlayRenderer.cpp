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

#include "OverlayRenderer.h"

#include "graphics/Overlay.h"
#include "graphics/TextureManager.h"
#include "lib/ogl.h"
#include "renderer/Renderer.h"

struct OverlayRendererInternals
{
	std::vector<SOverlayLine*> lines;
	std::vector<SOverlaySprite*> sprites;
};

OverlayRenderer::OverlayRenderer()
{
	m = new OverlayRendererInternals();
}

OverlayRenderer::~OverlayRenderer()
{
	delete m;
}

void OverlayRenderer::Submit(SOverlayLine* overlay)
{
	m->lines.push_back(overlay);
}

void OverlayRenderer::Submit(SOverlaySprite* overlay)
{
	m->sprites.push_back(overlay);
}

void OverlayRenderer::EndFrame()
{
	m->lines.clear();
	m->sprites.clear();
	// this should leave the capacity unchanged, which is okay since it
	// won't be very large or very variable
}

void OverlayRenderer::PrepareForRendering()
{
	// This is where we should do something like sort the overlays by
	// colour/sprite/etc for more efficient rendering
}

void OverlayRenderer::RenderOverlays()
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	for (size_t i = 0; i < m->lines.size(); ++i)
	{
		SOverlayLine* line = m->lines[i];
		if (line->m_Coords.empty())
			continue;

		debug_assert(line->m_Coords.size() % 3 == 0);

		glColor4fv(line->m_Color.FloatArray());
		glLineWidth((float)line->m_Thickness);

		glInterleavedArrays(GL_V3F, sizeof(float)*3, &line->m_Coords[0]);
		glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)line->m_Coords.size()/3);
	}

	glLineWidth(1.f);
	glDisable(GL_BLEND);
}

void OverlayRenderer::RenderForegroundOverlays(const CCamera& viewCamera)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	CVector3D right = -viewCamera.m_Orientation.GetLeft();
	CVector3D up = viewCamera.m_Orientation.GetUp();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	float uvs[8] = { 0,0, 1,0, 1,1, 0,1 };
	glTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, &uvs);

	for (size_t i = 0; i < m->sprites.size(); ++i)
	{
		SOverlaySprite* sprite = m->sprites[i];

		sprite->m_Texture->Bind();

		CVector3D pos[4] = {
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y1,
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y1
		};

		glVertexPointer(3, GL_FLOAT, sizeof(float)*3, &pos[0].X);

		glDrawArrays(GL_QUADS, 0, (GLsizei)4);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}
