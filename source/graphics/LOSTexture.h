/* Copyright (C) 2011 Wildfire Games.
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

#include "lib/ogl.h"

#include "maths/Matrix3D.h"
#include "simulation2/components/ICmpRangeManager.h"

#include "graphics/ShaderManager.h"


class CSimulation2;

/**
 * Maintains the LOS (fog-of-war / shroud-of-darkness) texture, used for
 * rendering and for the minimap.
 */
class CLOSTexture
{
	NONCOPYABLE(CLOSTexture);
	friend class TestLOSTexture;

public:
	CLOSTexture(CSimulation2& simulation);
	~CLOSTexture();

	/**
	 * Marks the LOS texture as needing recomputation. Call this after each
	 * simulation update, to ensure responsive updates.
	 */
	void MakeDirty();

	/**
	 * Recomputes the LOS texture if necessary, and binds it to the requested
	 * texture unit.
	 * Also switches the current active texture unit, and enables texturing on it.
	 * The texture is in 8-bit ALPHA format.
	 */
	void BindTexture(int unit);

	/**
	 * Recomputes the LOS texture if necessary, and returns the texture handle.
	 * Also potentially switches the current active texture unit, and enables texturing on it.
	 * The texture is in 8-bit ALPHA format.
	 */
	GLuint GetTexture();
	
	void InterpolateLOS();
	GLuint GetTextureSmooth();

	/**
	 * Returns a matrix to map (x,y,z) world coordinates onto (u,v) LOS texture
	 * coordinates, in the form expected by glLoadMatrixf.
	 * This must only be called after BindTexture.
	 */
	const CMatrix3D& GetTextureMatrix();

	/**
	 * Returns a matrix to map (0,0)-(1,1) texture coordinates onto LOS texture
	 * coordinates, in the form expected by glLoadMatrixf.
	 * This must only be called after BindTexture.
	 */
	const float* GetMinimapTextureMatrix();

private:
	void DeleteTexture();
	void ConstructTexture(int unit);
	void RecomputeTexture(int unit);

	size_t GetBitmapSize(size_t w, size_t h, size_t* pitch);
	void GenerateBitmap(ICmpRangeManager::CLosQuerier los, u8* losData, size_t w, size_t h, size_t pitch);

	CSimulation2& m_Simulation;

	bool m_Dirty;

	GLuint m_Texture;
	GLuint m_TextureSmooth1, m_TextureSmooth2;
	
	bool whichTex;
	
	GLuint m_smoothFbo;
	CShaderTechniquePtr m_smoothShader;	

	ssize_t m_MapSize; // vertexes per side
	GLsizei m_TextureSize; // texels per side

	CMatrix3D m_TextureMatrix;
	CMatrix3D m_MinimapTextureMatrix;
};
