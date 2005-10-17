function sourcesfromdirs(root, dirs)
	local res = {}
	for i=1, getn(dirs) do
		local files = matchfiles(
			root..dirs[i].."/*.cpp",
			root..dirs[i].."/*.h",
			root..dirs[i].."/*.asm")
		tconcat(res, files)
	end
	return res
end

function trimrootdir(root, dirs)
	for i=1, getn(dirs) do
		dirs[i] = strsub(dirs[i], strlen(root))
	end
end

function tconcat(table, values)
	for i=1, getn(values) do
		tinsert(table, values[i])
	end
end
