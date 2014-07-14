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
#include "simulation2/helpers/Grid.h"

class CSimulation2;

/**
 * Maintains the territory boundary texture, used for
 * rendering and for the minimap.
 */
class CTerritoryTexture
{
	NONCOPYABLE(CTerritoryTexture);

public:
	CTerritoryTexture(CSimulation2& simulation);
	~CTerritoryTexture();

	/**
	 * Recomputes the territory texture if necessary, and binds it to the requested
	 * texture unit.
	 * Also switches the current active texture unit, and enables texturing on it.
	 * The texture is in 32-bit BGRA format.
	 */
	void BindTexture(int unit);

	/**
	 * Recomputes the territory texture if necessary, and returns the texture handle.
	 * Also potentially switches the current active texture unit, and enables texturing on it.
	 * The texture is in 32-bit BGRA format.
	 */
	GLuint GetTexture();

	/**
	 * Returns a matrix to map (x,y,z) world coordinates onto (u,v) texture
	 * coordinates, in the form expected by glLoadMatrixf.
	 * This must only be called after BindTexture.
	 */
	const float* GetTextureMatrix();

	/**
	 * Returns a matrix to map (0,0)-(1,1) texture coordinates onto texture
	 * coordinates, in the form expected by glLoadMatrixf.
	 * This must only be called after BindTexture.
	 */
	const CMatrix3D* GetMinimapTextureMatrix();

private:
	/**
	 * Returns true if the territory state has changed since the last call to this function
	 */
	bool UpdateDirty();

	void DeleteTexture();
	void ConstructTexture(int unit);
	void RecomputeTexture(int unit);

	void GenerateBitmap(const Grid<u8>& territories, u8* bitmap, ssize_t w, ssize_t h);

	CSimulation2& m_Simulation;

	size_t m_DirtyID;

	GLuint m_Texture;

	ssize_t m_MapSize; // tiles per side
	GLsizei m_TextureSize; // texels per side

	CMatrix3D m_TextureMatrix;
	CMatrix3D m_MinimapTextureMatrix;
};
