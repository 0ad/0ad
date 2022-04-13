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

#include "Device.h"

#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

#if SDL_VERSION_ATLEAST(2, 0, 8)
#include <SDL_vulkan.h>
#endif

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

// static
std::unique_ptr<CDevice> CDevice::Create(SDL_Window* window)
{
	std::unique_ptr<CDevice> device(new CDevice());
	return device;
}

CDevice::CDevice() = default;

CDevice::~CDevice() = default;

void CDevice::Report(const ScriptRequest& rq, JS::HandleValue settings)
{
	std::string vulkanSupport = "unsupported";
	// According to http://wiki.libsdl.org/SDL_Vulkan_LoadLibrary the following
	// functionality is supported since SDL 2.0.8.
#if SDL_VERSION_ATLEAST(2, 0, 8)
	if (!SDL_Vulkan_LoadLibrary(nullptr))
	{
		void* vkGetInstanceProcAddr = SDL_Vulkan_GetVkGetInstanceProcAddr();
		if (vkGetInstanceProcAddr)
			vulkanSupport = "supported";
		else
			vulkanSupport = "noprocaddr";
		SDL_Vulkan_UnloadLibrary();
	}
	else
	{
		vulkanSupport = "cantload";
	}
#endif
	Script::SetProperty(rq, settings, "vulkan", vulkanSupport);
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
