dofile("functions.lua")

-- Set up the Project
project.name = "prometheus"
project.bindir = "../../../binaries/system"
project.libdir = "../../../binaries/system"

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

-- Files
package.files = {
	-- ps/
        { sourcesfromdirs("../../ps") },
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
	-- terrain/
	{ sourcesfromdirs(
                "../../terrain") },
	-- main
	{ "../../main.cpp" },
	-- scripting
	{ sourcesfromdirs("../../scripting") }
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

package.libpaths = {}

-- Platform Specifics
if (OS == "windows") then
	-- Libraries
	package.links = {
		"opengl32",
		"glu32",
		"fmodvc"
	}
	tinsert(package.files, sourcesfromdirs("../../lib/sysdep/win"))
	package.linkoptions = { "/ENTRY:entry" }
	package.linkflags = { "static-runtime" }
	package.buildflags = { "no-main" }
	package.pchHeader = "precompiled.h"
	package.pchSource = "precompiled.cpp"
else -- Non-Windows, = Unix
	-- Libraries
	package.links = {
		-- OpenGL and X-Windows
		"GL", "GLU", "X11",
		"SDL", "png",
		"fmod-3.70",
		"fam",
		-- Utilities
		"xerces-c", "z", "rt"
	}
	tinsert(package.libpaths, { "/usr/X11R6/lib" } )
	-- Defines
	package.defines = {
		"__STDC_VERSION__=199901L" }
	-- Includes
	tinsert(package.includepaths, { "/usr/X11R6/include/X11" } )
	
	-- Build Flags
	package.buildoptions = { "`pkg-config mozilla-js --cflags`" }
	package.linkoptions = { "`pkg-config mozilla-js --libs`" }
	package.config["Debug"].buildoptions = { "-ggdb" }
end
