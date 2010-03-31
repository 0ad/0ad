addoption("atlas", "Include Atlas scenario editor packages")
addoption("collada", "Include COLLADA packages (requires FCollada library)")
addoption("coverage", "Enable code coverage data collection (GCC only)")
addoption("aoe3ed", "Include AoE3Ed")
addoption("icc", "Use Intel C++ Compiler (Linux only; should use either \"--cc icc\" or --without-pch too, and then set CXX=icpc before calling make)")
addoption("outpath", "Location for generated project files")
addoption("without-tests", "Disable generation of test projects")
addoption("without-pch", "Disable generation and usage of precompiled headers")

use_dcdt = false -- disable it since it's a non-Free library

dofile("functions.lua")
dofile("extern_libs.lua")

-- detect CPU architecture (simplistic, currently only supports x86 and amd64
arch = "x86"
if OS == "windows" then
	if os.getenv("PROCESSOR_ARCHITECTURE") == "amd64" or os.getenv("PROCESSOR_ARCHITEW6432") == "amd64" then
		arch = "amd64"
	end
else
	arch = os.getenv("HOSTTYPE")
	if arch == "x86_64" then
		arch = "amd64"
	end
	if not arch then
		os.execute("gcc -dumpmachine > .gccmachine.tmp")
		local f = io.open(".gccmachine.tmp", "r")
		local machine = f:read("*line")
		f:close()
		if string.find(machine, "x86_64") == 1 then
			arch = "amd64"
		elseif string.find(machine, "i.86") == 1 then
			arch = "x86"
		else
			print("WARNING: Cannot determine architecture from GCC, assuming x86")
		end
	end
end

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
	has_broken_pch = false
else
	project.cxxtestpath = "../../build/bin/cxxtestgen.pl"
	if OS == "linux" and arch == "amd64" then
		-- Hack for amd64 linux - tell nasm to product 64-bit elf.
		project.nasmformat = "elf64"
	end

	-- GCC bug (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=10591) - PCH breaks anonymous namespaces
	-- Fixed in 4.2.0, but we have to disable PCH for earlier versions, else
	-- it conflicts annoyingly with wx 2.8 headers.
	-- It's too late to do this test by the time we start compiling the PCH file, so
	-- do the test in this build script instead (which is kind of ugly - please fix if
	-- you have a better idea)
	if not options["icc"] then
		os.execute("gcc -dumpversion > .gccver.tmp")
		local f = io.open(".gccver.tmp", "r")
		major, dot, minor = f:read(1, 1, 1)
		f:close()
		major = 0+major -- coerce to number
		minor = 0+minor
		has_broken_pch = (major < 4 or (major == 4 and minor < 2))
		if has_broken_pch then
			print("WARNING: Detected GCC <4.2 -- disabling PCH for Atlas (will increase build times)")
		end
	end
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

	package.buildflags = { "with-symbols", "no-edit-and-continue" }
	if not options["icc"] then
		-- adds the -Wall compiler flag
		tinsert(package.buildflags, "extra-warnings") -- this causes far too many warnings/remarks on ICC
	end

	-- PremakeWiki says with-symbols and optimize are automatically set for
	-- Debug and Release builds, respectively. doesn't happen though, so do it manually.

	package.config["Debug"].defines = { "DEBUG" }

	package.config["Testing"].buildflags = { "no-runtime-checks" }
	package.config["Testing"].defines = { "TESTING" }

	package.config["Release"].buildflags = { "no-runtime-checks", "optimize-speed" }
	package.config["Release"].defines = { "NDEBUG" }

	-- required for the lowlevel library. must be set from all packages that use it, otherwise it assumes it is
	-- being used as a DLL (which is currently not the case in 0ad)
	tinsert(package.defines, "LIB_STATIC_LINK")
	
	-- various platform-specific build flags
	if OS == "windows" then

		-- use native wchar_t type (not typedef to unsigned short)
		tinsert(package.buildflags, "native-wchar_t")

	else	-- *nix
		if options["icc"] then
			tinsert(package.buildoptions, {
				"-w1",
				-- "-Wabi",
				-- "-Wp64", -- complains about OBJECT_TO_JSVAL which is annoying
				"-Wpointer-arith",
				"-Wreturn-type",
				-- "-Wshadow",
				"-Wuninitialized",
				"-Wunknown-pragmas",
				"-Wunused-function",
				"-wd1292", -- avoid lots of 'attribute "__nonnull__" ignored'
			})
			tinsert(package.config["Debug"].buildoptions, {
				"-O0", -- ICC defaults to -O2
			})
			if OS == "macosx" then
				tinsert(package.linkoptions, {"-multiply_defined","suppress"})
			end
		else
			tinsert(package.buildoptions, {
				-- enable most of the standard warnings
				"-Wno-switch",		-- enumeration value not handled in switch (this is sometimes useful, but results in lots of noise)
				"-Wno-reorder",		-- order of initialization list in constructors (lots of noise)
				"-Wno-invalid-offsetof",	-- offsetof on non-POD types (see comment in renderer/PatchRData.cpp)

				"-Wextra",
				"-Wno-missing-field-initializers",	-- (this is common in external headers we can't fix)

				-- add some other useful warnings that need to be enabled explicitly
				"-Wunused-parameter",
				"-Wredundant-decls",	-- (useful for finding some multiply-included header files)
				-- "-Wformat=2",		-- (useful sometimes, but a bit noisy, so skip it by default)
				-- "-Wcast-qual",		-- (useful for checking const-correctness, but a bit noisy, so skip it by default)
				"-Wnon-virtual-dtor",	-- (sometimes noisy but finds real bugs)

				-- enable security features (stack checking etc) that shouldn't have
				-- a significant effect on performance and can catch bugs
				"-fstack-protector-all",
				"-D_FORTIFY_SOURCE=2",

				-- always enable strict aliasing (useful in debug builds because of the warnings)
				"-fstrict-aliasing",

				-- do something (?) so that ccache can handle compilation with PCH enabled
				"-fpch-preprocess",

				-- enable SSE intrinsics
				"-msse",

				-- don't omit frame pointers (for now), because performance will be impacted
				-- negatively by the way this breaks profilers more than it will be impacted
				-- positively by the optimisation
				"-fno-omit-frame-pointer",
			})
			if OS == "linux" then
				tinsert(package.linkoptions, {
					"-Wl,--no-undefined",
					"-Wl,--as-needed",
				})
			end
			if options["coverage"] then
				tinsert(package.buildoptions, {"-fprofile-arcs", "-ftest-coverage"})
				tinsert(package.links, "gcov")
			end
		end

		if OS == "linux" then
			-- To use our local SpiderMonkey library, it needs to be part of the runtime dynamic linker
			-- path. So add the executable path with -rpath:
			-- (TODO: is this a sane way to do it?)
			tinsert(package.linkoptions, {"-Wl,-rpath='$$ORIGIN'"}) -- use Makefile escaping of '$'
		end

		tinsert(package.buildoptions, {
			-- Hide symbols in dynamic shared objects by default, for efficiency and for equivalence with
			-- Windows - they should be exported explicitly with __attribute__ ((visibility ("default")))
			"-fvisibility=hidden",
		})

		-- X11 includes may be installed in one of a gadzillion of three places
		-- Famous last words: "You can't include too much! ;-)"
		package.includepaths = {
			"/usr/X11R6/include/X11",
			"/usr/X11R6/include",
			"/usr/include/X11"
		}
		package.libpaths = {
			"/usr/X11R6/lib"
		}

		if OS == "linux" and options["icc"] then
			tinsert(package.libpaths, "/usr/i686-pc-linux-gnu/lib") -- needed for ICC to find libbfd
		end
		
		package.defines = {
			-- "CONFIG_USE_MMGR",
		}
		if use_dcdt then
			tinsert(package.defines, "USE_DCDT")
		end
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

	package.includepaths = {}

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

	-- Put the project-specific PCH directory at the start of the
	-- include path, so '#include "precompiled.h"' will look in
	-- there first
	if not extra_params["no_default_pch"] then
		tinsert(package.includepaths, source_root .. "pch/" .. package.name)
	end

	-- next is source root dir, for absolute (nonrelative) includes
	-- (e.g. "lib/precompiled.h")
	tinsert(package.includepaths, source_root)

	for i,v in pairs(rel_include_dirs) do
		tinsert(package.includepaths, source_root .. v)
	end


	if extra_params["extra_files"] then
		for i,v in pairs(extra_params["extra_files"]) do
			tinsert(package.files, source_root .. v)
		end
	end

	if extra_params["extra_links"] then
		listconcat(package.links, extra_params["extra_links"])
	end

end

-- Detect and set up PCH for the current package
function package_setup_pch(pch_dir, header, source)
	if OS == "windows" or not options["without-pch"] then
		package.pchheader = header -- "precompiled.h"
		package.pchsource = source -- "precompiled.cpp"
		if pch_dir then
			tinsert(package.files, {
				pch_dir..header,
				pch_dir..source
			})
		end
		for i,v in pairs(project.configs) do
			tinsert(package.config[v].defines, "USING_PCH")
		end
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
function setup_static_lib_package (package_name, rel_source_dirs, extern_libs, extra_params)

	package_create(package_name, "lib")
	package_add_contents(source_root, rel_source_dirs, {}, extra_params)
	package_add_extern_libs(extern_libs)

	if not extra_params["no_default_link"] then
		tinsert(static_lib_names, package_name)
	end

	if OS == "windows" then
		tinsert(package.buildflags, "no-rtti")
	end

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
	if not extra_params["no_default_pch"] then
		pch_dir = source_root.."pch/"..package_name.."/"
		package_setup_pch(pch_dir, "precompiled.h", "precompiled.cpp")
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
		"enet",
		"boost",	-- dragged in via server->simulation.h->random
	}
	setup_static_lib_package("network", source_dirs, extern_libs, {})


	source_dirs = {
		"simulation2",
		"simulation2/components",
		"simulation2/helpers",
		"simulation2/scripting",
		"simulation2/serialization",
		"simulation2/system",
		"simulation2/testcomponents",
	}
	extern_libs = {
		"boost",
		"spidermonkey",
	}
	setup_static_lib_package("simulation2", source_dirs, extern_libs, {})


	source_dirs = {
		"scriptinterface",
	}
	extern_libs = {
		"boost",
		"spidermonkey",
		"valgrind",
	}
	setup_static_lib_package("scriptinterface", source_dirs, extern_libs, {})


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
		"maths/scripting",
	}
	if use_dcdt then
		tinsert(source_dirs, "dcdt/se")
	end
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"libxml2",
		"opengl",
		"zlib",
		"boost",
		"enet",
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
		"spidermonkey",
		"boost"
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
	if OS == "windows" then	-- add JS files to allow VS find-in-solution and VA open-file-in-workspace
		tinsert(source_dirs, "../binaries/data/mods/public/gui/test")
	end
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"opengl",
		"boost"
	}
	setup_static_lib_package("gui", source_dirs, extern_libs, {})


	source_dirs = {
		"lib",
		"lib/allocators",
		"lib/external_libraries",
		"lib/file",
		"lib/file/archive",
		"lib/file/common",
		"lib/file/io",
		"lib/file/vfs",
		"lib/posix",
		"lib/res",
		"lib/res/graphics",
		"lib/res/sound",
		"lib/sysdep",
		"lib/tex"
	}
	extern_libs = {
		"boost",
		"sdl",
		"opengl",
		"libpng",
		"zlib",
		"openal",
		"vorbis",
		"libjpg",
		"valgrind",
		"cxxtest",
	}

	-- CPU architecture-specific
	if arch == "amd64" then
		tinsert(source_dirs, "lib/sysdep/arch/amd64");
		tinsert(source_dirs, "lib/sysdep/arch/x86_x64");
	else
		tinsert(source_dirs, "lib/sysdep/arch/ia32");
		tinsert(source_dirs, "lib/sysdep/arch/x86_x64");
	end
	
	-- OS-specific
	sysdep_dirs = {
		linux = { "lib/sysdep/os/linux", "lib/sysdep/os/unix", "lib/sysdep/os/unix/x" },
		-- note: RC file must be added to main_exe package.
		-- note: don't add "lib/sysdep/os/win/aken.cpp" because that must be compiled with the DDK.
		windows = { "lib/sysdep/os/win", "lib/sysdep/os/win/wposix", "lib/sysdep/os/win/whrt" },
		macosx = { "lib/sysdep/os/osx", "lib/sysdep/os/unix" },
	}
	for i,v in pairs(sysdep_dirs[OS]) do
		tinsert(source_dirs, v);
	end

	-- runtime-library-specific
	if options["target"] == "gnu" then
		tinsert(source_dirs, "lib/sysdep/rtl/gcc");
	else
		tinsert(source_dirs, "lib/sysdep/rtl/msc");
	end

	setup_static_lib_package("lowlevel", source_dirs, extern_libs, {})


	-- CxxTest mock function support
	extern_libs = {
		"boost",
		"cxxtest",
	}
	-- 'real' implementations, to be linked against the main executable
	setup_static_lib_package("mocks_real", {}, extern_libs, { no_default_link = 1, no_default_pch = 1 })
	listconcat(package.files, matchfiles(source_root.."mocks/*.h", source_root.."mocks/*_real.cpp"))
	-- 'test' implementations, to be linked against the test executable
	setup_static_lib_package("mocks_test", {}, extern_libs, { no_default_link = 1, no_default_pch = 1 })
	listconcat(package.files, matchfiles(source_root.."mocks/*.h", source_root.."mocks/*_test.cpp"))
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
	"libxml2",

	"openal",
	"vorbis",

	"boost",
	"cxxtest",
	"comsuppw",
	"enet",
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

	tinsert(package.links, "mocks_real")

	-- Platform Specifics
	if OS == "windows" then

		tinsert(package.files, source_root.."lib/sysdep/os/win/icon.rc")
		-- from "lowlevel" static lib; must be added here to be linked in
		tinsert(package.files, source_root.."lib/sysdep/os/win/error_dialog.rc")

		-- VS2005 generates its own manifest, but earlier ones need us to add it manually
		if (options["target"] == "vs2002" or options["target"] == "vs2003") then
			tinsert(package.files, source_root.."lib/sysdep/os/win/manifest.rc")
		end

		tinsert(package.buildflags, "no-rtti")

		package.linkoptions = {
			-- wraps main thread in a __try block(see wseh.cpp). replace with mainCRTStartup if that's undesired.
			"/ENTRY:wseh_EntryPoint",

			-- see wstartup.h
			"/INCLUDE:_wstartup_InitAndRegisterShutdown",

			-- delay loading of various Windows DLLs (not specific to any of the
			-- external libraries; those are handled separately)
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
			"rt",
			-- Debugging
			"bfd", "iberty",
			-- Dynamic libraries (needed for linking for gold)
			"dl",
		})

		-- Threading support
		tinsert(package.buildoptions, "-pthread")
		tinsert(package.linkoptions, "-pthread")
	
		-- For debug_resolve_symbol
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }
	elseif OS == "macosx" then
		-- Libraries
		tinsert(package.links, {			-- Utilities
			"pthread"
		})
	end
end


--------------------------------------------------------------------------------
-- atlas
--------------------------------------------------------------------------------

-- setup a typical Atlas component package
-- extra_params: as in package_add_contents; also zero or more of the following:
-- * pch: (any type) set precompiled.h and .cpp as PCH
function setup_atlas_package(package_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	local source_root = "../../../source/tools/atlas/" .. package_name .. "/"
	package_create(package_name, target_type)

	-- Don't add the default 'sourceroot/pch/projectname' for finding PCH files
	extra_params["no_default_pch"] = 1

	package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)
	package_add_extern_libs(extern_libs)

	if extra_params["pch"] then
		package_setup_pch(nil, "precompiled.h", "precompiled.cpp");
	end

	if options["aoe3ed"] then
		tinsert(package.defines, "USE_AOE3ED")
	end

	-- Platform Specifics
	if OS == "windows" then

		tinsert(package.defines, "_UNICODE")

		-- Link to required libraries
		package.links = { "winmm", "comctl32", "rpcrt4", "delayimp", "ws2_32" }

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, "no-main")

		if extra_params["extra_links"] then 
			listconcat(package.links, extra_params["extra_links"]) 
		end

	elseif OS == "linux" then
		tinsert(package.buildoptions, "-rdynamic")
		tinsert(package.buildoptions, "-fPIC")
		tinsert(package.linkoptions, "-fPIC")
		tinsert(package.linkoptions, "-rdynamic")

		if extra_params["no_unused_warnings"] then
				if not options["icc"] then
					tinsert(package.buildoptions, "-Wno-unused-parameter")
				end
		end
	end

end


-- build all Atlas component packages
function setup_atlas_packages()

	setup_atlas_package("AtlasObject", "lib",
	{	-- src
		""
	},{	-- include
	},{	-- extern_libs
		"libxml2",
		"wxwidgets"
	},{	-- extra_params
	})

	setup_atlas_package("AtlasScript", "lib",
	{	-- src
		""
	},{	-- include
		".."
	},{	-- extern_libs
		"boost",
		"spidermonkey",
		"valgrind",
		"wxwidgets",
	},{	-- extra_params
	})

	setup_atlas_package("wxJS", "lib",
	{	-- src
		"",
		"common",
		"ext",
		"gui",
		"gui/control",
		"gui/event",
		"gui/misc",
		"io",
	},{	-- include
	},{	-- extern_libs
		"spidermonkey",
		"wxwidgets"
	},{	-- extra_params
		pch = (not has_broken_pch),
		no_unused_warnings = 1, -- wxJS has far too many and we're never going to fix them, so just ignore them
	})

	atlas_src = {
		"ActorEditor",
		"ActorViewer",
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
		"ErrorReporter",
		"General",
		"General/VideoRecorder",
		"Misc",
		"ScenarioEditor",
		"ScenarioEditor/Sections/Common",
		"ScenarioEditor/Sections/Cinematic",
		"ScenarioEditor/Sections/Environment",
		"ScenarioEditor/Sections/Map",
		"ScenarioEditor/Sections/Object",
		"ScenarioEditor/Sections/Terrain",
		"ScenarioEditor/Sections/Trigger",
		"ScenarioEditor/Tools",
		"ScenarioEditor/Tools/Common",
	}
	atlas_extra_links = {
		"AtlasObject",
		"AtlasScript",
		"wxJS",
	}
	if options["aoe3ed"] then
		tinsert(atlas_src, "ArchiveViewer")
		tinsert(atlas_src, "FileConverter")
		tinsert(atlas_extra_links, "DatafileIO")
	end

	setup_atlas_package("AtlasUI", "dll", atlas_src,
	{	-- include
		"..",
		"CustomControls",
		"Misc"
	},{	-- extern_libs
		"boost",
		"devil",
		--"ffmpeg", -- disabled for now because it causes too many build difficulties
		"libxml2",
		"spidermonkey",
		"wxwidgets",
		"comsuppw",
		"zlib",
		"x11",
	},{	-- extra_params
		pch = (not has_broken_pch),
		extra_links = atlas_extra_links,
		extra_files = { "Misc/atlas.rc" }
	})

	if options["aoe3ed"] then
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

end


-- Atlas 'frontend' tool-launching packages
function setup_atlas_frontend_package (package_name)

	package_create(package_name, "winexe")
	package_add_extern_libs({
		"spidermonkey",
	})

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

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, "no-main")

	else -- Non-Windows, = Unix
		if options["aoe3ed"] then
			tinsert(package.links, "DatafileIO")
		end
		tinsert(package.links, "AtlasObject")
	end

	tinsert(package.links, "AtlasUI")

end

function setup_atlas_frontends()
	setup_atlas_frontend_package("ActorEditor")
	setup_atlas_frontend_package("ColourTester")
	if options["aoe3ed"] then
		setup_atlas_frontend_package("ArchiveViewer")
		setup_atlas_frontend_package("FileConverter")
	end
end


--------------------------------------------------------------------------------
-- collada
--------------------------------------------------------------------------------

function setup_collada_package(package_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	package_create(package_name, target_type)

	-- Don't add the default 'sourceroot/pch/projectname' for finding PCH files
	extra_params["no_default_pch"] = 1

	package_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)
	package_add_extern_libs(extern_libs)

	if extra_params["pch"] then
		package_setup_pch(nil, "precompiled.h", "precompiled.cpp");
	end

	-- Platform Specifics
	if OS == "windows" then

		-- required to use WinMain() on Windows, otherwise will default to main()
		tinsert(package.buildflags, "no-main")

		if extra_params["extra_links"] then 
			listconcat(package.links, extra_params["extra_links"]) 
		end
	end

	if OS == "linux" then
		tinsert(package.defines, "LINUX");

		-- FCollada is not aliasing-safe, so disallow dangerous optimisations
		-- (TODO: It'd be nice to fix FCollada, but that looks hard)
		tinsert(package.buildoptions, "-fno-strict-aliasing")

		tinsert(package.buildoptions, "-rdynamic")
		tinsert(package.linkoptions, "-rdynamic")
	elseif OS == "macosx" then
		-- define MACOS-something? 

		-- On OSX, fcollada uses a few utility functions from coreservices
		tinsert(package.linkoptions, "-framework CoreServices")
	end

end

-- build all Collada component packages
function setup_collada_packages()

	setup_collada_package("Collada", "dll",
	{	-- src
		"collada"
	},{	-- include
	},{	-- extern_libs
		"fcollada",
		"dl",
		"libxml2",
	},{	-- extra_params
		pch = 1,
	})

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
	for i,v in pairs(all_files) do
		-- header file in subdirectory test
		if string.sub(v, -2) == ".h" and string.find(v, "/tests/") then
			-- don't include sysdep tests on the wrong sys
			-- don't include Atlas tests unless Atlas is being built
			if not (string.find(v, "/sysdep/os/win/") and OS ~= "windows") and
			   not (string.find(v, "/tools/atlas/") and not options["atlas"])
			then
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


	package_create("test_gen", "cxxtestgen")
	package.files = hdr_files
	package.rootfile = source_root .. "test_root.cpp"
	package.testoptions = "--have-std"
	package.rootoptions = "--have-std"
	if OS == "windows" then
		package.rootoptions = package.rootoptions .. " --gui=PsTestWrapper --runner=Win32ODSPrinter"
	else
		package.rootoptions = package.rootoptions .. " --gui=PsTestWrapper --runner=ErrorPrinter"
	end
	-- precompiled headers - the header is added to all generated .cpp files
	-- note that the header isn't actually precompiled here, only #included
	-- so that the build stage can use it as a precompiled header.
	include = " --include=precompiled.h"
	package.rootoptions = package.rootoptions .. include
	package.testoptions = package.testoptions .. include
	tinsert(package.buildflags, "no-manifest")

	package_create("test", "winexe")
	links = static_lib_names
	tinsert(links, "test_gen")
	if options["atlas"] then
		tinsert(links, "AtlasObject")
	end
	extra_params = {
		extra_files = { "test_root.cpp", "test_setup.cpp" },
		extra_links = links,
	}
	package_add_contents(source_root, {}, {}, extra_params)
	-- note: these are not relative to source_root and therefore can't be included via package_add_contents.
	listconcat(package.files, src_files)
	package_add_extern_libs(used_extern_libs)
	tinsert(package.links, "mocks_test")

	if OS == "windows" then
		-- from "lowlevel" static lib; must be added here to be linked in
		tinsert(package.files, source_root.."lib/sysdep/os/win/error_dialog.rc")

		-- see wstartup.h
		tinsert(package.linkoptions, "/INCLUDE:_wstartup_InitAndRegisterShutdown")

	elseif OS == "linux" then

		tinsert(package.links, {
			"fam",
			-- Utilities
			"rt",
			-- Debugging
			"bfd", "iberty",
			-- Dynamic libraries (needed for linking for gold)
			"dl",
		})

		-- Threading support
		tinsert(package.buildoptions, "-pthread")
		tinsert(package.linkoptions, "-pthread")

		-- For debug_resolve_symbol
		package.config["Debug"].linkoptions = { "-rdynamic" }
		package.config["Testing"].linkoptions = { "-rdynamic" }
	
		tinsert(package.includepaths, source_root .. "pch/test/")
	end

	package_setup_pch(
		source_root .. "pch/test/",
		"precompiled.h",
		"precompiled.cpp");

	tinsert(package.buildflags, "use-library-dep-inputs")

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

if options["collada"] then
	setup_collada_packages()
end

if not options["without-tests"] then
	setup_tests()
end
