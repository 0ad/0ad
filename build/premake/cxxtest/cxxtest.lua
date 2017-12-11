local m = {}
m._VERSION = "1.0.0-dev"

m.exepath = nil
m.rootfile = nil
m.runner = "ErrorPrinter"
m.options = ""

-- Premake module for CxxTest support (http://cxxtest.com/).
-- The module can be used for generating a root file (that contains the entrypoint
-- for the test executable) and source files for each test header.

-- Set the executable path for cxxtestgen
function m.setpath(exepath)
	m.exepath = path.getabsolute(exepath)
end

-- Pass all the necessary options to cxxtest (see http://cxxtest.com/guide.html)
-- for a reference of available options, that should eventually be implemented in
-- this module.
function m.init(source_root, have_std, runner, includes)

	m.rootfile = source_root.."test_root.cpp"
	m.runner = runner

	if m.have_std then
		m.options = m.options.." --have-std"
	end

	for _,includefile in ipairs(includes) do
		m.options = m.options.." --include="..includefile
	end

	-- With gmake, create a Utility project that generates the test root file
	-- This is a workaround for https://github.com/premake/premake-core/issues/286
	if _ACTION == "gmake" then
		project "cxxtestroot"
		kind "Utility"

		-- Note: this command is not silent and clutters the output
		-- Reported upstream: https://github.com/premake/premake-core/issues/954
		prebuildmessage 'Generating test root file'
		prebuildcommands { m.exepath.." --root "..m.options.." --runner="..m.runner.." -o "..path.getabsolute(m.rootfile) }
		buildoutputs { m.rootfile }
	end
end

-- Populate the test project that was created in premake5.lua.
function m.configure_project(hdrfiles)

	-- Generate the root file, or make sure the utility for generating
	-- it is a dependancy with gmake.
	if _ACTION == "gmake" then
		dependson { "cxxtestroot" }
	else
		prebuildmessage 'Generating test root file'
		prebuildcommands { m.exepath.." --root "..m.options.." --runner="..m.runner.." -o "..path.getabsolute(m.rootfile) }
	end

	-- Add headers
	for _,hdrfile in ipairs(hdrfiles) do
		files { hdrfile }
	end

	-- Generate the source files from headers
	-- This doesn't work with xcode, see https://github.com/premake/premake-core/issues/940
	filter { "files:**.h", "files:not **precompiled.h" }
		buildmessage 'Generating %{file.basename}.cpp'
		buildcommands { m.exepath.." --part "..m.options.." -o %{file.directory}/%{file.basename}.cpp %{file.relpath}" }
		buildoutputs { "%{file.directory}/%{file.basename}.cpp" }
	filter {}

	-- Add source files
	files { m.rootfile }
	for _,hdrfile in ipairs(hdrfiles) do
		local srcfile = string.sub(hdrfile, 1, -3) .. ".cpp"
		files { srcfile }
	end
end

return m
