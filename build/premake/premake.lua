dofile("../functions.lua")

-- Set up the Project
project.name = "pyrogenesis"
project.bindir = "../../../binaries/system"
project.libdir = "../../../binaries/system"
project.debugdir = "../../../binaries/data"
project.configs = { "Debug", "Release", "Testing" }

-- Start the package part
package = newpackage()
package.name = "pyrogenesis"
-- Windowed executable on windows, "exe" on all other platforms
package.kind = "winexe"
package.language = "c++"

-- Package target for debug and release build
-- On Windows, ".exe" is added on the end, on unices the name is used directly
package.config["Debug"].target = "ps_dbg"
package.config["Release"].target = "ps"
package.config["Testing"].target = "ps_test"

sourceroot = "../../../source/"
librariesroot = "../../../libraries/"

-- Files
package.files = {
	sourcesfromdirs(sourceroot,
				
		"ps",
		"ps/scripting",
		"ps/Network",

		"simulation",
		"simulation/scripting",

		"lib",
		"lib/sysdep",
		"lib/res",

		"graphics",
		"graphics/scripting",

		"maths",
		"maths/scripting",

		"renderer",

		"gui",
		"gui/scripting",

		"terrain",

		"sound",

		"scripting",

		"i18n",

		"tests"
	),

	sourceroot.."main.cpp"
}


include_dirs = {
	"ps",
	"simulation",
	"lib",
	"graphics",
	"maths",
	"renderer",
	"terrain",
	""
}

package.includepaths = {}

foreach(include_dirs, function (i,v)
	tinsert(package.includepaths, sourceroot .. v)
end)


package.libpaths = {}


package.buildflags = { "no-rtti" }

package.config["Testing"].buildflags = { "with-symbols", "no-runtime-checks", "no-edit-and-continue" }
package.config["Testing"].defines = { "TESTING" }

package.config["Release"].defines = { "NDEBUG" }

-- Docs says that premake does this automatically - it doesn't (at least not for GCC/Linux)
package.config["Debug"].buildflags = { "with-symbols", "no-edit-and-continue" }

-- Platform Specifics
if (OS == "windows") then

	-- Directories under 'libraries', each containing 'lib' and 'include':
	external_libraries = {
		"misc",
		"libpng",
		"zlib",
		"openal",
		"spidermonkey",
		"xerces",
		"vorbis"
	}

	-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
	foreach(external_libraries, function (i,v)
		tinsert(package.includepaths,	librariesroot..v.."/include")
		tinsert(package.libpaths,		librariesroot..v.."/lib")
	end)

	-- Libraries
	package.links = { "opengl32" }
	tinsert(package.files, sourcesfromdirs(sourceroot, "lib/sysdep/win"))
	tinsert(package.files, {sourceroot.."lib/sysdep/win/assert_dlg.rc"})

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
	
	-- 'Testing' uses 'Debug' DLL's
	package.config["Testing"].linkoptions = package.config["Debug"].linkoptions
	
	package.config["Release"].linkoptions = {
		"/DELAYLOAD:js32.dll",
		"/DELAYLOAD:zlib1.dll",
		"/DELAYLOAD:libpng13.dll",
	}
	
	tinsert(package.buildflags, { "no-main" })
	
	package.pchHeader = "precompiled.h"
	package.pchSource = "precompiled.cpp"

else -- Non-Windows, = Unix

	tinsert(package.files, sourcesfromdirs(sourceroot, "lib/sysdep/unix"))

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
