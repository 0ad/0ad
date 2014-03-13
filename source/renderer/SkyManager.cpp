/* Copyright (C) 2013 Wildfire Games.
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
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/Filesystem.h"
#include "ps/World.h"

#include "renderer/SkyManager.h"
#include "renderer/Renderer.h"

#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
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
	
	m_SkyCubeMap = 0;
}


///////////////////////////////////////////////////////////////////
// Load all sky textures
void SkyManager::LoadSkyTextures()
{
	/*for (size_t i = 0; i < ARRAY_SIZE(m_SkyTexture); ++i)
	{
		VfsPath path = VfsPath("art/textures/skies") / m_SkySet / (Path::String(s_imageNames[i])+L".dds");

		CTextureProperties textureProps(path);
		textureProps.SetWrap(GL_CLAMP_TO_EDGE);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_SkyTexture[i] = texture;
	}*/

	///////////////////////////////////////////////////////////////////////////
	// HACK: THE HORRIBLENESS HERE IS OVER 9000. The following code is a HUGE hack and will be removed completely
	// as soon as all the hardcoded GL_TEXTURE_2D references are corrected in the TextureManager/OGL/tex libs.
	
	glGenTextures(1, &m_SkyCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyCubeMap);
	
	int types[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	};
	
	const wchar_t* images[numTextures+1] = {
		L"front",
		L"back",
		L"right",
		L"left",
		L"top",
		L"top"
	};
	
	for (size_t i = 0; i < numTextures+1; ++i)
	{
		VfsPath path = VfsPath("art/textures/skies") / m_SkySet / (Path::String(images[i])+L".dds");
		
		shared_ptr<u8> file;
		size_t fileSize;
		if (g_VFS->LoadFile(path, file, fileSize) < 0)
		{
			VfsPath path2 = VfsPath("art/textures/skies") / m_SkySet / (Path::String(images[i])+L".dds.cached.dds");
			if (g_VFS->LoadFile(path2, file, fileSize) < 0)
			{
				glDeleteTextures(1, &m_SkyCubeMap);
				LOGERROR(L"Error creating sky cubemap.");
				return;
			}
		}
		
		Tex tex;
		tex.decode(file, fileSize);
		
		tex.transform_to((tex.m_Flags | TEX_BOTTOM_UP | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS));
		
		u8* data = tex.get_data();
		
		if (types[i] == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y || types[i] == GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
		{
			std::vector<u8> rotated(tex.m_DataSize);
		
			for (size_t y = 0; y < tex.m_Height; ++y)
			{
				for (size_t x = 0; x < tex.m_Width; ++x)
				{
					size_t invx = y, invy = tex.m_Width-x-1;
					
					rotated[(y*tex.m_Width + x) * 4 + 0] = data[(invy*tex.m_Width + invx) * 4 + 0];
					rotated[(y*tex.m_Width + x) * 4 + 1] = data[(invy*tex.m_Width + invx) * 4 + 1];
					rotated[(y*tex.m_Width + x) * 4 + 2] = data[(invy*tex.m_Width + invx) * 4 + 2];
					rotated[(y*tex.m_Width + x) * 4 + 3] = data[(invy*tex.m_Width + invx) * 4 + 3];
				}
			}
			
			glTexImage2D(types[i], 0, GL_RGB, tex.m_Width, tex.m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rotated[0]);
		}
		else
		{
			glTexImage2D(types[i], 0, GL_RGB, tex.m_Width, tex.m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if CONFIG2_GLES
#warning TODO: fix SkyManager::LoadSkyTextures for GLES
#else
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
	///////////////////////////////////////////////////////////////////////////
}


///////////////////////////////////////////////////////////////////
// Switch to a different sky set (while the game is running)
void SkyManager::SetSkySet( const CStrW& newSet )
{
	if(newSet == m_SkySet)
		return;
	
	if (m_SkyCubeMap)
	{
		glDeleteTextures(1, &m_SkyCubeMap);
		m_SkyCubeMap = 0;
	}
	
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
		LOGERROR(L"Error opening directory '%ls'", path.string().c_str());
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	for(size_t i = 0; i < subdirectories.size(); i++)
		skies.push_back(subdirectories[i].string());
	sort(skies.begin(), skies.end());

	return skies;
}

///////////////////////////////////////////////////////////////////
// Render sky
void SkyManager::RenderSky()
{
#if CONFIG2_GLES
#warning TODO: implement SkyManager::RenderSky for GLES
#else

	// Draw the sky as a small box around the map, with depth write enabled.
	// This will be done before anything else is drawn so we'll be overlapped by everything else.
	
	// Do nothing unless SetSkySet was called
	if (m_SkySet.empty())
		return;

	glDepthMask( GL_FALSE );
	
	pglActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	// Translate so we are at the center of the map.
	ssize_t mapSize = g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	glTranslatef(mapSize*(TERRAIN_TILE_SIZE/2.0f), m_HorizonHeight, mapSize*(TERRAIN_TILE_SIZE/2.0f) );

	// Rotate so that the "left" face, which contains the brightest part of each
	// skymap, is in the direction of the sun from our light environment
	glRotatef( 180.0f /*+ 45.0f*/ + RADTODEG(g_Renderer.GetLightEnv().GetRotation()), 0.0f, 1.0f, 0.0f );

	// Distance to draw the faces at
	const float D = 1500.0f; // distance from map center
	const float H = 500.0f; // height of the ceiling
	
	CShaderProgramPtr shader;
	CShaderTechniquePtr skytech;
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		skytech = g_Renderer.GetShaderManager().LoadEffect(str_sky_simple);
		skytech->BeginPass();
		shader = skytech->GetShader();
		shader->BindTexture(str_baseTex, m_SkyCubeMap);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyCubeMap);
	}

	glBegin(GL_QUADS);

	// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
		glTexCoord3f( +1, +1, +1 );  glVertex3f( -D, -H, -D );
		glTexCoord3f( +1, +1, -1 );  glVertex3f( -D, -H, +D );
		glTexCoord3f( +1, -1, -1 );  glVertex3f( -D, +H, +D );
		glTexCoord3f( +1, -1, +1 );  glVertex3f( -D, +H, -D );

	// GL_TEXTURE_CUBE_MAP_POSITIVE_X
		glTexCoord3f( -1, +1, -1 );  glVertex3f( +D, -H, +D );
		glTexCoord3f( -1, +1, +1 );  glVertex3f( +D, -H, -D );
		glTexCoord3f( -1, -1, +1 );  glVertex3f( +D, +H, -D );
		glTexCoord3f( -1, -1, -1 );  glVertex3f( +D, +H, +D );
	
	// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
		glTexCoord3f( -1, +1, +1 );  glVertex3f( +D, -H, -D );
		glTexCoord3f( -1, +1, -1 );  glVertex3f( +D, -H, +D );
		glTexCoord3f( +1, +1, -1 );  glVertex3f( -D, -H, +D );
		glTexCoord3f( +1, +1, +1 );  glVertex3f( -D, -H, -D );
		
	// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
		glTexCoord3f( +1, -1, +1 );  glVertex3f( -D, +H, -D );
		glTexCoord3f( +1, -1, -1 );  glVertex3f( -D, +H, +D );
		glTexCoord3f( -1, -1, -1 );  glVertex3f( +D, +H, +D );
		glTexCoord3f( -1, -1, +1 );  glVertex3f( +D, +H, -D );
	
	// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		glTexCoord3f( -1, +1, +1 );  glVertex3f( +D, -H, -D );
		glTexCoord3f( +1, +1, +1 );  glVertex3f( -D, -H, -D );
		glTexCoord3f( +1, -1, +1 );  glVertex3f( -D, +H, -D );
		glTexCoord3f( -1, -1, +1 );  glVertex3f( +D, +H, -D );

	// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
		glTexCoord3f( +1, +1, -1 );  glVertex3f( -D, -H, +D );
		glTexCoord3f( -1, +1, -1 );  glVertex3f( +D, -H, +D );
		glTexCoord3f( -1, -1, -1 );  glVertex3f( +D, +H, +D );
		glTexCoord3f( +1, -1, -1 );  glVertex3f( -D, +H, +D );
	glEnd();

	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		skytech->EndPass();
	}
	else
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glDisable(GL_TEXTURE_CUBE_MAP);
		glEnable(GL_TEXTURE_2D);
	}
	
	glPopMatrix();

	glDepthMask( GL_TRUE );

#endif
}
