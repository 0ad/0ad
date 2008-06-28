/**********************************************************************
 * Premake - premake.c
 * The program entry point.
 *
 * Copyright (c) 2002-2005 Jason Perkins and the Premake project
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the file LICENSE.txt for details.
 **********************************************************************/

#include <stdio.h>
#include "premake.h"
#include "arg.h"
#include "os.h"
#include "script.h"
#include "Lua/lua.h"

#include "gnu.h"
#include "sharpdev.h"
#include "vs6.h"
#include "vs2002.h"
#include "vs2005.h"

const char* DEFAULT   = "premake.lua";
const char* VERSION   = "3.1";
const char* COPYRIGHT = "Copyright (C) 2002-2006 Jason Perkins and the Premake Project";
const char* HELP_MSG  = "Type 'premake --help' for help";

const char* g_filename;
const char* g_cc;
const char* g_dotnet;
int         g_verbose;
int         g_hasScript;

static int  preprocess();
static int  postprocess();
static void showUsage();

int clean();


/**********************************************************************
 * Program entry point
 **********************************************************************/

int main(int argc, char** argv)
{
	/* If no args are specified... */
	if (argc == 1)
	{
		puts(HELP_MSG);
		return 1;
	}

	/* Set defaults */
	os_detect();
	g_filename = DEFAULT;
	g_cc       = NULL;
	g_dotnet   = NULL;
	g_verbose  = 0;

	/* Process any options that will effect script processing */
	arg_set(argc, argv);
	if (!preprocess())
		return 1;

	/* chdir() to the directory containing the project script, so that
	 * relative paths may be used in the script */
	io_chdir(path_getdir(g_filename));

	/* Now run the script */
	g_hasScript = script_run(g_filename);
	if (g_hasScript < 0)
	{
		puts("** Script failed to run, ending.");
		return 1;
	}

	/* Process any options that depend on the script output */
	arg_reset();
	if (!postprocess())
		return 1;

	/* All done */
	if (g_hasScript)
		script_close();
	prj_close();
	return 0;
}


/**********************************************************************
 * Command-line processing that is done before the script is run
 **********************************************************************/

static int preprocess()
{
	const char* flag = arg_getflag();
	while (flag != NULL)
	{
		if (matches(flag, "--file"))
		{
			g_filename = arg_getflagarg();
			if (g_filename == NULL)
			{
				puts("** Usage: --file filename");
				puts(HELP_MSG);
				return 1;
			}
		}
		else if (matches(flag, "--os"))
		{
			const char* os = arg_getflagarg();
			if (os == NULL || !os_set(os))
			{
				puts("** Usage: --os osname");
				puts(HELP_MSG);
				return 1;
			}
		}
		else if (matches(flag, "--version"))
		{
			printf("premake (Premake Build Script Generator) %s\n", VERSION);
		}

		flag = arg_getflag();
	}

	return 1;
}


/**********************************************************************
 * Command-line processing that happens after the script has run
 **********************************************************************/

static int postprocess()
{
	int noScriptWarning = 0;

	const char* flag = arg_getflag();
	while (flag != NULL)
	{
		if (g_hasScript && !script_export())
			return 0;

		if (matches(flag, "--help"))
		{
			showUsage();
		}
		else if (matches(flag, "--version"))
		{
			/* ignore quietly */
		}
		else if (matches(flag, "--file"))
		{
			arg_getflagarg();
		}
		else
		{
			if (!g_hasScript)
			{
				if (!noScriptWarning)
				{
					puts("** No Premake script found!");
					noScriptWarning = 1;
				}
			}
			else
			{
				script_docommand(flag);
			}
		}

		flag = arg_getflag();
	}

	return 1;
}


/**********************************************************************
 * Default command handler
 **********************************************************************/

int onCommand(const char* cmd, const char* arg)
{
	if (matches(cmd, "target"))
	{
		if (matches(arg, "gnu"))
		{
			return gnu_generate();
		}
		else if (matches(arg, "monodev") || matches(arg, "md"))
		{
			return sharpdev_generate("monodev");
		}
		else if (matches(arg, "sharpdev") || matches(arg, "sd"))
		{
			return sharpdev_generate("sharpdev");
		}
		else if (matches(arg, "vs6"))
		{
			return vs6_generate();
		}
		else if (matches(arg, "vs2002") || matches(arg, "vs7"))
		{
			return vs2002_generate(2002);
		}
		else if (matches(arg, "vs2003"))
		{
			return vs2002_generate(2003);
		}
		else if (matches(arg, "vs2005"))
		{
			return vs2005_generate(2005);
		}
		else if (matches(arg, "vs2008"))
		{
			return vs2005_generate(2008);
		}
		else
		{
			printf("** Unrecognized target '%s'\n", arg);
			return 0;
		}
	}

	else if (matches(cmd, "clean"))
	{
		return clean();
	}

	else if (matches(cmd, "cc"))
	{
		g_cc = arg;
	}
	else if (matches(cmd, "dotnet"))
	{
		g_dotnet = arg;
	}
	else if (matches(cmd, "verbose"))
	{
		g_verbose = 1;
	}

	return 1;
}


/**********************************************************************
 * Help message text
 **********************************************************************/

void showUsage()
{
	int i;

	printf("Premake %s, a build script generator\n", VERSION);
	puts(COPYRIGHT);
	printf("%s %s\n", LUA_VERSION, LUA_COPYRIGHT);
	puts("");
	puts(" --file name       Process the specified premake script file");
	puts("");
	puts(" --clean           Remove all binaries and build scripts");
	puts(" --verbose       Generate verbose makefiles (where applicable)");
	puts("");
	puts(" --cc name         Choose a C/C++ compiler, if supported by target; one of:");
	puts("      gcc       GNU gcc compiler");
	puts("      dmc       Digital Mars C/C+ compiler (experimental)");
	puts("      icc       Intel C++ Compiler (highly experimental)");
	puts("");
	puts(" --dotnet name     Choose a .NET compiler set, if supported by target; one of:");
	puts("      ms        Microsoft (csc)");
	puts("      mono      Mono (mcs)");
	puts("      mono2     Mono .NET 2.0 (gmcs)");
	puts("      pnet      Portable.NET (cscc)");
	puts("");
	puts(" --os name         Generate files for different operating system; one of:");
	puts("      bsd       OpenBSD, NetBSD, or FreeBSD");
	puts("      linux     Linux");
	puts("      macosx    MacOS X");
	puts("      windows   Microsoft Windows");
	puts("");
	puts(" --target name     Generate input files for the specified toolset; one of:");
	puts("      gnu       GNU Makefile for POSIX, MinGW, and Cygwin");
	puts("      monodev   MonoDevelop");
	puts("      sharpdev  ICSharpCode SharpDevelop");
	puts("      vs6       Microsoft Visual Studio 6");
	puts("      vs2002    Microsoft Visual Studio 2002");
	puts("      vs2003    Microsoft Visual Studio 2003");
	puts("      vs2005    Microsoft Visual Studio 2005 (includes Express editions)");
	puts("      vs2008    Microsoft Visual Studio 2008 (includes Express editions)");
	puts("");
	puts(" --help            Display this information");
	puts(" --version         Display version information");
	puts("");

	if (project != NULL && prj_get_numoptions() > 0)
	{
		puts("This premake configuration also supports the following custom options:");
		puts("");

		for (i = 0; i < prj_get_numoptions(); ++i)
		{
			prj_select_option(i);
			printf(" --%-15s %s\n", prj_get_optname(), prj_get_optdesc());
		}
	}

	puts("");
}
