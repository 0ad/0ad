/* Copyright (C) 2010 Wildfire Games.
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
#include "lib/utf8.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/gfx.h"
#include "lib/sysdep/os_cpu.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/Config.h"

void SetDisableAudio(void* UNUSED(cbdata), bool disabled)
{
	g_DisableAudio = disabled;
}

void RunHardwareDetection()
{
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

	CScriptValRooted settings;
	scriptInterface.Eval("({})", settings);

	scriptInterface.SetProperty(settings.get(), "os_unix", OS_UNIX, false);
	scriptInterface.SetProperty(settings.get(), "os_linux", OS_LINUX, false);
	scriptInterface.SetProperty(settings.get(), "os_macosx", OS_MACOSX, false);
	scriptInterface.SetProperty(settings.get(), "os_win", OS_WIN, false);

	scriptInterface.SetProperty(settings.get(), "gfx_card", std::wstring(gfx_card), false);
	scriptInterface.SetProperty(settings.get(), "gfx_drv_ver", std::wstring(gfx_drv_ver), false);
	scriptInterface.SetProperty(settings.get(), "gfx_mem", gfx_mem, false);

	scriptInterface.SetProperty(settings.get(), "gl_vendor", std::string((const char*)glGetString(GL_VENDOR)), false);
	scriptInterface.SetProperty(settings.get(), "gl_renderer", std::string((const char*)glGetString(GL_RENDERER)), false);
	scriptInterface.SetProperty(settings.get(), "gl_version", std::string((const char*)glGetString(GL_VERSION)), false);

	const char* exts = ogl_ExtensionString();
	if (!exts) exts = "";
	scriptInterface.SetProperty(settings.get(), "gl_extensions", std::string(exts), false);
	scriptInterface.SetProperty(settings.get(), "gl_max_tex_size", (int)ogl_max_tex_size, false);
	scriptInterface.SetProperty(settings.get(), "gl_max_tex_units", (int)ogl_max_tex_units, false);

	scriptInterface.SetProperty(settings.get(), "video_xres", g_VideoMode.GetXRes(), false);
	scriptInterface.SetProperty(settings.get(), "video_yres", g_VideoMode.GetYRes(), false);
	scriptInterface.SetProperty(settings.get(), "video_bpp", g_VideoMode.GetBPP(), false);

	struct utsname un;
	uname(&un);
	scriptInterface.SetProperty(settings.get(), "uname_sysname", std::string(un.sysname), false);
	scriptInterface.SetProperty(settings.get(), "uname_release", std::string(un.release), false);
	scriptInterface.SetProperty(settings.get(), "uname_version", std::string(un.version), false);
	scriptInterface.SetProperty(settings.get(), "uname_machine", std::string(un.machine), false);

	scriptInterface.SetProperty(settings.get(), "cpu_identifier", std::string(cpu_IdentifierString()), false);
	scriptInterface.SetProperty(settings.get(), "cpu_frequency", os_cpu_ClockFrequency(), false);

	scriptInterface.SetProperty(settings.get(), "ram_total", (int)os_cpu_MemorySize(), false);
	scriptInterface.SetProperty(settings.get(), "ram_free", (int)os_cpu_MemoryAvailable(), false);

	// Run the detection script:

	scriptInterface.CallFunctionVoid(scriptInterface.GetGlobalObject(), "RunHardwareDetection", settings);
}
