addoption("atlas", "Include Atlas scenario editor packages")
addoption("outpath", "Location for generated project files")

dofile("functions.lua")

-- Set up the Project
project.name = "pyrogenesis"
project.bindir   = "../../binaries/system"
project.libdir   = "../../binaries/system"
project.debugdir = "../../binaries/system"
if not options["outpath"] then
	error("You must specify the 'outpath' parameter")
end
project.path = options["outpath"]
project.configs = { "Debug", "Release", "Testing" }

if (OS == "windows") then
	project.nasmpath = "../../build/bin/nasm.exe"
end

function create_package()
	package = newpackage()
	package.path = project.path
	package.language = "c++"
	package.buildflags = { "no-manifest", "with-symbols", "no-edit-and-continue" }
	return package
end

---------------- Main game package (pyrogenesis) ----------------

function setuppackage_engine (projectname)

	-- Start the package part
	package = create_package()

	package.name = "pyrogenesis"
	exename = "ps"
	objdirprefix = "obj/"

	-- Windowed executable on windows, "exe" on all other platforms
	package.kind = "winexe"

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
		"i18n",

		"gui",
		"gui/scripting",

		"tools/atlas/GameInterface",
		"tools/atlas/GameInterface/Handlers"
	}

	package.files = sourcesfromdirs(sourceroot, source_dirs)
	table.insert(package.files, sourceroot.."main.cpp")

	package.trimprefix = sourceroot
	
	--trimrootdir(sourceroot, package.files)

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
	for i,v in include_dirs do
		table.insert(package.includepaths, sourceroot .. v)
	end

	package.libpaths = {}

	table.insert(package.buildflags, "extra-warnings")
	
	if (OS == "windows") then
		table.insert(package.buildflags, "no-rtti")
	end

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.

	package.config["Testing"].buildflags = { "no-runtime-checks" }
	package.config["Testing"].defines = { "TESTING" }

	package.config["Release"].buildflags = { "no-runtime-checks", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

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
		for i,v in external_libraries do
			table.insert(package.includepaths, librariesroot..v.."/include")
			table.insert(package.libpaths,     librariesroot..v.."/lib")
		end

		-- Libraries
		package.links = { "opengl32" }
		table.insert(package.files, sourcesfromdirs(sourceroot, {"lib/sysdep/win"}))
		table.insert(package.files, {sourceroot.."lib/sysdep/win/error_dialog.rc"})
		table.insert(package.files, {sourceroot.."lib/sysdep/win/icon.rc"})

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
		table.insert(package.buildflags, { "no-main" })

		-- use native wchar_t type (not typedef to unsigned short)
		table.insert(package.buildflags, { "native-wchar_t" })

		package.pchheader = "precompiled.h"
		package.pchsource = "precompiled.cpp"

	else -- Non-Windows, = Unix

		table.insert(package.files, sourcesfromdirs(sourceroot, {"lib/sysdep/unix"}))

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
		
		table.insert(package.libpaths, { "/usr/X11R6/lib" } )
		-- Defines
		package.defines = {
			"__STDC_VERSION__=199901L",
			"CONFIG_USE_MMGR" }
		-- Includes (X11 include path isn't standard across distributions)
		table.insert(package.includepaths, { "/usr/X11R6/include/X11" } )
		table.insert(package.includepaths, { "/usr/include/X11" } )

		-- speed up math functions by inlining. warning: this may result in
		-- non-IEEE-conformant results, but haven't noticed any trouble so far.
		table.insert(package.buildoptions, { "-ffast-math" })
	end
end



---------------- Main Atlas package ----------------

function setuppackage_atlas(package_name, target_type, source_dirs, include_dirs, flags)

	package = create_package()
	package.name = package_name
	objdirprefix = "obj/"..package_name.."_"

	package.kind = target_type

	-- Package target for debug and release build
	package.config["Debug"].target = package_name.."_d"
	package.config["Release"].target = package_name

	package.config["Debug"].objdir = objdirprefix.."Debug"
	package.config["Release"].objdir  = objdirprefix.."Release"

	sourceroot = "../../../source/tools/atlas/" .. package_name
	librariesroot = "../../../libraries/"

	sources = sourcesfromdirs(sourceroot, source_dirs)
	if flags["extrasource"] then
		for i,v in flags["extrasource"] do
			table.insert(sources, sourceroot .. v)
		end
	end

	-- We don't want three pointless levels of directories in each project,
	-- so remove the sourceroot directory from the filenames (where those
	-- names are used by Premake to construct the project tree), but set
	-- 'filesprefix' (with Premake altered to recognise that) so the project
	-- will still point to the correct filenames.
	package.trimprefix = sourceroot .. "/"
	package.files = sources

	package.includepaths = {}
	for i,v in include_dirs do
		table.insert(package.includepaths, sourceroot .. v)
	end

	package.libpaths = {}

	if (OS == "windows") then
		table.insert(package.buildflags, "no-rtti")
	end

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.
	package.config["Release"].buildflags = { "no-runtime-checks", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- Platform Specifics
	if (OS == "windows") then

		table.insert(package.defines, "_UNICODE")

		-- Directories under 'libraries', each containing 'lib' and 'include':
		external_libraries = {}
		if (flags["boost"])  then table.insert(external_libraries, "boost") end
		if (flags["devil"])  then table.insert(external_libraries, "devil") end
		if (flags["xerces"]) then table.insert(external_libraries, "xerces") end
		if (flags["zlib"])   then table.insert(external_libraries, "zlib") end
		
		external_libraries.n = nil; -- remove the array size, else it'll be interpreted as a directory

		-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
		for i,v in external_libraries do
			table.insert(package.includepaths,	librariesroot..v.."/include")
			table.insert(package.libpaths,		librariesroot..v.."/lib")
		end

		-- Handle wx specially
		if (flags["wx"]) then
			table.insert(package.includepaths, librariesroot.."wxwidgets/include/msvc")
			table.insert(package.includepaths, librariesroot.."wxwidgets/include")
			table.insert(package.libpaths, librariesroot.."wxwidgets/lib/vc_lib")
		end

		-- Link to required libraries		
		package.links = { "winmm", "comctl32", "rpcrt4" }
		package.config["Debug"].links = { "wxmsw26ud_gl" }
		package.config["Release"].links = { "wxmsw26u_gl" }
		if (flags["depends"]) then
			listconcat(package.links, flags["depends"])
		end

		-- required to use WinMain() on Windows, otherwise will default to main()
		table.insert(package.buildflags, { "no-main" })

		if (flags["pch"]) then
			package.pchheader = "stdafx.h"
			package.pchsource = "stdafx.cpp"
		end

	else -- Non-Windows, = Unix
		-- TODO
	end
	
end

---------------- Atlas 'frontend' tool-launching packages ----------------

function setuppackage_atlas_frontend (package_name)

	package = create_package()
	package.name = package_name
	objdirprefix = "obj/Frontend/"..package_name.."_"

	package.kind = "winexe"

	-- Package target for debug and release build
	package.config["Debug"].target = package_name.."_d"
	package.config["Release"].target = package_name

	package.config["Debug"].objdir = objdirprefix.."Debug"
	package.config["Release"].objdir  = objdirprefix.."Release"

	sourceroot = "../../../source/tools/atlas/AtlasFrontends/"

	package.files = {
		sourceroot..package_name..".cpp",
		sourceroot..package_name..".rc"
	}
	package.trimprefix = sourceroot
	
	package.includepaths = { sourceroot..".." }

	package.config["Release"].buildflags = { "no-runtime-checks", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- Platform Specifics
	if (OS == "windows") then
		table.insert(package.defines, "_UNICODE")
		table.insert(package.links, "AtlasUI")

		-- required to use WinMain() on Windows, otherwise will default to main()
		table.insert(package.buildflags, { "no-main" })

	else -- Non-Windows, = Unix
		-- TODO
	end
	
	package.config["Testing"] = package.config["Debug"]

end

---------------- Atlas packages ----------------

function setuppackages_atlas()
	setuppackage_atlas("AtlasObject", "lib", {
		-- src
		""
	},{
		-- include
	},{
		xerces = 1
	})

	setuppackage_atlas("AtlasUI", "dll", {
		-- src
		"/ActorEditor",
		"/ArchiveViewer",
		"/ColourTester",
		"/CustomControls/Buttons",
		"/CustomControls/DraggableListCtrl",
		"/CustomControls/EditableListCtrl",
		"/CustomControls/FileHistory",
		"/CustomControls/HighResTimer",
		"/CustomControls/SnapSplitterWindow",
		"/CustomControls/VirtualDirTreeCtrl",
		"/CustomControls/Windows",
		"/FileConverter",
		"/General",
		"/Misc",
		"/ScenarioEditor",
		"/ScenarioEditor/Sections/Common",
		"/ScenarioEditor/Sections/Map",
		"/ScenarioEditor/Sections/Object",
		"/ScenarioEditor/Sections/Terrain",
		"/ScenarioEditor/Tools",
		"/ScenarioEditor/Tools/Common"
	},{
		-- include
		"/..",
		"",
		"/CustomControls"
	},{
		pch = 1,
		boost = 1,
		devil = 1,
		wx = 1,
		xerces = 1,
		depends = { "AtlasObject", "DatafileIO" },
		extrasource = { "/Misc/icons.rc" }
	})

	setuppackage_atlas("DatafileIO", "lib", {
		-- src
		"",
		"/BAR",
		"/DDT",
		"/SCN",
		"/Stream",
		"/XMB"
	},{
		-- include
		""
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

if (options["atlas"]) then
	setuppackages_atlas()
end
