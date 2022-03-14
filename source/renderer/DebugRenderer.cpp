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

#include "renderer/DebugRenderer.h"

#include "graphics/Camera.h"
#include "graphics/Color.h"
#include "graphics/ShaderManager.h"
#include "graphics/ShaderProgram.h"
#include "lib/ogl.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/Brush.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CStrInternStatic.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"

#include <cmath>

namespace
{

void SetGraphicsPipelineStateFromTechAndColor(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderTechniquePtr& tech, const CColor& color, const bool depthTestEnabled = true,
	const bool wireframe = false)
{
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc = tech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = depthTestEnabled;
	if (color.a != 1.0f)
	{
		pipelineStateDesc.blendState.enabled = true;
		pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
			Renderer::Backend::BlendFactor::SRC_ALPHA;
		pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
			Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
		pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
			Renderer::Backend::BlendOp::ADD;
	}
	else
		pipelineStateDesc.blendState.enabled = false;
	if (wireframe)
		pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
	pipelineStateDesc.rasterizationState.cullMode = Renderer::Backend::CullMode::NONE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
}

} // anonymous namespace

void CDebugRenderer::DrawLine(
	const CVector3D& from, const CVector3D& to, const CColor& color,
	const float width, const bool depthTestEnabled)
{
	if (from == to)
		return;

	DrawLine({from, to}, color, width, depthTestEnabled);
}

void CDebugRenderer::DrawLine(
	const std::vector<CVector3D>& line, const CColor& color,
	const float width, const bool depthTestEnabled)
{
#if CONFIG2_GLES
	UNUSED2(line); UNUSED2(color); UNUSED2(width); UNUSED2(depthTestEnabled);
	#warning TODO: implement drawing line for GLES
#else
	CShaderTechniquePtr debugLineTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_line);
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();
	SetGraphicsPipelineStateFromTechAndColor(
		deviceCommandContext, debugLineTech, color, depthTestEnabled);
	deviceCommandContext->BeginPass();

	const CCamera& viewCamera = g_Renderer.GetSceneRenderer().GetViewCamera();

	Renderer::Backend::GL::CShaderProgram* debugLineShader = debugLineTech->GetShader();
	debugLineShader->Uniform(str_transform, viewCamera.GetViewProjection());
	debugLineShader->Uniform(str_color, color);

	const CVector3D cameraIn = viewCamera.GetOrientation().GetIn();

	std::vector<float> vertices;
	vertices.reserve(line.size() * 6 * 3);
#define ADD(position) \
	vertices.emplace_back((position).X); \
	vertices.emplace_back((position).Y); \
	vertices.emplace_back((position).Z);

	for (size_t idx = 1; idx < line.size(); ++idx)
	{
		const CVector3D from = line[idx - 1];
		const CVector3D to = line[idx];
		const CVector3D direction = (to - from).Normalized();
		const CVector3D view = direction.Dot(cameraIn) > 0.9f ?
			CVector3D(0.0f, 1.0f, 0.0f) :
			cameraIn;
		const CVector3D offset = view.Cross(direction).Normalized() * width;

		ADD(from + offset)
		ADD(to - offset)
		ADD(to + offset)
		ADD(from + offset)
		ADD(from - offset)
		ADD(to - offset)
	}

#undef ADD

	debugLineShader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, vertices.data());
	debugLineShader->AssertPointersBound();
	deviceCommandContext->Draw(0, vertices.size() / 3);

	deviceCommandContext->EndPass();
#endif
}

void CDebugRenderer::DrawCircle(const CVector3D& origin, const float radius, const CColor& color)
{
#if CONFIG2_GLES
	UNUSED2(origin); UNUSED2(radius); UNUSED2(color);
	#warning TODO: implement drawing circle for GLES
#else
	CShaderTechniquePtr debugCircleTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_line);
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();
	SetGraphicsPipelineStateFromTechAndColor(
		deviceCommandContext, debugCircleTech, color);
	deviceCommandContext->BeginPass();

	const CCamera& camera = g_Renderer.GetSceneRenderer().GetViewCamera();

	Renderer::Backend::GL::CShaderProgram* debugCircleShader = debugCircleTech->GetShader();
	debugCircleShader->Uniform(str_transform, camera.GetViewProjection());
	debugCircleShader->Uniform(str_color, color);

	const CVector3D cameraUp = camera.GetOrientation().GetUp();
	const CVector3D cameraLeft = camera.GetOrientation().GetLeft();

	std::vector<float> vertices;
#define ADD(position) \
	vertices.emplace_back((position).X); \
	vertices.emplace_back((position).Y); \
	vertices.emplace_back((position).Z);

	constexpr size_t segments = 16;
	for (size_t idx = 0; idx <= segments; ++idx)
	{
		const float angle = M_PI * 2.0f * idx / segments;
		const CVector3D offset = cameraUp * sin(angle) - cameraLeft * cos(angle);
		const float nextAngle = M_PI * 2.0f * (idx + 1) / segments;
		const CVector3D nextOffset = cameraUp * sin(nextAngle) - cameraLeft * cos(nextAngle);
		ADD(origin)
		ADD(origin + offset * radius)
		ADD(origin + nextOffset * radius)
	}

#undef ADD

	debugCircleShader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, vertices.data());
	debugCircleShader->AssertPointersBound();
	deviceCommandContext->Draw(0, vertices.size() / 3);

	deviceCommandContext->EndPass();
#endif
}

void CDebugRenderer::DrawCameraFrustum(const CCamera& camera, const CColor& color, int intermediates, bool wireframe)
{
#if CONFIG2_GLES
	UNUSED2(camera); UNUSED2(color); UNUSED2(intermediates);
	#warning TODO: implement camera frustum for GLES
#else
	CCamera::Quad nearPoints;
	CCamera::Quad farPoints;

	camera.GetViewQuad(camera.GetNearPlane(), nearPoints);
	camera.GetViewQuad(camera.GetFarPlane(), farPoints);
	for (int i = 0; i < 4; ++i)
	{
		nearPoints[i] = camera.m_Orientation.Transform(nearPoints[i]);
		farPoints[i] = camera.m_Orientation.Transform(farPoints[i]);
	}

	CShaderTechniquePtr overlayTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_line);
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();
	SetGraphicsPipelineStateFromTechAndColor(
		deviceCommandContext, overlayTech, color, true, wireframe);
	deviceCommandContext->BeginPass();

	Renderer::Backend::GL::CShaderProgram* overlayShader = overlayTech->GetShader();
	overlayShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	overlayShader->Uniform(str_color, color);

	std::vector<float> vertices;
#define ADD(position) \
	vertices.emplace_back((position).X); \
	vertices.emplace_back((position).Y); \
	vertices.emplace_back((position).Z);

	// Near plane.
	ADD(nearPoints[0]);
	ADD(nearPoints[1]);
	ADD(nearPoints[2]);
	ADD(nearPoints[0]);
	ADD(nearPoints[2]);
	ADD(nearPoints[3]);

	// Far plane.
	ADD(farPoints[0]);
	ADD(farPoints[1]);
	ADD(farPoints[2]);
	ADD(farPoints[0]);
	ADD(farPoints[2]);
	ADD(farPoints[3]);

	// Intermediate planes.
	CVector3D intermediatePoints[4];
	for (int i = 0; i < intermediates; ++i)
	{
		const float t = (i + 1.0f) / (intermediates + 1.0f);

		for (int j = 0; j < 4; ++j)
			intermediatePoints[j] = nearPoints[j] * t + farPoints[j] * (1.0f - t);

		ADD(intermediatePoints[0]);
		ADD(intermediatePoints[1]);
		ADD(intermediatePoints[2]);
		ADD(intermediatePoints[0]);
		ADD(intermediatePoints[2]);
		ADD(intermediatePoints[3]);
	}

	overlayShader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, vertices.data());
	overlayShader->AssertPointersBound();
	deviceCommandContext->Draw(0, vertices.size() / 3);

	vertices.clear();

	// Connection lines.
	for (int i = 0; i < 4; ++i)
	{
		const int nextI = (i + 1) % 4;
		ADD(nearPoints[i]);
		ADD(farPoints[nextI]);
		ADD(farPoints[i]);
		ADD(nearPoints[i]);
		ADD(nearPoints[nextI]);
		ADD(farPoints[nextI]);
	}

	overlayShader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, vertices.data());
	overlayShader->AssertPointersBound();
	deviceCommandContext->Draw(0, vertices.size() / 3);
#undef ADD

	deviceCommandContext->EndPass();
#endif
}

void CDebugRenderer::DrawBoundingBox(
	const CBoundingBoxAligned& boundingBox, const CColor& color,
	bool wireframe)
{
	DrawBoundingBox(
		boundingBox, color,
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection(), wireframe);
}

void CDebugRenderer::DrawBoundingBox(
	const CBoundingBoxAligned& boundingBox, const CColor& color,
	const CMatrix3D& transform, bool wireframe)
{
	CShaderTechniquePtr shaderTech = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();
	SetGraphicsPipelineStateFromTechAndColor(
		deviceCommandContext, shaderTech, color, true, wireframe);
	deviceCommandContext->BeginPass();

	Renderer::Backend::GL::CShaderProgram* shader = shaderTech->GetShader();
	shader->Uniform(str_color, color);
	shader->Uniform(str_transform, transform);

	std::vector<float> data;

#define ADD_FACE(x, y, z) \
	ADD_PT(0, 0, x, y, z); ADD_PT(1, 0, x, y, z); ADD_PT(1, 1, x, y, z); \
	ADD_PT(1, 1, x, y, z); ADD_PT(0, 1, x, y, z); ADD_PT(0, 0, x, y, z);
#define ADD_PT(u_, v_, x, y, z) \
	STMT(int u = u_; int v = v_; \
		data.push_back(boundingBox[x].X); \
		data.push_back(boundingBox[y].Y); \
		data.push_back(boundingBox[z].Z); \
	)

	ADD_FACE(u, v, 0);
	ADD_FACE(0, u, v);
	ADD_FACE(u, 0, 1-v);
	ADD_FACE(u, 1-v, 1);
	ADD_FACE(1, u, 1-v);
	ADD_FACE(u, 1, v);

#undef ADD_FACE

	shader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 3 * sizeof(float), data.data());

	shader->AssertPointersBound();
	deviceCommandContext->Draw(0, 6*6);

	deviceCommandContext->EndPass();
}

void CDebugRenderer::DrawBrush(const CBrush& brush, const CColor& color, bool wireframe)
{
	CShaderTechniquePtr shaderTech = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();
	SetGraphicsPipelineStateFromTechAndColor(
		deviceCommandContext, shaderTech, color, true, wireframe);
	deviceCommandContext->BeginPass();

	Renderer::Backend::GL::CShaderProgram* shader = shaderTech->GetShader();
	shader->Uniform(str_color, color);
	shader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

	std::vector<float> data;

	std::vector<std::vector<size_t>> faces;
	brush.GetFaces(faces);

#define ADD_VERT(a) \
	STMT( \
		data.push_back(brush.GetVertices()[faces[i][a]].X); \
		data.push_back(brush.GetVertices()[faces[i][a]].Y); \
		data.push_back(brush.GetVertices()[faces[i][a]].Z); \
	)

	for (size_t i = 0; i < faces.size(); ++i)
	{
		// Triangulate into (0,1,2), (0,2,3), ...
		for (size_t j = 1; j < faces[i].size() - 2; ++j)
		{
			ADD_VERT(0);
			ADD_VERT(j);
			ADD_VERT(j+1);
		}
	}

#undef ADD_VERT

	shader->VertexPointer(
		Renderer::Backend::Format::R32G32B32_SFLOAT, 3 * sizeof(float), data.data());

	shader->AssertPointersBound();
	deviceCommandContext->Draw(0, data.size() / 5);

	deviceCommandContext->EndPass();
}

