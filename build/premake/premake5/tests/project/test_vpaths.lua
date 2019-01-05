--
-- tests/project/test_vpaths.lua
-- Automated test suite for the project support functions.
-- Copyright (c) 2011-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("project_vpaths")
	local project = p.project


--
-- Setup and teardown
--

	local wks, prj

	function suite.setup()
		wks, prj = test.createWorkspace()
	end

	local function run()
		local cfg = test.getconfig(prj, "Debug")
		return project.getvpath(prj, cfg.files[1])
	end


--
-- Test simple replacements
--

	function suite.ReturnsOriginalPath_OnNoVpaths()
		files { "hello.c" }
		test.isequal(path.join(os.getcwd(), "hello.c"), run())
	end

	function suite.ReturnsOriginalPath_OnNoMatches()
		files { "hello.c" }
		vpaths { ["Headers"] = "**.h" }
		test.isequal(path.join(os.getcwd(), "hello.c"), run())
	end

	function suite.CanStripPaths()
		files { "src/myproject/hello.c" }
		vpaths { [""] = "src" }
		run()
		test.isequal("hello.c", run())
	end

	function suite.CanTrimLeadingPaths()
		files { "src/myproject/hello.c" }
		vpaths { ["*"] = "src" }
		test.isequal("myproject/hello.c", run())
	end

	function suite.PatternMayIncludeTrailingSlash()
		files { "src/myproject/hello.c" }
		vpaths { [""] = "src/myproject/" }
		test.isequal("hello.c", run())
	end

	function suite.SimpleReplacementPatterns()
		files { "src/myproject/hello.c" }
		vpaths { ["sources"] = "src/myproject" }
		test.isequal("sources/hello.c", run())
	end

	function suite.ExactFilenameMatch()
		files { "src/hello.c" }
		vpaths { ["sources"] = "src/hello.c" }
		test.isequal("sources/hello.c", run())
	end


--
-- Test wildcard patterns
--

	function suite.MatchFilePattern_ToGroup_Flat()
		files { "src/myproject/hello.h" }
		vpaths { ["Headers"] = "**.h" }
		test.isequal("Headers/hello.h", run())
	end

	function suite.MatchFilePattern_ToNone_Flat()
		files { "src/myproject/hello.h" }
		vpaths { [""] = "**.h" }
		test.isequal("hello.h", run())
	end

	function suite.MatchFilePattern_ToNestedGroup_Flat()
		files { "src/myproject/hello.h" }
		vpaths { ["Source/Headers"] = "**.h" }
		test.isequal("Source/Headers/hello.h", run())
	end

	function suite.MatchFilePattern_ToGroup_WithTrailingSlash()
		files { "src/myproject/hello.h" }
		vpaths { ["Headers/"] = "**.h" }
		test.isequal("Headers/hello.h", run())
	end

	function suite.MatchFilePattern_ToGroup_Nested()
		files { "src/myproject/hello.h" }
		vpaths { ["Headers/*"] = "**.h" }
		test.isequal("Headers/src/myproject/hello.h", run())
	end

	function suite.MatchFilePattern_ToGroup_Nested_OneStar()
		files { "src/myproject/hello.h" }
		vpaths { ["Headers/*"] = "**.h" }
		test.isequal("Headers/src/myproject/hello.h", run())
	end

	function suite.MatchFilePatternWithPath_ToGroup_Nested()
		files { "src/myproject/hello.h" }
		vpaths { ["Headers/*"] = "src/**.h" }
		test.isequal("Headers/myproject/hello.h", run())
	end

	function suite.matchBaseFileName_onWildcardExtension()
		files { "hello.cpp" }
		vpaths { ["Sources"] = "hello.*" }
		test.isequal("Sources/hello.cpp", run())
	end


--
-- Test with project locations
--

	function suite.MatchPath_OnProjectLocationSet()
		location "build"
		files "src/hello.h"
		vpaths { [""] = "src" }
		test.isequal("hello.h", run())
	end

	function suite.MatchFilePattern_OnProjectLocationSet()
		location "build"
		files "src/hello.h"
		vpaths { ["Headers"] = "**.h" }
		test.isequal("Headers/hello.h", run())
	end

	function suite.MatchFilePatternWithPath_OnProjectLocationSet()
		location "build"
		files "src/hello.h"
		vpaths { ["Headers"] = "src/**.h" }
		test.isequal("Headers/hello.h", run())
	end
