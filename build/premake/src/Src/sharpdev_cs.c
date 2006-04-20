/**********************************************************************
 * Premake - sharpdev.c
 * The SharpDevelop and MonoDevelop C# target
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
#include "sharpdev.h"
#include "os.h"

static char buffer[8192];

static void listFiles(const char* path, int stage);
static void printFile(const char* file);
static const char* listReferences(const char* name);


int sharpdev_cs()
{
	const char* kind;
	const char* runtime;
	const char* csc;
	int i;

	/* Figure out what I'm building */
	if (prj_is_kind("winexe"))
		kind = "WinExe";
	else if (prj_is_kind("exe"))
		kind = "Exe";
	else if (prj_is_kind("dll") || prj_is_kind("aspnet"))
		kind = "Library";
	else
	{
		printf("** Error: unknown package kind '%s'\n", prj_get_kind());
		return 0;
	}

	/* Figure out what .NET environment I'm using */
	if (g_dotnet == NULL)
		g_dotnet = (os_is("windows") || sharpdev_target == MONODEV) ? "ms" : "mono";
	
	if (strcmp(g_dotnet, "ms") == 0)
	{
		runtime = "MsNet";
		csc = "Csc";
	}
	else if (strcmp(g_dotnet, "mono") == 0)
	{
		runtime = "Mono";
		csc = "Mcs";
	}
	else if (strcmp(g_dotnet, "pnet") == 0)
	{
		printf("** Error: SharpDevelop does not yet support Portable.NET\n");
		return 0;
	}
	else
	{
		printf("** Error: unknown .NET runtime '%s'\n", g_dotnet);
		return 0;
	}

	/* Write the file */
	if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "prjx")))
		return 0;

	prj_select_config(0);

	/* Project Header */
	if (sharpdev_target == SHARPDEV)
		io_print("<Project name=\"%s\" standardNamespace=\"%s\" description=\"\" newfilesearch=\"None\" enableviewstate=\"True\" version=\"1.1\" projecttype=\"C#\">\n", prj_get_pkgname(), prj_get_pkgname());
	else
		io_print("<Project name=\"%s\" description=\"\" newfilesearch=\"None\" enableviewstate=\"True\" version=\"1.1\" projecttype=\"C#\">\n", prj_get_pkgname(), prj_get_pkgname());

	/* File List */
	io_print("  <Contents>\n");
	print_source_tree("", listFiles);
	io_print("  </Contents>\n");

	/* References - all configuration will use the same set */
	io_print("  <References>\n");
	print_list(prj_get_links(), "    <Reference", " />\n", "", listReferences);
	io_print("  </References>\n");

	io_print("  <DeploymentInformation target=\"\" script=\"\" strategy=\"File\" />\n");
	  
	 /* Configurations */
	io_print("  <Configurations active=\"%s\">\n", prj_get_cfgname());

	for (i = 0; i < prj_get_numconfigs(); ++i)
	{
		int optimized;

		prj_select_config(i);

		optimized = prj_has_flag("optimize") || prj_has_flag("optimize-size") || prj_has_flag("optimize-speed");

		io_print("    <Configuration runwithwarnings=\"%s\" name=\"%s\">\n", prj_has_flag("fatal-warnings") ? "False" : "True", prj_get_cfgname());
		io_print("      <CodeGeneration runtime=\"%s\" compiler=\"%s\" compilerversion=\"\" ", runtime, csc);
		io_print("warninglevel=\"4\" nowarn=\"\" ");  /* C# defaults to highest warning level */
		io_print("includedebuginformation=\"%s\" ", prj_has_flag("no-symbols") ? "False" : "True"); 
		io_print("optimize=\"%s\" ", optimized ? "True" : "False");
		io_print("unsafecodeallowed=\"%s\" ", prj_has_flag("unsafe") ? "True" : "False");
		io_print("generateoverflowchecks=\"%s\" ", optimized ? "False" : "True");
		io_print("mainclass=\"\" ");
		io_print("target=\"%s\" ", kind); 
		io_print("definesymbols=\"");
		print_list(prj_get_defines(), "", "", ";", NULL);
		io_print("\" ");
		io_print("generatexmldocumentation=\"False\" ");
		io_print("win32Icon=\"\" noconfig=\"False\" nostdlib=\"False\" ");
		io_print("/>\n");

		io_print("      <Execution commandlineparameters=\"\" consolepause=\"True\" />\n");

		io_print("      <Output directory=\"%s\" ", prj_get_outdir());
		prj_select_config(0);
		io_print("assembly=\"%s\" ", path_getbasename(prj_get_target()));
		io_print("executeScript=\"\" ");
		io_print("executeBeforeBuild=\"\" ");
		io_print("executeAfterBuild=\"\" ");
		io_print("executeBeforeBuildArguments=\"\" ");
		io_print("executeAfterBuildArguments=\"\" ");
		io_print("/>\n");		

		io_print("    </Configuration>\n");
	}

	io_print("  </Configurations>\n");
	io_print("</Project>\n");

	io_closefile();
	return 1;
}


/************************************************************************
 * Callback for print_source_tree()
 ***********************************************************************/

static void listFiles(const char* path, int stage)
{
	switch (stage)
	{
	case WST_OPENGROUP:
		if (strlen(path) > 0)
		{
			io_print("    <File name=\"");
			if (strncmp(path, "..", 2) != 0)
				io_print("./");
			io_print("%s\"", path);
			io_print(" subtype=\"Directory\" buildaction=\"Compile\" dependson=\"\" data=\"\" />\n");
		}
		break;

	case WST_CLOSEGROUP:
		break;

	case WST_SOURCEFILE:
		printFile(path);
		break;
	}
}

static void printFile(const char* file)
{
	const char* ext;
	const char* prefix  = "";
	const char* subtype = "";
	const char* action  = "Nothing";
	const char* depends = "";

	if (file[0] != '.')
		prefix = "./";

	ext = path_getextension(file);
	
	/* If a build action was specified, use it */
	prj_select_file(file);
	if (prj_get_buildaction() != NULL)
	{
		action = prj_get_buildaction();
		if (matches(action, "EmbeddedResource"))
			action = "EmbedAsResource";
		if (matches(action, "None"))
			action = "Nothing";
		if (matches(action, "Content"))
		{
			puts("** Warning: SharpDevelop does not support the 'Content' build action");
			action = "Nothing";
		}
		subtype = "Code";
	}
	else if (matches(ext, ".cs"))
	{
		subtype = "Code";
		action = "Compile";
	}
	else if (matches(ext, ".resx"))
	{
		/* If a matching .cs file exists, link it */
		strcpy(buffer, file);
		strcpy(buffer + strlen(file) - 5, ".cs");
		if (prj_has_file(buffer))
		{
			/* Path is relative to .resx file, I assume both are in same
			 * directory and cut off path information */
			depends = path_getname(buffer);
		}

		subtype = "Code";
		action = "EmbedAsResource";
	}
	else
	{
		prj_select_file(file);
		subtype = "Code";
		action = prj_get_buildaction();
		if (action == NULL || matches(action, "Content"))
		{
			sharpdev_warncontent = 1;
			action = "Nothing";
		}
	}

	io_print("    <File name=\"%s%s\" subtype=\"%s\" buildaction=\"%s\" dependson=\"%s\" data=\"\" />\n", prefix, file, subtype, action, depends);
}



/************************************************************************
 * Prints entry for each reference listed in the package
 ***********************************************************************/

static const char* listReferences(const char* name)
{
	int i;
	int isSibling = 0;
	const char* fileName = path_getname(name);

	strcpy(buffer," type=\"");

	/* A bit of craziness here...#dev wants to know if an assembly is local
	 * (type == "Assembly") or in the GAC (type == "GAC"). I would prefer
	 * to not have to specify this in the premake script if I can get away
	 * with it. So for each reference I check to see if it is a sibling
	 * project, and if so I consider it local. If not, I check all of the
	 * reference paths to see if I can find the DLL and if so I consider
	 * it local. If not, I consider it in the GAC. Seems to work so far */

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		if (matches(prj_get_pkgname_for(i), name))
		{
			isSibling = 1;
			break;
		}
	}
		
	if(isSibling)
	{
		strcat(buffer, "Project\"");
		strcat(buffer, " refto=\"");
		strcat(buffer, fileName);
		strcat(buffer, "\" localcopy=\"True\"");
	}
	else
	{
		const char* ext = strchr(name,',') ? "" : ".dll";

		/* See if this assembly exists on one of the link paths */
		if (io_fileexists(path_join(prj_get_bindir(), name, ext)))
		{
			strcat(buffer, "Assembly\" refto=\"");
			strcat(buffer, path_build(prj_get_pkgpath(), prj_get_bindir()));
			strcat(buffer, "/");
			strcat(buffer, name);
			strcat(buffer, ".dll\" localcopy=\"False\"");
		}
		else
		{
			strcat(buffer, "Gac\" refto=\"");
			strcat(buffer, name);
			strcat(buffer, ext);
			strcat(buffer, "\" localcopy=\"False\"");
		}
	}

	return buffer;
}

