/**********************************************************************
 * Premake - sharpdev.c
 * The SharpDevelop and MonoDevelop targets
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

int sharpdev_target;
int sharpdev_warncontent;

static int writeCombine();

int sharpdev_generate(const char* targetName)
{
	int i;

	sharpdev_target = matches(targetName, "monodev") ? MONODEV : SHARPDEV;
	sharpdev_warncontent = 0;

	printf("Generating %sDevelop combine and project files:\n", (sharpdev_target == SHARPDEV) ? "Sharp" : "Mono");

	if (!writeCombine())
		return 0;

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);

		printf("...%s\n", prj_get_pkgname());
		if (prj_is_lang("c#"))
		{
			if (!sharpdev_cs())
				return 0;
		}
		else if (prj_is_lang("c++") || prj_is_lang("c"))
		{
			printf("** Error: this generator does not support C/C++ development.\n");
			return 0;
		}
		else
		{
			printf("** Error: unrecognized language '%s'\n", prj_get_language());
			return 0;
		}
	}

	if (sharpdev_warncontent)
	{
		puts("\n** Warning: this project uses the 'Content' build action. This action is not");
		puts("            supported by #develop; some manual configuration may be needed.");
	}

	return 1;
}


static int writeCombine()
{
	const char* path;
	int i, j;

	if (!io_openfile(path_join(prj_get_path(), prj_get_name(), "cmbx")))
		return 0;

	io_print("<Combine fileversion=\"1.0\" name=\"%s\" description=\"\">\n", prj_get_name());

	/* TODO: select the first executable project */
	io_print("  <StartMode startupentry=\"\" single=\"True\">\n");

	/* Write out the startup entries */
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);
		io_print("    <Execute entry=\"%s\" type=\"None\" />\n", prj_get_pkgname());
	}

	io_print("  </StartMode>\n");
	io_print("  <Entries>\n");

	/* Write out the project entries */
	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		prj_select_package(i);
		path = prj_get_pkgfilename("prjx");
		io_print("    <Entry filename=\"");
		if (path[0] != '.')
			io_print("./");
		io_print("%s\" />\n", path);
	}

	io_print("  </Entries>\n");
	io_print("  <Configurations active=\"Debug\">\n");

	/* Write out the entries for each build configuration */
	for (i = 0; i < prj_get_numconfigs(); ++i)
	{
		prj_select_config(i);
		io_print("    <Configuration name=\"%s\">\n", prj_get_cfgname());

		/* List all packages under this configuration */
		prj_select_config(0);
		for(j = 0; j < prj_get_numpackages(); j++)
		{
			prj_select_package(j);
			io_print("      <Entry name=\"%s\" configurationname=\"%s\" build=\"False\" />\n", prj_get_pkgname(), prj_get_cfgname());
		}

		io_print("    </Configuration>\n");
	}

	/* Finish */
	io_print("  </Configurations>\n");
	io_print("</Combine>");
	io_closefile();

	/* MonoDevelop adds another file */
	if (sharpdev_target == MONODEV)
	{
		if (!io_openfile(path_join(prj_get_path(), prj_get_name(), "mdsx")))
			return 0;

		prj_select_config(0);
		io_print("<MonoDevelopSolution fileversion=\"1.0\">\n");
		io_print("  <RelativeOutputPath>%s</RelativeOutputPath>\n", prj_get_bindir());
		io_print("</MonoDevelopSolution>\n");

		io_closefile();
	}

	return 1;
}
