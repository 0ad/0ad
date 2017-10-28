---
-- tests/base/test_os.lua
-- Automated test suite for the new OS functions.
-- Copyright (c) 2008-2017 Jason Perkins and the Premake project
---

	local suite = test.declare("base_os")

	local cwd

	function suite.setup()
		cwd = os.getcwd()
		os.chdir(_TESTS_DIR)
	end

	function suite.teardown()
		os.chdir(cwd)
	end


--
-- os.findlib() tests
--

	function suite.findlib_FindSystemLib()
		if os.istarget("windows") then
			test.istrue(os.findlib("user32"))
		elseif os.istarget("haiku") then
			test.istrue(os.findlib("root"))
		else
			test.istrue(os.findlib("m"))
		end
	end

	function suite.findlib_FailsOnBadLibName()
		test.isfalse(os.findlib("NoSuchLibraryAsThisOneHere"))
	end

	function suite.findheader_stdheaders()
		if not os.istarget("windows") and not os.istarget("macosx") then
			test.istrue(os.findheader("stdlib.h"))
		end
	end

	function suite.findheader_failure()
		test.isfalse(os.findheader("Knights/who/say/Ni.hpp"))
	end


--
-- os.isfile() tests
--

	function suite.isfile_ReturnsTrue_OnExistingFile()
		test.istrue(os.isfile("_tests.lua"))
	end

	function suite.isfile_ReturnsFalse_OnNonexistantFile()
		test.isfalse(os.isfile("no_such_file.lua"))
	end



--
-- os.matchdirs() tests
--

	function suite.matchdirs_skipsDottedDirs()
		local result = os.matchdirs("*")
		test.isfalse(table.contains(result, ".."))
	end



--
-- os.matchfiles() tests
--

	function suite.matchfiles_OnNonRecursive()
		local result = os.matchfiles("*.lua")
		test.istrue(table.contains(result, "_tests.lua"))
		test.isfalse(table.contains(result, "folder/ok.lua"))
	end

	function suite.matchfiles_Recursive()
		local result = os.matchfiles("**.lua")
		test.istrue(table.contains(result, "folder/ok.lua"))
	end

	function suite.matchfiles_SkipsDotDirs_OnRecursive()
		local result = os.matchfiles("**.lua")
		test.isfalse(table.contains(result, ".svn/text-base/testfx.lua.svn-base"))
	end

	function suite.matchfiles_OnSubfolderMatch()
		local result = os.matchfiles("**/subfolder/*")
		test.istrue(table.contains(result, "folder/subfolder/hello.txt"))
		test.isfalse(table.contains(result, "premake4.lua"))
	end

	function suite.matchfiles_OnDotSlashPrefix()
		local result = os.matchfiles("./**.lua")
		test.istrue(table.contains(result, "folder/ok.lua"))
	end

	function suite.matchfiles_OnImplicitEndOfString()
		local result = os.matchfiles("folder/*.lua")
		test.istrue(table.contains(result, "folder/ok.lua"))
		test.isfalse(table.contains(result, "folder/ok.lua.2"))
	end

	function suite.matchfiles_OnLeadingDotSlashWithPath()
		local result = os.matchfiles("./folder/*.lua")
		test.istrue(table.contains(result, "folder/ok.lua"))
	end

	function suite.matchfiles_OnDottedFile()
		local result = os.matchfiles("base/.*")
		test.istrue(table.contains(result, "base/.testDotFile"))
	end

	function suite.matchfiles_onComboSearch()
		local result = os.matchfiles("folder/**/*.txt")
		test.istrue(table.contains(result, "folder/subfolder/hello.txt"))
	end


--
-- os.pathsearch() tests
--

	function suite.pathsearch_ReturnsNil_OnNotFound()
		test.istrue(os.pathsearch("nosuchfile", "aaa;bbb;ccc") == nil)
	end

	function suite.pathsearch_ReturnsPath_OnFound()
		test.isequal(_TESTS_DIR, os.pathsearch("_tests.lua", _TESTS_DIR))
	end

	function suite.pathsearch_FindsFile_OnComplexPath()
		test.isequal(_TESTS_DIR, os.pathsearch("_tests.lua", "aaa;" .. _TESTS_DIR .. ";bbb"))
	end

	function suite.pathsearch_NilPathsAllowed()
		test.isequal(_TESTS_DIR, os.pathsearch("_tests.lua", nil, _TESTS_DIR, nil))
	end


--
-- os.outputof() tests
--

	-- Check if outputof returns the command exit code
	-- in addition of the command output
	function suite.outputof_commandExitCode()
		if os.istarget("macosx")
			or os.istarget("linux")
			or os.istarget("solaris")
			or os.istarget("bsd")
		then
			-- Assumes 'true' and 'false' commands exist
			-- which should be the case on all *nix platforms
			for cmd, exitcode in pairs ({
				["true"] = 0,
				["false"] = 1
			})
			do
				local o, e = os.outputof(cmd)
				test.isequal(e, exitcode)
			end
		end
	end


--
-- os.translateCommand() tests
--

	function suite.translateCommand_onNoToken()
		test.isequal("cp a b", os.translateCommands("cp a b"))
	end

	function suite.translateCommand_callsProcessor()
		os.commandTokens.test = {
			copy = function(value) return "test " .. value end
		}
		test.isequal("test a b", os.translateCommands("{COPY} a b", "test"))
	end

	function suite.translateCommand_callsProcessor_multipleTokens()
		os.commandTokens.test = {
			copy = function(value) return "test " .. value end
		}
		test.isequal("test a b; test c d; test e f;", os.translateCommands("{COPY} a b; {COPY} c d; {COPY} e f;", "test"))
	end

--
-- os.translateCommand() windows COPY tests
--

	function suite.translateCommand_windowsCopyNoDst()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a > nul) ELSE (xcopy /Q /Y /I a > nul)', os.translateCommands('{COPY} a', "windows"))
	end

	function suite.translateCommand_windowsCopyNoDst_ExtraSpace()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a > nul) ELSE (xcopy /Q /Y /I a > nul)', os.translateCommands('{COPY} a ', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotes()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a b > nul) ELSE (xcopy /Q /Y /I a b > nul)', os.translateCommands('{COPY} a b', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotes_ExtraSpace()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a b > nul) ELSE (xcopy /Q /Y /I a b > nul)', os.translateCommands('{COPY} a b ', "windows"))
	end

	function suite.translateCommand_windowsCopyQuotes()
		test.isequal('IF EXIST "a a"\\ (xcopy /Q /E /Y /I "a a" "b" > nul) ELSE (xcopy /Q /Y /I "a a" "b" > nul)', os.translateCommands('{COPY} "a a" "b"', "windows"))
	end

	function suite.translateCommand_windowsCopyQuotes_ExtraSpace()
		test.isequal('IF EXIST "a a"\\ (xcopy /Q /E /Y /I "a a" "b" > nul) ELSE (xcopy /Q /Y /I "a a" "b" > nul)', os.translateCommands('{COPY} "a a" "b" ', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotesDst()
		test.isequal('IF EXIST "a a"\\ (xcopy /Q /E /Y /I "a a" b > nul) ELSE (xcopy /Q /Y /I "a a" b > nul)', os.translateCommands('{COPY} "a a" b', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotesDst_ExtraSpace()
		test.isequal('IF EXIST "a a"\\ (xcopy /Q /E /Y /I "a a" b > nul) ELSE (xcopy /Q /Y /I "a a" b > nul)', os.translateCommands('{COPY} "a a" b ', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotesSrc()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a "b" > nul) ELSE (xcopy /Q /Y /I a "b" > nul)', os.translateCommands('{COPY} a "b"', "windows"))
	end

	function suite.translateCommand_windowsCopyNoQuotesSrc_ExtraSpace()
		test.isequal('IF EXIST a\\ (xcopy /Q /E /Y /I a "b" > nul) ELSE (xcopy /Q /Y /I a "b" > nul)', os.translateCommands('{COPY} a "b" ', "windows"))
	end

--
-- os.getWindowsRegistry windows tests
--
	function suite.getreg_nonExistentValue()
		if os.ishost("windows") then
			local x = os.getWindowsRegistry("HKCU:Should\\Not\\Exist\\At\\All")
			test.isequal(nil, x)
		end
	end

	function suite.getreg_nonExistentDefaultValue()
		if os.ishost("windows") then
			local x = os.getWindowsRegistry("HKCU:Should\\Not\\Exist\\At\\All\\")
			test.isequal(nil, x)
		end
	end

	function suite.getreg_noSeparators()
		if os.ishost("windows") then
			os.getWindowsRegistry("HKCU:ShouldNotExistAtAll")
		end
	end

	function suite.getreg_namedValue()
		if os.ishost("windows") then
			local x = os.getWindowsRegistry("HKCU:Environment\\TEMP")
			test.istrue(x ~= nil)
		end
	end

	function suite.getreg_namedValueOptSeparator()
		if os.ishost("windows") then
			local x = os.getWindowsRegistry("HKCU:\\Environment\\TEMP")
			test.istrue(x ~= nil)
		end
	end

	function suite.getreg_defaultValue()
		if os.ishost("windows") then
			local x = os.getWindowsRegistry("HKLM:SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal\\AppInfo\\")
			test.isequal("Service", x)
		end
	end


--
-- os.getversion tests.
--

	function suite.getversion()
		local version = os.getversion();
		test.istrue(version ~= nil)
	end



--
-- os.translateCommandsAndPaths.
--

	function suite.translateCommandsAndPaths()
		test.isequal('cmdtool "../foo/path1"', os.translateCommandsAndPaths("cmdtool %[path1]", '../foo', '.', 'osx'))
	end

	function suite.translateCommandsAndPaths_PreserveSlash()
		test.isequal('cmdtool "../foo/path1/"', os.translateCommandsAndPaths("cmdtool %[path1/]", '../foo', '.', 'osx'))
	end

	function suite.translateCommandsAndPaths_MultipleTokens()
		test.isequal('cmdtool "../foo/path1" "../foo/path2/"', os.translateCommandsAndPaths("cmdtool %[path1] %[path2/]", '../foo', '.', 'osx'))
	end

