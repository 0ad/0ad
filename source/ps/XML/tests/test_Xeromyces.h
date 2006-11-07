#include "lib/self_test.h"

#include "ps/XML/Xeromyces.h"
#include "lib/res/file/vfs.h"
#include "lib/res/file/path.h"
#include "lib/res/file/trace.h"

class TestXeromyces : public CxxTest::TestSuite 
{
public:
	void test_paths()
	{
		file_init();
 		path_init();
		file_set_root_dir(0, "../data");
		vfs_init();

		vfs_mount("", "mods/_tests", VFS_MOUNT_RECURSIVE);
		vfs_set_write_target("mods/_tests");

		char xmbPath[PATH_MAX];

		CXeromyces::GetXMBPath("test1.xml", "test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath, "cache/mods/_tests/xmb/test1.xmb");

		CXeromyces::GetXMBPath("a/b/test1.xml", "a/b/test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath, "cache/mods/_tests/xmb/a/b/test1.xmb");

		vfs_shutdown();
		path_reset_root_dir();
	}
};
