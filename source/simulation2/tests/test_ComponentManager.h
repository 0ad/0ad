/* Copyright (C) 2011 Wildfire Games.
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

#include "simulation2/system/ComponentManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/serialization/ISerializer.h"
#include "simulation2/components/ICmpTest.h"
#include "simulation2/components/ICmpTemplateManager.h"

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

#define TS_ASSERT_STREAM(stream, len, buffer) \
	TS_ASSERT_EQUALS(stream.str().length(), (size_t)len); \
	TS_ASSERT_SAME_DATA(stream.str().data(), buffer, len)


#define TS_ASSERT_THROWS_PSERROR(e, t, s) \
	TS_ASSERT_THROWS_EQUALS(e, const t& ex, std::string(ex.what()), s)

class TestComponentManager : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"_test.sim", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir()/"_testcache"));
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_Load()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
	}

	void test_LookupCID()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		TS_ASSERT_EQUALS(man.LookupCID("Test1A"), (int)CID_Test1A);
		TS_ASSERT_EQUALS(man.LookupCID("Test1B"), (int)CID_Test1B);
	}

	void test_AllocateNewEntity()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);

		TS_ASSERT_EQUALS(man.AllocateNewEntity(), (u32)2);
		TS_ASSERT_EQUALS(man.AllocateNewEntity(), (u32)3);
		TS_ASSERT_EQUALS(man.AllocateNewEntity(), (u32)4);
		TS_ASSERT_EQUALS(man.AllocateNewEntity(100), (u32)100);
		TS_ASSERT_EQUALS(man.AllocateNewEntity(), (u32)101);
		// TODO:
		// TS_ASSERT_EQUALS(man.AllocateNewEntity(3), (u32)102);

		TS_ASSERT_EQUALS(man.AllocateNewLocalEntity(), (u32)FIRST_LOCAL_ENTITY);
		TS_ASSERT_EQUALS(man.AllocateNewLocalEntity(), (u32)FIRST_LOCAL_ENTITY+1);

		man.ResetState();

		TS_ASSERT_EQUALS(man.AllocateNewEntity(), (u32)2);
		TS_ASSERT_EQUALS(man.AllocateNewEntity(3), (u32)3);
		TS_ASSERT_EQUALS(man.AllocateNewLocalEntity(), (u32)FIRST_LOCAL_ENTITY);
	}

	void test_AddComponent_errors()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		CEntityHandle hnd1 = man.AllocateEntityHandle(1);

		CParamNode noParam;
		TS_ASSERT(man.AddComponent(hnd1, CID_Test1A, noParam));

		{
			TestLogger log;
			TS_ASSERT(! man.AddComponent(hnd1, 12345, noParam));
			TS_ASSERT_WSTR_CONTAINS(log.GetOutput(), L"ERROR: Invalid component id 12345");
		}

		{
			TestLogger log;
			TS_ASSERT(! man.AddComponent(hnd1, CID_Test1B, noParam));
			TS_ASSERT_WSTR_CONTAINS(log.GetOutput(), L"ERROR: Multiple components for interface ");
		}
	}

	void test_QueryInterface()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		man.AddComponent(hnd1, CID_Test1A, noParam);
		TS_ASSERT(man.QueryInterface(ent1, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent1, IID_Test2) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);

		man.AddComponent(hnd2, CID_Test1B, noParam);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);
		man.AddComponent(hnd2, CID_Test2A, noParam);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) != NULL);
	}

	void test_SendMessage()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3, ent4 = 4;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CEntityHandle hnd4 = man.AllocateEntityHandle(ent4);
		CParamNode noParam;

		man.AddComponent(hnd1, CID_Test1A, noParam);
		man.AddComponent(hnd2, CID_Test1B, noParam);
		man.AddComponent(hnd3, CID_Test2A, noParam);
		man.AddComponent(hnd4, CID_Test1A, noParam);
		man.AddComponent(hnd4, CID_Test2A, noParam);

		CMessageTurnStart msg1;
		CMessageUpdate msg2(fixed::FromInt(100));
		CMessageInterpolate msg3(0, 0, 0);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21000);

		// Test_1A subscribed locally to msg1, nothing subscribed globally
		man.PostMessage(ent1, msg1);
		man.PostMessage(ent1, msg2);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21000);

		man.BroadcastMessage(msg1);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11002);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21050);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21050);

		// Test_1B, Test_2A subscribed locally to msg2, nothing subscribed globally
		man.BroadcastMessage(msg2);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11002);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12010);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21150);

		// Test_1A subscribed locally to msg3, Test_1B subscribed globally
		man.BroadcastMessage(msg3);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11004); // local
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12030); // global
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11003); // local
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21150);

		man.PostMessage(ent1, msg3);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11006); // local
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12050); // global
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11003); // local - skipped
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21150);
	}

	void test_ParamNode()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(hnd1, CID_Test1A, noParam);
		man.AddComponent(hnd2, CID_Test1A, testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
	}

	void test_script_basic()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test.js"));

		TS_ASSERT_EQUALS(man.LookupCID("TestScript1A"), (int)CID__LastNative);
		TS_ASSERT_EQUALS(man.LookupCID("TestScript1B"), (int)CID__LastNative+1);

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CParamNode noParam;

		man.AddComponent(hnd1, CID_Test1A, noParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd3, man.LookupCID("TestScript1B"), noParam);
		man.AddComponent(hnd3, man.LookupCID("TestScript2A"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 101000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 102000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 201000);

		CMessageTurnStart msg1;
		CMessageUpdate msg2(fixed::FromInt(25));

		man.BroadcastMessage(msg1);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 101001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 102001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 201000);

		man.BroadcastMessage(msg2);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 101001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 102001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 201025);
	}

	void test_script_helper_basic()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-helper.js"));
		TS_ASSERT(man.LoadScript(L"simulation/helpers/test-helper.js"));

		entity_id_t ent1 = 1;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1_Helper"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 3);
	}

	void test_script_global_helper()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-global-helper.js"));

		entity_id_t ent1 = 1;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1_GlobalHelper"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 2);
	}

	void test_script_interface()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/interfaces/test-interface.js"));
		TS_ASSERT(man.LoadScript(L"simulation/components/test-interface.js"));

		entity_id_t ent1 = 1;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1_Interface"), noParam);
		man.AddComponent(hnd1, man.LookupCID("TestScript2_Interface"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 1000 + IID__LastNative);
	}

	void test_script_errors()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		ScriptTestSetup(man.m_ScriptInterface);
		man.LoadComponentTypes();

		{
			TestLogger log;
			TS_ASSERT(man.LoadScript(L"simulation/components/error.js"));
			// In SpiderMonkey 1.6, JS_ReportError calls the error reporter even if it's inside
			// a try{} in the script; in recent versions (not sure when it changed) it doesn't
			// so the error here won't get reported.
			TS_ASSERT_WSTR_NOT_CONTAINS(log.GetOutput(), L"ERROR: JavaScript error: simulation/components/error.js line 4\nInvalid interface id");
		}
	}

	void test_script_entityID()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		ScriptTestSetup(man.m_ScriptInterface);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-entityid.js"));

		entity_id_t ent1 = 1, ent2 = 234;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1A"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), (int)ent1);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
	}

	void test_script_QueryInterface()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-query.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd1, man.LookupCID("TestScript2A"), noParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd2, CID_Test2A, noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 400);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 21000);
	}

	void test_script_AddEntity()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-addentity.js"));
		TS_ASSERT(man.LoadScript(L"simulation/components/addentity/test-addentity.js"));
		man.InitSystemEntity();

		entity_id_t ent1 = man.AllocateNewEntity();
		entity_id_t ent2 = ent1 + 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(man.GetSystemEntity(), CID_TemplateManager, noParam));

		TS_ASSERT(man.AddComponent(hnd1, man.LookupCID("TestScript1_AddEntity"), noParam));

		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);

		{
			TestLogger logger; // ignore bogus-template warnings
			TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), (int)ent2);
		}

		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) != NULL);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 999);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 12345);
	}

	void test_script_AddLocalEntity()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-addentity.js"));
		TS_ASSERT(man.LoadScript(L"simulation/components/addentity/test-addentity.js"));
		man.InitSystemEntity();

		entity_id_t ent1 = man.AllocateNewEntity();
		entity_id_t ent2 = man.AllocateNewLocalEntity() + 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(man.GetSystemEntity(), CID_TemplateManager, noParam));

		TS_ASSERT(man.AddComponent(hnd1, man.LookupCID("TestScript1_AddLocalEntity"), noParam));

		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);

		{
			TestLogger logger; // ignore bogus-template warnings
			TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), (int)ent2);
		}

		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) != NULL);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 999);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 12345);
	}

	void test_script_DestroyEntity()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-destroyentity.js"));

		entity_id_t ent1 = 10;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(hnd1, man.LookupCID("TestScript1_DestroyEntity"), noParam));

		TS_ASSERT(man.QueryInterface(ent1, IID_Test1) != NULL);
		static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX();
		TS_ASSERT(man.QueryInterface(ent1, IID_Test1) != NULL);
		man.FlushDestroyedComponents();
		TS_ASSERT(man.QueryInterface(ent1, IID_Test1) == NULL);
	}

	void test_script_messages()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-msg.js"));

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd1, man.LookupCID("TestScript2A"), noParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(hnd2, CID_Test2A, noParam);
		man.AddComponent(hnd3, man.LookupCID("TestScript1B"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 100);

		// This GetX broadcasts messages
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent1, IID_Test2))->GetX(), 200);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 650);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 5150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 26050);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 5650);
	}

	void test_script_template()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-param.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<node><x>1</x><y>1<z w='100'><a>1000</a></z>0</y></node>"), PSRETURN_OK);

		man.AddComponent(hnd1, man.LookupCID("TestScript1_Init"), noParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1_Init"), testParam.GetChild("node"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1+10+100+1000);
	}

	void test_script_template_readonly()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-param.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>100</x>"), PSRETURN_OK);

		man.AddComponent(hnd1, man.LookupCID("TestScript1_readonly"), testParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1_readonly"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 102);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 102);
	}

	void test_script_hotload()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		TS_ASSERT(man.LoadScript(L"simulation/components/test-hotload1.js"));

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3, ent4 = 4;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CEntityHandle hnd4 = man.AllocateEntityHandle(ent4);

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>100</x>"), PSRETURN_OK);

		man.AddComponent(hnd1, man.LookupCID("HotloadA"), testParam);
		man.AddComponent(hnd2, man.LookupCID("HotloadB"), testParam);
		man.AddComponent(hnd2, man.LookupCID("HotloadC"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 200);

		TS_ASSERT(man.LoadScript(L"simulation/components/test-hotload2.js", true));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 1000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 200);

		man.AddComponent(hnd3, man.LookupCID("HotloadA"), testParam);
		man.AddComponent(hnd4, man.LookupCID("HotloadB"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 1000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 200);
	}

	void test_serialization()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 10, ent2 = 20, ent3 = FIRST_LOCAL_ENTITY;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(hnd1, CID_Test1A, noParam);
		man.AddComponent(hnd1, CID_Test2A, noParam);
		man.AddComponent(hnd2, CID_Test1A, testParam);
		man.AddComponent(hnd3, CID_Test2A, noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent1, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21000);

		std::stringstream debugStream;
		TS_ASSERT(man.DumpDebugState(debugStream, true));
		TS_ASSERT_STR_EQUALS(debugStream.str(),
				"rng: \"78606\"\n"
				"entities:\n"
				"- id: 10\n"
				"  Test1A:\n"
				"    x: 11000\n"
				"  Test2A:\n"
				"    x: 21000\n"
				"\n"
				"- id: 20\n"
				"  Test1A:\n"
				"    x: 1234\n"
				"\n"
				"- id: 536870912\n"
				"  type: local\n"
				"  Test2A:\n"
				"    x: 21000\n"
				"\n"
		);

		std::string hash;
		TS_ASSERT(man.ComputeStateHash(hash, false));
		TS_ASSERT_EQUALS(hash.length(), (size_t)16);
		TS_ASSERT_SAME_DATA(hash.data(), "\x3c\x25\x6e\x22\x58\x23\x09\x58\x38\xca\xb2\x1e\x0b\x8c\xac\xcf", 16);
		// echo -en "\x05\x00\x00\x0078606\x02\0\0\0\x01\0\0\0\x0a\0\0\0\xf8\x2a\0\0\x14\0\0\0\xd2\x04\0\0\x04\0\0\0\x0a\0\0\0\x08\x52\0\0" | md5sum | perl -pe 's/([0-9a-f]{2})/\\x$1/g'
		//           ^^^^^^^^ rng ^^^^^^^^ ^^next^^ ^^Test1A^^ ^^^ent1^^ ^^^11000^^^ ^^^ent2^^ ^^^1234^^^ ^^Test2A^^ ^^ent1^^ ^^^21000^^^

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));
		TS_ASSERT_STREAM(stateStream, 73,
				"\x05\x00\x00\x00\x37\x38\x36\x30\x36" // RNG
				"\x02\x00\x00\x00" // next entity ID
				"\x00\x00\x00\x00" // num system component types
				"\x02\x00\x00\x00" // num component types
				"\x06\x00\x00\x00Test1A"
				"\x02\x00\x00\x00" // num ents
				"\x0a\x00\x00\x00" // ent1
				"\xf8\x2a\x00\x00" // 11000
				"\x14\x00\x00\x00" // ent2
				"\xd2\x04\x00\x00" // 1234
				"\x06\x00\x00\x00Test2A"
				"\x01\x00\x00\x00" // num ents
				"\x0a\x00\x00\x00" // ent1
				"\x08\x52\x00\x00" // 21000
		);

		CSimContext context2;
		CComponentManager man2(context2, g_ScriptRuntime);
		man2.LoadComponentTypes();

		TS_ASSERT(man2.QueryInterface(ent1, IID_Test1) == NULL);
		TS_ASSERT(man2.QueryInterface(ent1, IID_Test2) == NULL);
		TS_ASSERT(man2.QueryInterface(ent2, IID_Test1) == NULL);
		TS_ASSERT(man2.QueryInterface(ent3, IID_Test2) == NULL);

		TS_ASSERT(man2.DeserializeState(stateStream));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man2.QueryInterface(ent1, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
		TS_ASSERT(man2.QueryInterface(ent3, IID_Test2) == NULL);
	}

	void test_script_serialization()
	{
		CSimContext context;

		CComponentManager man(context, g_ScriptRuntime);
		ScriptTestSetup(man.m_ScriptInterface);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3, ent4 = 4;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CEntityHandle hnd3 = man.AllocateEntityHandle(ent3);
		CEntityHandle hnd4 = man.AllocateEntityHandle(ent4);
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(hnd1, man.LookupCID("TestScript1_values"), testParam);
		man.AddComponent(hnd2, man.LookupCID("TestScript1_entity"), testParam);

		// TODO: Since the upgrade to SpiderMonkey v24 this test won't be able to correctly represent
		// non-tree structures because sharp variables were removed (bug 566700).
		// This also affects the debug serializer and it could make sense to implement correct serialization again.
		man.AddComponent(hnd3, man.LookupCID("TestScript1_nontree"), testParam);

		man.AddComponent(hnd4, man.LookupCID("TestScript1_custom"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 1234);
		{
			TestLogger log; // swallow warnings about this.entity being read-only
			TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
		}
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 8);

		std::stringstream debugStream;
		TS_ASSERT(man.DumpDebugState(debugStream, true));
		TS_ASSERT_STR_EQUALS(debugStream.str(),
				"rng: \"78606\"\n\
entities:\n\
- id: 1\n\
  TestScript1_values:\n\
    object: {\n\
  \"x\": 1234,\n\
  \"str\": \"this is a string\",\n\
  \"things\": {\n\
    \"a\": 1,\n\
    \"b\": \"2\",\n\
    \"c\": [\n\
      3,\n\
      \"4\",\n\
      [\n\
        5,\n\
        []\n\
      ]\n\
    ]\n\
  }\n\
}\n\
\n\
- id: 2\n\
  TestScript1_entity:\n\
    object: {}\n\
\n\
- id: 3\n\
  TestScript1_nontree:\n\
    object: ({x:[[2], [2], [], {y:[2]}]})\n\
\n\
- id: 4\n\
  TestScript1_custom:\n\
    object: {\n\
  \"c\": 1\n\
}\n\
\n"
		);

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));

		CSimContext context2;
		CComponentManager man2(context2, g_ScriptRuntime);
		man2.LoadComponentTypes();
		TS_ASSERT(man2.LoadScript(L"simulation/components/test-serialize.js"));

		TS_ASSERT(man2.QueryInterface(ent1, IID_Test1) == NULL);
		TS_ASSERT(man2.DeserializeState(stateStream));
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent1, IID_Test1))->GetX(), 1234);
		{
			TestLogger log; // swallow warnings about this.entity being read-only
			TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
		}
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent3, IID_Test1))->GetX(), 12);
	}

	void test_script_serialization_errors()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));

		entity_id_t ent1 = 1;
		CEntityHandle hnd1 = man.AllocateEntityHandle(ent1);
		CParamNode noParam;

		man.AddComponent(hnd1, man.LookupCID("TestScript1_getter"), noParam);

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));
		// (The script will die if the getter is executed)
	}

	void test_script_serialization_template()
	{
		CSimContext context;

		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));
		man.InitSystemEntity();

		entity_id_t ent2 = 2;
		CEntityHandle hnd2 = man.AllocateEntityHandle(ent2);
		CParamNode noParam;

		// The template manager takes care of reloading templates on deserialization,
		// so we need to use it here
		TS_ASSERT(man.AddComponent(man.GetSystemEntity(), CID_TemplateManager, noParam));
		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager));

		const CParamNode* testParam = tempMan->LoadTemplate(ent2, "template-serialize", -1);

		man.AddComponent(hnd2, man.LookupCID("TestScript1_consts"), testParam->GetChild("TestScript1_consts"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12347);

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));

		CSimContext context2;
		CComponentManager man2(context2, g_ScriptRuntime);
		man2.LoadComponentTypes();
		TS_ASSERT(man2.LoadScript(L"simulation/components/test-serialize.js"));

		TS_ASSERT(man2.DeserializeState(stateStream));
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), 12347);
	}
};
