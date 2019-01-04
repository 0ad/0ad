--
-- tests/test_msc.lua
-- Automated test suite for the Microsoft C toolset interface.
-- Copyright (c) 2012-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("tools_msc")

	local msc = p.tools.msc


--
-- Setup/teardown
--

	local wks, prj, cfg

	function suite.setup()
		wks, prj = test.createWorkspace()
		kind "SharedLib"
	end

	local function prepare()
		cfg = test.getconfig(prj, "Debug")
	end


--
-- Check the optimization flags.
--

	function suite.cflags_onNoOptimize()
		optimize "Off"
		prepare()
		test.contains("/Od", msc.getcflags(cfg))
	end

	function suite.cflags_onOptimize()
		optimize "On"
		prepare()
		test.contains("/Ot", msc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeSize()
		optimize "Size"
		prepare()
		test.contains("/O1", msc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeSpeed()
		optimize "Speed"
		prepare()
		test.contains("/O2", msc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeFull()
		optimize "Full"
		prepare()
		test.contains("/Ox", msc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeDebug()
		optimize "Debug"
		prepare()
		test.contains("/Od", msc.getcflags(cfg))
	end

	function suite.cflags_onNoFramePointers()
		flags "NoFramePointer"
		prepare()
		test.contains("/Oy", msc.getcflags(cfg))
	end

	function suite.cflags_onOmitFramePointer()
		omitframepointer "On"
		prepare()
		test.contains("/Oy", msc.getcflags(cfg))
	end

	function suite.cflags_onNoOmitFramePointers()
		omitframepointer "Off"
		prepare()
		test.excludes("/Oy", msc.getcflags(cfg))
	end

	function suite.ldflags_onLinkTimeOptimizations()
		flags "LinkTimeOptimization"
		prepare()
		test.contains("/GL", msc.getldflags(cfg))
	end

	function suite.cflags_onStringPoolingOn()
		stringpooling "On"
		prepare()
		test.contains("/GF", msc.getcflags(cfg))
	end

	function suite.cflags_onStringPoolingOff()
		stringpooling "Off"
		prepare()
		test.contains("/GF-", msc.getcflags(cfg))
	end

	function suite.cflags_onStringPoolingNotSpecified()
		prepare()
		test.excludes("/GF", msc.getcflags(cfg))
		test.excludes("/GF-", msc.getcflags(cfg))
	end

	function suite.cflags_onFloatingPointExceptionsOn()
		floatingpointexceptions "On"
		prepare()
		test.contains("/fp:except", msc.getcflags(cfg))
	end

	function suite.cflags_onFloatingPointExceptionsOff()
		floatingpointexceptions "Off"
		prepare()
		test.contains("/fp:except-", msc.getcflags(cfg))
	end

	function suite.cflags_onFloatingPointExceptionsNotSpecified()
		prepare()
		test.excludes("/fp:except", msc.getcflags(cfg))
		test.excludes("/fp:except-", msc.getcflags(cfg))
	end

	function suite.cflags_onFunctionLevelLinkingOn()
		functionlevellinking "On"
		prepare()
		test.contains("/Gy", msc.getcflags(cfg))
	end

	function suite.cflags_onFunctionLevelLinkingOff()
		functionlevellinking "Off"
		prepare()
		test.contains("/Gy-", msc.getcflags(cfg))
	end

	function suite.cflags_onFunctionLevelLinkingNotSpecified()
		prepare()
		test.excludes("/Gy", msc.getcflags(cfg))
		test.excludes("/Gy-", msc.getcflags(cfg))
	end

	function suite.cflags_onIntrinsicsOn()
		intrinsics "On"
		prepare()
		test.contains("/Oi", msc.getcflags(cfg))
	end

--
-- Check the translation of symbols.
--

	function suite.cflags_onDefaultSymbols()
		prepare()
		test.excludes({ "/Z7" }, msc.getcflags(cfg))
	end

	function suite.cflags_onNoSymbols()
		symbols "Off"
		prepare()
		test.excludes({ "/Z7" }, msc.getcflags(cfg))
	end

	function suite.cflags_onSymbols()
		symbols "On"
		prepare()
		test.contains({ "/Z7" }, msc.getcflags(cfg))
	end


--
-- Check the translation of unsignedchar.
--

	function suite.sharedflags_onUnsignedCharOn()
		unsignedchar "On"
		prepare()
		test.contains({ "/J" }, msc.getcflags(cfg))
		test.contains({ "/J" }, msc.getcxxflags(cfg))
	end

	function suite.sharedflags_onUnsignedCharOff()
		unsignedchar "Off"
		prepare()
		test.excludes({ "/J" }, msc.getcflags(cfg))
		test.excludes({ "/J" }, msc.getcxxflags(cfg))
	end


--
-- Check handling of debugging support.
--

	function suite.ldflags_onSymbols()
		symbols "On"
		prepare()
		test.contains("/DEBUG", msc.getldflags(cfg))
	end


--
-- Check handling warnings and errors.
--

	function suite.cflags_OnNoWarnings()
		warnings "Off"
		prepare()
		test.contains("/W0", msc.getcflags(cfg))
	end

	function suite.cflags_OnHighWarnings()
		warnings "High"
		prepare()
		test.contains("/W4", msc.getcflags(cfg))
	end

	function suite.cflags_OnExtraWarnings()
		warnings "Extra"
		prepare()
		test.contains("/W4", msc.getcflags(cfg))
	end

	function suite.cflags_OnFatalWarnings()
		flags "FatalWarnings"
		prepare()
		test.contains("/WX", msc.getcflags(cfg))
	end

	function suite.cflags_onSpecificWarnings()
		enablewarnings { "enable" }
		disablewarnings { "disable" }
		fatalwarnings { "fatal" }
		prepare()
		test.contains({ '/wd"disable"', '/we"fatal"' }, msc.getcflags(cfg))
	end

	function suite.ldflags_OnFatalWarnings()
		flags "FatalWarnings"
		prepare()
		test.contains("/WX", msc.getldflags(cfg))
	end


--
-- Check handling of library search paths.
--

	function suite.libdirs_onLibdirs()
		libdirs { "../libs" }
		prepare()
		test.contains('/LIBPATH:"../libs"', msc.getLibraryDirectories(cfg))
	end


--
-- Check handling of forced includes.
--

	function suite.forcedIncludeFiles()
		forceincludes { "include/sys.h" }
		prepare()
		test.contains('/FIinclude/sys.h', msc.getforceincludes(cfg))
	end


--
-- Check handling of floating point modifiers.
--

	function suite.cflags_onFloatingPointFast()
		floatingpoint "Fast"
		prepare()
		test.contains("/fp:fast", msc.getcflags(cfg))
	end

	function suite.cflags_onFloatingPointStrict()
		floatingpoint "Strict"
		prepare()
		test.contains("/fp:strict", msc.getcflags(cfg))
	end

	function suite.cflags_onSSE()
		vectorextensions "SSE"
		prepare()
		test.contains("/arch:SSE", msc.getcflags(cfg))
	end

	function suite.cflags_onSSE2()
		vectorextensions "SSE2"
		prepare()
		test.contains("/arch:SSE2", msc.getcflags(cfg))
	end

	function suite.cflags_onAVX()
		vectorextensions "AVX"
		prepare()
		test.contains("/arch:AVX", msc.getcflags(cfg))
	end

	function suite.cflags_onAVX2()
		vectorextensions "AVX2"
		prepare()
		test.contains("/arch:AVX2", msc.getcflags(cfg))
	end


--
-- Check the defines and undefines.
--

	function suite.defines()
		defines "DEF"
		prepare()
		test.contains({ '/D"DEF"' }, msc.getdefines(cfg.defines, cfg))
	end

	function suite.undefines()
		undefines "UNDEF"
		prepare()
		test.contains({ '/U"UNDEF"' }, msc.getundefines(cfg.undefines))
	end


--
-- Check compilation options.
--

	function suite.cflags_onNoMinimalRebuild()
		flags "NoMinimalRebuild"
		prepare()
		test.contains("/Gm-", msc.getcflags(cfg))
	end

	function suite.cflags_onMultiProcessorCompile()
		flags "MultiProcessorCompile"
		prepare()
		test.contains("/MP", msc.getcflags(cfg))
	end


--
-- Check handling of C++ language features.
--

	function suite.cxxflags_onExceptions()
		exceptionhandling "on"
		prepare()
		test.contains("/EHsc", msc.getcxxflags(cfg))
	end

	function suite.cxxflags_onSEH()
		exceptionhandling "SEH"
		prepare()
		test.contains("/EHa", msc.getcxxflags(cfg))
	end

	function suite.cxxflags_onNoExceptions()
		exceptionhandling "Off"
		prepare()
		test.missing("/EHsc", msc.getcxxflags(cfg))
	end

	function suite.cxxflags_onNoRTTI()
		rtti "Off"
		prepare()
		test.contains("/GR-", msc.getcxxflags(cfg))
	end


--
-- Check handling of additional linker options.
--

	function suite.ldflags_onOmitDefaultLibrary()
		flags "OmitDefaultLibrary"
		prepare()
		test.contains("/NODEFAULTLIB", msc.getldflags(cfg))
	end

	function suite.ldflags_onNoIncrementalLink()
		flags "NoIncrementalLink"
		prepare()
		test.contains("/INCREMENTAL:NO", msc.getldflags(cfg))
	end

	function suite.ldflags_onNoManifest()
		flags "NoManifest"
		prepare()
		test.contains("/MANIFEST:NO", msc.getldflags(cfg))
	end

	function suite.ldflags_onDLL()
		kind "SharedLib"
		prepare()
		test.contains("/DLL", msc.getldflags(cfg))
	end



--
-- Check handling of CLR settings.
--

	function suite.cflags_onClrOn()
		clr "On"
		prepare()
		test.contains("/clr", msc.getcflags(cfg))
	end

	function suite.cflags_onClrUnsafe()
		clr "Unsafe"
		prepare()
		test.contains("/clr", msc.getcflags(cfg))
	end

	function suite.cflags_onClrSafe()
		clr "Safe"
		prepare()
		test.contains("/clr:safe", msc.getcflags(cfg))
	end

	function suite.cflags_onClrPure()
		clr "Pure"
		prepare()
		test.contains("/clr:pure", msc.getcflags(cfg))
	end


--
-- Check handling of character set switches
--

	function suite.cflags_onCharSetDefault()
		prepare()
		test.contains('/D"_UNICODE"', msc.getdefines(cfg.defines, cfg))
	end

	function suite.cflags_onCharSetUnicode()
		characterset "Unicode"
		prepare()
		test.contains('/D"_UNICODE"', msc.getdefines(cfg.defines, cfg))
	end

	function suite.cflags_onCharSetMBCS()
		characterset "MBCS"
		prepare()
		test.contains('/D"_MBCS"', msc.getdefines(cfg.defines, cfg))
	end

	function suite.cflags_onCharSetASCII()
		characterset "ASCII"
		prepare()
		test.excludes({'/D"_MBCS"', '/D"_UNICODE"'}, msc.getdefines(cfg.defines, cfg))
	end


--
-- Check handling of system search paths.
--

	function suite.includeDirs_onSysIncludeDirs()
		sysincludedirs { "/usr/local/include" }
		prepare()
		test.contains("-I/usr/local/include", msc.getincludedirs(cfg, cfg.includedirs, cfg.sysincludedirs))
	end

	function suite.libDirs_onSysLibDirs()
		syslibdirs { "/usr/local/lib" }
		prepare()
		test.contains('/LIBPATH:"/usr/local/lib"', msc.getLibraryDirectories(cfg))
	end

--
-- Check handling of ignore default libraries
--

	function suite.ignoreDefaultLibraries_WithExtensions()
		ignoredefaultlibraries { "lib1.lib" }
		prepare()
		test.contains('/NODEFAULTLIB:lib1.lib', msc.getldflags(cfg))
	end

	function suite.ignoreDefaultLibraries_WithoutExtensions()
		ignoredefaultlibraries { "lib1" }
		prepare()
		test.contains('/NODEFAULTLIB:lib1.lib', msc.getldflags(cfg))
	end


--
-- Check handling of shared C/C++ flags.
--

	function suite.mixedToolFlags_onCFlags()
		flags { "FatalCompileWarnings" }
		prepare()
		test.isequal({ "/WX", "/MD" }, msc.getcflags(cfg))
	end

	function suite.mixedToolFlags_onCxxFlags()
		flags { "FatalCompileWarnings" }
		prepare()
		test.isequal({ "/WX", "/MD", "/EHsc" }, msc.getcxxflags(cfg))
	end
