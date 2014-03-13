/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/self_test.h"

#include "graphics/TextureConverter.h"

#include "lib/alignment.h"
#include "lib/file/vfs/vfs.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "ps/XML/Xeromyces.h"

class TestTextureConverter : public CxxTest::TestSuite
{
	PIVFS m_VFS;

public:

	void setUp()
	{
		DeleteDirectory(DataDir()/"_testcache"); // clean up in case the last test run failed

		m_VFS = CreateVfs(20*MiB);
		TS_ASSERT_OK(m_VFS->Mount(L"", DataDir()/"mods"/"_test.tex", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(m_VFS->Mount(L"cache/", DataDir()/"_testcache"));
	}

	void tearDown()
	{
		m_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_convert_quality()
	{
		// Test for the bug in http://code.google.com/p/nvidia-texture-tools/issues/detail?id=139

		VfsPath src = L"art/textures/b/test.png";

		CTextureConverter converter(m_VFS, false);
		CTextureConverter::Settings settings = converter.ComputeSettings(L"", std::vector<CTextureConverter::SettingsFile*>());
		TS_ASSERT(converter.ConvertTexture(CTexturePtr(), src, L"cache/test.png", settings));

		VfsPath dest;
		for (size_t i = 0; i < 100; ++i)
		{
			CTexturePtr texture;
			bool ok;
			if (converter.Poll(texture, dest, ok))
			{
				TS_ASSERT(ok);
				break;
			}
			SDL_Delay(10);
		}

		shared_ptr<u8> file;
		size_t fileSize = 0;
		TS_ASSERT_OK(m_VFS->LoadFile(dest, file, fileSize));

		Tex tex;
		TS_ASSERT_OK(tex.decode(file, fileSize));

		TS_ASSERT_OK(tex.transform_to((tex.m_Flags | TEX_BGR | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS)));

		u8* texdata = tex.get_data();

		// The source texture is repeated after 4 pixels, so the compressed texture
		// should be identical after 4 pixels
		TS_ASSERT_EQUALS(texdata[0*4], texdata[4*4]);
		// 1st, 2nd and 4th rows should be unequal
		TS_ASSERT_DIFFERS(texdata[0*4], texdata[8*4]);
		TS_ASSERT_EQUALS(texdata[8*4], texdata[16*4]);
		TS_ASSERT_DIFFERS(texdata[16*4], texdata[24*4]);

//		for (size_t i = 0; i < tex.dataSize; ++i)
//		{
//			if (i % 4 == 0) printf("\n");
//			printf("%02x ", texdata[i]);
//		}
	}
};
