--
-- tests/actions/vstudio/vc2010/test_character_set.lua
-- Validate generation Unicode/MBCS settings.
-- Copyright (c) 2011-2015 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("vstudio_vs2010_character_set")
	local vc2010 = p.vstudio.vc2010


	local wks, prj

	function suite.setup()
		p.action.set("vs2010")
		wks, prj = test.createWorkspace()
	end

	local function prepare()
		local cfg = test.getconfig(prj, "Debug")
		vc2010.characterSet(cfg)
	end


	function suite.onDefault()
		prepare()
		test.capture [[
<CharacterSet>Unicode</CharacterSet>
		]]
	end


	function suite.onUnicode()
		characterset "Unicode"
		prepare()
		test.capture [[
<CharacterSet>Unicode</CharacterSet>
		]]
	end


	function suite.onMBCS()
		characterset "MBCS"
		prepare()
		test.capture [[
<CharacterSet>MultiByte</CharacterSet>
		]]
	end

	function suite.onASCII()
		characterset "ASCII"
		prepare()
		test.capture [[
<CharacterSet>NotSet</CharacterSet>
		]]
	end

