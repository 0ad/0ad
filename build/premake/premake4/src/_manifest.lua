--
-- _manifest.lua
-- Manage the list of built-in Premake scripts.
-- Copyright (c) 2002-2010 Jason Perkins and the Premake project
--

-- The master list of built-in scripts. Order is important! If you want to
-- build a new script into Premake, add it to this list.

	return
	{
		-- core files
		"base/os.lua",
		"base/path.lua",
		"base/string.lua",
		"base/table.lua",
		"base/io.lua",
		"base/globals.lua",
		"base/action.lua",
		"base/option.lua",
		"base/tree.lua",
		"base/solution.lua",
		"base/project.lua",
		"base/configs.lua",
		"base/api.lua",
		"base/cmdline.lua",
		"tools/dotnet.lua",
		"tools/gcc.lua",
		"tools/msc.lua",
		"tools/ow.lua",
		"base/validate.lua",
		"base/help.lua",
		"base/premake.lua",
		
		-- CodeBlocks action
		"actions/codeblocks/_codeblocks.lua",
		"actions/codeblocks/codeblocks_workspace.lua",
		"actions/codeblocks/codeblocks_cbp.lua",
		
		-- CodeLite action
		"actions/codelite/_codelite.lua",
		"actions/codelite/codelite_workspace.lua",
		"actions/codelite/codelite_project.lua",
		
		-- GNU make action
		"actions/make/_make.lua",
		"actions/make/make_solution.lua",
		"actions/make/make_cpp.lua",
		"actions/make/make_csharp.lua",
		
		-- Visual Studio actions
		"actions/vstudio/_vstudio.lua",
		"actions/vstudio/vs2002_solution.lua",
		"actions/vstudio/vs2002_csproj.lua",
		"actions/vstudio/vs2002_csproj_user.lua",
		"actions/vstudio/vs200x_vcproj.lua",
		"actions/vstudio/vs2003_solution.lua",
		"actions/vstudio/vs2005_solution.lua",
		"actions/vstudio/vs2005_csproj.lua",
		"actions/vstudio/vs2005_csproj_user.lua",
		"actions/vstudio/vs_generic_solution.lua",
		"actions/vstudio/vs2010_vcxproxj.lua",
		
		-- Xcode action
		"actions/xcode/_xcode.lua",
		"actions/xcode/xcode_common.lua",
		"actions/xcode/xcode_project.lua",
		
		-- Xcode4 action
		"actions/xcode/xcode4_workspace.lua",
		
		-- Clean action
		"actions/clean/_clean.lua",
	}
