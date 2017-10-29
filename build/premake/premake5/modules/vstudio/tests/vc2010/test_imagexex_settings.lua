--
-- tests/actions/vstudio/vc2010/test_compile_settings.lua
-- Validate Xbox 360 XEX image settings in Visual Studio 2010 C/C++ projects.
-- Copyright (c) 2011-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("vstudio_vs2010_imagexex_settings")
	local vc2010 = p.vstudio.vc2010
	local project = p.project


--
-- Setup
--

	local wks, prj

	function suite.setup()
		p.action.set("vs2010")
		wks, prj = test.createWorkspace()
		platforms "xbox360"
	end

	local function prepare(platform)
		local cfg = test.getconfig(prj, "Debug", "xbox360")
		vc2010.imageXex(cfg)
	end

--
-- Test default ImageXex settings
--
	function suite.defaultSettings()
		prepare()
		test.capture [[
<ImageXex>
	<ConfigurationFile>
	</ConfigurationFile>
	<AdditionalSections>
	</AdditionalSections>
</ImageXex>
		]]
	end

--
-- Ensure configuration file is output in ImageXex block
--
	function suite.defaultSettings()
		configfile "testconfig.xml"
		prepare()
		test.capture [[
<ImageXex>
	<ConfigurationFile>testconfig.xml</ConfigurationFile>
	<AdditionalSections>
	</AdditionalSections>
</ImageXex>
		]]
	end
