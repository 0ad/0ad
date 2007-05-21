#include "lib/self_test.h"

#include "lib/res/file/vfs.h"
#include "lib/res/file/vfs_optimizer.h"
#include "lib/res/file/path.h"
#include "lib/res/file/trace.h"
#include "lib/res/h_mgr.h"

#include "graphics/ColladaManager.h"
#include "graphics/MeshManager.h"
#include "graphics/ModelDef.h"

#include "ps/CLogger.h"

#define MOD_PATH "mods/_test.mesh"
#define CACHE_PATH "_testcache"

const char* srcDAE = "tests/collada/sphere.dae";
const char* srcPMD = "tests/collada/sphere.pmd";
const char* testDAE = "art/meshes/skeletal/test.dae";
const char* testPMD = "art/meshes/skeletal/test.pmd";
const char* testBase = "art/meshes/skeletal/test";

const char* srcSkeletonDefs = "tests/collada/skeletons.xml";
const char* testSkeletonDefs = "art/skeletons/skeletons.xml";

class TestMeshManager : public CxxTest::TestSuite 
{
	void initVfs()
	{
		// Initialise VFS:

		TS_ASSERT_OK(file_init());
		TS_ASSERT_OK(file_set_root_dir(0, "../data"));

		// Set up a mod directory to work in:

		// Make sure the required directories doesn't exist when we start,
		// in case the previous test aborted and left them full of junk
		if (file_exists(MOD_PATH))
			TS_ASSERT_OK(dir_delete(MOD_PATH));

		if (file_exists(CACHE_PATH))
			TS_ASSERT_OK(dir_delete(CACHE_PATH));

		TS_ASSERT_OK(dir_create(MOD_PATH));
		TS_ASSERT_OK(dir_create(CACHE_PATH));

		vfs_init();

		// Mount the mod on /
		TS_ASSERT_OK(vfs_mount("", MOD_PATH, VFS_MOUNT_RECURSIVE|VFS_MOUNT_ARCHIVES|VFS_MOUNT_ARCHIVABLE));

		// Mount _testcache onto virtual /cache - don't use the normal cache
		// directory because that's full of loads of cached files from the
		// proper game and takes a long time to load.
		TS_ASSERT_OK(vfs_mount("cache/", CACHE_PATH, VFS_MOUNT_RECURSIVE|VFS_MOUNT_ARCHIVES|VFS_MOUNT_ARCHIVABLE));

		TS_ASSERT_OK(vfs_set_write_target(MOD_PATH));
	}

	void deinitVfs()
	{
		// (TODO: It'd be nice if this kind of code didn't have to be
		// duplicated in each test suite that's using VFS things)

		vfs_shutdown();
		TS_ASSERT_OK(file_shutdown());

		TS_ASSERT_OK(dir_delete(MOD_PATH));
		if (file_exists(CACHE_PATH))
			TS_ASSERT_OK(dir_delete(CACHE_PATH));

		path_reset_root_dir();
	}

	void copyFile(const char* src, const char* dst)
	{
		// Copy a file into the mod directory, so we can work on it:

		File f;
		TS_ASSERT_OK(file_open(src, 0, &f));
		FileIOBuf buf = FILE_BUF_ALLOC;
		ssize_t read = file_io(&f, 0, f.size, &buf);
		TS_ASSERT_EQUALS(read, f.size);
		file_close(&f);

		vfs_store(dst, buf, read, FILE_NO_AIO);

		file_buf_free(buf);
	}

	void buildArchive()
	{
		// Create a junk trace file first, because vfs_opt_auto_build requires one
		std::string trace = "000.000000: L \"-\" 0 0000\n";
		vfs_store("trace.txt", trace.c_str(), trace.size(), FILE_NO_AIO);

		// then make the archive
		TS_ASSERT_OK(vfs_opt_rebuild_main_archive(MOD_PATH"/trace.txt", MOD_PATH"/test%02d.zip"));
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

		buildArchive();

		// Have to specify FILE_WRITE_TO_TARGET in order to overwrite existent
		// files when they might have been archived
		vfs_store(testDAE, "Test", 4, FILE_NO_AIO | FILE_WRITE_TO_TARGET);

		// We can't overwrite cache files because FILE_WRITE_TO_TARGET won't
		// write into cache/ - it might be nice to fix that. For now we just
		// use unique filenames.
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
		// TODO: I get
		//  Assertion failed: "buf_in_cache == buf"
		//  Location: file_cache.cpp:1094 (file_buf_free)
		// when the order of these is swapped...

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

		CStr daeName1 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		CStr daeName2 = colladaManager->GetLoadableFilename(testBase, CColladaManager::PMD);
		TS_ASSERT(daeName1.length());
		TS_ASSERT_STR_EQUALS(daeName1, daeName2);
		// TODO: it'd be nice to test that it really isn't doing the DAE->PMD
		// conversion a second time, but there doesn't seem to be an easy way
		// to check that
	}

	void test_invalid_skeletons()
	{
		TestLogger logger;

		copyFile(srcDAE, testDAE);
		const char text[] = "Not valid XML";
		vfs_store(testSkeletonDefs, text, strlen(text), FILE_NO_AIO);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(! modeldef);
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "parser error");
	}

	void test_invalid_dae()
	{
		TestLogger logger;

		copyFile(srcSkeletonDefs, testSkeletonDefs);
		const char text[] = "Not valid XML";
		vfs_store(testDAE, text, strlen(text), FILE_NO_AIO);

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
