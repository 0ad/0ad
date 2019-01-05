--
-- d/tools/ldc.lua
-- Provides LDC-specific configuration strings.
-- Copyright (c) 2013-2015 Andrew Gough, Manu Evans, and the Premake project
--

	local p = premake
	p.tools.ldc = { }

	local ldc = p.tools.ldc
	local project = p.project
	local config = p.config
	local d = p.modules.d


--
-- Set default tools
--

	ldc.namestyle = "posix"


--
-- Returns list of D compiler flags for a configuration.
--


	ldc.dflags = {
		architecture = {
			x86 = "-m32",
			x86_64 = "-m64",
--			arm = "-march=arm",
--			ppc = "-march=ppc32",
--			ppc64 = "-march=ppc64",
--			spu = "-march=cellspu",
--			mips = "-march=mips",	-- -march=mipsel?
		},
		flags = {
			OmitDefaultLibrary		= "-mscrtlib=",
			CodeCoverage			= "-cov",
			Documentation			= "-D",
			FatalWarnings			= "-w", -- Use LLVM flag? : "-fatal-assembler-warnings",
			GenerateHeader			= "-H",
			GenerateJSON			= "-X",
--			Release					= "-release",
			RetainPaths				= "-op",
			SymbolsLikeC			= "-gc",
			UnitTest				= "-unittest",
			Verbose					= "-v",
			AllTemplateInst			= "-allinst",
			BetterC					= "-betterC",
			Main					= "-main",
			PerformSyntaxCheckOnly	= "-o-",
			ShowGC					= "-vgc",
			IgnorePragma			= "-ignore",
		},
		boundscheck = {
			Off = "-boundscheck=off",
			On = "-boundscheck=on",
			SafeOnly = "-boundscheck=safeonly",
		},
		deprecatedfeatures = {
			Allow = "-d",
			Warn = "-dw",
			Error = "-de",
		},
		floatingpoint = {
			Fast = "-fp-contract=fast -enable-unsafe-fp-math",
--			Strict = "-ffloat-store",
		},
		optimize = {
			Off = "-O0",
			On = "-O2",
			Debug = "-O0",
			Full = "-O3",
			Size = "-Oz",
			Speed = "-O3",
		},
		pic = {
			On = "-relocation-model=pic",
		},
		vectorextensions = {
			AVX = "-mattr=+avx",
			SSE = "-mattr=+sse",
			SSE2 = "-mattr=+sse2",
		},
		warnings = {
			Default = "-wi",
			High = "-wi",
			Extra = "-wi",	-- TODO: is there a way to get extra warnings?
		},
		symbols = {
			On = "-g",
			FastLink = "-g",
			Full = "-g",
		}
	}

	function ldc.getdflags(cfg)
		local flags = config.mapFlags(cfg, ldc.dflags)

		if config.isDebugBuild(cfg) then
			table.insert(flags, "-d-debug")
		else
			table.insert(flags, "-release")
		end

		if not cfg.flags.OmitDefaultLibrary then
			local releaseruntime = not config.isDebugBuild(cfg)
			local staticruntime = true
			if cfg.staticruntime == "Off" then
				staticruntime = false
			end
			if cfg.runtime == "Debug" then
				releaseruntime = false
			elseif cfg.runtime == "Release" then
				releaseruntime = true
			end

			if (cfg.staticruntime and cfg.staticruntime ~= "Default") or (cfg.runtime and cfg.runtime ~= "Default") then
				if staticruntime == true and releaseruntime == true then
					table.insert(flags, "-mscrtlib=libcmt")
				elseif staticruntime == true and releaseruntime == false then
					table.insert(flags, "-mscrtlib=libcmtd")
				elseif staticruntime == false and releaseruntime == true then
					table.insert(flags, "-mscrtlib=msvcrt")
				elseif staticruntime == false and releaseruntime == false then
					table.insert(flags, "-mscrtlib=msvcrtd")
				end
			end
		end

		if cfg.flags.Documentation then
			if cfg.docname then
				table.insert(flags, "-Df=" .. p.quoted(cfg.docname))
			end
			if cfg.docdir then
				table.insert(flags, "-Dd=" .. p.quoted(cfg.docdir))
			end
		end
		if cfg.flags.GenerateHeader then
			if cfg.headername then
				table.insert(flags, "-Hf=" .. p.quoted(cfg.headername))
			end
			if cfg.headerdir then
				table.insert(flags, "-Hd=" .. p.quoted(cfg.headerdir))
			end
		end

		return flags
	end


--
-- Decorate versions for the DMD command line.
--

	function ldc.getversions(versions, level)
		local result = {}
		for _, version in ipairs(versions) do
			table.insert(result, '-d-version=' .. version)
		end
		if level then
			table.insert(result, '-d-version=' .. level)
		end
		return result
	end


--
-- Decorate debug constants for the DMD command line.
--

	function ldc.getdebug(constants, level)
		local result = {}
		for _, constant in ipairs(constants) do
			table.insert(result, '-d-debug=' .. constant)
		end
		if level then
			table.insert(result, '-d-debug=' .. level)
		end
		return result
	end


--
-- Decorate import file search paths for the DMD command line.
--

	function ldc.getimportdirs(cfg, dirs)
		local result = {}
		for _, dir in ipairs(dirs) do
			dir = project.getrelative(cfg.project, dir)
			table.insert(result, '-I=' .. p.quoted(dir))
		end
		return result
	end


--
-- Decorate import file search paths for the DMD command line.
--

	function ldc.getstringimportdirs(cfg, dirs)
		local result = {}
		for _, dir in ipairs(dirs) do
			dir = project.getrelative(cfg.project, dir)
			table.insert(result, '-J=' .. p.quoted(dir))
		end
		return result
	end


--
-- Returns the target name specific to compiler
--

	function ldc.gettarget(name)
		return "-of=" .. name
	end


--
-- Return a list of LDFLAGS for a specific configuration.
--

	ldc.ldflags = {
		architecture = {
			x86 = { "-m32" },
			x86_64 = { "-m64" },
		},
		kind = {
			SharedLib = "-shared",
			StaticLib = "-lib",
		},
	}

	function ldc.getldflags(cfg)
		local flags = config.mapFlags(cfg, ldc.ldflags)
		return flags
	end


--
-- Return a list of decorated additional libraries directories.
--

	ldc.libraryDirectories = {
		architecture = {
			x86 = "-L=-L/usr/lib",
			x86_64 = "-L=-L/usr/lib64",
		}
	}

	function ldc.getLibraryDirectories(cfg)
		local flags = config.mapFlags(cfg, ldc.libraryDirectories)

		-- Scan the list of linked libraries. If any are referenced with
		-- paths, add those to the list of library search paths
		for _, dir in ipairs(config.getlinks(cfg, "system", "directory")) do
			table.insert(flags, '-L=-L' .. project.getrelative(cfg.project, dir))
		end

		return flags
	end


--
-- Return the list of libraries to link, decorated with flags as needed.
--

	function ldc.getlinks(cfg, systemonly)
		local result = {}

		local links
		if not systemonly then
			links = config.getlinks(cfg, "siblings", "object")
			for _, link in ipairs(links) do
				-- skip external project references, since I have no way
				-- to know the actual output target path
				if not link.project.external then
					if link.kind == p.STATICLIB then
						-- Don't use "-l" flag when linking static libraries; instead use
						-- path/libname.a to avoid linking a shared library of the same
						-- name if one is present
						table.insert(result, "-L=" .. project.getrelative(cfg.project, link.linktarget.abspath))
					else
						table.insert(result, "-L=-l" .. link.linktarget.basename)
					end
				end
			end
		end

		-- The "-l" flag is fine for system libraries
		links = config.getlinks(cfg, "system", "fullpath")
		for _, link in ipairs(links) do
			if path.isframework(link) then
				table.insert(result, "-framework " .. path.getbasename(link))
			elseif path.isobjectfile(link) then
				table.insert(result, "-L=" .. link)
			else
				table.insert(result, "-L=-l" .. path.getbasename(link))
			end
		end

		return result
	end


--
-- Returns makefile-specific configuration rules.
--

	ldc.makesettings = {
	}

	function ldc.getmakesettings(cfg)
		local settings = config.mapFlags(cfg, ldc.makesettings)
		return table.concat(settings)
	end


--
-- Retrieves the executable command name for a tool, based on the
-- provided configuration and the operating environment.
--
-- @param cfg
--    The configuration to query.
-- @param tool
--    The tool to fetch, one of "dc" for the D compiler, or "ar" for the static linker.
-- @return
--    The executable command name for a tool, or nil if the system's
--    default value should be used.
--

	ldc.tools = {
		dc = "ldc2",
		ar = "ar",
	}

	function ldc.gettoolname(cfg, tool)
		return ldc.tools[tool]
	end
