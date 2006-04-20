/**********************************************************************
 * Premake - vs2005_cs.c
 * The Visual Studio 2005 C# target
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
#include <string.h>
#include "premake.h"
#include "vs.h"
#include "vs2005.h"

static const char* listFiles(const char* name);
static const char* listReferences(const char* name);


int vs2005_cs()
{
	VsPkgData* data = (VsPkgData*)prj_get_data();
	int c;

	if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj")))
		return 0;

	io_print("<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
	io_print("  <PropertyGroup>\n");

	/* Write default configuration */
	prj_select_config(0);
	io_print("    <Configuration Condition=\" '$(Configuration)' == '' \">%s</Configuration>\n", prj_get_cfgname());
	io_print("    <Platform Condition=\" '$(Platform)' == '' \">AnyCPU</Platform>\n");
	io_print("    <ProductVersion>8.0.50727</ProductVersion>\n");
	io_print("    <SchemaVersion>2.0</SchemaVersion>\n");
	io_print("    <ProjectGuid>{%s}</ProjectGuid>\n", data->projGuid);

	io_print("    <OutputType>");
	if (prj_is_kind("winexe"))
		io_print("WinExe");
	else if (prj_is_kind("exe"))
		io_print("Exe");
	else if (prj_is_kind("dll") || prj_is_kind("aspnet"))
		io_print("Library");
	else
	{
		printf("** Error: unknown package kind '%s'\n", prj_get_kind());
		return 0;
	}
	io_print("</OutputType>\n");

	io_print("    <AppDesignerFolder>Properties</AppDesignerFolder>\n");
	io_print("    <RootNamespace>%s</RootNamespace>\n", path_getbasename(prj_get_target()));
	io_print("    <AssemblyName>%s</AssemblyName>\n", path_getbasename(prj_get_target()));
	io_print("  </PropertyGroup>\n");

	for (c = 0; c < prj_get_numconfigs(); ++c)
	{
		prj_select_config(c);

		io_print("  <PropertyGroup Condition=\" '$(Configuration)|$(Platform)' == '%s|AnyCPU' \">\n", prj_get_cfgname());

		if (!prj_has_flag("no-symbols"))
		{
			io_print("    <DebugSymbols>true</DebugSymbols>\n");
			io_print("    <DebugType>full</DebugType>\n");
		}
		else
		{
			io_print("    <DebugType>pdbonly</DebugType>\n");
		}

		if (prj_has_flag("optimize") || prj_has_flag("optimize-size") || prj_has_flag("optimize-speed"))
			io_print("    <Optimize>true</Optimize>\n");
		else
			io_print("    <Optimize>false</Optimize>\n");

		io_print("    <OutputPath>%s\\</OutputPath>\n", prj_get_outdir());

		io_print("    <DefineConstants>");
		print_list(prj_get_defines(), "", "", ";", NULL);
		io_print("</DefineConstants>\n");

		io_print("    <ErrorReport>prompt</ErrorReport>\n");
		io_print("    <WarningLevel>4</WarningLevel>\n");

		if (prj_has_flag("unsafe"))
			io_print("    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>\n");

		if (prj_has_flag("fatal-warnings"))
			io_print("    <TreatWarningsAsErrors>true</TreatWarningsAsErrors>\n");

		io_print("  </PropertyGroup>\n");
	}

	/* Must use same references for all configurations */
	prj_select_config(0);
	io_print("  <ItemGroup>\n");
	print_list(prj_get_links(), "", "", "", listReferences);
	io_print("  </ItemGroup>\n");

	io_print("  <ItemGroup>\n");
	print_list(prj_get_files(), "", "", "", listFiles);
	io_print("  </ItemGroup>\n");
	
	io_print("  <Import Project=\"$(MSBuildBinPath)\\Microsoft.CSharp.targets\" />\n");
	io_print("  <!-- To modify your build process, add your task inside one of the targets below and uncomment it. \n");
	io_print("       Other similar extension points exist, see Microsoft.Common.targets.\n");
	io_print("  <Target Name=\"BeforeBuild\">\n");
	io_print("  </Target>\n");
	io_print("  <Target Name=\"AfterBuild\">\n");
	io_print("  </Target>\n");
	io_print("  -->\n");
	io_print("</Project>\n");

	io_closefile();

	/* Now write the .csproj.user file for non-web applications or
	 * .csproj.webinfo for web applications */
	if (!prj_is_kind("aspnet"))
	{
		if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj.user")))
			return 0;

		io_print("<Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
		io_print("  <PropertyGroup>\n");
		io_print("    <ReferencePath>");

		strcpy(g_buffer, io_getcwd());
		io_chdir(prj_get_pkgpath());
		print_list(prj_get_libpaths(), "", ";", "", vs_list_refpaths);
		io_print(vs_list_refpaths(prj_get_bindir()));
		io_chdir(g_buffer);

		io_print("</ReferencePath>\n");
		io_print("  </PropertyGroup>\n");
		io_print("</Project>\n");
	}
	else
	{
		if (!io_openfile(path_join(prj_get_pkgpath(), prj_get_pkgname(), "csproj.webinfo")))
			return 0;
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
	/* Is this a sibling package? */
	int i = prj_find_package(name);
	if (i >= 0)
	{
		VsPkgData* data = (VsPkgData*)prj_get_data_for(i);

		strcpy(g_buffer, path_build(prj_get_pkgpath(), prj_get_pkgpath_for(i)));
		io_print("    <ProjectReference Include=\"%s\\%s.csproj\">\n", path_translate(g_buffer, "windows"), name);
		io_print("      <Project>{%s}</Project>\n", data->projGuid);
		io_print("      <Name>%s</Name>\n", name);
		io_print("    </ProjectReference>\n");
	}
	else
	{
		io_print("    <Reference Include=\"%s\" />\n", name);
	}

	return NULL;
}


/************************************************************************
 * Builds an entry for each file in the project
 ***********************************************************************/

static const char* listFiles(const char* name)
{
	const char* related;

	/* If a build action was specified, use it */
	prj_select_file(name);
	if (prj_get_buildaction() != NULL)
	{
		if (matches(prj_get_buildaction(), "Content"))
		{
			io_print("    <Content Include=\"%s\">\n", path_translate(name, "windows"));
			io_print("      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>\n");
			io_print("    </Content>\n");
		}
		else
		{
			io_print("    <%s Include=\"%s\" />\n", prj_get_buildaction(), path_translate(name, "windows"));
		}
		return NULL;
	}

	if (endsWith(name, ".cs"))
	{
		int fullstop = 0;
		io_print("    <Compile Include=\"%s\"", path_translate(name, "windows"));

		/* If this is a ".Designer.cs" file... */
		if (endsWith(name, ".Designer.cs"))
		{
			/* ...look for a .cs dependency */
			strcpy(g_buffer, path_swapextension(name, ".Designer.cs", ".cs"));
			if (prj_has_file(g_buffer))
			{
				io_print(">\n      <DependentUpon>%s</DependentUpon>\n", path_getname(g_buffer));
				fullstop = 1;
			}
			else
			{
				/* ...look for a .resx dependency */
				strcpy(g_buffer, path_swapextension(name, ".Designer.cs", ".resx"));
				if (prj_has_file(g_buffer))
				{
					io_print(">\n      <AutoGen>True</AutoGen>\n");
					io_print("      <DependentUpon>%s</DependentUpon>\n", path_getname(g_buffer));
					fullstop = 1;
				}
			}
		}
		else
		{
			/* If a matching ".Designer.cs" exists, mark this as a Form */
			related = path_swapextension(name, ".cs", ".Designer.cs");
			if (prj_has_file(related))
			{
				io_print(">\n      <SubType>Form</SubType>\n");
				fullstop = 1;
			}
		}
		io_print(fullstop ? "    </Compile>\n" : " />\n");
	}
	else if (endsWith(name, ".resx"))
	{
		int fullstop = 0;
		io_print("    <EmbeddedResource Include=\"%s\"", path_translate(name, "windows"));
		
		/* If a matching .cs file exists, link it */
		strcpy(g_buffer, path_swapextension(name, ".resx", ".cs"));
		if (prj_has_file(g_buffer))
		{
			fullstop = 1;
			io_print(">\n");

			/* Is this related to a form? */
			related = path_swapextension(name, ".resx", ".Designer.cs");
			if (prj_has_file(related))
				io_print("      <SubType>Designer</SubType>\n");
			io_print("      <DependentUpon>%s</DependentUpon>\n", path_getname(g_buffer));
		}
		else 
		{
			/* If no .cs but .Designer.cs then auto-generated */
			strcpy(g_buffer, path_swapextension(name, ".resx", ".Designer.cs"));
			if (prj_has_file(g_buffer))
			{
				fullstop = 1;
				io_print(">\n");

				io_print("      <SubType>Designer</SubType>\n");
				io_print("      <Generator>ResXFileCodeGenerator</Generator>\n");
				io_print("      <LastGenOutput>%s</LastGenOutput>\n", path_getname(g_buffer));
			}
		}
		io_print(fullstop ? "    </EmbeddedResource>\n" : " />\n");
	}
	else
	{
		io_print("    <None Include=\"%s\" />\n", path_translate(name, "windows"));
	}

	return NULL;
}

