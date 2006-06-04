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

if OS == "windows" then
	project.nasmpath = "../../build/bin/nasm.exe"
end

-- note: Whenever "extern_libs" are specified, they are a table ("array") of
-- library names, which are assumed to be subdirectories of this path.
-- They must each contain an "include" [on Windows: also "lib"] subdirectory.
--
-- When adding new ones, make sure to also add them in the main_exe link step
-- (it'd be nice to automate this, but lib names and debug suffixes differ wildly).
libraries_dir = "../../../libraries/"
source_root = "../../../source/" -- default for most projects - overridden by local in others

-- Rationale: packages should not have any additional include paths except for
-- those required by external libraries. Instead, we should always write the
-- full relative path, e.g. #include "maths/Vector3d.h". This avoids confusion
-- ("which file is meant?") and avoids enormous include path lists.

-- packages: engine static libs, main exe, atlas, atlas frontends
-- the engine libs are necessary because they are referenced by the separate
-- test workspace.


-- create a package and set the attributes that are common to all packages.
function create_package_with_cflags (package_name, target_type)

 	-- Note: don't store in local variable. A global variable needs to
 	-- be set for Premake's use; it is implicitly used in e.g. matchfiles()
	package = newpackage()
	package.path = project.path
	package.language = "c++"
	package.name = package_name
	package.kind = target_type


	------------- target --------------

	-- Note: On Windows, ".exe" is added on the end, on unices the name is used directly
	package.config["Debug"  ].target = package_name.."_dbg"
	package.config["Testing"].target = package_name.."_test"
	package.config["Release"].target = package_name

	local obj_dir_prefix = "obj/"..package_name.."_"
	package.config["Debug"  ].objdir = obj_dir_prefix.."Debug"
	package.config["Testing"].objdir = obj_dir_prefix.."Test"
	package.config["Release"].objdir = obj_dir_prefix.."Release"


	------------- build flags --------------

	package.buildflags = { "with-symbols", "no-edit-and-continue", "extra-warnings" }

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.

	package.config["Testing"].buildflags = { "no-runtime-checks" }
	package.config["Testing"].defines = { "TESTING" }

	package.config["Release"].buildflags = { "no-runtime-checks", "optimize" }
	package.config["Release"].defines = { "NDEBUG" }

	-- various platform-specific build flags
	if OS == "windows" then
		table.insert(package.buildflags, "no-rtti")

		-- use native wchar_t type (not typedef to unsigned short)
		table.insert(package.buildflags, { "native-wchar_t" })
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

	return package
end


function package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	-- We don't want the VC project to be deeply nested (once for each
	-- folder in source_root). Therefore, remove the source_root
	-- directory from the filenames (where those
	-- names are used by Premake to construct the project tree), but set
	-- 'trimprefix' (with Premake altered to recognise that) so the project
	-- will still point to the correct filenames.
	package.trimprefix = source_root
	package.files = sourcesfromdirs(source_root, rel_source_dirs)

	if extra_params["extrasource"] then
		for i,v in extra_params["extrasource"] do
			table.insert(package.files, source_root .. v)
		end
	end

	package.includepaths = {}

	-- Put the project-specific PCH directory at the start of the
	-- include path, so '#include "precompiled.h"' will look in
	-- there first
	if not extra_params["no_default_pch"] then
		table.insert(package.includepaths, source_root .. "pch/" .. package.name)
	end

	-- next is source root dir, for absolute (nonrelative) includes
	-- (e.g. "lib/precompiled.h")
	table.insert(package.includepaths, source_root)

	for i,v in rel_include_dirs do
		table.insert(package.includepaths, source_root .. v)
	end

	-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
	for i,v in extern_libs do
		table.insert(package.includepaths, libraries_dir..v.."/include")
		table.insert(package.libpaths,     libraries_dir..v.."/lib")
	end

end


--------------------------------------------------------------------------------
-- engine static libraries
--------------------------------------------------------------------------------

-- the engine is split up into several static libraries. this eases separate
-- distribution of those components, reduces dependencies a bit, and can]
-- also speed up builds.

-- note: rel_source_dirs and rel_include_dirs are relative to global source_root.
function setup_static_lib_package (package_name, rel_source_dirs, extern_libs, extra_params)
	create_package_with_cflags(package_name, "lib")

	package_add_contents(source_root, rel_source_dirs, {}, extern_libs, extra_params)

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
		table.insert(package.files, pch_dir.."precompiled.cpp")

	end
end

-- this is where the source tree is chopped up into static libs.
-- can be changed fairly easily; just copy+paste here, then add the new
-- library to the setup_main_exe package.links .
function setup_all_libs ()
	-- relative to global source_root.
	local source_dirs = {}
	-- names of external libraries used (see libraries_dir comment)
	local extern_libs = {}

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
		"xerces",
		"misc",
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
		"misc",	-- i.e. OpenGL. our newer version of these headers is required.
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
		"misc",	-- i.e. OpenGL. our newer version of these headers is required.
		"spidermonkey"
	}
	setup_static_lib_package("atlas", source_dirs, extern_libs, {})


	source_dirs = {
		"gui",
		"gui/scripting"
	}
	extern_libs = {
		"spidermonkey",
		"misc",	-- i.e. OpenGL. our newer version of these headers is required.
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
		"misc",	-- i.e. OpenGL. our newer version of these headers is required.
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
		table.insert(package.files, sourcesfromdirs(source_root, {"lib/sysdep/win"}))
		-- note: RC file must be added to main_exe package.
	else
		table.insert(package.files, sourcesfromdirs(source_root, {"lib/sysdep/unix"}))
		-- (X11 include path isn't standard across distributions)
		table.insert(package.includepaths, { "/usr/X11R6/include/X11" } )
		table.insert(package.includepaths, { "/usr/include/X11" } )
	end
end


-- Bundles static libs together with main.cpp and builds game executable.
function setup_main_exe ()

	create_package_with_cflags("pyrogenesis", "winexe")
	
	package.files = { source_root .. "main.cpp" }
	package.includepaths = { source_root }

	-- For VS2005, tell the linker to use the libraries' .obj files instead of
	-- the .lib, to allow incremental linking.
	-- (Reduces re-link time from ~20 seconds to ~2 secs)
	table.insert(package.buildflags, "use-library-dep-inputs")

	package.links = {
		"engine",
		"graphics",
		"i18n",
		"atlas",
		"gui",
		"lowlevel"
	}

	package.libpaths = {}

	-- Platform Specifics
	if OS == "windows" then
		-- see libraries_dir comment
		local extern_libs = {
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
			"cxxtest",
			"directx"
		}
		-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
		for i,v in extern_libs do
			table.insert(package.includepaths, libraries_dir..v.."/include")
			table.insert(package.libpaths,     libraries_dir..v.."/lib")
		end


		table.insert(package.links, { "opengl32" })
		table.insert(package.files, {source_root.."lib/sysdep/win/icon.rc"})
		-- from lowlevel static lib; must be added here to be linked in
		table.insert(package.files, {source_root.."lib/sysdep/win/error_dialog.rc"})

		-- VS2005 generates its own manifest, but earlier ones need us to add it manually
		if (options["target"] == "vs2002" or options["target"] == "vs2003") then
			table.insert(package.files, {source_root.."lib/sysdep/win/manifest.rc"})
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

		-- 'Testing' uses 'Debug' DLLs
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

	else -- Non-Windows, = Unix

		-- Libraries
		table.insert(package.links, {
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
		})
		-- For debug_resolve_symbol
--		package.config["Debug"].links = { "bfd", "iberty" }
--		package.config["Testing"].links = { "bfd", "iberty" }
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }

		table.insert(package.libpaths, { "/usr/X11R6/lib" } )
	end
end



---------------- Main Atlas package ----------------

function setuppackage_atlas(package_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, flags)

	local source_root = "../../../source/tools/atlas/" .. package_name .. "/"
	create_package_with_cflags(package_name, target_type)

	-- Don't add the default 'sourceroot/pch/projectname' for finding PCH files
	flags["no_default_pch"] = 1

	package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extern_libs, flags)

	-- Platform Specifics
	if OS == "windows" then

		table.insert(package.defines, "_UNICODE")

		-- Handle wx specially
		if flags["wx"] then
			table.insert(package.includepaths, libraries_dir.."wxwidgets/include/msvc")
			table.insert(package.includepaths, libraries_dir.."wxwidgets/include")
			table.insert(package.libpaths, libraries_dir.."wxwidgets/lib/vc_lib")
		end

		-- Link to required libraries
		package.links = { "winmm", "comctl32", "rpcrt4" }
		package.config["Debug"].links = { "wxmsw26ud_gl" }
		package.config["Release"].links = { "wxmsw26u_gl" }
		if flags["depends"] then
			listconcat(package.links, flags["depends"])
		end

		-- required to use WinMain() on Windows, otherwise will default to main()
		table.insert(package.buildflags, { "no-main" })

		if flags["pch"] then
			package.pchheader = "stdafx.h"
			package.pchsource = "stdafx.cpp"
		end

	else -- Non-Windows, = Unix
		-- TODO
	end

end

---------------- Atlas 'frontend' tool-launching packages ----------------

function setuppackage_atlas_frontend (package_name)

	create_package_with_cflags(package_name, "winexe")

	local source_root = "../../../source/tools/atlas/AtlasFrontends/"
	package.files = {
		source_root..package_name..".cpp",
		source_root..package_name..".rc"
	}
	package.trimprefix = source_root
	package.includepaths = { source_root..".." }

	-- Platform Specifics
	if OS == "windows" then
		table.insert(package.defines, "_UNICODE")
		table.insert(package.links, "AtlasUI")

		-- required to use WinMain() on Windows, otherwise will default to main()
		table.insert(package.buildflags, { "no-main" })

	else -- Non-Windows, = Unix
		-- TODO
	end

end

---------------- Atlas packages ----------------

function setuppackages_atlas()
	setuppackage_atlas("AtlasObject", "lib",
	{	-- src
		""
	},{	-- include
	},{	-- extern_libs
		"xerces"
	},{	-- flags
	})

	setuppackage_atlas("AtlasUI", "dll",
	{	-- src
		"ActorEditor",
		"ArchiveViewer",
		"ColourTester",
		"CustomControls/Buttons",
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
		"xerces"
	},{	-- flags
		pch = 1,
		wx = 1,
		depends = { "AtlasObject", "DatafileIO" },
		extrasource = { "Misc/atlas.rc" }
	})

	setuppackage_atlas("DatafileIO", "lib",
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
	},{	-- flags
		pch = 1,
	})

	setuppackage_atlas_frontend("ActorEditor")
	setuppackage_atlas_frontend("ArchiveViewer")
	setuppackage_atlas_frontend("ColourTester")
	setuppackage_atlas_frontend("FileConverter")

end

--------------------------------

-- must come first, so that VC sets it as the default project and therefore
-- allows running via F5 without the "where is the EXE" dialog.
setup_main_exe()
setup_all_libs()

if options["atlas"] then
	setuppackages_atlas()
end
