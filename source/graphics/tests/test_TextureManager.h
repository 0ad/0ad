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

#include "lib/self_test.h"

#include "graphics/TextureManager.h"
#include "lib/alignment.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/file/vfs/vfs.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "lib/ogl.h"
#include "ps/XML/Xeromyces.h"

class TestTextureManager : public CxxTest::TestSuite
{
	PIVFS m_VFS;

public:

	void setUp()
	{
		DeleteDirectory(DataDir()/"_testcache"); // clean up in case the last test run failed

		m_VFS = CreateVfs(20*MiB);
		TS_ASSERT_OK(m_VFS->Mount(L"", DataDir()/"mods"/"_test.tex", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(m_VFS->Mount(L"cache/", DataDir()/"_testcache"));

		h_mgr_init();

		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();

		h_mgr_shutdown();

		m_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_load_basic()
	{
		{
			CTextureManager texman(m_VFS, false, true);

			CTexturePtr t1 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo.png"));

			TS_ASSERT(!t1->IsLoaded());
			TS_ASSERT(!t1->TryLoad());
			TS_ASSERT(!t1->IsLoaded());

			TS_ASSERT(texman.MakeProgress());
			for (size_t i = 0; i < 100; ++i)
			{
				if (texman.MakeProgress())
					break;
				SDL_Delay(10);
			}

			TS_ASSERT(t1->IsLoaded());

			// We can't test sizes because we had to disable GL function calls
			// and therefore couldn't load the texture. Maybe we should try loading
			// the texture file directly, to make sure it's actually worked.
//			TS_ASSERT_EQUALS(t1->GetWidth(), (size_t)64);
//			TS_ASSERT_EQUALS(t1->GetHeight(), (size_t)64);

			// CreateTexture should return the same object
			CTexturePtr t2 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo.png"));
			TS_ASSERT(t1 == t2);
		}

		// New texture manager - should use the cached file
		{
			CTextureManager texman(m_VFS, false, true);

			CTexturePtr t1 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo.png"));

			TS_ASSERT(!t1->IsLoaded());
			TS_ASSERT(t1->TryLoad());
			TS_ASSERT(t1->IsLoaded());
		}
	}

	void test_load_formats()
	{
		CTextureManager texman(m_VFS, false, true);
		CTexturePtr t1 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo.tga"));
		CTexturePtr t2 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo-abgr.dds"));
		CTexturePtr t3 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo-dxt1.dds"));
		CTexturePtr t4 = texman.CreateTexture(CTextureProperties(L"art/textures/a/demo-dxt5.dds"));
		t1->TryLoad();
		t2->TryLoad();
		t3->TryLoad();
		t4->TryLoad();

		size_t done = 0;
		for (size_t i = 0; i < 100; ++i)
		{
			if (texman.MakeProgress())
				++done;
			if (done == 8) // 4 loads, 4 conversions
				break;
			SDL_Delay(10);
		}

		TS_ASSERT(t1->IsLoaded());
		TS_ASSERT(t2->IsLoaded());
		TS_ASSERT(t3->IsLoaded());
		TS_ASSERT(t4->IsLoaded());
	}
};
