--
-- tests/actions/make/cpp/test_make_pch.lua
-- Validate the setup for precompiled headers in makefiles.
-- Copyright (c) 2010-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("make_pch")
	local make = p.make
	local project = p.project



--
-- Setup and teardown
--

	local wks, prj
	function suite.setup()
		os.chdir(_TESTS_DIR)
		wks, prj = test.createWorkspace()
	end

	local function prepareVars()
		local cfg = test.getconfig(prj, "Debug")
		make.pch(cfg)
	end

	local function prepareRules()
		local cfg = test.getconfig(prj, "Debug")
		make.pchRules(cfg.project)
	end


--
-- If no header has been set, nothing should be output.
--

	function suite.noConfig_onNoHeaderSet()
		prepareVars()
		test.isemptycapture()
	end


--
-- If a header is set, but the NoPCH flag is also set, then
-- nothing should be output.
--

	function suite.noConfig_onHeaderAndNoPCHFlag()
		pchheader "include/myproject.h"
		flags "NoPCH"
		prepareVars()
		test.isemptycapture()
	end


--
-- If a header is specified and the NoPCH flag is not set, then
-- the header can be used.
--

	function suite.config_onPchEnabled()
		pchheader "include/myproject.h"
		prepareVars()
		test.capture [[
  PCH = include/myproject.h
  GCH = $(OBJDIR)/$(notdir $(PCH)).gch
		]]
	end


--
-- The PCH can be specified relative the an includes search path.
--

	function suite.pch_searchesIncludeDirs()
		pchheader "premake.h"
		includedirs { "../../../src/host" }
		prepareVars()
		test.capture [[
  PCH = ../../../src/host/premake.h
		]]
	end


--
-- Verify the format of the PCH rules block for a C++ file.
--

	function suite.buildRules_onCpp()
		pchheader "include/myproject.h"
		prepareRules()
		test.capture [[
ifneq (,$(PCH))
$(OBJECTS): $(GCH) $(PCH) | $(OBJDIR)
$(GCH): $(PCH) | $(OBJDIR)
	@echo $(notdir $<)
	$(SILENT) $(CXX) -x c++-header $(ALL_CXXFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
else
$(OBJECTS): | $(OBJDIR)
endif
		]]
	end


--
-- Verify the format of the PCH rules block for a C file.
--

	function suite.buildRules_onC()
		language "C"
		pchheader "include/myproject.h"
		prepareRules()
		test.capture [[
ifneq (,$(PCH))
$(OBJECTS): $(GCH) $(PCH) | $(OBJDIR)
$(GCH): $(PCH) | $(OBJDIR)
	@echo $(notdir $<)
	$(SILENT) $(CC) -x c-header $(ALL_CFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
else
$(OBJECTS): | $(OBJDIR)
endif
		]]
	end



	--
	-- If the header is located on one of the include file
	-- search directories, it should get found automatically.
	--

		function suite.findsPCH_onIncludeDirs()
			location "MyProject"
			pchheader "premake.h"
			includedirs { "../../../src/host" }
			prepareVars()
			test.capture [[
  PCH = ../../../../src/host/premake.h
			]]
		end
