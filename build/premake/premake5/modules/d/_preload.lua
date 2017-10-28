--
-- Name:        d/_preload.lua
-- Purpose:     Define the D language API's.
-- Author:      Manu Evans
-- Created:     2013/10/28
-- Copyright:   (c) 2013-2015 Manu Evans and the Premake project
--

-- TODO:
-- MonoDevelop/Xamarin Studio has 'workspaces', which correspond to collections
-- of Premake workspaces. If premake supports multiple workspaces, we should
-- write out a workspace file...

	local p = premake
	local api = p.api

--
-- Register the D extension
--

	p.D = "D"
	api.addAllowed("language", p.D)

	api.addAllowed("floatingpoint", "None")
	api.addAllowed("flags", {
		"CodeCoverage",
		"Deprecated",
		"Documentation",
		"GenerateHeader",
		"GenerateJSON",
		"GenerateMap",
		"NoBoundsCheck",
--		"PIC",		// Note: this should be supported elsewhere...
		"Profile",
		"Quiet",
--		"Release",	// Note: We infer this flag from config.isDebugBuild()
		"RetainPaths",
		"SeparateCompilation",
		"SymbolsLikeC",
		"UnitTest",
		"Verbose",
	})


--
-- Register some D specific properties
--

	api.register {
		name = "versionconstants",
		scope = "config",
		kind = "list:string",
		tokens = true,
	}

	api.register {
		name = "versionlevel",
		scope = "config",
		kind = "integer",
	}

	api.register {
		name = "debugconstants",
		scope = "config",
		kind = "list:string",
		tokens = true,
	}

	api.register {
		name = "debuglevel",
		scope = "config",
		kind = "integer",
	}

	api.register {
		name = "docdir",
		scope = "config",
		kind = "path",
		tokens = true,
	}

	api.register {
		name = "docname",
		scope = "config",
		kind = "string",
		tokens = true,
	}

	api.register {
		name = "headerdir",
		scope = "config",
		kind = "path",
		tokens = true,
	}

	api.register {
		name = "headername",
		scope = "config",
		kind = "string",
		tokens = true,
	}


--
-- Provide information for the help output
--
	newoption
	{
		trigger		= "dc",
		value		= "VALUE",
		description	= "Choose a D compiler",
		allowed = {
			{ "dmd", "Digital Mars (dmd)" },
			{ "gdc", "GNU GDC (gdc)" },
			{ "ldc", "LLVM LDC (ldc2)" },
		}
	}


--
-- Decide when to load the full module
--

	return function (cfg)
		return (cfg.language == p.D)
	end
