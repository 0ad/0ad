/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_LOSTEXTURE
#define INCLUDED_LOSTEXTURE

#include "graphics/ShaderTechniquePtr.h"
#include "maths/Matrix3D.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/backend/IFramebuffer.h"
#include "renderer/backend/IShaderProgram.h"
#include "renderer/backend/ITexture.h"

#include <memory>

class CLosQuerier;
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
	 * Recomputes the LOS texture if necessary, and returns the texture handle.
	 * Also potentially switches the current active texture unit, and enables texturing on it.
	 * The texture is in 8-bit ALPHA format.
	 */
	Renderer::Backend::ITexture* GetTexture();
	Renderer::Backend::ITexture* GetTextureSmooth();

	void InterpolateLOS(Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	/**
	 * Returns a matrix to map (x,y,z) world coordinates onto (u,v) LOS texture
	 * coordinates, in the form expected by a matrix uniform.
	 * This must only be called after InterpolateLOS.
	 */
	const CMatrix3D& GetTextureMatrix();

	/**
	 * Returns a matrix to map (0,0)-(1,1) texture coordinates onto LOS texture
	 * coordinates, in the form expected by a matrix uniform.
	 * This must only be called after InterpolateLOS.
	 */
	const CMatrix3D& GetMinimapTextureMatrix();

private:
	void DeleteTexture();
	bool CreateShader();
	void ConstructTexture(Renderer::Backend::IDeviceCommandContext* deviceCommandContext);
	void RecomputeTexture(Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	size_t GetBitmapSize(size_t w, size_t h, size_t* pitch);
	void GenerateBitmap(const CLosQuerier& los, u8* losData, size_t w, size_t h, size_t pitch);

	CSimulation2& m_Simulation;

	bool m_Dirty = true;

	bool m_ShaderInitialized = false;

	// We need to choose the smallest format. We always use the red channel but
	// R8_UNORM might be unavailable on some platforms. So we fallback to
	// R8G8B8A8_UNORM.
	Renderer::Backend::Format m_TextureFormat =
		Renderer::Backend::Format::UNDEFINED;
	size_t m_TextureFormatStride = 0;
	std::unique_ptr<Renderer::Backend::ITexture>
		m_Texture, m_SmoothTextures[2];
	Renderer::Backend::IVertexInputLayout* m_VertexInputLayout = nullptr;

	uint32_t m_WhichTexture = 0;
	double m_LastTextureRecomputeTime = 0.0;

	// We update textures once a frame, so we change a Framebuffer once a frame.
	// That allows us to use two ping-pong FBOs instead of checking completeness
	// of Framebuffer each frame.
	std::unique_ptr<Renderer::Backend::IFramebuffer>
		m_SmoothFramebuffers[2];
	CShaderTechniquePtr m_SmoothTech;

	size_t m_MapSize = 0; // vertexes per side

	CMatrix3D m_TextureMatrix;
	CMatrix3D m_MinimapTextureMatrix;
};

#endif // INCLUDED_LOSTEXTURE
