//-----------------------------------------------------------------------------
// Premake - project.h
//
// An interface to the project settings.
//
// Copyright (C) 2002-2004 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: project.h,v 1.10 2004/03/27 13:42:24 jason379 Exp $
//-----------------------------------------------------------------------------

typedef struct _Config
{
	const char* name;
	const char* target;
	const char** buildFlags;
	const char** buildOptions;
	const char** defines;
	const char** includePaths;
	const char** libPaths;
	const char** linkFlags;
	const char** linkOptions;
	const char** links;
	const char* pchHeader;
	const char* pchSource;
	const char* nasmPath;
	char* objdir;
	int build; // 0 or 1, to indicate whether to include it in the default build
	int numBuildFlags;
	int numBuildOptions;
	int numDefines;
	int numIncludePaths;
	int numLibPaths;
	int numLinkFlags;
	int numLinkOptions;
	int numLinks;
} Config;

typedef struct _Package
{
	const char* name;
	const char* script;
	const char* path;
	const char* language;
	const char* kind;
	const char* url;
	const char** files;
	int numFiles;
	Config** config;
	int numConfigs;
	void* data;
} Package;

typedef struct _Option
{
	char* flag;
	char* description;
	struct _Option* next;
} Option;

typedef struct _Project
{
	const char* name;
	const char* binaries;
	const char* libraries;
	const char* debugfiles;
	const char* path;
	Package** package;
	Option** option;
	int numPackages;
	int numOptions;
} Project;


extern Project* project;

extern void createProject(char* argv[]);
extern int  loadProject(char* project);
extern void closeProject();
extern void handleCommand(char* args[], int i);

// Deprecated?
extern Package* getPackage(const char* name);
extern void configureProject();
extern void setProjectOption(char* option);

// Access to the lua objects
extern int getProject();
extern const char* getString(int ref, char* name);
