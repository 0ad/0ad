/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"

#if OS_WIN
#include "lib/sysdep/os/win/wgfx.h"
#endif

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace
{

std::string GetNameImpl()
{
	// GL_VENDOR+GL_RENDERER are good enough here, so we don't use WMI to detect the cards.
	// On top of that WMI can cause crashes with Nvidia Optimus and some netbooks
	// see http://trac.wildfiregames.com/ticket/1952
	//     http://trac.wildfiregames.com/ticket/1575
	char cardName[128];
	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	// Happens if called before GL initialization.
	if (!vendor || !renderer)
		return {};
	sprintf_s(cardName, std::size(cardName), "%s %s", vendor, renderer);

	// Remove crap from vendor names. (don't dare touch the model name -
	// it's too risky, there are too many different strings).
#define SHORTEN(what, charsToKeep) \
	if (!strncmp(cardName, what, std::size(what) - 1)) \
		memmove(cardName + charsToKeep, cardName + std::size(what) - 1, (strlen(cardName) - (std::size(what) - 1) + 1) * sizeof(char));
	SHORTEN("ATI Technologies Inc.", 3);
	SHORTEN("NVIDIA Corporation", 6);
	SHORTEN("S3 Graphics", 2);					// returned by EnumDisplayDevices
	SHORTEN("S3 Graphics, Incorporated", 2);	// returned by GL_VENDOR
#undef SHORTEN

	return cardName;
}

std::string GetVersionImpl()
{
	return reinterpret_cast<const char*>(glGetString(GL_VERSION));
}

std::string GetDriverInformationImpl()
{
	const std::string version = GetVersionImpl();

	std::string driverInfo;
#if OS_WIN
	driverInfo = CStrW(wgfx_DriverInfo()).ToUTF8();
	if (driverInfo.empty())
#endif
	{
		if (!version.empty())
		{
			// Add "OpenGL" to differentiate this from the real driver version
			// (returned by platform-specific detect routines).
			driverInfo = std::string("OpenGL ") + version;
		}
	}

	if (driverInfo.empty())
		return version;
	return version + " " + driverInfo;
}

std::vector<std::string> GetExtensionsImpl()
{
	std::vector<std::string> extensions;
	const std::string exts = ogl_ExtensionString();
	boost::split(extensions, exts, boost::algorithm::is_space(), boost::token_compress_on);
	std::sort(extensions.begin(), extensions.end());
	return extensions;
}

} // anonymous namespace

// static
std::unique_ptr<CDevice> CDevice::Create(SDL_Window* window)
{
	std::unique_ptr<CDevice> device(new CDevice());

	if (window)
	{
		device->m_Context = SDL_GL_CreateContext(window);
		if (!device->m_Context)
		{
			LOGERROR("SDL_GL_CreateContext failed: '%s'", SDL_GetError());
			return nullptr;
		}
	}

	ogl_Init();

	if ((ogl_HaveExtensions(0, "GL_ARB_vertex_program", "GL_ARB_fragment_program", nullptr) // ARB
		&& ogl_HaveExtensions(0, "GL_ARB_vertex_shader", "GL_ARB_fragment_shader", nullptr)) // GLSL
		|| !ogl_HaveExtension("GL_ARB_vertex_buffer_object") // VBO
		|| ogl_HaveExtensions(0, "GL_ARB_multitexture", "GL_EXT_draw_range_elements", nullptr)
		|| (!ogl_HaveExtension("GL_EXT_framebuffer_object") && !ogl_HaveExtension("GL_ARB_framebuffer_object")))
	{
		// It doesn't make sense to continue working here, because we're not
		// able to display anything.
		DEBUG_DISPLAY_FATAL_ERROR(
			L"Your graphics card doesn't appear to be fully compatible with OpenGL shaders."
			L" The game does not support pre-shader graphics cards."
			L" You are advised to try installing newer drivers and/or upgrade your graphics card."
			L" For more information, please see http://www.wildfiregames.com/forum/index.php?showtopic=16734"
		);
	}

	device->m_Name = GetNameImpl();
	device->m_Version = GetVersionImpl();
	device->m_DriverInformation = GetDriverInformationImpl();
	device->m_Extensions = GetExtensionsImpl();

	return device;
}

CDevice::CDevice() = default;

CDevice::~CDevice()
{
	if (m_Context)
		SDL_GL_DeleteContext(m_Context);
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
