/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_DECALRDATA
#define INCLUDED_DECALRDATA

#include "graphics/Camera.h"
#include "graphics/RenderableObject.h"
#include "graphics/ShaderProgramPtr.h"
#include "renderer/VertexArray.h"

class CModelDecal;
class CShaderDefines;
class CSimulation2;
class ShadowMap;

class CDecalRData : public CRenderData
{
public:
	CDecalRData(CModelDecal* decal, CSimulation2* simulation);
	~CDecalRData();

	void Update(CSimulation2* simulation);

	static void RenderDecals(std::vector<CDecalRData*>& decals, const CShaderDefines& context, 
			       ShadowMap* shadow, bool isDummyShader=false, const CShaderProgramPtr& dummy=CShaderProgramPtr());

	CModelDecal* GetDecal() { return m_Decal; }

private:
	void BuildArrays();

	VertexIndexArray m_IndexArray;

	VertexArray m_Array;
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_DiffuseColor;
	VertexArray::Attribute m_UV;

	CModelDecal* m_Decal;

	CSimulation2* m_Simulation;
};

#endif // INCLUDED_DECALRDATA
