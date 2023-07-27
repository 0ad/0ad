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

#ifndef INCLUDED_SHADERPROGRAM
#define INCLUDED_SHADERPROGRAM

#include "graphics/ShaderDefines.h"
#include "graphics/ShaderProgramPtr.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "renderer/backend/IShaderProgram.h"

#include <vector>

/**
 * A wrapper for backend shader program to handle high-level operations like
 * file reloading and handling errors on reload.
 */
class CShaderProgram
{
	NONCOPYABLE(CShaderProgram);

public:
	static CShaderProgramPtr Create(
		Renderer::Backend::IDevice* device, const CStr& name, const CShaderDefines& defines);

	void Reload();

	std::vector<VfsPath> GetFileDependencies() const;

	Renderer::Backend::IShaderProgram* GetBackendShaderProgram() { return m_BackendShaderProgram.get(); }

	// TODO: add reloadable handles.

protected:
	CShaderProgram(
		Renderer::Backend::IDevice* device, const CStr& name, const CShaderDefines& defines);

	Renderer::Backend::IDevice* m_Device = nullptr;
	CStr m_Name;
	CShaderDefines m_Defines;
	std::unique_ptr<Renderer::Backend::IShaderProgram> m_BackendShaderProgram;
};

#endif // INCLUDED_SHADERPROGRAM
