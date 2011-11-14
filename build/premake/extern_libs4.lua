-- this file provides project_add_extern_libs, which takes care of the
-- dirty details of adding the libraries' include and lib paths.
--
-- TYPICAL TASK: add new library. Instructions:
-- 1) add a new extern_lib_defs entry
-- 2) add library name to extern_libs tables in premake.lua for all 'projects' that want to use it


-- directory in which all library subdirectories reside.
libraries_dir = rootdir.."/libraries/"

local function add_default_lib_paths(extern_lib)
	libdirs { libraries_dir .. extern_lib .. "/lib" }
end

local function add_default_include_paths(extern_lib)
	includedirs { libraries_dir .. extern_lib .. "/include" }
end


-- For unixes: add buildflags and linkflags using pkg-config or another similar command.
-- By default, pkg-config is used. Other commands can be passed as "alternative_cmd".
-- Running such commands at build/linktime does not work on all environments.
-- For those environments where it does not work, we already run it now.
-- Any better ideas?
local function pkgconfig_cflags(lib, alternative_cmd)
	local cmd_cflags = ""
	local result_cflags
	if not alternative_cmd then
		cmd_cflags = "pkg-config "..lib.." --cflags"
	else
		cmd_cflags = alternative_cmd
	end

	if _ACTION == "xcode3" then
		result_cflags = string.gsub(os.capture(cmd_cflags), "\n", "")
		buildoptions { result_cflags }
	else
		buildoptions { "`"..cmd_cflags.."`" }
	end
end

local function pkgconfig_libs(lib, alternative_cmd)
	local cmd_libs = ""
	local result_libs
	if not alternative_cmd then
		cmd_libs = "pkg-config "..lib.." --libs"
	else
		cmd_libs = alternative_cmd
	end

	if _ACTION == "xcode3" then
		-- The syntax of -Wl with the comma separated list doesn't work and -Wl apparently isn't needed in xcode.
		-- This is a hack, but it works...
		result_libs = string.gsub(os.capture(cmd_libs), "-Wl", "")
		result_libs = string.gsub(result_libs, ",", " ")
		result_libs = string.gsub(result_libs, "\n", "")
		linkoptions { result_libs }
	elseif _ACTION == "gmake" then	
		gnuexternals { "`"..cmd_libs.."`" }
	else
		linkoptions { "`"..cmd_libs.."`" }
	end
end

function os.capture(cmd)
	local f = io.popen(cmd, 'r')
	local s = f:read('*a')
	return s
end

local function add_delayload(name, suffix, def)

	if def["no_delayload"] then
		return
	end

	-- currently only supported by VC; nothing to do on other platforms.
	if not os.is("windows") then
		return
	end

	-- no extra debug version; use same library in all configs
	if suffix == "" then
		linkoptions { "/DELAYLOAD:"..name..".dll" }
	-- extra debug version available; use in debug config
	else
		local dbg_cmd = "/DELAYLOAD:" .. name .. suffix .. ".dll"
		local cmd     = "/DELAYLOAD:" .. name .. ".dll"

		configuration "Debug"
			linkoptions { dbg_cmd }
		configuration "Release"
			linkoptions { cmd }
		configuration { }
	end

end

local function add_default_links(def)

	-- careful: make sure to only use *_names when on the correct platform.
	local names = {}
	if os.is("windows") then
		if def.win_names then
			names = def.win_names
		end
	elseif os.is("linux") and def.linux_names then
		names = def.linux_names
	elseif os.is("macosx") and def.osx_names then
		names = def.osx_names
	elseif def.unix_names then
		names = def.unix_names
	end

	local suffix = "d"
	-- library is overriding default suffix (typically "" to indicate there is none)
	if def["dbg_suffix"] then
		suffix = def["dbg_suffix"]
	end
	-- non-Windows doesn't have the distinction of debug vs. release libraries
	-- (to be more specific, they do, but the two are binary compatible;
	-- usually only one type - debug or release - is installed at a time).
	if not os.is("windows") then
		suffix = ""
	end

	-- OS X "Frameworks" need to be added in a special way to the link
	-- i.e. by linkoptions += "-framework ..."
	if os.is("macosx") and def.osx_frameworks then
		for i,name in pairs(def.osx_frameworks) do
			linkoptions { "-framework " .. name }
		end
	else
		for i,name in pairs(names) do
			configuration "Debug"
				links { name .. suffix }
			configuration "Release"
				links { name }
			configuration { }

			add_delayload(name, suffix, def)
		end
	end
end

-- Library definitions
-- In a perfect world, libraries would have a common installation template,
-- i.e. location of include directory, naming convention for .lib, etc.
-- this table provides a means of working around each library's differences.
--
-- The basic approach is defining two functions per library:
--
-- 1. compile_settings
-- This function should set all settings requred during the compile-phase like
-- includedirs, defines etc...
--
-- 2. link_settings
-- This function should set all settings required during the link-phase like
-- libdirs, linkflag etc...
--
-- The main reason for separating those settings is different linking behaviour
-- on osx and xcode. For more details, read the comment in project_add_extern_libs.
--
-- There are some helper functions for the most common tasks. You should use them
-- if they can be used in your situation to make the definitions more consistent and
-- use their default beviours as a guideline.
--
--
-- add_default_lib_paths(extern_lib)
--      Description: Add '<libraries root>/<libraryname>/lib'to the libpaths
--      Parameters:
--          * extern_lib: <libraryname> to be used in the libpath.
--
-- add_default_include_paths(extern_lib)
--      Description: Add '<libraries root>/<libraryname>/include' to the includepaths
--      Parameters:
--          * extern_lib: <libraryname> to be used in the libpath.
--
-- add_default_links
--		Description: Adds links to libraries and configures delayloading.
-- 		If the *_names parameter for a plattform is missing, no linking will be done
-- 		on that plattform.
--		The default assumptions are:
--		* debug import library and DLL are distinguished with a "d" suffix
-- 		* the library should be marked for delay-loading.
--		Parameters:
--      * win_names: table of import library / DLL names (no extension) when
--   	  running on Windows.
--      * unix_names: as above; shared object names when running on non-Windows.
--      * osx_names: as above; for OS X specificall (overrides unix_names if both are
--        specified)
--      * linux_names: ditto for Linux (overrides unix_names if both given)
--      * dbg_suffix: changes the debug suffix from the above default.
--        can be "" to indicate the library doesn't have a debug build;
--        in that case, the same library (without suffix) is used in
--        all build configurations.
--      * no_delayload: indicate the library is not to be delay-loaded.
--        this is necessary for some libraries that do not support it,
--        e.g. Xerces (which is so stupid as to export variables).

extern_lib_defs = {
	boost = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("boost")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("boost")
			end
			add_default_links({
				unix_names = { "boost_signals-mt", "boost_filesystem-mt", "boost_system-mt" },
				osx_names = { "boost_signals-mt", "boost_filesystem-mt", "boost_system-mt" },
			})
		end,
	},
	cryptopp = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("cryptopp")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("cryptopp")
			end
			add_default_links({
				win_names  = { "cryptopp" },
				unix_names = { "cryptopp" },
			})
		end,
	},
	cxxtest = {
		compile_settings = function()
			add_default_include_paths("cxxtest")
		end,
		link_settings = function()
			add_default_lib_paths("cxxtest")
		end,
	},
	comsuppw = {
		link_settings = function()
			add_default_links({
				win_names = { "comsuppw" },
				dbg_suffix = "d",
				no_delayload = 1,
			})
		end,
	},
	devil = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("devil")
			end
		end,
		link_settings = function()
			-- On Windows, it uses #pragma comment(lib ...) to link the library,
			-- so we only need to include the library-search-path
			if os.is("windows") then
				add_default_lib_paths("devil")
			end
			add_default_links({
				unix_names = { "IL", "ILU" },
			})
		end,
	},
	enet = {
		compile_settings = function()
			if not _OPTIONS["with-system-enet"] then
				add_default_include_paths("enet")
			end
		end,
		link_settings = function()
			if not _OPTIONS["with-system-enet"] then
				add_default_lib_paths("enet")
			end
			add_default_links({
				win_names  = { "enet" },
				unix_names = { "enet" },
			})
		end,
	},
	fcollada = {
		compile_settings = function()
			add_default_include_paths("fcollada")
		end,
		link_settings = function()
			add_default_lib_paths("fcollada")
			if os.is("windows") then
				configuration "Debug"
					links { "FColladaD" }
				configuration "Release"
					links { "FCollada" }
			 	configuration { }
			else
				configuration "Debug"
					links { "FColladaSD" }
				configuration "Release"
					links { "FColladaSR" }
				configuration { }
			end
		end,
	},
	ffmpeg = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("ffmpeg")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("ffmpeg")
			end
			add_default_links({
				win_names = { "avcodec-51", "avformat-51", "avutil-49", "swscale-0" },
				unix_names = { "avcodec", "avformat", "avutil" },
				dbg_suffix = "",
			})
		end,
	},
	libcurl = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("libcurl")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("libcurl")
			end
			add_default_links({
				win_names  = { "libcurl" },
				unix_names = { "curl" },
			})
		end,
	},
	libjpg = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("libjpg")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("libjpg")
			end
			add_default_links({
				win_names  = { "jpeg-6b" },
				unix_names = { "jpeg" },
			})
		end,
	},
	libpng = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("libpng")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("libpng")
			end
			add_default_links({
				win_names  = { "libpng14" },
				unix_names = { "png" },
			})
		end,
	},
	libxml2 = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("libxml2")
			else
				pkgconfig_cflags("libxml-2.0")
				-- libxml2 needs _REENTRANT or __MT__ for thread support;
				-- OS X doesn't get either set by default, so do it manually
				if os.is("macosx") then
					defines { "_REENTRANT" }
				end
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("libxml2")
				configuration "Debug"
					links { "libxml2" }
				configuration "Release"
					links { "libxml2" }
				configuration { }
			else
				pkgconfig_libs("libxml-2.0")
			end
		end,
	},
	nvtt = {
		compile_settings = function()
			if not _OPTIONS["with-system-nvtt"] then
				add_default_include_paths("nvtt")
			end
			defines { "NVTT_SHARED=1" }
		end,
		link_settings = function()
			if not _OPTIONS["with-system-nvtt"] then
				add_default_lib_paths("nvtt")
			end
			add_default_links({
				win_names  = { "nvtt" },
				unix_names = { "nvcore", "nvmath", "nvimage", "nvtt" },
				dbg_suffix = "", -- for performance we always use the release-mode version
			})
		end,
	},
	openal = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("openal")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("openal")
			end
			add_default_links({
				win_names  = { "openal32" },
				unix_names = { "openal" },
				osx_frameworks = { "OpenAL" },
				dbg_suffix = "",
				no_delayload = 1, -- delayload seems to cause errors on startup
			})
		end,
	},
	opengl = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("opengl")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("opengl")
			end
			add_default_links({
				win_names  = { "opengl32", "gdi32" },
				unix_names = { "GL", "X11" },
				osx_frameworks = { "OpenGL" },
				dbg_suffix = "",
				no_delayload = 1, -- delayload seems to cause errors on startup
			})
		end,
	},
	sdl = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("sdl")
			else
				-- "pkg-config sdl --libs" appears to include both static and dynamic libs
				-- when on MacPorts, which is bad, so use sdl-config instead
				pkgconfig_cflags(nil, "sdl-config --cflags")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("sdl")
			else
				pkgconfig_libs(nil, "sdl-config --libs")
			end
		end,
	},
	spidermonkey = {
		compile_settings = function()
			if _OPTIONS["with-system-mozjs185"] then
				pkgconfig_cflags("mozjs185")
				defines { "WITH_SYSTEM_MOZJS185" }
			else
				if os.is("windows") then
					include_dir = "include-win32"
				else
					include_dir = "include-unix"
				end
				configuration "Debug"
					includedirs { libraries_dir.."spidermonkey/"..include_dir }
				configuration "Release"
					includedirs { libraries_dir.."spidermonkey/"..include_dir }
				configuration { }
			end
		end,
		link_settings = function()
			if _OPTIONS["with-system-mozjs185"] then
				pkgconfig_libs("mozjs185")
			else
				configuration "Debug"
			  		links { "mozjs185-ps-debug" }
				configuration "Release"
					links { "mozjs185-ps-release" }
				configuration { }
				add_default_lib_paths("spidermonkey")
			end
		end,
	},
	valgrind = {
		compile_settings = function()
			add_default_include_paths("valgrind")
		end,
		link_settings = function()
			add_default_lib_paths("valgrind")
		end,
	},
	vorbis = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("vorbis")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("vorbis")
			end
			add_default_links({
				win_names  = { "vorbisfile" },
				unix_names = { "vorbisfile" },
				dbg_suffix = "_d",
			})
		end,
	},
	wxwidgets = {
		compile_settings = function()
			if os.is("windows") then
				includedirs { libraries_dir.."wxwidgets/include/msvc" }
				add_default_include_paths("wxwidgets")
			else
				pkgconfig_cflags(nil, "wx-config --unicode=yes --cxxflags")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				libdirs { libraries_dir.."wxwidgets/lib/vc_lib" }
				configuration "Debug"
					links { "wxmsw28ud_gl" }
				configuration "Release"
					links { "wxmsw28u_gl" }
				configuration { }
			else
				pkgconfig_libs(nil, "wx-config --unicode=yes --libs std,gl")
			end
		end,
	},
	x11 = {
		link_settings = function()
			add_default_links({
				win_names  = { },
				unix_names = { "X11" },
			})
		end,
	},
	xerces = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("xerces")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("xerces")
			end
			add_default_links({
				win_names  = { "xerces-c_2" },
				unix_names = { "xerces-c" },
				no_delayload = 1,
			})
		end,
	},
	zlib = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("zlib")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("zlib")
			end
			add_default_links({
				win_names  = { "zlib1" },
				unix_names = { "z" },
			})
		end,
	},
}


-- add a set of external libraries to the project; takes care of
-- include / lib path and linking against the import library.
-- extern_libs: table of library names [string]
-- target_type: String defining the projects kind [string]
function project_add_extern_libs(extern_libs, target_type)

	for i,extern_lib in pairs(extern_libs) do
		local def = extern_lib_defs[extern_lib]
		assert(def, "external library " .. extern_lib .. " not defined")

		if def.compile_settings then
			def.compile_settings()
		end

		-- Linking to external libraries will only be done in the main executable and not in the
		-- static libraries. Premake would silently skip linking into static libraries for some
		-- actions anyway (e.g. vs2010).
		-- On osx using xcode, if this linking would be defined in the static libraries, it would fail to
		-- link if only dylibs are available. If both *.a and *.dylib are available, it would link statically.
		-- I couldn't find any problems with that approach.
		if target_type ~= "StaticLib" and def.link_settings then
			def.link_settings()
		end
	end

	if os.is("macosx") then
		-- MacPorts uses /opt/local as the prefix for most of its libraries,
		-- which isn't on the default compiler's default include path.
		-- This needs to come after we add our own external libraries, so that
		-- our versions take precedence over the system versions (especially
		-- for SpiderMonkey), which means we have to do it per-config
		configuration "Debug"
			includedirs { "/opt/local/include" }
			libdirs { "/opt/local/lib" }
		configuration "Release"
			includedirs { "/opt/local/include" }
			libdirs { "/opt/local/lib" }
		configuration { }
	end
end
