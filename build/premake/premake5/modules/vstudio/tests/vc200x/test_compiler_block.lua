--
-- tests/actions/vstudio/vc200x/test_compiler_block.lua
-- Validate generation the VCCLCompiler element in Visual Studio 200x C/C++ projects.
-- Copyright (c) 2011-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("vs200x_compiler_block")
	local vc200x = p.vstudio.vc200x


--
-- Setup/teardown
--

	local wks, prj

	function suite.setup()
		p.action.set("vs2008")
		wks, prj = test.createWorkspace()
	end

	local function prepare()
		local cfg = test.getconfig(prj, "Debug")
		vc200x.VCCLCompilerTool(cfg)
	end


--
-- Verify the basic structure of the compiler block with no flags or settings.
--

	function suite.looksGood_onDefaultSettings()
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- If include directories are specified, the <AdditionalIncludeDirectories> should be added.
--


	function suite.additionalIncludeDirs_onIncludeDirs()
		includedirs { "include/lua", "include/zlib" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	AdditionalIncludeDirectories="include\lua;include\zlib"
		]]
	end


--
-- Ensure macros are not truncated (see issue #63)
--


	function suite.additionalIncludeDirs_onIncludeDirs_with_vs_macros()
		includedirs { "$(Macro1)/foo/bar/$(Macro2)/baz" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	AdditionalIncludeDirectories="$(Macro1)\foo\bar\$(Macro2)\baz"
		]]
	end


--
-- Verify the handling of the Symbols flag. The format must be set, and the
-- debug runtime library must be selected.
--

	function suite.looksGood_onSymbolsFlag()
		symbols "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	MinimalRebuild="true"
	BasicRuntimeChecks="3"
	RuntimeLibrary="3"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="4"
/>
		]]
	end


--
-- Verify the handling of the Symbols in conjunction with the Optimize flag.
-- The release runtime library must be used.
--

	function suite.looksGood_onSymbolsAndOptimizeFlags()
		symbols "On"
		optimize "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="3"
	StringPooling="true"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="3"
/>
		]]
	end


--
-- Verify the handling of the C7 debug information format.
--

	function suite.looksGood_onC7DebugFormat()
		symbols "On"
		debugformat "C7"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="3"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="1"
/>
		]]
	end


---
-- Test precompiled header handling; the header should be treated as
-- a plain string value, with no path manipulation applied, since it
-- needs to match the value of the #include statement used in the
-- project code.
---

	function suite.compilerBlock_OnPCH()
		location "build"
		pchheader "include/common.h"
		pchsource "source/common.cpp"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="2"
	PrecompiledHeaderThrough="include/common.h"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Floating point flag tests
--

	function suite.compilerBlock_OnFpFast()
		floatingpoint "Fast"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	FloatingPointModel="2"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end

	function suite.compilerBlock_OnFpStrict()
		floatingpoint "Strict"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	FloatingPointModel="1"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Check that the "minimal rebuild" flag is applied correctly.
--

	function suite.minimalRebuildFlagsSet_onMinimalRebuildAndSymbols()
		flags { "NoMinimalRebuild" }
		symbols "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="3"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="4"
/>
		]]
	end

--
-- Check that the "no buffer security check" flag is applied correctly.
--

	function suite.noBufferSecurityFlagSet_onBufferSecurityCheck()
		flags { "NoBufferSecurityCheck" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	BufferSecurityCheck="false"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end

--
-- Check that the CompileAs value is set correctly for C language projects.
--

	function suite.compileAsSet_onC()
		language "C"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
	CompileAs="1"
/>
		]]
	end


--
-- Verify the correct runtime library is used when symbols are enabled.
--

	function suite.runtimeLibraryIsDebug_onSymbolsNoOptimize()
		symbols "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	MinimalRebuild="true"
	BasicRuntimeChecks="3"
	RuntimeLibrary="3"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="4"
/>
		]]
	end


--
-- Verify the correct warnings settings are used when extra warnings are enabled.
--

	function suite.runtimeLibraryIsDebug_onExtraWarnings()
		warnings "Extra"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="4"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Verify the correct warnings settings are used when FatalWarnings are enabled.
--

	function suite.runtimeLibraryIsDebug_onFatalWarnings()
		flags { "FatalWarnings" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	WarnAsError="true"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Verify the correct warnings settings are used when no warnings are enabled.
--

	function suite.runtimeLibraryIsDebug_onNoWarnings_whichDisablesAllOtherWarningsFlags()
		flags { "FatalWarnings" }
		warnings "Off"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="0"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Verify the correct Detect64BitPortabilityProblems settings are used when _ACTION < "VS2008".
--

	function suite._64BitPortabilityOn_onVS2005()
		p.action.set("vs2005")
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	Detect64BitPortabilityProblems="true"
	DebugInformationFormat="0"
/>
		]]
	end

	function suite._64BitPortabilityOff_onVS2005_andCLR()
		p.action.set("vs2005")
		clr "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Verify the correct warnings settings are used when no warnings are enabled.
--

	function suite.runtimeLibraryIsDebug_onVS2005_NoWarnings()
		p.action.set("vs2005")
		warnings "Off"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="0"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Xbox 360 uses the same structure, but changes the element name.
--

	function suite.looksGood_onXbox360()
		system "Xbox360"
		prepare()
		test.capture [[
<Tool
	Name="VCCLX360CompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
/>
		]]
	end


--
-- Check handling of forced includes.
--

	function suite.forcedIncludeFiles()
		forceincludes { "stdafx.h", "include/sys.h" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
	ForcedIncludeFiles="stdafx.h;include\sys.h"
		]]
	end

	function suite.forcedUsingFiles()
		forceusings { "stdafx.h", "include/sys.h" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
	ForcedUsingFiles="stdafx.h;include\sys.h"
		]]
	end


--
-- Verify handling of the NoRuntimeChecks flag.
--

	function suite.onNoRuntimeChecks()
		flags { "NoRuntimeChecks" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	RuntimeLibrary="2"
		]]
	end



--
-- Check handling of the EnableMultiProcessorCompile flag.
--

	function suite.onMultiProcessorCompile()
		flags { "MultiProcessorCompile" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	AdditionalOptions="/MP"
	Optimization="0"
	BasicRuntimeChecks="3"
		]]
	end


--
-- Check handling of the ReleaseRuntime flag; should override the
-- default behavior of linking the debug runtime when symbols are
-- enabled with no optimizations.
--

	function suite.releaseRuntime_onFlag()
		flags { "ReleaseRuntime" }
		symbols "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	MinimalRebuild="true"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
		]]
	end

	function suite.releaseRuntime_onStaticAndReleaseRuntime()
		flags { "ReleaseRuntime", "StaticRuntime" }
		symbols "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	MinimalRebuild="true"
	BasicRuntimeChecks="3"
	RuntimeLibrary="0"
		]]
	end

--
-- Check the LinkTimeOptimization flag.
--

	function suite.flags_onLinkTimeOptimization()
		flags { "LinkTimeOptimization" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	WholeProgramOptimization="true"
		]]

	end


--
-- Check the optimization flags.
--

	function suite.optimization_onOptimize()
		optimize "On"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="3"
	StringPooling="true"
		]]
	end

	function suite.optimization_onOptimizeSize()
		optimize "Size"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="1"
	StringPooling="true"
		]]
	end

	function suite.optimization_onOptimizeSpeed()
		optimize "Speed"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="2"
	StringPooling="true"
		]]
	end

	function suite.optimization_onOptimizeFull()
		optimize "Full"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="3"
	StringPooling="true"
		]]
	end

	function suite.optimization_onOptimizeOff()
		optimize "Off"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
		]]
	end

	function suite.optimization_onOptimizeDebug()
		optimize "Debug"
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
		]]
	end


--
-- Check handling of the OmitDefaultLibrary flag.
--

	function suite.onOmitDefaultLibrary()
		flags { "OmitDefaultLibrary" }
		prepare()
		test.capture [[
<Tool
	Name="VCCLCompilerTool"
	Optimization="0"
	BasicRuntimeChecks="3"
	RuntimeLibrary="2"
	EnableFunctionLevelLinking="true"
	UsePrecompiledHeader="0"
	WarningLevel="3"
	DebugInformationFormat="0"
	OmitDefaultLibName="true"
		]]
	end
