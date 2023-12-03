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

#include "renderer/backend/dummy/Device.h"

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

CShaderProgram::CShaderProgram() = default;

CShaderProgram::~CShaderProgram() = default;

// static
std::unique_ptr<CShaderProgram> CShaderProgram::Create(CDevice* device)
{
	std::unique_ptr<CShaderProgram> shaderProgram(new CShaderProgram());
	shaderProgram->m_Device = device;
	return shaderProgram;
}

IDevice* CShaderProgram::GetDevice()
{
	return m_Device;
}

int32_t CShaderProgram::GetBindingSlot(const CStrIntern UNUSED(name)) const
{
	return -1;
}

std::vector<VfsPath> CShaderProgram::GetFileDependencies() const
{
	return {};
}

} // namespace Dummy

} // namespace Backend

} // namespace Renderer
