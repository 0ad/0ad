/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "lib/file/vfs/vfs.h"
#include "ps/ConfigDB.h"

#include <memory>

extern PIVFS g_VFS;

class TestConfigDB : public CxxTest::TestSuite
{
	std::unique_ptr<CConfigDB> configDB;
public:

	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"config", DataDir() / "_testconfig" / ""));

		configDB = std::make_unique<CConfigDB>();
	}

	void tearDown()
	{
		DeleteDirectory(DataDir()/"_testconfig");
		g_VFS.reset();
		configDB.reset();
	}

	void test_setting_int()
	{
		configDB->SetConfigFile(CFG_SYSTEM, "config/file.cfg");
		configDB->WriteFile(CFG_SYSTEM);
		configDB->Reload(CFG_SYSTEM);
		configDB->SetValueString(CFG_SYSTEM, "test_setting", "5");
		configDB->WriteFile(CFG_SYSTEM);
		configDB->Reload(CFG_SYSTEM);
		{
			std::string res;
			configDB->GetValue(CFG_SYSTEM, "test_setting", res);
			TS_ASSERT_EQUALS(res, "5");
		}
		{
			int res;
			configDB->GetValue(CFG_SYSTEM, "test_setting", res);
			TS_ASSERT_EQUALS(res, 5);
		}
	}

	void test_setting_empty()
	{
		configDB->SetConfigFile(CFG_SYSTEM, "config/file.cfg");
		configDB->WriteFile(CFG_SYSTEM);
		configDB->Reload(CFG_SYSTEM);
		configDB->SetValueList(CFG_SYSTEM, "test_setting", {});
		configDB->WriteFile(CFG_SYSTEM);
		configDB->Reload(CFG_SYSTEM);
		{
			std::string res = "toto";
			configDB->GetValue(CFG_SYSTEM, "test_setting", res);
			// Empty config values don't overwrite
			TS_ASSERT_EQUALS(res, "toto");
		}
		{
			int res = 3;
			configDB->GetValue(CFG_SYSTEM, "test_setting", res);
			// Empty config values don't overwrite
			TS_ASSERT_EQUALS(res, 3);
		}
	}
};
