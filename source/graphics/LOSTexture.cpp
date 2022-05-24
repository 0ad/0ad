/* Copyright (C) 2022 Wildfire Games.
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

#include "LOSTexture.h"

#include "graphics/ShaderManager.h"
#include "lib/bits.h"
#include "lib/config2.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/TimeManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/helpers/Los.h"

/*

The LOS bitmap is computed with one value per LOS vertex, based on
CCmpRangeManager's visibility information.

The bitmap is then blurred using an NxN filter (in particular a
7-tap Binomial filter as an efficient integral approximation of a Gaussian).
To implement the blur efficiently without using extra memory for a second copy
of the bitmap, we generate the bitmap with (N-1)/2 pixels of padding on each side,
then the blur shifts the image back into the corner.

The blurred bitmap is then uploaded into a GL texture for use by the renderer.

*/


// Blur with a NxN filter, where N = g_BlurSize must be an odd number.
// Keep it in relation to the number of impassable tiles in MAP_EDGE_TILES.
static const size_t g_BlurSize = 7;

// Alignment (in bytes) of the pixel data passed into texture uploading.
// This must be a multiple of GL_UNPACK_ALIGNMENT, which ought to be 1 (since
// that's what we set it to) but in some weird cases appears to have a different
// value. (See Trac #2594). Multiples of 4 are possibly good for performance anyway.
static const size_t g_SubTextureAlignment = 4;

CLOSTexture::CLOSTexture(CSimulation2& simulation)
	: m_Simulation(simulation)
{
	if (CRenderer::IsInitialised() && g_RenderingOptions.GetSmoothLOS())
		CreateShader();
}

CLOSTexture::~CLOSTexture()
{
	m_SmoothFramebuffers[0].reset();
	m_SmoothFramebuffers[1].reset();

	if (m_Texture)
		DeleteTexture();
}

// Create the LOS texture engine. Should be ran only once.
bool CLOSTexture::CreateShader()
{
	m_SmoothTech = g_Renderer.GetShaderManager().LoadEffect(str_los_interp);
	Renderer::Backend::IShaderProgram* shader = m_SmoothTech->GetShader();

	m_ShaderInitialized = m_SmoothTech && shader;

	if (!m_ShaderInitialized)
	{
		LOGERROR("Failed to load SmoothLOS shader, disabling.");
		g_RenderingOptions.SetSmoothLOS(false);
		return false;
	}

	return true;
}

void CLOSTexture::DeleteTexture()
{
	m_Texture.reset();
	m_SmoothTextures[0].reset();
	m_SmoothTextures[1].reset();
}

void CLOSTexture::MakeDirty()
{
	m_Dirty = true;
}

Renderer::Backend::ITexture* CLOSTexture::GetTextureSmooth()
{
	if (CRenderer::IsInitialised() && !g_RenderingOptions.GetSmoothLOS())
		return GetTexture();
	else
		return m_SmoothTextures[m_WhichTexture].get();
}

void CLOSTexture::InterpolateLOS(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	const bool skipSmoothLOS = CRenderer::IsInitialised() && !g_RenderingOptions.GetSmoothLOS();
	if (!skipSmoothLOS && !m_ShaderInitialized)
	{
		if (!CreateShader())
			return;

		// RecomputeTexture will not cause the ConstructTexture to run.
		// Force the textures to be created.
		DeleteTexture();
		ConstructTexture(deviceCommandContext);
		m_Dirty = true;
	}

	if (m_Dirty)
	{
		RecomputeTexture(deviceCommandContext);
		m_Dirty = false;
	}

	if (skipSmoothLOS)
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render LOS texture");
	deviceCommandContext->SetFramebuffer(m_SmoothFramebuffers[m_WhichTexture].get());

	deviceCommandContext->SetGraphicsPipelineState(
		m_SmoothTech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();

	Renderer::Backend::IShaderProgram* shader = m_SmoothTech->GetShader();

	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_losTex1), m_Texture.get());
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_losTex2), m_SmoothTextures[m_WhichTexture].get());

	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_delta),
		static_cast<float>(g_Renderer.GetTimeManager().GetFrameDelta() * 4.0f));

	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp =
	{
		0, 0,
		static_cast<int>(m_Texture->GetWidth()),
		static_cast<int>(m_Texture->GetHeight())
	};
	g_Renderer.SetViewport(vp);

	float quadVerts[] =
	{
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] =
	{
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV0,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 1);

	deviceCommandContext->SetVertexBufferData(
		0, quadVerts, std::size(quadVerts) * sizeof(quadVerts[0]));
	deviceCommandContext->SetVertexBufferData(
		1, quadTex, std::size(quadTex) * sizeof(quadTex[0]));

	deviceCommandContext->Draw(0, 6);

	g_Renderer.SetViewport(oldVp);

	deviceCommandContext->EndPass();

	deviceCommandContext->SetFramebuffer(
		deviceCommandContext->GetDevice()->GetCurrentBackbuffer());

	m_WhichTexture = 1u - m_WhichTexture;
}


Renderer::Backend::ITexture* CLOSTexture::GetTexture()
{
	ENSURE(!m_Dirty);
	return m_Texture.get();
}

const CMatrix3D& CLOSTexture::GetTextureMatrix()
{
	ENSURE(!m_Dirty);
	return m_TextureMatrix;
}

const CMatrix3D& CLOSTexture::GetMinimapTextureMatrix()
{
	ENSURE(!m_Dirty);
	return m_MinimapTextureMatrix;
}

void CLOSTexture::ConstructTexture(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;

	m_MapSize = cmpRangeManager->GetVerticesPerSide();

	const size_t textureSize = round_up_to_pow2(round_up((size_t)m_MapSize + g_BlurSize - 1, g_SubTextureAlignment));

	Renderer::Backend::IDevice* backendDevice = deviceCommandContext->GetDevice();

	const Renderer::Backend::Sampler::Desc defaultSamplerDesc =
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);

	m_Texture = backendDevice->CreateTexture2D("LOSTexture",
		Renderer::Backend::Format::A8_UNORM, textureSize, textureSize, defaultSamplerDesc);

	// Initialise texture with SoD color, for the areas we don't
	// overwrite with uploading later.
	std::unique_ptr<u8[]> texData = std::make_unique<u8[]>(textureSize * textureSize);
	memset(texData.get(), 0x00, textureSize * textureSize);

	if (CRenderer::IsInitialised() && g_RenderingOptions.GetSmoothLOS())
	{
		m_SmoothTextures[0] = backendDevice->CreateTexture2D("LOSSmoothTexture0",
			Renderer::Backend::Format::A8_UNORM, textureSize, textureSize, defaultSamplerDesc);
		m_SmoothTextures[1] = backendDevice->CreateTexture2D("LOSSmoothTexture1",
			Renderer::Backend::Format::A8_UNORM, textureSize, textureSize, defaultSamplerDesc);

		m_SmoothFramebuffers[0] = backendDevice->CreateFramebuffer("LOSSmoothFramebuffer0",
			m_SmoothTextures[0].get(), nullptr);
		m_SmoothFramebuffers[1] = backendDevice->CreateFramebuffer("LOSSmoothFramebuffer1",
			m_SmoothTextures[1].get(), nullptr);
		if (!m_SmoothFramebuffers[0] || !m_SmoothFramebuffers[1])
		{
			LOGERROR("Failed to create LOS framebuffers");
			g_RenderingOptions.SetSmoothLOS(false);
		}

		deviceCommandContext->UploadTexture(
			m_SmoothTextures[0].get(), Renderer::Backend::Format::A8_UNORM,
			texData.get(), textureSize * textureSize);
		deviceCommandContext->UploadTexture(
			m_SmoothTextures[1].get(), Renderer::Backend::Format::A8_UNORM,
			texData.get(), textureSize * textureSize);
	}

	deviceCommandContext->UploadTexture(
		m_Texture.get(), Renderer::Backend::Format::A8_UNORM,
		texData.get(), textureSize * textureSize);

	texData.reset();

	{
		// Texture matrix: We want to map
		//   world pos (0, y, 0)  (i.e. first vertex)
		//     onto texcoord (0.5/texsize, 0.5/texsize)  (i.e. middle of first texel);
		//   world pos ((mapsize-1)*cellsize, y, (mapsize-1)*cellsize)  (i.e. last vertex)
		//     onto texcoord ((mapsize-0.5) / texsize, (mapsize-0.5) / texsize)  (i.e. middle of last texel)

		float s = (m_MapSize-1) / static_cast<float>(textureSize * (m_MapSize-1) * LOS_TILE_SIZE);
		float t = 0.5f / textureSize;
		m_TextureMatrix.SetZero();
		m_TextureMatrix._11 = s;
		m_TextureMatrix._23 = s;
		m_TextureMatrix._14 = t;
		m_TextureMatrix._24 = t;
		m_TextureMatrix._44 = 1;
	}

	{
		// Minimap matrix: We want to map UV (0,0)-(1,1) onto (0,0)-(mapsize/texsize, mapsize/texsize)

		float s = m_MapSize / (float)textureSize;
		m_MinimapTextureMatrix.SetZero();
		m_MinimapTextureMatrix._11 = s;
		m_MinimapTextureMatrix._22 = s;
		m_MinimapTextureMatrix._44 = 1;
	}
}

void CLOSTexture::RecomputeTexture(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	// If the map was resized, delete and regenerate the texture
	if (m_Texture)
	{
		CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
		if (!cmpRangeManager || m_MapSize != cmpRangeManager->GetVerticesPerSide())
			DeleteTexture();
	}

	bool recreated = false;
	if (!m_Texture)
	{
		ConstructTexture(deviceCommandContext);
		recreated = true;
	}

	PROFILE("recompute LOS texture");

	size_t pitch;
	const size_t dataSize = GetBitmapSize(m_MapSize, m_MapSize, &pitch);
	ENSURE(pitch * m_MapSize <= dataSize);
	std::unique_ptr<u8[]> losData = std::make_unique<u8[]>(dataSize);

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;

	CLosQuerier los(cmpRangeManager->GetLosQuerier(g_Game->GetSimulation2()->GetSimContext().GetCurrentDisplayedPlayer()));

	GenerateBitmap(los, &losData[0], m_MapSize, m_MapSize, pitch);

	if (CRenderer::IsInitialised() && g_RenderingOptions.GetSmoothLOS() && recreated)
	{
		deviceCommandContext->UploadTextureRegion(
			m_SmoothTextures[0].get(), Renderer::Backend::Format::A8_UNORM, losData.get(),
			pitch * m_MapSize, 0, 0, pitch, m_MapSize);
		deviceCommandContext->UploadTextureRegion(
			m_SmoothTextures[1].get(), Renderer::Backend::Format::A8_UNORM, losData.get(),
			pitch * m_MapSize, 0, 0, pitch, m_MapSize);
	}

	deviceCommandContext->UploadTextureRegion(
		m_Texture.get(), Renderer::Backend::Format::A8_UNORM, losData.get(),
		pitch * m_MapSize, 0, 0, pitch, m_MapSize);
}

size_t CLOSTexture::GetBitmapSize(size_t w, size_t h, size_t* pitch)
{
	*pitch = round_up(w + g_BlurSize - 1, g_SubTextureAlignment);
	return *pitch * (h + g_BlurSize - 1);
}

void CLOSTexture::GenerateBitmap(const CLosQuerier& los, u8* losData, size_t w, size_t h, size_t pitch)
{
	u8 *dataPtr = losData;

	// Initialise the top padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < pitch; ++i)
			*dataPtr++ = 0;

	for (size_t j = 0; j < h; ++j)
	{
		// Initialise the left padding
		for (size_t i = 0; i < g_BlurSize/2; ++i)
			*dataPtr++ = 0;

		// Fill in the visibility data
		for (size_t i = 0; i < w; ++i)
		{
			if (los.IsVisible_UncheckedRange(i, j))
				*dataPtr++ = 255;
			else if (los.IsExplored_UncheckedRange(i, j))
				*dataPtr++ = 127;
			else
				*dataPtr++ = 0;
		}

		// Initialise the right padding
		for (size_t i = 0; i < pitch - w - g_BlurSize/2; ++i)
			*dataPtr++ = 0;
	}

	// Initialise the bottom padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < pitch; ++i)
			*dataPtr++ = 0;

	// Horizontal blur:

	for (size_t j = g_BlurSize/2; j < h + g_BlurSize/2; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			u8* d = &losData[i+j*pitch];
			*d = (
				1*d[0] +
				6*d[1] +
				15*d[2] +
				20*d[3] +
				15*d[4] +
				6*d[5] +
				1*d[6]
			) / 64;
		}
	}

	// Vertical blur:

	for (size_t j = 0; j < h; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			u8* d = &losData[i+j*pitch];
			*d = (
				1*d[0*pitch] +
				6*d[1*pitch] +
				15*d[2*pitch] +
				20*d[3*pitch] +
				15*d[4*pitch] +
				6*d[5*pitch] +
				1*d[6*pitch]
			) / 64;
		}
	}
}
