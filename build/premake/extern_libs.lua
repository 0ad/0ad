-- this file provides package_add_extern_libs, which takes care of the
-- dirty details of adding the libraries' include and lib paths.
--
-- TYPICAL TASK: add new library. Instructions:
-- 1) add a new extern_lib_defs entry
-- 2) add library name to extern_libs tables in premake.lua for all 'packages' that want to use it


-- directory in which all library subdirectories reside.
libraries_dir = "../../../libraries/"

local function add_extern_lib_paths(extern_lib)
	-- Add '<libraries root>/<libraryname>/lib' and '/include' to the includepaths and libpaths
	
	-- Often, the headers in libraries/ are windows-specific (always, except
	-- for cxxtest and fcollada). So don't add the include dir unless on
	-- windows or processing one of those libs.
	if OS == "windows" or extern_lib == "cxxtest" or extern_lib == "fcollada" or extern_lib == "valgrind" or extern_lib == "spidermonkey" then
		tinsert(package.includepaths, libraries_dir .. extern_lib .. "/include")
		-- Insert libs at pos=1 (i.e. the front of the list) so they take
		-- precedence over system libraries
		tinsert(package.libpaths, 1, libraries_dir .. extern_lib .. "/lib")
	end

end

-- For unixes: add buildflags and linkflags from the given pkg-config module.
local function pkgconfig(lib)
	tinsert(package.buildoptions, "`pkg-config "..lib.." --cflags`")
	tinsert(package.gnu_external, "`pkg-config "..lib.." --libs`")
end

-- library definitions
-- in a perfect world, libraries would have a common installation template,
-- i.e. location of include directory, naming convention for .lib, etc.
-- this table provides a means of working around each library's differences.
--
-- the default assumptions are:
-- * extern_lib (name of library [string]) is subdirectory of libraries_dir;
-- * this directory contains include and lib subdirectories which hold the
--   appendant files
-- * debug import library and DLL are distinguished with a "d" suffix
-- * the library should be marked for delay-loading.
--
-- the following options can override these:
-- * win_names: table of import library / DLL names (no extension) when
--   running on Windows.
-- * unix_names: as above; shared object names when running on non-Windows.
-- * osx_names: as above; for OS X specificall (overrides unix_names if both are
--   specified)
-- * linux_names: ditto for Linux (overrides unix_names if both given)
--
-- win- and unix-names are 'required'; if not specified, no linking against the
-- library happens on platforms whose *_names are missing.
-- (rationale: this allows for libraries that do not link against anything,
-- e.g. Boost).
--
-- * dbg_suffix: changes the debug suffix from the above default.
--   can be "" to indicate the library doesn't have a debug build;
--   in that case, the same library (without suffix) is used in
--   all build configurations.
-- * no_delayload: indicate the library is not to be delay-loaded.
--   this is necessary for some libraries that do not support it,
--   e.g. Xerces (which is so stupid as to export variables).
-- * add_func: a function that overrides everything else. responsible for
--   setting include and library paths, adding .links (linker input), and
--   arranging for delay-loading. this is necessary e.g. for wxWidgets,
--   which is unfortunately totally incompatible with our
--   library installation rules.
-- * depends: a table of external libraries that this library depends on
-- * defines: a table of symbols to define
extern_lib_defs = {
	boost = {
		unix_names = { "boost_signals-mt", "boost_filesystem-mt", "boost_system-mt" },
		osx_names = { "boost_signals-mt", "boost_filesystem-mt", "boost_system-mt" }
	},
	cryptopp = {
	},
	cxxtest = {
	},
	misc = {
	},
	comsuppw = {
		win_names  = { "comsuppw" },
		dbg_suffix = "d",
		no_delayload = 1
	},
	devil = {
		unix_names = { "IL", "ILU" },
	},
	dl = {
		win_names  = { },
		unix_names = { "dl" },
	},
	-- rationale: see libraries_dir..enet/lib/rationale.txt
	enet =
	{
		add_func = function()
			if OS == "windows" then
				tinsert(package.includepaths, libraries_dir.."enet/include")
				tinsert(package.config["Debug"  ].libpaths, libraries_dir.."enet/lib/debug")
				tinsert(package.config["Testing"].libpaths, libraries_dir.."enet/lib/debug")
				tinsert(package.config["Release"].libpaths, libraries_dir.."enet/lib/release")
				tinsert(package.config["Debug"  ].links, "enet_dbg")
				tinsert(package.config["Testing"].links, "enet_dbg")
				tinsert(package.config["Release"].links, "enet")
			else
				-- We should be using pkgconfig, but (at least on ubuntu) that adds /usr/include/enet which contains a time.h that gets used before the system header...
				tinsert(package.links, "enet")
			end
		end,
	},
	fcollada = {
		win_names  = { "FCollada" },
		unix_names = { "FColladaSD" },
		dbg_suffix = "D",
		no_delayload = 1,
	},
	ffmpeg = {
		win_names  = { "avcodec-51", "avformat-51", "avutil-49", "swscale-0" },
		unix_names = { "avcodec", "avformat", "avutil" },
		dbg_suffix = "",
	},
	libjpg = {
		win_names  = { "jpeg-6b" },
		unix_names = { "jpeg" },
	},
	libpng = {
		win_names  = { "libpng13" },
		unix_names = { "png" },
	},
	libxml2 = {
		add_func = function()
			if OS == "windows" then
				tinsert(package.includepaths, libraries_dir.."libxml2/include")
				tinsert(package.libpaths, libraries_dir.."libxml2/lib")
				tinsert(package.config["Debug"  ].links, "libxml2")
				tinsert(package.config["Testing"].links, "libxml2")
				tinsert(package.config["Release"].links, "libxml2")
			else
				pkgconfig("libxml-2.0")
				-- libxml2 needs _REENTRANT or __MT__ for thread support;
				-- OS X doesn't get either set by default, so do it manually
				if OS == "macosx" then
					tinsert(package.defines, "_REENTRANT")
				end
			end
		end,
	},
	nspr = {
		-- On windows, this is somehow baked into the js library (ask philip)
		-- but on unix we need to explicitly include this (it's used by libjs).
		add_func = function()
			if OS ~= "windows" then
				pkgconfig("nspr")
			end
		end,
	},
	openal = {
		win_names  = { "openal32" },
		unix_names = { "openal" },
		osx_frameworks = { "OpenAL" },
		dbg_suffix = "",
	},
	opengl = {
		win_names  = { "opengl32", "glu32", "gdi32" },
		unix_names = { "GL", "GLU", "X11" },
		osx_frameworks = { "OpenGL" },
		dbg_suffix = "",
	},
	sdl = {
		add_func = function()
			add_extern_lib_paths("sdl")
			if OS ~= "windows" then
				pkgconfig("sdl")
			end
		end
	},
	spidermonkey =
	options["with-spidermonkey-tip"] and
	{
		add_func = function()
			tinsert(package.config["Debug"  ].includepaths, libraries_dir.."spidermonkey-tip/include/debug")
			tinsert(package.config["Testing"].includepaths, libraries_dir.."spidermonkey-tip/include/debug")
			tinsert(package.config["Release"].includepaths, libraries_dir.."spidermonkey-tip/include/release")
			tinsert(package.libpaths, libraries_dir.."spidermonkey-tip/lib")
			tinsert(package.config["Debug"  ].links, "mozjs-debug")
			tinsert(package.config["Testing"].links, "mozjs-release")
			tinsert(package.config["Release"].links, "mozjs-release")
		end,
	}
	or -- SpiderMonkey 1.6
	{
		win_names  = { "js32" },
		unix_names = { "js" },
		depends = { "nspr" },
		defines = { "JS_THREADSAFE" },
	},
	valgrind = {
	},
	vorbis = {
		win_names  = { "vorbisfile" },
		unix_names = { "vorbisfile" },
		dbg_suffix = "_d",
	},
	wxwidgets = {
		add_func = function()
			if OS == "windows" then
				tinsert(package.includepaths, libraries_dir.."wxwidgets/include/msvc")
				tinsert(package.includepaths, libraries_dir.."wxwidgets/include")
				tinsert(package.libpaths, libraries_dir.."wxwidgets/lib/vc_lib")
				tinsert(package.config["Debug"  ].links, "wxmsw28ud_gl")
				tinsert(package.config["Testing"].links, "wxmsw28ud_gl")
				tinsert(package.config["Release"].links, "wxmsw28u_gl")
			else
				tinsert(package.buildoptions, "`wx-config --unicode=yes --cxxflags`")
				tinsert(package.gnu_external, "`wx-config --unicode=yes --libs std,gl`")
			end
		end,
	},
	x11 = {
		win_names  = { },
		unix_names = { "X11" },
	},
	xerces = {
		win_names  = { "xerces-c_2" },
		unix_names = { "xerces-c" },
		no_delayload = 1,
	},
	zlib = {
		win_names  = { "zlib1" },
		unix_names = { "z" },
	},
}


local function add_delayload(name, suffix, def)

	if def["no_delayload"] then
		return
	end
	
	-- currently only supported by VC; nothing to do on other platforms.
	if OS ~= "windows" then
		return
	end

	-- no extra debug version; use same library in all configs
	if suffix == "" then
		tinsert(package.linkoptions, "/DELAYLOAD:"..name..".dll")
	-- extra debug version available; use in debug/testing config
	else
		local dbg_cmd = "/DELAYLOAD:" .. name .. suffix .. ".dll"
		local cmd     = "/DELAYLOAD:" .. name .. ".dll"

		tinsert(package.config["Debug"  ].linkoptions, dbg_cmd)
		-- 'Testing' config uses 'Debug' DLLs
		tinsert(package.config["Testing"].linkoptions, dbg_cmd)
		tinsert(package.config["Release"].linkoptions, cmd)
	end

end

local function add_extern_lib(extern_lib, def)

	add_extern_lib_paths(extern_lib)

	-- careful: make sure to only use *_names when on the correct platform.
	local names = {}
	if OS == "windows" then
		if def.win_names then
			names = def.win_names
		end
	elseif OS == "linux" and def.linux_names then
		names = def.linux_names
	elseif OS == "macosx" and def.osx_names then
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
	if OS ~= "windows" then
		suffix = ""
	end

	if def.defines then
		tinsert(package.defines, def.defines)
	end

	-- OS X "Frameworks" need to be added in a special way to the link
	-- i.e. by linkoptions += "-framework ..."
	if OS == "macosx" and def.osx_frameworks then
		for i,name in pairs(def.osx_frameworks) do
			tinsert(package.linkoptions, "-framework " .. name)
		end
	else
		for i,name in pairs(names) do
			tinsert(package.config["Debug"  ].links, name .. suffix)
			-- 'Testing' config uses 'Debug' DLLs
			tinsert(package.config["Testing"].links, name .. suffix)
			tinsert(package.config["Release"].links, name)
	
			add_delayload(name, suffix, def)
		end
	end
end


-- add a set of external libraries to the package; takes care of
-- include / lib path and linking against the import library.
-- extern_libs: table of library names [string]
function package_add_extern_libs(extern_libs)

	local function add_with_deps(libs, lib)
		local def = extern_lib_defs[lib]
		assert(def, "external library " .. lib .. " not defined")
		tinsert(libs, lib)
		if def.depends then
			for i,dep in pairs(def.depends) do
				add_with_deps(libs, dep)
			end
		end
	end

	local libs = {}

	for i,extern_lib in pairs(extern_libs) do
		add_with_deps(libs, extern_lib)
	end

	for i,extern_lib in pairs(libs) do
		local def = extern_lib_defs[extern_lib]

		if def.add_func then
			def.add_func()
		else
			add_extern_lib(extern_lib, def)
		end
	end
end
