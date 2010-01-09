/* Copyright (C) 2010 Wildfire Games.
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
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/L"mods/_test.sim"));
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
	}

	void test_Load()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
	}

	void test_LookupCID()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		TS_ASSERT_EQUALS(man.LookupCID("Test1A"), (int)CID_Test1A);
		TS_ASSERT_EQUALS(man.LookupCID("Test1B"), (int)CID_Test1B);
	}

	void test_AddComponent_errors()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		CParamNode noParam;
		TS_ASSERT(man.AddComponent(1, CID_Test1A, noParam));

		{
			TestLogger log;
			TS_ASSERT(! man.AddComponent(1, 12345, noParam));
			TS_ASSERT_WSTR_CONTAINS(log.GetOutput(), L"ERROR: Invalid component id 12345");
		}

		{
			TestLogger log;
			TS_ASSERT(! man.AddComponent(1, CID_Test1B, noParam));
			TS_ASSERT_WSTR_CONTAINS(log.GetOutput(), L"ERROR: Multiple components for interface ");
		}
	}

	void test_QueryInterface()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		man.AddComponent(ent1, CID_Test1A, noParam);
		TS_ASSERT(man.QueryInterface(ent1, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent1, IID_Test2) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) == NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);

		man.AddComponent(ent2, CID_Test1B, noParam);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) == NULL);
		man.AddComponent(ent2, CID_Test2A, noParam);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test1) != NULL);
		TS_ASSERT(man.QueryInterface(ent2, IID_Test2) != NULL);
	}

	void test_SendMessage()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3, ent4 = 4;
		CParamNode noParam;

		man.AddComponent(ent1, CID_Test1A, noParam);
		man.AddComponent(ent2, CID_Test1B, noParam);
		man.AddComponent(ent3, CID_Test2A, noParam);
		man.AddComponent(ent4, CID_Test1A, noParam);
		man.AddComponent(ent4, CID_Test2A, noParam);

		CMessageTurnStart msg1;
		CMessageUpdate msg2(CFixed_23_8::FromInt(100));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21000);

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

		man.BroadcastMessage(msg2);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11002);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12010);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 21150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 11001);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent4, IID_Test2))->GetX(), 21150);
	}

	void test_ParamNode()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(ent1, CID_Test1A, noParam);
		man.AddComponent(ent2, CID_Test1A, testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
	}

	void test_script_basic()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test.js"));

		TS_ASSERT_EQUALS(man.LookupCID("TestScript1A"), (int)CID__LastNative);
		TS_ASSERT_EQUALS(man.LookupCID("TestScript1B"), (int)CID__LastNative+1);

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3;
		CParamNode noParam;

		man.AddComponent(ent1, CID_Test1A, noParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent3, man.LookupCID("TestScript1B"), noParam);
		man.AddComponent(ent3, man.LookupCID("TestScript2A"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 101000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 102000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent3, IID_Test2))->GetX(), 201000);

		CMessageTurnStart msg1;
		CMessageUpdate msg2(CFixed_23_8::FromInt(25));

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
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-helper.js"));
		TS_ASSERT(man.LoadScript(L"simulation/helpers/test-helper.js"));

		entity_id_t ent1 = 1;
		CParamNode noParam;

		man.AddComponent(ent1, man.LookupCID("TestScript1_Helper"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 3);
	}

	void test_script_errors()
	{
		CSimContext context;
		CComponentManager man(context);
		ScriptTestSetup(man.m_ScriptInterface);
		man.LoadComponentTypes();

		{
			TestLogger log;
			TS_ASSERT(man.LoadScript(L"simulation/components/error.js"));
#if JS_VERSION >= 180
			// In SpiderMonkey 1.6, JS_ReportError calls the error reporter even if it's inside
			// a try{} in the script; in recent versions (not sure when it changed) it doesn't
			// so the error here won't get reported.
			TS_ASSERT_WSTR_NOT_CONTAINS(log.GetOutput(), L"ERROR: JavaScript error: simulation/components/error.js line 4\nInvalid interface id");
#else
			TS_ASSERT_WSTR_CONTAINS(log.GetOutput(), L"ERROR: JavaScript error: simulation/components/error.js line 4\nInvalid interface id");
#endif
		}
	}

	void test_script_entityID()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-entityid.js"));

		entity_id_t ent1 = 1, ent2 = 234;
		CParamNode noParam;

		man.AddComponent(ent1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1A"), noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), (int)ent1);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
	}

	void test_script_QueryInterface()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-query.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		man.AddComponent(ent1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent1, man.LookupCID("TestScript2A"), noParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent2, CID_Test2A, noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 400);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 21000);
	}

	void test_script_messages()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-msg.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		man.AddComponent(ent1, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent1, man.LookupCID("TestScript2A"), noParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1A"), noParam);
		man.AddComponent(ent2, CID_Test2A, noParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 21000);

		// This GetX broadcasts messages
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test2))->GetX(), 200);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 650);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 5150);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test2))->GetX(), 26050);
	}

	void test_script_template()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-param.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<node><x>1</x><y>1<z w='100'><a>1000</a></z>0</y></node>"), PSRETURN_OK);

		man.AddComponent(ent1, man.LookupCID("TestScript1_Init"), noParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1_Init"), *testParam.GetChild("node"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1+10+100+1000);
	}

	void test_script_template_readonly()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-param.js"));

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>100</x>"), PSRETURN_OK);

		man.AddComponent(ent1, man.LookupCID("TestScript1_readonly"), testParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1_readonly"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 102);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 102);
	}

	void test_script_hotload()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		TS_ASSERT(man.LoadScript(L"simulation/components/test-hotload1.js"));

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3, ent4 = 4;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>100</x>"), PSRETURN_OK);

		man.AddComponent(ent1, man.LookupCID("HotloadA"), testParam);
		man.AddComponent(ent2, man.LookupCID("HotloadB"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 100);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 200);

		TS_ASSERT(man.LoadScript(L"simulation/components/test-hotload2.js", true));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 1000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 200);

		man.AddComponent(ent3, man.LookupCID("HotloadA"), testParam);
		man.AddComponent(ent4, man.LookupCID("HotloadB"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 1000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent4, IID_Test1))->GetX(), 200);
	}

	void test_serialization()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(ent1, CID_Test1A, noParam);
		man.AddComponent(ent1, CID_Test2A, noParam);
		man.AddComponent(ent2, CID_Test1A, testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man.QueryInterface(ent1, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 1234);

		std::stringstream debugStream;
		TS_ASSERT(man.DumpDebugState(debugStream));
		TS_ASSERT_STR_EQUALS(debugStream.str(),
				"- id: 1\n"
				"  Test1A:\n"
				"    x: 11000\n"
				"  Test2A:\n"
				"    x: 21000\n"
				"\n"
				"- id: 2\n"
				"  Test1A:\n"
				"    x: 1234\n"
				"\n"
		);

		std::string hash;
		TS_ASSERT(man.ComputeStateHash(hash));
		TS_ASSERT_EQUALS(hash.length(), (size_t)20);
		TS_ASSERT_SAME_DATA(hash.data(), "\x32\x73\x30\x3a\xf2\x52\xda\x23\x5a\x25\xca\xc8\x1e\xe3\x57\xa7\x63\xc9\x5f\x0f", 20);
		// echo -en "\x01\0\0\0\x01\0\0\0\xf8\x2a\0\0\x02\0\0\0\xd2\x04\0\0\x04\0\0\0\x01\0\0\0\x08\x52\0\0" | openssl dgst -sha1 -hex | perl -pe 's/(..)/\\x$1/g'
		//           ^^Test1A^^ ^^^ent1^^ ^^^11000^^^ ^^^ent2^^ ^^^1234^^^ ^^Test2A^^ ^^ent1^^ ^^^21000^^^

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));
		TS_ASSERT_STREAM(stateStream, 56,
				"\x02\x00\x00\x00" // num component types
				"\x06\x00\x00\x00Test1A"
				"\x02\x00\x00\x00" // num ents
				"\x01\x00\x00\x00" // ent1
				"\xf8\x2a\x00\x00" // 11000
				"\x02\x00\x00\x00" // ent2
				"\xd2\x04\x00\x00" // 1234
				"\x06\x00\x00\x00Test2A"
				"\x01\x00\x00\x00" // num ents
				"\x01\x00\x00\x00" // ent1
				"\x08\x52\x00\x00" // 21000
		);

		CSimContext context2;
		CComponentManager man2(context2);
		man2.LoadComponentTypes();

		TS_ASSERT(man2.QueryInterface(ent1, IID_Test1) == NULL);
		TS_ASSERT(man2.QueryInterface(ent1, IID_Test2) == NULL);
		TS_ASSERT(man2.QueryInterface(ent2, IID_Test1) == NULL);

		TS_ASSERT(man2.DeserializeState(stateStream));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent1, IID_Test1))->GetX(), 11000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (man2.QueryInterface(ent1, IID_Test2))->GetX(), 21000);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
	}

	void test_script_serialization()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));

		entity_id_t ent1 = 1, ent2 = 2, ent3 = 3;
		CParamNode noParam;

		CParamNode testParam;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(testParam, "<x>1234</x>"), PSRETURN_OK);

		man.AddComponent(ent1, man.LookupCID("TestScript1_values"), testParam);
		man.AddComponent(ent2, man.LookupCID("TestScript1_entity"), testParam);
		man.AddComponent(ent3, man.LookupCID("TestScript1_nontree"), testParam);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent1, IID_Test1))->GetX(), 1234);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent3, IID_Test1))->GetX(), 8);

		std::stringstream debugStream;
		TS_ASSERT(man.DumpDebugState(debugStream));
		TS_ASSERT_STR_EQUALS(debugStream.str(),
				"- id: 1\n"
				"  TestScript1_values:\n"
				"    object: ({x:1234, str:\"this is a string\", things:{a:1, b:\"2\", c:[3, \"4\", [5, []]]}})\n"
				"\n"
				"- id: 2\n"
				"  TestScript1_entity:\n"
				"    object: ({})\n"
				"\n"
				"- id: 3\n"
				"  TestScript1_nontree:\n"
				"    object: ({x:#2=[#1=[2], #1#, #2#, {y:#1#}]})\n"
				"\n"
		);

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));

		CSimContext context2;
		CComponentManager man2(context2);
		man2.LoadComponentTypes();
		TS_ASSERT(man2.LoadScript(L"simulation/components/test-serialize.js"));

		TS_ASSERT(man2.QueryInterface(ent1, IID_Test1) == NULL);
		TS_ASSERT(man2.DeserializeState(stateStream));
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent1, IID_Test1))->GetX(), 1234);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), (int)ent2);
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent3, IID_Test1))->GetX(), 12);
	}

	void test_script_serialization_errors()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));

		entity_id_t ent1 = 1;
		CParamNode noParam;

		man.AddComponent(ent1, man.LookupCID("TestScript1_getter"), noParam);

		std::stringstream stateStream;
		TS_ASSERT_THROWS_PSERROR(man.SerializeState(stateStream), PSERROR_Serialize_ScriptError, "Cannot serialize property getters");
	}

	void test_script_serialization_template()
	{
		CSimContext context;
		CComponentManager man(context);
		man.LoadComponentTypes();
		TS_ASSERT(man.LoadScript(L"simulation/components/test-serialize.js"));

		entity_id_t ent2 = 2;
		CParamNode noParam;

		// The template manager takes care of reloading templates on deserialization,
		// so we need to use it here
		TS_ASSERT(man.AddComponent(SYSTEM_ENTITY, CID_TemplateManager, noParam));
		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager));

		const CParamNode* testParam = tempMan->LoadTemplate(ent2, L"template-serialize", -1);

		man.AddComponent(ent2, man.LookupCID("TestScript1_consts"), *testParam->GetChild("TestScript1_consts"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man.QueryInterface(ent2, IID_Test1))->GetX(), 12347);

		std::stringstream stateStream;
		TS_ASSERT(man.SerializeState(stateStream));

		CSimContext context2;
		CComponentManager man2(context2);
		man2.LoadComponentTypes();
		TS_ASSERT(man2.LoadScript(L"simulation/components/test-serialize.js"));

		TS_ASSERT(man2.DeserializeState(stateStream));
		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (man2.QueryInterface(ent2, IID_Test1))->GetX(), 12347);
	}
};
