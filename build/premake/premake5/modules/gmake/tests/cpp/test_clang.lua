--
-- tests/actions/make/cpp/test_clang.lua
-- Test Clang support in Makefiles.
-- Copyright (c) 2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("make_clang")
	local make = p.make
	local cpp = p.make.cpp
	local project = p.project


--
-- Setup
--

	local wks, prj

	function suite.setup()
		wks = test.createWorkspace()
		toolset "clang"
		prj = p.workspace.getproject(wks, 1)
	end


--
-- Make sure that the correct compilers are used.
--

	function suite.usesCorrectCompilers()
		make.cppConfigs(prj)
		test.capture [[
ifeq ($(config),debug)
  ifeq ($(origin CC), default)
    CC = clang
  endif
  ifeq ($(origin CXX), default)
    CXX = clang++
  endif
  ifeq ($(origin AR), default)
    AR = ar
  endif
		]]
	end

