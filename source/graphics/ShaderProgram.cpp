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

#include "ShaderProgram.h"

#include "ps/VideoMode.h"
#include "renderer/backend/gl/Device.h"

CShaderProgram::CShaderProgram(const CStr& name, const CShaderDefines& defines)
	: m_Name(name), m_Defines(defines)
{
}

// static
CShaderProgramPtr CShaderProgram::Create(const CStr& name, const CShaderDefines& defines)
{
	return CShaderProgramPtr(new CShaderProgram(name, defines));
}

void CShaderProgram::Reload()
{
	std::unique_ptr<Renderer::Backend::GL::CShaderProgram> backendShaderProgram =
		g_VideoMode.GetBackendDevice()->CreateShaderProgram(m_Name, m_Defines);
	if (backendShaderProgram)
		m_BackendShaderProgram = std::move(backendShaderProgram);
}

std::vector<VfsPath> CShaderProgram::GetFileDependencies() const
{
	if (m_BackendShaderProgram)
		return m_BackendShaderProgram->GetFileDependencies();
	return {};
}
