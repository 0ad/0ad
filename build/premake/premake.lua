addoption("atlas", "Include Atlas scenario editor packages")
addoption("sced", "Include ScEd package")

dofile("../functions.lua")

-- Set up the Project
project.name = "pyrogenesis"
project.bindir   = "../../../binaries/system"
project.libdir   = "../../../binaries/system"
project.debugdir = "../../../binaries/system"
project.configs = { "Debug", "Release", "Testing" }

if (OS == "windows") then
	project.nasmpath = "../../../build/bin/nasm"
end

---------------- Main game package (pyrogenesis/sced) ----------------

function setuppackage_engine (projectname)

	-- Start the package part
	package = newpackage()

	if (projectname == "sced") then
		package.name = "sced"
		exename = "sced"
		objdirprefix = "obj/ScEd_"
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
		"ps/GameSetup",
		"ps/XML",

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

		"renderer",
		"terrain",
		"sound",
		"scripting",
		"i18n"
	}

	if (projectname ~= "sced") then tconcat(source_dirs, {
		"gui",
		"gui/scripting",

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
			"xerces-c", "z", "pthread", "rt", "js",
			-- Debugging
			"bfd", "iberty"
		}
		-- For debug_resolve_symbol
--		package.config["Debug"].links = { "bfd", "iberty" }
--		package.config["Testing"].links = { "bfd", "iberty" }
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }

		package.buildoptions = {
			"-Wall",
			"-Wunused-parameter",	-- needs to be enabled explicitly
			"-Wno-switch",		-- enumeration value not handled in switch
			"-Wno-reorder",		-- order of initialization list in constructors
			"-Wno-non-virtual-dtor",
		}
		
		tinsert(package.libpaths, { "/usr/X11R6/lib" } )
		-- Defines
		package.defines = {
			"__STDC_VERSION__=199901L",
			"CONFIG_USE_MMGR" }
		-- Includes (X11 include path isn't standard across distributions)
		tinsert(package.includepaths, { "/usr/X11R6/include/X11" } )
		tinsert(package.includepaths, { "/usr/include/X11" } )

		-- speed up math functions by inlining. warning: this may result in
		-- non-IEEE-conformant results, but haven't noticed any trouble so far.
		tinsert(package.buildoptions, { "-ffast-math" })
	end
end



---------------- Main Atlas package ----------------

function setuppackage_atlas(package_name, target_type, source_dirs, include_dirs, flags)

	package = newpackage()
	package.name = package_name
	objdirprefix = "obj/"..package_name.."_"

	package.kind = target_type
	package.language = "c++"

	-- Package target for debug and release build
	package.config["Debug"].target = package_name.."_d"
	package.config["Release"].target = package_name

	package.config["Debug"].objdir = objdirprefix.."Debug"
	package.config["Release"].objdir  = objdirprefix.."Release"

	sourceroot = "../../../source/tools/atlas/"
	librariesroot = "../../../libraries/"


	sources = sourcesfromdirs(sourceroot, source_dirs)
	if (flags["extrasource"]) then
		foreach(flags["extrasource"], function (i,v)
			tinsert(sources, sourceroot..v)
		end)
	end

	-- We don't want three pointless levels of directories in each project,
	-- so remove the sourceroot directory from the filenames (where those
	-- names are used by Premake to construct the project tree), but set
	-- 'filesprefix' (with Premake altered to recognise that) so the project
	-- will still point to the correct filenames.
	trimrootdir(sourceroot, sources)
	package.filesprefix = sourceroot
	package.files = sources

	package.includepaths = {}
	foreach(include_dirs, function (i,v)
		tinsert(package.includepaths, sourceroot .. v)
	end)

	package.libpaths = {}

	if (OS == "windows") then
		package.buildflags = { "no-rtti" }
	else
		package.buildflags = { }
	end

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.
	package.config["Debug"].buildflags = { "with-symbols", "no-edit-and-continue" }
	package.config["Release"].buildflags = { "with-symbols", "no-runtime-checks", "no-edit-and-continue", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- Platform Specifics
	if (OS == "windows") then

		tinsert(package.defines, "_UNICODE")

		-- Directories under 'libraries', each containing 'lib' and 'include':
		external_libraries = {}
		if (flags["boost"])  then tinsert(external_libraries, "boost") end
		if (flags["devil"])  then tinsert(external_libraries, "devil/src") end
		if (flags["xerces"]) then tinsert(external_libraries, "xerces") end
		if (flags["zlib"])   then tinsert(external_libraries, "zlib") end
		
		external_libraries.n = nil; -- remove the array size, else it'll be interpreted as a directory

		-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
		foreach(external_libraries, function (i,v)
			tinsert(package.includepaths,	librariesroot..v.."/include")
			tinsert(package.libpaths,		librariesroot..v.."/lib")
		end)

		-- Handle wx specially
		if (flags["wx"]) then
			tinsert(package.includepaths, librariesroot.."wxwidgets/include/msvc")
			tinsert(package.includepaths, librariesroot.."wxwidgets/include")
			tinsert(package.libpaths, librariesroot.."wxwidgets/lib/vc_lib")
		end

		-- Link to required libraries		
		package.links = { "winmm", "comctl32", "rpcrt4" }
		package.config["Debug"].links = { "wxmsw26ud_gl" }
		package.config["Release"].links = { "wxmsw26u_gl" }
		if (flags["depends"]) then
			tconcat(package.links, flags["depends"])
		end

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, { "no-main" })

		if (flags["pch"]) then
			package.pchHeader = "stdafx.h"
			package.pchSource = "stdafx.cpp"
		end

	else -- Non-Windows, = Unix
		-- TODO
	end
	
end

---------------- Atlas 'frontend' tool-launching packages ----------------

function setuppackage_atlas_frontend (package_name)

	package = newpackage()
	package.name = package_name
	objdirprefix = "obj/Frontend/"..package_name.."_"

	package.kind = "winexe"
	package.language = "c++"

	-- Package target for debug and release build
	package.config["Debug"].target = package_name.."_d"
	package.config["Release"].target = package_name

	package.config["Debug"].objdir = objdirprefix.."Debug"
	package.config["Release"].objdir  = objdirprefix.."Release"

	sourceroot = "../../../source/tools/atlas/AtlasFrontends/"

	package.filesprefix = sourceroot
	package.files = {
		package_name..".cpp",
		package_name..".rc"
	}
	
	package.includepaths = { sourceroot..".." }

	package.config["Debug"].buildflags = { "with-symbols", "no-edit-and-continue" }
	package.config["Release"].buildflags = { "with-symbols", "no-runtime-checks", "no-edit-and-continue", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- Platform Specifics
	if (OS == "windows") then
		tinsert(package.defines, "_UNICODE")
		tinsert(package.links, "AtlasUI")

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, { "no-main" })

	else -- Non-Windows, = Unix
		-- TODO
	end
	
	package.config["Testing"] = package.config["Debug"]

end

---------------- Atlas packages ----------------

function setuppackages_atlas()
	setuppackage_atlas("AtlasObject", "lib", {
		-- src
		"AtlasObject"
	},{
		-- include
	},{
		xerces = 1
	})

	setuppackage_atlas("AtlasUI", "dll", {
		-- src
		"AtlasUI/ActorEditor",
		"AtlasUI/ArchiveViewer",
		"AtlasUI/ColourTester",
		"AtlasUI/CustomControls/Buttons",
		"AtlasUI/CustomControls/DraggableListCtrl",
		"AtlasUI/CustomControls/EditableListCtrl",
		"AtlasUI/CustomControls/FileHistory",
		"AtlasUI/CustomControls/HighResTimer",
		"AtlasUI/CustomControls/SnapSplitterWindow",
		"AtlasUI/CustomControls/VirtualDirTreeCtrl",
		"AtlasUI/CustomControls/Windows",
		"AtlasUI/FileConverter",
		"AtlasUI/General",
		"AtlasUI/Misc",
		"AtlasUI/ScenarioEditor",
		"AtlasUI/ScenarioEditor/Sections/Common",
		"AtlasUI/ScenarioEditor/Sections/Map",
		"AtlasUI/ScenarioEditor/Sections/Terrain",
		"AtlasUI/ScenarioEditor/Tools",
		"AtlasUI/ScenarioEditor/Tools/Common"
	},{
		-- include
		"",
		"AtlasUI",
		"AtlasUI/CustomControls"
	},{
		pch = 1,
		boost = 1,
		devil = 1,
		wx = 1,
		xerces = 1,
		depends = { "AtlasObject", "DatafileIO" },
		extrasource = { "AtlasUI/Misc/icons.rc" }
	})

	setuppackage_atlas("DatafileIO", "lib", {
		-- src
		"DatafileIO",
		"DatafileIO/BAR",
		"DatafileIO/DDT",
		"DatafileIO/SCN",
		"DatafileIO/Stream",
		"DatafileIO/XMB"
	},{
		-- include
		"DatafileIO"
	},{
		pch = 1,
		devil = 1,
		xerces = 1,
		zlib = 1
	})
	
	setuppackage_atlas_frontend("ActorEditor")
	setuppackage_atlas_frontend("ArchiveViewer")
	setuppackage_atlas_frontend("ColourTester")
	setuppackage_atlas_frontend("FileConverter")

end

--------------------------------

setuppackage_engine("pyrogenesis")

if (options["sced"]) then
	setuppackage_engine("sced")
end

if (options["atlas"]) then
	setuppackages_atlas()
end
