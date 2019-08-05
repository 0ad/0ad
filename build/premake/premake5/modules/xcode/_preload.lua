---
-- xcode/_preload.lua
-- Define the Apple XCode actions and new APIs.
-- Copyright (c) 2009-2015 Jason Perkins and the Premake project
---

	local p = premake


--
-- Register new Xcode-specific project fields.
--

	p.api.register {
		name = "xcodebuildsettings",
		scope = "config",
		kind = "key-array",
	}

	p.api.register {
		name = "xcodebuildresources",
		scope = "config",
		kind = "list",
	}

	p.api.register {
		name = "xcodecodesigningidentity",
		scope = "config",
		kind = "string",
	}

	p.api.register {
		name = "xcodesystemcapabilities",
		scope = "project",
		kind = "key-boolean",
	}

	p.api.register {
		name = "iosfamily",
		scope = "config",
		kind = "string",
		allowed = {
			"iPhone/iPod touch",
			"iPad",
			"Universal",
		}
	}

--
-- Register the Xcode exporters.
--

	newaction {
		trigger     = "xcode4",
		shortname   = "Apple Xcode 4",
		description = "Generate Apple Xcode 4 project files",

		-- Xcode always uses Mac OS X path and naming conventions

		toolset  = "clang",

		-- The capabilities of this action

		valid_kinds     = { "ConsoleApp", "WindowedApp", "SharedLib", "StaticLib", "Makefile", "Utility", "None" },
		valid_languages = { "C", "C++" },
		valid_tools     = {
			cc = { "gcc", "clang" },
		},


		-- Workspace and project generation logic

		onWorkspace = function(wks)
			p.generate(wks, ".xcworkspace/contents.xcworkspacedata", p.modules.xcode.generateWorkspace)
		end,

		onProject = function(prj)
			p.generate(prj, ".xcodeproj/project.pbxproj", p.modules.xcode.generateProject)
		end,
	}


--
-- Decide when the full module should be loaded.
--

	return function(cfg)
		return (_ACTION == "xcode4")
	end
