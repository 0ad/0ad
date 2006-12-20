#include "lib/self_test.h"

#include "lib/res/file/vfs.h"
#include "lib/res/file/vfs_optimizer.h"
#include "lib/res/file/path.h"
#include "lib/res/file/trace.h"
#include "lib/res/h_mgr.h"

#include "graphics/MeshManager.h"
#include "graphics/ModelDef.h"

#define MOD_PATH "mods/_test.mesh"
#define CACHE_PATH "_testcache"

const char* srcDAE = "tests/collada/sphere.dae";
const char* srcPMD = "tests/collada/sphere.pmd";
const char* testDAE = "art/meshes/skeletal/test.dae";
const char* testPMD = "art/meshes/skeletal/test.pmd";
const char* testBase = "art/meshes/skeletal/test";

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

		TS_ASSERT_OK(dir_create(MOD_PATH, S_IRWXU|S_IRWXG|S_IRWXO));
		TS_ASSERT_OK(dir_create(CACHE_PATH, S_IRWXU|S_IRWXG|S_IRWXO));

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
		h_mgr_shutdown();
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

		vfs_store(dst, buf, read, FILE_NO_AIO);

		file_buf_free(buf);
		file_close(&f);
	}

	void buildArchive()
	{
		// Create a junk trace file first, because vfs_opt_auto_build requires one
		std::string trace = "000.000000: L \"-\" 0 0000\n";
		vfs_store("trace.txt", trace.c_str(), trace.size(), FILE_NO_AIO);

		// then make the archive
		TS_ASSERT_OK(vfs_opt_rebuild_main_archive(MOD_PATH"/trace.txt", MOD_PATH"/test%02d.zip"));
	}

	CMeshManager* meshManager;

public:

	void setUp()
	{
		initVfs();
		meshManager = new CMeshManager();
	}

	void tearDown()
	{
		delete meshManager;
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
		TS_ASSERT_EQUALS(modeldef1.get(), modeldef2.get());
	}

	void test_load_dae()
	{
		copyFile(srcDAE, testDAE);

		CModelDefPtr modeldef = meshManager->GetMesh(testDAE);
		TS_ASSERT(modeldef);
		if (modeldef) TS_ASSERT_STR_EQUALS(modeldef->GetName(), testBase);
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
