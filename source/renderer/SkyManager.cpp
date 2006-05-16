/**
 * =========================================================================
 * File        : SkyManager.cpp
 * Project     : Pyrogenesis
 * Description : Sky settings, texture management and rendering.
 *
 * @author Matei Zaharia <matei@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "lib/timer.h"
#include "lib/res/file/vfs.h"
#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "ps/CLogger.h"
#include "ps/Loader.h"

#include "renderer/SkyManager.h"
#include "renderer/Renderer.h"

#include "graphics/Camera.h"

#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// SkyManager implementation


///////////////////////////////////////////////////////////////////
// Construction/Destruction
SkyManager::SkyManager()
{
	// water
	m_RenderSky = true;

	for (uint i = 0; i < ARRAY_SIZE(m_SkyTexture); i++)
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
	// Note: this must be kept in sync with the IMG_ constants in SkyManager.h
	static const char* IMAGE_NAMES[6] = {
		"front",
		"back",
		"right",
		"left",
		"top",
		"bottom"
	};

	const uint num_textures = ARRAY_SIZE(m_SkyTexture);

	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 100e-3;

	while (cur_loading_tex < num_textures)
	{
		char filename[PATH_MAX];
		// TODO: add a member variable and setter for this. (can't make this
		// a parameter because this function is called via delay-load code)
		static const char* const set_name = "day1";
		snprintf(filename, ARRAY_SIZE(filename), "art/textures/skies/%s/%s.dds", set_name, IMAGE_NAMES[cur_loading_tex]);
		Handle ht = ogl_tex_load(filename);
		if (ht <= 0)
		{
			LOG(ERROR, LOG_CATEGORY, "SkyManager::LoadSkyTextures failed on \"%s\"", filename);
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
	for(uint i = 0; i < ARRAY_SIZE(m_SkyTexture); i++)
	{
		ogl_tex_free(m_SkyTexture[i]);
		m_SkyTexture[i] = 0;
	}
	cur_loading_tex = 0; // so they will be reloaded if LoadWaterTextures is called again
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
	
	// Translate so we are at the camera in the X and Z directions,
	// but put the horizon at Y=0 so it looks right when the camera is higher
	const CCamera& camera = g_Renderer.GetViewCamera();
	CVector3D pos = camera.m_Orientation.GetTranslation();
	glTranslatef( pos.X, 0.0f, pos.Z );

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

	// Bottom face (negative Y)
	// Note: These texcoords not be completely correct (haven't had a good texture to test with)
	ogl_tex_bind( m_SkyTexture[IMG_BOTTOM] );
	glBegin( GL_QUADS );
		glTexCoord2f( 1, 0 );
		glVertex3f( +D, -D, -D );
		glTexCoord2f( 1, 1 );
		glVertex3f( +D, -D, +D );
		glTexCoord2f( 0, 1 );
		glVertex3f( -D, -D, +D );
		glTexCoord2f( 0, 0 );
		glVertex3f( -D, -D, -D );
	glEnd();

	glPopMatrix();

	glDepthMask( GL_TRUE );
}
