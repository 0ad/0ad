dofile("../functions.lua")

atlas = 0 -- temporarily disabled, until the whole of Atlas
          -- can be integrated into this build process

-- Set up the Project
project.name = "pyrogenesis"
project.bindir = "../../../binaries/system"
project.libdir = "../../../binaries/system"
project.debugdir = "../../../binaries/data"
project.configs = { "Debug", "Release", "Testing" }

function setuppackage (projectname)

	-- Start the package part
	package = newpackage()

	if (projectname == "sced") then
		package.name = "sced"
		exename = "sced"
		objdirprefix = "obj/ScEd_"
		package.build = 0   -- Don't build Sced by default
		atlas = 0  -- Don't build Atlas into ScEd
	else
		package.name = "pyrogenesis"
		exename = "ps"
		objdirprefix = "obj/"
	end

	-- Windowed executable on windows, "exe" on all other platforms
	package.kind = "winexe"
	package.language = "c++"

	-- Package target for debug and release build
	-- On Windows, ".exe" is added on the end, on unices the name is used directly
	package.config["Debug"].target = exename.."_dbg"
	package.config["Release"].target = exename
	package.config["Testing"].target = exename.."_test"

	package.config["Debug"].objdir = objdirprefix.."Debug"
	package.config["Release"].objdir  = objdirprefix.."Release"
	package.config["Testing"].objdir = objdirprefix.."Testing"


	sourceroot = "../../../source/"
	librariesroot = "../../../libraries/"

	source_dirs = {
		"ps",
		"ps/scripting",
		"ps/Network",

		"simulation",
		"simulation/scripting",

		"lib",
		"lib/sysdep",
		"lib/res",
		"lib/res/file",
		"lib/res/graphics",
		"lib/res/sound",

		"graphics",
		"graphics/scripting",

		"maths",
		"maths/scripting",

		"renderer"
		"terrain",
		"sound",
		"scripting",
		"i18n",
		"tests"
	}

	if (projectname ~= "sced") then tconcat(source_dirs, {
		"gui",
		"gui/scripting"
	}) end

	if (atlas == 1) then tconcat(source_dirs, {
		"tools/atlas/GameInterface",
		"tools/atlas/GameInterface/Handlers"
	}) end

	if (projectname == "sced") then tconcat(source_dirs, {
		"tools/sced",
		"tools/sced/ui"
	}) end

	package.files = sourcesfromdirs(sourceroot, source_dirs)

	tinsert(package.files, sourceroot.."main.cpp")


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

	if (projectname == "sced") then
		tinsert(include_dirs, "tools/sced")
	end

	package.includepaths = {}
	foreach(include_dirs, function (i,v)
		tinsert(package.includepaths, sourceroot .. v)
	end)

	package.libpaths = {}

	if (OS == "windows") then
		package.buildflags = { "no-rtti" }
		package.buildoptions = { "/W4" }
	else
		package.buildflags = { }
	end

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.

	package.config["Debug"].buildflags = { "with-symbols", "no-edit-and-continue" }

	package.config["Testing"].buildflags = { "with-symbols", "no-runtime-checks", "no-edit-and-continue" }
	package.config["Testing"].defines = { "TESTING" }

	package.config["Release"].buildflags = { "with-symbols", "no-runtime-checks", "no-edit-and-continue", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }


	if (projectname == "sced") then
		tinsert(package.defines, "SCED")
		tinsert(package.defines, "_AFXDLL")
		-- Disable the whole GUI system, since it conflicts with the MFC headers
		tinsert(package.defines, "NO_GUI")
		-- Some definitions to make the lib code initialisation work
		tinsert(package.defines, "USE_WINMAIN")
		tinsert(package.defines, "NO_MAIN_REDIRECT")
	end

	if (atlas == 1) then
		tinsert(package.defines, "ATLAS")
	end

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
			"vorbis",
			"boost",
			"libjpg",
			"dbghelp",
			"directx"
		}

		-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
		foreach(external_libraries, function (i,v)
			tinsert(package.includepaths,	librariesroot..v.."/include")
			tinsert(package.libpaths,		librariesroot..v.."/lib")
		end)

		-- Libraries
		package.links = { "opengl32" }
		tinsert(package.files, sourcesfromdirs(sourceroot, {"lib/sysdep/win"}))
		tinsert(package.files, {sourceroot.."lib/sysdep/win/error_dialog.rc"})

		if (projectname == "sced") then
			tinsert(package.files, {sourceroot.."tools/sced/ui/ScEd.rc"})
		end

		if (projectname ~= "sced") then
			tinsert(package.files, {sourceroot.."lib/sysdep/win/icon.rc"})
		end

		package.linkoptions = { "/ENTRY:entry",
			"/DELAYLOAD:opengl32.dll",
			"/DELAYLOAD:oleaut32.dll",
			"/DELAYLOAD:advapi32.dll",
			"/DELAYLOAD:gdi32.dll",
			"/DELAYLOAD:user32.dll",
			"/DELAYLOAD:ws2_32.dll",
			"/DELAYLOAD:version.dll",
			"/DELAYLOAD:ddraw.dll",
			"/DELAYLOAD:dsound.dll",
			"/DELAYLOAD:glu32.dll",
			"/DELAYLOAD:openal32.dll",
			"/DELAYLOAD:dbghelp.dll",
			"/DELAY:UNLOAD"		-- allow manual unload of delay-loaded DLLs
		}

		package.config["Debug"].linkoptions = {
			"/DELAYLOAD:js32d.dll",
			"/DELAYLOAD:zlib1d.dll",
			"/DELAYLOAD:libpng13d.dll",
			"/DELAYLOAD:jpeg-6bd.dll",
			"/DELAYLOAD:vorbisfile_d.dll",
		}

		-- 'Testing' uses 'Debug' DLL's
		package.config["Testing"].linkoptions = package.config["Debug"].linkoptions

		package.config["Release"].linkoptions = {
			"/DELAYLOAD:js32.dll",
			"/DELAYLOAD:zlib1.dll",
			"/DELAYLOAD:libpng13.dll",
			"/DELAYLOAD:jpeg-6b.dll",
			"/DELAYLOAD:vorbisfile.dll",
		}

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, { "no-main" })

		package.pchHeader = "precompiled.h"
		package.pchSource = "precompiled.cpp"

	else -- Non-Windows, = Unix

		tinsert(package.files, sourcesfromdirs(sourceroot, {"lib/sysdep/unix"}))

		-- Libraries
		package.links = {
			-- OpenGL and X-Windows
			"GL", "GLU", "X11",
			"SDL", "png", "jpeg",
			"fam",
			-- Audio
			"openal", "vorbisfile",
			-- Utilities
			"xerces-c", "z", "pthread", "rt", "js"
		}
		tinsert(package.libpaths, { "/usr/X11R6/lib" } )
		-- Defines
		package.defines = {
			"__STDC_VERSION__=199901L" }
		-- Includes
		tinsert(package.includepaths, { "/usr/X11R6/include/X11" } )
	end
end

setuppackage("pyrogenesis")
if (OS == "windows") then
	setuppackage("sced")
end
