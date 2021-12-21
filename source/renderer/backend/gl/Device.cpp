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
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

#if OS_WIN
#include "lib/sysdep/os/win/wgfx.h"
#endif

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// TODO: Support OpenGL platforms which don't use GLX as well.
#if defined(SDL_VIDEO_DRIVER_X11) && !CONFIG2_GLES
#include <GL/glx.h>
#include <SDL_syswm.h>

// Define the GLX_MESA_query_renderer macros if built with
// an old Mesa (<10.0) that doesn't provide them
#ifndef GLX_MESA_query_renderer
#define GLX_MESA_query_renderer 1
#define GLX_RENDERER_VENDOR_ID_MESA                      0x8183
#define GLX_RENDERER_DEVICE_ID_MESA                      0x8184
#define GLX_RENDERER_VERSION_MESA                        0x8185
#define GLX_RENDERER_ACCELERATED_MESA                    0x8186
#define GLX_RENDERER_VIDEO_MEMORY_MESA                   0x8187
#define GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA    0x8188
#define GLX_RENDERER_PREFERRED_PROFILE_MESA              0x8189
#define GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA    0x818A
#define GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA    0x818B
#define GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA      0x818C
#define GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA     0x818D
#define GLX_RENDERER_ID_MESA                             0x818E
#endif /* GLX_MESA_query_renderer */

#endif

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
		device->m_Window = window;
		device->m_Context = SDL_GL_CreateContext(device->m_Window);
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

void CDevice::Report(const ScriptRequest& rq, JS::HandleValue settings)
{
	const char* errstr = "(error)";

#define INTEGER(id) do { \
	GLint i = -1; \
	glGetIntegerv(GL_##id, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_" #id, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_" #id, i); \
	} while (false)

#define INTEGER2(id) do { \
	GLint i[2] = { -1, -1 }; \
	glGetIntegerv(GL_##id, i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) { \
		Script::SetProperty(rq, settings, "GL_" #id "[0]", errstr); \
		Script::SetProperty(rq, settings, "GL_" #id "[1]", errstr); \
	} else { \
		Script::SetProperty(rq, settings, "GL_" #id "[0]", i[0]); \
		Script::SetProperty(rq, settings, "GL_" #id "[1]", i[1]); \
	} \
	} while (false)

#define FLOAT(id) do { \
	GLfloat f = std::numeric_limits<GLfloat>::quiet_NaN(); \
	glGetFloatv(GL_##id, &f); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_" #id, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_" #id, f); \
	} while (false)

#define FLOAT2(id) do { \
	GLfloat f[2] = { std::numeric_limits<GLfloat>::quiet_NaN(), std::numeric_limits<GLfloat>::quiet_NaN() }; \
	glGetFloatv(GL_##id, f); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) { \
		Script::SetProperty(rq, settings, "GL_" #id "[0]", errstr); \
		Script::SetProperty(rq, settings, "GL_" #id "[1]", errstr); \
	} else { \
		Script::SetProperty(rq, settings, "GL_" #id "[0]", f[0]); \
		Script::SetProperty(rq, settings, "GL_" #id "[1]", f[1]); \
	} \
	} while (false)

#define STRING(id) do { \
	const char* c = (const char*)glGetString(GL_##id); \
	if (!c) c = ""; \
	if (ogl_SquelchError(GL_INVALID_ENUM)) c = errstr; \
	Script::SetProperty(rq, settings, "GL_" #id, std::string(c)); \
	}  while (false)

#define QUERY(target, pname) do { \
	GLint i = -1; \
	pglGetQueryivARB(GL_##target, GL_##pname, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_" #target ".GL_" #pname, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_" #target ".GL_" #pname, i); \
	} while (false)

#define VERTEXPROGRAM(id) do { \
	GLint i = -1; \
	pglGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_##id, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_VERTEX_PROGRAM_ARB.GL_" #id, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_VERTEX_PROGRAM_ARB.GL_" #id, i); \
	} while (false)

#define FRAGMENTPROGRAM(id) do { \
	GLint i = -1; \
	pglGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_##id, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_FRAGMENT_PROGRAM_ARB.GL_" #id, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_FRAGMENT_PROGRAM_ARB.GL_" #id, i); \
	} while (false)

#define BOOL(id) INTEGER(id)

	ogl_WarnIfError();

	// Core OpenGL 1.3:
	// (We don't bother checking extension strings for anything older than 1.3;
	// it'll just produce harmless warnings)
	STRING(VERSION);
	STRING(VENDOR);
	STRING(RENDERER);
	STRING(EXTENSIONS);

#if !CONFIG2_GLES
	INTEGER(MAX_CLIP_PLANES);
#endif
	INTEGER(SUBPIXEL_BITS);
#if !CONFIG2_GLES
	INTEGER(MAX_3D_TEXTURE_SIZE);
#endif
	INTEGER(MAX_TEXTURE_SIZE);
	INTEGER(MAX_CUBE_MAP_TEXTURE_SIZE);
	INTEGER2(MAX_VIEWPORT_DIMS);

#if !CONFIG2_GLES
	BOOL(RGBA_MODE);
	BOOL(INDEX_MODE);
	BOOL(DOUBLEBUFFER);
	BOOL(STEREO);
#endif

	FLOAT2(ALIASED_POINT_SIZE_RANGE);
	FLOAT2(ALIASED_LINE_WIDTH_RANGE);
#if !CONFIG2_GLES
	INTEGER(MAX_ELEMENTS_INDICES);
	INTEGER(MAX_ELEMENTS_VERTICES);
	INTEGER(MAX_TEXTURE_UNITS);
#endif
	INTEGER(SAMPLE_BUFFERS);
	INTEGER(SAMPLES);
	// TODO: compressed texture formats
	INTEGER(RED_BITS);
	INTEGER(GREEN_BITS);
	INTEGER(BLUE_BITS);
	INTEGER(ALPHA_BITS);
#if !CONFIG2_GLES
	INTEGER(INDEX_BITS);
#endif
	INTEGER(DEPTH_BITS);
	INTEGER(STENCIL_BITS);

#if !CONFIG2_GLES

	// Core OpenGL 2.0 (treated as extensions):

	if (ogl_HaveExtension("GL_EXT_texture_lod_bias"))
	{
		FLOAT(MAX_TEXTURE_LOD_BIAS_EXT);
	}

	if (ogl_HaveExtension("GL_ARB_occlusion_query"))
	{
		QUERY(SAMPLES_PASSED, QUERY_COUNTER_BITS);
	}

	if (ogl_HaveExtension("GL_ARB_shading_language_100"))
	{
		STRING(SHADING_LANGUAGE_VERSION_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_vertex_shader"))
	{
		INTEGER(MAX_VERTEX_ATTRIBS_ARB);
		INTEGER(MAX_VERTEX_UNIFORM_COMPONENTS_ARB);
		INTEGER(MAX_VARYING_FLOATS_ARB);
		INTEGER(MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB);
		INTEGER(MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_fragment_shader"))
	{
		INTEGER(MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_vertex_shader") || ogl_HaveExtension("GL_ARB_fragment_shader") ||
		ogl_HaveExtension("GL_ARB_vertex_program") || ogl_HaveExtension("GL_ARB_fragment_program"))
	{
		INTEGER(MAX_TEXTURE_IMAGE_UNITS_ARB);
		INTEGER(MAX_TEXTURE_COORDS_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_draw_buffers"))
	{
		INTEGER(MAX_DRAW_BUFFERS_ARB);
	}

	// Core OpenGL 3.0:

	if (ogl_HaveExtension("GL_EXT_gpu_shader4"))
	{
		INTEGER(MIN_PROGRAM_TEXEL_OFFSET); // no _EXT version of these in glext.h
		INTEGER(MAX_PROGRAM_TEXEL_OFFSET);
	}

	if (ogl_HaveExtension("GL_EXT_framebuffer_object"))
	{
		INTEGER(MAX_COLOR_ATTACHMENTS_EXT);
		INTEGER(MAX_RENDERBUFFER_SIZE_EXT);
	}

	if (ogl_HaveExtension("GL_EXT_framebuffer_multisample"))
	{
		INTEGER(MAX_SAMPLES_EXT);
	}

	if (ogl_HaveExtension("GL_EXT_texture_array"))
	{
		INTEGER(MAX_ARRAY_TEXTURE_LAYERS_EXT);
	}

	if (ogl_HaveExtension("GL_EXT_transform_feedback"))
	{
		INTEGER(MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT);
		INTEGER(MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT);
		INTEGER(MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT);
	}


	// Other interesting extensions:

	if (ogl_HaveExtension("GL_EXT_timer_query") || ogl_HaveExtension("GL_ARB_timer_query"))
	{
		QUERY(TIME_ELAPSED, QUERY_COUNTER_BITS);
	}

	if (ogl_HaveExtension("GL_ARB_timer_query"))
	{
		QUERY(TIMESTAMP, QUERY_COUNTER_BITS);
	}

	if (ogl_HaveExtension("GL_EXT_texture_filter_anisotropic"))
	{
		FLOAT(MAX_TEXTURE_MAX_ANISOTROPY_EXT);
	}

	if (ogl_HaveExtension("GL_ARB_texture_rectangle"))
	{
		INTEGER(MAX_RECTANGLE_TEXTURE_SIZE_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_vertex_program") || ogl_HaveExtension("GL_ARB_fragment_program"))
	{
		INTEGER(MAX_PROGRAM_MATRICES_ARB);
		INTEGER(MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_vertex_program"))
	{
		VERTEXPROGRAM(MAX_PROGRAM_ENV_PARAMETERS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_LOCAL_PARAMETERS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_INSTRUCTIONS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_TEMPORARIES_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_PARAMETERS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_ATTRIBS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_ADDRESS_REGISTERS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_NATIVE_TEMPORARIES_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_NATIVE_PARAMETERS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_NATIVE_ATTRIBS_ARB);
		VERTEXPROGRAM(MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB);

		if (ogl_HaveExtension("GL_ARB_fragment_program"))
		{
			// The spec seems to say these should be supported, but
			// Mesa complains about them so let's not bother
			/*
			VERTEXPROGRAM(MAX_PROGRAM_ALU_INSTRUCTIONS_ARB);
			VERTEXPROGRAM(MAX_PROGRAM_TEX_INSTRUCTIONS_ARB);
			VERTEXPROGRAM(MAX_PROGRAM_TEX_INDIRECTIONS_ARB);
			VERTEXPROGRAM(MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB);
			VERTEXPROGRAM(MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB);
			VERTEXPROGRAM(MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB);
			*/
		}
	}

	if (ogl_HaveExtension("GL_ARB_fragment_program"))
	{
		FRAGMENTPROGRAM(MAX_PROGRAM_ENV_PARAMETERS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_LOCAL_PARAMETERS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_ALU_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_TEX_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_TEX_INDIRECTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_TEMPORARIES_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_PARAMETERS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_ATTRIBS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_TEMPORARIES_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_PARAMETERS_ARB);
		FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_ATTRIBS_ARB);

		if (ogl_HaveExtension("GL_ARB_vertex_program"))
		{
			// The spec seems to say these should be supported, but
			// Intel drivers on Windows complain about them so let's not bother
			/*
			FRAGMENTPROGRAM(MAX_PROGRAM_ADDRESS_REGISTERS_ARB);
			FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB);
			*/
		}
	}

	if (ogl_HaveExtension("GL_ARB_geometry_shader4"))
	{
		INTEGER(MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB);
		INTEGER(MAX_GEOMETRY_OUTPUT_VERTICES_ARB);
		INTEGER(MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB);
		INTEGER(MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB);
		INTEGER(MAX_GEOMETRY_VARYING_COMPONENTS_ARB);
		INTEGER(MAX_VERTEX_VARYING_COMPONENTS_ARB);
	}

#else // CONFIG2_GLES

	// Core OpenGL ES 2.0:

	STRING(SHADING_LANGUAGE_VERSION);
	INTEGER(MAX_VERTEX_ATTRIBS);
	INTEGER(MAX_VERTEX_UNIFORM_VECTORS);
	INTEGER(MAX_VARYING_VECTORS);
	INTEGER(MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	INTEGER(MAX_VERTEX_TEXTURE_IMAGE_UNITS);
	INTEGER(MAX_FRAGMENT_UNIFORM_VECTORS);
	INTEGER(MAX_TEXTURE_IMAGE_UNITS);
	INTEGER(MAX_RENDERBUFFER_SIZE);

#endif // CONFIG2_GLES


// TODO: Support OpenGL platforms which don't use GLX as well.
#if defined(SDL_VIDEO_DRIVER_X11) && !CONFIG2_GLES

#define GLXQCR_INTEGER(id) do { \
	unsigned int i = UINT_MAX; \
	if (pglXQueryCurrentRendererIntegerMESA(id, &i)) \
		Script::SetProperty(rq, settings, #id, i); \
	} while (false)

#define GLXQCR_INTEGER2(id) do { \
	unsigned int i[2] = { UINT_MAX, UINT_MAX }; \
	if (pglXQueryCurrentRendererIntegerMESA(id, i)) { \
		Script::SetProperty(rq, settings, #id "[0]", i[0]); \
		Script::SetProperty(rq, settings, #id "[1]", i[1]); \
	} \
	} while (false)

#define GLXQCR_INTEGER3(id) do { \
	unsigned int i[3] = { UINT_MAX, UINT_MAX, UINT_MAX }; \
	if (pglXQueryCurrentRendererIntegerMESA(id, i)) { \
		Script::SetProperty(rq, settings, #id "[0]", i[0]); \
		Script::SetProperty(rq, settings, #id "[1]", i[1]); \
		Script::SetProperty(rq, settings, #id "[2]", i[2]); \
	} \
	} while (false)

#define GLXQCR_STRING(id) do { \
	const char* str = pglXQueryCurrentRendererStringMESA(id); \
	if (str) \
		Script::SetProperty(rq, settings, #id ".string", str); \
	} while (false)


	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	const int ret = SDL_GetWindowWMInfo(m_Window, &wminfo);
	if (ret && wminfo.subsystem == SDL_SYSWM_X11)
	{
		Display* dpy = wminfo.info.x11.display;
		int scrnum = DefaultScreen(dpy);

		const char* glxexts = glXQueryExtensionsString(dpy, scrnum);

		Script::SetProperty(rq, settings, "glx_extensions", glxexts);

		if (strstr(glxexts, "GLX_MESA_query_renderer") && pglXQueryCurrentRendererIntegerMESA && pglXQueryCurrentRendererStringMESA)
		{
			GLXQCR_INTEGER(GLX_RENDERER_VENDOR_ID_MESA);
			GLXQCR_INTEGER(GLX_RENDERER_DEVICE_ID_MESA);
			GLXQCR_INTEGER3(GLX_RENDERER_VERSION_MESA);
			GLXQCR_INTEGER(GLX_RENDERER_ACCELERATED_MESA);
			GLXQCR_INTEGER(GLX_RENDERER_VIDEO_MEMORY_MESA);
			GLXQCR_INTEGER(GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA);
			GLXQCR_INTEGER(GLX_RENDERER_PREFERRED_PROFILE_MESA);
			GLXQCR_INTEGER2(GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA);
			GLXQCR_INTEGER2(GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA);
			GLXQCR_INTEGER2(GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA);
			GLXQCR_INTEGER2(GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA);
			GLXQCR_STRING(GLX_RENDERER_VENDOR_ID_MESA);
			GLXQCR_STRING(GLX_RENDERER_DEVICE_ID_MESA);
		}
	}
#endif // SDL_VIDEO_DRIVER_X11
}

void CDevice::Present()
{
	if (m_Window)
	{
		PROFILE3("swap buffers");
		SDL_GL_SwapWindow(m_Window);
		ogl_WarnIfError();
	}

	bool checkGLErrorAfterSwap = false;
	CFG_GET_VAL("gl.checkerrorafterswap", checkGLErrorAfterSwap);
#if defined(NDEBUG)
	if (!checkGLErrorAfterSwap)
		return;
#endif
	PROFILE3("error check");
	// We have to check GL errors after SwapBuffer to avoid possible
	// synchronizations during rendering.
	if (GLenum err = glGetError())
		ONCE(LOGERROR("GL error %s (0x%04x) occurred", ogl_GetErrorName(err), err));
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
