/**********************************************************************
 * Premake - clean.c
 * The cleanup target.
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

static char buffer[8192];


int clean()
{
	int i, j;

	puts("Removing all project and intermediate files...");

	/* VS.NET 200x */
	io_remove(path_join(prj_get_path(), prj_get_name(), "sln"));
	io_remove(path_join(prj_get_path(), prj_get_name(), "suo"));

	/* VS6 */
	io_remove(path_join(prj_get_path(), prj_get_name(), "ncb"));
	io_remove(path_join(prj_get_path(), prj_get_name(), "dsw"));
	io_remove(path_join(prj_get_path(), prj_get_name(), "opt"));

	/* GNU */
	io_remove(path_join(prj_get_path(), "Makefile", ""));

	/* SharpDevelop */
	io_remove(path_join(prj_get_path(), prj_get_name(), "cmbx"));

	/* MonoDevelop */
	io_remove(path_join(prj_get_path(), prj_get_name(), "mdsx"));
	io_remove(path_join(prj_get_path(), "make", "sh"));

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		char cwd[8192];
		MaskHandle mask;

		prj_select_package(i);

		strcpy(cwd, io_getcwd());
		io_chdir(prj_get_pkgpath());

		for (j = 0; j < prj_get_numconfigs(); ++j)
		{
			prj_select_config(j);

			/* POSIX shared lib */
			strcpy(buffer, prj_get_prefix() != NULL ? prj_get_prefix() : "lib");
			strcat(buffer, path_getbasename(prj_get_target()));
			strcat(buffer, ".");
			strcat(buffer, prj_get_extension() != NULL ? prj_get_extension() : "so");
			io_remove(path_join(prj_get_outdir(), buffer, ""));

			/* POSIX executable */
			strcpy(buffer, prj_get_prefix() != NULL ? prj_get_prefix() : "");
			strcat(buffer, path_getbasename(prj_get_target()));
			io_remove(path_join(prj_get_outdir(), buffer, ""));

			/* Windows executable */
			io_remove(path_join(prj_get_outdir(), buffer, "exe"));

			/* .NET assembly manifest */
			io_remove(path_join(prj_get_outdir(), buffer, "exe.manifest"));

			/* DLL or assembly */
			io_remove(path_join(prj_get_outdir(), buffer, "dll"));

			/* Windows static library */
			io_remove(path_join(prj_get_outdir(), buffer, "lib"));

			/* Visual Studio symbol file */
			io_remove(path_join(prj_get_outdir(), buffer, "pdb"));

			/* Visual Studio incremental link file */
			io_remove(path_join(prj_get_outdir(), buffer, "ilk"));

			/* Visual Studio VSHOST */
			io_remove(path_join(prj_get_outdir(), buffer, "vshost.exe"));

			/* Windows DLL exports library */
			io_remove(path_join(prj_get_libdir(), buffer, "lib"));
			io_remove(path_join(prj_get_libdir(), buffer, "exp"));

			/* Mono debugger symbols */
			io_remove(path_join(prj_get_outdir(), buffer, "mdb"));

			/* All */
			io_rmdir(".", prj_get_objdir());
		}

		/* VS.NET 2005 */
		strcpy(g_buffer, path_join(".", prj_get_pkgname(), "vcproj.*.user"));
		mask = io_mask_open(g_buffer);
		while (io_mask_getnext(mask))
			io_remove(io_mask_getname(mask));
		io_mask_close(mask);

		strcpy(g_buffer, path_join(".", prj_get_pkgname(), "csproj.*.user"));
		mask = io_mask_open(g_buffer);
		while (io_mask_getnext(mask))
			io_remove(io_mask_getname(mask));
		io_mask_close(mask);

		/* VS.NET 200x */
		io_remove(path_join(".", prj_get_pkgname(), "csproj"));
		io_remove(path_join(".", prj_get_pkgname(), "csproj.user"));
		io_remove(path_join(".", prj_get_pkgname(), "csproj.webinfo"));
		io_remove(path_join(".", prj_get_pkgname(), "vcproj"));

		/* VS6 */
		io_remove(path_join(".", prj_get_pkgname(), "dsp"));
		io_remove(path_join(".", prj_get_pkgname(), "plg"));

		/* GNU */
		io_remove(path_join(".", "Makefile", ""));
		io_remove(path_join(".", prj_get_pkgname(), "mak"));

		/* SharpDevelop */
		io_remove(path_join(".", prj_get_pkgname(), "prjx"));

		/* MonoDevelop */
		io_remove(path_join(".", prj_get_pkgname(), "cmbx"));
		io_remove(path_join(".", "Makefile", prj_get_pkgname()));
		io_remove(path_join(".", prj_get_pkgname(), "pidb"));

		/* All */
		if (prj_get_pkgobjdir() != NULL)
			io_rmdir(".", prj_get_pkgobjdir());

		io_chdir(cwd);
	}

	return 1;
}
