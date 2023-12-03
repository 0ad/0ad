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

#ifndef INCLUDED_RENDERER_BACKEND_IDEVICEOBJECT
#define INCLUDED_RENDERER_BACKEND_IDEVICEOBJECT

#include <type_traits>

namespace Renderer
{

namespace Backend
{

class IDevice;

template<typename BaseDeviceObject>
class IDeviceObject
{
public:
	virtual ~IDeviceObject() {}

	virtual IDevice* GetDevice() = 0;

	template<typename T>
	T* As()
	{
		static_assert(std::is_base_of_v<BaseDeviceObject, T>);
		return static_cast<T*>(this);
	}
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_IDEVICEOBJECT
