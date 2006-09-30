addoption("atlas", "Include Atlas scenario editor packages")
addoption("outpath", "Location for generated project files")
addoption("without-tests", "Disable generation of test projects")

dofile("functions.lua")
dofile("extern_libs.lua")

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

if OS == "windows" then
	project.nasmpath = "../../build/bin/nasm.exe"
	project.cxxtestpath = "../../build/bin/cxxtestgen.exe"
else
	project.cxxtestpath = "../../build/bin/cxxtestgen.pl"
end

source_root = "../../../source/" -- default for most projects - overridden by local in others

-- Rationale: packages should not have any additional include paths except for
-- those required by external libraries. Instead, we should always write the
-- full relative path, e.g. #include "maths/Vector3d.h". This avoids confusion
-- ("which file is meant?") and avoids enormous include path lists.


-- packages: engine static libs, main exe, atlas, atlas frontends, test.


--------------------------------------------------------------------------------
-- package helper functions
--------------------------------------------------------------------------------

function package_set_target(package_name)

	-- Note: On Windows, ".exe" is added on the end, on unices the name is used directly
	package.config["Debug"  ].target = package_name.."_dbg"
	package.config["Testing"].target = package_name.."_test"
	package.config["Release"].target = package_name

	local obj_dir_prefix = "obj/"..package_name.."_"
	package.config["Debug"  ].objdir = obj_dir_prefix.."Debug"
	package.config["Testing"].objdir = obj_dir_prefix.."Test"
	package.config["Release"].objdir = obj_dir_prefix.."Release"
end


function package_set_build_flags()

	package.buildflags = { "with-symbols", "no-edit-and-continue", "extra-warnings" }

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.

	package.config["Testing"].buildflags = { "no-runtime-checks" }
	package.config["Testing"].defines = { "TESTING" }

	package.config["Release"].buildflags = { "no-runtime-checks", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- various platform-specific build flags
	if OS == "windows" then
		tinsert(package.buildflags, "no-rtti")

		-- use native wchar_t type (not typedef to unsigned short)
		tinsert(package.buildflags, "native-wchar_t")
	else	-- *nix
		package.buildoptions = {
			"-Wall",
			"-Wunused-parameter",	-- needs to be enabled explicitly
			"-Wno-switch",		-- enumeration value not handled in switch
			"-Wno-reorder",		-- order of initialization list in constructors
			"-Wno-non-virtual-dtor",

			-- speed up math functions by inlining. warning: this may result in
			-- non-IEEE-conformant results, but haven't noticed any trouble so far.
			"-ffast-math",
		}
		package.defines = {
			"__STDC_VERSION__=199901L",
			"CONFIG_USE_MMGR"
		}
	end
end


-- create a package and set the attributes that are common to all packages.
function package_create(package_name, target_type)

 	-- Note: don't store in local variable. A global variable needs to
 	-- be set for Premake's use; it is implicitly used in e.g. matchfiles()
	package = newpackage()
	package.path = project.path
	package.language = "c++"
	package.name = package_name
	package.kind = target_type

	package_set_target(package_name)
	package_set_build_flags()

	return package
end


-- extra_params: table including zero or more of the following:
-- * no_default_pch: (any type) prevents adding the PCH include dir.
--   see setup_static_lib_package() for explanation of this scheme and rationale.
-- * extra_files: table of filenames (relative to source_root) to add to project
-- * extra_links: table of library names to add to link step
function package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)

	-- We don't want the VC project to be deeply nested (once for each
	-- folder in source_root). Therefore, remove the source_root
	-- directory from the filenames (where those
	-- names are used by Premake to construct the project tree), but set
	-- 'trimprefix' (with Premake altered to recognise that) so the project
	-- will still point to the correct filenames.
	package.trimprefix = source_root
	package.files = sourcesfromdirs(source_root, rel_source_dirs)

	package.includepaths = {}

	-- Put the project-specific PCH directory at the start of the
	-- include path, so '#include "precompiled.h"' will look in
	-- there first
	if not extra_params["no_default_pch"] then
		tinsert(package.includepaths, source_root .. "pch/" .. package.name)
	end

	-- next is source root dir, for absolute (nonrelative) includes
	-- (e.g. "lib/precompiled.h")
	tinsert(package.includepaths, source_root)

	for i,v in rel_include_dirs do
		tinsert(package.includepaths, source_root .. v)
	end


	if extra_params["extra_files"] then
		for i,v in extra_params["extra_files"] do
			tinsert(package.files, source_root .. v)
		end
	end

	if extra_params["extra_links"] then
		listconcat(package.links, extra_params["extra_links"])
	end

end


--------------------------------------------------------------------------------
-- engine static libraries
--------------------------------------------------------------------------------

-- the engine is split up into several static libraries. this eases separate
-- distribution of those components, reduces dependencies a bit, and can
-- also speed up builds.
-- more to the point, it is necessary to efficiently support a separate
-- test executable that also includes much of the game code.

-- names of all static libs created. automatically added to the
-- main app project later (see explanation at end of this file)
static_lib_names = {}

-- set up one of the static libraries into which the main engine code is split.
-- extra_params: see package_add_contents().
-- note: rel_source_dirs and rel_include_dirs are relative to global source_root.
local function setup_static_lib_package (package_name, rel_source_dirs, extern_libs, extra_params)

	package_create(package_name, "lib")
	package_add_contents(source_root, rel_source_dirs, {}, extra_params)
	package_add_extern_libs(extern_libs)

	tinsert(static_lib_names, package_name)

	if OS == "windows" then
		-- Precompiled Headers
		-- rationale: we need one PCH per static lib, since one global header would
		-- increase dependencies. To that end, we can either include them as
		-- "packagedir/precompiled.h", or add "source/PCH/packagedir" to the
		-- include path and put the PCH there. The latter is better because
		-- many packages contain several dirs and it's unclear where there the
		-- PCH should be stored. This way is also a bit easier to use in that
		-- source files always include "precompiled.h".
		-- Notes:
		-- * Visual Assist manages to use the project include path and can
		--   correctly open these files from the IDE.
		-- * precompiled.cpp (needed to "Create" the PCH) also goes in
		--   the abovementioned dir.
		pch_dir = source_root.."pch/"..package_name.."/"
		package.pchheader = "precompiled.h"
		package.pchsource = "precompiled.cpp"
		tinsert(package.files, { pch_dir.."precompiled.cpp", pch_dir.."precompiled.h" })

	end
	
end

-- this is where the source tree is chopped up into static libs.
-- can be changed very easily; just copy+paste a new setup_static_lib_package,
-- or remove existing ones. static libs are automagically added to
-- main_exe link step.
function setup_all_libs ()

	-- relative to global source_root.
	local source_dirs = {}
	-- names of external libraries used (see libraries_dir comment)
	local extern_libs = {}


	source_dirs = {
		"network",
	}
	extern_libs = {
		"spidermonkey",
		"xerces",
		"boost",	-- dragged in via server->simulation.h->random
	}
	setup_static_lib_package("network", source_dirs, extern_libs, {})

	source_dirs = {
		"ps",
		"ps/scripting",
		"ps/Network",
		"ps/GameSetup",
		"ps/XML",
		"simulation",
		"simulation/scripting",
		"sound",
		"scripting",
		"maths",
		"maths/scripting"
	}
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"xerces",
		"opengl",
		"zlib",
		"boost"
	}
	setup_static_lib_package("engine", source_dirs, extern_libs, {})


	source_dirs = {
		"graphics",
		"graphics/scripting",
		"renderer"
	}
	extern_libs = {
		"opengl",
		"sdl",	-- key definitions
		"spidermonkey",	-- for graphics/scripting
		"boost"
	}
	setup_static_lib_package("graphics", source_dirs, extern_libs, {})


	-- internationalization = i18n
	-- note: this package isn't large, but is separate because it may be
	-- useful for other projects.
	source_dirs = {
		"i18n"
	}
	extern_libs = {
		"spidermonkey"
	}
	setup_static_lib_package("i18n", source_dirs, extern_libs, {})


	source_dirs = {
		"tools/atlas/GameInterface",
		"tools/atlas/GameInterface/Handlers"
	}
	extern_libs = {
		"boost",
		"sdl",	-- key definitions
		"opengl",
		"spidermonkey"
	}
	setup_static_lib_package("atlas", source_dirs, extern_libs, {})


	source_dirs = {
		"gui",
		"gui/scripting"
	}
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"opengl",
		"boost"
	}
	setup_static_lib_package("gui", source_dirs, extern_libs, {})


	source_dirs = {
		"lib",
		"lib/sysdep",
		"lib/res",
		"lib/res/file",
		"lib/res/graphics",
		"lib/res/sound"
	}
	extern_libs = {
		"sdl",
		"opengl",
		"libpng",
		"zlib",
		"openal",
		"vorbis",
		"libjpg",
		"dbghelp",
		"directx"
	}
	setup_static_lib_package("lowlevel", source_dirs, extern_libs, {})
	if OS == "windows" then
		tinsert(package.files, sourcesfromdirs(source_root, {"lib/sysdep/win"}))
		-- note: RC file must be added to main_exe package.
	elseif OS == "linux" then
		tinsert(package.files, sourcesfromdirs(source_root, {"lib/sysdep/unix"}))
		-- (X11 include path isn't standard across distributions)
		tinsert(package.includepaths, "/usr/X11R6/include/X11" )
		tinsert(package.includepaths, "/usr/include/X11" )
	elseif OS == "macosx" then 
		tinsert(package.files, sourcesfromdirs(source_root, {"lib/sysdep/osx"}))
	end
end


--------------------------------------------------------------------------------
-- main EXE
--------------------------------------------------------------------------------

-- used for main EXE as well as test
used_extern_libs = {
	"opengl",
	"sdl",

	"libjpg",
	"libpng",
	"zlib",

	"spidermonkey",
	"xerces",

	"openal",
	"vorbis",

	"boost",
	"dbghelp",
	"cxxtest",
	"directx",
}

-- Bundles static libs together with main.cpp and builds game executable.
function setup_main_exe ()

	package_create("pyrogenesis", "winexe")

	-- For VS2005, tell the linker to use the libraries' .obj files instead of
	-- the .lib, to allow incremental linking.
	-- (Reduces re-link time from ~20 seconds to ~2 secs)
	tinsert(package.buildflags, "use-library-dep-inputs")

	local extra_params = {
		extra_files = { "main.cpp" },
	}
	package_add_contents(source_root, {}, {}, extra_params)

	package_add_extern_libs(used_extern_libs)


	-- Platform Specifics
	if OS == "windows" then

		tinsert(package.files, source_root.."lib/sysdep/win/icon.rc")
		-- from "lowlevel" static lib; must be added here to be linked in
		tinsert(package.files, source_root.."lib/sysdep/win/error_dialog.rc")

		-- VS2005 generates its own manifest, but earlier ones need us to add it manually
		if (options["target"] == "vs2002" or options["target"] == "vs2003") then
			tinsert(package.files, source_root.."lib/sysdep/win/manifest.rc")
		end

		package.linkoptions = {
			-- required for win.cpp's init mechanism
			"/ENTRY:entry",

			-- delay loading of various Windows DLLs (not specific to any of the
			-- external libraries; those and handled separately)
			"/DELAYLOAD:oleaut32.dll",
			"/DELAYLOAD:advapi32.dll",
			"/DELAYLOAD:user32.dll",
			"/DELAYLOAD:ws2_32.dll",
			"/DELAYLOAD:version.dll",

			-- allow manual unload of delay-loaded DLLs
			"/DELAY:UNLOAD"
		}

	elseif OS == "linux" then

		-- Libraries
		tinsert(package.links, {
			"fam",
			-- Utilities
			"pthread", "rt",
			-- Debugging
			"bfd", "iberty"
		})
		-- For debug_resolve_symbol
		
	
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }

	
		tinsert(package.libpaths, "/usr/X11R6/lib")
	elseif OS == "macosx" then
		-- Libraries
		tinsert(package.links, {			-- Utilities
			"pthread"
		})

	
		tinsert(package.libpaths, "/usr/X11R6/lib")

	
	end
end


--------------------------------------------------------------------------------
-- atlas
--------------------------------------------------------------------------------

-- setup a typical Atlas component package
-- extra_params: as in package_add_contents; also zero or more of the following:
-- * pch: (any type) set stdafx.h and .cpp as PCH
local function setup_atlas_package(package_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	local source_root = "../../../source/tools/atlas/" .. package_name .. "/"
	package_create(package_name, target_type)

	-- Don't add the default 'sourceroot/pch/projectname' for finding PCH files
	extra_params["no_default_pch"] = 1

	package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)
	package_add_extern_libs(extern_libs)

	-- Platform Specifics
	if OS == "windows" then

		tinsert(package.defines, "_UNICODE")

		-- Link to required libraries
		package.links = { "winmm", "comctl32", "rpcrt4" }

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, "no-main")

		if extra_params["pch"] then
			package.pchheader = "stdafx.h"
			package.pchsource = "stdafx.cpp"
		end

		if extra_params["extra_links"] then 
			listconcat(package.links, extra_params["extra_links"]) 
		end

	else -- Non-Windows, = Unix
		-- TODO
	end

end


-- build all Atlas component packages
function setup_atlas_packages()

	setup_atlas_package("AtlasObject", "lib",
	{	-- src
		""
	},{	-- include
	},{	-- extern_libs
		"xerces"
	},{	-- extra_params
	})

	setup_atlas_package("AtlasUI", "dll",
	{	-- src
		"ActorEditor",
		"ActorViewer",
		"ArchiveViewer",
		"ColourTester",
		"CustomControls/Buttons",
		"CustomControls/Canvas",
		"CustomControls/ColourDialog",
		"CustomControls/DraggableListCtrl",
		"CustomControls/EditableListCtrl",
		"CustomControls/FileHistory",
		"CustomControls/HighResTimer",
		"CustomControls/SnapSplitterWindow",
		"CustomControls/VirtualDirTreeCtrl",
		"CustomControls/Windows",
		"FileConverter",
		"General",
		"Misc",
		"ScenarioEditor",
		"ScenarioEditor/Sections/Common",
		"ScenarioEditor/Sections/Cinematic",
		"ScenarioEditor/Sections/Environment",
		"ScenarioEditor/Sections/Map",
		"ScenarioEditor/Sections/Object",
		"ScenarioEditor/Sections/Terrain",
		"ScenarioEditor/Tools",
		"ScenarioEditor/Tools/Common"
	},{	-- include
		"..",
		"CustomControls"
	},{	-- extern_libs
		"boost",
		"devil",
		"xerces",
		"wxwidgets"
	},{	-- extra_params
		pch = 1,
		extra_links = { "AtlasObject", "DatafileIO" },
		extra_files = { "Misc/atlas.rc" }
	})

	setup_atlas_package("DatafileIO", "lib",
	{	-- src
		"",
		"BAR",
		"DDT",
		"SCN",
		"Stream",
		"XMB"
	},{	-- include
	},{	-- extern_libs
		"devil",
		"xerces",
		"zlib"
	},{	-- extra_params
		pch = 1,
	})

end


-- Atlas 'frontend' tool-launching packages
local function setup_atlas_frontend_package (package_name)

	package_create(package_name, "winexe")

	local source_root = "../../../source/tools/atlas/AtlasFrontends/"
	package.files = {
		source_root..package_name..".cpp",
		source_root..package_name..".rc"
	}
	package.trimprefix = source_root
	package.includepaths = { source_root .. ".." }

	-- Platform Specifics
	if OS == "windows" then
		tinsert(package.defines, "_UNICODE")
		tinsert(package.links, "AtlasUI")

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, "no-main")

	else -- Non-Windows, = Unix
		-- TODO
	end

end

function setup_atlas_frontends()
	setup_atlas_frontend_package("ActorEditor")
	setup_atlas_frontend_package("ArchiveViewer")
	setup_atlas_frontend_package("ColourTester")
	setup_atlas_frontend_package("FileConverter")
end


--------------------------------------------------------------------------------
-- tests
--------------------------------------------------------------------------------

function get_all_test_files(root, src_files, hdr_files)

	-- note: lua doesn't have any directory handling functions at all, ugh.
	-- premake's matchrecursive on patterns like *tests*.h doesn't work -
	-- apparently it applies them to filenames, not the complete path.
	-- our workaround is to enumerate all files and manually filter out the
	-- desired */tests/* files. this is a bit slow, but hey.

	local all_files = matchrecursive(root .. "*.h")
	for i,v in all_files do
		-- header file in subdirectory test
		if string.sub(v, -2) == ".h" and string.find(v, "/tests/") then
			-- don't include sysdep tests on the wrong sys
			if not (string.find(v, "/sysdep/win/") and OS ~= "windows") then
				tinsert(hdr_files, v)
				-- add the corresponding source file immediately, instead of
				-- waiting for it to appear after cxxtestgen. this avoids
				-- having to recreate workspace 2x after adding a test.
				tinsert(src_files, string.sub(v, 1, -3) .. ".cpp")
			end
		end
	end
	
end


function setup_tests()

	local src_files = {}
	local hdr_files = {}
	get_all_test_files(source_root, src_files, hdr_files)


	package_create("test_3_gen", "cxxtestgen")
	package.files = hdr_files
	package.rootfile = source_root .. "test_root.cpp"
	if OS == "windows" then
		-- precompiled headers - the header is added to all generated .cpp files
		package.pchheader = "precompiled.h"

		package.rootoptions = "--gui=Win32Gui --runner=ParenPrinter --include="..package.pchheader
		package.testoptions = "--include="..package.pchheader
	else
		package.rootoptions = "--runner=ErrorPrinter --include=pch/test/precompiled.h"
		package.testoptions = "--include=pch/test/precompiled.h"
	end


	package_create("test_2_build", "winexe")
	links = static_lib_names
	tinsert(links, "test_3_gen")
	extra_params = {
		extra_files = { "test_root.cpp" },
		extra_links = links,
	}
	package_add_contents(source_root, {}, {}, extra_params)
	-- note: these are not relative to source_root and therefore can't be included via package_add_contents.
	listconcat(package.files, src_files)
	package_add_extern_libs(used_extern_libs)
	if OS == "windows" then
		-- required for win.cpp's init mechanism
		tinsert(package.linkoptions, "/ENTRY:entry")

		-- from "lowlevel" static lib; must be added here to be linked in
		tinsert(package.files, source_root.."lib/sysdep/win/error_dialog.rc")

		-- precompiled headers
		pch_dir = source_root.."pch/test/"
		package.pchheader = "precompiled.h"
		package.pchsource = "precompiled.cpp"
		tinsert(package.files, { pch_dir.."precompiled.cpp", pch_dir.."precompiled.h" })
		package.rootoptions = "--include=precompiled.h"

	elseif OS == "linux" then

		tinsert(package.links, {
			"fam",
			-- Utilities
			"pthread", "rt",
			-- Debugging
			"bfd", "iberty"
		})
		-- For debug_resolve_symbol
		
	
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }

	
		tinsert(package.libpaths, "/usr/X11R6/lib")
	
		package.rootoptions = "--include=precompiled.h"
	end

	tinsert(package.buildflags, "use-library-dep-inputs")

	
	package_create("test_1_run", "run")
	package.links = { "test_2_build" } -- This determines which project's executable to run

end




-- must come first, so that VC sets it as the default project and therefore
-- allows running via F5 without the "where is the EXE" dialog.
setup_main_exe()

	-- save package global variable for later (will be overwritten by setup_all_libs)
	main_exe_package = package

setup_all_libs()

	-- HACK: add the static libs to the main EXE project. only now (after
	-- setup_all_libs has run) are the lib names known. cannot move
	-- setup_main_exe to run after setup_all_libs (see comment above).
	-- we also don't want to hardcode the names - that would require more
	-- work when changing the static lib breakdown.
	listconcat(main_exe_package.links, static_lib_names)

if options["atlas"] then
	setup_atlas_packages()
	setup_atlas_frontends()
end

if not options["without-tests"] then
	setup_tests()
end
