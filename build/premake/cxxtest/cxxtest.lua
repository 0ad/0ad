local m = {}
m._VERSION = "1.0.0-dev"

m.path = nil

-- We need to create .cpp files from the .h files before they can be compiled.
-- This is done by the cxxtestgen utility.

-- We use a custom build rule for headers in order to generate the .cpp files.
-- This has several advantages over using pre-build commands:
-- - It cannot break when running parallel builds
-- - No new generation happens when the header files are untouched

function m.configure_project(rootfile, hdrfiles, rootoptions, testoptions)
	if rootoptions == nil then
		rootoptions = ''
	end
	if testoptions == nil then
		testoptions = ''
	end

	local abspath = path.getabsolute(m.path)
	local rootpath = path.getabsolute(rootfile)

	-- Add headers

	for _,hdrfile in ipairs(hdrfiles) do
		files { hdrfile }
	end

	-- Generate the root file
	prebuildmessage 'Generating test root file'
	prebuildcommands { abspath.." --root "..rootoptions.." -o "..rootpath }

	-- Generate the source files from headers

	filter { "files:**.h", "files:not **precompiled.h" }
		buildmessage 'Generating %{file.basename}.cpp'
		buildcommands { abspath.." --part "..testoptions.." -o %{file.directory}/%{file.basename}.cpp %{file.relpath}" }
		buildoutputs { "%{file.directory}/%{file.basename}.cpp" }
	filter {}

	-- Add source files

	files { rootfile }

	for _,hdrfile in ipairs(hdrfiles) do
		local srcfile = string.sub(hdrfile, 1, -3) .. ".cpp"
		files { srcfile }
	end
end

return m
