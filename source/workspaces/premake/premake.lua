dofile("functions.lua")

-- Set up the Project
project.name = "prometheus"
project.bindir = "../../../binaries/system"
project.libdir = "../../../binaries/system"
project.configs = { "Debug", "Release", "Testing" }

-- Start the package part
package = newpackage()
package.name = "prometheus"
-- Windowed executable on windows, "exe" on all other platforms
package.kind = "winexe"
package.language = "c++"

-- Package target for debug and release build
-- On Windows, ".exe" is added on the end, on unices the name is used directly
package.config["Debug"].target = "ps_dbg"
package.config["Release"].target = "ps"
package.config["Testing"].target = "ps_test"

-- Files
package.files = {
	-- ps/
	{ sourcesfromdirs("../../ps") },
	{ sourcesfromdirs("../../ps/scripting") },
	{ sourcesfromdirs("../../ps/Network") },

	-- simulation/
	{ sourcesfromdirs("../../simulation") },
	{ sourcesfromdirs("../../simulation/scripting") },

	-- lib/
	{ sourcesfromdirs(
		"../../lib",
		"../../lib/sysdep",
		"../../lib/res") },

	-- graphics/
	{ sourcesfromdirs(
		"../../graphics") },
	{ sourcesfromdirs( "../../graphics/scripting" ) },

	-- maths/
	{ sourcesfromdirs(
		"../../maths") },
	{ sourcesfromdirs( "../../maths/scripting" ) },

	-- renderer/
	{ sourcesfromdirs(
		"../../renderer") },

	-- gui/
	{ sourcesfromdirs(
		"../../gui") },
	{ sourcesfromdirs( "../../gui/scripting" ) },

	-- terrain/
	{ sourcesfromdirs(
		"../../terrain") },

	-- sound/
	{ sourcesfromdirs(
		"../../sound") },

	-- main
	{ "../../main.cpp" },

	-- scripting
	{ sourcesfromdirs("../../scripting") },

	-- i18n
	{ sourcesfromdirs("../../i18n") },

	-- tests
	{ sourcesfromdirs("../../tests") }
}

package.includepaths = {
	"../../ps",
	"../../simulation",
	"../../lib",
	"../../graphics",
	"../../maths",
	"../../renderer",
	"../../terrain",
	"../.." }

package.libpaths = {
	}

package.buildflags = { "no-rtti" }

package.config["Testing"].buildflags = { "with-symbols" }
package.config["Testing"].defines = { "TESTING" }

package.config["Release"].defines = { "NDEBUG" }

-- Docs says that premake does this automatically - it doesn't (at least not for
-- GCC/Linux)
package.config["Debug"].buildflags = { "with-symbols" }

-- Platform Specifics
if (OS == "windows") then
	-- Libraries
	package.links = { "opengl32" }
--	package.defines = { "XERCES_STATIC_LIB" }
	tinsert(package.files, sourcesfromdirs("../../lib/sysdep/win"))
	tinsert(package.files, {"../../lib/sysdep/win/assert_dlg.rc"})
	package.linkoptions = { "/ENTRY:entry",
		"/DELAYLOAD:opengl32.dll",
		"/DELAYLOAD:advapi32.dll",
		"/DELAYLOAD:gdi32.dll",
		"/DELAYLOAD:user32.dll",
		"/DELAYLOAD:ws2_32.dll",
		"/DELAYLOAD:version.dll",
		"/DELAYLOAD:ddraw.dll",
		"/DELAYLOAD:dsound.dll",
		"/DELAYLOAD:glu32.dll",
		"/DELAYLOAD:openal32.dll",
		"/DELAY:UNLOAD"		-- allow manual unload of delay-loaded DLLs
	}
	package.config["Debug"].linkoptions = {
		"/DELAYLOAD:js32d.dll",
		"/DELAYLOAD:zlib1d.dll",
		"/DELAYLOAD:libpng13d.dll",
	}
	
	-- Testing uses Debug DLL's
	package.config["Testing"].linkoptions = package.config["Debug"].linkoptions
	
	package.config["Release"].linkoptions = {
		"/DELAYLOAD:js32.dll",
		"/DELAYLOAD:zlib1.dll",
		"/DELAYLOAD:libpng13.dll",
	}
	
	tinsert(package.buildflags, { "no-main" })
	package.config["Testing"].buildoptions = {
		" /Zi"
	}
	
	package.pchHeader = "precompiled.h"
	package.pchSource = "precompiled.cpp"
else -- Non-Windows, = Unix

	tinsert(package.files, sourcesfromdirs("../../lib/sysdep/unix"))

	-- Libraries
	package.links = {
		-- OpenGL and X-Windows
		"GL", "GLU", "X11",
		"SDL", "png",
		"fam",
		-- Audio
		"openal", "vorbisfile", 
		-- Utilities
		"xerces-c", "z", "rt", "js"
	}
	tinsert(package.libpaths, { "/usr/X11R6/lib" } )
	-- Defines
	package.defines = {
		"__STDC_VERSION__=199901L" }
	-- Includes
	tinsert(package.includepaths, { "/usr/X11R6/include/X11" } )
end
