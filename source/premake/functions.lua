function sourcesfromdirs(...)
	local res = {}
	for i=1, getn(arg) do
		res[i]=matchfiles(arg[i].."/*.cpp", arg[i].."/*.h")
	end
	return res
end