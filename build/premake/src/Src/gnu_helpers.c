/**********************************************************************
 * Premake - gnu_helpers.c
 * The GNU makefile target
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

#include "premake.h"
#include <string.h>


int gnu_pkgOwnsPath()
{
	int i;

	if (path_compare(prj_get_pkgpath(), prj_get_path()))
		return 0;

	for (i = 0; i < prj_get_numpackages(); ++i)
	{
		if (prj_get_package() != prj_get_package_for(i))
		{
			if (path_compare(prj_get_pkgpath(), prj_get_pkgpath_for(i)))
				return 0;
		}
	}

	return 1;
}

