#include "lib/self_test.h"

#include "lib/file/vfs/vfs.h"
#include "lib/file/io/io.h"
#include "lib/file/path.h"
#include "lib/file/file_system_posix.h"

#include "graphics/ColladaManager.h"
#include "graphics/MeshManager.h"
#include "graphics/ModelDef.h"

#include "ps/CLogger.h"

static Path MOD_PATH("mods/_test.mesh");
static Path CACHE_PATH("_testcache");

const char* srcDAE = "collada/sphere.dae";
const char* srcPMD = "collada/sphere.pmd";
const char* testDAE = "art/skeletons/test.dae";
const char* testPMD = "art/skeletons/test.pmd";
const char* testBase = "art/skeletons/test";

const char* srcSkeletonDefs = "collada/skeletons.xml";
const char* testSkeletonDefs = "art/skeletons/skeletons.xml";

extern PIVFS g_VFS;

class TestMeshManager : public CxxTest::TestSuite 
{
	void initVfs()
	{
		// Initialise VFS:

		TS_ASSERT_OK(path_SetRoot(0, "../data"));

		// Set up a mod directory to work in:

		// Make sure the required directories doesn't exist when we start,
		// in case the previous test aborted and left them full of junk
		if(exists(MOD_PATH))
			fsPosix.DeleteDirectory(MOD_PATH);
		if(exists(CACHE_PATH))
			fsPosix.DeleteDirectory(CACHE_PATH);

		TS_ASSERT(fs::create_directory(MOD_PATH.external_directory_string()));
		TS_ASSERT(fs::create_directory(CACHE_PATH.external_directory_string()));

		g_VFS = CreateVfs(20*MiB);

		TS_ASSERT_OK(g_VFS->Mount("", MOD_PATH));
		TS_ASSERT_OK(g_VFS->Mount("collada/", "tests/collada"));

		// Mount _testcache onto virtual /cache - don't use the normal cache
		// directory because that's full of loads of cached files from the
		// proper game and takes a long time to load.
		TS_ASSERT_OK(g_VFS->Mount("cache/", CACHE_PATH));
	}

	void deinitVfs()
	{
//		fsPosix.DeleteDirectory(MOD_PATH);
//		fsPosix.DeleteDirectory(CACHE_PATH);

		path_ResetRootDir();

		g_VFS.reset();
	}

	void copyFile(const char* src, const char* dst)
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
	FileSystem_Posix fsPosix;

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
		SAFE_STRCPY((char*)buf.get(), "Test");
		g_VFS->CreateFile(testDAE, buf, 4);
	}

	void test_load_pmd_with_extension()
	{
		copyFile(srcPMD, testPMD);

		CModelDefPtr modeldef = meshManager->GetMesh(testPMD);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_STR_EQUALS(modeldef->GetName(), testBase);
	}

	void test_load_pmd_without_extension()
	{
		copyFile(srcPMD, testPMD);

		CModelDefPtr modeldef = meshManager->GetMesh(testBase);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_STR_EQUALS(modeldef->GetName(), testBase);
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
		if (modeldef) TS_ASSERT_STR_EQUALS(modeldef->GetName(), testBase);
	}

	void test_load_dae_caching()
	{
		copyFile(srcDAE, testDAE);
		copyFile(srcSkeletonDefs, testSkeletonDefs);

		VfsPath daeName1 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		VfsPath daeName2 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		TS_ASSERT(!daeName1.empty());
		TS_ASSERT_STR_EQUALS(daeName1.string(), daeName2.string());
		// TODO: it'd be nice to test that it really isn't doing the DAE->PMD
		// conversion a second time, but there doesn't seem to be an easy way
		// to check that
	}

	void test_invalid_skeletons()
	{
		TestLogger logger;

		copyFile(srcDAE, testDAE);
		shared_ptr<u8> buf = io_Allocate(100);
		SAFE_STRCPY((char*)buf.get(), "Not valid XML");
		g_VFS->CreateFile(testSkeletonDefs, buf, 13);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "parser error");
	}

	void test_invalid_dae()
	{
		TestLogger logger;

		copyFile(srcSkeletonDefs, testSkeletonDefs);
		shared_ptr<u8> buf = io_Allocate(100);
		SAFE_STRCPY((char*)buf.get(), "Not valid XML");
		g_VFS->CreateFile(testDAE, buf, 13);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "parser error");
	}

	void test_load_nonexistent_pmd()
	{
		CModelDefPtr modeldef = meshManager->GetMesh(testPMD);
		TS_ASSERT(! modeldef);
	}

	void test_load_nonexistent_dae()
	{
		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
	}
};
