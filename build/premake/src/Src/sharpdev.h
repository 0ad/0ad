/**********************************************************************
 * Premake - sharpdev.h
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

int sharpdev_generate(const char* target);

int sharpdev_cs();

#define SHARPDEV  0
#define MONODEV   1

extern int sharpdev_target;
extern int sharpdev_warncontent;
