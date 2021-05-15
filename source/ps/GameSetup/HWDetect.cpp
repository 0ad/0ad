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

#include "lib/ogl.h"
#include "lib/svn_revision.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/posix/posix_utsname.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/gfx.h"
#include "lib/sysdep/numa.h"
#include "lib/sysdep/os_cpu.h"
#if ARCH_X86_X64
# include "lib/sysdep/arch/x86_x64/topology.h"
#endif
#if CONFIG2_AUDIO
#include "soundmanager/SoundManager.h"
#endif
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Profile.h"
#include "ps/scripting/JSInterface_ConfigDB.h"
#include "ps/scripting/JSInterface_Debug.h"
#include "ps/UserReport.h"
#include "ps/VideoMode.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/JSON.h"

#if OS_LINUX
#include <fstream>
#endif

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

static void ReportSDL(const ScriptRequest& rq, JS::HandleValue settings);
static void ReportGLLimits(const ScriptRequest& rq, JS::HandleValue settings);

void SetDisableAudio(bool disabled)
{
	g_DisableAudio = disabled;
}

void RunHardwareDetection()
{
	TIMER(L"RunHardwareDetection");

	ScriptInterface scriptInterface("Engine", "HWDetect", g_ScriptContext);

	ScriptRequest rq(scriptInterface);

	JSI_Debug::RegisterScriptFunctions(scriptInterface); // Engine.DisplayErrorDialog
	JSI_ConfigDB::RegisterScriptFunctions(scriptInterface);

	ScriptFunction::Register<SetDisableAudio>(rq, "SetDisableAudio");

	// Load the detection script:

	const wchar_t* scriptName = L"hwdetect/hwdetect.js";
	CVFSFile file;
	if (file.Load(g_VFS, scriptName) != PSRETURN_OK)
	{
		LOGERROR("Failed to load hardware detection script");
		return;
	}

	std::string code = file.DecodeUTF8(); // assume it's UTF-8
	scriptInterface.LoadScript(scriptName, code);

	// Collect all the settings we'll pass to the script:
	// (We'll use this same data for the opt-in online reporting system, so it
	// includes some fields that aren't directly useful for the hwdetect script)

	JS::RootedValue settings(rq.cx);
	Script::CreateObject(rq, &settings);

	Script::SetProperty(rq, settings, "os_unix", OS_UNIX);
	Script::SetProperty(rq, settings, "os_bsd", OS_BSD);
	Script::SetProperty(rq, settings, "os_linux", OS_LINUX);
	Script::SetProperty(rq, settings, "os_android", OS_ANDROID);
	Script::SetProperty(rq, settings, "os_macosx", OS_MACOSX);
	Script::SetProperty(rq, settings, "os_win", OS_WIN);

	Script::SetProperty(rq, settings, "arch_ia32", ARCH_IA32);
	Script::SetProperty(rq, settings, "arch_amd64", ARCH_AMD64);
	Script::SetProperty(rq, settings, "arch_arm", ARCH_ARM);
	Script::SetProperty(rq, settings, "arch_aarch64", ARCH_AARCH64);
	Script::SetProperty(rq, settings, "arch_e2k", ARCH_E2K);
	Script::SetProperty(rq, settings, "arch_ppc64", ARCH_PPC64);

#ifdef NDEBUG
	Script::SetProperty(rq, settings, "build_debug", 0);
#else
	Script::SetProperty(rq, settings, "build_debug", 1);
#endif
	Script::SetProperty(rq, settings, "build_opengles", CONFIG2_GLES);

	Script::SetProperty(rq, settings, "build_datetime", std::string(__DATE__ " " __TIME__));
	Script::SetProperty(rq, settings, "build_revision", std::wstring(svn_revision));

	Script::SetProperty(rq, settings, "build_msc", (int)MSC_VERSION);
	Script::SetProperty(rq, settings, "build_icc", (int)ICC_VERSION);
	Script::SetProperty(rq, settings, "build_gcc", (int)GCC_VERSION);
	Script::SetProperty(rq, settings, "build_clang", (int)CLANG_VERSION);

	Script::SetProperty(rq, settings, "gfx_card", gfx::CardName());
	Script::SetProperty(rq, settings, "gfx_drv_ver", gfx::DriverInfo());
#if CONFIG2_AUDIO
	if (g_SoundManager)
	{
		Script::SetProperty(rq, settings, "snd_card", g_SoundManager->GetSoundCardNames());
		Script::SetProperty(rq, settings, "snd_drv_ver", g_SoundManager->GetOpenALVersion());
	}
#endif
	ReportSDL(scriptInterface, settings);

	ReportGLLimits(scriptInterface, settings);

	Script::SetProperty(rq, settings, "video_desktop_xres", g_VideoMode.GetDesktopXRes());
	Script::SetProperty(rq, settings, "video_desktop_yres", g_VideoMode.GetDesktopYRes());
	Script::SetProperty(rq, settings, "video_desktop_bpp", g_VideoMode.GetDesktopBPP());
	Script::SetProperty(rq, settings, "video_desktop_freq", g_VideoMode.GetDesktopFreq());

	struct utsname un;
	uname(&un);
	Script::SetProperty(rq, settings, "uname_sysname", std::string(un.sysname));
	Script::SetProperty(rq, settings, "uname_release", std::string(un.release));
	Script::SetProperty(rq, settings, "uname_version", std::string(un.version));
	Script::SetProperty(rq, settings, "uname_machine", std::string(un.machine));

#if OS_LINUX
	{
		std::ifstream ifs("/etc/lsb-release");
		if (ifs.good())
		{
			std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
			Script::SetProperty(rq, settings, "linux_release", str);
		}
	}
#endif

	Script::SetProperty(rq, settings, "cpu_identifier", std::string(cpu_IdentifierString()));
	Script::SetProperty(rq, settings, "cpu_frequency", os_cpu_ClockFrequency());
	Script::SetProperty(rq, settings, "cpu_pagesize", (u32)os_cpu_PageSize());
	Script::SetProperty(rq, settings, "cpu_largepagesize", (u32)os_cpu_LargePageSize());
	Script::SetProperty(rq, settings, "cpu_numprocs", (u32)os_cpu_NumProcessors());
#if ARCH_X86_X64
	Script::SetProperty(rq, settings, "cpu_numpackages", (u32)topology::NumPackages());
	Script::SetProperty(rq, settings, "cpu_coresperpackage", (u32)topology::CoresPerPackage());
	Script::SetProperty(rq, settings, "cpu_logicalpercore", (u32)topology::LogicalPerCore());
#endif

	Script::SetProperty(rq, settings, "numa_numnodes", (u32)numa_NumNodes());
	Script::SetProperty(rq, settings, "numa_factor", numa_Factor());
	Script::SetProperty(rq, settings, "numa_interleaved", numa_IsMemoryInterleaved());

	Script::SetProperty(rq, settings, "ram_total", (u32)os_cpu_MemorySize());
	Script::SetProperty(rq, settings, "ram_total_os", (u32)os_cpu_QueryMemorySize());

#if ARCH_X86_X64
	Script::SetProperty(rq, settings, "x86_vendor", (u32)x86_x64::Vendor());
	Script::SetProperty(rq, settings, "x86_model", (u32)x86_x64::Model());
	Script::SetProperty(rq, settings, "x86_family", (u32)x86_x64::Family());

	u32 caps0, caps1, caps2, caps3;
	x86_x64::GetCapBits(&caps0, &caps1, &caps2, &caps3);
	Script::SetProperty(rq, settings, "x86_caps[0]", caps0);
	Script::SetProperty(rq, settings, "x86_caps[1]", caps1);
	Script::SetProperty(rq, settings, "x86_caps[2]", caps2);
	Script::SetProperty(rq, settings, "x86_caps[3]", caps3);
#endif

	Script::SetProperty(rq, settings, "timer_resolution", timer_Resolution());

	// The version should be increased for every meaningful change.
	const int reportVersion = 14;

	// Send the same data to the reporting system
	g_UserReporter.SubmitReport(
		"hwdetect",
		reportVersion,
		Script::StringifyJSON(rq, &settings, false),
		Script::StringifyJSON(rq, &settings, true));

	// Run the detection script:
	JS::RootedValue global(rq.cx, rq.globalValue());
	ScriptFunction::CallVoid(rq, global, "RunHardwareDetection", settings);
}

static void ReportSDL(const ScriptRequest& rq, JS::HandleValue settings)
{
	SDL_version build, runtime;
	SDL_VERSION(&build);

	char version[16];
	snprintf(version, ARRAY_SIZE(version), "%d.%d.%d", build.major, build.minor, build.patch);
	Script::SetProperty(rq, settings, "sdl_build_version", version);

	SDL_GetVersion(&runtime);
	snprintf(version, ARRAY_SIZE(version), "%d.%d.%d", runtime.major, runtime.minor, runtime.patch);
	Script::SetProperty(rq, settings, "sdl_runtime_version", version);

	// This is null in atlas (and further the call triggers an assertion).
	const char* backend = g_VideoMode.GetWindow() ? GetSDLSubsystem(g_VideoMode.GetWindow()) : "none";
	Script::SetProperty(rq, settings, "sdl_video_backend", backend ? backend : "unknown");
}

static void ReportGLLimits(const ScriptRequest& rq, JS::HandleValue settings)
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
	INTEGER(MAX_LIGHTS);
	INTEGER(MAX_CLIP_PLANES);
	// Skip MAX_COLOR_MATRIX_STACK_DEPTH (only in imaging subset)
	INTEGER(MAX_MODELVIEW_STACK_DEPTH);
	INTEGER(MAX_PROJECTION_STACK_DEPTH);
	INTEGER(MAX_TEXTURE_STACK_DEPTH);
#endif
	INTEGER(SUBPIXEL_BITS);
#if !CONFIG2_GLES
	INTEGER(MAX_3D_TEXTURE_SIZE);
#endif
	INTEGER(MAX_TEXTURE_SIZE);
	INTEGER(MAX_CUBE_MAP_TEXTURE_SIZE);
#if !CONFIG2_GLES
	INTEGER(MAX_PIXEL_MAP_TABLE);
	INTEGER(MAX_NAME_STACK_DEPTH);
	INTEGER(MAX_LIST_NESTING);
	INTEGER(MAX_EVAL_ORDER);
#endif
	INTEGER2(MAX_VIEWPORT_DIMS);
#if !CONFIG2_GLES
	INTEGER(MAX_ATTRIB_STACK_DEPTH);
	INTEGER(MAX_CLIENT_ATTRIB_STACK_DEPTH);
	INTEGER(AUX_BUFFERS);
	BOOL(RGBA_MODE);
	BOOL(INDEX_MODE);
	BOOL(DOUBLEBUFFER);
	BOOL(STEREO);
#endif
	FLOAT2(ALIASED_POINT_SIZE_RANGE);
#if !CONFIG2_GLES
	FLOAT2(SMOOTH_POINT_SIZE_RANGE);
	FLOAT(SMOOTH_POINT_SIZE_GRANULARITY);
#endif
	FLOAT2(ALIASED_LINE_WIDTH_RANGE);
#if !CONFIG2_GLES
	FLOAT2(SMOOTH_LINE_WIDTH_RANGE);
	FLOAT(SMOOTH_LINE_WIDTH_GRANULARITY);
	// Skip MAX_CONVOLUTION_WIDTH, MAX_CONVOLUTION_HEIGHT (only in imaging subset)
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
	INTEGER(ACCUM_RED_BITS);
	INTEGER(ACCUM_GREEN_BITS);
	INTEGER(ACCUM_BLUE_BITS);
	INTEGER(ACCUM_ALPHA_BITS);
#endif

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
	const int ret = SDL_GetWindowWMInfo(g_VideoMode.GetWindow(), &wminfo);
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
