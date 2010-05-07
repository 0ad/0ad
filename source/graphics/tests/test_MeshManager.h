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

#include "lib/self_test.h"

#include "lib/file/vfs/vfs.h"
#include "lib/file/io/io.h"

#include "graphics/ColladaManager.h"
#include "graphics/MeshManager.h"
#include "graphics/ModelDef.h"

#include "ps/CLogger.h"
#include "ps/XML/RelaxNG.h"

static fs::wpath MOD_PATH(DataDir()/L"mods/_test.mesh");
static fs::wpath CACHE_PATH(DataDir()/L"_testcache");

const wchar_t* srcDAE = L"collada/sphere.dae";
const wchar_t* srcPMD = L"collada/sphere.pmd";
const wchar_t* testDAE = L"art/skeletons/test.dae";
const wchar_t* testPMD = L"art/skeletons/test.pmd";
const wchar_t* testBase = L"art/skeletons/test";

const wchar_t* srcSkeletonDefs = L"collada/skeletons.xml";
const wchar_t* testSkeletonDefs = L"art/skeletons/skeletons.xml";

extern PIVFS g_VFS;

class TestMeshManager : public CxxTest::TestSuite 
{
	void initVfs()
	{
		// Initialise VFS:

		// Set up a mod directory to work in:

		// Make sure the required directories doesn't exist when we start,
		// in case the previous test aborted and left them full of junk
		if(exists(MOD_PATH))
			DeleteDirectory(MOD_PATH);
		if(exists(CACHE_PATH))
			DeleteDirectory(CACHE_PATH);

		g_VFS = CreateVfs(20*MiB);

		TS_ASSERT_OK(g_VFS->Mount(L"", MOD_PATH));
		TS_ASSERT_OK(g_VFS->Mount(L"collada/", DataDir()/L"tests/collada", VFS_MOUNT_MUST_EXIST));

		// Mount _testcache onto virtual /cache - don't use the normal cache
		// directory because that's full of loads of cached files from the
		// proper game and takes a long time to load.
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", CACHE_PATH));
	}

	void deinitVfs()
	{
		g_VFS.reset();
		DeleteDirectory(MOD_PATH);
		DeleteDirectory(CACHE_PATH);
	}

	void copyFile(const VfsPath& src, const VfsPath& dst)
	{
		// Copy a file into the mod directory, so we can work on it:
		shared_ptr<u8> data; size_t size = 0;
		TS_ASSERT_OK(g_VFS->LoadFile(src, data, size));
		TS_ASSERT_OK(g_VFS->CreateFile(dst, data, size));
	}

	void buildArchive()
	{
		// Create a junk trace file first, because vfs_opt_auto_build requires one
//		std::string trace = "000.000000: L \"-\" 0 0000\n";
//		vfs_store("trace.txt", (const u8*)trace.c_str(), trace.size(), FILE_NO_AIO);

		// then make the archive
//		TS_ASSERT_OK(vfs_opt_rebuild_main_archive(MOD_PATH"/trace.txt", MOD_PATH"/test%02d.zip"));
	}

	CColladaManager* colladaManager;
	CMeshManager* meshManager;

public:

	void setUp()
	{
		initVfs();
		colladaManager = new CColladaManager();
		meshManager = new CMeshManager(*colladaManager);
	}

	void tearDown()
	{
		delete meshManager;
		delete colladaManager;
		deinitVfs();
	}

	void IRRELEVANT_test_archived()
	{
		copyFile(srcDAE, testDAE);
		//buildArchive();
		shared_ptr<u8> buf = io_Allocate(100);
		strcpy_s((char*)buf.get(), 5, "Test");
		g_VFS->CreateFile(testDAE, buf, 4);
	}

	void test_load_pmd_with_extension()
	{
		copyFile(srcPMD, testPMD);

		CModelDefPtr modeldef = meshManager->GetMesh(testPMD);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_WSTR_EQUALS(modeldef->GetName().string(), testBase);
	}

	void test_load_pmd_without_extension()
	{
		copyFile(srcPMD, testPMD);

		CModelDefPtr modeldef = meshManager->GetMesh(testBase);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_WSTR_EQUALS(modeldef->GetName().string(), testBase);
	}

	void test_caching()
	{
		copyFile(srcPMD, testPMD);

		CModelDefPtr modeldef1 = meshManager->GetMesh(testPMD);
		CModelDefPtr modeldef2 = meshManager->GetMesh(testPMD);
		TS_ASSERT(modeldef1 && modeldef2);
		if (modeldef1 && modeldef2) TS_ASSERT_EQUALS(modeldef1.get(), modeldef2.get());
	}

	void test_load_dae()
	{
		copyFile(srcDAE, testDAE);
		copyFile(srcSkeletonDefs, testSkeletonDefs);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_WSTR_EQUALS(modeldef->GetName().string(), testBase);
	}

	void test_load_dae_caching()
	{
		copyFile(srcDAE, testDAE);
		copyFile(srcSkeletonDefs, testSkeletonDefs);

		VfsPath daeName1 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		VfsPath daeName2 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		TS_ASSERT(!daeName1.empty());
		TS_ASSERT_WSTR_EQUALS(daeName1.string(), daeName2.string());
		// TODO: it'd be nice to test that it really isn't doing the DAE->PMD
		// conversion a second time, but there doesn't seem to be an easy way
		// to check that
	}

	void test_invalid_skeletons()
	{
		TestLogger logger;

		copyFile(srcDAE, testDAE);
		shared_ptr<u8> buf = io_Allocate(100);
		strcpy_s((char*)buf.get(), 100, "Not valid XML");
		g_VFS->CreateFile(testSkeletonDefs, buf, 13);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"parser error");
	}

	void test_invalid_dae()
	{
		TestLogger logger;

		copyFile(srcSkeletonDefs, testSkeletonDefs);
		shared_ptr<u8> buf = io_Allocate(100);
		strcpy_s((char*)buf.get(), 100, "Not valid XML");
		g_VFS->CreateFile(testDAE, buf, 13);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"parser error");
	}

	void test_load_nonexistent_pmd()
	{
		TestLogger logger;

		CModelDefPtr modeldef = meshManager->GetMesh(testPMD);
		TS_ASSERT(! modeldef);
	}

	void test_load_nonexistent_dae()
	{
		TestLogger logger;

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
	}

	void test_load_across_relaxng()
	{
		// Verify that loading meshes doesn't invalidate other users of libxml2 by calling xmlCleanupParser
		// (Run this in Valgrind and check for use-of-freed-memory errors)

		RelaxNGValidator v;
		TS_ASSERT(v.LoadGrammar("<element xmlns='http://relaxng.org/ns/structure/1.0' datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes' name='test'><data type='decimal'/></element>"));
		TS_ASSERT(v.Validate(L"doc", L"<test>2.0</test>"));

		copyFile(srcDAE, testDAE);
		copyFile(srcSkeletonDefs, testSkeletonDefs);
		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_WSTR_EQUALS(modeldef->GetName().string(), testBase);

		TS_ASSERT(v.Validate(L"doc", L"<test>2.0</test>"));
	}


	//////////////////////////////////////////////////////////////////////////

	// Tests based on real DAE files:

	void test_load_dae_bogus_material_target()
	{
		copyFile(L"collada/bogus_material_target.dae", testDAE);
		copyFile(srcSkeletonDefs, testSkeletonDefs);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(modeldef);
	}

};
