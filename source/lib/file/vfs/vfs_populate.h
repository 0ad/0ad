/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : vfs_populate.h
 * Project     : 0 A.D.
 * Description : populate VFS directories with files
 * =========================================================================
 */

#ifndef INCLUDED_VFS_POPULATE
#define INCLUDED_VFS_POPULATE

#include "lib/file/common/real_directory.h"

class VfsDirectory;

/**
 * attach a real directory to a VFS directory.
 *
 * when the VFS directory is accessed, it will first be populated from
 * the real directory. (this delays the impact of mounting a large
 * directory, distributing the cost from startup to the first accesses
 * to each subdirectory.)
 *
 * note: the most recently attached real directory will be used when
 * creating files in the VFS directory.
 **/
extern LibError vfs_Attach(VfsDirectory* directory, const PRealDirectory& realDirectory);

/**
 * populate the directory from the attached real directory.
 *
 * adds each real file and subdirectory entry to the VFS directory.
 * the full contents of any archives in the real directory are also added.
 *
 * has no effect if no directory has been attached since the last populate.
 **/
extern LibError vfs_Populate(VfsDirectory* directory);

#endif	// #ifndef INCLUDED_VFS_POPULATE
