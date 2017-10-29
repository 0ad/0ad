--
-- _preload.lua
-- Define the makefile action(s).
-- Copyright (c) 2002-2015 Jason Perkins and the Premake project
--

	local p = premake
	local project = p.project

---
-- The GNU make action, with support for the new platforms API
---

	newaction {
		trigger         = "gmake",
		shortname       = "GNU Make",
		description     = "Generate GNU makefiles for POSIX, MinGW, and Cygwin",
		toolset         = "gcc",

		valid_kinds     = { "ConsoleApp", "WindowedApp", "StaticLib", "SharedLib", "Utility", "Makefile" },
		valid_languages = { "C", "C++", "C#" },
		valid_tools     = {
			cc     = { "clang", "gcc" },
			dotnet = { "mono", "msnet", "pnet" }
		},

		onWorkspace = function(wks)
			p.escaper(p.make.esc)
			p.generate(wks, p.make.getmakefilename(wks, false), p.make.generate_workspace)
		end,

		onProject = function(prj)
			p.escaper(p.make.esc)
			local makefile = p.make.getmakefilename(prj, true)
			if prj.kind == p.UTILITY then
				p.generate(prj, makefile, p.make.utility.generate)
			elseif prj.kind == p.MAKEFILE then
				p.generate(prj, makefile, p.make.makefile.generate)
			else
				if project.isdotnet(prj) then
					p.generate(prj, makefile, p.make.cs.generate)
				elseif project.isc(prj) or project.iscpp(prj) then
					p.generate(prj, makefile, p.make.cpp.generate)
				end
			end
		end,

		onCleanWorkspace = function(wks)
			p.clean.file(wks, p.make.getmakefilename(wks, false))
		end,

		onCleanProject = function(prj)
			p.clean.file(prj, p.make.getmakefilename(prj, true))
		end
	}


--
-- Decide when the full module should be loaded.
--

	return function(cfg)
		return (_ACTION == "gmake")
	end
