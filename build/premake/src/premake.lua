project.name     = "Premake"

package.language = "c++"
package.kind     = "exe"
package.target   = "premake"

package.buildflags = { "no-64bit-checks", "static-runtime" }
package.config["Release"].buildflags = { "no-symbols", "optimize-size" }

package.files =
{
	"Src/premake.c",
	"Src/project.h",
	"Src/project.c",
	"Src/util.h",
	"Src/util.c",
	"Src/clean.c",
	"Src/gnu.c",
	"Src/sharpdev.c",
	"Src/vs7.c",
	"Src/vs6.c",
	matchfiles("Src/Lua/*.c", "Src/Lua/*.h")
}

if (OS == "windows") then
	tinsert(package.files, "Src/windows.c")
else
	tinsert(package.files, "Src/posix.c")
end

