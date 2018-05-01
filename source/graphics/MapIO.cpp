/* Copyright (C) 2018 Wildfire Games.
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

#include "precompiled.h"

#include "MapIO.h"

#include "graphics/Patch.h"
#include "lib/file/file.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/os_path.h"
#include "lib/status.h"
#include "lib/tex/tex.h"
#include "maths/MathUtil.h"
#include "ps/Filesystem.h"

#include <algorithm>
#include <vector>

Status ParseHeightmapImage(const shared_ptr<u8>& fileData, size_t fileSize, std::vector<u16>& heightmap);

Status LoadHeightmapImageVfs(const VfsPath& filepath, std::vector<u16>& heightmap)
{
	shared_ptr<u8> fileData;
	size_t fileSize;

	RETURN_STATUS_IF_ERR(g_VFS->LoadFile(filepath, fileData, fileSize));

	return ParseHeightmapImage(fileData, fileSize, heightmap);
}

Status LoadHeightmapImageOs(const OsPath& filepath, std::vector<u16>& heightmap)
{
	File file;
	RETURN_STATUS_IF_ERR(file.Open(OsString(filepath), O_RDONLY));

	size_t fileSize = lseek(file.Descriptor(), 0, SEEK_END);
	lseek(file.Descriptor(), 0, SEEK_SET);

	shared_ptr<u8> fileData = shared_ptr<u8>(new u8[fileSize]);
	Status readvalue = read(file.Descriptor(), fileData.get(), fileSize);
	file.Close();

	RETURN_STATUS_IF_ERR(readvalue);

	return ParseHeightmapImage(fileData, fileSize, heightmap);
}

Status ParseHeightmapImage(const shared_ptr<u8>& fileData, size_t fileSize, std::vector<u16>& heightmap)
{
	// Decode to a raw pixel format
	Tex tex;
	RETURN_STATUS_IF_ERR(tex.decode(fileData, fileSize));

	// Convert to uncompressed BGRA with no mipmaps
	RETURN_STATUS_IF_ERR(tex.transform_to((tex.m_Flags | TEX_BGR | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS)));

	// Pick smallest side of texture; truncate if not divisible by PATCH_SIZE
	ssize_t tileSize = std::min(tex.m_Width, tex.m_Height);
	tileSize -= tileSize % PATCH_SIZE;

	u8* mapdata = tex.get_data();
	ssize_t bytesPP = tex.m_Bpp / 8;
	ssize_t mapLineSkip = tex.m_Width * bytesPP;

	// Copy image data into the heightmap
	heightmap.resize(SQR(tileSize + 1));
	for (ssize_t y = 0; y < tileSize + 1; ++y)
		for (ssize_t x = 0; x < tileSize + 1; ++x)
		{
			// Repeat the last pixel of the image for the last vertex of the heightmap
			int offset = std::min(y, tileSize - 1) * mapLineSkip + std::min(x, tileSize - 1) * bytesPP;

			// Pick color channel with highest value
			u16 value = std::max({mapdata[offset], mapdata[offset + bytesPP], mapdata[offset + bytesPP * 2]});
			value = mapdata[offset];

			heightmap[(tileSize - y) * (tileSize + 1) + x] = clamp(value * 256, 0, 65535);
		}

	return INFO::OK;
}
