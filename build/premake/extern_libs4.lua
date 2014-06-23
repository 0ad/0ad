-- this file provides project_add_extern_libs, which takes care of the
-- dirty details of adding the libraries' include and lib paths.
--
-- TYPICAL TASK: add new library. Instructions:
-- 1) add a new extern_lib_defs entry
-- 2) add library name to extern_libs tables in premake.lua for all 'projects' that want to use it


-- directory in which OS-specific library subdirectories reside.
if os.is("macosx") then
	libraries_dir = rootdir.."/libraries/osx/"
elseif os.is("windows") then
	libraries_dir = rootdir.."/libraries/win32/"
else
	-- No Unix-specific libs yet (use source directory instead!)
end
-- directory for shared, bundled libraries
libraries_source_dir = rootdir.."/libraries/source/"
third_party_source_dir = rootdir.."/source/third_party/"

local function add_default_lib_paths(extern_lib)
	libdirs { libraries_dir .. extern_lib .. "/lib" }
end

local function add_source_lib_paths(extern_lib)
	libdirs { libraries_source_dir .. extern_lib .. "/lib" }
end

local function add_default_include_paths(extern_lib)
	includedirs { libraries_dir .. extern_lib .. "/include" }
end

local function add_source_include_paths(extern_lib)
	includedirs { libraries_source_dir .. extern_lib .. "/include" }
end

local function add_third_party_include_paths(extern_lib)
	includedirs { third_party_source_dir .. extern_lib .. "/include" }
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

	if _ACTION == "xcode3" or _ACTION == "xcode4" then
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

	if _ACTION == "xcode3" or _ACTION == "xcode4" then
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
	elseif _OPTIONS["android"] and def.android_names then
		names = def.android_names
	elseif os.is("linux") and def.linux_names then
		names = def.linux_names
	elseif os.is("macosx") and def.osx_names then
		names = def.osx_names
	elseif os.is("bsd") and def.bsd_names then
		names = def.bsd_names
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
--      * osx_names: as above; for OS X specifically (overrides unix_names if both are
--        specified)
--      * bsd_names: as above; for BSD specifically (overrides unix_names if both are
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
			elseif os.is("macosx") then
				-- Suppress all the Boost warnings on OS X by including it as a system directory
				buildoptions { "-isystem../" .. libraries_dir .. "boost/include" }
			end
			if os.getversion().description == "OpenBSD" then
				includedirs { "/usr/local/include" }
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
				add_default_lib_paths("boost")
			end
			add_default_links({
				-- The following are not strictly link dependencies on all systems, but
				-- are included for compatibility with different versions of Boost
				android_names = { "boost_filesystem-gcc-mt", "boost_system-gcc-mt" },
				unix_names = { os.findlib("boost_filesystem-mt") and "boost_filesystem-mt" or "boost_filesystem", os.findlib("boost_system-mt") and "boost_system-mt" or "boost_system" },
				osx_names = { "boost_filesystem-mt", "boost_system-mt" },
			})
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
	cxxtest = {
		compile_settings = function()
			includedirs { libraries_source_dir .. "cxxtest-4.3" }
		end,
		link_settings = function()
			add_source_lib_paths("cxxtest-4.3")
		end,
	},
	enet = {
		compile_settings = function()
			if not _OPTIONS["with-system-enet"] then
				add_source_include_paths("enet")
			end
		end,
		link_settings = function()
			if not _OPTIONS["with-system-enet"] then
				add_source_lib_paths("enet")
			end
			add_default_links({
				win_names  = { "enet" },
				unix_names = { "enet" },
			})
		end,
	},
	fcollada = {
		compile_settings = function()
			add_source_include_paths("fcollada")
		end,
		link_settings = function()
			add_source_lib_paths("fcollada")
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
	gloox = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("gloox")
			elseif os.is("macosx") then
				-- Support GLOOX_CONFIG for overriding the default PATH-based gloox-config
				gloox_config_path = os.getenv("GLOOX_CONFIG")
				if not gloox_config_path then
					gloox_config_path = "gloox-config"
				end
				pkgconfig_cflags(nil, gloox_config_path.." --cflags")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("gloox")
			end
			if os.is("macosx") then
				gloox_config_path = os.getenv("GLOOX_CONFIG")
				if not gloox_config_path then
					gloox_config_path = "gloox-config"
				end
				pkgconfig_libs(nil, gloox_config_path.." --libs")
			else
				-- TODO: consider using pkg-config on non-Windows (for compile_settings too)
				add_default_links({
					win_names  = { "gloox-1.0" },
					unix_names = { "gloox" },
				})
			end
		end,
	},
	iconv = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("iconv")
				defines { "HAVE_ICONV_CONST" }
				defines { "LIBICONV_STATIC" }
			elseif os.is("macosx") then
				add_default_include_paths("iconv")
				defines { "LIBICONV_STATIC" }
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
				add_default_lib_paths("iconv")
			end
			add_default_links({
				win_names  = { "libiconv" },
				-- TODO: glibc provides symbols for this, so we should only include that (and depend on libiconv) on non-glibc unix
				osx_names = { "iconv" },
				dbg_suffix = "",
			})
		end,
	},
	icu = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("icu")
			elseif os.is("macosx") then
				-- Support ICU_CONFIG for overriding the default PATH-based icu-config
				icu_config_path = os.getenv("ICU_CONFIG")
				if not icu_config_path then
					icu_config_path = "icu-config"
				end
				pkgconfig_cflags(nil, icu_config_path.." --cppflags")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("icu")
			end
			if os.is("macosx") then
				icu_config_path = os.getenv("ICU_CONFIG")
				if not icu_config_path then
					icu_config_path = "gloox-config"
				end
				pkgconfig_libs(nil, icu_config_path.." --ldflags-searchpath --ldflags-libsonly --ldflags-system")
			else
				add_default_links({
					win_names  = { "icuuc", "icuin" },
					unix_names = { "icui18n", "icuuc" },
					dbg_suffix = "",
				})
			end
		end,
	},
	libcurl = {
		compile_settings = function()
			if os.is("windows") or os.is("macosx") then
				add_default_include_paths("libcurl")
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
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
			if os.is("windows") or os.is("macosx") then
				add_default_include_paths("libjpg")
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
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
			if os.is("windows") or os.is("macosx") then
				add_default_include_paths("libpng")
			end
			if os.getversion().description == "OpenBSD" then
				includedirs { "/usr/local/include/libpng" }
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
				add_default_lib_paths("libpng")
			end
			add_default_links({
				win_names  = { "libpng15" },
				unix_names = { "png" },
				-- Otherwise ld will sometimes pull in ancient 1.2 from the SDK, which breaks the build :/
				-- TODO: Figure out why that happens
				osx_names = { "png15" },
			})
		end,
	},
	libxml2 = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("libxml2")
			elseif os.is("macosx") then
				-- Support XML2_CONFIG for overriding for the default PATH-based xml2-config
				xml2_config_path = os.getenv("XML2_CONFIG")
				if not xml2_config_path then
					xml2_config_path = "xml2-config"
				end

				-- use xml2-config instead of pkg-config on OS X
				pkgconfig_cflags(nil, xml2_config_path.." --cflags")
				-- libxml2 needs _REENTRANT or __MT__ for thread support;
				-- OS X doesn't get either set by default, so do it manually
				defines { "_REENTRANT" }
			else
				pkgconfig_cflags("libxml-2.0")
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
			elseif os.is("macosx") then
				xml2_config_path = os.getenv("XML2_CONFIG")
				if not xml2_config_path then
					xml2_config_path = "xml2-config"
				end
				pkgconfig_libs(nil, xml2_config_path.." --libs")
			else
				pkgconfig_libs("libxml-2.0")
			end
		end,
	},
	miniupnpc = {
		compile_settings = function()
			if not _OPTIONS["with-system-miniupnpc"] then
				add_source_include_paths("miniupnpc")
			end
		end,
		link_settings = function()
			if not _OPTIONS["with-system-miniupnpc"] then
				add_source_lib_paths("miniupnpc")
			end
			add_default_links({
				win_names  = { "miniupnpc" },
				unix_names = { "miniupnpc" },
			})
		end,
	},
	nvtt = {
		compile_settings = function()
			if not _OPTIONS["with-system-nvtt"] then
				add_source_include_paths("nvtt")
			end
			defines { "NVTT_SHARED=1" }
		end,
		link_settings = function()
			if not _OPTIONS["with-system-nvtt"] then
				add_source_lib_paths("nvtt")
			end
			add_default_links({
				win_names  = { "nvtt" },
				unix_names = { "nvcore", "nvmath", "nvimage", "nvtt" },
				osx_names = { "nvcore", "nvmath", "nvimage", "nvtt", "squish" },
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
			if _OPTIONS["gles"] then
				add_default_links({
					unix_names = { "GLESv2" },
					dbg_suffix = "",
				})
			else
				add_default_links({
					win_names  = { "opengl32", "gdi32" },
					unix_names = { "GL" },
					osx_frameworks = { "OpenGL" },
					dbg_suffix = "",
					no_delayload = 1, -- delayload seems to cause errors on startup
				})
			end
		end,
	},
	sdl = {
		compile_settings = function()
			if os.is("windows") then
				includedirs { libraries_dir .. "sdl/include/SDL" }
			elseif not _OPTIONS["android"] then
				-- Support SDL_CONFIG for overriding for the default PATH-based sdl-config
				sdl_config_path = os.getenv("SDL_CONFIG")
				if not sdl_config_path then
					sdl_config_path = "sdl-config"
				end

				-- "pkg-config sdl --libs" appears to include both static and dynamic libs
				-- when on MacPorts, which is bad, so use sdl-config instead
				pkgconfig_cflags(nil, sdl_config_path.." --cflags")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("sdl")
			elseif not _OPTIONS["android"] then
				sdl_config_path = os.getenv("SDL_CONFIG")
				if not sdl_config_path then
					sdl_config_path = "sdl-config"
				end
				pkgconfig_libs(nil, sdl_config_path.." --libs")
			end
		end,
	},
	spidermonkey = {
		compile_settings = function()
			if _OPTIONS["with-system-mozjs24"] then
				if not _OPTIONS["android"] then
					pkgconfig_cflags("mozjs-24")
				end
				defines { "WITH_SYSTEM_MOZJS24" }
			else
				if os.is("windows") then
					include_dir = "include-win32"
				else
					include_dir = "include-unix"
				end
				configuration "Debug"
					includedirs { libraries_source_dir.."spidermonkey/"..include_dir.."-debug" }
					defines { "DEBUG" }
				configuration "Release"
					includedirs { libraries_source_dir.."spidermonkey/"..include_dir.."-release" }
				configuration { }
			end
		end,
		link_settings = function()
			if _OPTIONS["with-system-mozjs24"] then
				if _OPTIONS["android"] then
					links { "mozjs-24" }
				else
					pkgconfig_libs("nspr")
					pkgconfig_libs("mozjs-24")
				end
			else
				if os.is("macosx") then
					add_default_lib_paths("nspr")
					links { "nspr4", "plc4", "plds4" }
				end
				configuration "Debug"
					links { "mozjs24-ps-debug" }
				configuration "Release"
					links { "mozjs24-ps-release" }
				configuration { }
				add_source_lib_paths("spidermonkey")
			end
		end,
	},
	tinygettext = {
		compile_settings = function()
			add_third_party_include_paths("tinygettext")
		end,
	},
	valgrind = {
		compile_settings = function()
			add_source_include_paths("valgrind")
		end,
		link_settings = function()
			add_source_lib_paths("valgrind")
		end,
	},
	vorbis = {
		compile_settings = function()
			if os.is("windows") then
				add_default_include_paths("vorbis")
			elseif os.is("macosx") then
				add_default_include_paths("libogg")
				add_default_include_paths("vorbis")
			end
		end,
		link_settings = function()
			if os.is("windows") then
				add_default_lib_paths("vorbis")
			elseif os.is("macosx") then
				add_default_lib_paths("libogg")
				add_default_lib_paths("vorbis")
			end
			-- TODO: We need to force linking with these as currently
			-- they need to be loaded explicitly on execution
			if os.getversion().description == "OpenBSD" then
				add_default_links({
					unix_names = { "ogg",
						"vorbis" },
				})
			end
			add_default_links({
				win_names  = { "vorbisfile" },
				unix_names = { "vorbisfile" },
				osx_names = { "vorbis", "vorbisenc", "vorbisfile", "ogg" },
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

				-- Support WX_CONFIG for overriding for the default PATH-based wx-config
				wx_config_path = os.getenv("WX_CONFIG")
				if not wx_config_path then
					wx_config_path = "wx-config"
				end

				pkgconfig_cflags(nil, wx_config_path.." --unicode=yes --cxxflags")
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
				wx_config_path = os.getenv("WX_CONFIG")
				if not wx_config_path then
					wx_config_path = "wx-config"
				end
				pkgconfig_libs(nil, wx_config_path.." --unicode=yes --libs std,gl")
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
	xcursor = {
		link_settings = function()
			add_default_links({
				unix_names = { "Xcursor" },
			})
		end,
	},
	zlib = {
		compile_settings = function()
			if os.is("windows") or os.is("macosx") then
				add_default_include_paths("zlib")
			end
		end,
		link_settings = function()
			if os.is("windows") or os.is("macosx") then
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
end
