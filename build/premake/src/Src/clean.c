//-----------------------------------------------------------------------------
// Premake - clean.c
//
// Remove all project and intermediate files.
//
// Copyright (C) 2002-2003 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: clean.c,v 1.6 2004/03/27 13:42:24 jason379 Exp $
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "project.h"
#include "util.h"

//-----------------------------------------------------------------------------

int makeClean()
{
	int i, j;
	const char* binaries = project->binaries;
	const char* libraries = project->libraries;

	puts("Removing all project and intermediate files...");

	deleteFile(project->path, project->name, ".sln");    // VS200x
	deleteFile(project->path, project->name, ".suo");    // VS200x
	deleteFile(project->path, project->name, ".ncb");    // VS6/200x
	deleteFile(project->path, project->name, ".dsw");    // VS6
	deleteFile(project->path, project->name, ".opt");    // VS6
	deleteFile(project->path, "Makefile", "");           // GNU
	deleteFile(project->path, project->name, ".cmbx");   // SharpDevelop
	
	for (i = 0; i < project->numPackages; ++i)
	{
		Package* package = project->package[i];
		const char* name = package->name;
		const char* path = package->path;

		for (j = 0; j < package->numConfigs; ++j)
		{
			char buffer[256];
			Config* config = package->config[j];
			const char* target = config->target;
			
			strcpy(buffer, "lib");                  // posix shared lib
			strcat(buffer, target);
			deleteFile(binaries, buffer, ".so");

			deleteFile(binaries, target, "");       // posix executable
			deleteFile(binaries, target, ".exe");   // windows executable
			deleteFile(binaries, target, ".dll");   // windows or .NET shared lib
			deleteFile(binaries, target, ".pdb");   // VS symbol file
			deleteFile(binaries, target, ".ilk");   // VS incremental link
			deleteFile(libraries, target, ".pdb");  // VS symbol file
			deleteFile(libraries, target, ".exp");  // VS export lib
			deleteFile(libraries, target, ".lib");  // windows static lib
		}
		
		deleteFile(path, name, ".csproj");          // VS200x
		deleteFile(path, name, ".csproj.user");     // VS200x
		deleteFile(path, name, ".csproj.webinfo");  // VS200x
		deleteFile(path, name, ".vcproj");          // VS200x
		deleteFile(path, name, ".dsp");             // VS6
		deleteFile(path, name, ".plg");             // VS6
		deleteFile(path, name, ".make");            // GNU
		deleteFile(path, name, ".prjx");            // SharpDevelop
		
		deleteDirectory(path, "obj");  // intermediates directory
	}
	
	return 1;
}
