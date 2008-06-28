/**********************************************************************
 * Premake - vs2005.c
 * The Visual Studio 2005 target
 *
 * Copyright (c) 2002-2006 Jason Perkins and the Premake project
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
#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "vs.h"
#include "vs2005.h"

static int vs2005_write_solution(int target);
static const char* list_aspnet_refs(const char* name);

int vs2005_generate(int target)
{
	int p;

	if (target == 2005)
	{
		vs_setversion(VS2005);
		printf("Generating Visual Studio 2005 solution and project files:\n");
	}
	else
	{
		vs_setversion(VS2008);
		printf("Generating Visual Studio 2008 solution and project files:\n");
	}

	/* Assign GUIDs to packages */
	vs_assign_guids();

	/* Generate the project files */
	for (p = 0; p < prj_get_numpackages(); ++p)
	{
		prj_select_package(p);
		prj_select_config(0);

		printf("...%s\n", prj_get_pkgname());

		if (prj_is_kind("aspnet"))
		{
			/* No project files?! */
		}
		else if (prj_is_lang("c++") || prj_is_lang("c"))
		{
			if (!vs_write_cpp())
				return 0;
		}
		else if (prj_is_lang("c#"))
		{
			if (!vs2005_cs())
				return 0;
		}
		else
		{
			printf("** Warning: %s packages are not supported by this generator\n", prj_get_language());
		}
	}

	return vs2005_write_solution(target);
}


/************************************************************************
 * Write out the solution file
 ***********************************************************************/

static int vs2005_write_solution(int target)
{
	VsPkgData* data;
	int hasDotNet, hasCpp;
	int i, j;
	int numAspNet, port;

	if (!io_openfile(path_join(prj_get_path(), prj_get_name(), "sln")))
		return 0;

	/* Format identification string */
	if (target == 2005)
	{
		io_print("Microsoft Visual Studio Solution File, Format Version 9.00\n");
		io_print("# Visual Studio 2005\n");
	}
	else
	{
		io_print("Microsoft Visual Studio Solution File, Format Version 10.00\n");
		io_print("# Visual Studio 2008\n");
	}

	/* List packages */
	numAspNet = 0;
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);
		prj_select_config(0);

		data = (VsPkgData*)prj_get_data();

		if (prj_is_kind("aspnet"))
		{
			const char* path = prj_get_pkgpath();
			if (strlen(path) == 0) path = ".";

			io_print("Project(\"{%s}\") = \"%s\", \"%s\\\", \"{%s}\"\n", data->toolGuid, prj_get_pkgname(), path, data->projGuid);
			io_print("\tProjectSection(WebsiteProperties) = preProject\n");

			if (prj_get_numlinks() > 0)
				print_list(prj_get_links(), "\t\tProjectReferences = \"", ";\"\n", ";", list_aspnet_refs);

			for (j = 0; j < prj_get_numconfigs(); ++j)
			{
				prj_select_config(j);
				io_print("\t\t%s.AspNetCompiler.VirtualPath = \"/%s\"\n", prj_get_cfgname(), prj_get_pkgname());
				io_print("\t\t%s.AspNetCompiler.PhysicalPath = \"%s\\\"\n", prj_get_cfgname(), path);
				io_print("\t\t%s.AspNetCompiler.TargetPath = \"PrecompiledWeb\\%s\\\"\n", prj_get_cfgname(), prj_get_pkgname());
				io_print("\t\t%s.AspNetCompiler.Updateable = \"true\"\n", prj_get_cfgname());
				io_print("\t\t%s.AspNetCompiler.ForceOverwrite = \"true\"\n", prj_get_cfgname());
				io_print("\t\t%s.AspNetCompiler.FixedNames = \"false\"\n", prj_get_cfgname());
				io_print("\t\t%s.AspNetCompiler.Debug = \"%s\"\n", prj_get_cfgname(), prj_has_flag("no-symbols") ? "False" : "True");
			}

			if (numAspNet == 0)
				port = 1106;
			else if (numAspNet == 1)
				port = 1231;
			else
				port = 1251 + 2 * (numAspNet - 2);
			io_print("\t\tVWDPort = \"%d\"\n", port);
			numAspNet++;

			if (prj_is_lang("c#"))
				io_print("\t\tDefaultWebSiteLanguage = \"Visual C#\"\n");

			io_print("\tEndProjectSection\n");
		}
		else
		{
			io_print("Project(\"{%s}\") = \"%s\", \"%s\", \"{%s}\"\n", data->toolGuid, prj_get_pkgname(), prj_get_pkgfilename(data->projExt), data->projGuid);
	
			/* Write dependencies */
			prj_select_config(0);
			io_print("\tProjectSection(ProjectDependencies) = postProject\n");
			print_list(prj_get_links(), "\t\t", "\n", "", vs_list_pkgdeps);
			io_print("\tEndProjectSection\n");
		}

		io_print("EndProject\n");
	}

	/* List configurations */
	io_print("Global\n");
	io_print("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n");

	hasDotNet = 0;
	hasCpp = 0;
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);
		if (prj_is_lang("c") || prj_is_lang("c++"))
			hasCpp = 1;
		else
			hasDotNet = 1;
	}

	prj_select_package(0);
	for (i = 0; i < prj_get_numconfigs(); ++i)
	{
		prj_select_config(i);
		if (hasDotNet)
			io_print("\t\t%s|Any CPU = %s|Any CPU\n", prj_get_cfgname(), prj_get_cfgname());
		if (hasDotNet && hasCpp)
			io_print("\t\t%s|Mixed Platforms = %s|Mixed Platforms\n", prj_get_cfgname(), prj_get_cfgname());
		if (hasCpp)
			io_print("\t\t%s|Win32 = %s|Win32\n", prj_get_cfgname(), prj_get_cfgname());
	}
	io_print("\tEndGlobalSection\n");

	/* Write configuration for each package */
	io_print("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);
		for (j = 0; j < prj_get_numconfigs(); ++j)
		{
			const char* arch;

			prj_select_config(j);
			data = (VsPkgData*)prj_get_data();

			if (prj_is_lang("c") || prj_is_lang("c++"))
				arch = "Win32";
			else
				arch = "Any CPU";

			if (hasDotNet)	
			{
				io_print("\t\t{%s}.%s|Any CPU.ActiveCfg = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
				if (!prj_is_lang("c") && !prj_is_lang("c++"))
					io_print("\t\t{%s}.%s|Any CPU.Build.0 = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
			}

			if (hasDotNet && hasCpp)	
			{
				io_print("\t\t{%s}.%s|Mixed Platforms.ActiveCfg = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
				io_print("\t\t{%s}.%s|Mixed Platforms.Build.0 = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
			}

			if (hasCpp)	
			{
				io_print("\t\t{%s}.%s|Win32.ActiveCfg = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
				if (prj_is_lang("c") || prj_is_lang("c++"))
					io_print("\t\t{%s}.%s|Win32.Build.0 = %s|%s\n", data->projGuid, prj_get_cfgname(), prj_get_cfgname(), arch);
			}
		}
	}
	io_print("\tEndGlobalSection\n");

	/* Finish */
	io_print("\tGlobalSection(SolutionProperties) = preSolution\n");
	io_print("\t\tHideSolutionNode = FALSE\n");
	io_print("\tEndGlobalSection\n");
	io_print("EndGlobal\n");

	io_closefile();
	return 1;
}



static const char* list_aspnet_refs(const char* name)
{
	int i = prj_find_package(name);
	if (i >= 0)
	{
		VsPkgData* data = (VsPkgData*)prj_get_data_for(i);
		sprintf(g_buffer, "{%s}|%s.dll", data->projGuid, prj_get_pkgname_for(i));
		return g_buffer;
	}
	return NULL;
}
