//-----------------------------------------------------------------------------
// Premake - sharpdev.c
//
// ICSharpCode SharpDevelop tool target.
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// Written by Chris McGuirk (leedgitar@latenitegames.com)
// Modified by Jason Perkins
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "util.h"

static char buffer[4096];

static int writeCombine();
static int writeCsProject(Package* package);

extern const char* dotnet;

//-----------------------------------------------------------------------------

int makeSharpDevScripts()
{
	int i;

	puts("Generating SharpDevelop combine and project files:");

	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];

		printf("...%s\n", package->name);

		if (strcmp(package->language, "c#") == 0)
		{
			if (!writeCsProject(package))
				return 0;
		}

		else if (strcmp(package->language, "c++") == 0 || strcmp(package->language, "c") == 0)
		{
			printf("** Error: SharpDevelop does not support C/C++ development.\n");
			return 0;
		}

		else
		{
			printf("** Error: unrecognized language '%s'\n", package->language);
			return 0;
		}
	}

	if (!writeCombine())
		return 0;

	return 1;
}

//-----------------------------------------------------------------------------

static int writeCombine()
{
	int i, j, k;

	FILE* file;
	file = openFile(project->path, project->name, ".cmbx");
	if (file == NULL)
		return 0;

	fprintf(file, "<Combine fileversion=\"1.0\" name=\"%s\" description=\"\">\n", project->name);

	// TODO: select the first executable project
	// pick the first project as the startup by default in the meantime
	if(project->numPackages != 0)
	{
		fprintf(file, "  <StartMode startupentry=\"%s\" single=\"True\">\n", project->package[0]->name);
	}

	// first write out the startup entries
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		const char* name = package->name;
		fprintf(file, "    <Execute entry=\"%s\" type=\"None\" />\n", name);
	}

	fprintf(file, "  </StartMode>\n");
	fprintf(file, "  <Entries>\n");

	// now write out the project entries
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		const char* name = package->name;
		const char* path = reversePath(project->path, package->path, NATIVE);
		fprintf(file, "    <Entry filename=\"%s%s.%s\" />\n", path, name, "prjx");
	}

	fprintf(file, "  </Entries>\n");
	fprintf(file, "  <Configurations active=\"Debug\">\n");

	// write out the entries for each build configuration
	for (i = 0; i < project->package[0]->numConfigs; ++i)
	{
		const char* configName = project->package[0]->config[i]->name;

		fprintf(file, "    <Configuration name=\"%s\">\n", configName);

		// loop through each package.  if has a configuration matching the curent one, write an entry
		for(j = 0; j < project->numPackages; j++)
		{
			Package* package = project->package[j];
			const char* name = package->name;

			int isIncluded = 0;

			// look at each of this projects configs
			for(k = 0; k < package->numConfigs; k++)
			{
				if(strcmp(configName, package->config[k]->name) != 0)
				{
					isIncluded = 1;
					break;
				}
			}

			// write the entry for the current project in this build configuration
			fprintf(file, "      <Entry name=\"%s\" configurationname=\"%s\" build=\"%s\" />\n", package->name, configName, isIncluded ? "True" : "False");
		}

        fprintf(file, "    </Configuration>\n");
	}

	fprintf(file, "  </Configurations>\n");
	fprintf(file, "</Combine>");

	// Finish
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
	if (strcmp(package->language, "c#") != 0) return NULL;
	return package->config[*((int*)data)]->target;
}

//-----------------------------------------------------------------------------

static const char* checkRefs(const char* ref, void* data)
{
	int i;
	int isSibling = 0;
	const char* fileName = getFilename(ref, 0);

	strcpy(buffer," type=\"");

	// Is this a sibling project?
	for (i = 0; i < project->numPackages; ++i)
	{
		if (strcmp(project->package[i]->name, ref) == 0)
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
		const char* ext = strchr(ref,',') ? "" : ".dll";

		// See if this assembly exists on one of the link paths
		Package* package = (Package*)data;
		if (fileExists(project->binaries, ref, ext))
		{
			strcat(buffer, "Assembly\" refto=\"");
			strcat(buffer, reversePath(package->path, project->binaries, NATIVE));
			strcat(buffer, "/");
			strcat(buffer, ref);
			strcat(buffer, ".dll\" localcopy=\"False\"");
		}
		else
		{
			strcat(buffer, "Gac\" refto=\"");
			strcat(buffer, ref);
			strcat(buffer, ext);
			strcat(buffer, "\" localcopy=\"False\"");
		}
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
	const char* ext;

	strcpy(buffer, translatePath(file, WIN32));
	strcat(buffer, "\" subtype=\"Code\" ");
	
	ext = getExtension(file);
	if (strcmp(ext, ".cs") == 0)
	{
		strcat(buffer, "BuildAction=\"Compile\"");
	}
	else if (strcmp(ext, ".resx") == 0)
	{
		strcat(buffer, "BuildAction=\"EmbedAsResource\"");
	}
	else
	{
		strcat(buffer, "BuildAction=\"Nothing\"");
	}

	return buffer;
}

static int writeCsProject(Package* package)
{
	FILE* file;
	const char* runtime;
	const char* csc;
	int i;

	const char* name = package->name;
	const char* path = package->path;
	const char* kind = package->kind;

	if (strcmp(kind, "winexe") == 0)
		kind = "WinExe";
	else if (strcmp(kind, "exe") == 0)
		kind = "Exe";
	else if (strcmp(kind, "dll") == 0 || strcmp(kind, "aspnet") == 0)
		kind = "Library";
	else
	{
		printf("** Error: unknown package kind '%s'\n", kind);
		return 0;
	}

	// Figure out what .net environment I'm using
	if (dotnet == NULL)
		dotnet = (strcmp(osIdent, "windows") == 0) ? "ms" : "mono";
	
	if (strcmp(dotnet, "ms") == 0)
	{
		runtime = "MsNet";
		csc = "Csc";
	}
	else if (strcmp(dotnet, "mono") == 0)
	{
		runtime = "Mono";
		csc = "Mcs";
	}
	else if (strcmp(dotnet, "pnet") == 0)
	{
		printf("** Error: SharpDevelop does not yet support Portable.NET\n");
		return 0;
	}
	else
	{
		printf("** Error: unknown .NET runtime '%s'\n", dotnet);
		return 0;
	}


	// Open the project file and write the header
	file = openFile(path, name, ".prjx");
	if (file == NULL)
		return 0;

	// Project Header
	fprintf(file, "<Project name=\"%s\" description=\"\" newfilesearch=\"None\" enableviewstate=\"True\" version=\"1.1\" projecttype=\"C#\">\n", name);

	// File List
	fprintf(file, "  <Contents>\n");
	writeList(file, package->files, "    <File name=\".\\", " dependson=\"\" data=\"\" />\n", "", checkSrcFileType, NULL);
	fprintf(file, "  </Contents>\n");

	// References
	fprintf(file, "  <References>\n");
	writeList(file, package->config[0]->links, "    <Reference", " />\n", "", checkRefs, package);
	fprintf(file, "  </References>\n");

	// Configurations
	fprintf(file, "  <Configurations active=\"%s\">\n", package->config[0]->name);

	for (i = 0; i < package->numConfigs; ++i)
	{
		Config* config = package->config[i];

		int symbols = !inArray(config->buildFlags, "no-symbols");
		int optimize = inArray(config->buildFlags, "optimize") || inArray(config->buildFlags, "optimize-size") || inArray(config->buildFlags, "optimize-speed");
		int unsafe = inArray(config->buildFlags, "unsafe");

		fprintf(file, "    <Configuration runwithwarnings=\"True\" name=\"%s\">\n", config->name);
		fprintf(file, "      <CodeGeneration runtime=\"%s\" compiler=\"%s\"", runtime, csc);
		fprintf(file, " warninglevel=\"0\" ");
		fprintf(file, "includedebuginformation=\"%s\" ", symbols ? "True" : "False"); 
		fprintf(file, "optimize=\"%s\" ", optimize ? "True" : "False");
		fprintf(file, "unsafecodeallowed=\"%s\" ", unsafe ? "True" : "False");
		fprintf(file, "generateoverflowchecks=\"True\" ");
		fprintf(file, "mainclass=\"\" ");
		fprintf(file, "target=\"%s\" ", kind); 
		fprintf(file, "definesymbols=\"");
			writeList(file, config->defines, "", "", ";", NULL, NULL);
			fprintf(file, "\" ");
		fprintf(file, "generatexmldocumentation=\"False\" ");
		fprintf(file, "/>\n");

		fprintf(file, "      <Output ");
		fprintf(file, "directory=\"");
			fprintf(file, reversePath(path, project->binaries, NATIVE));
			insertPath(file, getDirectory(config->target), NATIVE);
			fprintf(file, "\" ");
		fprintf(file, "assembly=\"%s\" ", getFilename(config->target,0));
		fprintf(file, "executeScript=\"\" ");
		fprintf(file, "executeBeforeBuild=\"\" ");
		fprintf(file, "executeAfterBuild=\"\" ");
		fprintf(file, "/>\n");		

		fprintf(file, "    </Configuration>\n", config->name);
	}

	fprintf(file, "  </Configurations>\n");
	fprintf(file, "</Project>\n");

	fclose(file);
	return 1;
}
