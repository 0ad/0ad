newoption { trigger = "android", description = "Use non-working Android cross-compiling mode" }
newoption { trigger = "atlas", description = "Include Atlas scenario editor projects" }
newoption { trigger = "coverage", description = "Enable code coverage data collection (GCC only)" }
newoption { trigger = "gles", description = "Use non-working OpenGL ES 2.0 mode" }
newoption { trigger = "icc", description = "Use Intel C++ Compiler (Linux only; should use either \"--cc icc\" or --without-pch too, and then set CXX=icpc before calling make)" }
newoption { trigger = "jenkins-tests", description = "Configure CxxTest to use the XmlPrinter runner which produces Jenkins-compatible output" }
newoption { trigger = "minimal-flags", description = "Only set compiler/linker flags that are really needed. Has no effect on Windows builds" }
newoption { trigger = "outpath", description = "Location for generated project files" }
newoption { trigger = "with-system-mozjs38", description = "Search standard paths for libmozjs38, instead of using bundled copy" }
newoption { trigger = "with-system-nvtt", description = "Search standard paths for nvidia-texture-tools library, instead of using bundled copy" }
newoption { trigger = "without-audio", description = "Disable use of OpenAL/Ogg/Vorbis APIs" }
newoption { trigger = "without-lobby", description = "Disable the use of gloox and the multiplayer lobby" }
newoption { trigger = "without-miniupnpc", description = "Disable use of miniupnpc for port forwarding" }
newoption { trigger = "without-nvtt", description = "Disable use of NVTT" }
newoption { trigger = "without-pch", description = "Disable generation and usage of precompiled headers" }
newoption { trigger = "without-tests", description = "Disable generation of test projects" }

-- OS X specific options
newoption { trigger = "macosx-bundle", description = "Enable OSX bundle, the argument is the bundle identifier string (e.g. com.wildfiregames.0ad)" }
newoption { trigger = "macosx-version-min", description = "Set minimum required version of the OS X API, the build will possibly fail if an older SDK is used, while newer API functions will be weakly linked (i.e. resolved at runtime)" }
newoption { trigger = "sysroot", description = "Set compiler system root path, used for building against a non-system SDK. For example /usr/local becomes SYSROOT/user/local" }

-- Windows specific options
newoption { trigger = "build-shared-glooxwrapper", description = "Rebuild glooxwrapper DLL for Windows. Requires the same compiler version that gloox was built with" }
newoption { trigger = "use-shared-glooxwrapper", description = "Use prebuilt glooxwrapper DLL for Windows" }
newoption { trigger = "large-address-aware", description = "Make the executable large address aware. Do not use for development, in order to spot memory issues easily" }

-- Install options
newoption { trigger = "bindir", description = "Directory for executables (typically '/usr/games'); default is to be relocatable" }
newoption { trigger = "datadir", description = "Directory for data files (typically '/usr/share/games/0ad'); default is ../data/ relative to executable" }
newoption { trigger = "libdir", description = "Directory for libraries (typically '/usr/lib/games/0ad'); default is ./ relative to executable" }

-- Root directory of project checkout relative to this .lua file
rootdir = "../.."

dofile("extern_libs5.lua")

-- detect compiler for non-Windows
if os.istarget("macosx") then
	cc = "clang"
elseif os.istarget("linux") and _OPTIONS["icc"] then
	cc = "icc"
elseif not os.istarget("windows") then
	cc = os.getenv("CC")
	if cc == nil or cc == "" then
		local hasgcc = os.execute("which gcc > .gccpath")
		local f = io.open(".gccpath", "r")
		local gccpath = f:read("*line")
		f:close()
		os.execute("rm .gccpath")
		if gccpath == nil then
			cc = "clang"
		else
			cc = "gcc"
		end
	end
end

-- detect CPU architecture (simplistic, currently only supports x86, amd64 and ARM)
arch = "x86"
if _OPTIONS["android"] then
	arch = "arm"
elseif os.istarget("windows") then
	if os.getenv("PROCESSOR_ARCHITECTURE") == "amd64" or os.getenv("PROCESSOR_ARCHITEW6432") == "amd64" then
		arch = "amd64"
	end
else
	arch = os.getenv("HOSTTYPE")
	if arch == "x86_64" or arch == "amd64" then
		arch = "amd64"
	else
		os.execute(cc .. " -dumpmachine > .gccmachine.tmp")
		local f = io.open(".gccmachine.tmp", "r")
		local machine = f:read("*line")
		f:close()
		if string.find(machine, "x86_64") == 1 or string.find(machine, "amd64") == 1 then
			arch = "amd64"
		elseif string.find(machine, "i.86") == 1 then
			arch = "x86"
		elseif string.find(machine, "arm") == 1 then
			arch = "arm"
		elseif string.find(machine, "aarch64") == 1 then
			arch = "aarch64"
		else
			print("WARNING: Cannot determine architecture from GCC, assuming x86")
		end
	end
end

-- Set up the Workspace
workspace "pyrogenesis"
targetdir(rootdir.."/binaries/system")
libdirs(rootdir.."/binaries/system")
if not _OPTIONS["outpath"] then
	error("You must specify the 'outpath' parameter")
end
location(_OPTIONS["outpath"])
configurations { "Release", "Debug" }

source_root = rootdir.."/source/" -- default for most projects - overridden by local in others

-- Rationale: projects should not have any additional include paths except for
-- those required by external libraries. Instead, we should always write the
-- full relative path, e.g. #include "maths/Vector3d.h". This avoids confusion
-- ("which file is meant?") and avoids enormous include path lists.


-- projects: engine static libs, main exe, atlas, atlas frontends, test.

--------------------------------------------------------------------------------
-- project helper functions
--------------------------------------------------------------------------------

function project_set_target(project_name)

	-- Note: On Windows, ".exe" is added on the end, on unices the name is used directly

	local obj_dir_prefix = _OPTIONS["outpath"].."/obj/"..project_name.."_"

	filter "Debug"
		objdir(obj_dir_prefix.."Debug")
		targetsuffix("_dbg")

	filter "Release"
		objdir(obj_dir_prefix.."Release")

	filter { }

end


function project_set_build_flags()

	editandcontinue "Off"

	if not _OPTIONS["minimal-flags"] then
		symbols "On"
	end

	if cc ~= "icc" and (os.istarget("windows") or not _OPTIONS["minimal-flags"]) then
		-- adds the -Wall compiler flag
		warnings "Extra" -- this causes far too many warnings/remarks on ICC
	end

	-- disable Windows debug heap, since it makes malloc/free hugely slower when
	-- running inside a debugger
	if os.istarget("windows") then
		debugenvs { "_NO_DEBUG_HEAP=1" }
	end

	filter "Debug"
		defines { "DEBUG" }

	filter "Release"
		if os.istarget("windows") or not _OPTIONS["minimal-flags"] then
			optimize "Speed"
		end
		defines { "NDEBUG", "CONFIG_FINAL=1" }

	filter { }

	if _OPTIONS["gles"] then
		defines { "CONFIG2_GLES=1" }
	end

	if _OPTIONS["without-audio"] then
		defines { "CONFIG2_AUDIO=0" }
	end

	if _OPTIONS["without-nvtt"] then
		defines { "CONFIG2_NVTT=0" }
	end

	if _OPTIONS["without-lobby"] then
		defines { "CONFIG2_LOBBY=0" }
	end

	if _OPTIONS["without-miniupnpc"] then
		defines { "CONFIG2_MINIUPNPC=0" }
	end

	-- required for the lowlevel library. must be set from all projects that use it, otherwise it assumes it is
	-- being used as a DLL (which is currently not the case in 0ad)
	defines { "LIB_STATIC_LINK" }

	-- various platform-specific build flags
	if os.istarget("windows") then

		-- use native wchar_t type (not typedef to unsigned short)
		nativewchar "on"

	else	-- *nix

		-- TODO, FIXME: This check is incorrect because it means that some additional flags will be added inside the "else" branch if the
		-- compiler is ICC and minimal-flags is specified (ticket: #2994)
		if cc == "icc" and not _OPTIONS["minimal-flags"] then
			buildoptions {
				"-w1",
				-- "-Wabi",
				-- "-Wp64", -- complains about OBJECT_TO_JSVAL which is annoying
				"-Wpointer-arith",
				"-Wreturn-type",
				-- "-Wshadow",
				"-Wuninitialized",
				"-Wunknown-pragmas",
				"-Wunused-function",
				"-wd1292" -- avoid lots of 'attribute "__nonnull__" ignored'
			}
			filter "Debug"
				buildoptions { "-O0" } -- ICC defaults to -O2
			filter { }
			if os.istarget("macosx") then
				linkoptions { "-multiply_defined","suppress" }
			end
		else
			-- exclude most non-essential build options for minimal-flags
			if not _OPTIONS["minimal-flags"] then
				buildoptions {
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
					"-Wundef",				-- (useful for finding macro name typos)

					-- enable security features (stack checking etc) that shouldn't have
					-- a significant effect on performance and can catch bugs
					"-fstack-protector-all",
					"-U_FORTIFY_SOURCE",	-- (avoid redefinition warning if already defined)
					"-D_FORTIFY_SOURCE=2",

					-- always enable strict aliasing (useful in debug builds because of the warnings)
					"-fstrict-aliasing",

					-- don't omit frame pointers (for now), because performance will be impacted
					-- negatively by the way this breaks profilers more than it will be impacted
					-- positively by the optimisation
					"-fno-omit-frame-pointer"
				}

				if not _OPTIONS["without-pch"] then
					buildoptions {
						-- do something (?) so that ccache can handle compilation with PCH enabled
						-- (ccache 3.1+ also requires CCACHE_SLOPPINESS=time_macros for this to work)
						"-fpch-preprocess"
					}
				end

				if os.istarget("linux") or os.istarget("bsd") then
					buildoptions { "-fPIC" }
					linkoptions { "-Wl,--no-undefined", "-Wl,--as-needed", "-Wl,-z,relro" }
				end

				if arch == "x86" then
					buildoptions {
						-- To support intrinsics like __sync_bool_compare_and_swap on x86
						-- we need to set -march to something that supports them (i686).
						-- We use pentium3 to also enable other features like mmx and sse,
						-- while tuning for generic to have good performance on every
						-- supported CPU.
						-- Note that all these features are already supported on amd64.
						"-march=pentium3 -mtune=generic"
					}
				end
			end

			buildoptions {
				-- Enable C++11 standard.
				"-std=c++0x"
			}

			if arch == "arm" then
				-- disable warnings about va_list ABI change and use
				-- compile-time flags for futher configuration.
				buildoptions { "-Wno-psabi" }
				if _OPTIONS["android"] then
					-- Android uses softfp, so we should too.
					buildoptions { "-mfloat-abi=softfp" }
				end
			end

			if _OPTIONS["coverage"] then
				buildoptions { "-fprofile-arcs", "-ftest-coverage" }
				links { "gcov" }
			end

			-- We don't want to require SSE2 everywhere yet, but OS X headers do
			-- require it (and Intel Macs always have it) so enable it here
			if os.istarget("macosx") then
				buildoptions { "-msse2" }
			end

			-- Check if SDK path should be used
			if _OPTIONS["sysroot"] then
				buildoptions { "-isysroot " .. _OPTIONS["sysroot"] }
				linkoptions { "-Wl,-syslibroot," .. _OPTIONS["sysroot"] }
			end

			-- On OS X, sometimes we need to specify the minimum API version to use
			if _OPTIONS["macosx-version-min"] then
				buildoptions { "-mmacosx-version-min=" .. _OPTIONS["macosx-version-min"] }
				-- clang and llvm-gcc look at mmacosx-version-min to determine link target
				-- and CRT version, and use it to set the macosx_version_min linker flag
				linkoptions { "-mmacosx-version-min=" .. _OPTIONS["macosx-version-min"] }
			end

			-- Check if we're building a bundle
			if _OPTIONS["macosx-bundle"] then
				defines { "BUNDLE_IDENTIFIER=" .. _OPTIONS["macosx-bundle"] }
			end

			-- On OS X, force using libc++ since it has better C++11 support,
			-- now required by the game
			if os.istarget("macosx") then
				buildoptions { "-stdlib=libc++" }
				linkoptions { "-stdlib=libc++" }
			end
		end

		buildoptions {
			-- Hide symbols in dynamic shared objects by default, for efficiency and for equivalence with
			-- Windows - they should be exported explicitly with __attribute__ ((visibility ("default")))
			"-fvisibility=hidden"
		}

		if _OPTIONS["bindir"] then
			defines { "INSTALLED_BINDIR=" .. _OPTIONS["bindir"] }
		end
		if _OPTIONS["datadir"] then
			defines { "INSTALLED_DATADIR=" .. _OPTIONS["datadir"] }
		end
		if _OPTIONS["libdir"] then
			defines { "INSTALLED_LIBDIR=" .. _OPTIONS["libdir"] }
		end

		if os.istarget("linux") or os.istarget("bsd") then
			-- To use our local shared libraries, they need to be found in the
			-- runtime dynamic linker path. Add their path to -rpath.
			if _OPTIONS["libdir"] then
				linkoptions {"-Wl,-rpath," .. _OPTIONS["libdir"] }
			else
				-- On FreeBSD we need to allow use of $ORIGIN
				if os.istarget("bsd") then
					linkoptions { "-Wl,-z,origin" }
				end

				-- Adding the executable path and taking care of correct escaping
				if _ACTION == "gmake" then
					linkoptions { "-Wl,-rpath,'$$ORIGIN'" }
				elseif _ACTION == "codeblocks" then
					linkoptions { "-Wl,-R\\\\$$$ORIGIN" }
				end
			end
		end

	end
end

-- add X11 includes paths after all the others so they don't conflict with
-- bundled libs
function project_add_x11_dirs()
	if not os.istarget("windows") and not os.istarget("macosx") then
		-- X11 includes may be installed in one of a gadzillion of five places
		-- Famous last words: "You can't include too much! ;-)"
		sysincludedirs {
			"/usr/X11R6/include/X11",
			"/usr/X11R6/include",
			"/usr/local/include/X11",
			"/usr/local/include",
			"/usr/include/X11"
		}
		libdirs { "/usr/X11R6/lib" }
	end
end

-- create a project and set the attributes that are common to all projects.
function project_create(project_name, target_type)

	project(project_name)
	language "C++"
	kind(target_type)

	filter "action:vs2013"
		toolset "v120_xp"
	filter "action:vs2015"
		toolset "v140_xp"
	filter {}

	project_set_target(project_name)
	project_set_build_flags()
end


-- OSX creates a .app bundle if the project type of the main application is set to "WindowedApp".
-- We don't want this because this bundle would be broken (it lacks all the resources and external dependencies, Info.plist etc...)
-- Windows opens a console in the background if it's set to ConsoleApp, which is not what we want.
-- I didn't check if this setting matters for linux, but WindowedApp works there.
function get_main_project_target_type()
	if _OPTIONS["android"] then
		return "SharedLib"
	elseif os.istarget("macosx") then
		return "ConsoleApp"
	else
		return "WindowedApp"
	end
end


-- source_root: rel_source_dirs and rel_include_dirs are relative to this directory
-- rel_source_dirs: A table of subdirectories. All source files in these directories are added.
-- rel_include_dirs: A table of subdirectories to be included.
-- extra_params: table including zero or more of the following:
-- * no_pch: If specified, no precompiled headers are used for this project.
-- * pch_dir: If specified, this directory will be used for precompiled headers instead of the default
--   <source_root>/pch/<projectname>/.
-- * extra_files: table of filenames (relative to source_root) to add to project
-- * extra_links: table of library names to add to link step
function project_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)

	for i,v in pairs(rel_source_dirs) do
		local prefix = source_root..v.."/"
		files { prefix.."*.cpp", prefix.."*.h", prefix.."*.inl", prefix.."*.js", prefix.."*.asm", prefix.."*.mm" }
	end

	-- Put the project-specific PCH directory at the start of the
	-- include path, so '#include "precompiled.h"' will look in
	-- there first
	local pch_dir
	if not extra_params["pch_dir"] then
		pch_dir = source_root .. "pch/" .. project().name .. "/"
	else
		pch_dir = extra_params["pch_dir"]
	end
	includedirs { pch_dir }

	-- Precompiled Headers
	-- rationale: we need one PCH per static lib, since one global header would
	-- increase dependencies. To that end, we can either include them as
	-- "projectdir/precompiled.h", or add "source/PCH/projectdir" to the
	-- include path and put the PCH there. The latter is better because
	-- many projects contain several dirs and it's unclear where there the
	-- PCH should be stored. This way is also a bit easier to use in that
	-- source files always include "precompiled.h".
	-- Notes:
	-- * Visual Assist manages to use the project include path and can
	--   correctly open these files from the IDE.
	-- * precompiled.cpp (needed to "Create" the PCH) also goes in
	--   the abovementioned dir.
	if (not _OPTIONS["without-pch"] and not extra_params["no_pch"]) then
		filter "action:vs*"
			pchheader("precompiled.h")
		filter "action:xcode*"
			pchheader("../"..pch_dir.."precompiled.h")
		filter { "action:not vs*", "action:not xcode*" }
			pchheader(pch_dir.."precompiled.h")
		filter {}
		pchsource(pch_dir.."precompiled.cpp")
		defines { "USING_PCH" }
		files { pch_dir.."precompiled.h", pch_dir.."precompiled.cpp" }
	else
		flags { "NoPCH" }
	end

	-- next is source root dir, for absolute (nonrelative) includes
	-- (e.g. "lib/precompiled.h")
	includedirs { source_root }

	for i,v in pairs(rel_include_dirs) do
		includedirs { source_root .. v }
	end

	if extra_params["extra_files"] then
		for i,v in pairs(extra_params["extra_files"]) do
			-- .rc files are only needed on Windows
			if path.getextension(v) ~= ".rc" or os.istarget("windows") then
				files { source_root .. v }
			end
		end
	end

	if extra_params["extra_links"] then
		links { extra_params["extra_links"] }
	end
end


-- Add command-line options to set up the manifest dependencies for Windows
-- (See lib/sysdep/os/win/manifest.cpp)
function project_add_manifest()
	linkoptions { "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"" }
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
static_lib_names_debug = {}
static_lib_names_release = {}

-- set up one of the static libraries into which the main engine code is split.
-- extra_params:
--	no_default_link: If specified, linking won't be done by default.
-- 	For the rest of extra_params, see project_add_contents().
-- note: rel_source_dirs and rel_include_dirs are relative to global source_root.
function setup_static_lib_project (project_name, rel_source_dirs, extern_libs, extra_params)

	local target_type = "StaticLib"
	project_create(project_name, target_type)
	project_add_contents(source_root, rel_source_dirs, {}, extra_params)
	project_add_extern_libs(extern_libs, target_type)
	project_add_x11_dirs()

	if not extra_params["no_default_link"] then
		table.insert(static_lib_names, project_name)
	end

	if os.istarget("windows") then
		rtti "off"
	end
end

function setup_third_party_static_lib_project (project_name, rel_source_dirs, extern_libs, extra_params)

	setup_static_lib_project(project_name, rel_source_dirs, extern_libs, extra_params)
	includedirs { source_root .. "third_party/" .. project_name .. "/include/" }
end

function setup_shared_lib_project (project_name, rel_source_dirs, extern_libs, extra_params)

	local target_type = "SharedLib"
	project_create(project_name, target_type)
	project_add_contents(source_root, rel_source_dirs, {}, extra_params)
	project_add_extern_libs(extern_libs, target_type)
	project_add_x11_dirs()

	if not extra_params["no_default_link"] then
		table.insert(static_lib_names, project_name)
	end

	if os.istarget("windows") then
		rtti "off"
		links { "delayimp" }
	end
end


-- this is where the source tree is chopped up into static libs.
-- can be changed very easily; just copy+paste a new setup_static_lib_project,
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
	if not _OPTIONS["without-miniupnpc"] then
		table.insert(extern_libs, "miniupnpc")
	end
	setup_static_lib_project("network", source_dirs, extern_libs, {})

	source_dirs = {
		"third_party/tinygettext/src",
	}
	extern_libs = {
		"iconv",
		"boost",
	}
	setup_third_party_static_lib_project("tinygettext", source_dirs, extern_libs, { } )

	-- it's an external library and we don't want to modify its source to fix warnings, so we just disable them to avoid noise in the compile output
	filter "action:vs*"
		buildoptions {
			"/wd4127",
			"/wd4309",
			"/wd4800",
			"/wd4100",
			"/wd4996",
			"/wd4099",
			"/wd4503"
		}
	filter {}


	if not _OPTIONS["without-lobby"] then
		source_dirs = {
			"lobby",
			"lobby/scripting",
			"i18n",
			"third_party/encryption"
		}

		extern_libs = {
			"spidermonkey",
			"boost",
			"enet",
			"gloox",
			"icu",
			"iconv",
			"tinygettext"
		}
		setup_static_lib_project("lobby", source_dirs, extern_libs, {})

		if _OPTIONS["use-shared-glooxwrapper"] and not _OPTIONS["build-shared-glooxwrapper"] then
			table.insert(static_lib_names_debug, "glooxwrapper_dbg")
			table.insert(static_lib_names_release, "glooxwrapper")
		else
			source_dirs = {
				"lobby/glooxwrapper",
			}
			extern_libs = {
				"boost",
				"gloox",
			}
			if _OPTIONS["build-shared-glooxwrapper"] then
				setup_shared_lib_project("glooxwrapper", source_dirs, extern_libs, {})
			else
				setup_static_lib_project("glooxwrapper", source_dirs, extern_libs, {})
			end
		end
	else
		source_dirs = {
			"lobby/scripting",
			"third_party/encryption"
		}
		extern_libs = {
			"spidermonkey",
			"boost"
		}
		setup_static_lib_project("lobby", source_dirs, extern_libs, {})
		files { source_root.."lobby/Globals.cpp" }
	end


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
		"opengl",
		"spidermonkey",
	}
	setup_static_lib_project("simulation2", source_dirs, extern_libs, {})


	source_dirs = {
		"scriptinterface",
		"scriptinterface/third_party"
	}
	extern_libs = {
		"boost",
		"spidermonkey",
		"valgrind",
		"sdl",
	}
	setup_static_lib_project("scriptinterface", source_dirs, extern_libs, {})


	source_dirs = {
		"ps",
		"ps/scripting",
		"network/scripting",
		"ps/GameSetup",
		"ps/XML",
		"soundmanager",
		"soundmanager/data",
		"soundmanager/items",
		"soundmanager/scripting",
		"maths",
		"maths/scripting",
		"i18n",
		"i18n/scripting",
		"third_party/cppformat",
	}
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"libxml2",
		"opengl",
		"zlib",
		"boost",
		"enet",
		"libcurl",
		"tinygettext",
		"icu",
		"iconv",
	}

	if not _OPTIONS["without-audio"] then
		table.insert(extern_libs, "openal")
		table.insert(extern_libs, "vorbis")
	end

	setup_static_lib_project("engine", source_dirs, extern_libs, {})


	source_dirs = {
		"graphics",
		"graphics/scripting",
		"renderer",
		"renderer/scripting",
		"third_party/mikktspace"
	}
	extern_libs = {
		"opengl",
		"sdl",	-- key definitions
		"spidermonkey",	-- for graphics/scripting
		"boost"
	}
	if not _OPTIONS["without-nvtt"] then
		table.insert(extern_libs, "nvtt")
	end
	setup_static_lib_project("graphics", source_dirs, extern_libs, {})


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
	setup_static_lib_project("atlas", source_dirs, extern_libs, {})


	source_dirs = {
		"gui",
		"gui/scripting",
		"i18n"
	}
	extern_libs = {
		"spidermonkey",
		"sdl",	-- key definitions
		"opengl",
		"boost",
		"enet",
		"tinygettext",
		"icu",
		"iconv",
	}
	if not _OPTIONS["without-audio"] then
		table.insert(extern_libs, "openal")
	end
	setup_static_lib_project("gui", source_dirs, extern_libs, {})


	source_dirs = {
		"lib",
		"lib/adts",
		"lib/allocators",
		"lib/external_libraries",
		"lib/file",
		"lib/file/archive",
		"lib/file/common",
		"lib/file/io",
		"lib/file/vfs",
		"lib/pch",
		"lib/posix",
		"lib/res",
		"lib/res/graphics",
		"lib/sysdep",
		"lib/tex"
	}
	extern_libs = {
		"boost",
		"sdl",
		"openal",
		"opengl",
		"libpng",
		"zlib",
		"valgrind",
		"cxxtest",
	}

	-- CPU architecture-specific
	if arch == "amd64" then
		table.insert(source_dirs, "lib/sysdep/arch/amd64");
		table.insert(source_dirs, "lib/sysdep/arch/x86_x64");
	elseif arch == "x86" then
		table.insert(source_dirs, "lib/sysdep/arch/ia32");
		table.insert(source_dirs, "lib/sysdep/arch/x86_x64");
	elseif arch == "arm" then
		table.insert(source_dirs, "lib/sysdep/arch/arm");
	elseif arch == "aarch64" then
		table.insert(source_dirs, "lib/sysdep/arch/aarch64");
	end

	-- OS-specific
	sysdep_dirs = {
		linux = { "lib/sysdep/os/linux", "lib/sysdep/os/unix" },
		-- note: RC file must be added to main_exe project.
		-- note: don't add "lib/sysdep/os/win/aken.cpp" because that must be compiled with the DDK.
		windows = { "lib/sysdep/os/win", "lib/sysdep/os/win/wposix", "lib/sysdep/os/win/whrt" },
		macosx = { "lib/sysdep/os/osx", "lib/sysdep/os/unix" },
		bsd = { "lib/sysdep/os/bsd", "lib/sysdep/os/unix", "lib/sysdep/os/unix/x" },
	}
	for i,v in pairs(sysdep_dirs[os.target()]) do
		table.insert(source_dirs, v);
	end

	if os.istarget("linux") then
		if _OPTIONS["android"] then
			table.insert(source_dirs, "lib/sysdep/os/android")
		else
			table.insert(source_dirs, "lib/sysdep/os/unix/x")
		end
	end

	-- On OSX, disable precompiled headers because C++ files and Objective-C++ files are
	-- mixed in this project. To fix that, we would need per-file basis configuration which
	-- is not yet supported by the gmake action in premake. We should look into using gmake2.
	extra_params = {}
	if os.istarget("macosx") then
		extra_params = { no_pch = 1 }
	end

	-- runtime-library-specific
	if _ACTION == "vs2013" or _ACTION == "vs2015" then
		table.insert(source_dirs, "lib/sysdep/rtl/msc");
	else
		table.insert(source_dirs, "lib/sysdep/rtl/gcc");
	end

	setup_static_lib_project("lowlevel", source_dirs, extern_libs, extra_params)


	-- Third-party libraries that are built as part of the main project,
	-- not built externally and then linked
	source_dirs = {
		"third_party/mongoose",
	}
	extern_libs = {
	}
	setup_static_lib_project("mongoose", source_dirs, extern_libs, { no_pch = 1 })


	-- CxxTest mock function support
	extern_libs = {
		"boost",
		"cxxtest",
	}

	-- 'real' implementations, to be linked against the main executable
	-- (files are added manually and not with setup_static_lib_project
	-- because not all files in the directory are included)
	setup_static_lib_project("mocks_real", {}, extern_libs, { no_default_link = 1, no_pch = 1 })
	files { "mocks/*.h", source_root.."mocks/*_real.cpp" }
	-- 'test' implementations, to be linked against the test executable
	setup_static_lib_project("mocks_test", {}, extern_libs, { no_default_link = 1, no_pch = 1 })
	files { source_root.."mocks/*.h", source_root.."mocks/*_test.cpp" }
end

--------------------------------------------------------------------------------
-- main EXE
--------------------------------------------------------------------------------

-- used for main EXE as well as test
used_extern_libs = {
	"opengl",
	"sdl",

	"libpng",
	"zlib",

	"spidermonkey",
	"libxml2",

	"boost",
	"cxxtest",
	"comsuppw",
	"enet",
	"libcurl",
	"tinygettext",
	"icu",
	"iconv",

	"valgrind",
}

if not os.istarget("windows") and not _OPTIONS["android"] and not os.istarget("macosx") then
	-- X11 should only be linked on *nix
	table.insert(used_extern_libs, "x11")
	table.insert(used_extern_libs, "xcursor")
end

if not _OPTIONS["without-audio"] then
	table.insert(used_extern_libs, "openal")
	table.insert(used_extern_libs, "vorbis")
end

if not _OPTIONS["without-nvtt"] then
	table.insert(used_extern_libs, "nvtt")
end

if not _OPTIONS["without-lobby"] then
	table.insert(used_extern_libs, "gloox")
end

if not _OPTIONS["without-miniupnpc"] then
	table.insert(used_extern_libs, "miniupnpc")
end

-- Bundles static libs together with main.cpp and builds game executable.
function setup_main_exe ()

	local target_type = get_main_project_target_type()
	project_create("pyrogenesis", target_type)

	filter "system:not macosx"
		linkgroups 'On'
	filter {}

	links { "mocks_real" }

	local extra_params = {
		extra_files = { "main.cpp" },
		no_pch = 1
	}
	project_add_contents(source_root, {}, {}, extra_params)
	project_add_extern_libs(used_extern_libs, target_type)
	project_add_x11_dirs()

	dependson { "Collada" }

	-- Platform Specifics
	if os.istarget("windows") then

		files { source_root.."lib/sysdep/os/win/icon.rc" }
		-- from "lowlevel" static lib; must be added here to be linked in
		files { source_root.."lib/sysdep/os/win/error_dialog.rc" }

		rtti "off"

		linkoptions {
			-- wraps main thread in a __try block(see wseh.cpp). replace with mainCRTStartup if that's undesired.
			"/ENTRY:wseh_EntryPoint",

			-- see wstartup.h
			"/INCLUDE:_wstartup_InitAndRegisterShutdown",

			-- allow manual unload of delay-loaded DLLs
			"/DELAY:UNLOAD",
		}

		-- allow the executable to use more than 2GB of RAM.
		-- this should not be enabled during development, so that memory issues are easily spotted.
		if _OPTIONS["large-address-aware"] then
			linkoptions { "/LARGEADDRESSAWARE" }
		end

		-- see manifest.cpp
		project_add_manifest()

	elseif os.istarget("linux") or os.istarget("bsd") then

		if not _OPTIONS["android"] and not (os.getversion().description == "OpenBSD") then
			links { "rt" }
		end

		if _OPTIONS["android"] then
			-- NDK's STANDALONE-TOOLCHAIN.html says this is required
			linkoptions { "-Wl,--fix-cortex-a8" }

			links { "log" }
		end

		if os.istarget("linux") or os.getversion().description == "GNU/kFreeBSD" then
			links {
				-- Dynamic libraries (needed for linking for gold)
				"dl",
			}
		elseif os.istarget("bsd") then
			links {
				-- Needed for backtrace* on BSDs
				"execinfo",
			}
		end

		-- Threading support
		buildoptions { "-pthread" }
		if not _OPTIONS["android"] then
			linkoptions { "-pthread" }
		end

		-- For debug_resolve_symbol
		filter "Debug"
			linkoptions { "-rdynamic" }
		filter { }

	elseif os.istarget("macosx") then

		links { "pthread" }
		links { "ApplicationServices.framework", "Cocoa.framework", "CoreFoundation.framework" }

	end
end


--------------------------------------------------------------------------------
-- atlas
--------------------------------------------------------------------------------

-- setup a typical Atlas component project
-- extra_params, rel_source_dirs and rel_include_dirs: as in project_add_contents;
function setup_atlas_project(project_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	local source_root = rootdir.."/source/tools/atlas/" .. project_name .. "/"
	project_create(project_name, target_type)

	-- if not specified, the default for atlas pch files is in the project root.
	if not extra_params["pch_dir"] then
		extra_params["pch_dir"] = source_root
	end

	project_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)
	project_add_extern_libs(extern_libs, target_type)
	project_add_x11_dirs()

	-- Platform Specifics
	if os.istarget("windows") then
		-- Link to required libraries
		links { "winmm", "comctl32", "rpcrt4", "delayimp", "ws2_32" }

	elseif os.istarget("linux") or os.istarget("bsd") then
		buildoptions { "-rdynamic", "-fPIC" }
		linkoptions { "-fPIC", "-rdynamic" }

		-- warnings triggered by wxWidgets
		buildoptions { "-Wno-unused-local-typedefs" }

	elseif os.istarget("macosx") then
		-- install_name settings aren't really supported yet by premake, but there are plans for the future.
		-- we currently use this hack to work around some bugs with wrong install_names.
		if target_type == "SharedLib" then
			if _OPTIONS["macosx-bundle"] then
				-- If we're building a bundle, it will be in ../Frameworks
				filter "Debug"
					linkoptions { "-install_name @executable_path/../Frameworks/lib"..project_name.."_dbg.dylib" }
				filter "Release"
					linkoptions { "-install_name @executable_path/../Frameworks/lib"..project_name..".dylib" }
				filter { }
			else
				filter "Debug"
					linkoptions { "-install_name @executable_path/lib"..project_name.."_dbg.dylib" }
				filter "Release"
					linkoptions { "-install_name @executable_path/lib"..project_name..".dylib" }
				filter { }
			end
		end
	end

end


-- build all Atlas component projects
function setup_atlas_projects()

	setup_atlas_project("AtlasObject", "StaticLib",
	{	-- src
		".",
		"../../../third_party/jsonspirit"

	},{	-- include
		"../../../third_party/jsonspirit"
	},{	-- extern_libs
		"boost",
		"iconv",
		"libxml2",
		"wxwidgets"
	},{	-- extra_params
		no_pch = 1
	})

	atlas_src = {
		"ActorEditor",
		"CustomControls/Buttons",
		"CustomControls/Canvas",
		"CustomControls/ColorDialog",
		"CustomControls/DraggableListCtrl",
		"CustomControls/EditableListCtrl",
		"CustomControls/FileHistory",
		"CustomControls/HighResTimer",
		"CustomControls/MapDialog",
		"CustomControls/SnapSplitterWindow",
		"CustomControls/VirtualDirTreeCtrl",
		"CustomControls/Windows",
		"General",
		"General/VideoRecorder",
		"Misc",
		"ScenarioEditor",
		"ScenarioEditor/Sections/Common",
		"ScenarioEditor/Sections/Cinema",
		"ScenarioEditor/Sections/Environment",
		"ScenarioEditor/Sections/Map",
		"ScenarioEditor/Sections/Object",
		"ScenarioEditor/Sections/Player",
		"ScenarioEditor/Sections/Terrain",
		"ScenarioEditor/Tools",
		"ScenarioEditor/Tools/Common",
	}
	atlas_extra_links = {
		"AtlasObject"
	}

	atlas_extern_libs = {
		"boost",
		"comsuppw",
		"iconv",
		"libxml2",
		"sdl",	-- key definitions
		"wxwidgets",
		"zlib",
	}
	if not os.istarget("windows") and not os.istarget("macosx") then
		-- X11 should only be linked on *nix
		table.insert(atlas_extern_libs, "x11")
	end

	setup_atlas_project("AtlasUI", "SharedLib", atlas_src,
	{	-- include
		"..",
		"CustomControls",
		"Misc"
	},
	atlas_extern_libs,
	{	-- extra_params
		pch_dir = rootdir.."/source/tools/atlas/AtlasUI/Misc/",
		no_pch = false,
		extra_links = atlas_extra_links,
		extra_files = { "Misc/atlas.rc" }
	})
end


-- Atlas 'frontend' tool-launching projects
function setup_atlas_frontend_project (project_name)

	local target_type = get_main_project_target_type()
	project_create(project_name, target_type)
	project_add_x11_dirs()

	local source_root = rootdir.."/source/tools/atlas/AtlasFrontends/"
	files { source_root..project_name..".cpp" }

	if os.istarget("windows") then
		files { source_root..project_name..".rc" }
	end

	includedirs { source_root .. ".." }

	-- Platform Specifics
	if os.istarget("windows") then
		-- see manifest.cpp
		project_add_manifest()

	else -- Non-Windows, = Unix
		links { "AtlasObject" }
	end

	links { "AtlasUI" }

end

function setup_atlas_frontends()
	setup_atlas_frontend_project("ActorEditor")
end


--------------------------------------------------------------------------------
-- collada
--------------------------------------------------------------------------------

function setup_collada_project(project_name, target_type, rel_source_dirs, rel_include_dirs, extern_libs, extra_params)

	project_create(project_name, target_type)
	local source_root = source_root.."collada/"
	extra_params["pch_dir"] = source_root
	project_add_contents(source_root, rel_source_dirs, rel_include_dirs, extra_params)
	project_add_extern_libs(extern_libs, target_type)
	project_add_x11_dirs()

	-- Platform Specifics
	if os.istarget("windows") then
		characterset "MBCS"
	elseif os.istarget("linux") then
		defines { "LINUX" }

		links {
			"dl",
		}

		-- FCollada is not aliasing-safe, so disallow dangerous optimisations
		-- (TODO: It'd be nice to fix FCollada, but that looks hard)
		buildoptions { "-fno-strict-aliasing" }

		buildoptions { "-rdynamic" }
		linkoptions { "-rdynamic" }

	elseif os.istarget("bsd") then
		if os.getversion().description == "OpenBSD" then
			links { "c", }
		end

		if os.getversion().description == "GNU/kFreeBSD" then
			links {
				"dl",
			}
		end

		buildoptions { "-fno-strict-aliasing" }

		buildoptions { "-rdynamic" }
		linkoptions { "-rdynamic" }

	elseif os.istarget("macosx") then
		-- define MACOS-something?

		-- install_name settings aren't really supported yet by premake, but there are plans for the future.
		-- we currently use this hack to work around some bugs with wrong install_names.
		if target_type == "SharedLib" then
			if _OPTIONS["macosx-bundle"] then
				-- If we're building a bundle, it will be in ../Frameworks
				linkoptions { "-install_name @executable_path/../Frameworks/lib"..project_name..".dylib" }
			else
				linkoptions { "-install_name @executable_path/lib"..project_name..".dylib" }
			end
		end

		buildoptions { "-fno-strict-aliasing" }
		-- On OSX, fcollada uses a few utility functions from coreservices
		links { "CoreServices.framework" }
	end

end

-- build all Collada component projects
function setup_collada_projects()

	setup_collada_project("Collada", "SharedLib",
	{	-- src
		"."
	},{	-- include
	},{	-- extern_libs
		"fcollada",
		"iconv",
		"libxml2"
	},{	-- extra_params
	})

end


--------------------------------------------------------------------------------
-- tests
--------------------------------------------------------------------------------

function setup_tests()

	local cxxtest = require "cxxtest"

	if os.istarget("windows") then
		cxxtest.setpath(rootdir.."/build/bin/cxxtestgen.exe")
	else
		cxxtest.setpath(rootdir.."/libraries/source/cxxtest-4.4/bin/cxxtestgen")
	end

	local runner = "ErrorPrinter"
	if _OPTIONS["jenkins-tests"] then
		runner = "XmlPrinter"
	end

	local includefiles = {
		-- Precompiled headers - the header is added to all generated .cpp files
		-- note that the header isn't actually precompiled here, only #included
		-- so that the build stage can use it as a precompiled header.
		"precompiled.h",
		-- This is required to build against SDL 2.0.4 on Windows.
		"lib/external_libraries/libsdl.h",
	}

	cxxtest.init(source_root, true, runner, includefiles)

	local target_type = get_main_project_target_type()
	project_create("test", target_type)

	-- Find header files in 'test' subdirectories
	local all_files = os.matchfiles(source_root .. "**/tests/*.h")
	local test_files = {}
	for i,v in pairs(all_files) do
		-- Don't include sysdep tests on the wrong sys
		-- Don't include Atlas tests unless Atlas is being built
		if not (string.find(v, "/sysdep/os/win/") and not os.istarget("windows")) and
		   not (string.find(v, "/tools/atlas/") and not _OPTIONS["atlas"]) and
		   not (string.find(v, "/sysdep/arch/x86_x64/") and ((arch ~= "amd64") or (arch ~= "x86")))
		then
			table.insert(test_files, v)
		end
	end

	cxxtest.configure_project(test_files)

	filter "system:not macosx"
		linkgroups 'On'
	filter {}

	links { static_lib_names }
	filter "Debug"
		links { static_lib_names_debug }
	filter "Release"
		links { static_lib_names_release }
	filter { }

	links { "mocks_test" }
	if _OPTIONS["atlas"] then
		links { "AtlasObject" }
		project_add_extern_libs({"wxwidgets"}, target_type)
	end
	extra_params = {
		extra_files = { "test_setup.cpp" },
	}

	project_add_contents(source_root, {}, {}, extra_params)
	project_add_extern_libs(used_extern_libs, target_type)
	project_add_x11_dirs()

	dependson { "Collada" }

	-- TODO: should fix the duplication between this OS-specific linking
	-- code, and the similar version in setup_main_exe

	if os.istarget("windows") then
		-- from "lowlevel" static lib; must be added here to be linked in
		files { source_root.."lib/sysdep/os/win/error_dialog.rc" }

		rtti "off"

		-- see wstartup.h
		linkoptions { "/INCLUDE:_wstartup_InitAndRegisterShutdown" }
		-- Enables console for the TEST project on Windows
		linkoptions { "/SUBSYSTEM:CONSOLE" }

		project_add_manifest()

	elseif os.istarget("linux") or os.istarget("bsd") then

		if not _OPTIONS["android"] and not (os.getversion().description == "OpenBSD") then
			links { "rt" }
		end

		if _OPTIONS["android"] then
			-- NDK's STANDALONE-TOOLCHAIN.html says this is required
			linkoptions { "-Wl,--fix-cortex-a8" }
		end

		if os.istarget("linux") or os.getversion().description == "GNU/kFreeBSD" then
			links {
				-- Dynamic libraries (needed for linking for gold)
				"dl",
			}
		elseif os.istarget("bsd") then
			links {
				-- Needed for backtrace* on BSDs
				"execinfo",
			}
		end

		-- Threading support
		buildoptions { "-pthread" }
		if not _OPTIONS["android"] then
			linkoptions { "-pthread" }
		end

		-- For debug_resolve_symbol
		filter "Debug"
			linkoptions { "-rdynamic" }
		filter { }

		includedirs { source_root .. "pch/test/" }
	end
end

-- must come first, so that VC sets it as the default project and therefore
-- allows running via F5 without the "where is the EXE" dialog.
setup_main_exe()

setup_all_libs()

-- add the static libs to the main EXE project. only now (after
-- setup_all_libs has run) are the lib names known. cannot move
-- setup_main_exe to run after setup_all_libs (see comment above).
-- we also don't want to hardcode the names - that would require more
-- work when changing the static lib breakdown.
project("pyrogenesis") -- Set the main project active
	links { static_lib_names }
	filter "Debug"
		links { static_lib_names_debug }
	filter "Release"
		links { static_lib_names_release }
	filter { }

if _OPTIONS["atlas"] then
	setup_atlas_projects()
	setup_atlas_frontends()
end

setup_collada_projects()

if not _OPTIONS["without-tests"] then
	setup_tests()
end
