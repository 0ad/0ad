/* Copyright (C) 2016 Wildfire Games.
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

#ifndef PS_UTIL_H
#define PS_UTIL_H

#include "lib/file/vfs/vfs_path.h"

struct Tex;

extern void WriteSystemInfo();

extern const wchar_t* ErrorString(int err);

extern OsPath createDateIndexSubdirectory(const OsPath& parentDir);

extern void WriteScreenshot(const VfsPath& extension);
extern void WriteBigScreenshot(const VfsPath& extension, int tiles);

extern Status tex_write(Tex* t, const VfsPath& filename);

#endif // PS_UTIL_H
