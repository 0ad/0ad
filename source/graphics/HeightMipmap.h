/* Copyright (C) 2012 Wildfire Games.
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


/*
 * Describes ground using heightmap mipmaps
 * Used for camera movement
 */

#ifndef INCLUDED_HEIGHTMIPMAP
#define INCLUDED_HEIGHTMIPMAP

#include "lib/file/vfs/vfs_path.h"

struct SMipmap
{
	SMipmap() : m_MapSize(0), m_Heightmap(0) { }
	SMipmap(size_t MapSize, u16* Heightmap) : m_MapSize(MapSize), m_Heightmap(Heightmap) { }

	size_t m_MapSize;
	u16* m_Heightmap;
};

class CHeightMipmap
{
	NONCOPYABLE(CHeightMipmap);
public:

	CHeightMipmap();
	~CHeightMipmap();

	void Initialize(size_t mapSize, const u16* ptr);
	void ReleaseData();

	// update the heightmap mipmaps
	void Update(const u16* ptr);

	// update a section of the heightmap mipmaps
	// (coordinates are heightmap cells, inclusive of lower bounds,
	// exclusive of upper bounds)
	void Update(const u16* ptr, size_t left, size_t bottom, size_t right, size_t top);

	float GetTrilinearGroundLevel(float x, float z, float radius) const;

	void DumpToDisk(const VfsPath& path) const;

private:

	// get bilinear filtered height from mipmap
	float BilinearFilter(const SMipmap &mipmap, float x, float z) const;

	// update rectangle of the output mipmap by bilinear interpolating an input mipmap of exactly twice its size
	void HalfResizeUpdate(SMipmap &out_mipmap, size_t mapSize, const u16* ptr, size_t left, size_t bottom, size_t right, size_t top);

	// update rectangle of the output mipmap by bilinear interpolating the input mipmap
	void BilinearUpdate(SMipmap &out_mipmap, size_t mapSize, const u16* ptr, size_t left, size_t bottom, size_t right, size_t top);

	// size of this map in each direction
	size_t m_MapSize;

	// mipmap list
	std::vector<SMipmap> m_Mipmap;
};

#endif
