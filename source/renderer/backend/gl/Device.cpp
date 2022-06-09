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

#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/backend/gl/Texture.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

#if OS_WIN
#include "lib/sysdep/os/win/wgfx.h"

// We can't include wutil directly because GL headers conflict with Windows
// until we use a proper GL loader.
extern void* wutil_GetAppHDC();
#endif

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#if !CONFIG2_GLES && (defined(SDL_VIDEO_DRIVER_X11) || defined(SDL_VIDEO_DRIVER_WAYLAND))

#if defined(SDL_VIDEO_DRIVER_X11)
#include <glad/glx.h>
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
#include <glad/egl.h>
#endif
#include <SDL_syswm.h>

#endif // !CONFIG2_GLES && (defined(SDL_VIDEO_DRIVER_X11) || defined(SDL_VIDEO_DRIVER_WAYLAND))

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

void GLAD_API_PTR OnDebugMessage(
	GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei UNUSED(length), const GLchar* message, const void* UNUSED(user_param))
{
	std::string debugSource = "unknown";
	std::string debugType = "unknown";
	std::string debugSeverity = "unknown";

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		debugSource = "the API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		debugSource = "the window system";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		debugSource = "the shader compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		debugSource = "a third party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		debugSource = "the application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		debugSource = "somewhere";
		break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		debugType = "error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		debugType = "deprecated behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		debugType = "undefined behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		debugType = "portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		debugType = "performance";
		break;
	case GL_DEBUG_TYPE_OTHER:
		debugType = "other";
		break;
	case GL_DEBUG_TYPE_MARKER:
		debugType = "marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		debugType = "push group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		debugType = "pop group";
		break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		debugSeverity = "high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		debugSeverity = "medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		debugSeverity = "low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		debugSeverity = "notification";
		break;
	}

	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		debug_printf(
			"OpenGL | %s: %s source: %s id %u: %s\n", debugSeverity.c_str(), debugType.c_str(), debugSource.c_str(), id, message);
	}
	else
	{
		LOGWARNING(
			"OpenGL | %s: %s source: %s id %u: %s\n", debugSeverity.c_str(), debugType.c_str(), debugSource.c_str(), id, message);
	}
}

} // anonymous namespace

// static
std::unique_ptr<IDevice> CDevice::Create(SDL_Window* window, const bool arb)
{
	std::unique_ptr<CDevice> device(new CDevice());

	if (window)
	{
		// According to https://wiki.libsdl.org/SDL_CreateWindow we don't need to
		// call SDL_GL_LoadLibrary if we have a window with SDL_WINDOW_OPENGL,
		// because it'll be called internally for the first created window.
		device->m_Window = window;
		device->m_Context = SDL_GL_CreateContext(device->m_Window);
		if (!device->m_Context)
		{
			LOGERROR("SDL_GL_CreateContext failed: '%s'", SDL_GetError());
			return nullptr;
		}
#if OS_WIN
		ogl_Init(SDL_GL_GetProcAddress, wutil_GetAppHDC());
#elif (defined(SDL_VIDEO_DRIVER_X11) || defined(SDL_VIDEO_DRIVER_WAYLAND)) && !CONFIG2_GLES
		SDL_SysWMinfo wminfo;
		// The info structure must be initialized with the SDL version.
		SDL_VERSION(&wminfo.version);
		if (!SDL_GetWindowWMInfo(window, &wminfo))
		{
			LOGERROR("Failed to query SDL WM info: %s", SDL_GetError());
			return nullptr;
		}
		switch (wminfo.subsystem)
		{
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
		case SDL_SYSWM_WAYLAND:
			// TODO: maybe we need to load X11 functions
			// dynamically as well.
			ogl_Init(SDL_GL_GetProcAddress,
				GetWaylandDisplay(device->m_Window),
				static_cast<int>(wminfo.subsystem));
			break;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
		case SDL_SYSWM_X11:
			ogl_Init(SDL_GL_GetProcAddress,
				GetX11Display(device->m_Window),
				static_cast<int>(wminfo.subsystem));
			break;
#endif
		default:
			ogl_Init(SDL_GL_GetProcAddress, nullptr,
				static_cast<int>(wminfo.subsystem));
			break;
		}
#else
		ogl_Init(SDL_GL_GetProcAddress);
#endif
	}
	else
	{
#if OS_WIN
		ogl_Init(SDL_GL_GetProcAddress, wutil_GetAppHDC());
#elif (defined(SDL_VIDEO_DRIVER_X11) || defined(SDL_VIDEO_DRIVER_WAYLAND)) && !CONFIG2_GLES
		bool initialized = false;
		// Currently we don't have access to the backend type without
		// the window. So we use hack to detect X11.
#if defined(SDL_VIDEO_DRIVER_X11)
		Display* display = XOpenDisplay(NULL);
		if (display)
		{
			ogl_Init(SDL_GL_GetProcAddress, display, static_cast<int>(SDL_SYSWM_X11));
			initialized = true;
		}
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
		if (!initialized)
		{
			// glad will find default EGLDisplay internally.
			ogl_Init(SDL_GL_GetProcAddress, nullptr, static_cast<int>(SDL_SYSWM_WAYLAND));
			initialized = true;
		}
#endif
		if (!initialized)
		{
			LOGERROR("Can't initialize GL");
			return nullptr;
		}
#else
		ogl_Init(SDL_GL_GetProcAddress);
#endif

#if OS_WIN || defined(SDL_VIDEO_DRIVER_X11) && !CONFIG2_GLES
		// Hack to stop things looking very ugly when scrolling in Atlas.
		ogl_SetVsyncEnabled(true);
#endif
	}

	// If we don't have GL2.0 then we don't have GLSL in core.
	if (!arb && !ogl_HaveVersion(2, 0))
		return nullptr;

	if ((ogl_HaveExtensions(0, "GL_ARB_vertex_program", "GL_ARB_fragment_program", nullptr) // ARB
		&& !ogl_HaveVersion(2, 0)) // GLSL
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

	device->m_ARB = arb;

	device->m_Name = GetNameImpl();
	device->m_Version = GetVersionImpl();
	device->m_DriverInformation = GetDriverInformationImpl();
	device->m_Extensions = GetExtensionsImpl();

	// Set packing parameters for uploading and downloading data.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_TEXTURE_2D);

	if (arb)
	{
#if !CONFIG2_GLES
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
#endif
	}

	device->m_Backbuffer = CFramebuffer::CreateBackbuffer(device.get());

	Capabilities& capabilities = device->m_Capabilities;
	capabilities.ARBShaders = !ogl_HaveExtensions(0, "GL_ARB_vertex_program", "GL_ARB_fragment_program", nullptr);
	if (capabilities.ARBShaders)
		capabilities.ARBShadersShadow = ogl_HaveExtension("GL_ARB_fragment_program_shadow");
	capabilities.computeShaders = ogl_HaveVersion(4, 3) || ogl_HaveExtension("GL_ARB_compute_shader");
#if CONFIG2_GLES
	// Some GLES implementations have GL_EXT_texture_compression_dxt1
	// but that only supports DXT1 so we can't use it.
	capabilities.S3TC = ogl_HaveExtensions(0, "GL_EXT_texture_compression_s3tc", nullptr) == 0;
#else
	// Note: we don't bother checking for GL_S3_s3tc - it is incompatible
	// and irrelevant (was never widespread).
	capabilities.S3TC = ogl_HaveExtensions(0, "GL_ARB_texture_compression", "GL_EXT_texture_compression_s3tc", nullptr) == 0;
#endif
#if CONFIG2_GLES
	capabilities.multisampling = false;
	capabilities.maxSampleCount = 1;
#else
	capabilities.multisampling =
		ogl_HaveVersion(3, 3) &&
		ogl_HaveExtension("GL_ARB_multisample") &&
		ogl_HaveExtension("GL_ARB_texture_multisample");
	if (capabilities.multisampling)
	{
		// By default GL_MULTISAMPLE should be enabled, but enable it for buggy drivers.
		glEnable(GL_MULTISAMPLE);
		GLint maxSamples = 1;
		glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
		capabilities.maxSampleCount = maxSamples;
	}
#endif
	capabilities.anisotropicFiltering = ogl_HaveExtension("GL_EXT_texture_filter_anisotropic");
	if (capabilities.anisotropicFiltering)
	{
		GLfloat maxAnisotropy = 1.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
		capabilities.maxAnisotropy = maxAnisotropy;
	}
	GLint maxTextureSize = 1024;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	capabilities.maxTextureSize = maxTextureSize;

#if CONFIG2_GLES
	const bool isDebugInCore = ogl_HaveVersion(3, 2);
#else
	const bool isDebugInCore = ogl_HaveVersion(4, 3);
#endif
	const bool hasDebug = isDebugInCore || ogl_HaveExtension("GL_KHR_debug");
	if (hasDebug)
	{
#ifdef NDEBUG
		bool enableDebugMessages = false;
		CFG_GET_VAL("renderer.backend.debugmessages", enableDebugMessages);
		capabilities.debugLabels = false;
		CFG_GET_VAL("renderer.backend.debuglabels", capabilities.debugLabels);
		capabilities.debugScopedLabels = false;
		CFG_GET_VAL("renderer.backend.debugscopedlabels", capabilities.debugScopedLabels);
#else
		const bool enableDebugMessages = true;
		capabilities.debugLabels = true;
		capabilities.debugScopedLabels = true;
#endif
		if (enableDebugMessages)
		{
			glEnable(GL_DEBUG_OUTPUT);
#if !CONFIG2_GLES
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#else
#warning GLES without GL_DEBUG_OUTPUT_SYNCHRONOUS might call the callback from different threads which might be unsafe.
#endif
			glDebugMessageCallback(OnDebugMessage, nullptr);

			// Filter out our own debug group messages
			const GLuint id = 0x0AD;
			glDebugMessageControl(
				GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 1, &id, GL_FALSE);
			glDebugMessageControl(
				GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 1, &id, GL_FALSE);
		}
	}

#if CONFIG2_GLES
	capabilities.instancing = false;
#else
	capabilities.instancing =
		!device->m_ARB &&
		(ogl_HaveVersion(3, 3) ||
		(ogl_HaveExtension("GL_ARB_draw_instanced") &&
		ogl_HaveExtension("GL_ARB_instanced_arrays")));
#endif

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

	Script::SetProperty(rq, settings, "name", m_ARB ? "glarb" : "gl");

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
	glGetQueryivARB(GL_##target, GL_##pname, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_" #target ".GL_" #pname, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_" #target ".GL_" #pname, i); \
	} while (false)

#define VERTEXPROGRAM(id) do { \
	GLint i = -1; \
	glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_##id, &i); \
	if (ogl_SquelchError(GL_INVALID_ENUM)) \
		Script::SetProperty(rq, settings, "GL_VERTEX_PROGRAM_ARB.GL_" #id, errstr); \
	else \
		Script::SetProperty(rq, settings, "GL_VERTEX_PROGRAM_ARB.GL_" #id, i); \
	} while (false)

#define FRAGMENTPROGRAM(id) do { \
	GLint i = -1; \
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_##id, &i); \
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
		INTEGER(MIN_PROGRAM_TEXEL_OFFSET_EXT); // no _EXT version of these in glext.h
		INTEGER(MAX_PROGRAM_TEXEL_OFFSET_EXT);
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

	if (m_ARB)
	{
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
	if (glXQueryCurrentRendererIntegerMESA(id, &i)) \
		Script::SetProperty(rq, settings, #id, i); \
	} while (false)

#define GLXQCR_INTEGER2(id) do { \
	unsigned int i[2] = { UINT_MAX, UINT_MAX }; \
	if (glXQueryCurrentRendererIntegerMESA(id, i)) { \
		Script::SetProperty(rq, settings, #id "[0]", i[0]); \
		Script::SetProperty(rq, settings, #id "[1]", i[1]); \
	} \
	} while (false)

#define GLXQCR_INTEGER3(id) do { \
	unsigned int i[3] = { UINT_MAX, UINT_MAX, UINT_MAX }; \
	if (glXQueryCurrentRendererIntegerMESA(id, i)) { \
		Script::SetProperty(rq, settings, #id "[0]", i[0]); \
		Script::SetProperty(rq, settings, #id "[1]", i[1]); \
		Script::SetProperty(rq, settings, #id "[2]", i[2]); \
	} \
	} while (false)

#define GLXQCR_STRING(id) do { \
	const char* str = glXQueryCurrentRendererStringMESA(id); \
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

		if (strstr(glxexts, "GLX_MESA_query_renderer") && glXQueryCurrentRendererIntegerMESA && glXQueryCurrentRendererStringMESA)
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

std::unique_ptr<IDeviceCommandContext> CDevice::CreateCommandContext()
{
	std::unique_ptr<CDeviceCommandContext> commandContet = CDeviceCommandContext::Create(this);
	m_ActiveCommandContext = commandContet.get();
	return commandContet;
}

std::unique_ptr<ITexture> CDevice::CreateTexture(const char* name, const ITexture::Type type,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CTexture::Create(this, name, type,
		format, width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<ITexture> CDevice::CreateTexture2D(const char* name,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CreateTexture(name, CTexture::Type::TEXTURE_2D,
		format, width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char* name, ITexture* colorAttachment,
	ITexture* depthStencilAttachment)
{
	return CreateFramebuffer(name, colorAttachment, depthStencilAttachment, CColor(0.0f, 0.0f, 0.0f, 0.0f));
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char* name, ITexture* colorAttachment,
	ITexture* depthStencilAttachment, const CColor& clearColor)
{
	return CFramebuffer::Create(
		this, name, colorAttachment->As<CTexture>(), depthStencilAttachment->As<CTexture>(), clearColor);
}

std::unique_ptr<IBuffer> CDevice::CreateBuffer(
	const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic)
{
	return CBuffer::Create(this, name, type, size, dynamic);
}

std::unique_ptr<IShaderProgram> CDevice::CreateShaderProgram(
	const CStr& name, const CShaderDefines& defines)
{
	return CShaderProgram::Create(this, name, defines);
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

bool CDevice::IsTextureFormatSupported(const Format format) const
{
	bool supported = false;
	switch (format)
	{
	case Format::UNDEFINED:
		break;

	case Format::R8G8B8_UNORM: FALLTHROUGH;
	case Format::R8G8B8A8_UNORM: FALLTHROUGH;
	case Format::A8_UNORM: FALLTHROUGH;
	case Format::L8_UNORM:
		supported = true;
		break;

	case Format::R32_SFLOAT: FALLTHROUGH;
	case Format::R32G32_SFLOAT: FALLTHROUGH;
	case Format::R32G32B32_SFLOAT: FALLTHROUGH;
	case Format::R32G32B32A32_SFLOAT:
		break;

	case Format::D16: FALLTHROUGH;
	case Format::D24: FALLTHROUGH;
	case Format::D32:
		supported = true;
		break;
	case Format::D24_S8:
#if !CONFIG2_GLES
		supported = true;
#endif
		break;

	case Format::BC1_RGB_UNORM: FALLTHROUGH;
	case Format::BC1_RGBA_UNORM: FALLTHROUGH;
	case Format::BC2_UNORM: FALLTHROUGH;
	case Format::BC3_UNORM:
		supported = m_Capabilities.S3TC;
		break;

	default:
		break;
	}
	return supported;
}

bool CDevice::IsFramebufferFormatSupported(const Format format) const
{
	bool supported = false;
	switch (format)
	{
	case Format::UNDEFINED:
		break;
#if !CONFIG2_GLES
	case Format::R8_UNORM:
		supported = ogl_HaveVersion(3, 0);
		break;
#endif
	case Format::R8G8B8A8_UNORM:
		supported = true;
		break;
	default:
		break;
	}
	return supported;
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
