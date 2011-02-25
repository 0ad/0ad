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
#include "lib/res/graphics/ogl_tex.h"
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

static void ReportGLLimits(ScriptInterface& scriptInterface, CScriptValRooted settings);

#if ARCH_X86_X64
CScriptVal ConvertCaches(ScriptInterface& scriptInterface, const x86_x64_Caches* caches)
{
	CScriptVal ret;
	scriptInterface.Eval("[]", ret);
	for (size_t i = 0; i < caches->numLevels; ++i)
	{
		CScriptVal cache;
		scriptInterface.Eval("({})", cache);
		scriptInterface.SetProperty(cache.get(), "type", (u32)caches->levels[i].type);
		scriptInterface.SetProperty(cache.get(), "level", (u32)caches->levels[i].level);
		scriptInterface.SetProperty(cache.get(), "associativity", (u32)caches->levels[i].associativity);
		scriptInterface.SetProperty(cache.get(), "linesize", (u32)caches->levels[i].lineSize);
		scriptInterface.SetProperty(cache.get(), "sharedby", (u32)caches->levels[i].sharedBy);
		scriptInterface.SetProperty(cache.get(), "totalsize", (u32)caches->levels[i].totalSize);
		scriptInterface.SetPropertyInt(ret.get(), i, cache);
	}
	return ret;
}

CScriptVal ConvertTLBs(ScriptInterface& scriptInterface, const x86_x64_TLBs* tlbs)
{
	CScriptVal ret;
	scriptInterface.Eval("[]", ret);
	for (size_t i = 0; i < tlbs->numLevels; ++i)
	{
		CScriptVal tlb;
		scriptInterface.Eval("({})", tlb);
		scriptInterface.SetProperty(tlb.get(), "type", (u32)tlbs->levels[i].type);
		scriptInterface.SetProperty(tlb.get(), "level", (u32)tlbs->levels[i].level);
		scriptInterface.SetProperty(tlb.get(), "associativity", (u32)tlbs->levels[i].associativity);
		scriptInterface.SetProperty(tlb.get(), "pagesize", (u32)tlbs->levels[i].pageSize);
		scriptInterface.SetProperty(tlb.get(), "entries", (u32)tlbs->levels[i].entries);
		scriptInterface.SetPropertyInt(ret.get(), i, tlb);
	}
	return ret;
}
#endif



void SetDisableAudio(void* UNUSED(cbdata), bool disabled)
{
	g_DisableAudio = disabled;
}

void SetDisableS3TC(void* UNUSED(cbdata), bool disabled)
{
	ogl_tex_override(OGL_TEX_S3TC, disabled ? OGL_TEX_DISABLE : OGL_TEX_ENABLE);
}

void SetDisableShadows(void* UNUSED(cbdata), bool disabled)
{
	g_Shadows = !disabled;
}

void SetDisableFancyWater(void* UNUSED(cbdata), bool disabled)
{
	g_FancyWater = !disabled;
}

void RunHardwareDetection()
{
	TIMER(L"RunHardwareDetection");

	ScriptInterface& scriptInterface = g_ScriptingHost.GetScriptInterface();

	scriptInterface.RegisterFunction<void, bool, &SetDisableAudio>("SetDisableAudio");
	scriptInterface.RegisterFunction<void, bool, &SetDisableS3TC>("SetDisableS3TC");
	scriptInterface.RegisterFunction<void, bool, &SetDisableShadows>("SetDisableShadows");
	scriptInterface.RegisterFunction<void, bool, &SetDisableFancyWater>("SetDisableFancyWater");

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

#if OS_LINUX
	{
		std::ifstream ifs("/etc/lsb-release");
		if (ifs.good())
		{
			std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
			scriptInterface.SetProperty(settings.get(), "linux_release", str);
		}
	}
#endif

	scriptInterface.SetProperty(settings.get(), "cpu_identifier", std::string(cpu_IdentifierString()));
	scriptInterface.SetProperty(settings.get(), "cpu_frequency", os_cpu_ClockFrequency());
	scriptInterface.SetProperty(settings.get(), "cpu_pagesize", (u32)os_cpu_PageSize());
	scriptInterface.SetProperty(settings.get(), "cpu_largepagesize", (u32)os_cpu_LargePageSize());
	scriptInterface.SetProperty(settings.get(), "cpu_numprocs", (u32)os_cpu_NumProcessors());
#if ARCH_X86_X64
	scriptInterface.SetProperty(settings.get(), "cpu_numpackages", (u32)cpu_topology_NumPackages());
	scriptInterface.SetProperty(settings.get(), "cpu_coresperpackage", (u32)cpu_topology_CoresPerPackage());
	scriptInterface.SetProperty(settings.get(), "cpu_logicalpercore", (u32)cpu_topology_LogicalPerCore());
	scriptInterface.SetProperty(settings.get(), "cpu_numcaches", (u32)cache_topology_NumCaches());
#endif

	scriptInterface.SetProperty(settings.get(), "numa_numnodes", (u32)numa_NumNodes());
	scriptInterface.SetProperty(settings.get(), "numa_factor", numa_Factor());
	scriptInterface.SetProperty(settings.get(), "numa_interleaved", numa_IsMemoryInterleaved());

	scriptInterface.SetProperty(settings.get(), "ram_total", (u32)os_cpu_MemorySize());
	scriptInterface.SetProperty(settings.get(), "ram_free", (u32)os_cpu_MemoryAvailable());

#if ARCH_X86_X64
	scriptInterface.SetProperty(settings.get(), "x86_vendor", (u32)x86_x64_Vendor());
	scriptInterface.SetProperty(settings.get(), "x86_model", (u32)x86_x64_Model());
	scriptInterface.SetProperty(settings.get(), "x86_family", (u32)x86_x64_Family());

	u32 caps0, caps1, caps2, caps3;
	x86_x64_caps(&caps0, &caps1, &caps2, &caps3);
	scriptInterface.SetProperty(settings.get(), "x86_caps[0]", caps0);
	scriptInterface.SetProperty(settings.get(), "x86_caps[1]", caps1);
	scriptInterface.SetProperty(settings.get(), "x86_caps[2]", caps2);
	scriptInterface.SetProperty(settings.get(), "x86_caps[3]", caps3);

	scriptInterface.SetProperty(settings.get(), "x86_icaches", ConvertCaches(scriptInterface, x86_x64_ICaches()));
	scriptInterface.SetProperty(settings.get(), "x86_dcaches", ConvertCaches(scriptInterface, x86_x64_DCaches()));
	scriptInterface.SetProperty(settings.get(), "x86_itlbs", ConvertTLBs(scriptInterface, x86_x64_ITLBs()));
	scriptInterface.SetProperty(settings.get(), "x86_dtlbs", ConvertTLBs(scriptInterface, x86_x64_DTLBs()));
#endif

	// Send the same data to the reporting system
	g_UserReporter.SubmitReport("hwdetect", 4, scriptInterface.StringifyJSON(settings.get(), false));

	// Run the detection script:

	scriptInterface.CallFunctionVoid(scriptInterface.GetGlobalObject(), "RunHardwareDetection", settings);
}

// We use some constants that aren't provided by glext.h on some old systems.
// Define all the necessary ones that are missing from GL_GLEXT_VERSION 39 (Mesa 7.0)
// since that's probably an old enough baseline:
#ifndef GL_VERSION_3_0
# define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
# define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#endif
#ifndef GL_EXT_transform_feedback
# define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT 0x8C8A
# define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT 0x8C8B
# define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT 0x8C80
#endif
#ifndef GL_ARB_geometry_shader4
# define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB 0x8C29
# define GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB 0x8DDD
# define GL_MAX_VERTEX_VARYING_COMPONENTS_ARB 0x8DDE
# define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB 0x8DDF
# define GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB 0x8DE0
# define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB 0x8DE1
#endif

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
	// Skip MAX_COLOR_MATRIX_STACK_DEPTH (only in imaging subset)
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
	// Skip MAX_CONVOLUTION_WIDTH, MAX_CONVOLUTION_HEIGHT (only in imaging subset)
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
}
