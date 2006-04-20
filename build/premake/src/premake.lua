project.name     = "Premake"

-- Options

	addoption("with-tests", "Include the unit tests (requires NUnit)")

-- Project Settings

	project.bindir   = "bin"

	dopackage("Src")
	if (options["with-tests"]) then
		dopackage("Tests")
	end

-- A little extra cleanup

	function doclean(cmd, arg)
		docommand(cmd, arg)
		os.rmdir("bin")
	end
