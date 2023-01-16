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

#include "precompiled.h"

#include "Material.h"
#include "ps/CStrInternStatic.h"

CMaterial::CMaterial() :
	m_AlphaBlending(false)
{
}

void CMaterial::SetShaderEffect(const CStr& effect)
{
	m_ShaderEffect = CStrIntern(effect);
}

void CMaterial::AddShaderDefine(CStrIntern key, CStrIntern value)
{
	m_ShaderDefines.Add(key, value);
}

void CMaterial::AddStaticUniform(const char* key, const CVector4D& value)
{
	m_StaticUniforms.Add(key, value);
}

void CMaterial::AddSampler(const TextureSampler& texture)
{
	m_Samplers.push_back(texture);
	if (texture.Name == str_baseTex)
		m_DiffuseTexture = texture.Sampler;
}

void CMaterial::AddRenderQuery(const char* key)
{
	m_RenderQueries.Add(key);
}

void CMaterial::AddRequiredSampler(const CStr& samplerName)
{
	CStrIntern string(samplerName);
	m_RequiredSamplers.push_back(string);
}
