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

/**
 * =========================================================================
 * File        : SkyManager.cpp
 * Project     : Pyrogenesis
 * Description : Sky settings, texture management and rendering.
 * =========================================================================
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

#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// SkyManager implementation


///////////////////////////////////////////////////////////////////
// String names for each image, in order of the IMG_ constants
const char* SkyManager::IMAGE_NAMES[5] = {
	"front",
	"back",
	"right",
	"left",
	"top"
};


///////////////////////////////////////////////////////////////////
// Construction/Destruction
SkyManager::SkyManager()
{
	m_RenderSky = true;

	// TODO: add a way to set the initial skyset before progressive load
	m_SkySet = L"default";

	m_HorizonHeight = -150.0f;

	for (size_t i = 0; i < ARRAY_SIZE(m_SkyTexture); i++)
		m_SkyTexture[i] = 0;

	cur_loading_tex = 0;
}

SkyManager::~SkyManager()
{
	// Cleanup if the caller messed up
	UnloadSkyTextures();
}


///////////////////////////////////////////////////////////////////
// Progressive load of sky textures
int SkyManager::LoadSkyTextures()
{
	const size_t num_textures = ARRAY_SIZE(m_SkyTexture);

	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = timer_Time() + 100e-3;

	while (cur_loading_tex < num_textures)
	{
		char filename[PATH_MAX];
		snprintf(filename, ARRAY_SIZE(filename), "art/textures/skies/%s/%s.dds", 
			CStr8(m_SkySet).c_str(), IMAGE_NAMES[cur_loading_tex]);
		Handle ht = ogl_tex_load(filename);
		if (ht <= 0)
		{
			LOG(CLogger::Error, LOG_CATEGORY, "SkyManager::LoadSkyTextures failed on \"%s\"", filename);
			return ht;
		}
		ogl_tex_set_wrap(ht, GL_CLAMP_TO_EDGE);
		m_SkyTexture[cur_loading_tex] = ht;
		RETURN_ERR(ogl_tex_upload(ht));

		cur_loading_tex++;
		LDR_CHECK_TIMEOUT(cur_loading_tex, num_textures);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////
// Unload sky textures
void SkyManager::UnloadSkyTextures()
{
	for(size_t i = 0; i < ARRAY_SIZE(m_SkyTexture); i++)
	{
		ogl_tex_free(m_SkyTexture[i]);
		m_SkyTexture[i] = 0;
	}
	cur_loading_tex = 0;
}


///////////////////////////////////////////////////////////////////
// Switch to a different sky set (while the game is running)
void SkyManager::SetSkySet( const CStrW& newSet )
{
	if( newSet != m_SkySet )
	{
		m_SkySet = newSet;

		UnloadSkyTextures();

		for( size_t i=0; i<ARRAY_SIZE(m_SkyTexture); i++ ) {
			char filename[PATH_MAX];
			snprintf(filename, ARRAY_SIZE(filename), "art/textures/skies/%s/%s.dds", 
				CStr8(m_SkySet).c_str(), IMAGE_NAMES[i]);
			Handle ht = ogl_tex_load(filename);
			if (ht <= 0)
			{
				LOG(CLogger::Error, LOG_CATEGORY, "SkyManager::SetSkySet failed on \"%s\"", filename);
				return;
			}
			ogl_tex_set_wrap(ht, GL_CLAMP_TO_EDGE);
			m_SkyTexture[i] = ht;
			ogl_tex_upload(ht);
		}
	}
}

///////////////////////////////////////////////////////////////////
// Generate list of available skies
std::vector<CStrW> SkyManager::GetSkySets() const
{
	std::vector<CStrW> skies;

	// Find all subdirectories in art/textures/skies

	const char* dirname = "art/textures/skies/";
	DirectoryNames subdirectories;
	if(g_VFS->GetDirectoryEntries(dirname, 0, &subdirectories) < 0)
	{
		LOG(CLogger::Error, "vfs", "Error opening directory '%s'", dirname);
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	for(size_t i = 0; i < subdirectories.size(); i++)
		skies.push_back(CStr(subdirectories[i]));

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
	ogl_tex_bind( m_SkyTexture[IMG_FRONT] );
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
	ogl_tex_bind( m_SkyTexture[IMG_BACK] );
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
	ogl_tex_bind( m_SkyTexture[IMG_RIGHT] );
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
	ogl_tex_bind( m_SkyTexture[IMG_LEFT] );
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
	ogl_tex_bind( m_SkyTexture[IMG_TOP] );
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
