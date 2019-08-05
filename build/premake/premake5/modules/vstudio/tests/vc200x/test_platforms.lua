--
-- tests/actions/vstudio/vc200x/test_platforms.lua
-- Test the Visual Studio 2002-2008 project's Platforms block
-- Copyright (c) 2009-2012 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("vstudio_vc200x_platforms")
	local vc200x = p.vstudio.vc200x


--
-- Setup
--

	local wks, prj

	function suite.setup()
		p.action.set("vs2008")
		wks = test.createWorkspace()
	end

	local function prepare()
		prj = test.getproject(wks, 1)
		vc200x.platforms(prj)
	end


--
-- If no architectures are specified, Win32 should be the default.
--

	function suite.win32Listed_onNoPlatforms()
		prepare()
		test.capture [[
<Platforms>
	<Platform
		Name="Win32"
	/>
</Platforms>
		]]
	end


--
-- If multiple configurations use the same architecture, it should
-- still only be listed once.
--

	function suite.architectureListedOnlyOnce_onMultipleConfigurations()
		platforms { "Static", "Dynamic" }
		prepare()
		test.capture [[
<Platforms>
	<Platform
		Name="Win32"
	/>
</Platforms>
		]]
	end


--
-- If multiple architectures are used, they should all be listed.
--

	function suite.allArchitecturesListed_onMultipleArchitectures()
		platforms { "x86", "x86_64" }
		prepare()
		test.capture [[
<Platforms>
	<Platform
		Name="Win32"
	/>
	<Platform
		Name="x64"
	/>
</Platforms>
		]]
	end
