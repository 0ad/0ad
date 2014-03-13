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

#include "precompiled.h"

#include "HeightMipmap.h"

#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/tex/tex.h"
#include "maths/MathUtil.h"
#include "ps/Filesystem.h"

#include <cmath>

CHeightMipmap::CHeightMipmap()
{
}

CHeightMipmap::~CHeightMipmap()
{
	ReleaseData();
}

void CHeightMipmap::ReleaseData()
{
	for (size_t i = 0; i < m_Mipmap.size(); ++i)
	{
		delete[] m_Mipmap[i].m_Heightmap;
		m_Mipmap[i].m_MapSize = 0;
	}
	m_Mipmap.clear();
}

void CHeightMipmap::Update(const u16* ptr)
{
	ENSURE(ptr != 0);

	Update(ptr, 0, 0, m_MapSize, m_MapSize);
}

void CHeightMipmap::Update(const u16* ptr, size_t left, size_t bottom, size_t right, size_t top)
{
	ENSURE(ptr != 0);

	size_t mapSize = m_MapSize;

	for (size_t i = 0; i < m_Mipmap.size(); ++i)
	{
		// update window
		left = clamp<size_t>((size_t)floorf((float)left / mapSize * m_Mipmap[i].m_MapSize), 0, m_Mipmap[i].m_MapSize - 1);
		bottom = clamp<size_t>((size_t)floorf((float)bottom / mapSize * m_Mipmap[i].m_MapSize), 0, m_Mipmap[i].m_MapSize - 1);

		right = clamp<size_t>((size_t)ceilf((float)right / mapSize * m_Mipmap[i].m_MapSize), 0, m_Mipmap[i].m_MapSize);
		top = clamp<size_t>((size_t)ceilf((float)top / mapSize * m_Mipmap[i].m_MapSize), 0, m_Mipmap[i].m_MapSize);

		// TODO: should verify that the bounds calculations are actually correct

		// update mipmap
		BilinearUpdate(m_Mipmap[i], mapSize, ptr, left, bottom, right, top);

		mapSize = m_Mipmap[i].m_MapSize;
		ptr = m_Mipmap[i].m_Heightmap;
	}
}

void CHeightMipmap::Initialize(size_t mapSize, const u16* ptr)
{
	ENSURE(ptr != 0);
	ENSURE(mapSize > 0);

	ReleaseData();

	m_MapSize = mapSize;
	size_t mipmapSize = round_down_to_pow2(mapSize);

	while (mipmapSize > 1)
	{
		m_Mipmap.push_back(SMipmap(mipmapSize, new u16[mipmapSize*mipmapSize]));
		mipmapSize >>= 1;
	};

	Update(ptr);
}

float CHeightMipmap::GetTrilinearGroundLevel(float x, float z, float radius) const
{
	float y;
	if (radius <= 0.0f) // avoid logf of non-positive value
		y = 0.0f;
	else
		y = clamp<float>(logf(radius * m_Mipmap[0].m_MapSize) / logf(2), 0, m_Mipmap.size());

	const size_t iy = (size_t)clamp<ssize_t>((ssize_t)floorf(y), 0, m_Mipmap.size() - 2);

	const float fy = y - iy;

	const float h0 = BilinearFilter(m_Mipmap[iy], x, z);
	const float h1 = BilinearFilter(m_Mipmap[iy + 1], x, z);

	return (1 - fy) * h0 + fy * h1;
}

float CHeightMipmap::BilinearFilter(const SMipmap &mipmap, float x, float z) const
{
	x *= mipmap.m_MapSize;
	z *= mipmap.m_MapSize;

	const size_t xi = (size_t)clamp<ssize_t>((ssize_t)floor(x), 0, mipmap.m_MapSize - 2);
	const size_t zi = (size_t)clamp<ssize_t>((ssize_t)floor(z), 0, mipmap.m_MapSize - 2);

	const float xf = clamp<float>(x-xi, 0.0f, 1.0f);
	const float zf = clamp<float>(z-zi, 0.0f, 1.0f);

	const float h00 = mipmap.m_Heightmap[zi*mipmap.m_MapSize + xi];
	const float h01 = mipmap.m_Heightmap[(zi+1)*mipmap.m_MapSize + xi];
	const float h10 = mipmap.m_Heightmap[zi*mipmap.m_MapSize + (xi+1)];
	const float h11 = mipmap.m_Heightmap[(zi+1)*mipmap.m_MapSize + (xi+1)];

	return
		(1.f - xf) * (1.f - zf) * h00 +
			   xf  * (1.f - zf) * h10 +
		(1.f - xf) *        zf  * h01 +
			   xf  *        zf  * h11;
}

void CHeightMipmap::HalfResizeUpdate(SMipmap &out_mipmap, size_t mapSize, const u16* ptr, size_t left, size_t bottom, size_t right, size_t top)
{
	// specialized, faster version of BilinearUpdate for powers of 2

	ENSURE(out_mipmap.m_MapSize != 0);

	if (out_mipmap.m_MapSize * 2 != mapSize)
		debug_warn(L"wrong size");

	// valid update window
	ENSURE(left < out_mipmap.m_MapSize);
	ENSURE(bottom < out_mipmap.m_MapSize);
	ENSURE(right > left && right <= out_mipmap.m_MapSize);
	ENSURE(top > bottom && top <= out_mipmap.m_MapSize);

	for (size_t dstZ = bottom; dstZ < top; ++dstZ)
	{
		for (size_t dstX = left; dstX < right; ++dstX)
		{
			size_t srcX = dstX << 1;
			size_t srcZ = dstZ << 1;

			u16 h00 = ptr[srcX + 0 + srcZ * mapSize];
			u16 h10 = ptr[srcX + 1 + srcZ * mapSize];
			u16 h01 = ptr[srcX + 0 + (srcZ + 1) * mapSize];
			u16 h11 = ptr[srcX + 1 + (srcZ + 1) * mapSize];

			out_mipmap.m_Heightmap[dstX + dstZ * out_mipmap.m_MapSize] = (h00 + h10 + h01 + h11) / 4;
		}
	}
}

void CHeightMipmap::BilinearUpdate(SMipmap &out_mipmap, size_t mapSize, const u16* ptr, size_t left, size_t bottom, size_t right, size_t top)
{
	ENSURE(out_mipmap.m_MapSize != 0);

	// filter should have full coverage
	ENSURE(out_mipmap.m_MapSize <= mapSize && out_mipmap.m_MapSize * 2 >= mapSize);

	// valid update window
	ENSURE(left < out_mipmap.m_MapSize);
	ENSURE(bottom < out_mipmap.m_MapSize);
	ENSURE(right > left && right <= out_mipmap.m_MapSize);
	ENSURE(top > bottom && top <= out_mipmap.m_MapSize);

	if (out_mipmap.m_MapSize * 2 == mapSize)
	{
		// optimized for powers of 2
		HalfResizeUpdate(out_mipmap, mapSize, ptr, left, bottom, right, top);
	}
	else
	{
		for (size_t dstZ = bottom; dstZ < top; ++dstZ)
		{
			for (size_t dstX = left; dstX < right; ++dstX)
			{
				const float x = ((float)dstX / (float)out_mipmap.m_MapSize) * mapSize;
				const float z = ((float)dstZ / (float)out_mipmap.m_MapSize) * mapSize;

				const size_t srcX = clamp<size_t>((size_t)x, 0, mapSize - 2);
				const size_t srcZ = clamp<size_t>((size_t)z, 0, mapSize - 2);

				const float fx = clamp<float>(x - srcX, 0.0f, 1.0f);
				const float fz = clamp<float>(z - srcZ, 0.0f, 1.0f);

				const float h00 = ptr[srcX + 0 + srcZ * mapSize];
				const float h10 = ptr[srcX + 1 + srcZ * mapSize];
				const float h01 = ptr[srcX + 0 + (srcZ + 1) * mapSize];
				const float h11 = ptr[srcX + 1 + (srcZ + 1) * mapSize];

				out_mipmap.m_Heightmap[dstX + dstZ * out_mipmap.m_MapSize] = (u16)
					((1.f - fx) * (1.f - fz) * h00 +
							fx  * (1.f - fz) * h10 +
					 (1.f - fx) *        fz  * h01 +
							fx  *        fz  * h11);
			}
		}
	}
}

void CHeightMipmap::DumpToDisk(const VfsPath& filename) const
{
	const size_t w = m_MapSize;
	const size_t h = m_MapSize * 2;
	const size_t bpp = 8;
	int flags = TEX_GREY|TEX_TOP_DOWN;

	const size_t img_size = w * h * bpp/8;
	const size_t hdr_size = tex_hdr_size(filename);
	shared_ptr<u8> buf;
	AllocateAligned(buf, hdr_size+img_size, maxSectorSize);
	void* img = buf.get() + hdr_size;
	Tex t;
	WARN_IF_ERR(t.wrap(w, h, bpp, flags, buf, hdr_size));

	memset(img, 0x00, img_size);
	size_t yoff = 0;
	for (size_t i = 0; i < m_Mipmap.size(); ++i)
	{
		size_t size = m_Mipmap[i].m_MapSize;
		u16* heightmap = m_Mipmap[i].m_Heightmap;
		ENSURE(size+yoff <= h);
		for (size_t y = 0; y < size; ++y)
		{
			for (size_t x = 0; x < size; ++x)
			{
				u16 val = heightmap[x + y*size];
				((u8*)img)[x + (y+yoff)*w] = val >> 8;
			}
		}
		yoff += size;
	}

	DynArray da;
	WARN_IF_ERR(t.encode(filename.Extension(), &da));
	g_VFS->CreateFile(filename, DummySharedPtr(da.base), da.pos);
	(void)da_free(&da);
}
