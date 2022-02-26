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

#include "ShadowMap.h"

#include "graphics/Camera.h"
#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"
#include "gui/GUIMatrix.h"
#include "lib/bits.h"
#include "lib/ogl.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/Brush.h"
#include "maths/Frustum.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Profile.h"
#include "ps/VideoMode.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/Texture.h"
#include "renderer/DebugRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/SceneRenderer.h"

#include <array>

namespace
{

constexpr int MAX_CASCADE_COUNT = 4;

constexpr float DEFAULT_SHADOWS_CUTOFF_DISTANCE = 300.0f;
constexpr float DEFAULT_CASCADE_DISTANCE_RATIO = 1.7f;

} // anonymous namespace

/**
 * Struct ShadowMapInternals: Internal data for the ShadowMap implementation
 */
struct ShadowMapInternals
{
	std::unique_ptr<Renderer::Backend::GL::CFramebuffer> Framebuffer;
	std::unique_ptr<Renderer::Backend::GL::CTexture> Texture;

	// bit depth for the depth texture
	int DepthTextureBits;
	// width, height of shadow map
	int Width, Height;
	// Shadow map quality (-1 - Low, 0 - Medium, 1 - High, 2 - Very High)
	int QualityLevel;
	// used width, height of shadow map
	int EffectiveWidth, EffectiveHeight;

	// Transform world space into light space; calculated on SetupFrame
	CMatrix3D LightTransform;

	// transform light space into world space
	CMatrix3D InvLightTransform;
	CBoundingBoxAligned ShadowReceiverBound;

	int CascadeCount;
	float CascadeDistanceRatio;
	float ShadowsCutoffDistance;
	bool ShadowsCoverMap;

	struct Cascade
	{
		// transform light space into projected light space
		// in projected light space, the shadowbound box occupies the [-1..1] cube
		// calculated on BeginRender, after the final shadow bounds are known
		CMatrix3D LightProjection;
		float Distance;
		CBoundingBoxAligned FrustumBBAA;
		CBoundingBoxAligned ConvexBounds;
		CBoundingBoxAligned ShadowRenderBound;
		// Bounding box of shadowed objects in the light space.
		CBoundingBoxAligned ShadowCasterBound;
		// Transform world space into texture space of the shadow map;
		// calculated on BeginRender, after the final shadow bounds are known
		CMatrix3D TextureMatrix;
		// View port of the shadow texture where the cascade should be rendered.
		SViewPort ViewPort;
	};
	std::array<Cascade, MAX_CASCADE_COUNT> Cascades;

	// Camera transformed into light space
	CCamera LightspaceCamera;

	// Some drivers (at least some Intel Mesa ones) appear to handle alpha testing
	// incorrectly when the FBO has only a depth attachment.
	// When m_ShadowAlphaFix is true, we use DummyTexture to store a useless
	// alpha texture which is attached to the FBO as a workaround.
	std::unique_ptr<Renderer::Backend::GL::CTexture> DummyTexture;

	// Copy of renderer's standard view camera, saved between
	// BeginRender and EndRender while we replace it with the shadow camera
	CCamera SavedViewCamera;

	void CalculateShadowMatrices(const int cascade);
	void CreateTexture();
	void UpdateCascadesParameters();
};

void ShadowMapInternals::UpdateCascadesParameters()
{
	CascadeCount = 1;
	CFG_GET_VAL("shadowscascadecount", CascadeCount);

	if (CascadeCount < 1 || CascadeCount > MAX_CASCADE_COUNT || g_VideoMode.GetBackend() == CVideoMode::Backend::GL_ARB)
		CascadeCount = 1;

	ShadowsCoverMap = false;
	CFG_GET_VAL("shadowscovermap", ShadowsCoverMap);
}

void CalculateBoundsForCascade(
	const CCamera& camera, const CMatrix3D& lightTransform,
	const float nearPlane, const float farPlane, CBoundingBoxAligned* bbaa,
	CBoundingBoxAligned* frustumBBAA)
{
	frustumBBAA->SetEmpty();

	// We need to calculate a circumscribed sphere for the camera to
	// create a rotation stable bounding box.
	const CVector3D cameraIn = camera.m_Orientation.GetIn();
	const CVector3D cameraTranslation = camera.m_Orientation.GetTranslation();
	const CVector3D centerNear = cameraTranslation + cameraIn * nearPlane;
	const CVector3D centerDist = cameraTranslation + cameraIn * farPlane;

	// We can solve 3D problem in 2D space, because the frustum is
	// symmetric by 2 planes. Than means we can use only one corner
	// to find a circumscribed sphere.
	CCamera::Quad corners;

	camera.GetViewQuad(nearPlane, corners);
	for (CVector3D& corner : corners)
		corner = camera.GetOrientation().Transform(corner);
	const CVector3D cornerNear = corners[0];
	for (const CVector3D& corner : corners)
		*frustumBBAA += lightTransform.Transform(corner);

	camera.GetViewQuad(farPlane, corners);
	for (CVector3D& corner : corners)
		corner = camera.GetOrientation().Transform(corner);
	const CVector3D cornerDist = corners[0];
	for (const CVector3D& corner : corners)
		*frustumBBAA += lightTransform.Transform(corner);

	// We solve 2D case for the right trapezoid.
	const float firstBase = (cornerNear - centerNear).Length();
	const float secondBase = (cornerDist - centerDist).Length();
	const float height = (centerDist - centerNear).Length();
	const float distanceToCenter =
		(height * height + secondBase * secondBase - firstBase * firstBase) * 0.5f / height;

	CVector3D position = cameraTranslation + cameraIn * (nearPlane + distanceToCenter);
	const float radius = (cornerNear - position).Length();

	// We need to convert the bounding box to the light space.
	position = lightTransform.Rotate(position);

	const float insets = 0.2f;
	*bbaa = CBoundingBoxAligned(position, position);
	bbaa->Expand(radius);
	bbaa->Expand(insets);
}

ShadowMap::ShadowMap()
{
	m = new ShadowMapInternals;
	m->Framebuffer = 0;
	m->Width = 0;
	m->Height = 0;
	m->QualityLevel = 0;
	m->EffectiveWidth = 0;
	m->EffectiveHeight = 0;
	m->DepthTextureBits = 0;
	// DepthTextureBits: 24/32 are very much faster than 16, on GeForce 4 and FX;
	// but they're very much slower on Radeon 9800.
	// In both cases, the default (no specified depth) is fast, so we just use
	// that by default and hope it's alright. (Otherwise, we'd probably need to
	// do some kind of hardware detection to work out what to use.)

	// Avoid using uninitialised values in AddShadowedBound if SetupFrame wasn't called first
	m->LightTransform.SetIdentity();

	m->UpdateCascadesParameters();
}

ShadowMap::~ShadowMap()
{
	m->Framebuffer.reset();
	m->Texture.reset();
	m->DummyTexture.reset();

	delete m;
}

// Force the texture/buffer/etc to be recreated, particularly when the renderer's
// size has changed
void ShadowMap::RecreateTexture()
{
	m->Framebuffer.reset();
	m->Texture.reset();
	m->DummyTexture.reset();

	m->UpdateCascadesParameters();

	// (Texture will be constructed in next SetupFrame)
}

// SetupFrame: camera and light direction for this frame
void ShadowMap::SetupFrame(const CCamera& camera, const CVector3D& lightdir)
{
	if (!m->Texture)
		m->CreateTexture();

	CVector3D x(0, 1, 0), eyepos;

	CVector3D z = lightdir;
	z.Normalize();
	x -= z * z.Dot(x);
	if (x.Length() < 0.001)
	{
		// this is invoked if the camera and light directions almost coincide
		// assumption: light direction has a significant Z component
		x = CVector3D(1.0, 0.0, 0.0);
		x -= z * z.Dot(x);
	}
	x.Normalize();
	CVector3D y = z.Cross(x);

	// X axis perpendicular to light direction, flowing along with view direction
	m->LightTransform._11 = x.X;
	m->LightTransform._12 = x.Y;
	m->LightTransform._13 = x.Z;

	// Y axis perpendicular to light and view direction
	m->LightTransform._21 = y.X;
	m->LightTransform._22 = y.Y;
	m->LightTransform._23 = y.Z;

	// Z axis is in direction of light
	m->LightTransform._31 = z.X;
	m->LightTransform._32 = z.Y;
	m->LightTransform._33 = z.Z;

	// eye is at the origin of the coordinate system
	m->LightTransform._14 = -x.Dot(eyepos);
	m->LightTransform._24 = -y.Dot(eyepos);
	m->LightTransform._34 = -z.Dot(eyepos);

	m->LightTransform._41 = 0.0;
	m->LightTransform._42 = 0.0;
	m->LightTransform._43 = 0.0;
	m->LightTransform._44 = 1.0;

	m->LightTransform.GetInverse(m->InvLightTransform);
	m->ShadowReceiverBound.SetEmpty();

	m->LightspaceCamera = camera;
	m->LightspaceCamera.m_Orientation = m->LightTransform * camera.m_Orientation;
	m->LightspaceCamera.UpdateFrustum();

	m->ShadowsCutoffDistance = DEFAULT_SHADOWS_CUTOFF_DISTANCE;
	m->CascadeDistanceRatio = DEFAULT_CASCADE_DISTANCE_RATIO;
	CFG_GET_VAL("shadowscutoffdistance", m->ShadowsCutoffDistance);
	CFG_GET_VAL("shadowscascadedistanceratio", m->CascadeDistanceRatio);
	m->CascadeDistanceRatio = Clamp(m->CascadeDistanceRatio, 1.1f, 16.0f);

	m->Cascades[GetCascadeCount() - 1].Distance = m->ShadowsCutoffDistance;
	for (int cascade = GetCascadeCount() - 2; cascade >= 0; --cascade)
		m->Cascades[cascade].Distance = m->Cascades[cascade + 1].Distance / m->CascadeDistanceRatio;

	if (GetCascadeCount() == 1 || m->ShadowsCoverMap)
	{
		m->Cascades[0].ViewPort =
			SViewPort{1, 1, m->EffectiveWidth - 2, m->EffectiveHeight - 2};
		if (m->ShadowsCoverMap)
			m->Cascades[0].Distance = camera.GetFarPlane();
	}
	else
	{
		for (int cascade = 0; cascade < GetCascadeCount(); ++cascade)
		{
			const int offsetX = (cascade & 0x1) ? m->EffectiveWidth / 2 : 0;
			const int offsetY = (cascade & 0x2) ? m->EffectiveHeight / 2 : 0;
			m->Cascades[cascade].ViewPort =
				SViewPort{offsetX + 1, offsetY + 1,
				m->EffectiveWidth / 2 - 2, m->EffectiveHeight / 2 - 2};
		}
	}

	for (int cascadeIdx = 0; cascadeIdx < GetCascadeCount(); ++cascadeIdx)
	{
		ShadowMapInternals::Cascade& cascade = m->Cascades[cascadeIdx];

		const float nearPlane = cascadeIdx > 0 ?
			m->Cascades[cascadeIdx - 1].Distance : camera.GetNearPlane();
		const float farPlane = cascade.Distance;

		CalculateBoundsForCascade(camera, m->LightTransform,
			nearPlane, farPlane, &cascade.ConvexBounds, &cascade.FrustumBBAA);
		cascade.ShadowCasterBound.SetEmpty();
	}
}

// AddShadowedBound: add a world-space bounding box to the bounds of shadowed
// objects
void ShadowMap::AddShadowCasterBound(const int cascade, const CBoundingBoxAligned& bounds)
{
	CBoundingBoxAligned lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->Cascades[cascade].ShadowCasterBound += lightspacebounds;
}

void ShadowMap::AddShadowReceiverBound(const CBoundingBoxAligned& bounds)
{
	CBoundingBoxAligned lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->ShadowReceiverBound += lightspacebounds;
}

CFrustum ShadowMap::GetShadowCasterCullFrustum(const int cascade)
{
	// Get the bounds of all objects that can receive shadows
	CBoundingBoxAligned bound = m->ShadowReceiverBound;

	// Intersect with the camera frustum, so the shadow map doesn't have to get
	// stretched to cover the off-screen parts of large models
	bound.IntersectFrustumConservative(m->Cascades[cascade].FrustumBBAA.ToFrustum());

	// ShadowBound might have been empty to begin with, producing an empty result
	if (bound.IsEmpty())
	{
		// CFrustum can't easily represent nothingness, so approximate it with
		// a single point which won't match many objects
		bound += CVector3D(0.0f, 0.0f, 0.0f);
		return bound.ToFrustum();
	}

	// Extend the bounds a long way towards the light source, to encompass
	// all objects that might cast visible shadows.
	// (The exact constant was picked entirely arbitrarily.)
	bound[0].Z -= 1000.f;

	CFrustum frustum = bound.ToFrustum();
	frustum.Transform(m->InvLightTransform);
	return frustum;
}

// CalculateShadowMatrices: calculate required matrices for shadow map generation - the light's
// projection and transformation matrices
void ShadowMapInternals::CalculateShadowMatrices(const int cascade)
{
	CBoundingBoxAligned& shadowRenderBound = Cascades[cascade].ShadowRenderBound;
	shadowRenderBound = Cascades[cascade].ConvexBounds;

	if (ShadowsCoverMap)
	{
		// Start building the shadow map to cover all objects that will receive shadows
		CBoundingBoxAligned receiverBound = ShadowReceiverBound;

		// Intersect with the camera frustum, so the shadow map doesn't have to get
		// stretched to cover the off-screen parts of large models
		receiverBound.IntersectFrustumConservative(LightspaceCamera.GetFrustum());

		// Intersect with the shadow caster bounds, because there's no point
		// wasting space around the edges of the shadow map that we're not going
		// to draw into
		shadowRenderBound[0].X = std::max(receiverBound[0].X, Cascades[cascade].ShadowCasterBound[0].X);
		shadowRenderBound[0].Y = std::max(receiverBound[0].Y, Cascades[cascade].ShadowCasterBound[0].Y);
		shadowRenderBound[1].X = std::min(receiverBound[1].X, Cascades[cascade].ShadowCasterBound[1].X);
		shadowRenderBound[1].Y = std::min(receiverBound[1].Y, Cascades[cascade].ShadowCasterBound[1].Y);
	}
	else if (CascadeCount > 1)
	{
		// We need to offset the cascade to its place on the texture.
		const CVector3D size = (shadowRenderBound[1] - shadowRenderBound[0]) * 0.5f;
		if (!(cascade & 0x1))
			shadowRenderBound[1].X += size.X * 2.0f;
		else
			shadowRenderBound[0].X -= size.X * 2.0f;
		if (!(cascade & 0x2))
			shadowRenderBound[1].Y += size.Y * 2.0f;
		else
			shadowRenderBound[0].Y -= size.Y * 2.0f;
	}

	// Set the near and far planes to include just the shadow casters,
	// so we make full use of the depth texture's range. Add a bit of a
	// delta so we don't accidentally clip objects that are directly on
	// the planes.
	shadowRenderBound[0].Z = Cascades[cascade].ShadowCasterBound[0].Z - 2.f;
	shadowRenderBound[1].Z = Cascades[cascade].ShadowCasterBound[1].Z + 2.f;

	// Setup orthogonal projection (lightspace -> clip space) for shadowmap rendering
	CVector3D scale = shadowRenderBound[1] - shadowRenderBound[0];
	CVector3D shift = (shadowRenderBound[1] + shadowRenderBound[0]) * -0.5;

	if (scale.X < 1.0)
		scale.X = 1.0;
	if (scale.Y < 1.0)
		scale.Y = 1.0;
	if (scale.Z < 1.0)
		scale.Z = 1.0;

	scale.X = 2.0 / scale.X;
	scale.Y = 2.0 / scale.Y;
	scale.Z = 2.0 / scale.Z;

	// make sure a given world position falls on a consistent shadowmap texel fractional offset
	float offsetX = fmod(shadowRenderBound[0].X - LightTransform._14, 2.0f/(scale.X*EffectiveWidth));
	float offsetY = fmod(shadowRenderBound[0].Y - LightTransform._24, 2.0f/(scale.Y*EffectiveHeight));

	CMatrix3D& lightProjection = Cascades[cascade].LightProjection;
	lightProjection.SetZero();
	lightProjection._11 = scale.X;
	lightProjection._14 = (shift.X + offsetX) * scale.X;
	lightProjection._22 = scale.Y;
	lightProjection._24 = (shift.Y + offsetY) * scale.Y;
	lightProjection._33 = scale.Z;
	lightProjection._34 = shift.Z * scale.Z;
	lightProjection._44 = 1.0;

	// Calculate texture matrix by creating the clip space to texture coordinate matrix
	// and then concatenating all matrices that have been calculated so far

	float texscalex = scale.X * 0.5f * (float)EffectiveWidth / (float)Width;
	float texscaley = scale.Y * 0.5f * (float)EffectiveHeight / (float)Height;
	float texscalez = scale.Z * 0.5f;

	CMatrix3D lightToTex;
	lightToTex.SetZero();
	lightToTex._11 = texscalex;
	lightToTex._14 = (offsetX - shadowRenderBound[0].X) * texscalex;
	lightToTex._22 = texscaley;
	lightToTex._24 = (offsetY - shadowRenderBound[0].Y) * texscaley;
	lightToTex._33 = texscalez;
	lightToTex._34 = -shadowRenderBound[0].Z * texscalez;
	lightToTex._44 = 1.0;

	Cascades[cascade].TextureMatrix = lightToTex * LightTransform;
}

// Create the shadow map
void ShadowMapInternals::CreateTexture()
{
	// Cleanup
	Framebuffer.reset();
	Texture.reset();
	DummyTexture.reset();

	Renderer::Backend::GL::CDevice* backendDevice = g_VideoMode.GetBackendDevice();

	CFG_GET_VAL("shadowquality", QualityLevel);

	// Get shadow map size as next power of two up from view width/height.
	int shadowMapSize;
	switch (QualityLevel)
	{
	// Low
	case -1:
		shadowMapSize = 512;
		break;
	// High
	case 1:
		shadowMapSize = 2048;
		break;
	// Ultra
	case 2:
		shadowMapSize = std::max(round_up_to_pow2(std::max(g_Renderer.GetWidth(), g_Renderer.GetHeight())) * 4, 4096);
		break;
	// Medium as is
	default:
		shadowMapSize = 1024;
		break;
	}

	// Clamp to the maximum texture size.
	shadowMapSize = std::min(
		shadowMapSize, static_cast<int>(backendDevice->GetCapabilities().maxTextureSize));

	Width = Height = shadowMapSize;

	// Since we're using a framebuffer object, the whole texture is available
	EffectiveWidth = Width;
	EffectiveHeight = Height;

	const char* formatName;
	Renderer::Backend::Format backendFormat = Renderer::Backend::Format::UNDEFINED;
#if CONFIG2_GLES
	formatName = "DEPTH_COMPONENT";
	backendFormat = Renderer::Backend::Format::D24;
#else
	switch (DepthTextureBits)
	{
	case 16: formatName = "Format::D16"; backendFormat = Renderer::Backend::Format::D16; break;
	case 24: formatName = "Format::D24"; backendFormat = Renderer::Backend::Format::D24; break;
	case 32: formatName = "Format::D32"; backendFormat = Renderer::Backend::Format::D32;  break;
	default: formatName = "Format::D24"; backendFormat = Renderer::Backend::Format::D24; break;
	}
#endif
	ENSURE(formatName);

	LOGMESSAGE("Creating shadow texture (size %dx%d) (format = %s)",
		Width, Height, formatName);

	if (g_RenderingOptions.GetShadowAlphaFix())
	{
		DummyTexture = backendDevice->CreateTexture2D("ShadowMapDummy",
			Renderer::Backend::Format::R8G8B8A8, Width, Height,
			Renderer::Backend::Sampler::MakeDefaultSampler(
				Renderer::Backend::Sampler::Filter::NEAREST,
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));
	}

	Renderer::Backend::Sampler::Desc samplerDesc =
		Renderer::Backend::Sampler::MakeDefaultSampler(
#if CONFIG2_GLES
			// GLES doesn't do depth comparisons, so treat it as a
			// basic unfiltered depth texture
			Renderer::Backend::Sampler::Filter::NEAREST,
#else
			// Use LINEAR to trigger automatic PCF on some devices.
			Renderer::Backend::Sampler::Filter::LINEAR,
#endif
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);
	// Enable automatic depth comparisons
	samplerDesc.compareEnabled = true;
	samplerDesc.compareOp = Renderer::Backend::CompareOp::LESS_OR_EQUAL;

	Texture = backendDevice->CreateTexture2D("ShadowMapDepth",
		backendFormat, Width, Height, samplerDesc);

	Framebuffer = backendDevice->CreateFramebuffer("ShadowMapFramebuffer",
		g_RenderingOptions.GetShadowAlphaFix() ? DummyTexture.get() : nullptr, Texture.get());

	if (!Framebuffer)
	{
		LOGERROR("Failed to create shadows framebuffer");

		// Disable shadow rendering (but let the user try again if they want).
		g_RenderingOptions.SetShadows(false);
	}
}

// Set up to render into shadow map texture
void ShadowMap::BeginRender()
{
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();

	{
		PROFILE("bind framebuffer");
		deviceCommandContext->SetFramebuffer(m->Framebuffer.get());
	}

	// clear buffers
	{
		PROFILE("clear depth texture");
		// In case we used m_ShadowAlphaFix, we ought to clear the unused
		// color buffer too, else Mali 400 drivers get confused.
		// Might as well clear stencil too for completeness.
		deviceCommandContext->ClearFramebuffer();
	}

	m->SavedViewCamera = g_Renderer.GetSceneRenderer().GetViewCamera();
}

void ShadowMap::PrepareCamera(const int cascade)
{
	m->CalculateShadowMatrices(cascade);

	const SViewPort vp = { 0, 0, m->EffectiveWidth, m->EffectiveHeight };
	g_Renderer.SetViewport(vp);

	CCamera camera = m->SavedViewCamera;
	camera.SetProjection(m->Cascades[cascade].LightProjection);
	camera.GetOrientation() = m->InvLightTransform;
	g_Renderer.GetSceneRenderer().SetViewCamera(camera);

	const SViewPort& cascadeViewPort = m->Cascades[cascade].ViewPort;
	Renderer::Backend::GL::CDeviceCommandContext::ScissorRect scissorRect;
	scissorRect.x = cascadeViewPort.m_X;
	scissorRect.y = cascadeViewPort.m_Y;
	scissorRect.width = cascadeViewPort.m_Width;
	scissorRect.height = cascadeViewPort.m_Height;
	g_Renderer.GetDeviceCommandContext()->SetScissors(1, &scissorRect);
}

// Finish rendering into shadow map texture
void ShadowMap::EndRender()
{
	g_Renderer.GetDeviceCommandContext()->SetScissors(0, nullptr);

	g_Renderer.GetSceneRenderer().SetViewCamera(m->SavedViewCamera);

	{
		PROFILE("unbind framebuffer");
		g_Renderer.GetDeviceCommandContext()->SetFramebuffer(
			g_VideoMode.GetBackendDevice()->GetCurrentBackbuffer());
	}

	const SViewPort vp = { 0, 0, g_Renderer.GetWidth(), g_Renderer.GetHeight() };
	g_Renderer.SetViewport(vp);
}

void ShadowMap::BindTo(const CShaderProgramPtr& shader) const
{
	if (!shader->GetTextureBinding(str_shadowTex).Active() || !m->Texture)
		return;

	shader->BindTexture(str_shadowTex, m->Texture.get());
	shader->Uniform(str_shadowScale, m->Width, m->Height, 1.0f / m->Width, 1.0f / m->Height);
	const CVector3D cameraForward = g_Renderer.GetSceneRenderer().GetCullCamera().GetOrientation().GetIn();
	shader->Uniform(str_cameraForward, cameraForward.X, cameraForward.Y, cameraForward.Z,
		cameraForward.Dot(g_Renderer.GetSceneRenderer().GetCullCamera().GetOrientation().GetTranslation()));
	if (GetCascadeCount() == 1)
	{
		shader->Uniform(str_shadowTransform, m->Cascades[0].TextureMatrix);
		shader->Uniform(str_shadowDistance, m->Cascades[0].Distance);
	}
	else
	{
		std::vector<float> shadowDistances;
		std::vector<CMatrix3D> shadowTransforms;
		for (const ShadowMapInternals::Cascade& cascade : m->Cascades)
		{
			shadowDistances.emplace_back(cascade.Distance);
			shadowTransforms.emplace_back(cascade.TextureMatrix);
		}
		shader->Uniform(str_shadowTransforms_0, GetCascadeCount(), shadowTransforms.data());
		shader->Uniform(str_shadowTransforms, GetCascadeCount(), shadowTransforms.data());
		shader->Uniform(str_shadowDistances_0, GetCascadeCount(), shadowDistances.data());
		shader->Uniform(str_shadowDistances, GetCascadeCount(), shadowDistances.data());
	}
}

// Depth texture bits
int ShadowMap::GetDepthTextureBits() const
{
	return m->DepthTextureBits;
}

void ShadowMap::SetDepthTextureBits(int bits)
{
	if (bits != m->DepthTextureBits)
	{
		m->Texture.reset();
		m->Width = m->Height = 0;

		m->DepthTextureBits = bits;
	}
}

void ShadowMap::RenderDebugBounds()
{
	// Render various shadow bounds:
	//  Yellow = bounds of objects in view frustum that receive shadows
	//  Red = culling frustum used to find potential shadow casters
	//  Blue = frustum used for rendering the shadow map

	const CMatrix3D transform = g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection() * m->InvLightTransform;

	g_Renderer.GetDebugRenderer().DrawBoundingBox(
		m->ShadowReceiverBound, CColor(1.0f, 1.0f, 0.0f, 1.0f), transform, true);

	for (int cascade = 0; cascade < GetCascadeCount(); ++cascade)
	{
		g_Renderer.GetDebugRenderer().DrawBoundingBox(
			m->Cascades[cascade].ShadowRenderBound, CColor(0.0f, 0.0f, 1.0f, 0.10f), transform);
		g_Renderer.GetDebugRenderer().DrawBoundingBox(
			m->Cascades[cascade].ShadowRenderBound, CColor(0.0f, 0.0f, 1.0f, 0.5f), transform, true);

		const CFrustum frustum = GetShadowCasterCullFrustum(cascade);
		// We don't have a function to create a brush directly from a frustum, so use
		// the ugly approach of creating a large cube and then intersecting with the frustum
		const CBoundingBoxAligned dummy(CVector3D(-1e4, -1e4, -1e4), CVector3D(1e4, 1e4, 1e4));
		CBrush brush(dummy);
		CBrush frustumBrush;
		brush.Intersect(frustum, frustumBrush);

		g_Renderer.GetDebugRenderer().DrawBrush(frustumBrush, CColor(1.0f, 0.0f, 0.0f, 0.1f));
		g_Renderer.GetDebugRenderer().DrawBrush(frustumBrush, CColor(1.0f, 0.0f, 0.0f, 0.1f), true);
	}

	ogl_WarnIfError();
}

void ShadowMap::RenderDebugTexture(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (!m->Texture)
		return;

#if !CONFIG2_GLES
	deviceCommandContext->BindTexture(0, GL_TEXTURE_2D, m->Texture->GetHandle());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
#endif

	CShaderTechniquePtr texTech = g_Renderer.GetShaderManager().LoadEffect(str_canvas2d);
	texTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		texTech->GetGraphicsPipelineStateDesc());

	const CShaderProgramPtr& texShader = texTech->GetShader();

	texShader->Uniform(str_transform, GetDefaultGuiMatrix());
	texShader->BindTexture(str_tex, m->Texture.get());
	texShader->Uniform(str_colorAdd, CColor(0.0f, 0.0f, 0.0f, 1.0f));
	texShader->Uniform(str_colorMul, CColor(1.0f, 1.0f, 1.0f, 0.0f));
	texShader->Uniform(str_grayscaleFactor, 0.0f);

	float s = 256.f;
	float boxVerts[] =
	{
 		0,0, 0,s, s,0,
		s,0, 0,s, s,s
	};
	float boxUV[] =
	{
		0,0, 0,1, 1,0,
		1,0, 0,1, 1,1
	};

	texShader->VertexPointer(2, GL_FLOAT, 0, boxVerts);
	texShader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, boxUV);
	texShader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	texTech->EndPass();

#if !CONFIG2_GLES
	deviceCommandContext->BindTexture(0, GL_TEXTURE_2D, m->Texture->GetHandle());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
#endif

	ogl_WarnIfError();
}

int ShadowMap::GetCascadeCount() const
{
#if CONFIG2_GLES
	return 1;
#else
	return m->ShadowsCoverMap ? 1 : m->CascadeCount;
#endif
}
