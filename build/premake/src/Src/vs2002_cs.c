/**********************************************************************
 * Premake - vs2002_cs.c
 * The Visual Studio 2002 and 2003 C# target
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
#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "vs.h"
#include "vs2002.h"

static const char* listReferences(const char* name);
static const char* listFiles(const char* name);


int vs2002_cs()
{
	const char* outputType;
	int i;

	VsPkgData* data = (VsPkgData*)prj_get_data();

	/* Figure out what I'm building */
	prj_select_config(0);
	if (prj_is_kind("winexe"))
		outputType = "WinExe";
	else if (prj_is_kind("exe"))
		outputType = "Exe";
	else if (prj_is_kind("dll") || prj_is_kind("aspnet"))
		outputType = "Library";
	else
	{
		printf("** Error: unknown package kind '%s'\n", prj_get_kind());
		return 0;
	}

	/* Open the file and write the header */
	if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj")))
		return 0;

	io_print("<VisualStudioProject>\n");
	io_print("\t<CSHARP\n");
	io_print("\t\tProjectType = \"%s\"\n", prj_is_kind("aspnet") ? "Web" : "Local");

	if (vs_getversion() == VS2003)
	{
		io_print("\t\tProductVersion = \"7.10.3077\"\n");
		io_print("\t\tSchemaVersion = \"2.0\"\n");
	}
	else
	{
		io_print("\t\tProductVersion = \"7.0.9254\"\n");
		io_print("\t\tSchemaVersion = \"1.0\"\n");
	}

	io_print("\t\tProjectGuid = \"{%s}\"\n", data->projGuid);
	io_print("\t>\n");
	io_print("\t\t<Build>\n");
	io_print("\t\t\t<Settings\n");
	io_print("\t\t\t\tApplicationIcon = \"\"\n");
	io_print("\t\t\t\tAssemblyKeyContainerName = \"\"\n");
	io_print("\t\t\t\tAssemblyName = \"%s\"\n", path_getbasename(prj_get_target()));
	io_print("\t\t\t\tAssemblyOriginatorKeyFile = \"\"\n");
	io_print("\t\t\t\tDefaultClientScript = \"JScript\"\n");
	io_print("\t\t\t\tDefaultHTMLPageLayout = \"Grid\"\n");
	io_print("\t\t\t\tDefaultTargetSchema = \"IE50\"\n");
	io_print("\t\t\t\tDelaySign = \"false\"\n");
	if (vs_getversion() == VS2002) 
	{
		io_print("\t\t\t\tNoStandardLibraries = \"false\"\n");
	}
	io_print("\t\t\t\tOutputType = \"%s\"\n", outputType);
	if (vs_getversion() == VS2003)
	{
		io_print("\t\t\t\tPreBuildEvent = \"\"\n");
		io_print("\t\t\t\tPostBuildEvent = \"\"\n");
	}
	io_print("\t\t\t\tRootNamespace = \"%s\"\n", path_getbasename(prj_get_target()));
	if (vs_getversion() == VS2003)
	{		
		io_print("\t\t\t\tRunPostBuildEvent = \"OnBuildSuccess\"\n");
	}

	io_print("\t\t\t\tStartupObject = \"\"\n");
	io_print("\t\t\t>\n");

	for (i = 0; i < prj_get_numconfigs(); ++i)
	{
		int optimize;

		prj_select_config(i);

		optimize = prj_has_flag("optimize") || prj_has_flag("optimize-size") || prj_has_flag("optimize-speed");

		io_print("\t\t\t\t<Config\n");
		io_print("\t\t\t\t\tName = \"%s\"\n", prj_get_cfgname());
		io_print("\t\t\t\t\tAllowUnsafeBlocks = \"%s\"\n", prj_has_flag("unsafe") ? "true" : "false");
		io_print("\t\t\t\t\tBaseAddress = \"285212672\"\n");
		io_print("\t\t\t\t\tCheckForOverflowUnderflow = \"false\"\n");
		io_print("\t\t\t\t\tConfigurationOverrideFile = \"\"\n");

		io_print("\t\t\t\t\tDefineConstants = \"");
		print_list(prj_get_defines(), "", "", ";", NULL);
		io_print("\"\n");

		io_print("\t\t\t\t\tDocumentationFile = \"\"\n");
		io_print("\t\t\t\t\tDebugSymbols = \"%s\"\n", prj_has_flag("no-symbols") ? "false" : "true");
		io_print("\t\t\t\t\tFileAlignment = \"4096\"\n");
		io_print("\t\t\t\t\tIncrementalBuild = \"false\"\n");
		if (vs_getversion() == VS2003)
		{
			io_print("\t\t\t\t\tNoStdLib = \"false\"\n");
			io_print("\t\t\t\t\tNoWarn = \"\"\n");
		}
		io_print("\t\t\t\t\tOptimize = \"%s\"\n", optimize ? "true" : "false");
		io_print("\t\t\t\t\tOutputPath = \"%s\"\n", prj_get_outdir());
		io_print("\t\t\t\t\tRegisterForComInterop = \"false\"\n");
		io_print("\t\t\t\t\tRemoveIntegerChecks = \"false\"\n");
		io_print("\t\t\t\t\tTreatWarningsAsErrors = \"%s\"\n", prj_has_flag("fatal-warnings") ? "true" : "false");
		io_print("\t\t\t\t\tWarningLevel = \"4\"\n");  /* C# defaults to highest warning level */
		io_print("\t\t\t\t/>\n");
	}

	io_print("\t\t\t</Settings>\n");

	/* VS7 requires same references for all configurations */
	prj_select_config(0);
	io_print("\t\t\t<References>\n");
	print_list(prj_get_links(), "\t\t\t\t<Reference\n", "\t\t\t\t/>\n", "", listReferences);
	io_print("\t\t\t</References>\n");
	io_print("\t\t</Build>\n");

	io_print("\t\t<Files>\n");
	io_print("\t\t\t<Include>\n");
	print_list(prj_get_files(), "\t\t\t\t<File\n\t\t\t\t\tRelPath = \"", "\t\t\t\t/>\n", "", listFiles);
	io_print("\t\t\t</Include>\n");
	io_print("\t\t</Files>\n");
	io_print("\t</CSHARP>\n");
	io_print("</VisualStudioProject>\n");

	io_closefile();

	/* Now write the .csproj.user file for non-web applications or
	 * .csproj.webinfo for web applications */
	if (!prj_is_kind("aspnet"))
	{
		if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj.user")))
			return 0;


		io_print("<VisualStudioProject>\n");
		io_print("\t<CSHARP>\n");
		io_print("\t\t<Build>\n");
		io_print("\t\t\t<Settings ReferencePath = \"");

		strcpy(g_buffer, io_getcwd());
		io_chdir(prj_get_pkgpath());
		print_list(prj_get_libpaths(), "", ";", "", vs_list_refpaths);
		io_print(vs_list_refpaths(prj_get_bindir()));
		io_chdir(g_buffer);

		io_print("\" >\n");


		for (i = 0; i < prj_get_numconfigs(); ++i)
		{
			prj_select_config(i);

			io_print("\t\t\t\t<Config\n");
			io_print("\t\t\t\t\tName = \"%s\"\n", prj_get_cfgname());
			io_print("\t\t\t\t\tEnableASPDebugging = \"false\"\n");
			io_print("\t\t\t\t\tEnableASPXDebugging = \"false\"\n");
			io_print("\t\t\t\t\tEnableUnmanagedDebugging = \"false\"\n");
			io_print("\t\t\t\t\tEnableSQLServerDebugging = \"false\"\n");
			io_print("\t\t\t\t\tRemoteDebugEnabled = \"false\"\n");
			io_print("\t\t\t\t\tRemoteDebugMachine = \"\"\n");
			io_print("\t\t\t\t\tStartAction = \"Project\"\n");
			io_print("\t\t\t\t\tStartArguments = \"\"\n");
			io_print("\t\t\t\t\tStartPage = \"\"\n");
			io_print("\t\t\t\t\tStartProgram = \"\"\n");
			io_print("\t\t\t\t\tStartURL = \"\"\n");
			io_print("\t\t\t\t\tStartWorkingDirectory = \"\"\n");
			io_print("\t\t\t\t\tStartWithIE = \"false\"\n");
			io_print("\t\t\t\t/>\n");
		}

		io_print("\t\t\t</Settings>\n");
		io_print("\t\t</Build>\n");
		io_print("\t\t<OtherProjectSettings\n");
		io_print("\t\t\tCopyProjectDestinationFolder = \"\"\n");
		io_print("\t\t\tCopyProjectUncPath = \"\"\n");
		io_print("\t\t\tCopyProjectOption = \"0\"\n");
		io_print("\t\t\tProjectView = \"ProjectFiles\"\n");
		io_print("\t\t\tProjectTrust = \"0\"\n");
		io_print("\t\t/>\n");
		io_print("\t</CSHARP>\n");
		io_print("</VisualStudioProject>\n");
	}
	else
	{
		if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj.webinfo")))
			return 0;

		io_print("<VisualStudioUNCWeb>\n");
		io_print("\t<Web URLPath = \"");
		if (prj_get_url() == NULL)
			io_print("http://localhost/%s", prj_get_pkgname());
		else
			io_print(prj_get_url());
		io_print("/%s.csproj\" />\n", prj_get_pkgname());
		io_print("</VisualStudioUNCWeb>\n");
	}

	io_closefile();
	return 1;
}



/************************************************************************
 * Checks each entry in the list of package links. If the entry refers
 * to a sibling package, returns the path to that package's output
 ***********************************************************************/

static const char* listReferences(const char* name)
{
	char assembly[8192];
	char* comma;
	int i;

	/* Pull out the file name, the comma check is for full assembly names */
	strcpy(assembly, name);
	comma = strchr(name, ',');
	if (comma != NULL)
		*comma = '\0';

	strcpy(g_buffer, "\t\t\t\t\tName = \"");
	strcat(g_buffer, assembly);
	strcat(g_buffer, "\"\n");

	/* Is this a sibling package? */
	i = prj_find_package(name);
	if (i >= 0)
	{
		VsPkgData* data = (VsPkgData*)prj_get_data_for(i);
		strcat(g_buffer, "\t\t\t\t\tProject = \"{");
		strcat(g_buffer, data->projGuid);
		strcat(g_buffer, "}\"\n");
		strcat(g_buffer, "\t\t\t\t\tPackage = \"{");
		strcat(g_buffer, data->toolGuid);
		strcat(g_buffer, "}\"\n");
		return g_buffer;
	}

	strcat(g_buffer, "\t\t\t\t\tAssemblyName = \"");
	strcat(g_buffer, path_getname(assembly));
	strcat(g_buffer, "\"\n");
	if (!matches(assembly, path_getname(assembly)))
	{
		strcat(g_buffer, "\t\t\t\t\tHintPath = \"");
		strcat(g_buffer, assembly);
		strcat(g_buffer, ".dll\"\n");
	}

	/* Tack on any extra information about the assembly */
	while (comma != NULL)
	{
		char* start;
		for (start = comma + 1; *start == ' '; ++start);
		comma = strchr(start, '=');
		*comma = '\0';
		strcat(g_buffer, "\t\t\t\t\t");
		strcat(g_buffer, start);
		strcat(g_buffer, " = \"");

		start = comma + 1;
		comma = strchr(start, ',');
		if (comma != NULL) *comma = '\0';
		strcat(g_buffer, start);
		strcat(g_buffer, "\"\n");
	}

	return g_buffer;
}


/************************************************************************
 * Builds an entry for each file in the project
 ***********************************************************************/

static const char* listFiles(const char* name)
{
	io_print("\t\t\t\t<File\n\t\t\t\t\tRelPath = \"%s\"\n", path_translate(name, "windows"));

	/* If a build action was specified, use it */
	prj_select_file(name);
	if (prj_get_buildaction() != NULL)
	{
		io_print("\t\t\t\t\tBuildAction = \"%s\"\n", prj_get_buildaction());
	}
	else if (endsWith(name, ".aspx.cs") || endsWith(name, ".asax.cs"))
	{
		/* The path to the parent .aspx file is relative to the .cs file. 
		 * I assume that they are in the same directory */
		io_print("\t\t\t\t\tDependentUpon = \"%s\"\n", path_getbasename(name));
		io_print("\t\t\t\t\tSubType = \"ASPXCodeBehind\"\n");
		io_print("\t\t\t\t\tBuildAction = \"Compile\"\n");
	}
	else if (endsWith(name, ".cs"))
	{
		io_print("\t\t\t\t\tSubType = \"Code\"\n");
		io_print("\t\t\t\t\tBuildAction = \"Compile\"\n");
	}
	else if (endsWith(name, ".aspx"))
	{
		io_print("\t\t\t\t\tSubType = \"Form\"\n");
		io_print("\t\t\t\t\tBuildAction = \"Content\"\n");
	}
	else if (endsWith(name, ".asax"))
	{
		io_print("\t\t\t\t\tSubType = \"Component\"\n");
		io_print("\t\t\t\t\tBuildAction = \"Content\"\n");
	}
	else if (endsWith(name, ".resx"))
	{
		/* If a matching .cs file exists, link it */
		strcpy(g_buffer, path_swapextension(name, ".resx", ".cs"));
		if (prj_has_file(g_buffer))
		{
			/* Path is relative to .resx file, I assume both are in same dir */
			io_print("\t\t\t\t\tDependentUpon = \"%s\"\n", path_getname(g_buffer));
		}
		io_print("\t\t\t\t\tBuildAction = \"EmbeddedResource\"\n");
	}
	else
	{
		io_print("\t\t\t\t\tBuildAction = \"None\"\n");
	}

	io_print("\t\t\t\t/>\n");
	return NULL;
}
