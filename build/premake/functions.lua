function sourcesfromdirs(root, ...)
	local res = {}
	for i=1, getn(arg) do
		res[i]=matchfiles(root..arg[i].."/*.cpp", root..arg[i].."/*.h")
	end
	return res
end