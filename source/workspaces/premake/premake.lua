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
	-- lib/
	{ sourcesfromdirs(
                "../../lib",
                "../../lib/sysdep",
                "../../lib/res") },
	-- terrain/
	{ sourcesfromdirs(
                "../../terrain") },
	-- gui/
	{ sourcesfromdirs(
                "../../gui") },
	-- main
        { "../../main.cpp" },
	-- scripting
	{ sourcesfromdirs("../../scripting") }
}
package.includepaths = { "../../ps", "../../simulation", "../../lib", "../../terrain", "../.." }

-- Platform Specifics
if (OS == "windows") then
	-- Libraries
	package.links = {
		"opengl32",
		"glu32"
	}
        tinsert(package.files, sourcesfromdirs("../../lib/sysdep/win"))
	package.linkoptions = { "/ENTRY:entry" }
	package.linkflags = { "static-runtime" }
	package.buildflags = { "no-main" }
else -- Non-Windows, = Unix
	-- Libraries
	package.links = { "GL", "GLU", "X" }
	-- Defines
	package.defines = { "HAVE_X" }
end

