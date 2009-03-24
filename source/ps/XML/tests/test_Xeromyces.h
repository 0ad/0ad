#include "lib/self_test.h"

#include "ps/XML/Xeromyces.h"
#include "lib/file/vfs/vfs.h"
#include "lib/file/path.h"

class TestXeromyces : public CxxTest::TestSuite 
{
public:
	void test_paths()
	{
		TS_ASSERT_OK(path_SetRoot(0, "../data"));
		PIVFS vfs = CreateVfs(20*MiB);

		TS_ASSERT_OK(vfs->Mount("", "mods/_test.xero"));

		VfsPath xmbPath;

		CXeromyces::GetXMBPath(vfs, "test1.xml", "test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath.string(), "cache/mods/_test.xero/xmb/test1.xmb");

		CXeromyces::GetXMBPath(vfs, "a/b/test1.xml", "a/b/test1.xmb", xmbPath);
		TS_ASSERT_STR_EQUALS(xmbPath.string(), "cache/mods/_test.xero/xmb/a/b/test1.xmb");

		path_ResetRootDir();
	}

	// TODO: Should test the reading/parsing/writing code,
	// and parse error handling
};
