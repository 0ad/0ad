/* Copyright (C) 2024 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_GL_BUFFER
#define INCLUDED_RENDERER_BACKEND_GL_BUFFER

#include "lib/ogl.h"
#include "renderer/backend/IBuffer.h"

#include <cstdint>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDevice;

class CBuffer final : public IBuffer
{
public:
	~CBuffer() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	uint32_t GetSize() const override { return m_Size; }
	uint32_t GetUsage() const override { return m_Usage; }

	GLuint GetHandle() { return m_Handle; }

private:
	friend class CDevice;

	static std::unique_ptr<CBuffer> Create(
		CDevice* device, const char* name,
		const Type type, const uint32_t size, const uint32_t usage);

	CBuffer();

	CDevice* m_Device = nullptr;
	Type m_Type = Type::VERTEX;
	uint32_t m_Size = 0;
	uint32_t m_Usage = 0;

	GLuint m_Handle = 0;
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_BUFFER
