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

#include "renderer/SkyManager.h"

#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "lib/bits.h"
#include "lib/tex/tex.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/RenderingOptions.h"

#include <algorithm>

SkyManager::SkyManager()
	: m_VertexArray(Renderer::Backend::GL::CBuffer::Type::VERTEX, false)
{
	CFG_GET_VAL("showsky", m_RenderSky);
}

void SkyManager::LoadAndUploadSkyTexturesIfNeeded(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (m_SkyCubeMap)
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Load Sky Textures");
	static const CStrW images[NUMBER_OF_TEXTURES + 1] = {
		L"front",
		L"back",
		L"top",
		L"top",
		L"right",
		L"left"
	};

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

	Tex textures[NUMBER_OF_TEXTURES + 1];

	for (size_t i = 0; i < NUMBER_OF_TEXTURES + 1; ++i)
	{
		VfsPath path = VfsPath("art/textures/skies") / m_SkySet / (Path::String(images[i]) + L".dds");

		std::shared_ptr<u8> file;
		size_t fileSize;
		if (g_VFS->LoadFile(path, file, fileSize) != INFO::OK)
		{
			path = VfsPath("art/textures/skies") / m_SkySet / (Path::String(images[i]) + L".dds.cached.dds");
			if (g_VFS->LoadFile(path, file, fileSize) != INFO::OK)
			{
				LOGERROR("Error creating sky cubemap '%s', can't load file: '%s'.", m_SkySet.ToUTF8().c_str(), path.string8().c_str());
				return;
			}
		}

		textures[i].decode(file, fileSize);
		textures[i].transform_to((textures[i].m_Flags | TEX_BOTTOM_UP | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS));

		if (!is_pow2(textures[i].m_Width) || !is_pow2(textures[i].m_Height))
		{
			LOGERROR("Error creating sky cubemap '%s', cube textures should have power of 2 sizes.", m_SkySet.ToUTF8().c_str());
			return;
		}

		if (textures[i].m_Width != textures[0].m_Width || textures[i].m_Height != textures[0].m_Height)
		{
			LOGERROR("Error creating sky cubemap '%s', cube textures have different sizes.", m_SkySet.ToUTF8().c_str());
			return;
		}
	}

	m_SkyCubeMap = g_VideoMode.GetBackendDevice()->CreateTexture("SkyCubeMap",
		Renderer::Backend::GL::CTexture::Type::TEXTURE_CUBE,
		Renderer::Backend::Format::R8G8B8A8, textures[0].m_Width, textures[0].m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, 1);

	std::vector<u8> rotated;
	for (size_t i = 0; i < NUMBER_OF_TEXTURES + 1; ++i)
	{
		u8* data = textures[i].get_data();

		// We need to rotate the side if it's looking up or down.
		// TODO: maybe it should be done during texture conversion.
		if (i == 2 || i == 3)
		{
			rotated.resize(textures[i].m_DataSize);

			for (size_t y = 0; y < textures[i].m_Height; ++y)
			{
				for (size_t x = 0; x < textures[i].m_Width; ++x)
				{
					const size_t invX = y;
					const size_t invY = textures[i].m_Width - x - 1;

					rotated[(y * textures[i].m_Width + x) * 4 + 0] = data[(invY * textures[i].m_Width + invX) * 4 + 0];
					rotated[(y * textures[i].m_Width + x) * 4 + 1] = data[(invY * textures[i].m_Width + invX) * 4 + 1];
					rotated[(y * textures[i].m_Width + x) * 4 + 2] = data[(invY * textures[i].m_Width + invX) * 4 + 2];
					rotated[(y * textures[i].m_Width + x) * 4 + 3] = data[(invY * textures[i].m_Width + invX) * 4 + 3];
				}
			}

			deviceCommandContext->UploadTexture(
				m_SkyCubeMap.get(), Renderer::Backend::Format::R8G8B8A8,
				&rotated[0], textures[i].m_DataSize, 0, i);
		}
		else
		{
			deviceCommandContext->UploadTexture(
				m_SkyCubeMap.get(), Renderer::Backend::Format::R8G8B8A8,
				data, textures[i].m_DataSize, 0, i);
		}
	}
	///////////////////////////////////////////////////////////////////////////
}

void SkyManager::SetSkySet(const CStrW& newSet)
{
	if (newSet == m_SkySet)
		return;

	m_SkyCubeMap.reset();

	m_SkySet = newSet;
}

std::vector<CStrW> SkyManager::GetSkySets() const
{
	std::vector<CStrW> skies;

	// Find all subdirectories in art/textures/skies

	const VfsPath path(L"art/textures/skies/");
	DirectoryNames subdirectories;
	if (g_VFS->GetDirectoryEntries(path, 0, &subdirectories) != INFO::OK)
	{
		LOGERROR("Error opening directory '%s'", path.string8());
		return std::vector<CStrW>(1, GetSkySet()); // just return what we currently have
	}

	for(size_t i = 0; i < subdirectories.size(); i++)
		skies.push_back(subdirectories[i].string());
	sort(skies.begin(), skies.end());

	return skies;
}

void SkyManager::RenderSky(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	GPU_SCOPED_LABEL(deviceCommandContext, "Render sky");
#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
#warning TODO: implement SkyManager::RenderSky for GLES
#else
	if (!m_RenderSky)
		return;

	// Do nothing unless SetSkySet was called
	if (m_SkySet.empty() || !m_SkyCubeMap)
		return;

	if (m_VertexArray.GetNumberOfVertices() == 0)
		CreateSkyCube();

	const CCamera& camera = g_Renderer.GetSceneRenderer().GetViewCamera();

	CShaderTechniquePtr skytech =
		g_Renderer.GetShaderManager().LoadEffect(str_sky_simple);
	skytech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		skytech->GetGraphicsPipelineStateDesc());
	const CShaderProgramPtr& shader = skytech->GetShader();
	shader->BindTexture(str_baseTex, m_SkyCubeMap.get());

	// Translate so the sky center is at the camera space origin.
	CMatrix3D translate;
	translate.SetTranslation(camera.GetOrientation().GetTranslation());

	// Currently we have a hardcoded near plane in the projection matrix.
	CMatrix3D scale;
	scale.SetScaling(10.0f, 10.0f, 10.0f);

	// Rotate so that the "left" face, which contains the brightest part of
	// each skymap, is in the direction of the sun from our light
	// environment.
	CMatrix3D rotate;
	rotate.SetYRotation(M_PI + g_Renderer.GetSceneRenderer().GetLightEnv().GetRotation());

	shader->Uniform(
		str_transform,
		camera.GetViewProjection() * translate * rotate * scale);

	m_VertexArray.PrepareForRendering();

	u8* base = m_VertexArray.Bind(deviceCommandContext);
	const GLsizei stride = static_cast<GLsizei>(m_VertexArray.GetStride());

	shader->VertexPointer(
		3, GL_FLOAT, stride, base + m_AttributePosition.offset);
	shader->TexCoordPointer(
		GL_TEXTURE0, 3, GL_FLOAT, stride, base + m_AttributeUV.offset);
	shader->AssertPointersBound();

	deviceCommandContext->Draw(0, m_VertexArray.GetNumberOfVertices());

	skytech->EndPass();
#endif
}

void SkyManager::CreateSkyCube()
{
	m_AttributePosition.type = GL_FLOAT;
	m_AttributePosition.elems = 3;
	m_VertexArray.AddAttribute(&m_AttributePosition);

	m_AttributeUV.type = GL_FLOAT;
	m_AttributeUV.elems = 3;
	m_VertexArray.AddAttribute(&m_AttributeUV);

	// 6 sides of cube with 6 vertices.
	m_VertexArray.SetNumberOfVertices(6 * 6);
	m_VertexArray.Layout();

	VertexArrayIterator<CVector3D> attrPosition = m_AttributePosition.GetIterator<CVector3D>();
	VertexArrayIterator<CVector3D> attrUV = m_AttributeUV.GetIterator<CVector3D>();

#define ADD_VERTEX(U, V, W, VX, VY, VZ) \
	STMT( \
		attrPosition->X = VX; \
		attrPosition->Y = VY; \
		attrPosition->Z = VZ; \
		++attrPosition; \
		attrUV->X = U; \
		attrUV->Y = V; \
		attrUV->Z = W; \
		++attrUV;)

	// Axis -X
	ADD_VERTEX(+1, +1, +1, -1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, +1, -1, -1.0f, -1.0f, +1.0f);
	ADD_VERTEX(+1, -1, -1, -1.0f, +1.0f, +1.0f);
	ADD_VERTEX(+1, +1, +1, -1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, -1, -1, -1.0f, +1.0f, +1.0f);
	ADD_VERTEX(+1, -1, +1, -1.0f, +1.0f, -1.0f);

	// Axis +X
	ADD_VERTEX(-1, +1, -1, +1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, +1, +1, +1.0f, -1.0f, -1.0f);
	ADD_VERTEX(-1, -1, +1, +1.0f, +1.0f, -1.0f);
	ADD_VERTEX(-1, +1, -1, +1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, -1, +1, +1.0f, +1.0f, -1.0f);
	ADD_VERTEX(-1, -1, -1, +1.0f, +1.0f, +1.0f);

	// Axis -Y
	ADD_VERTEX(-1, +1, +1, +1.0f, -1.0f, -1.0f);
	ADD_VERTEX(-1, +1, -1, +1.0f, -1.0f, +1.0f);
	ADD_VERTEX(+1, +1, -1, -1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, +1, +1, +1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, +1, -1, -1.0f, -1.0f, +1.0f);
	ADD_VERTEX(+1, +1, +1, -1.0f, -1.0f, -1.0f);

	// Axis +Y
	ADD_VERTEX(+1, -1, +1, -1.0f, +1.0f, -1.0f);
	ADD_VERTEX(+1, -1, -1, -1.0f, +1.0f, +1.0f);
	ADD_VERTEX(-1, -1, -1, +1.0f, +1.0f, +1.0f);
	ADD_VERTEX(+1, -1, +1, -1.0f, +1.0f, -1.0f);
	ADD_VERTEX(-1, -1, -1, +1.0f, +1.0f, +1.0f);
	ADD_VERTEX(-1, -1, +1, +1.0f, +1.0f, -1.0f);

	// Axis -Z
	ADD_VERTEX(-1, +1, +1, +1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, +1, +1, -1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, -1, +1, -1.0f, +1.0f, -1.0f);
	ADD_VERTEX(-1, +1, +1, +1.0f, -1.0f, -1.0f);
	ADD_VERTEX(+1, -1, +1, -1.0f, +1.0f, -1.0f);
	ADD_VERTEX(-1, -1, +1, +1.0f, +1.0f, -1.0f);

	// Axis +Z
	ADD_VERTEX(+1, +1, -1, -1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, +1, -1, +1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, -1, -1, +1.0f, +1.0f, +1.0f);
	ADD_VERTEX(+1, +1, -1, -1.0f, -1.0f, +1.0f);
	ADD_VERTEX(-1, -1, -1, +1.0f, +1.0f, +1.0f);
	ADD_VERTEX(+1, -1, -1, -1.0f, +1.0f, +1.0f);
#undef ADD_VERTEX

	m_VertexArray.Upload();
	m_VertexArray.FreeBackingStore();
}
