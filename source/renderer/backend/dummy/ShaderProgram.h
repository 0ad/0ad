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

#ifndef INCLUDED_RENDERER_BACKEND_DUMMY_SHADERPROGRAM
#define INCLUDED_RENDERER_BACKEND_DUMMY_SHADERPROGRAM

#include "renderer/backend/IShaderProgram.h"

#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

class CDevice;

class CShaderProgram : public IShaderProgram
{
public:
	~CShaderProgram() override;

	IDevice* GetDevice() override;

	int32_t GetBindingSlot(const CStrIntern name) const override;

	std::vector<VfsPath> GetFileDependencies() const override;

protected:
	friend class CDevice;

	CShaderProgram();

	static std::unique_ptr<CShaderProgram> Create(CDevice* device);

	CDevice* m_Device = nullptr;
};

} // namespace Dummy

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_DUMMY_SHADERPROGRAM
