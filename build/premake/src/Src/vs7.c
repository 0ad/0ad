//-----------------------------------------------------------------------------
// Premake - vs_xml.c
//
// MS Visual Studio XML projects files (v7.0-2003)
//
// Copyright (C) 2002-2004 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: vs7.c,v 1.34 2004/05/11 00:38:58 jason379 Exp $
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "util.h"

typedef struct _PkgData
{
	char projGuid[38];
	char toolGuid[38];
	char projExt[8];
	char projType[8];
} PkgData;

static char buffer[4096];

static int writeSolution(int version);
static int writeVcProject(int version, Package* package);
static int writeCsProject(int version, Package* package);

//-----------------------------------------------------------------------------

int makeVsXmlScripts(int version)
{
	int i;
	printf("Generating Visual Studio %d solution and project files:\n", version);

	// Assign UUIDs to all packages
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		PkgData* data = (PkgData*)malloc(sizeof(PkgData));
		package->data = data;
		generateUUID(data->projGuid);
		if (strcmp(package->language, "c++") == 0 || strcmp(package->language, "c") == 0)
		{
			strcpy(data->toolGuid, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942");
		}
		else if (strcmp(package->language, "c#") == 0)
		{
			strcpy(data->toolGuid, "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC");
		}
		else
		{
			printf("** Error: unrecognized language '%s'\n", package->language);
			return 0;
		}
	}

	// Generate the project files
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		PkgData* data = (PkgData*)package->data;

		printf("...%s\n", package->name);

		if (strcmp(package->language, "c++") == 0 || strcmp(package->language, "c") == 0)
		{
			strcpy(data->projExt, "vcproj");
			strcpy(data->projType, "Win32");
			if (!writeVcProject(version, package))
				return 0;
		}

		else if (strcmp(package->language, "c#") == 0)
		{
			strcpy(data->projExt, "csproj");
			strcpy(data->projType, ".NET");
			if (!writeCsProject(version, package))
				return 0;
		}
	}

	if (!writeSolution(version))
		return 0;

	return 1;
}

//-----------------------------------------------------------------------------

static int numDeps = 0;   /* Use by the project dependency writer */

/* Look for a package with the same name as 'name' and if found, return a
 * dependency description to be included in the solution file */
static const char* checkProjectDependencies(const char* name, void* data)
{
	int i;
	int version = (int)data;
	for (i = 0; i < project->numPackages; ++i)
	{
		if (strcmp(project->package[i]->name, name) == 0)
		{
			PkgData* data = (PkgData*)project->package[i]->data;
			if (version == 7)
			{
				sprintf(buffer, "%d = {%s}", numDeps++, data->projGuid);
			}
			else
			{
				sprintf(buffer, "{%s} = {%s}", data->projGuid, data->projGuid);
			}
			return buffer;
		}
	}
	return NULL;
}

static int writeSolution(int version)
{
	int i, j;
	FILE* file;

	file = openFile(project->path, project->name, ".sln");
	if (file == NULL)
		return 0;

	/* Line 1: Format identification string */
	fprintf(file, "Microsoft Visual Studio Solution File, Format Version ");
	switch (version)
	{
	case 7:
		fprintf(file, "7.00\n");
		break;
	case 2003:
		fprintf(file, "8.00\n");
		break;
	}

	/* List of projects that make up the solution */
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		PkgData* data = package->data;
		const char* name = package->name;
		const char* path = reversePath(project->path, package->path, WIN32);

		/* VS.NET doesn't write out the leading '.\' on solution-relative
		 * paths. Not really necessary, but I'm trying to match the original */
		if (strncmp(path, ".\\", 2) == 0)
			path = path + 2;

		fprintf(file, "Project(\"{%s}\") = \"%s\", \"%s%s.%s\", \"{%s}\"\n", data->toolGuid, name, path, name, data->projExt, data->projGuid);
		if (version == 2003)
		{
			fprintf(file, "\tProjectSection(ProjectDependencies) = postProject\n");
			writeList(file, package->config[0]->links, "\t\t", "\n", "", checkProjectDependencies, (void*)version);
			fprintf(file, "\tEndProjectSection\n");
		}
		fprintf(file, "EndProject\n");
	}

	fprintf(file, "Global\n");
	fprintf(file, "\tGlobalSection(SolutionConfiguration) = preSolution\n");
	for (i = 0; i < project->package[0]->numConfigs; ++i)
	{
		if (version == 7)
		{
			fprintf(file, "\t\tConfigName.%d = %s\n", i, project->package[0]->config[i]->name);
		}
		else
		{
			fprintf(file, "\t\t%s = %s\n", project->package[0]->config[i]->name, project->package[0]->config[i]->name);
		}
	}
	fprintf(file, "\tEndGlobalSection\n");

	/* Find package dependencies for VS7 */
	if (version == 7)
	{
		fprintf(file, "\tGlobalSection(ProjectDependencies) = postSolution\n");
		for (i = 0; i < project->numPackages; ++i)
		{
			char prefix[128];
			Package* package = project->package[i];
			PkgData* data = (PkgData*)package->data;
			Config* config = package->config[0];

			numDeps = 0;
			sprintf(prefix, "\t\t{%s}.", data->projGuid);
			writeList(file, config->links, prefix, "\n", "", checkProjectDependencies, (void*)version);
		}
		fprintf(file, "\tEndGlobalSection\n");
	}

	// Write configuration for each project
	fprintf(file, "\tGlobalSection(ProjectConfiguration) = postSolution\n");
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		for (j = 0; j < package->numConfigs; ++j)
		{
			Config* config = package->config[j];
			PkgData* data = (PkgData*)package->data;
            fprintf(file, "\t\t{%s}.%s.ActiveCfg = %s|%s\n", data->projGuid, config->name, config->name, data->projType);
			if (config->build)
				fprintf(file, "\t\t{%s}.%s.Build.0 = %s|%s\n", data->projGuid, config->name, config->name, data->projType);
		}
	}
	fprintf(file, "\tEndGlobalSection\n");

	// Finish

	fprintf(file, "\tGlobalSection(ExtensibilityGlobals) = postSolution\n");
	fprintf(file, "\tEndGlobalSection\n");
	fprintf(file, "\tGlobalSection(ExtensibilityAddIns) = postSolution\n");
	fprintf(file, "\tEndGlobalSection\n");
	fprintf(file, "EndGlobal\n");

	fclose(file);
	return 1;
}

//-----------------------------------------------------------------------------

static const char* checkDir(const char* path, void* data)
{
	return translatePath(path, WIN32);
}

static const char* checkLibs(const char* file, void* data)
{
	Package* package = getPackage(file);
	if (package == NULL) return file;
	if (strcmp(package->language, "c++") != 0) return NULL;
	return package->config[*((int*)data)]->target;
}

static void vcFiles(FILE* file, const char* path, int stage)
{
	char indent[128];
	char* ptr;
	int i=0, j=0;

	strcpy(indent, "    ");
	if (strlen(path) > 0) strcat(indent, "    ");
	ptr = strchr(path, '/');
	while (ptr != NULL) {
		strcat(indent, "    ");
		ptr = strchr(ptr + 1, '/');
	}

	ptr = strrchr(path, '/');
	ptr = (ptr == NULL) ? (char*)path : ptr + 1;

	switch (stage)
	{
	case WST_OPENGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0)
		{
			fprintf(file, "%s<Filter\n", indent);
			fprintf(file, "%s    Name=\"%s\"\n", indent, ptr);
			fprintf(file, "%s    Filter=\"\">\n", indent);
		}
		break;

	case WST_CLOSEGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0) 
			fprintf(file, "%s</Filter>\n", indent);
		break;

	case WST_SOURCEFILE:
		fprintf(file, "%s<File\n", indent);
		fprintf(file, "%s    RelativePath=\"%s\">\n", indent, translatePath(path, WIN32));
		for (i=0;i<project->numPackages;i++)
		{
			Package *package=project->package[i];
			for (j=0;j<package->numConfigs;j++)
			{
				Config *config=package->config[j];
				
				if (config->pchSource && strcmp(ptr, config->pchSource)==0)
				{
					fprintf(file, "%s    <FileConfiguration Name=\"%s|Win32\">\n", indent, config->name);
					fprintf(file, "%s        <Tool Name=\"VCCLCompilerTool\" UsePrecompiledHeader=\"1\" />\n", indent);
					fprintf(file, "%s    </FileConfiguration>", indent);
				}
			}
		}
		fprintf(file, "%s</File>\n", indent);
		break;
	}
}

static int writeVcProject(int version, Package* package)
{
	int configType, subsystem, managed, i;
	const char* extension;
	FILE* file;

	const char* name = package->name;
	const char* path = package->path;
	const char* kind = package->kind;
	PkgData* data = (PkgData*)package->data;

	// Set up target type information

	if (strcmp(kind, "winexe") == 0 || strcmp(kind, "exe") == 0)
	{
		configType = 1;
		extension = "exe";
	}
	else if (strcmp(kind, "dll") == 0)
	{
		configType = 2;
		extension = "dll";
	}
	else if (strcmp(kind, "lib") == 0)
	{
		configType = 4;
		extension = "lib";
	}
	else if (strcmp(kind, "aspnet") == 0)
	{
		puts("** Error: C++ ASP.NET projects are not supported");
		return 0;
	}
	else
	{
		printf("** Error: unknown package kind '%s'\n", kind);
		return 0;
	}

	// Check the build subsystem...must be the same for all configs

	subsystem = (strcmp(kind, "exe") == 0) ? 1 : 2;
	managed = inArray(package->config[0]->buildFlags, "managed");

	// Open the file and write the header

	file = openFile(path, name, ".vcproj");
	if (file == NULL)
		return 0;

	fprintf(file, "<?xml version=\"1.0\" encoding = \"Windows-1252\"?>\n");
	fprintf(file, "<VisualStudioProject\n");
	fprintf(file, "    ProjectType=\"Visual C++\"\n");
	fprintf(file, "    Version=\"");
	switch (version)
	{
	case 7:
		fprintf(file, "7.00");
		break;
	case 2003:
		fprintf(file, "7.10");
		break;
	}
	fprintf(file, "\"\n");
	fprintf(file, "    Name=\"%s\"\n", name);
	fprintf(file, "    ProjectGUID=\"{%s}\"\n", data->projGuid);
	fprintf(file, "    Keyword=\"%s\">\n", managed ? "ManagedCProj" : "Win32Proj");
	fprintf(file, "    <Platforms>\n");
	fprintf(file, "        <Platform\n");
	fprintf(file, "            Name=\"Win32\"/>\n");
	fprintf(file, "    </Platforms>\n");

	// Write configurations

	fprintf(file, "    <Configurations>\n");
	for (i = 0; i < package->numConfigs; ++i)
	{
		int optLevel, debug, symbols, runtime, noMain;
		Config* config = package->config[i];

		int optimize = inArray(config->buildFlags, "optimize");
		int size  = inArray(config->buildFlags, "optimize-size");
		int speed = inArray(config->buildFlags, "optimize-speed");

		int check64bit = !inArray(config->buildFlags, "no-64bit-checks");
		int importlib  = !inArray(config->buildFlags, "no-import-lib");
		int exceptions = !inArray(config->buildFlags, "no-exceptions");
		int rtti       = !inArray(config->buildFlags, "no-rtti");
		int rtc        = !inArray(config->buildFlags, "no-runtime-checks");

		if (speed)
			optLevel = 2;
		else if (size)
			optLevel = 1;
		else if (optimize)
			optLevel = 3;
		else
			optLevel = 0;
		debug = (optLevel == 0);

		if (!inArray(config->buildFlags, "no-symbols"))
			symbols = (managed || inArray(config->buildFlags, "no-edit-and-continue")) ? 3 : 4;
		else
			symbols = 0;

		if (inArray(config->linkFlags, "static-runtime") || inArray(config->buildFlags, "static-runtime"))
			runtime = (debug) ? 1 : 0;
		else
			runtime = (debug) ? 3 : 2;

		noMain = inArray(config->buildFlags, "no-main");

		fprintf(file, "        <Configuration\n");
		fprintf(file, "            Name=\"%s|Win32\"\n", config->name);
		fprintf(file, "            OutputDirectory=\"");
			fprintf(file, reversePath(path, (configType != 4 ? project->binaries : project->libraries), WIN32));
			insertPath(file, getDirectory(config->target), WIN32);
			fprintf(file, "\"\n");
		fprintf(file, "            IntermediateDirectory=\"%s\\\"\n", config->objdir);
		fprintf(file, "            ConfigurationType=\"%d\"\n", configType);
		fprintf(file, "            CharacterSet=\"2\"");
		if (managed) fprintf(file, "\n            ManagedExtensions=\"TRUE\"");
		fprintf(file, ">\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCCLCompilerTool\"\n");
		fprintf(file, "                Optimization=\"%d\"\n", optLevel);
		if (!debug) fprintf(file, "                InlineFunctionExpansion=\"1\"\n");
		if (!debug) fprintf(file, "                OmitFramePointers=\"TRUE\"\n");
		fprintf(file, "                AdditionalOptions=\"");
			writeList(file, config->buildOptions, "", "", " ", NULL, NULL);
			fprintf(file, "\"\n");
		fprintf(file, "                AdditionalIncludeDirectories=\"");
			writeList(file, config->includePaths, "", "", ",", checkDir, NULL);
			fprintf(file, "\"\n");
		if (managed)
		{
			fprintf(file, "                AdditionalUsingDirectories=\"");
			fprintf(file, reversePath(path, project->binaries, WIN32));
			fprintf(file, "\"\n");
		}
		fprintf(file, "                PreprocessorDefinitions=\"");
			writeList(file, config->defines, "", "", ";", NULL, NULL);
			fprintf(file, "\"\n");
		fprintf(file, "                MinimalRebuild=\"%s\"\n", (debug && !managed) ? "TRUE" : "FALSE");
		if (!exceptions) fprintf(file, "                ExceptionHandling=\"FALSE\"\n");
		fprintf(file, "                BasicRuntimeChecks=\"%d\"\n", (debug && !managed && rtc) ? 3 : 0);
		if (!debug) fprintf(file, "                StringPooling=\"TRUE\"\n");
		if (!debug) fprintf(file, "                EnableFunctionLevelLinking=\"TRUE\"\n");
		fprintf(file, "                RuntimeLibrary=\"%d\"\n", runtime);
		if (rtti)
			fprintf(file, "                RuntimeTypeInfo=\"TRUE\"\n");
		
		if (config->pchHeader)
		{
			fprintf(file, "                UsePrecompiledHeader=\"3\"\n");
			fprintf(file, "                PrecompiledHeaderThrough=\"%s\"\n", config->pchHeader);
		}
		else
			fprintf(file, "                UsePrecompiledHeader=\"0\"\n");

		fprintf(file, "                WarningLevel=\"4\"\n");
		if (!managed) fprintf(file, "                Detect64BitPortabilityProblems=\"%s\"\n", check64bit ? "TRUE" : "FALSE");
		fprintf(file, "                DebugInformationFormat=\"%d\"/>\n", symbols);

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCCustomBuildTool\"/>\n");

		if (configType != 4)
		{
			fprintf(file, "            <Tool\n");
			fprintf(file, "                Name=\"VCLinkerTool\"\n");
			fprintf(file, "                IgnoreImportLibrary=\"%s\"\n", importlib ? "TRUE" : "FALSE");
			fprintf(file, "                AdditionalOptions=\"");
				writeList(file, config->linkOptions, " ", "", "", NULL, NULL);
				fprintf(file, "\"\n");
			fprintf(file, "                AdditionalDependencies=\"");
				writeList(file, config->links, "", ".lib", " ", checkLibs, &i);
				fprintf(file, "\"\n");
			fprintf(file, "                OutputFile=\"$(OutDir)\\%s.%s\"\n", getFilename(config->target, 0), extension);
			fprintf(file, "                LinkIncremental=\"%d\"\n", debug ? 2 : 1);
			fprintf(file, "                AdditionalLibraryDirectories=\"");
				fprintf(file, reversePath(path, project->libraries, WIN32));
				writeList(file, config->libPaths, ";", "", "", checkDir, NULL);
				fprintf(file, "\"\n");
			fprintf(file, "                GenerateDebugInformation=\"%s\"\n", symbols ? "TRUE" : "FALSE");

			if (symbols)
			{
				if (project->debugfiles)
				{
					fprintf(file, "                ProgramDatabaseFile=\"");
					fprintf(file, reversePath(path, project->debugfiles, WIN32));
					fprintf(file, "%s.pdb\"\n", getFilename(config->target, 0));
				}
				else
				{
					fprintf(file, "                ProgramDatabaseFile=\"%s\\%s.pdb\"\n", config->objdir, getFilename(config->target, 0));
				}
			}
			fprintf(file, "                SubSystem=\"%d\"\n", subsystem);
			if (!debug) fprintf(file, "                OptimizeReferences=\"2\"\n");
			if (!debug) fprintf(file, "                EnableCOMDATFolding=\"2\"\n");
			if ((strcmp(kind, "winexe") == 0 || strcmp(kind, "exe") == 0) && !noMain)
				fprintf(file, "                EntryPointSymbol=\"mainCRTStartup\"\n");
			else if (strcmp(kind, "dll") == 0) {
				fprintf(file, "                ImportLibrary=\"");
				if (importlib)
					fprintf(file, reversePath(path, project->libraries, WIN32));
				else
					fprintf(file, "$(IntDir)");
				fprintf(file, "%s.lib\"\n", getFilename(config->target, 1));
			}
			fprintf(file, "                TargetMachine=\"1\"/>\n");
		}
		else
		{
			fprintf(file, "            <Tool\n");
			fprintf(file, "                Name=\"VCLibrarianTool\"\n");
			fprintf(file, "                OutputFile=\"$(OutDir)\\%s.lib\"/>\n", config->target);
		}

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCMIDLTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCPostBuildEventTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCPreBuildEventTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCPreLinkEventTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCResourceCompilerTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCWebServiceProxyGeneratorTool\"/>\n");

		fprintf(file, "            <Tool\n");
		fprintf(file, "                Name=\"VCWebDeploymentTool\"/>\n");
		fprintf(file, "        </Configuration>\n");
	}

	fprintf(file, "    </Configurations>\n");

	fprintf(file, "    <Files>\n");
	walkSourceList(file, package, "", vcFiles);
	fprintf(file, "    </Files>\n");

	fprintf(file, "    <Globals>\n");
	fprintf(file, "    </Globals>\n");
	fprintf(file, "</VisualStudioProject>\n");

	fclose(file);
	return 1;
}

//-----------------------------------------------------------------------------

static const char* checkRefs(const char* ref, void* data)
{
	int i;
	char* comma;
	char tmp[1024];

	// Pull the name out of the full reference
	strcpy(tmp, ref);
	comma = strchr(tmp, ',');
	if (comma != NULL)
		*comma = '\0';

	// Write the reference name
	strcpy(buffer, "                    Name = \"");
	strcat(buffer, tmp);
	strcat(buffer, "\"\n");

	// Is this a sibling project?
	for (i = 0; i < project->numPackages; ++i)
	{
		if (strcmp(project->package[i]->name, ref) == 0)
		{
			PkgData* data = (PkgData*)project->package[i]->data;
			strcat(buffer, "                    Project = \"{");
			strcat(buffer, data->projGuid);
			strcat(buffer, "}\"\n");
			strcat(buffer, "                    Package = \"{");
			strcat(buffer, data->toolGuid);
			strcat(buffer, "}\"\n");
			return buffer;
		}
	}

	strcat(buffer, "                    AssemblyName = \"");
	strcat(buffer, getFilename(tmp, 0));
	strcat(buffer, "\"\n");
	if (strlen(tmp) != strlen(getFilename(tmp, 0))) {
		strcat(buffer, "                    HintPath = \"");
		strcat(buffer, tmp);
		strcat(buffer, ".dll\"\n");
	}

	// Tack on any extra information about the assembly
	while (comma != NULL)
	{
		char* start;
		for (start = comma + 1; *start == ' '; ++start);
		comma = strchr(start, '=');
		*comma = '\0';
		strcat(buffer, "                    ");
		strcat(buffer, start);
		strcat(buffer, " = \"");

		start = comma + 1;
		comma = strchr(start, ',');
		if (comma != NULL) *comma = '\0';
		strcat(buffer, start);
		strcat(buffer, "\"\n");
	}

	return buffer;
}

static const char* checkRefPaths(const char* ref, void* data)
{
	const char* projPath = (const char*)data;
	return makeAbsolute(projPath, ref);
}

static const char* checkSrcFileType(const char* file, void* data)
{
	Package* package = (Package*)data;

	strcpy(buffer, translatePath(file, WIN32));
	strcat(buffer, "\"\n");

	if (endsWith(file, ".aspx.cs") || endsWith(file, ".asax.cs"))
	{
		/* The path to the parent .aspx file is relative to the .cs file. 
		 * I assume that they are in the same directory and strip off all
		 * path information, then cut off the '.cs' */
		char* ptr = strrchr(file, '/');
		if (ptr == NULL) 
			ptr = (char*)file;
		else
			ptr++;

		strcat(buffer, "\t\t\t\t\tDependentUpon = \"");
		strncat(buffer, ptr, strlen(ptr) - 3);
		strcat(buffer, "\"\n");
		strcat(buffer, "\t\t\t\t\tSubType = \"ASPXCodeBehind\"\n");
		strcat(buffer, "\t\t\t\t\tBuildAction = \"Compile\"\n");
	}
/* obsolete, handled by ".resx" block below
	else if (endsWith(file, ".aspx.resx") || endsWith(file, ".asax.resx"))
	{
		strcat(buffer, "\t\t\t\t\tDependentUpon = \"");
		strncat(buffer, file, strlen(file) - 5);
		strcat(buffer, ".cs\"\n");
		strcat(buffer, "\t\t\t\t\tBuildAction = \"EmbeddedResource\"\n");
	}
*/
	else if (endsWith(file, ".cs"))
	{
		strcat(buffer, "\t\t\t\t\tSubType = \"Code\"\n");
		strcat(buffer, "\t\t\t\t\tBuildAction = \"Compile\"\n");
	}
	else if (endsWith(file, ".aspx"))
	{
		strcat(buffer, "\t\t\t\t\tSubType = \"Form\"\n");
		strcat(buffer, "\t\t\t\t\tBuildAction = \"Content\"\n");
	}
	else if (endsWith(file, ".asax"))
	{
		strcat(buffer, "\t\t\t\t\tSubType = \"Component\"\n");
		strcat(buffer, "\t\t\t\t\tBuildAction = \"Content\"\n");
	}
	else if (endsWith(file, ".resx"))
	{
		/* If a matching .cs file exists, link it */
		char depname[2048];
		strcpy(depname, file);
		strcpy(depname + strlen(file) - 5, ".cs");
		if (inArray(package->files, depname))
		{
			/* Path is relative to .resx file, I assume both are in same
			 * directory and cut off path information */
			char* ptr = strrchr(depname, '/');
			ptr = (ptr != NULL) ? ptr+1 : depname;
			strcat(buffer, "\t\t\t\t\tDependentUpon = \"");
			strcat(buffer, ptr);
			strcat(buffer, "\"\n");
		}
		strcat(buffer, "\t\t\t\t\tBuildAction = \"EmbeddedResource\"\n");
	}
	else
	{
		strcat(buffer, "\t\t\t\t\tBuildAction = \"");
		strcat(buffer, strcmp(package->kind, "aspnet") == 0 ? "Content" : "None");
		strcat(buffer, "\"\n");
	}

	return buffer;
}

static int writeCsProject(int version, Package* package)
{
	FILE* file;
	const char* target;
	const char* outputType;
	int i;

	const char* name = package->name;
	const char* path = package->path;
	const char* kind = package->kind;
	PkgData* data = (PkgData*)package->data;

	if (strcmp(kind, "winexe") == 0)
		outputType = "WinExe";
	else if (strcmp(kind, "exe") == 0)
		outputType = "Exe";
	else if (strcmp(kind, "dll") == 0 || strcmp(kind, "aspnet") == 0)
		outputType = "Library";
	else
	{
		printf("** Error: unknown package kind '%s'\n", kind);
		return 0;
	}

	// Open the project file and write the header
	file = openFile(path, name, ".csproj");
	if (file == NULL)
		return 0;

	fprintf(file, "<VisualStudioProject>\n");
	fprintf(file, "    <CSHARP\n");
	fprintf(file, "        ProjectType = \"");
		fprintf(file, strcmp(kind, "aspnet") == 0 ? "Web" : "Local");
		fprintf(file, "\"\n");
	switch (version)
	{
	case 7:
		fprintf(file, "        ProductVersion = \"7.0.9254\"\n");
		fprintf(file, "        SchemaVersion = \"1.0\"\n");
		break;
	case 2003:
		fprintf(file, "        ProductVersion = \"7.10.3077\"\n");
		fprintf(file, "        SchemaVersion = \"2.0\"\n");
		break;
	}
	fprintf(file, "        ProjectGuid = \"{%s}\"\n", data->projGuid);
	fprintf(file, "    >\n");
	fprintf(file, "        <Build>\n");
	fprintf(file, "            <Settings\n");
	fprintf(file, "                ApplicationIcon = \"\"\n");
	fprintf(file, "                AssemblyKeyContainerName = \"\"\n");
	fprintf(file, "                AssemblyName = \"%s\"\n", getFilename(package->config[0]->target, 0));
	fprintf(file, "                AssemblyOriginatorKeyFile = \"\"\n");
	fprintf(file, "                DefaultClientScript = \"JScript\"\n");
	fprintf(file, "                DefaultHTMLPageLayout = \"Grid\"\n");
	fprintf(file, "                DefaultTargetSchema = \"IE50\"\n");
	fprintf(file, "                DelaySign = \"false\"\n");
	if (version == 7) 
	{
		fprintf(file, "                NoStandardLibraries = \"false\"\n");
	}
	fprintf(file, "                OutputType = \"%s\"\n", outputType);
	if (version == 2003)
	{
		fprintf(file, "                PreBuildEvent = \"\"\n");
		fprintf(file, "                PostBuildEvent = \"\"\n");
	}
	fprintf(file, "                RootNamespace = \"%s\"\n", getFilename(package->config[0]->target, 0));
	if (version == 2003)
	{		
		fprintf(file, "                RunPostBuildEvent = \"OnBuildSuccess\"\n");
	}

	fprintf(file, "                StartupObject = \"\"\n");
	fprintf(file, "            >\n");

	for (i = 0; i < package->numConfigs; ++i)
	{
		Config* config = package->config[i];

		int symbols = !inArray(config->buildFlags, "no-symbols");
		int optimize = inArray(config->buildFlags, "optimize") || inArray(config->buildFlags, "optimize-size") || inArray(config->buildFlags, "optimize-speed");
		int unsafe = inArray(config->buildFlags, "unsafe");

		fprintf(file, "                <Config\n");
		fprintf(file, "                    Name = \"%s\"\n", config->name);
		fprintf(file, "                    AllowUnsafeBlocks = \"%s\"\n", unsafe ? "true" : "false");
		fprintf(file, "                    BaseAddress = \"285212672\"\n");
		fprintf(file, "                    CheckForOverflowUnderflow = \"false\"\n");
		fprintf(file, "                    ConfigurationOverrideFile = \"\"\n");
		fprintf(file, "                    DefineConstants = \"");
			writeList(file, config->defines, "", "", ";", NULL, NULL);
			fprintf(file, "\"\n");
		fprintf(file, "                    DocumentationFile = \"\"\n");
		fprintf(file, "                    DebugSymbols = \"%s\"\n", symbols ? "true" : "false");
		fprintf(file, "                    FileAlignment = \"4096\"\n");
		fprintf(file, "                    IncrementalBuild = \"");
			fprintf(file, strcmp(kind, "aspnet") == 0 ? "false" : "true");
			fprintf(file, "\"\n");
		if (version == 2003)
		{
			fprintf(file, "                    NoStdLib = \"false\"\n");
			fprintf(file, "                    NoWarn = \"\"\n");
		}
		fprintf(file, "                    Optimize = \"%s\"\n", optimize ? "true" : "false");

		fprintf(file, "                    OutputPath = \"");
			fprintf(file, reversePath(path, project->binaries, WIN32));
			insertPath(file, getDirectory(config->target), WIN32);
			fprintf(file, "\"\n");

		fprintf(file, "                    RegisterForComInterop = \"false\"\n");
		fprintf(file, "                    RemoveIntegerChecks = \"false\"\n");
		fprintf(file, "                    TreatWarningsAsErrors = \"false\"\n");
		fprintf(file, "                    WarningLevel = \"4\"\n");
		fprintf(file, "                />\n");
	}

	fprintf(file, "            </Settings>\n");

	// VS7 requires same references for all configurations
	fprintf(file, "            <References>\n");
	writeList(file, package->config[0]->links, "                <Reference\n", "                />\n", "", checkRefs, package);
	fprintf(file, "            </References>\n");
	fprintf(file, "        </Build>\n");

	fprintf(file, "        <Files>\n");
	fprintf(file, "            <Include>\n");
	writeList(file, package->files, "                <File\n                    RelPath = \"", "                />\n", "", checkSrcFileType, package);
	fprintf(file, "            </Include>\n");
	fprintf(file, "        </Files>\n");
	fprintf(file, "    </CSHARP>\n");
	fprintf(file, "</VisualStudioProject>\n");

	fclose(file);

	// Now write the .csproj.user file for non-web applications or
	// .csproj.webinfo for web applications
	
	if (strcmp(kind, "aspnet") != 0)
	{
		file = openFile(path, name, ".csproj.user");
		if (file == NULL)
			return 0;

		fprintf(file, "<VisualStudioProject>\n");
		fprintf(file, "    <CSHARP>\n");
		fprintf(file, "        <Build>\n");
		fprintf(file, "            <Settings ReferencePath = \"");
			writeList(file, package->config[0]->libPaths, "", ";", "", checkRefPaths, (void*)path);
			target = makeAbsolute(".", project->binaries);
			if (target != NULL) fprintf(file, target);
			fprintf(file, "\" >\n");

		for (i = 0; i < package->numConfigs; ++i)
		{
			fprintf(file, "                <Config\n");
			fprintf(file, "                    Name = \"%s\"\n", package->config[i]->name);
			fprintf(file, "                    EnableASPDebugging = \"false\"\n");
			fprintf(file, "                    EnableASPXDebugging = \"false\"\n");
			fprintf(file, "                    EnableUnmanagedDebugging = \"false\"\n");
			fprintf(file, "                    EnableSQLServerDebugging = \"false\"\n");
			fprintf(file, "                    RemoteDebugEnabled = \"false\"\n");
			fprintf(file, "                    RemoteDebugMachine = \"\"\n");
			fprintf(file, "                    StartAction = \"Project\"\n");
			fprintf(file, "                    StartArguments = \"\"\n");
			fprintf(file, "                    StartPage = \"\"\n");
			fprintf(file, "                    StartProgram = \"\"\n");
			fprintf(file, "                    StartURL = \"\"\n");
			fprintf(file, "                    StartWorkingDirectory = \"\"\n");
			fprintf(file, "                    StartWithIE = \"false\"\n");
			fprintf(file, "                />\n");
		}

		fprintf(file, "            </Settings>\n");
		fprintf(file, "        </Build>\n");
		fprintf(file, "        <OtherProjectSettings\n");
		fprintf(file, "            CopyProjectDestinationFolder = \"\"\n");
		fprintf(file, "            CopyProjectUncPath = \"\"\n");
		fprintf(file, "            CopyProjectOption = \"0\"\n");
		fprintf(file, "            ProjectView = \"ProjectFiles\"\n");
		fprintf(file, "            ProjectTrust = \"0\"\n");
		fprintf(file, "        />\n");
		fprintf(file, "    </CSHARP>\n");
		fprintf(file, "</VisualStudioProject>\n");

		fclose(file);
	}
	else
	{
		if (package->url == NULL)
		{
			sprintf(buffer, "http://localhost/%s", package->name);
			package->url = buffer;
		}

		file = openFile(path, name, ".csproj.webinfo");
		if (file == NULL)
			return 0;

		fprintf(file, "<VisualStudioUNCWeb>\n");
		fprintf(file, "    <Web URLPath = \"%s/%s.csproj\" />\n", package->url, package->name);
		fprintf(file, "</VisualStudioUNCWeb>\n");

		fclose(file);
	}

	return 1;
}
