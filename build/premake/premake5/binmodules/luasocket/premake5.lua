project "luasocket"
	language    "C"
	kind        "SharedLib"
	warnings    "extra"

	includedirs
	{
		"../../contrib/lua/src",
		"../../contrib/luashim"
	}
	
	links { 'luashim-lib' }

	files
	{
		"src/*.c",
		"src/*.h",
		"src/*.lua",
		"*.c"
	}

	filter "system:windows"
		removefiles
		{
			"src/serial.c",
			"src/unixdgram.*",
			"src/unixstream.*",
			"src/unix.*",
			"src/usocket.*",
		}
		
		links { 'ws2_32' }
		characterset "MBCS"
		
		defines { "LUASOCKET_API=__declspec(dllexport)" }
	
	filter "system:not windows"
		removefiles
		{
			"src/wsocket.*",
		}
		
		pic         "on"
		
		defines { "LUASOCKET_API=__attribute__((visibility(\"default\")))" }
		
	filter "configurations:Release"
		targetdir "../../bin/release"

	filter "configurations:Debug"
		targetdir "../../bin/debug"