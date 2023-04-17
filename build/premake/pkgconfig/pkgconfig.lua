local m = {}
m._VERSION = "1.1.2-dev"

m.additional_pc_path = nil
m.static_link_libs = false

local function os_capture(cmd)
	return io.popen(cmd, 'r'):read('*a'):gsub("\n", " ")
end

function m.add_includes(lib, alternative_cmd, alternative_flags)
	local result
	if not alternative_cmd then
		local pc_path = m.additional_pc_path and "PKG_CONFIG_PATH="..m.additional_pc_path or ""
		result = os_capture(pc_path.." pkg-config --cflags "..lib)
	else
		if not alternative_flags then
			result = os_capture(alternative_cmd.." --cflags")
		else
			result = os_capture(alternative_cmd.." "..alternative_flags)
		end
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

	-- As of premake5-beta2, `sysincludedirs` has been deprecated in favour of
	-- `externalincludedirs`, and continuing to use it causes warnings to be emitted.
	-- We use `externalincludedirs` when available to prevent the warnings, falling back
	-- to `sysincludedirs` when not to prevent breakage of the `--with-system-premake5`
	-- build argument.
	if externalincludedirs then
		externalincludedirs(dirs)
	else
		sysincludedirs(dirs)
	end

	forceincludes(files)
	buildoptions(options)
end

function m.add_links(lib, alternative_cmd, alternative_flags)
	local result
	if not alternative_cmd then
		local pc_path = m.additional_pc_path and "PKG_CONFIG_PATH="..m.additional_pc_path or ""
		local static = m.static_link_libs and " --static " or ""
		result = os_capture(pc_path.." pkg-config --libs "..static..lib)
	else
		if not alternative_flags then
			result = os_capture(alternative_cmd.." --libs")
		else
			result = os_capture(alternative_cmd.." "..alternative_flags)
		end
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
