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
 * Sky settings, texture management and rendering.
 */

#include "precompiled.h"

#include <algorithm>

#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "maths/MathUtil.h"

#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/Loader.h"
#include "ps/Filesystem.h"

#include "renderer/SkyManager.h"
#include "renderer/Renderer.h"

#include "graphics/Camera.h"
#include "graphics/LightEnv.h"
#include "graphics/TextureManager.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// SkyManager implementation


///////////////////////////////////////////////////////////////////
// String names for each image, in order of the IMG_ constants
const wchar_t* SkyManager::s_imageNames[numTextures] = {
	L"front",
	L"back",
	L"right",
	L"left",
	L"top"
};


///////////////////////////////////////////////////////////////////
// Construction/Destruction
SkyManager::SkyManager()
{
	m_RenderSky = true;

	m_SkySet = L"";

	m_HorizonHeight = -150.0f;
}


///////////////////////////////////////////////////////////////////
// Load all sky textures
void SkyManager::LoadSkyTextures()
{
	for (size_t i = 0; i < ARRAY_SIZE(m_SkyTexture); ++i)
	{
		VfsPath path = Path::Join("art/textures/skies", Path::Join(NativePath(m_SkySet), NativePath(s_imageNames[i])+L".dds"));

		CTextureProperties textureProps(path);
		textureProps.SetWrap(GL_CLAMP_TO_EDGE);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_SkyTexture[i] = texture;
	}
}


///////////////////////////////////////////////////////////////////
// Switch to a different sky set (while the game is running)
void SkyManager::SetSkySet( const CStrW& newSet )
{
	if(newSet == m_SkySet)
		return;
	m_SkySet = newSet;

	LoadSkyTextures();
}

///////////////////////////////////////////////////////////////////
// Generate list of available skies
std::vector<CStrW> SkyManager::GetSkySets() const
{
	std::vector<CStrW> skies;

	// Find all subdirectories in art/textures/skies

	const VfsPath path(L"art/textures/skies/");
	DirectoryNames subdirectories;
	if(g_VFS->GetDirectoryEntries(path, 0, &subdirectories) < 0)
	{
		LOGERROR(L"Error opening directory '%ls'", path.c_str());
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	for(size_t i = 0; i < subdirectories.size(); i++)
		skies.push_back(subdirectories[i]);
	sort(skies.begin(), skies.end());

	return skies;
}

///////////////////////////////////////////////////////////////////
// Render sky
void SkyManager::RenderSky()
{
	// Draw the sky as a small box around the camera position, with depth write enabled.
	// This will be done before anything else is drawn so we'll be overlapped by everything else.

	// Note: The coordinates for this were set up through a rather cumbersome trial-and-error 
	//       process - there might be a smarter way to do it, but this seems to work.

	// Do nothing unless SetSkySet was called
	if (m_SkySet.empty())
		return;

	glDepthMask( GL_FALSE );

	pglActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	// Translate so we are at the camera in the X and Z directions, but
	// put the horizon at a fixed height regardless of camera Y
	const CCamera& camera = g_Renderer.GetViewCamera();
	CVector3D pos = camera.m_Orientation.GetTranslation();
	glTranslatef( pos.X, m_HorizonHeight, pos.Z );

	// Rotate so that the "left" face, which contains the brightest part of each
	// skymap, is in the direction of the sun from our light environment
	glRotatef( 90.0f + RADTODEG(g_Renderer.GetLightEnv().GetRotation()), 0.0f, 1.0f, 0.0f );

	// Distance to draw the faces at
	const float D = 2000.0;

	// Front face (positive Z)
	m_SkyTexture[FRONT]->Bind();
	glBegin( GL_QUADS );
		glTexCoord2f( 0, 1 );
		glVertex3f( -D, -D, +D );
		glTexCoord2f( 1, 1 );
		glVertex3f( +D, -D, +D );
		glTexCoord2f( 1, 0 );
		glVertex3f( +D, +D, +D );
		glTexCoord2f( 0, 0 );
		glVertex3f( -D, +D, +D );
	glEnd();

	// Back face (negative Z)
	m_SkyTexture[BACK]->Bind();
	glBegin( GL_QUADS );
		glTexCoord2f( 1, 1 );
		glVertex3f( -D, -D, -D );
		glTexCoord2f( 1, 0 );
		glVertex3f( -D, +D, -D );
		glTexCoord2f( 0, 0 );
		glVertex3f( +D, +D, -D );
		glTexCoord2f( 0, 1 );
		glVertex3f( +D, -D, -D );
	glEnd();

	// Right face (negative X)
	m_SkyTexture[RIGHT]->Bind();
	glBegin( GL_QUADS );
		glTexCoord2f( 0, 1 );
		glVertex3f( -D, -D, -D );
		glTexCoord2f( 1, 1 );
		glVertex3f( -D, -D, +D );
		glTexCoord2f( 1, 0 );
		glVertex3f( -D, +D, +D );
		glTexCoord2f( 0, 0 );
		glVertex3f( -D, +D, -D );
	glEnd();

	// Left face (positive X)
	m_SkyTexture[LEFT]->Bind();
	glBegin( GL_QUADS );
		glTexCoord2f( 1, 1 );
		glVertex3f( +D, -D, -D );
		glTexCoord2f( 1, 0 );
		glVertex3f( +D, +D, -D );
		glTexCoord2f( 0, 0 );
		glVertex3f( +D, +D, +D );
		glTexCoord2f( 0, 1 );
		glVertex3f( +D, -D, +D );
	glEnd();

	// Top face (positive Y)
	m_SkyTexture[TOP]->Bind();
	glBegin( GL_QUADS );
		glTexCoord2f( 1, 0 );
		glVertex3f( +D, +D, -D );
		glTexCoord2f( 0, 0 );
		glVertex3f( -D, +D, -D );
		glTexCoord2f( 0, 1 );
		glVertex3f( -D, +D, +D );
		glTexCoord2f( 1, 1 );
		glVertex3f( +D, +D, +D );
	glEnd();

	glPopMatrix();

	glDepthMask( GL_TRUE );
}
