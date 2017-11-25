local m = {}
m._VERSION = "1.0.0-dev"

local function os_capture(cmd)
	return io.popen(cmd, 'r'):read('*a'):gsub("\n", " ")
end

function m.add_includes(lib, alternative_cmd)
	local result
	if not alternative_cmd then
		result = os_capture("pkg-config --cflags "..lib)
	else
		result = os_capture(alternative_cmd)
	end

	-- Small trick: delete the space after -include so that we can detect
	-- which files have to be force-included without difficulty.
	result = result:gsub("%-include +(%g+)", "-include%1")

	local dirs = {}
	local files = {}
	local options = {}
	for w in string.gmatch(result, "[^' ']+") do
		if string.sub(w,1,2) == "-I" then
			table.insert(dirs, string.sub(w,3))
		elseif string.sub(w,1,8) == "-include" then
			table.insert(files, string.sub(w,9))
		else
			table.insert(options, w)
		end
	end

	sysincludedirs(dirs)
	forceincludes(files)
	buildoptions(options)
end

function m.add_links(lib, alternative_cmd)
	local result
	if not alternative_cmd then
		result = os_capture("pkg-config --libs "..lib)
	else
		result = os_capture(alternative_cmd)
	end

	-- On OSX, wx-config outputs "-framework foo" instead of "-Wl,-framework,foo"
	-- which doesn't fare well with the splitting into libs, libdirs and options
	-- we perform afterwards.
	result = result:gsub("%-framework +(%g+)", "-Wl,-framework,%1")

	local libs = {}
	local dirs = {}
	local options = {}
	for w in string.gmatch(result, "[^' ']+") do
		if string.sub(w,1,2) == "-l" then
			table.insert(libs, string.sub(w,3))
		elseif string.sub(w,1,2) == "-L" then
			table.insert(dirs, string.sub(w,3))
		else
			table.insert(options, w)
		end
	end

	links(libs)
	libdirs(dirs)
	linkoptions(options)
end

return m
