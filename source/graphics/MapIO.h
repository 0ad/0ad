/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_MAPIO
#define INCLUDED_MAPIO

#include "lib/file/vfs/vfs_path.h"
#include "lib/os_path.h"
#include "lib/status.h"

// Opens the given texture file and stores it in a one-dimensional u16 vector.
Status LoadHeightmapImageVfs(const VfsPath& filepath, std::vector<u16>& heightmap);
Status LoadHeightmapImageOs(const OsPath& filepath, std::vector<u16>& heightmap);

class CMapIO
{
public:
	// Current file version given to saved maps.
	enum { FILE_VERSION = 7 };
	// Supported file read version - file with version less than this will be rejected.
	enum { FILE_READ_VERSION = 7 };

#pragma pack(push, 1)
	// Description of a tile for I/O purposes.
	struct STileDesc
	{
		// Index into the texture array of first texture on tile.
		u16 m_Tex1Index;
		// Index into the texture array of second texture; (0xFFFF) if none.
		u16 m_Tex2Index;
		u32 m_Priority;
	};
#pragma pack(pop)
};

#endif
