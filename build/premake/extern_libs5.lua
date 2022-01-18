-- this file provides project_add_extern_libs, which takes care of the
-- dirty details of adding the libraries' include and lib paths.
--
-- TYPICAL TASK: add new library. Instructions:
-- 1) add a new extern_lib_defs entry
-- 2) add library name to extern_libs tables in premake.lua for all 'projects' that want to use it

-- directory in which OS-specific library subdirectories reside.
if os.istarget("macosx") then
	libraries_dir = rootdir.."/libraries/osx/"
elseif os.istarget("windows") then
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
	sysincludedirs { libraries_dir .. extern_lib .. "/include" }
end

local function add_source_include_paths(extern_lib)
	sysincludedirs { libraries_source_dir .. extern_lib .. "/include" }
end

local function add_third_party_include_paths(extern_lib)
	sysincludedirs { third_party_source_dir .. extern_lib .. "/include" }
end

pkgconfig = require "pkgconfig"

-- Configure pkgconfig for MacOSX systems
if os.istarget("macosx") then
	pkgconfig.additional_pc_path = libraries_dir .. "pkgconfig/"
	pkgconfig.static_link_libs = true
end

local function add_delayload(name, suffix, def)

	if def["no_delayload"] then
		return
	end

	-- currently only supported by VC; nothing to do on other platforms.
	if not os.istarget("windows") then
		return
	end

	-- no extra debug version; use same library in all configs
	if suffix == "" then
		linkoptions { "/DELAYLOAD:"..name..".dll" }
	-- extra debug version available; use in debug config
	else
		local dbg_cmd = "/DELAYLOAD:" .. name .. suffix .. ".dll"
		local cmd     = "/DELAYLOAD:" .. name .. ".dll"

		filter "Debug"
			linkoptions { dbg_cmd }
		filter "Release"
			linkoptions { cmd }
		filter { }
	end

end

local function add_default_links(def)

	-- careful: make sure to only use *_names when on the correct platform.
	local names = {}
	if os.istarget("windows") then
		if def.win_names then
			names = def.win_names
		end
	elseif _OPTIONS["android"] and def.android_names then
		names = def.android_names
	elseif os.istarget("linux") and def.linux_names then
		names = def.linux_names
	elseif os.istarget("macosx") and (def.osx_names or def.osx_frameworks) then
		if def.osx_names then
			names = def.osx_names
		end
		-- OS X "Frameworks" are added to the links as "name.framework"
		if def.osx_frameworks then
			for i,name in pairs(def.osx_frameworks) do
				links { name .. ".framework" }
			end
		end
	elseif os.istarget("bsd") and def.bsd_names then
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
	if not os.istarget("windows") then
		suffix = ""
	end

	for i,name in pairs(names) do
		filter "Debug"
			links { name .. suffix }
		filter "Release"
			links { name }
		filter { }

		add_delayload(name, suffix, def)
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
--      Description: Adds links to libraries and configures delayloading.
--      If the *_names parameter for a plattform is missing, no linking will be done
--      on that plattform.
--      The default assumptions are:
--      * debug import library and DLL are distinguished with a "d" suffix
--      * the library should be marked for delay-loading.
--      Parameters:
--      * win_names: table of import library / DLL names (no extension) when
--        running on Windows.
--      * unix_names: as above; shared object names when running on non-Windows.
--      * osx_names, osx_frameworks: for OS X specifically; if any of those is
--        specified, unix_names is ignored. Using both is possible if needed.
--          * osx_names: as above.
--          * osx_frameworks: as above, for system libraries that need to be linked
--            as "name.framework".
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
			if os.istarget("windows") then
				-- Force the autolink to use the vc141 libs.
				defines { 'BOOST_LIB_TOOLSET="vc141"' }
				add_default_include_paths("boost")
			elseif os.istarget("macosx") then
				-- Suppress all the Boost warnings on OS X by including it as a system directory
				buildoptions { "-isystem../" .. libraries_dir .. "boost/include" }
			end
			-- TODO: This actually applies to most libraries we use on BSDs, make this a global setting.
			if os.istarget("bsd") then
				sysincludedirs { "/usr/local/include" }
			end
		end,
		link_settings = function()
			if os.istarget("windows") or os.istarget("macosx") then
				if os.istarget("windows") then
					defines { 'BOOST_LIB_TOOLSET="vc141"' }
				end
				add_default_lib_paths("boost")
			end
			add_default_links({
				-- The following are not strictly link dependencies on all systems, but
				-- are included for compatibility with different versions of Boost
				android_names = { "boost_filesystem-gcc-mt", "boost_system-gcc-mt" },
				unix_names = { os.findlib("boost_filesystem-mt") and "boost_filesystem-mt" or "boost_filesystem", os.findlib("boost_system-mt") and "boost_system-mt" or "boost_system" },
				osx_names = { "boost_filesystem", "boost_system" },
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
			sysincludedirs { libraries_source_dir .. "cxxtest-4.4" }
		end,
	},
	enet = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("enet")
			else
				pkgconfig.add_includes("libenet")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("enet")
				add_default_links({
					win_names  = { "enet" },
				})
			else
				pkgconfig.add_links("libenet")
			end
		end,
	},
	fcollada = {
		compile_settings = function()
			add_source_include_paths("fcollada")
		end,
		link_settings = function()
			add_source_lib_paths("fcollada")
			if os.istarget("windows") then
				filter "Debug"
					links { "FColladaD" }
				filter "Release"
					links { "FCollada" }
				filter { }
			else
				filter "Debug"
					links { "FColladaSD" }
				filter "Release"
					links { "FColladaSR" }
				filter { }
			end
		end,
	},
	fmt = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("fmt")
			elseif os.istarget("macosx") then
				pkgconfig.add_includes("fmt")
			end

			-- With Linux & BSD, we assume that fmt is installed in a standard location.
			--
			-- It would be nice to not assume, and to instead use pkg-config: however that
			-- requires fmt 5.3.0 or greater.
			--
			-- Unfortunately (at the time of writing) only 92 out of 114 (~80.7%) of distros
			-- that provide a fmt package meet this, according to
			-- https://repology.org/badge/vertical-allrepos/fmt.svg?minversion=5.3
			--
			-- Whilst that might seem like a healthy majority, this does not include the 2018
			-- Long Term Support and 2019.10 releases of Ubuntu - not only popular and widely
			-- used as-is, but which are also used as a base for other popular distros (e.g.
			-- Mint).
			--
			-- When fmt 5.3 (or better) becomes more widely used, then we can safely use the
			-- same line as we currently use for osx
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("fmt")
				add_default_links({
					win_names = { "fmt" },
					dbg_suffix = "d",
					no_delayload = 1,
				})
			elseif os.istarget("macosx") then
				-- See comment above as to why this is not also used on Linux or BSD.
				pkgconfig.add_links("fmt")
			else
				add_default_links({
					unix_names = { "fmt" },
				})
			end
		end
	},
	freetype = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("freetype")
			else
				pkgconfig.add_includes("freetype2")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("freetype")
			else
				pkgconfig.add_links("freetype2")
			end
			add_default_links({
				win_names = { "freetype" },
				no_delayload = 1,
			})
		end,
	},
	glad = {
		add_source_include_paths("glad")
	},
	gloox = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("gloox")
			else
				pkgconfig.add_includes("gloox")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("gloox")
				add_default_links({
					win_names  = { "gloox-1.0" },
					no_delayload = 1,
				})
			else
				pkgconfig.add_links("gloox")

				if os.istarget("macosx") then
					-- gloox depends on gnutls, but doesn't identify this via pkg-config
					pkgconfig.add_links("gnutls")
				end
			end
		end,
	},
	iconv = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("iconv")
				defines { "HAVE_ICONV_CONST" }
				defines { "ICONV_CONST=const" }
				defines { "LIBICONV_STATIC" }
			elseif os.istarget("macosx") then
				add_default_include_paths("iconv")
				defines { "LIBICONV_STATIC" }
			elseif os.getversion().description == "FreeBSD" then
				-- On FreeBSD you need this flag to tell it to use the BSD libc iconv
				defines { "LIBICONV_PLUG" }
			end
		end,
		link_settings = function()
			if os.istarget("windows") or os.istarget("macosx") then
				add_default_lib_paths("iconv")
			end
			add_default_links({
				win_names  = { "libiconv" },
				osx_names = { "iconv" },
				dbg_suffix = "",
				no_delayload = 1,
			})
			-- glibc (used on Linux and GNU/kFreeBSD) has iconv
		end,
	},
	icu = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("icu")
			else
				pkgconfig.add_includes("icu-i18n")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("icu")
				add_default_links({
					win_names  = { "icuuc", "icuin" },
					dbg_suffix = "",
					no_delayload = 1,
				})
			else
				pkgconfig.add_links("icu-i18n")
			end
		end,
	},
	libcurl = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("libcurl")
			else
				pkgconfig.add_includes("libcurl")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("libcurl")
			else
				pkgconfig.add_links("libcurl")
			end
			add_default_links({
				win_names  = { "libcurl" },
				osx_frameworks = { "Security" }, -- Not supplied by curl's pkg-config
			})
		end,
	},
	libpng = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("libpng")
			else
				pkgconfig.add_includes("libpng")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("libpng")
				add_default_links({
					win_names  = { "libpng16" },
				})
			else
				pkgconfig.add_links("libpng")
			end
		end,
	},
	libsodium = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("libsodium")
			else
				pkgconfig.add_includes("libsodium")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("libsodium")
				add_default_links({
					win_names  = { "libsodium" },
				})
			else
				pkgconfig.add_links("libsodium")
			end
		end,
	},
	libxml2 = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("libxml2")
			else
				pkgconfig.add_includes("libxml-2.0")

				if os.istarget("macosx") then
					-- libxml2 needs _REENTRANT or __MT__ for thread support;
					-- OS X doesn't get either set by default, so do it manually
					defines { "_REENTRANT" }
				end
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("libxml2")
				filter "Debug"
					links { "libxml2" }
				filter "Release"
					links { "libxml2" }
				filter { }
			else
				pkgconfig.add_links("libxml-2.0")
			end
		end,
	},
	miniupnpc = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("miniupnpc")
			elseif os.istarget("macosx") then
				pkgconfig.add_includes("miniupnpc")
			end

			-- On Linux and BSD systems we assume miniupnpc is installed in a standard location.
			--
			-- Support for pkg-config was added in v2.1 of miniupnpc (May 2018). However, the
			-- implementation was flawed - it provided the wrong path to the project's headers.
			-- This was corrected in v2.2.1 (December 2020).
			--
			-- At the time of writing, of the 123 Linux and BSD package repositories tracked by
			-- Repology that supply a version of miniupnpc:
			-- * 88 (~71.54%) have >= v2.1, needed to locate libraries
			-- * 50 (~40.65%) have >= v2.2.1, needed to (correctly) locate headers
			--
			-- Once more recent versions become more widespread, we can safely start to use
			-- pkg-config to find miniupnpc on Linux and BSD systems.
			-- https://repology.org/badge/vertical-allrepos/miniupnpc.svg?minversion=2.2.1
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("miniupnpc")
				add_default_links({
					win_names  = { "miniupnpc" },
				})
			elseif os.istarget("macosx") then
				pkgconfig.add_links("miniupnpc")
			else
				-- Once miniupnpc v2.1 or better becomes near-universal (see above comment),
				-- we can use pkg-config for Linux and BSD.
				add_default_links({
					unix_names = { "miniupnpc" },
				})
			end
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
				osx_names = { "bc6h", "bc7", "nvcore", "nvimage", "nvmath", "nvthread", "nvtt", "squish" },
				dbg_suffix = "", -- for performance we always use the release-mode version
			})
		end,
	},
	openal = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("openal")
			elseif not os.istarget("macosx") then
				pkgconfig.add_includes("openal")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("openal")
				add_default_links({
					win_names  = { "openal32" },
					dbg_suffix = "",
					no_delayload = 1, -- delayload seems to cause errors on startup
				})
			elseif os.istarget("macosx") then
				add_default_links({
					osx_frameworks = { "OpenAL" },
				})
			else
				pkgconfig.add_links("openal")
			end
		end,
	},
	sdl = {
		compile_settings = function()
			if os.istarget("windows") then
				includedirs { libraries_dir .. "sdl2/include/SDL" }
			elseif not _OPTIONS["android"] then
				pkgconfig.add_includes("sdl2")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("sdl2")
			elseif not _OPTIONS["android"] then
				pkgconfig.add_links("sdl2")
			end
		end,
	},
	spidermonkey = {
		compile_settings = function()
			if _OPTIONS["with-system-mozjs"] then
				if not _OPTIONS["android"] then
					pkgconfig.add_includes("mozjs-78")
				end
			else
				if os.istarget("windows") then
					include_dir = "include-win32"
					buildoptions { "/FI\"js/RequiredDefines.h\"" }
				else
					include_dir = "include-unix"
				end
				filter "Debug"
					sysincludedirs { libraries_source_dir.."spidermonkey/"..include_dir.."-debug" }
					defines { "DEBUG" }
				filter "Release"
					sysincludedirs { libraries_source_dir.."spidermonkey/"..include_dir.."-release" }
				filter { }
			end
		end,
		link_settings = function()
			if _OPTIONS["with-system-mozjs"] then
				if _OPTIONS["android"] then
					links { "mozjs-78" }
				else
					pkgconfig.add_links("mozjs-78")
				end
			else
				filter { "Debug", "action:vs*" }
					links { "mozjs78-ps-debug" }
					links { "mozjs78-ps-rust-debug" }
				filter { "Debug", "action:not vs*" }
					links { "mozjs78-ps-debug" }
					links { "mozjs78-ps-rust" }
				filter { "Release" }
					links { "mozjs78-ps-release" }
					links { "mozjs78-ps-rust" }
				filter { }
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
			-- Optional dependency
			--
			-- valgrind doesn't support windows:
			--   https://valgrind.org/info/platforms.html
			if _OPTIONS["with-valgrind"] and not os.istarget("windows") then
				pkgconfig.add_includes("valgrind")
				defines { "CONFIG2_VALGRIND=1" }
			end
		end,
	},
	vorbis = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("vorbis")
			else
				pkgconfig.add_includes("ogg")
				pkgconfig.add_includes("vorbisfile")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("vorbis")
				add_default_links({
					win_names  = { "libvorbisfile" },
				})
			elseif os.getversion().description == "OpenBSD" then
				-- TODO: We need to force linking with these as currently
				-- they need to be loaded explicitly on execution
				add_default_links({
					unix_names = { "ogg", "vorbis" },
				})
			else
				pkgconfig.add_links("vorbisfile")
			end
		end,
	},
	wxwidgets = {
		compile_settings = function()
			if os.istarget("windows") then
				includedirs { libraries_dir.."wxwidgets/include/msvc" }
				add_default_include_paths("wxwidgets")
			else
				-- wxwidgets does not come with a definition file for pkg-config,
				-- so we have to use wxwidgets' own config tool
				wx_config_path = os.getenv("WX_CONFIG") or "wx-config"
				pkgconfig.add_includes(nil, wx_config_path, "--unicode=yes --cxxflags")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				libdirs { libraries_dir.."wxwidgets/lib/vc_lib" }
			else
				wx_config_path = os.getenv("WX_CONFIG") or "wx-config"
				pkgconfig.add_links(nil, wx_config_path, "--unicode=yes --libs std,gl")
			end
		end,
	},
	x11 = {
		compile_settings = function()
			if not os.istarget("windows") and not os.istarget("macosx") then
				pkgconfig.add_includes("x11")
			end
		end,
		link_settings = function()
			if not os.istarget("windows") and not os.istarget("macosx") then
				pkgconfig.add_links("x11")
			end
		end,
	},
	zlib = {
		compile_settings = function()
			if os.istarget("windows") then
				add_default_include_paths("zlib")
			else
				pkgconfig.add_includes("zlib")
			end
		end,
		link_settings = function()
			if os.istarget("windows") then
				add_default_lib_paths("zlib")
				add_default_links({
					win_names  = { "zlib1" },
					no_delayload = 1,
				})
			else
				pkgconfig.add_links("zlib")
			end
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
