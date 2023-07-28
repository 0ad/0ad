/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_POSTPROCMANAGER
#define INCLUDED_POSTPROCMANAGER

#include "graphics/ShaderTechniquePtr.h"
#include "ps/CStr.h"
#include "renderer/backend/IFramebuffer.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/backend/IShaderProgram.h"
#include "renderer/backend/ITexture.h"

#include <array>
#include <vector>

class CPostprocManager
{
public:
	CPostprocManager(Renderer::Backend::IDevice* device);
	~CPostprocManager();

	// Returns true if the the manager can be used.
	bool IsEnabled() const;

	// Create all buffers/textures in GPU memory and set default effect.
	// @note Must be called before using in the renderer. May be called multiple times.
	void Initialize();

	// Update the size of the screen
	void Resize();

	// Returns a list of xml files found in shaders/effects/postproc.
	static std::vector<CStrW> GetPostEffects();

	// Returns the name of the current effect.
	const CStrW& GetPostEffect() const
	{
		return m_PostProcEffect;
	}

	// Sets the current effect.
	void SetPostEffect(const CStrW& name);

	// Triggers update of shaders and FBO if needed.
	void UpdateAntiAliasingTechnique();
	void UpdateSharpeningTechnique();
	void UpdateSharpnessFactor();

	void SetDepthBufferClipPlanes(float nearPlane, float farPlane);

	// @note CPostprocManager must be initialized first
	Renderer::Backend::IFramebuffer* PrepareAndGetOutputFramebuffer();

	// First renders blur textures, then calls ApplyEffect for each effect pass,
	// ping-ponging the buffers at each step.
	// @note CPostprocManager must be initialized first
	void ApplyPostproc(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	// Blits the final postprocessed texture to the system framebuffer. The system
	// framebuffer is selected as the output buffer. Should be called before
	// silhouette rendering.
	// @note CPostprocManager must be initialized first
	void BlitOutputFramebuffer(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		Renderer::Backend::IFramebuffer* destination);

	// Returns true if we render main scene in the MSAA framebuffer.
	bool IsMultisampleEnabled() const;

	// Resolves the MSAA framebuffer into the regular one.
	void ResolveMultisampleFramebuffer(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

private:
	void CreateMultisampleBuffer();
	void DestroyMultisampleBuffer();

	Renderer::Backend::IDevice* m_Device = nullptr;

	std::unique_ptr<Renderer::Backend::IFramebuffer> m_CaptureFramebuffer;

	// Two framebuffers, that we flip between at each shader pass.
	std::unique_ptr<Renderer::Backend::IFramebuffer>
		m_PingFramebuffer, m_PongFramebuffer;

	// Unique color textures for the framebuffers.
	std::unique_ptr<Renderer::Backend::ITexture> m_ColorTex1, m_ColorTex2;

	// The framebuffers share a depth/stencil texture.
	std::unique_ptr<Renderer::Backend::ITexture> m_DepthTex;
	float m_NearPlane, m_FarPlane;

	// A framebuffer and textures x2 for each blur level we render.
	struct BlurScale
	{
		struct Step
		{
			std::unique_ptr<Renderer::Backend::IFramebuffer> framebuffer;
			std::unique_ptr<Renderer::Backend::ITexture> texture;
		};
		std::array<Step, 2> steps;
	};
	std::array<BlurScale, 3> m_BlurScales;

	// Indicates which of the ping-pong buffers is used for reading and which for drawing.
	bool m_WhichBuffer;

	Renderer::Backend::IVertexInputLayout* m_VertexInputLayout = nullptr;

	// The name and shader technique we are using. "default" name means no technique is used
	// (i.e. while we do allocate the buffers, no effects are rendered).
	CStrW m_PostProcEffect;
	CShaderTechniquePtr m_PostProcTech;

	CStr m_SharpName;
	CShaderTechniquePtr m_SharpTech;
	float m_Sharpness;

	CStr m_AAName;
	CShaderTechniquePtr m_AATech;
	bool m_UsingMultisampleBuffer;
	std::unique_ptr<Renderer::Backend::IFramebuffer> m_MultisampleFramebuffer;
	std::unique_ptr<Renderer::Backend::ITexture>
		m_MultisampleColorTex, m_MultisampleDepthTex;
	uint32_t m_MultisampleCount;
	std::vector<uint32_t> m_AllowedSampleCounts;

	// The current screen dimensions in pixels.
	int m_Width, m_Height;

	// Is the postproc manager initialized? Buffers created? Default effect loaded?
	bool m_IsInitialized;

	// Creates blur textures at various scales, for bloom, DOF, etc.
	void ApplyBlur(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	// High quality GPU image scaling to half size. outTex must have exactly half the size
	// of inTex. inWidth and inHeight are the dimensions of inTex in texels.
	void ApplyBlurDownscale2x(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		Renderer::Backend::IFramebuffer* framebuffer,
		Renderer::Backend::ITexture* inTex,
		int inWidth, int inHeight);

	// GPU-based Gaussian blur in two passes. inOutTex contains the input image and will be filled
	// with the blurred image. tempTex must have the same size as inOutTex.
	// inWidth and inHeight are the dimensions of the images in texels.
	void ApplyBlurGauss(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		Renderer::Backend::ITexture* inTex,
		Renderer::Backend::ITexture* tempTex,
		Renderer::Backend::IFramebuffer* tempFramebuffer,
		Renderer::Backend::IFramebuffer* outFramebuffer,
		int inWidth, int inHeight);

	// Applies a pass of a given effect to the entire current framebuffer. The shader is
	// provided with a number of general-purpose variables, including the rendered screen so far,
	// the depth buffer, a number of blur textures, the screen size, the zNear/zFar planes and
	// some other parameters used by the optional bloom/HDR pass.
	void ApplyEffect(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const CShaderTechniquePtr& shaderTech, int pass);

	// Delete all allocated buffers/textures from GPU memory.
	void Cleanup();

	// Delete existing buffers/textures and create them again, using a new screen size if needed.
	// (the textures are also attached to the framebuffers)
	void RecreateBuffers();
};

#endif // INCLUDED_POSTPROCMANAGER
