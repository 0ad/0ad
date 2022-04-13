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

#include "lib/svn_revision.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/posix/posix_utsname.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/numa.h"
#include "lib/sysdep/os_cpu.h"
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
#include "renderer/backend/gl/Device.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"

// FreeType headers might have an include order.
#include <ft2build.h>
#include <freetype/freetype.h>

#if OS_LINUX
#include <fstream>
#endif

#include <sstream>
#include <string>

static void ReportSDL(const ScriptRequest& rq, JS::HandleValue settings);
static void ReportFreeType(const ScriptRequest& rq, JS::HandleValue settings);

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

	Script::SetProperty(rq, settings, "gfx_card", g_VideoMode.GetBackendDevice()->GetName());
	Script::SetProperty(rq, settings, "gfx_drv_ver", g_VideoMode.GetBackendDevice()->GetDriverInformation());
#if CONFIG2_AUDIO
	if (g_SoundManager)
	{
		Script::SetProperty(rq, settings, "snd_card", g_SoundManager->GetSoundCardNames());
		Script::SetProperty(rq, settings, "snd_drv_ver", g_SoundManager->GetOpenALVersion());
	}
#endif
	ReportSDL(rq, settings);

	ReportFreeType(rq, settings);

	g_VideoMode.GetBackendDevice()->Report(rq, settings);

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
	const int reportVersion = 18;

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

static void ReportFreeType(const ScriptRequest& rq, JS::HandleValue settings)
{
	FT_Library FTLibrary;
	std::string FTSupport = "unsupported";
	if (!FT_Init_FreeType(&FTLibrary))
	{
		FT_Int major, minor, patch;
		FT_Library_Version(FTLibrary, &major, &minor, &patch);
		FT_Done_FreeType(FTLibrary);
		std::stringstream version;
		version << major << "." << minor << "." << patch;
		FTSupport = version.str();
	}
	else
	{
		FTSupport = "cantload";
	}
	Script::SetProperty(rq, settings, "freetype", FTSupport);
}

