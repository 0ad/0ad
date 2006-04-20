/**********************************************************************
 * Premake - vs6_cpp.c
 * The Visual C++ 6 C/C++ target
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

static void writeCppFlags();
static void writeLinkFlags();
static const char* filterLinks(const char* name);
static void listFiles(const char* path, int stage);


int vs6_cpp()
{
	const char* projTypeTag;
	const char* projTypeId;
	int i;

	/* Start the file */
	if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "dsp")))
		return 0;

	prj_select_config(0);
	if (prj_is_kind("winexe"))
	{
		projTypeTag = "Win32 (x86) Application";
		projTypeId = "0x0101";
	}
	else if (prj_is_kind("exe"))
	{
		projTypeTag = "Win32 (x86) Console Application";
		projTypeId = "0x0103";
	}
	else if (prj_is_kind("dll"))
	{
		projTypeTag = "Win32 (x86) Dynamic-Link Library";
		projTypeId = "0x0102";
	}
	else if (prj_is_kind("lib"))
	{
		projTypeTag = "Win32 (x86) Static Library";
		projTypeId = "0x0104";
	}
	else
	{
		puts("** Error: unrecognized package type");
		return 0;
	}

	io_print("# Microsoft Developer Studio Project File - Name=\"%s\" - Package Owner=<4>\n", prj_get_pkgname());
	io_print("# Microsoft Developer Studio Generated Build File, Format Version 6.00\n");
	io_print("# ** DO NOT EDIT **\n");
	io_print("\n");
	io_print("# TARGTYPE \"%s\" %s\n", projTypeTag, projTypeId);
	io_print("\n");

	prj_select_config(0);
	io_print("CFG=%s - Win32 %s\n", prj_get_pkgname(), prj_get_cfgname());

	io_print("!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n");
	io_print("!MESSAGE use the Export Makefile command and run\n");
	io_print("!MESSAGE \n");
	io_print("!MESSAGE NMAKE /f \"%s.mak\".\n", prj_get_pkgname());
	io_print("!MESSAGE \n");
	io_print("!MESSAGE You can specify a configuration when running NMAKE\n");
	io_print("!MESSAGE by defining the macro CFG on the command line. For example:\n");
	io_print("!MESSAGE \n");
	io_print("!MESSAGE NMAKE /f \"%s.mak\" CFG=\"%s - Win32 %s\"\n", prj_get_pkgname(), prj_get_pkgname(), prj_get_cfgname());
	io_print("!MESSAGE \n");
	io_print("!MESSAGE Possible choices for configuration are:\n");
	io_print("!MESSAGE \n");

	for (i = prj_get_numconfigs() - 1; i >= 0; --i)
	{
		prj_select_config(i);
		io_print("!MESSAGE \"%s - Win32 %s\" (based on \"%s\")\n", prj_get_pkgname(), prj_get_cfgname(), projTypeTag);
	}

	io_print("!MESSAGE \n");
	io_print("\n");
	io_print("# Begin Project\n");
	io_print("# PROP AllowPerConfigDependencies 0\n");
	io_print("# PROP Scc_ProjName \"\"\n");
	io_print("# PROP Scc_LocalPath \"\"\n");

	io_print("CPP=cl.exe\n");
	if (!prj_is_kind("lib"))
		io_print("MTL=midl.exe\n");
	io_print("RSC=rc.exe\n\n");

	for (i = prj_get_numconfigs() - 1; i >= 0; --i)
	{
		const char* debugSymbol;

		int optimizeSize  =  prj_has_flag("optimize-size");
		int optimizeSpeed =  prj_has_flag("optimize-speed") || prj_has_flag("optimize");
		int useDebugLibs  =  (!optimizeSize && !optimizeSpeed);

		prj_select_config(i);

		io_print("!%s  \"$(CFG)\" == \"%s - Win32 %s\"\n\n", 
			(i == (prj_get_numconfigs() - 1) ? "IF" : "ELSEIF"), prj_get_pkgname(), prj_get_cfgname());

		io_print("# PROP BASE Use_MFC 0\n");
		io_print("# PROP BASE Use_Debug_Libraries %d\n", useDebugLibs ? 1 : 0);
		io_print("# PROP BASE Output_Dir \"%s\"\n", prj_get_outdir());
		io_print("# PROP BASE Intermediate_Dir \"%s\"\n", prj_get_objdir());
		io_print("# PROP BASE Target_Dir \"\"\n");
		io_print("# PROP Use_MFC 0\n");
		io_print("# PROP Use_Debug_Libraries %d\n", useDebugLibs ? 1 : 0);
		io_print("# PROP Output_Dir \"%s\"\n", prj_get_outdir());
		io_print("# PROP Intermediate_Dir \"%s\"\n", prj_get_objdir());
		if (prj_is_kind("dll") && prj_has_flag("no-import-lib"))
			io_print("# PROP Ignore_Export_Lib 1\n");
		io_print("# PROP Target_Dir \"\"\n");

		io_print("# ADD BASE CPP /nologo");
		writeCppFlags();
		io_print("# ADD CPP /nologo");
		writeCppFlags();

		debugSymbol = prj_has_flag("no-symbols") ? "NDEBUG" : "_DEBUG";
		if (prj_is_kind("winexe") || prj_is_kind("dll"))
		{
			io_print("# ADD BASE MTL /nologo /D \"%s\" /mktyplib203 /win32\n", debugSymbol);
			io_print("# ADD MTL /nologo /D \"%s\" /mktyplib203 /win32\n", debugSymbol);
		}

		io_print("# ADD BASE RSC /l 0x409 /d \"%s\"\n", debugSymbol);
		io_print("# ADD RSC /l 0x409 /d \"%s\"\n", debugSymbol);
		io_print("BSC32=bscmake.exe\n");
		io_print("# ADD BASE BSC32 /nologo\n");
		io_print("# ADD BSC32 /nologo\n");

		if (prj_is_kind("lib"))
		{
			io_print("LINK32=link.exe -lib\n");
			io_print("# ADD BASE LIB32 /nologo\n");
			io_print("# ADD LIB32 /nologo\n");
		}
		else
		{
			io_print("LINK32=link.exe\n");
			io_print("# ADD BASE LINK32");
			writeLinkFlags();
			io_print("# ADD LINK32");
			writeLinkFlags();
		}

		io_print("\n");
	}

	io_print("!ENDIF\n");
	io_print("\n");
	io_print("# Begin Target\n");
	io_print("\n");

	for (i = prj_get_numconfigs() - 1; i >= 0; --i)
	{
		prj_select_config(i);
		io_print("# Name \"%s - Win32 %s\"\n", prj_get_pkgname(), prj_get_cfgname());
	}

	print_source_tree("", listFiles);

	io_print("# End Target\n");
	io_print("# End Project\n");

	io_closefile();
	return 1;
}


/************************************************************************
 * Writes compiler flags for current configuration
 ***********************************************************************/

static void writeCppFlags()
{
	int optimizeSize  =  prj_has_flag("optimize-size");
	int optimizeSpeed =  prj_has_flag("optimize-speed") || prj_has_flag("optimize");
	int useDebugLibs  =  (!optimizeSize && !optimizeSpeed);

	if (useDebugLibs)
		io_print(prj_has_flag("static-runtime") ? " /MTd" : " /MDd");
	else
		io_print(prj_has_flag("static-runtime") ? " /MT" : " /MD");
	
	io_print(" /W%d", prj_has_flag("extra-warnings") ? 4 : 3);
	
	if (prj_has_flag("fatal-warnings"))
		io_print(" /WX");
	
	if (useDebugLibs)
		io_print(" /Gm");  /* minimal rebuild */
	
	if (!prj_has_flag("no-rtti"))
		io_print(" /GR");
	
	if (!prj_has_flag("no-exceptions"))
		io_print(" /GX");
	
	if (!prj_has_flag("no-symbols"))
		io_print(" /ZI");  /* debug symbols for edit-and-continue */
	
	if (optimizeSize)
		io_print(" /O1");
	else if (optimizeSpeed)
		io_print(" /O2");
	else
		io_print(" /Od");
	
	if (prj_has_flag("no-frame-pointer"))
		io_print(" /Oy");
	
	print_list(prj_get_incpaths(), " /I \"", "\"", "", NULL);
	print_list(prj_get_defines(), " /D \"", "\"", "", NULL);
	
	io_print(" /YX /FD");
	
	if (!optimizeSize && !optimizeSpeed)
		io_print(" /GZ");
	
	io_print(" /c");
	
	print_list(prj_get_buildoptions(), " ", "", "", NULL);

	io_print("\n");
}


/************************************************************************
 * Writes linker flags for current configuration
 ***********************************************************************/

static void writeLinkFlags()
{
	print_list(prj_get_links(), " ", ".lib", "", filterLinks);
	io_print(" /nologo");

	if ((prj_is_kind("winexe") || prj_is_kind("exe")) && !prj_has_flag("no-main"))
		io_print(" /entry:\"mainCRTStartup\"");

	if (prj_is_kind("winexe"))
		io_print(" /subsystem:windows");
	else if (prj_is_kind("exe"))
		io_print(" /subsystem:console");
	else
		io_print(" /dll");

	if (!prj_has_flag("no-symbols"))
		io_print(" /incremental:yes /debug");

	io_print(" /machine:I386");

	if (prj_is_kind("dll"))
	{
		io_print(" /implib:\"");
		if (prj_has_flag("no-import-lib"))
			io_print(prj_get_objdir());
		else
			io_print(prj_get_libdir());
		io_print("/%s.lib\"", path_getbasename(prj_get_target()));
	}

	io_print(" /out:\"%s\"", prj_get_target());

	if (!prj_has_flag("no-symbols"))
		io_print(" /pdbtype:sept");

	io_print(" /libpath:\"%s\"", prj_get_libdir());
	print_list(prj_get_libpaths(), " /libpath:\"", "\"", "", NULL);
	print_list(prj_get_linkoptions(), " ", "", "", NULL);

	io_print("\n");
}


/************************************************************************
 * Checks each entry in the list of package links. If the entry refers
 * to a sibling package, returns the path to that package's output
 ***********************************************************************/

static const char* filterLinks(const char* name)
{
	int i = prj_find_package(name);
	if (i >= 0)
	{
		/* If this is a sibling package, don't return anything. VC6
		 * links to dependent projects implicitly */
		return NULL;
	}
	else
	{
		return name;
	}
}


/************************************************************************
 * Callback for print_source_tree()
 ***********************************************************************/

static void listFiles(const char* path, int stage)
{
	const char* ptr = strrchr(path, '/');
	ptr = (ptr == NULL) ? path : ptr + 1;

	switch (stage)
	{
	case WST_OPENGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0) {
			io_print("# Begin Group \"%s\"\n\n", ptr);
			io_print("# PROP Default_Filter \"\"\n");
		}
		break;
	case WST_CLOSEGROUP:
		if (strlen(path) > 0 && strcmp(ptr, "..") != 0) 
			io_print("# End Group\n");
		break;
	case WST_SOURCEFILE:
		io_print("# Begin Source File\n\n");
		io_print("SOURCE=%s\n", path);
		io_print("# End Source File\n");
		break;
	}
}

