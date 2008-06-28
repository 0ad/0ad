/**********************************************************************
 * Premake - vs.c
 * Common code for Visual Studio 2002-2008 targets.
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
#include <stdarg.h>
#include "premake.h"
#include "vs.h"

static int version;

#define S_TRUE  (version < VS2005 ? "TRUE" : "true")
#define S_FALSE (version < VS2005 ? "FALSE" : "false")

enum Blocks
{
	BlocksEnd,
	VCCLCompilerTool,
	VCCustomBuildTool,
	VCLinkerTool,
	VCMIDLTool,
	VCPostBuildEventTool,
	VCPreBuildEventTool,
	VCPreLinkEventTool,
	VCResourceCompilerTool,
	VCWebServiceProxyGeneratorTool,
	VCXMLDataGeneratorTool,
	VCWebDeploymentTool,
	VCManagedWrapperGeneratorTool,
	VCAuxiliaryManagedWrapperGeneratorTool,
	VCManagedResourceCompilerTool,
	VCALinkTool,
	VCManifestTool,
	VCXDCMakeTool,
	VCBscMakeTool,
	VCFxCopTool,
	VCAppVerifierTool
};

static int blocks_2002[] = 
{
	VCCLCompilerTool,
	VCCustomBuildTool,
	VCLinkerTool,
	VCMIDLTool,
	VCPostBuildEventTool,
	VCPreBuildEventTool,
	VCPreLinkEventTool,
	VCResourceCompilerTool,
	VCWebServiceProxyGeneratorTool,
	VCWebDeploymentTool,
	BlocksEnd
};

static int blocks_2003[] = 
{
	VCCLCompilerTool,
	VCCustomBuildTool,
	VCLinkerTool,
	VCMIDLTool,
	VCPostBuildEventTool,
	VCPreBuildEventTool,
	VCPreLinkEventTool,
	VCResourceCompilerTool,
	VCWebServiceProxyGeneratorTool,
	VCXMLDataGeneratorTool,
	VCWebDeploymentTool,
	VCManagedWrapperGeneratorTool,
	VCAuxiliaryManagedWrapperGeneratorTool,
	BlocksEnd
};

static int blocks_2005_2008[] =
{
	VCPreBuildEventTool,
	VCCustomBuildTool,
	VCXMLDataGeneratorTool,
	VCWebServiceProxyGeneratorTool,
	VCMIDLTool,
	VCCLCompilerTool,
	VCManagedResourceCompilerTool,
	VCResourceCompilerTool,
	VCPreLinkEventTool,
	VCLinkerTool,
	VCALinkTool,
	VCManifestTool,
	VCXDCMakeTool,
	VCBscMakeTool,
	VCFxCopTool,
	VCAppVerifierTool,
	VCWebDeploymentTool,
	VCPostBuildEventTool,
	BlocksEnd
};

static int* blocks[] =
{
	blocks_2002,
	blocks_2003,
	blocks_2005_2008,
	blocks_2005_2008
};


/************************************************************************
 * Helpers to hide XML formatting style differences
 ***********************************************************************/

static int indent = 0;
static int opened = 0;
static int attrib = 0;

static void tag_indent()
{
	int i;
	for (i = 0; i < indent; ++i)
		io_print("\t");
}

static void tag_open(const char* name)
{
	if (opened)
	{
		if (version >= VS2005 && attrib > 0)
		{
			io_print("\n");
			tag_indent();
		}
		io_print(">\n");
	}

	tag_indent();
	io_print("<");
	io_print(name);
	indent++;
	opened = 1;
	attrib = 0;
}

static void tag_attr(const char* attr, ...)
{
	va_list args;
	va_start(args, attr);
	vsprintf(g_buffer, attr, args);
	va_end(args);

	io_print("\n");
	tag_indent();
	io_print(g_buffer);
	attrib++;
}

static void tag_attr_open(const char* name)
{
	io_print("\n");
	tag_indent();
	io_print(name);
	io_print("=\"");
}

static tag_attr_close()
{
	io_print("\"");
	attrib++;
}

static void tag_close(const char* name, int form)
{
	indent--;
	if (form == 1)
	{
		if (opened)
			io_print(">\n");
		tag_indent();
		io_print("</");
		io_print(name);
		io_print(">\n");
	}
	else
	{
		if (version >= VS2005)
		{
			io_print("\n");
			tag_indent();
		}
		io_print("/>\n");
	}
	opened = 0;
}


/************************************************************************
 * Store the current version of VS.NET being targeted
 ***********************************************************************/

void vs_setversion(int ver)
{
	version = ver;
}

int vs_getversion()
{
	return version;
}



/************************************************************************
 * Assign GUIDs to each package by type
 ***********************************************************************/

void vs_assign_guids()
{
	int p;
	for (p = 0; p < prj_get_numpackages(); ++p)
	{
		VsPkgData* data = ALLOCT(VsPkgData);
		prj_select_package(p);
		prj_set_data(data);
	
		generateUUID(data->projGuid);

		prj_select_config(0);
		if (version >= VS2005 && prj_is_kind("aspnet"))
		{
			strcpy(data->toolGuid, "E24C65DC-7377-472B-9ABA-BC803B73C61A");
			strcpy(data->projExt, "");
			strcpy(data->projType, "");
		}
		else if (prj_is_lang("c++") || prj_is_lang("c"))
		{
			strcpy(data->toolGuid, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942");
			strcpy(data->projExt, "vcproj");
			strcpy(data->projType, "Win32");
		}
		else if (prj_is_lang("c#"))
		{
			strcpy(data->toolGuid, "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC");
			strcpy(data->projExt, "csproj");
			if (version < VS2005)
				strcpy(data->projType, ".NET");
			else
				strcpy(data->projType, "Any CPU");
		}

		data->numDependencies = 0;
	}
}


/************************************************************************
 * Write out a C++ project file
 ***********************************************************************/

int vs_write_cpp()
{
	const char* str;
	int i, b;

	VsPkgData* data = (VsPkgData*)prj_get_data();

	/* Open the file and write the header */
	if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "vcproj")))
		return 0;

	prj_select_config(0);
	io_print("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\n");
	
	tag_open("VisualStudioProject");
	tag_attr("ProjectType=\"Visual C++\"");

	switch (version)
	{
	case VS2002:
		str = "7.00";
		break;
	case VS2003:
		str = "7.10";
		break;
	case VS2005:
		str = "8.00";
		break;
	case VS2008:
		str = "9.00";
		break;
	}
	tag_attr("Version=\"%s\"", str);

	tag_attr("Name=\"%s\"", prj_get_pkgname());
	tag_attr("ProjectGUID=\"{%s}\"", data->projGuid);

	if (version >= VS2005)
		tag_attr("RootNamespace=\"%s\"", prj_get_pkgname());

	tag_attr("Keyword=\"%s\"", prj_has_flag("managed") ? "ManagedCProj" : "Win32Proj");  

	tag_open("Platforms");
	tag_open("Platform");
	tag_attr("Name=\"Win32\"");
	tag_close("Platform", 0);
	tag_close("Platforms", 1);

	if (version >= VS2005)
	{
		tag_open("ToolFiles");
		tag_close("ToolFiles", 1);
	}

	/* Write configurations */
	tag_open("Configurations");
	for (i = 0; i < prj_get_numconfigs(); ++i)
	{
		int optimization, debug, runtime, symbols, configTypeId;
		const char* pchHeader = prj_get_pch_header();

		prj_select_config(i);

		if (prj_is_kind("winexe") || prj_is_kind("exe") || prj_is_kind("cxxtestgen") || prj_is_kind("run"))
		{
			configTypeId = 1;
		}
		else if (prj_is_kind("dll"))
		{
			configTypeId = 2;
		}
		else if (prj_is_kind("lib"))
		{
			configTypeId = 4;
		}
		else if (prj_is_kind("aspnet"))
		{
			puts("** Error: C++ ASP.NET projects are not supported");
			return 0;
		}
		else
		{
			printf("** Error: unknown package kind '%s'\n", prj_get_kind());
			return 0;
		}

		if (prj_has_flag("optimize-speed"))
			optimization = 2;
		else if (prj_has_flag("optimize-size"))
			optimization = 1;
		else if (prj_has_flag("optimize"))
			optimization = 3;
		else
			optimization = 0;

		debug = (optimization ==0);

		if (prj_has_flag("static-runtime"))
			runtime = (debug) ? 1 : 0;
		else
			runtime = (debug) ? 3 : 2;

		if (prj_has_flag("no-symbols"))
			symbols = 0;
		else
			symbols = (prj_has_flag("managed") || prj_has_flag("no-edit-and-continue")) ? 3 : 4;

		tag_open("Configuration");
		tag_attr("Name=\"%s|Win32\"", prj_get_cfgname());
		tag_attr("OutputDirectory=\"%s\"", prj_get_outdir());
		tag_attr("IntermediateDirectory=\"%s\"", prj_get_objdir());
		tag_attr("ConfigurationType=\"%d\"", configTypeId);
		tag_attr("CharacterSet=\"%d\"", prj_has_flag("unicode") ? 1 : 2);
		if (prj_has_flag("managed")) 
			tag_attr("ManagedExtensions=\"%s\"", S_TRUE);

		/* Write out tool blocks */
		for (b = 0; blocks[version][b] != BlocksEnd; ++b)
		{
			tag_open("Tool");
			switch (blocks[version][b])
			{
			case VCPreBuildEventTool:
            tag_attr("Name=\"VCPreBuildEventTool\"");
				break;
			case VCCustomBuildTool:
            tag_attr("Name=\"VCCustomBuildTool\"");
				if (prj_is_kind("run"))
				{
					tag_attr("Description=\"Running CxxTest Test Suite\"");
					tag_attr_open("CommandLine");
					print_list(prj_get_links(), "", "", " ", vs_filter_links);
					print_list(prj_get_buildoptions(), " ", "", "", NULL);
					tag_attr_close();
					// This so that we're always run (and never "up-to-date")
					tag_attr("Outputs=\".\\dummy.file.thats.never.created\"");
				}
				else if (prj_is_kind("cxxtestgen"))
				{
					char *rootfile = strdup(prj_get_cxxtest_rootfile());
					char *ctpath = strdup(path_translate(prj_get_cxxtestpath(), "windows"));
					char *options = strdup(prj_get_cxxtest_rootoptions());

					tag_attr("Description=\"Generating test_root.cpp\"");
					tag_attr("CommandLine=\"%s%s --root %s -o &quot;%s&quot;\"",
						endsWith(prj_get_cxxtestpath(), ".pl")?"perl ":"",
						ctpath,
						options,
						rootfile);

					tag_attr("Outputs=\"%s\"", rootfile);

					free(options);
					free(ctpath);
					free(rootfile);
				}
				break;
			case VCXMLDataGeneratorTool:
            tag_attr("Name=\"VCXMLDataGeneratorTool\"");
				break;
			case VCManagedWrapperGeneratorTool:
				tag_attr("Name=\"VCManagedWrapperGeneratorTool\"");
				break;
			case VCAuxiliaryManagedWrapperGeneratorTool:
				tag_attr("Name=\"VCAuxiliaryManagedWrapperGeneratorTool\"");
				break;
			case VCWebServiceProxyGeneratorTool:
            tag_attr("Name=\"VCWebServiceProxyGeneratorTool\"");
				break;
			case VCMIDLTool:
            tag_attr("Name=\"VCMIDLTool\"");
				break;
			case VCManagedResourceCompilerTool:
            tag_attr("Name=\"VCManagedResourceCompilerTool\"");
				break;
			case VCResourceCompilerTool:
				tag_attr("Name=\"VCResourceCompilerTool\"");
				if (prj_get_numincpaths() > 0)
				{
					tag_attr_open("AdditionalIncludeDirectories");
					print_list(prj_get_incpaths(), "", "", ";", NULL);
					tag_attr_close();
				}
				break;
			case VCPreLinkEventTool:
            tag_attr("Name=\"VCPreLinkEventTool\"");
				break;
			case VCALinkTool:
            tag_attr("Name=\"VCALinkTool\"");
				break;
			case VCManifestTool:
            tag_attr("Name=\"VCManifestTool\"");
				break;
			case VCXDCMakeTool:
            tag_attr("Name=\"VCXDCMakeTool\"");
				break;
			case VCBscMakeTool:
            tag_attr("Name=\"VCBscMakeTool\"");
				break;
			case VCFxCopTool:
            tag_attr("Name=\"VCFxCopTool\"");
				break;
			case VCAppVerifierTool:
            tag_attr("Name=\"VCAppVerifierTool\"");
				break;
			case VCWebDeploymentTool:
            tag_attr("Name=\"VCWebDeploymentTool\"");
				break;
			case VCPostBuildEventTool:
				tag_attr("Name=\"VCPostBuildEventTool\"");
				break;
			case VCCLCompilerTool:
				tag_attr("Name=\"VCCLCompilerTool\"");

				if (prj_get_numbuildoptions() > 0)
				{
					tag_attr_open("AdditionalOptions");
					print_list(prj_get_buildoptions(), "", "", " ", NULL);
					tag_attr_close();
				}

				tag_attr("Optimization=\"%d\"", optimization);

				if (prj_has_flag("no-frame-pointer")) 
					tag_attr("OmitFramePointers=\"%s\"", S_TRUE);

				if (prj_get_numincpaths() > 0)
				{
					tag_attr_open("AdditionalIncludeDirectories");
					print_list(prj_get_incpaths(), "", "", ";", NULL);
					tag_attr_close();
				}

				if (prj_has_flag("managed"))
					tag_attr("AdditionalUsingDirectories=\"%s\"", prj_get_bindir());

				if (prj_get_numdefines() > 0)
				{
					tag_attr_open("PreprocessorDefinitions");
					print_list(prj_get_defines(), "", "", ";", NULL);
					tag_attr_close();
				}

				if (debug && !prj_has_flag("managed"))
					tag_attr("MinimalRebuild=\"%s\"", S_TRUE);

				if (prj_has_flag("no-exceptions")) 
					tag_attr("ExceptionHandling=\"%s\"", S_FALSE);

				if (debug && !prj_has_flag("managed"))
					tag_attr("BasicRuntimeChecks=\"3\"");
				
				if (!debug) 
					tag_attr("StringPooling=\"%s\"", S_TRUE);
				
				tag_attr("RuntimeLibrary=\"%d\"", runtime);
				tag_attr("EnableFunctionLevelLinking=\"%s\"", S_TRUE);

				if (version < VS2005 && !prj_has_flag("no-rtti"))
					tag_attr("RuntimeTypeInfo=\"%s\"", S_TRUE);
				if (version >= VS2005 && prj_has_flag("no-rtti"))
					tag_attr("RuntimeTypeInfo=\"%s\"", S_FALSE);

				if (version < VS2005 && prj_has_flag("native-wchar_t"))
					tag_attr("TreatWChar_tAsBuiltInType=\"%s\"", S_TRUE);
				if (version >= VS2005 && prj_has_flag("no-native-wchar_t"))
					tag_attr("TreatWChar_tAsBuiltInType=\"%s\"", S_FALSE);

				if (pchHeader)
				{
					tag_attr("UsePrecompiledHeader=\"%d\"", version < VS2005 ? 3 : 2);
					tag_attr("PrecompiledHeaderThrough=\"%s\"", pchHeader);
				}
				else
					tag_attr("UsePrecompiledHeader=\"%d\"", version < VS2005 ? 2 : 0);

				tag_attr("WarningLevel=\"%d\"", prj_has_flag("extra-warnings") ? 4 : 3);
				if (prj_has_flag("fatal-warnings"))
					tag_attr("WarnAsError=\"%s\"", S_TRUE);
				if (version < VS2008 && !prj_has_flag("managed")) 
					tag_attr("Detect64BitPortabilityProblems=\"%s\"", prj_has_flag("no-64bit-checks") ? S_FALSE : S_TRUE);

				tag_attr("DebugInformationFormat=\"%d\"", symbols);
				break;
			case VCLinkerTool:
				if (!prj_is_kind("lib"))
				{
					tag_attr("Name=\"VCLinkerTool\"");
					if (prj_has_flag("no-import-lib"))
						tag_attr("IgnoreImportLibrary=\"%s\"", S_TRUE);

					if (prj_has_flag("use-library-dep-inputs") && version >= VS2005)
						tag_attr("UseLibraryDependencyInputs=\"%s\"", S_TRUE);

					if (prj_get_numlinkoptions() > 0)
					{
						tag_attr_open("AdditionalOptions");
						print_list(prj_get_linkoptions(), " ", "", "", NULL);
						tag_attr_close();
					}

					if (prj_get_numlinks() > 0)
					{
						tag_attr_open("AdditionalDependencies");
						print_list(prj_get_links(), "", ".lib", " ", vs_filter_links);
						tag_attr_close();
					}

					tag_attr("OutputFile=\"$(OutDir)/%s\"", path_getname(prj_get_target()));
					tag_attr("LinkIncremental=\"%d\"", debug ? 2 : 1);

					tag_attr_open("AdditionalLibraryDirectories");
					io_print(prj_get_libdir());
					print_list(prj_get_libpaths(), ";", "", "", NULL);
					tag_attr_close();

					/* Look for a .def file for DLLs */
					if (prj_find_filetype(".def") != NULL)
						tag_attr("ModuleDefinitionFile=\"%s\"", prj_find_filetype(".def"));

					if (prj_has_flag("no-manifest"))
						tag_attr("GenerateManifest=\"%s\"", S_FALSE);

					tag_attr("GenerateDebugInformation=\"%s\"", symbols ? S_TRUE : S_FALSE);
					if (symbols) 
						tag_attr("ProgramDatabaseFile=\"$(OutDir)/%s.pdb\"", path_getbasename(prj_get_target()));

					tag_attr("SubSystem=\"%d\"", prj_is_kind("exe") ? 1 : 2);
					if (!debug) tag_attr("OptimizeReferences=\"2\"");
					if (!debug) tag_attr("EnableCOMDATFolding=\"2\"");

					if ((prj_is_kind("exe") || prj_is_kind("winexe")) && !prj_has_flag("no-main"))
					{
						tag_attr("EntryPointSymbol=\"mainCRTStartup\"");
					}
					else if (prj_is_kind("dll")) 
					{
						tag_attr_open("ImportLibrary");
						if (prj_has_flag("no-import-lib"))
							io_print(prj_get_objdir());
						else
							io_print(prj_get_libdir());
						io_print("/%s.lib", path_getbasename(prj_get_target()));
						tag_attr_close();
					}
					tag_attr("TargetMachine=\"1\"");
				}
				else
				{
					tag_attr("Name=\"VCLibrarianTool\"");
					tag_attr("OutputFile=\"$(OutDir)/%s.lib\"", path_getbasename(prj_get_target()));
				}
				break;
			}
			tag_close("", 0);
		}

		tag_close("Configuration", 1);
	}
	tag_close("Configurations", 1);

	if (version > VS2002)
	{
		tag_open("References");
		tag_close("References", 1);
	}

	tag_open("Files");
	print_source_tree("", vs_list_files);
	/*if (prj_is_kind("cxxtestgen"))
	{
		char *rootfile = strdup(prj_get_cxxtest_rootfile());
		char *ctpath = strdup(path_translate(prj_get_cxxtestpath(), "windows"));
		int i;

		tag_open("File");
		tag_attr_open("RelativePath");
		if (rootfile[0] != '.')
			io_print(".\\");
		io_print(rootfile);
		tag_attr_close();

		for (i = 0; i < prj_get_numconfigs(); ++i)
		{
			char *options;
			prj_select_config(i);

			options = strdup(prj_get_cxxtest_rootoptions());

			tag_open("FileConfiguration");
			tag_attr("Name=\"%s|Win32\"", prj_get_cfgname());

			tag_open("Tool");
			tag_attr("Name=\"VCCustomBuildTool\"");
			tag_attr("Description=\"Generating test_root.cpp\"");
			tag_attr("CommandLine=\"%s%s --root %s -o &quot;%s&quot;\"",
				endsWith(prj_get_cxxtestpath(), ".pl")?"perl ":"",
				ctpath,
				options,
				rootfile);

			tag_attr("Outputs=\"%s\"", rootfile);
			tag_close("Tool", 0);
			tag_close("FileConfiguration", 1);

			free(options);
		}
		tag_close("File", 1);
		free(ctpath);
		free(rootfile);
	}*/
	tag_close("Files", 1);

	tag_open("Globals");
	tag_close("Globals", 1);
	tag_close("VisualStudioProject", 1);

	io_closefile();
	return 1;
}


/************************************************************************
 * Checks each entry in the list of package links. If the entry refers
 * to a sibling package, returns the path to that package's output
 ***********************************************************************/

const char* vs_filter_links(const char* name)
{
	int i = prj_find_package(name);
	if (i >= 0)
	{
		const char* lang = prj_get_language_for(i);
		const char* kind = prj_get_config_for(i)->kind;
		if (matches(kind, "cxxtestgen"))
			return NULL;
		// note that "run" matches against the /current/ project, not the link
		// dependency we've been run for
		else if (prj_is_kind("run"))
		{
			return path_translate(prj_get_target_for(i), "windows");
		}
		else if (matches(lang, "c") || matches(lang, "c++"))
		{
			strcpy(g_buffer, prj_get_libdir_for(i));
			return path_combine(g_buffer, prj_get_targetname_for(i));
		}
		else
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

void vs_list_files(const char* path, int stage)
{
	int i;
	const char* pchHeader = prj_get_pch_header();
	const char* pchSource = prj_get_pch_source();
	const char* trimPrefix = prj_get_trimprefix();

	const char* ptr = path;
	while (strncmp(ptr, "../", 3) == 0)
		ptr += 3;

	ptr = strchr(ptr, '/');
	while (ptr != NULL) 
		ptr = strchr(ptr + 1, '/');

	ptr = strrchr(path, '/');
	ptr = (ptr == NULL) ? (char*)path : ptr + 1;

	switch (stage)
	{
	case WST_OPENGROUP:
		if (strlen(path) > 0 && !matches(ptr, "..") && (!trimPrefix || strncmp(path, trimPrefix, strlen(trimPrefix)) == 0))
		{
			tag_open("Filter");
			tag_attr("Name=\"%s\"", ptr);
			tag_attr("Filter=\"\"");
		}
		break;

	case WST_CLOSEGROUP:
		if (strlen(path) > 0 && !matches(ptr, "..") && (!trimPrefix || strncmp(path, trimPrefix, strlen(trimPrefix)) == 0))
			tag_close("Filter", 1);
		break;

	case WST_SOURCEFILE:
		tag_open("File");
		tag_attr_open("RelativePath");
		if (path[0] != '.')
			io_print(".\\");
		io_print(path_translate(path, "windows"));
		tag_attr_close();

		/* Add FileConfiguration section if this is a special PCH file, or
		   if it's a custom-built .asm */
		if (matches(path_getname(path), pchSource) || endsWith(path, ".asm")
			|| (prj_is_kind("cxxtestgen") && endsWith(path, ".h")))
		{
			for (i = 0; i < prj_get_numconfigs(); ++i)
			{
				prj_select_config(i);

				tag_open("FileConfiguration");
				tag_attr("Name=\"%s|Win32\"", prj_get_cfgname());

				if (endsWith(path, ".asm"))
				{
					tag_open("Tool");
					tag_attr("Name=\"VCCustomBuildTool\"");
					tag_attr("Description=\"Assembling $(InputPath)\"");

					{
					// note: copy into our strings because path_* use static buffers.
					char nasm_path[512];	// PATH_MAX isn't defined
					char input_dir[512];
					strcpy(nasm_path, path_translate(prj_get_nasmpath(), "windows"));
					// note: NASM chokes on -i"path" and says "no input file".
					// we therefore use the *relative* input directory, not VC's $(InputDir) -
					// this is more likely not to contain a space in directory name.
					strcpy(input_dir, path_translate(path_getdir(path) , "windows"));
					// required - NASM just concatenates include path and filename.
					strcat(input_dir, "\\");
					tag_attr("CommandLine=\"%s -i %s -f win32 &quot;$(InputPath)&quot; -o &quot;$(IntDir)\\$(InputName).obj&quot;\"",
						nasm_path, input_dir);
					}
					tag_attr("Outputs=\"$(IntDir)\\$(InputName).obj\"");
					tag_close("Tool", 0);
				}
				else if (prj_is_kind("cxxtestgen") && endsWith(path, ".h"))
				{
					char *targetname = strdup(path_swapextension(path, ".h", ".cpp"));
					char *testoptions = strdup(prj_get_cxxtest_options());
					
					tag_open("Tool");
					tag_attr("Name=\"VCCustomBuildTool\"");
					tag_attr("Description=\"Generating %s\"", targetname);
					tag_attr("CommandLine=\"%s%s --part %s -o &quot;%s&quot; &quot;$(InputPath)&quot;\"",
						endsWith(prj_get_cxxtestpath(), ".pl")?"perl ":"",
						path_translate(prj_get_cxxtestpath(), "windows"),
						testoptions,
						targetname);
					tag_attr("Outputs=\"%s\"", targetname);
					tag_close("Tool", 0);

					free(targetname);
					free(testoptions);
				}
				else /* (.asm doesn't need PCH) */
				{
					tag_open("Tool");
					tag_attr("Name=\"VCCLCompilerTool\"");
					tag_attr("UsePrecompiledHeader=\"1\"");
					tag_close("Tool", 0);
				}
				tag_close("FileConfiguration", 1);
			}
		}

		tag_close("File", 1);

		break;
	}
}


/************************************************************************
 * List callback: scans the list of links for a package. If a link is
 * found to a sibling package, return a dependency string for the 
 * solution file. 
 ***********************************************************************/

const char* vs_list_pkgdeps(const char* name)
{
	int i;
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		if (matches(prj_get_pkgname_for(i), name))
		{
			VsPkgData* data = (VsPkgData*)prj_get_data_for(i);
			if (version > VS2002)
			{
				sprintf(g_buffer, "{%s} = {%s}", data->projGuid, data->projGuid);
			}
			else
			{
				VsPkgData* src = (VsPkgData*)prj_get_data();
				sprintf(g_buffer, "{%s}.%d = {%s}", src->projGuid, src->numDependencies, data->projGuid);
				++(src->numDependencies);
			}
			return g_buffer;
		}
	}

	return NULL;
}


/************************************************************************
 * VS.NET requires that all reference search paths be absolute
 ***********************************************************************/

const char* vs_list_refpaths(const char* name)
{
	char* path = (char*)path_absolute(name);
	path_translateInPlace(path, "windows");
	return path;
}
