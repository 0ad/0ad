/**********************************************************************
 * Premake - vs6.c
 * The Visual C++ 6 target
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
#include <string.h>
#include "premake.h"
#include "vs6.h"

static int writeWorkspace();

static const char* listPackageDeps(const char* name);


int vs6_generate()
{
	int i;

	puts("Generating Visual Studio 6 workspace and project files:");

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);

		printf("...%s\n", prj_get_pkgname());

		if (prj_is_lang("c++") || prj_is_lang("c"))
		{
			vs6_cpp();
		}
		else if (prj_is_lang("c#"))
		{
			puts("** Error: C# projects are not supported by Visual Studio 6");
			return 0;
		}
		else
		{
			printf("** Error: unrecognized language '%s'\n", prj_get_language());
			return 0;
		}
	}

	return writeWorkspace();
}


static int writeWorkspace()
{
	int i;

	if (!io_openfile(path_join(prj_get_path(), prj_get_name(), "dsw")))
		return 0;

	io_print("Microsoft Developer Studio Workspace File, Format Version 6.00\n");
	io_print("# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!\n");
	io_print("\n");
	io_print("###############################################################################\n");
	io_print("\n");

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);

		io_print("Project: \"%s\"=%s - Package Owner=<4>\n", prj_get_pkgname(), prj_get_pkgfilename("dsp"));
		io_print("\n");
		io_print("Package=<5>\n");
		io_print("{{{\n");
		io_print("}}}\n");
		io_print("\n");
		io_print("Package=<4>\n");
		io_print("{{{\n");

		/* Write package dependencies */
		prj_select_config(0);
		print_list(prj_get_links(), "", "", "", listPackageDeps);

		io_print("}}}\n");
		io_print("\n");
		io_print("###############################################################################\n");
		io_print("\n");
	}

	io_print("Global:\n");
	io_print("\n");
	io_print("Package=<5>\n");
	io_print("{{{\n");
	io_print("}}}\n");
	io_print("\n");
	io_print("Package=<3>\n");
	io_print("{{{\n");
	io_print("}}}\n");
	io_print("\n");
	io_print("###############################################################################\n");
	io_print("\n");

	io_closefile();
	return 1;
}


/************************************************************************
 * Checks if a package link matches the name of a sibling package. If
 * so, generate a dependency on that sibling
 ***********************************************************************/

static const char* listPackageDeps(const char* name)
{
	int i = prj_find_package(name);
	if (i >= 0)
	{
		strcpy(g_buffer, "    Begin Project Dependency\n");
		strcat(g_buffer, "    Project_Dep_Name ");
		strcat(g_buffer, prj_get_pkgname_for(i));
		strcat(g_buffer, "\n");
		strcat(g_buffer, "    End Project Dependency\n");
		return g_buffer;
	}
	else
	{
		return NULL;
	}
}
