//-----------------------------------------------------------------------------
// Premake - vs6.c
//
// MS Visual Studio 6 tool target.
//
// Copyright (C) 2002-2004 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: vs6.c,v 1.20 2004/05/07 00:12:29 jason379 Exp $
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "util.h"

extern const char* dotnet;

static int   writeWorkspace();
static int   writeCsProject(Package* package);
static int   writeVcProject(Package* package);
static int   writeProjectHeader(FILE* file, Package* package);

#if VS6_DOTNET
static char* csc;
#endif

//-----------------------------------------------------------------------------

int makeVs6Scripts()
{
	int i, csharp = 0;

	puts("Generating Visual Studio 6 workspace and project files:");

#if VS6_DOTNET
	if (dotnet == NULL)
		csc = "csc.exe";
	else if (strcmp(dotnet, "mono") == 0)
		csc = "mcs.exe";
	else if (strcmp(dotnet, "pnet") == 0)
		csc = "cscc.exe";
	else
		csc = "csc.exe";
#endif

	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		const char* language = package->language;
		printf("...%s\n", package->name);

		if (strcmp(language, "c++") == 0 || strcmp(language, "c") == 0)
		{
			writeVcProject(package);
		}
		else if (strcmp(language, "c#") == 0)
		{
#if VS6_DOTNET
			csharp = 1;
			writeCsProject(package);
#else
			printf("** Error: C# projects are not supported by Visual Studio 6\n");
			return 0;
#endif
		}
		else
		{
			printf("** Error: unrecognized language '%s'\n", language);
			return 0;
		}
	}

#if VS6_DOTNET
	if (csharp && getenv("DOTNET_PATH") == NULL)
	{
		printf("\nBEFORE USING THESE PROJECT FILES you must set the environment variable\n");
		printf("DOTNET_PATH to the directory containing %s.\n\n", csc);
		
		if (strcmp(csc, "csc.exe") == 0)
			printf("This will be something like C:\\WinNT\\Microsoft.NET\\Framework\\v1.0.3705.\n\n");
	}
#endif

	return writeWorkspace();
}

//-----------------------------------------------------------------------------

static int writeWorkspace()
{
	int i, j, k;

	FILE* file = openFile(project->path, project->name, ".dsw");
	if (file == NULL)
		return 0;

	fprintf(file, "Microsoft Developer Studio Workspace File, Format Version 6.00\n");
	fprintf(file, "# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!\n");
	fprintf(file, "\n");

	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];

		fprintf(file, "###############################################################################\n");
		fprintf(file, "\n");
		fprintf(file, "Project: \"%s\"=%s%s.dsp\n", replaceChars(package->name,"-"), reversePath(project->path,package->path,WIN32), package->name);
		fprintf(file, "\n");
		fprintf(file, "Package=<5>\n");
		fprintf(file, "{{{\n");
		fprintf(file, "}}}\n");
		fprintf(file, "\n");
		fprintf(file, "Package=<4>\n");
		fprintf(file, "{{{\n");

		// List package dependencies on sibling projects
		for (j = 0; j < package->config[0]->numLinks; ++j)
		{
			const char* link = package->config[0]->links[j];
			for (k = 0; k < project->numPackages; k++)
			{
				if (strcmp(project->package[k]->name, link) == 0)
				{
					fprintf(file, "    Begin Project Dependency\n");
					fprintf(file, "    Project_Dep_Name %s\n", replaceChars(link, "-"));
					fprintf(file, "    End Project Dependency\n");
				}
			}
		}

		fprintf(file, "}}}\n");
		fprintf(file, "\n");
	}

	fprintf(file, "###############################################################################\n");
	fprintf(file, "\n");
	fprintf(file, "Global:\n");
	fprintf(file, "\n");
	fprintf(file, "Package=<5>\n");
	fprintf(file, "{{{\n");
	fprintf(file, "}}}\n");
	fprintf(file, "\n");
	fprintf(file, "Package=<3>\n");
	fprintf(file, "{{{\n");
	fprintf(file, "}}}\n");
	fprintf(file, "\n");
	fprintf(file, "###############################################################################\n");
	fprintf(file, "\n");

	fclose(file);
	return 1;
}

//-----------------------------------------------------------------------------

static void vcFiles(FILE* file, const char* path, int stage)
{
	const char* ptr = strrchr(path, '/');
	int i,j;
	ptr = (ptr == NULL) ? path : ptr + 1;

	switch (stage)
	{
	case WST_OPENGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0) {
			fprintf(file, "# Begin Group \"%s\"\n\n", ptr);
			fprintf(file, "# PROP Default_Filter \"\"\n");
		}
		break;
	case WST_CLOSEGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0) 
			fprintf(file, "# End Group\n");
		break;
	case WST_SOURCEFILE:
		fprintf(file, "# Begin Source File\n\n");
		fprintf(file, "SOURCE=.\\%s\n", translatePath(path,WIN32));
		for (i=0;i<project->numPackages;i++)
		{
			Package *package=project->package[i];
			for (j=0;j<package->numConfigs;j++)
			{
				Config *config=package->config[j];
				
				fprintf(file, "!%s  \"$(CFG)\" == \"%s - Win32 %s\"\n", (i == 0 ? "IF" : "ELSEIF"), replaceChars(package->name,"-"), config->name);
				fprintf(file, "\n");

				if (config->pchSource && strcmp(ptr, config->pchSource)==0)
				{
					fprintf(file, "# ADD CPP /Yc\"%s\"\n", config->pchHeader);
				}
			}
			fprintf(file, "!ENDIF\n");
		}
		fprintf(file, "# End Source File\n");
		break;
	}
}

static const char* checkLink(const char* path, void* data)
{
	Package* package = getPackage(path);
	if (package == NULL) return path;

	if (strcmp(package->language, "c++") == 0)
		return package->config[*((int*)data)]->target;
	
	return NULL;
}

static int writeVcProject(Package* package)
{
	const char* ldflags;
	const char* extension;
	const char* outdir;
	FILE* file;
	int i;

	// Some target type specific info...
	if (strcmp(package->kind, "winexe") == 0)
	{
		ldflags = "/subsystem:windows";
		extension = "exe";
		outdir = project->binaries;
	}
	else if (strcmp(package->kind, "exe") == 0)
	{
		ldflags = "/subsystem:console";
		extension = "exe";
		outdir = project->binaries;
	}
	else if (strcmp(package->kind, "dll") == 0)
	{
		ldflags = "/dll";
		extension = "dll";
		outdir = project->binaries;
	}
	else if (strcmp(package->kind, "lib") == 0)
	{
		ldflags = 
		extension = "lib";
		outdir = project->libraries;
	}
	else
	{
		printf("** Error: unknown package type '%s'\n", package->kind);
		return 0;
	}

	// Open the file and write the header
	file = openFile(package->path, package->name, ".dsp");
	if (file == NULL)
		return 0;
	writeProjectHeader(file, package);

	fprintf(file, "CPP=cl.exe\n");
	fprintf(file, "MTL=midl.exe\n");
	fprintf(file, "RSC=rc.exe\n");
	fprintf(file, "\n");

	for (i = 0; i < package->numConfigs; ++i)
	{
		char* runtime;

		Config* config = package->config[i];

		int size      =  inArray(config->buildFlags, "optimize-size");
		int speed     =  inArray(config->buildFlags, "optimize-speed") || inArray(config->buildFlags, "optimize");
		int debug     = !size && !speed;
		int importlib = !inArray(config->buildFlags, "no-import-lib");
		int noMain    =  inArray(config->buildFlags, "no-main");
		int exceptions = !inArray(config->buildFlags, "no-exceptions");

		int rtti       = !inArray(config->buildFlags, "no-rtti");

		if (inArray(config->linkFlags, "static-runtime") || inArray(config->buildFlags, "static-runtime"))
			runtime = (debug) ? "/MTd" : "/MT";
		else
			runtime = (debug) ? "/MDd" : "/MD";

		fprintf(file, "!%s  \"$(CFG)\" == \"%s - Win32 %s\"\n", (i == 0 ? "IF" : "ELSEIF"), replaceChars(package->name,"-"), config->name);
		fprintf(file, "\n");
		fprintf(file, "# PROP BASE Use_MFC 0\n");
		fprintf(file, "# PROP BASE Use_Debug_Libraries %d\n", debug ? 1 : 0);
		fprintf(file, "# PROP BASE Output_Dir \"%s\\\"\n", debug ? "Debug" : "Release");
		fprintf(file, "# PROP BASE Intermediate_Dir \"%s\\\"\n", debug ? "Debug" : "Release");
		fprintf(file, "# PROP BASE Target_Dir \"\"\n");
		fprintf(file, "# PROP Use_MFC 0\n");
		fprintf(file, "# PROP Use_Debug_Libraries %d\n", debug ? 1 : 0);
		fprintf(file, "# PROP Output_Dir \"");
			fprintf(file, reversePath(package->path, outdir, WIN32));
			insertPath(file, getDirectory(config->target), WIN32);
			fprintf(file, "\"\n");
		fprintf(file, "# PROP Intermediate_Dir \"%s\\\"\n", config->objdir);
		if (strcmp(package->kind, "lib") != 0)
			fprintf(file, "# PROP Ignore_Export_Lib %d\n", importlib ? 0 : 1);
		fprintf(file, "# PROP Target_Dir \"\"\n");

		fprintf(file, "# ADD BASE CPP /nologo /W3");
		if (debug)  
			fprintf(file, " /Gm /GX /ZI /Od /D \"_DEBUG\" /YX /FD /GZ /c\n");
		else
			fprintf(file, " /GX /O2 /D \"NDEBUG\" /YX /FD /c\n");
			
		fprintf(file, "# ADD CPP /nologo %s /W3", runtime);
		if (debug) fprintf(file, " /Gm /ZI /Od");

		if (exceptions) fprintf(file, " /GX");

		if (rtti) fprintf(file, " /GR");

		if (size) fprintf(file, " /O1");
		if (speed) fprintf(file, " /O2");
		writeList(file, config->defines, " /D \"", "\"", "", NULL, NULL);
		writeList(file, config->includePaths, " /I \"", "\"", "", NULL, NULL);
		writeList(file, config->buildOptions, "", "", " ", NULL, NULL);

		if (config->pchHeader)
			fprintf(file, " /Yu\"%s\"", config->pchHeader);
		else
			fprintf(file, " /YX");

		if (debug)
			fprintf(file, " /FD /GZ /c\n");
		else
			fprintf(file, " /FD /c\n");

		if (strcmp(package->kind, "lib") != 0)
		{
			fprintf(file, "# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n");
			fprintf(file, "# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n");
		}

		fprintf(file, "# ADD BASE RSC /l 0x409 /d \"NDEBUG\"\n");
		fprintf(file, "# ADD RSC /l 0x409 /d \"NDEBUG\"\n");
		fprintf(file, "BSC32=bscmake.exe\n");
		fprintf(file, "# ADD BASE BSC32 /nologo\n");
		fprintf(file, "# ADD BSC32 /nologo\n");
		
		if (strcmp(package->kind, "lib") != 0)
		{
			fprintf(file, "LINK32=link.exe\n");
			fprintf(file, "# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo %s /machine:I386 %s\n", 
				ldflags, debug ? "/debug /pdbtype:sept" : "");

			fprintf(file, "# ADD LINK32");
				writeList(file, config->links, " ", ".lib", "", checkLink, &i);
				fprintf(file, " /nologo %s", ldflags);
				if (strcmp(package->kind, "winexe") == 0 && !noMain)
					fprintf(file, " /entry:\"mainCRTStartup\"");
				fprintf(file, " /out:\"%s", reversePath(package->path, project->binaries, WIN32));
				fprintf(file, "%s.%s\"", translatePath(config->target,WIN32), extension);
				fprintf(file, " /machine:I386");
				if (strcmp(package->kind, "dll") == 0)
				{
					fprintf(file, " /implib:\"");
					if (importlib)
						fprintf(file, reversePath(package->path, project->libraries, WIN32));
					else
						fprintf(file, "$(IntDir)\\");
					fprintf(file, "%s.lib\"", translatePath(config->target, WIN32));
				}
				fprintf(file, " /pdbtype:sept /libpath:\"%s\"", reversePath(package->path, project->libraries, WIN32));
				if (debug) {
					fprintf(file, " /pdb:\"%s", reversePath(package->path,project->binaries,WIN32));
					fprintf(file, "%s.pdb\" /debug", translatePath(config->target,WIN32));
				}
				writeList(file, config->linkOptions, " ", "", "", NULL, NULL);
				fprintf(file, "\n");
		}
		else
		{
			fprintf(file, "LIB32=link.exe -lib\n");
			fprintf(file, "# ADD BASE LIB32 /nologo\n");
			fprintf(file, "# ADD LIB32 /nologo /out:\"%s", reversePath(package->path, project->libraries, WIN32));
				fprintf(file, "%s.%s\"\n", translatePath(config->target,WIN32), extension);
		}
	}

	fprintf(file, "!ENDIF \n");
	fprintf(file, "\n");
	fprintf(file, "# Begin Target\n");
	fprintf(file, "\n");

	for (i = 0; i < package->numConfigs; ++i)
        fprintf(file, "# Name \"%s - Win32 %s\"\n", replaceChars(package->name,"-"), package->config[i]->name);
	walkSourceList(file, package, "", vcFiles);

	fprintf(file, "# End Target\n");
	fprintf(file, "# End Project\n");

	fclose(file);
	return 1;
}

//-----------------------------------------------------------------------------

#if VS6_DOTNET

static const char* checkRef(const char* path, void* data)
{
	Package* package = getPackage(path);
	if (package == NULL) return path;
	
	if (strcmp(package->language, "c#") == 0)
		return package->config[*((int*)data)]->target;
	
	return NULL;
}

static int writeCsProject(Package* package)
{
	const char* extension;
	FILE* file;
	int i, j;

	const char* kind = package->kind;

	// .NET requires all configurations use the same assembly filespec
	const char* target = package->config[0]->target;

	// Figure out the object type
	if (strcmp(kind, "winexe") == 0)
	{
		extension = "exe";
	}
	else if (strcmp(kind, "exe") == 0)
	{
		extension = "exe";
	}
	else if (strcmp(kind, "dll") == 0)
	{
		extension = "dll";
		kind = "library";
	}
	else
	{
		printf("** Error: unknown package type '%s'\n", kind);
		return 0;
	}

	// Open the project file and write the header

	file = openFile(package->path, package->name, ".dsp");
	if (file == NULL) return 0;
	if (!writeProjectHeader(file, package)) return 0;

	fprintf(file, "MTL=midl.exe\n");
	fprintf(file, "\n");

	for (i = 0; i < package->numConfigs; ++i)
	{
		Config* config = package->config[i];

		int symbols = !inArray(config->buildFlags, "no-symbols");
		int optimize = inArray(config->buildFlags,"optimize") || inArray(config->buildFlags,"optimize-size") && inArray(config->buildFlags,"optimize-speed");
		int unsafe =  inArray(config->buildFlags, "unsafe");

		fprintf(file, "!%s  \"$(CFG)\" == \"%s - Win32 %s\"\n", i == 0 ? "IF" : "ELSEIF", replaceChars(package->name,"-"), config->name);
		fprintf(file, "\n");
		fprintf(file, "# PROP BASE Use_MFC 0\n");
		fprintf(file, "# PROP BASE Use_Debug_Libraries 1\n");
		fprintf(file, "# PROP BASE Output_Dir \"Release\"\n");
		fprintf(file, "# PROP BASE Intermediate_Dir \"Release\"\n");
		fprintf(file, "# PROP BASE Target_Dir \"\"\n");
		fprintf(file, "# PROP Use_MFC 0\n");
		fprintf(file, "# PROP Use_Debug_Libraries 0\n");
		fprintf(file, "# PROP Output_Dir \"");
			fprintf(file, reversePath(package->path, project->binaries, WIN32));
			insertPath(file, getDirectory(config->target), WIN32);
			fprintf(file, "\"\n");
		fprintf(file, "# PROP Intermediate_Dir \"%s\"\n", config->objdir);
		fprintf(file, "# PROP Target_Dir \"\"\n");
		fprintf(file, "# Begin Special Build Tool\n");
		fprintf(file, "SOURCE=\"$(InputPath)\"\n");

		fprintf(file, "PostBuild_Cmds=$(DOTNET_PATH)\\%s /nologo", csc);
			fprintf(file, " /out:\"%s", reversePath(package->path,project->binaries,WIN32));
			fprintf(file, "\\%s.%s\"", translatePath(config->target,WIN32), extension);
			fprintf(file, " /target:%s", kind);
			if (symbols) fprintf(file, " /debug");
			if (optimize) fprintf(file, " /optimize");
			if (unsafe) fprintf(file, " /unsafe");
			fprintf(file, " /lib:\"%s\"", reversePath(package->path, project->binaries, WIN32));
			writeList(file, config->links, " /r:", ".dll", "", checkRef, &i);
			fprintf(file, " %s\\*.cs\n", config->objdir);

		fprintf(file, "# End Special Build Tool\n");
		fprintf(file, "\n");
	}

	fprintf(file, "!ENDIF\n");
	fprintf(file, "\n");
	fprintf(file, "# Begin Target\n");
	fprintf(file, "\n");
	for (i = 0; i < package->numConfigs; ++i)
		fprintf(file, "# Name \"%s - Win32 %s\"\n", replaceChars(package->name,"-"), package->config[i]->name);
	for (i = 0; i < package->numFiles; ++i)
	{
		const char* source = package->files[i];

		fprintf(file, "# Begin Source File\n");
		fprintf(file, "\n");
		fprintf(file, "SOURCE=.\\%s\n", translatePath(source, WIN32));
		fprintf(file, "\n");

		for (j = 0; j < package->numConfigs; ++j)
		{
			Config* config = package->config[j];

			fprintf(file, "!%s  \"$(CFG)\" == \"%s - Win32 %s\"", (i == 0 ? "IF" : "ELSEIF"), replaceChars(package->name,"-"), config->name);
			fprintf(file, "\n");
			fprintf(file, "# Begin Custom Build - %s\n", source);
			fprintf(file, "IntDir=.\\%s\n", config->objdir);
			fprintf(file, "InputPath=.\\%s\n", translatePath(source, WIN32));
			fprintf(file, "\n");
			fprintf(file, "\"$(INTDIR)\\%s\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n", translatePath(source, WIN32));
			fprintf(file, "\tcopy $(InputPath) $(IntDir)\\%s > out\n", translatePath(source, WIN32));
			fprintf(file, "\terase out\n");
			fprintf(file, "\n");
			fprintf(file, "# End Custom Build\n");
			fprintf(file, "\n");
		}

		fprintf(file, "!ENDIF \n");
		fprintf(file, "\n");
		fprintf(file, "# End Source File\n");
	}

	fprintf(file, "# End Target\n");
	fprintf(file, "# End Project\n");
	fprintf(file, "\n");

	fclose(file);
	return 1;
}

#endif

//-----------------------------------------------------------------------------

static int writeProjectHeader(FILE* file, Package* package)
{
	int i;
	const char* tag;
	const char* num;

	const char* name = replaceChars(package->name, "-");

	if (strcmp(package->language, "c#") == 0)
	{
		tag = "Win32 (x86) Generic Project";
		num = "0x010a";
	}
	else
	{
		if (strcmp(package->kind, "winexe") == 0)
		{
			tag = "Win32 (x86) Application";
			num = "0x0101";
		}
		else if (strcmp(package->kind, "exe") == 0)
		{
			tag = "Win32 (x86) Console Application";
			num = "0x0103";
		}
		else if (strcmp(package->kind, "dll") == 0)
		{
			tag = "Win32 (x86) Dynamic-Link Library";
			num = "0x0102";
		}
		else if (strcmp(package->kind, "lib") == 0)
		{
			tag = "Win32 (x86) Static Library";
			num = "0x0104";
		}
		else
		{
			puts("** Error: unrecognized package type");
			return 0;
		}
	}

	fprintf(file, "# Microsoft Developer Studio Project File - Name=\"%s\" - Package Owner=<4>\n", name);
	fprintf(file, "# Microsoft Developer Studio Generated Build File, Format Version 6.00\n");
	fprintf(file, "# ** DO NOT EDIT **\n");
	fprintf(file, "\n");
	fprintf(file, "# TARGTYPE \"%s\" %s\n", tag, num);
	fprintf(file, "\n");
	fprintf(file, "CFG=%s - Win32 Debug\n", name);
	fprintf(file, "!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n");
	fprintf(file, "!MESSAGE use the Export Makefile command and run\n");
	fprintf(file, "!MESSAGE \n");
	fprintf(file, "!MESSAGE NMAKE /f \"%s.mak\".\n", package->name);
	fprintf(file, "!MESSAGE \n");
	fprintf(file, "!MESSAGE You can specify a configuration when running NMAKE\n");
	fprintf(file, "!MESSAGE by defining the macro CFG on the command line. For example:\n");
	fprintf(file, "!MESSAGE \n");
	fprintf(file, "!MESSAGE NMAKE /f \"%s.mak\" CFG=\"%s - Win32 Debug\"\n", package->name,  name);
	fprintf(file, "!MESSAGE \n");
	fprintf(file, "!MESSAGE Possible choices for configuration are:\n");
	fprintf(file, "!MESSAGE \n");
	for (i = 0; i < package->numConfigs; ++i)
		fprintf(file, "!MESSAGE \"%s - Win32 %s\" (based on \"%s\")\n", name, package->config[i]->name, tag);
	fprintf(file, "!MESSAGE \n");
	fprintf(file, "\n");
	fprintf(file, "# Begin Project\n");
	fprintf(file, "# PROP AllowPerConfigDependencies 0\n");
	fprintf(file, "# PROP Scc_ProjName \"\"\n");
	fprintf(file, "# PROP Scc_LocalPath \"\"\n");

	return 1;
}
