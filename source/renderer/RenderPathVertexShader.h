/* Copyright (C) 2009 Wildfire Games.
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

#ifndef INCLUDED_RENDERPATHVERTEXSHADER
#define INCLUDED_RENDERPATHVERTEXSHADER

#include "graphics/LightEnv.h"

// Interface for globallight.vs
struct VS_GlobalLight
{
public:
	void Init(Handle shader);

	void SetFromLightEnv(const CLightEnv& lightenv, bool units) const
	{
		SetAmbient(units ? lightenv.m_UnitsAmbientColor : lightenv.m_TerrainAmbientColor);
		SetSunDir(lightenv.GetSunDir());
		SetSunColor(lightenv.m_SunColor);
	}

	void SetAmbient(const RGBColor& color) const
	{
		pglUniform3fvARB(m_Ambient, 1, &color.X);
	}

	void SetSunDir(const CVector3D& sundir) const
	{
		pglUniform3fvARB(m_SunDir, 1, &sundir.X);
	}

	void SetSunColor(const RGBColor& color) const
	{
		pglUniform3fvARB(m_SunColor, 1, &color.X);
	}

private:
	GLint m_Ambient;
	GLint m_SunDir;
	GLint m_SunColor;
};

// Interface for instancing_base.vs
struct VS_Instancing
{
public:
	void Init(Handle shader);

	void SetMatrix(const CMatrix3D& mat) const
	{
		pglVertexAttrib4fARB(m_Instancing1, mat._11, mat._12, mat._13, mat._14);
		pglVertexAttrib4fARB(m_Instancing2, mat._21, mat._22, mat._23, mat._24);
		pglVertexAttrib4fARB(m_Instancing3, mat._31, mat._32, mat._33, mat._34);
	}

private:
	GLint m_Instancing1;
	GLint m_Instancing2;
	GLint m_Instancing3;
};


// Interface for postouv1.vs
struct VS_PosToUV1
{
public:
	void Init(Handle shader);

	void SetMatrix(const CMatrix3D& mat) const
	{
		pglUniform4fARB(m_TextureMatrix1, mat._11, mat._12, mat._13, mat._14);
		pglUniform4fARB(m_TextureMatrix2, mat._21, mat._22, mat._23, mat._24);
		pglUniform4fARB(m_TextureMatrix3, mat._31, mat._32, mat._33, mat._34);
	}

private:
	GLint m_TextureMatrix1;
	GLint m_TextureMatrix2;
	GLint m_TextureMatrix3;
};

class RenderPathVertexShader
{
public:
	RenderPathVertexShader();
	~RenderPathVertexShader();

	// Initialize this render path.
	bool Init();

	// Call once per frame to update program stuff
	void BeginFrame();

public:
	Handle m_ModelLight;
	VS_GlobalLight m_ModelLight_Light;

	Handle m_ModelLightP;
	VS_GlobalLight m_ModelLightP_Light;
	VS_PosToUV1 m_ModelLightP_PosToUV1;

	Handle m_InstancingLight;
	VS_GlobalLight m_InstancingLight_Light;
	VS_Instancing m_InstancingLight_Instancing;

	Handle m_InstancingLightP;
	VS_GlobalLight m_InstancingLightP_Light;
	VS_Instancing m_InstancingLightP_Instancing;
	VS_PosToUV1 m_InstancingLightP_PosToUV1;

	Handle m_Instancing;
	VS_Instancing m_Instancing_Instancing;

	Handle m_InstancingP;
	VS_Instancing m_InstancingP_Instancing;
	VS_PosToUV1 m_InstancingP_PosToUV1;
};

#endif // INCLUDED_RENDERPATHVERTEXSHADER
