/* Copyright (C) 2011 Wildfire Games.
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

#include "scripting/ScriptingHost.h"
#include "scriptinterface/ScriptInterface.h"

#include "lib/ogl.h"
#include "lib/svn_revision.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/gfx.h"
#include "lib/sysdep/numa.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/snd.h"
#if ARCH_X86_X64
# include "lib/sysdep/arch/x86_x64/topology.h"
#endif
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/UserReport.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/Config.h"

void SetDisableAudio(void* UNUSED(cbdata), bool disabled)
{
	g_DisableAudio = disabled;
}

static void ReportGLLimits(ScriptInterface& scriptInterface, CScriptValRooted settings);

void RunHardwareDetection()
{
	TIMER(L"RunHardwareDetection");

	ScriptInterface& scriptInterface = g_ScriptingHost.GetScriptInterface();

	scriptInterface.RegisterFunction<void, bool, &SetDisableAudio>("SetDisableAudio");

	// Load the detection script:

	const wchar_t* scriptName = L"hwdetect/hwdetect.js";
	CVFSFile file;
	if (file.Load(g_VFS, scriptName) != PSRETURN_OK)
	{
		LOGERROR(L"Failed to load hardware detection script");
		return;
	}

	LibError err; // ignore encoding errors
	std::wstring code = wstring_from_utf8(file.GetAsString(), &err);

	scriptInterface.LoadScript(scriptName, code);

	// Collect all the settings we'll pass to the script:
	// (We'll use this same data for the opt-in online reporting system, so it
	// includes some fields that aren't directly useful for the hwdetect script)

	CScriptValRooted settings;
	scriptInterface.Eval("({})", settings);

	scriptInterface.SetProperty(settings.get(), "os_unix", OS_UNIX);
	scriptInterface.SetProperty(settings.get(), "os_linux", OS_LINUX);
	scriptInterface.SetProperty(settings.get(), "os_macosx", OS_MACOSX);
	scriptInterface.SetProperty(settings.get(), "os_win", OS_WIN);

	scriptInterface.SetProperty(settings.get(), "arch_ia32", ARCH_IA32);
	scriptInterface.SetProperty(settings.get(), "arch_amd64", ARCH_AMD64);

#ifdef NDEBUG
	scriptInterface.SetProperty(settings.get(), "build_debug", 0);
#else
	scriptInterface.SetProperty(settings.get(), "build_debug", 1);
#endif
	scriptInterface.SetProperty(settings.get(), "build_datetime", std::string(__DATE__ " " __TIME__));
	scriptInterface.SetProperty(settings.get(), "build_revision", std::wstring(svn_revision));
	scriptInterface.SetProperty(settings.get(), "build_msc", (int)MSC_VERSION);
	scriptInterface.SetProperty(settings.get(), "build_icc", (int)ICC_VERSION);
	scriptInterface.SetProperty(settings.get(), "build_gcc", (int)GCC_VERSION);

	scriptInterface.SetProperty(settings.get(), "gfx_card", std::wstring(gfx_card));
	scriptInterface.SetProperty(settings.get(), "gfx_drv_ver", std::wstring(gfx_drv_ver));
	scriptInterface.SetProperty(settings.get(), "gfx_mem", gfx_mem);

	scriptInterface.SetProperty(settings.get(), "snd_card", std::wstring(snd_card));
	scriptInterface.SetProperty(settings.get(), "snd_drv_ver", std::wstring(snd_drv_ver));

	ReportGLLimits(scriptInterface, settings);

	scriptInterface.SetProperty(settings.get(), "video_xres", g_VideoMode.GetXRes());
	scriptInterface.SetProperty(settings.get(), "video_yres", g_VideoMode.GetYRes());
	scriptInterface.SetProperty(settings.get(), "video_bpp", g_VideoMode.GetBPP());

	struct utsname un;
	uname(&un);
	scriptInterface.SetProperty(settings.get(), "uname_sysname", std::string(un.sysname));
	scriptInterface.SetProperty(settings.get(), "uname_release", std::string(un.release));
	scriptInterface.SetProperty(settings.get(), "uname_version", std::string(un.version));
	scriptInterface.SetProperty(settings.get(), "uname_machine", std::string(un.machine));

	scriptInterface.SetProperty(settings.get(), "cpu_identifier", std::string(cpu_IdentifierString()));
	scriptInterface.SetProperty(settings.get(), "cpu_frequency", os_cpu_ClockFrequency());
	scriptInterface.SetProperty(settings.get(), "cpu_pagesize", os_cpu_PageSize());
	scriptInterface.SetProperty(settings.get(), "cpu_largepagesize", os_cpu_LargePageSize());
	scriptInterface.SetProperty(settings.get(), "cpu_numprocs", os_cpu_NumProcessors());
#if ARCH_X86_X64
	scriptInterface.SetProperty(settings.get(), "cpu_numpackages", cpu_topology_NumPackages());
	scriptInterface.SetProperty(settings.get(), "cpu_coresperpackage", cpu_topology_CoresPerPackage());
	scriptInterface.SetProperty(settings.get(), "cpu_logicalpercore", cpu_topology_LogicalPerCore());
	scriptInterface.SetProperty(settings.get(), "cpu_numcaches", cache_topology_NumCaches());
#endif

	scriptInterface.SetProperty(settings.get(), "numa_numnodes", numa_NumNodes());
	scriptInterface.SetProperty(settings.get(), "numa_factor", numa_Factor());
	scriptInterface.SetProperty(settings.get(), "numa_interleaved", numa_IsMemoryInterleaved());

	scriptInterface.SetProperty(settings.get(), "ram_total", (int)os_cpu_MemorySize());
	scriptInterface.SetProperty(settings.get(), "ram_free", (int)os_cpu_MemoryAvailable());

#if ARCH_X86_X64
	scriptInterface.SetProperty(settings.get(), "x86_vendor", (int)x86_x64_Vendor());
	scriptInterface.SetProperty(settings.get(), "x86_model", (int)x86_x64_Model());
	scriptInterface.SetProperty(settings.get(), "x86_family", (int)x86_x64_Family());
	scriptInterface.SetProperty(settings.get(), "x86_l1line", (int)x86_x64_L1CacheLineSize());
	scriptInterface.SetProperty(settings.get(), "x86_l2line", (int)x86_x64_L2CacheLineSize());
	u32 caps0, caps1, caps2, caps3;
	x86_x64_caps(&caps0, &caps1, &caps2, &caps3);
	scriptInterface.SetProperty(settings.get(), "x86_caps[0]", caps0);
	scriptInterface.SetProperty(settings.get(), "x86_caps[1]", caps1);
	scriptInterface.SetProperty(settings.get(), "x86_caps[2]", caps2);
	scriptInterface.SetProperty(settings.get(), "x86_caps[3]", caps3);
#endif

	// Send the same data to the reporting system
	g_UserReporter.SubmitReport("hwdetect", 3, scriptInterface.StringifyJSON(settings.get(), false));

	// Run the detection script:

	scriptInterface.CallFunctionVoid(scriptInterface.GetGlobalObject(), "RunHardwareDetection", settings);
}


static void ReportGLLimits(ScriptInterface& scriptInterface, CScriptValRooted settings)
{
#define INTEGER(id) do { \
	GLint i = -1; \
	glGetIntegerv(GL_##id, &i); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id, i); \
	} while (false)

#define INTEGER2(id) do { \
	GLint i[2] = { -1, -1 }; \
	glGetIntegerv(GL_##id, i); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id "[0]", i[0]); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id "[1]", i[1]); \
	} while (false)

#define FLOAT(id) do { \
	GLfloat f = std::numeric_limits<GLfloat>::quiet_NaN(); \
	glGetFloatv(GL_##id, &f); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id, f); \
	} while (false)

#define FLOAT2(id) do { \
	GLfloat f[2] = { std::numeric_limits<GLfloat>::quiet_NaN(), std::numeric_limits<GLfloat>::quiet_NaN() }; \
	glGetFloatv(GL_##id, f); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id "[0]", f[0]); \
	scriptInterface.SetProperty(settings.get(), "GL_" #id "[1]", f[1]); \
	} while (false)

#define STRING(id) do { \
	const char* c = (const char*)glGetString(GL_##id); \
	ogl_WarnIfError(); \
	if (!c) c = ""; \
	scriptInterface.SetProperty(settings.get(), "GL_" #id, std::string(c)); \
	}  while (false)

#define VERTEXPROGRAM(id) do { \
	GLint i = -1; \
	pglGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_##id, &i); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_VERTEX_PROGRAM_ARB.GL_" #id, i); \
	} while (false)

#define FRAGMENTPROGRAM(id) do { \
	GLint i = -1; \
	pglGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_##id, &i); \
	ogl_WarnIfError(); \
	scriptInterface.SetProperty(settings.get(), "GL_FRAGMENT_PROGRAM_ARB.GL_" #id, i); \
	} while (false)

#define BOOL(id) INTEGER(id)

	// Core OpenGL 1.3:
	// (We don't bother checking extension strings for anything older than 1.3;
	// it'll just produce harmless warnings)
	STRING(VERSION);
	STRING(VENDOR);
	STRING(RENDERER);
	STRING(EXTENSIONS);
	INTEGER(MAX_LIGHTS);
	INTEGER(MAX_CLIP_PLANES);
	if (ogl_HaveExtension("GL_ARB_imaging")) // only in imaging subset
		INTEGER(MAX_COLOR_MATRIX_STACK_DEPTH);
	INTEGER(MAX_MODELVIEW_STACK_DEPTH);
	INTEGER(MAX_PROJECTION_STACK_DEPTH);
	INTEGER(MAX_TEXTURE_STACK_DEPTH);
	INTEGER(SUBPIXEL_BITS);
	INTEGER(MAX_3D_TEXTURE_SIZE);
	INTEGER(MAX_TEXTURE_SIZE);
	INTEGER(MAX_CUBE_MAP_TEXTURE_SIZE);
	INTEGER(MAX_PIXEL_MAP_TABLE);
	INTEGER(MAX_NAME_STACK_DEPTH);
	INTEGER(MAX_LIST_NESTING);
	INTEGER(MAX_EVAL_ORDER);
	INTEGER2(MAX_VIEWPORT_DIMS);
	INTEGER(MAX_ATTRIB_STACK_DEPTH);
	INTEGER(MAX_CLIENT_ATTRIB_STACK_DEPTH);
	INTEGER(AUX_BUFFERS);
	BOOL(RGBA_MODE);
	BOOL(INDEX_MODE);
	BOOL(DOUBLEBUFFER);
	BOOL(STEREO);
	FLOAT2(ALIASED_POINT_SIZE_RANGE);
	FLOAT2(SMOOTH_POINT_SIZE_RANGE);
	FLOAT(SMOOTH_POINT_SIZE_GRANULARITY);
	FLOAT2(ALIASED_LINE_WIDTH_RANGE);
	FLOAT2(SMOOTH_LINE_WIDTH_RANGE);
	FLOAT(SMOOTH_LINE_WIDTH_GRANULARITY);
	// Skip MAX_CONVOLUTION_WIDTH, MAX_CONVOLUTION_HEIGHT since they'd need GetConvolutionParameteriv
	INTEGER(MAX_ELEMENTS_INDICES);
	INTEGER(MAX_ELEMENTS_VERTICES);
	INTEGER(MAX_TEXTURE_UNITS);
	INTEGER(SAMPLE_BUFFERS);
	INTEGER(SAMPLES);
	// TODO: compressed texture formats
	INTEGER(RED_BITS);
	INTEGER(GREEN_BITS);
	INTEGER(BLUE_BITS);
	INTEGER(ALPHA_BITS);
	INTEGER(INDEX_BITS);
	INTEGER(DEPTH_BITS);
	INTEGER(STENCIL_BITS);
	INTEGER(ACCUM_RED_BITS);
	INTEGER(ACCUM_GREEN_BITS);
	INTEGER(ACCUM_BLUE_BITS);
	INTEGER(ACCUM_ALPHA_BITS);

	// Core OpenGL 2.0 (treated as extensions):

	if (ogl_HaveExtension("GL_EXT_texture_lod_bias"))
		FLOAT(MAX_TEXTURE_LOD_BIAS_EXT);

	// Skip GL_ARB_occlusion_query's QUERY_COUNTER_BITS_ARB since it'd need GetQueryiv

	if (ogl_HaveExtension("GL_ARB_shading_language_100"))
		STRING(SHADING_LANGUAGE_VERSION_ARB);

	if (ogl_HaveExtension("GL_ARB_vertex_shader"))
	{
		INTEGER(MAX_VERTEX_ATTRIBS_ARB);
		INTEGER(MAX_VERTEX_UNIFORM_COMPONENTS_ARB);
		INTEGER(MAX_VARYING_FLOATS_ARB);
		INTEGER(MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB);
		INTEGER(MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_fragment_shader"))
		INTEGER(MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB);

	if (ogl_HaveExtension("GL_ARB_vertex_shader") || ogl_HaveExtension("GL_ARB_fragment_shader") ||
		ogl_HaveExtension("GL_ARB_vertex_program") || ogl_HaveExtension("GL_ARB_fragment_program"))
	{
		INTEGER(MAX_TEXTURE_IMAGE_UNITS_ARB);
		INTEGER(MAX_TEXTURE_COORDS_ARB);
	}

	if (ogl_HaveExtension("GL_ARB_draw_buffers"))
		INTEGER(MAX_DRAW_BUFFERS_ARB);

	// Core OpenGL 3.0:

	if (ogl_HaveExtension("GL_EXT_gpu_shader4"))
	{
		INTEGER(MIN_PROGRAM_TEXEL_OFFSET);
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
			// The spec seems to say these should be supported, but Mesa complains
			// about them so let's not bother
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
			FRAGMENTPROGRAM(MAX_PROGRAM_ADDRESS_REGISTERS_ARB);
			FRAGMENTPROGRAM(MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB);
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
}
