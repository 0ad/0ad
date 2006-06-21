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

#include "maths/MathUtil.h"

#include "ps/CLogger.h"
#include "ps/Loader.h"
#include "ps/VFSUtil.h"

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
	const uint num_textures = ARRAY_SIZE(m_SkyTexture);

	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 100e-3;

	while (cur_loading_tex < num_textures)
	{
		char filename[PATH_MAX];
		snprintf(filename, ARRAY_SIZE(filename), "art/textures/skies/%s/%s.dds", 
			CStr8(m_SkySet).c_str(), IMAGE_NAMES[cur_loading_tex]);
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
	cur_loading_tex = 0;
}


///////////////////////////////////////////////////////////////////
// Switch to a different sky set (while the game is running)
void SkyManager::SetSkySet( CStrW newSet )
{
	if( newSet != m_SkySet )
	{
		m_SkySet = newSet;

		UnloadSkyTextures();

		for( int i=0; i<ARRAY_SIZE(m_SkyTexture); i++ ) {
			char filename[PATH_MAX];
			snprintf(filename, ARRAY_SIZE(filename), "art/textures/skies/%s/%s.dds", 
				CStr8(m_SkySet).c_str(), IMAGE_NAMES[i]);
			Handle ht = ogl_tex_load(filename);
			if (ht <= 0)
			{
				LOG(ERROR, LOG_CATEGORY, "SkyManager::SetSkySet failed on \"%s\"", filename);
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
	Handle dir = vfs_dir_open(dirname);
	if (dir <= 0)
	{
		LOG(ERROR, "vfs", "Error opening directory '%s' (%lld)", dirname, dir);
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	const char* filter = "/";
	int err;
	DirEnt entry;
	while ((err = vfs_dir_next_ent(dir, &entry, filter)) == 0)
	{
		skies.push_back(CStr(entry.name));
	}

	if (err != ERR_DIR_END)
	{
		LOG(ERROR, "vfs", "Error reading files from directory '%s' (%d)", dirname, err);
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	vfs_dir_close(dir);

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
	glRotatef( 90.0f + g_Renderer.GetLightEnv().GetRotation()*180.0f/M_PI, 0.0f, 1.0f, 0.0f );

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
