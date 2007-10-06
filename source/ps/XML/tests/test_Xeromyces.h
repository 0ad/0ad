#include "lib/self_test.h"

#include "ps/XML/Xeromyces.h"
#include "lib/res/file/vfs.h"
#include "lib/res/file/path.h"
#include "lib/res/h_mgr.h"

class TestXeromyces : public CxxTest::TestSuite 
{
public:
	void test_paths()
	{
		TS_ASSERT_OK(file_init());
		TS_ASSERT_OK(file_set_root_dir(0, "../data"));
		vfs_init();

		TS_ASSERT_OK(vfs_mount("", "mods/_test.xero", VFS_MOUNT_RECURSIVE));
		TS_ASSERT_OK(vfs_set_write_target("mods/_test.xero"));

		char xmbPath[PATH_MAX];

		CXeromyces::GetXMBPath("test1.xml", "test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath, "cache/mods/_test.xero/xmb/test1.xmb");

		CXeromyces::GetXMBPath("a/b/test1.xml", "a/b/test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath, "cache/mods/_test.xero/xmb/a/b/test1.xmb");

		vfs_shutdown();
		path_reset_root_dir();
		TS_ASSERT_OK(file_shutdown());
	}
};
