---
-- tests/actions/vstudio/vc2010/test_floatingpoint.lua
-- Validate handling of vectorextensions() in VS 2010 C/C++ projects.
--
-- Created 26 Mar 2015 by Jason Perkins
-- Copyright (c) 2015 Jason Perkins and the Premake project
---

	local p = premake
	local suite = test.declare("vs2010_vc_floatingpoint")
	local m = p.vstudio.vc2010


	local wks, prj

	function suite.setup()
		p.action.set("vs2010")
		wks, prj = test.createWorkspace()
	end

	local function prepare()
		local cfg = test.getconfig(prj, "Debug")
		m.floatingPointModel(cfg)
	end



	function suite.instructionSet_onNotSet()
		test.isemptycapture()
	end


	function suite.floatingPoint_onFloatFast()
		floatingpoint "fast"
		prepare()
		test.capture [[
<FloatingPointModel>Fast</FloatingPointModel>
		]]
	end

	function suite.floatingPoint_onFloatStrict()
		floatingpoint "strict"
		prepare()
		test.capture [[
<FloatingPointModel>Strict</FloatingPointModel>
		]]
	end

	function suite.floatingPoint_onDefault()
		floatingpoint "Default"
		prepare()
		test.isemptycapture()
	end
