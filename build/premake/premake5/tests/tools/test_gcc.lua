--
-- tests/test_gcc.lua
-- Automated test suite for the GCC toolset interface.
-- Copyright (c) 2009-2013 Jason Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("tools_gcc")

	local gcc = p.tools.gcc
	local project = p.project


--
-- Setup/teardown
--

	local wks, prj, cfg

	function suite.setup()
		wks, prj = test.createWorkspace()
		system "Linux"
	end

	local function prepare()
		cfg = test.getconfig(prj, "Debug")
	end


--
-- Check the selection of tools based on the target system.
--

	function suite.tools_onDefaults()
		prepare()
		test.isnil(gcc.gettoolname(cfg, "cc"))
		test.isnil(gcc.gettoolname(cfg, "cxx"))
		test.isnil(gcc.gettoolname(cfg, "ar"))
		test.isequal("windres", gcc.gettoolname(cfg, "rc"))
	end

	function suite.tools_withPrefix()
		gccprefix "test-prefix-"
		prepare()
		test.isequal("test-prefix-gcc", gcc.gettoolname(cfg, "cc"))
		test.isequal("test-prefix-g++", gcc.gettoolname(cfg, "cxx"))
		test.isequal("test-prefix-ar", gcc.gettoolname(cfg, "ar"))
		test.isequal("test-prefix-windres", gcc.gettoolname(cfg, "rc"))
	end


--
-- By default, the -MMD -MP are used to generate dependencies.
--

	function suite.cppflags_defaultWithMMD()
		prepare()
		test.contains({"-MMD", "-MP"}, gcc.getcppflags(cfg))
	end


--
-- Haiku OS doesn't support the -MP flag yet (that's weird, isn't it?)
--

	function suite.cppflagsExcludeMP_onHaiku()
		system "Haiku"
		prepare()
		test.excludes({ "-MP" }, gcc.getcppflags(cfg))
	end


--
-- Check the translation of CFLAGS.
--

	function suite.cflags_onExtraWarnings()
		warnings "extra"
		prepare()
		test.contains({ "-Wall", "-Wextra" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onHighWarnings()
		warnings "high"
		prepare()
		test.contains({ "-Wall" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onFatalWarnings()
		flags { "FatalWarnings" }
		prepare()
		test.contains({ "-Werror" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onSpecificWarnings()
		enablewarnings { "enable" }
		disablewarnings { "disable" }
		fatalwarnings { "fatal" }
		prepare()
		test.contains({ "-Wenable", "-Wno-disable", "-Werror=fatal" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onFloastFast()
		floatingpoint "Fast"
		prepare()
		test.contains({ "-ffast-math" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onFloastStrict()
		floatingpoint "Strict"
		prepare()
		test.contains({ "-ffloat-store" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onNoWarnings()
		warnings "Off"
		prepare()
		test.contains({ "-w" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onSSE()
		vectorextensions "SSE"
		prepare()
		test.contains({ "-msse" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onSSE2()
		vectorextensions "SSE2"
		prepare()
		test.contains({ "-msse2" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onAVX()
		vectorextensions "AVX"
		prepare()
		test.contains({ "-mavx" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onAVX2()
		vectorextensions "AVX2"
		prepare()
		test.contains({ "-mavx2" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onMOVBE()
		isaextensions "MOVBE"
		prepare()
		test.contains({ "-mmovbe" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onPOPCNT()
		isaextensions "POPCNT"
		prepare()
		test.contains({ "-mpopcnt" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onPCLMUL()
		isaextensions "PCLMUL"
		prepare()
		test.contains({ "-mpclmul" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onLZCNT()
		isaextensions "LZCNT"
		prepare()
		test.contains({ "-mlzcnt" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onBMI()
		isaextensions "BMI"
		prepare()
		test.contains({ "-mbmi" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onBMI2()
		isaextensions "BMI2"
		prepare()
		test.contains({ "-mbmi2" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onF16C()
		isaextensions "F16C"
		prepare()
		test.contains({ "-mf16c" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onAES()
		isaextensions "AES"
		prepare()
		test.contains({ "-maes" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onFMA()
		isaextensions "FMA"
		prepare()
		test.contains({ "-mfma" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onFMA4()
		isaextensions "FMA4"
		prepare()
		test.contains({ "-mfma4" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onRDRND()
		isaextensions "RDRND"
		prepare()
		test.contains({ "-mrdrnd" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onMultipleISA()
		isaextensions {
			"RDRND",
			"FMA4"
		}
		prepare()
		test.contains({ "-mrdrnd", "-mfma4" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onAdditionalISA()
		isaextensions "RDRND"
		isaextensions "FMA4"
		prepare()
		test.contains({ "-mrdrnd", "-mfma4" }, gcc.getcflags(cfg))
	end

--
-- Check the defines and undefines.
--

	function suite.defines()
		defines "DEF"
		prepare()
		test.contains({ "-DDEF" }, gcc.getdefines(cfg.defines))
	end

	function suite.undefines()
		undefines "UNDEF"
		prepare()
		test.contains({ "-UUNDEF" }, gcc.getundefines(cfg.undefines))
	end


--
-- Check the optimization flags.
--

	function suite.cflags_onNoOptimize()
		optimize "Off"
		prepare()
		test.contains({ "-O0" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onOptimize()
		optimize "On"
		prepare()
		test.contains({ "-O2" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeSize()
		optimize "Size"
		prepare()
		test.contains({ "-Os" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeSpeed()
		optimize "Speed"
		prepare()
		test.contains({ "-O3" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeFull()
		optimize "Full"
		prepare()
		test.contains({ "-O3" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onOptimizeDebug()
		optimize "Debug"
		prepare()
		test.contains({ "-Og" }, gcc.getcflags(cfg))
	end


--
-- Check the translation of symbols.
--

	function suite.cflags_onDefaultSymbols()
		prepare()
		test.excludes({ "-g" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onNoSymbols()
		symbols "Off"
		prepare()
		test.excludes({ "-g" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onSymbols()
		symbols "On"
		prepare()
		test.contains({ "-g" }, gcc.getcflags(cfg))
	end


--
-- Check the translation of CXXFLAGS.
--

	function suite.cflags_onNoExceptions()
		exceptionhandling "Off"
		prepare()
		test.contains({ "-fno-exceptions" }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_onNoBufferSecurityCheck()
		flags { "NoBufferSecurityCheck" }
		prepare()
		test.contains({ "-fno-stack-protector" }, gcc.getcxxflags(cfg))
	end

--
-- Check the basic translation of LDFLAGS for a Posix system.
--

	function suite.ldflags_onNoSymbols()
		prepare()
		test.contains({ "-s" }, gcc.getldflags(cfg))
	end

	function suite.ldflags_onSymbols()
		symbols "On"
		prepare()
		test.excludes("-s", gcc.getldflags(cfg))
	end

	function suite.ldflags_onSharedLib()
		kind "SharedLib"
		prepare()
		test.contains({ "-shared" }, gcc.getldflags(cfg))
	end

--
-- Check Mac OS X variants on LDFLAGS.
--

	function suite.ldflags_onMacOSXBundle()
		system "MacOSX"
		kind "SharedLib"
		sharedlibtype "OSXBundle"
		prepare()
		test.contains({ "-Wl,-x", "-bundle" }, gcc.getldflags(cfg))
	end

	function suite.ldflags_onMacOSXFramework()
		system "MacOSX"
		kind "SharedLib"
		sharedlibtype "OSXFramework"
		prepare()
		test.contains({ "-Wl,-x", "-framework" }, gcc.getldflags(cfg))
	end

	function suite.ldflags_onMacOSXNoSymbols()
		system "MacOSX"
		prepare()
		test.contains({ "-Wl,-x" }, gcc.getldflags(cfg))
	end

	function suite.ldflags_onMacOSXSharedLib()
		system "MacOSX"
		kind "SharedLib"
		prepare()
		test.contains({ "-dynamiclib" }, gcc.getldflags(cfg))
	end


--
-- Check Windows variants on LDFLAGS.
--

	function suite.ldflags_onWindowsharedLib()
		system "Windows"
		kind "SharedLib"
		prepare()
		test.contains({ "-shared", '-Wl,--out-implib="bin/Debug/MyProject.lib"' }, gcc.getldflags(cfg))
	end

	function suite.ldflags_onWindowsApp()
		system "Windows"
		kind "WindowedApp"
		prepare()
		test.contains({ "-mwindows" }, gcc.getldflags(cfg))
	end



--
-- Make sure system or architecture flags are added properly.
--

	function suite.cflags_onX86()
		architecture "x86"
		prepare()
		test.contains({ "-m32" }, gcc.getcflags(cfg))
	end

	function suite.ldflags_onX86()
		architecture "x86"
		prepare()
		test.contains({ "-m32" }, gcc.getldflags(cfg))
	end

	function suite.cflags_onX86_64()
		architecture "x86_64"
		prepare()
		test.contains({ "-m64" }, gcc.getcflags(cfg))
	end

	function suite.ldflags_onX86_64()
		architecture "x86_64"
		prepare()
		test.contains({ "-m64" }, gcc.getldflags(cfg))
	end


--
-- Non-Windows shared libraries should marked as position independent.
--

	function suite.cflags_onWindowsSharedLib()
		system "MacOSX"
		kind "SharedLib"
		prepare()
		test.contains({ "-fPIC" }, gcc.getcflags(cfg))
	end


--
-- Check the formatting of linked system libraries.
--

	function suite.links_onSystemLibs()
		links { "fs_stub", "net_stub" }
		prepare()
		test.contains({ "-lfs_stub", "-lnet_stub" }, gcc.getlinks(cfg))
	end

	function suite.links_onFramework()
		links { "Cocoa.framework" }
		prepare()
		test.contains({ "-framework Cocoa" }, {table.implode (gcc.getlinks(cfg), '', '', ' ')})
	end

	function suite.links_onSystemLibs_onWindows()
		system "windows"
		links { "ole32" }
		prepare()
		test.contains({ "-lole32" }, gcc.getlinks(cfg))
	end


--
-- When linking to a static sibling library, the relative path to the library
-- should be used instead of the "-l" flag. This prevents linking against a
-- shared library of the same name, should one be present.
--

	function suite.links_onStaticSiblingLibrary()
		links { "MyProject2" }

		test.createproject(wks)
		system "Linux"
		kind "StaticLib"
		targetdir "lib"

		prepare()
		test.isequal({ "lib/libMyProject2.a" }, gcc.getlinks(cfg))
	end


--
-- Use the -lname format when linking to sibling shared libraries.
--

	function suite.links_onSharedSiblingLibrary()
		links { "MyProject2" }

		test.createproject(wks)
		system "Linux"
		kind "SharedLib"
		targetdir "lib"

		prepare()
		test.isequal({ "lib/libMyProject2.so" }, gcc.getlinks(cfg))
	end


--
-- When linking object files, leave off the "-l".
--

	function suite.links_onObjectFile()
		links { "generated.o" }
		prepare()
		test.isequal({ "generated.o" }, gcc.getlinks(cfg))
	end


--
-- If the object file is referenced with a path, it should be
-- made relative to the project.
--

	function suite.links_onObjectFileOutsideProject()
		location "MyProject"
		links { "obj/Debug/generated.o" }
		prepare()
		test.isequal({ "../obj/Debug/generated.o" }, gcc.getlinks(cfg))
	end


--
-- Make sure shell variables are kept intact for object file paths.
--

	function suite.links_onObjectFileWithShellVar()
		location "MyProject"
		links { "$(IntDir)/generated.o" }
		prepare()
		test.isequal({ "$(IntDir)/generated.o" }, gcc.getlinks(cfg))
	end


--
-- Include directories should be made project relative.
--

	function suite.includeDirsAreRelative()
		includedirs { "../include", "src/include" }
		prepare()
		test.isequal({ '-I../include', '-Isrc/include' }, gcc.getincludedirs(cfg, cfg.includedirs))
	end


--
-- Check handling of forced includes.
--

	function suite.forcedIncludeFiles()
		forceincludes { "stdafx.h", "include/sys.h" }
		prepare()
		test.isequal({'-include stdafx.h', '-include include/sys.h'}, gcc.getforceincludes(cfg))
	end


--
-- Include directories containing spaces (or which could contain spaces)
-- should be wrapped in quotes.
--

	function suite.includeDirs_onSpaces()
		includedirs { "include files" }
		prepare()
		test.isequal({ '-I"include files"' }, gcc.getincludedirs(cfg, cfg.includedirs))
	end

	function suite.includeDirs_onEnvVars()
		includedirs { "$(IntDir)/includes" }
		prepare()
		test.isequal({ '-I"$(IntDir)/includes"' }, gcc.getincludedirs(cfg, cfg.includedirs))
	end



--
-- Check handling of strict aliasing flags.
--

	function suite.cflags_onNoStrictAliasing()
		strictaliasing "Off"
		prepare()
		test.contains("-fno-strict-aliasing", gcc.getcflags(cfg))
	end

	function suite.cflags_onLevel1Aliasing()
		strictaliasing "Level1"
		prepare()
		test.contains({ "-fstrict-aliasing", "-Wstrict-aliasing=1" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onLevel2Aliasing()
		strictaliasing "Level2"
		prepare()
		test.contains({ "-fstrict-aliasing", "-Wstrict-aliasing=2" }, gcc.getcflags(cfg))
	end

	function suite.cflags_onLevel3Aliasing()
		strictaliasing "Level3"
		prepare()
		test.contains({ "-fstrict-aliasing", "-Wstrict-aliasing=3" }, gcc.getcflags(cfg))
	end


--
-- Check handling of system search paths.
--

	function suite.includeDirs_onSysIncludeDirs()
		sysincludedirs { "/usr/local/include" }
		prepare()
		test.contains("-isystem /usr/local/include", gcc.getincludedirs(cfg, cfg.includedirs, cfg.sysincludedirs))
	end

	function suite.libDirs_onSysLibDirs()
		syslibdirs { "/usr/local/lib" }
		prepare()
		test.contains("-L/usr/local/lib", gcc.getLibraryDirectories(cfg))
	end


--
-- Check handling of link time optimization flag.
--

	function suite.cflags_onLinkTimeOptimization()
		flags "LinkTimeOptimization"
		prepare()
		test.contains("-flto", gcc.getcflags(cfg))
	end

	function suite.ldflags_onLinkTimeOptimization()
		flags "LinkTimeOptimization"
		prepare()
		test.contains("-flto", gcc.getldflags(cfg))
	end


--
-- Check link mode preference for system libraries.
--
	function suite.linksModePreference_onAllStatic()
		links { "fs_stub:static", "net_stub:static" }
		prepare()
		test.contains({ "-Wl,-Bstatic", "-lfs_stub", "-Wl,-Bdynamic", "-lnet_stub"}, gcc.getlinks(cfg))
	end

	function suite.linksModePreference_onStaticAndShared()
		links { "fs_stub:static", "net_stub" }
		prepare()
		test.contains({ "-Wl,-Bstatic", "-lfs_stub", "-Wl,-Bdynamic", "-lnet_stub"}, gcc.getlinks(cfg))
	end

	function suite.linksModePreference_onAllShared()
		links { "fs_stub:shared", "net_stub:shared" }
		prepare()
		test.excludes({ "-Wl,-Bstatic" }, gcc.getlinks(cfg))
	end


--
-- Test language flags are added properly.
--

	function suite.cflags_onCDefault()
		cdialect "Default"
		prepare()
		test.contains({ }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_onC89()
		cdialect "C89"
		prepare()
		test.contains({ "-std=c89" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_onC90()
		cdialect "C90"
		prepare()
		test.contains({ "-std=c90" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_onC99()
		cdialect "C99"
		prepare()
		test.contains({ "-std=c99" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_onC11()
		cdialect "C11"
		prepare()
		test.contains({ "-std=c11" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_ongnu89()
		cdialect "gnu89"
		prepare()
		test.contains({ "-std=gnu89" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_ongnu90()
		cdialect "gnu90"
		prepare()
		test.contains({ "-std=gnu90" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_ongnu99()
		cdialect "gnu99"
		prepare()
		test.contains({ "-std=gnu99" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cflags_ongnu11()
		cdialect "gnu11"
		prepare()
		test.contains({ "-std=gnu11" }, gcc.getcflags(cfg))
		test.contains({ }, gcc.getcxxflags(cfg))
	end

	function suite.cxxflags_onCppDefault()
		cppdialect "Default"
		prepare()
		test.contains({ }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCpp98()
		cppdialect "C++98"
		prepare()
		test.contains({ "-std=c++98" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCpp11()
		cppdialect "C++11"
		prepare()
		test.contains({ "-std=c++11" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCpp14()
		cppdialect "C++14"
		prepare()
		test.contains({ "-std=c++14" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCpp17()
		cppdialect "C++17"
		prepare()
		test.contains({ "-std=c++17" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCppGnu98()
		cppdialect "gnu++98"
		prepare()
		test.contains({ "-std=gnu++98" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCppGnu11()
		cppdialect "gnu++11"
		prepare()
		test.contains({ "-std=gnu++11" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCppGnu14()
		cppdialect "gnu++14"
		prepare()
		test.contains({ "-std=gnu++14" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

	function suite.cxxflags_onCppGnu17()
		cppdialect "gnu++17"
		prepare()
		test.contains({ "-std=gnu++17" }, gcc.getcxxflags(cfg))
		test.contains({ }, gcc.getcflags(cfg))
	end

--
-- Test unsigned-char flags.
--

	function suite.sharedflags_onUnsignedChar()
		unsignedchar "On"

		prepare()
		test.contains({ "-funsigned-char" }, gcc.getcxxflags(cfg))
		test.contains({ "-funsigned-char" }, gcc.getcflags(cfg))
	end

	function suite.sharedflags_onNoUnsignedChar()
		unsignedchar "Off"

		prepare()
		test.contains({ "-fno-unsigned-char" }, gcc.getcxxflags(cfg))
		test.contains({ "-fno-unsigned-char" }, gcc.getcflags(cfg))
	end

--
-- Test omit-frame-pointer flags.
--

	function suite.sharedflags_onOmitFramePointerDefault()
		omitframepointer "Default"

		prepare()
		test.excludes({ "-fomit-frame-pointer", "-fno-omit-frame-pointer" }, gcc.getcxxflags(cfg))
		test.excludes({ "-fomit-frame-pointer", "-fno-omit-frame-pointer" }, gcc.getcflags(cfg))
	end

	function suite.sharedflags_onOmitFramePointer()
		omitframepointer "On"

		prepare()
		test.contains({ "-fomit-frame-pointer" }, gcc.getcxxflags(cfg))
		test.contains({ "-fomit-frame-pointer" }, gcc.getcflags(cfg))
	end

	function suite.sharedflags_onNoOmitFramePointer()
		omitframepointer "Off"

		prepare()
		test.contains({ "-fno-omit-frame-pointer" }, gcc.getcxxflags(cfg))
		test.contains({ "-fno-omit-frame-pointer" }, gcc.getcflags(cfg))
	end

--
-- Test visibility.
--

	function suite.cxxflags_onVisibilityDefault()
		visibility "Default"
		prepare()
		test.excludes({ "-fvisibility=default" }, gcc.getcflags(cfg))
		test.contains({ "-fvisibility=default" }, gcc.getcxxflags(cfg))
	end

	function suite.cxxflags_onVisibilityHidden()
		visibility "Hidden"
		prepare()
		test.excludes({ "-fvisibility=hidden" }, gcc.getcflags(cfg))
		test.contains({ "-fvisibility=hidden" }, gcc.getcxxflags(cfg))
	end

	function suite.cxxflags_onVisibilityInternal()
		visibility "Internal"
		prepare()
		test.excludes({ "-fvisibility=internal" }, gcc.getcflags(cfg))
		test.contains({ "-fvisibility=internal" }, gcc.getcxxflags(cfg))
	end

	function suite.cxxflags_onVisibilityProtected()
		visibility "Protected"
		prepare()
		test.excludes({ "-fvisibility=protected" }, gcc.getcflags(cfg))
		test.contains({ "-fvisibility=protected" }, gcc.getcxxflags(cfg))
	end

--
-- Test inlines visibility flags.
--

	function suite.cxxflags_onInlinesVisibilityDefault()
		inlinesvisibility "Default"
		prepare()
		test.excludes({ "-fvisibility-inlines-hidden" }, gcc.getcflags(cfg))
		test.excludes({ "-fvisibility-inlines-hidden" }, gcc.getcxxflags(cfg))
	end

	function suite.cxxflags_onInlinesVisibilityHidden()
		inlinesvisibility "Hidden"
		prepare()
		test.excludes({ "-fvisibility-inlines-hidden" }, gcc.getcflags(cfg))
		test.contains({ "-fvisibility-inlines-hidden" }, gcc.getcxxflags(cfg))
	end
