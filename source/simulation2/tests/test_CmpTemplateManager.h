/* Copyright (C) 2017 Wildfire Games.
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

#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpTest.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/Simulation2.h"

#include "graphics/Terrain.h"
#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "ps/XML/Xeromyces.h"

class TestCmpTemplateManager : public CxxTest::TestSuite
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

	void test_LoadTemplate()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.LookupEntityHandle(ent1, true);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(hnd1, CID_TemplateManager, noParam));

		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(ent1, IID_TemplateManager));
		TS_ASSERT(tempMan != NULL);

		const CParamNode* basic = tempMan->LoadTemplate(ent2, "basic");
		TS_ASSERT(basic != NULL);
		TS_ASSERT_WSTR_EQUALS(basic->ToXML(), L"<Test1A>12345</Test1A>");

		const CParamNode* inherit2 = tempMan->LoadTemplate(ent2, "inherit2");
		TS_ASSERT(inherit2 != NULL);
		TS_ASSERT_WSTR_EQUALS(inherit2->ToXML(), L"<Test1A a=\"a2\" b=\"b1\" c=\"c1\"><d>d2</d><e>e1</e><f>f1</f><g>g2</g></Test1A>");

		const CParamNode* inherit1 = tempMan->LoadTemplate(ent2, "inherit1");
		TS_ASSERT(inherit1 != NULL);
		TS_ASSERT_WSTR_EQUALS(inherit1->ToXML(), L"<Test1A a=\"a1\" b=\"b1\" c=\"c1\"><d>d1</d><e>e1</e><f>f1</f></Test1A>");

		const CParamNode* actor = tempMan->LoadTemplate(ent2, "actor|example1");
		TS_ASSERT(actor != NULL);
		TS_ASSERT_WSTR_EQUALS(actor->ToXML(),
				L"<Footprint><Circle radius=\"2.0\"></Circle><Height>1.0</Height></Footprint><Selectable><EditorOnly></EditorOnly><Overlay><Texture><MainTexture>actor.png</MainTexture><MainTextureMask>actor_mask.png</MainTextureMask></Texture></Overlay></Selectable>"
				L"<VisualActor><Actor>example1</Actor><ActorOnly></ActorOnly><SilhouetteDisplay>false</SilhouetteDisplay><SilhouetteOccluder>false</SilhouetteOccluder><VisibleInAtlasOnly>false</VisibleInAtlasOnly></VisualActor>");
	}

	void test_LoadTemplate_scriptcache()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.LookupEntityHandle(ent1, true);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(hnd1, CID_TemplateManager, noParam));

		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(ent1, IID_TemplateManager));
		TS_ASSERT(tempMan != NULL);
		tempMan->DisableValidation();

		JSContext* cx = man.GetScriptInterface().GetContext();
		JSAutoRequest rq(cx);

		// This is testing some bugs in the template JS object caching

		const CParamNode* inherit1 = tempMan->LoadTemplate(ent2, "inherit1");
		JS::RootedValue val(cx);
		ScriptInterface::ToJSVal(cx, &val, inherit1);
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({Test1A:{'@a':\"a1\", '@b':\"b1\", '@c':\"c1\", d:\"d1\", e:\"e1\", f:\"f1\"}})");

		const CParamNode* inherit2 = tempMan->LoadTemplate(ent2, "inherit2");
		ScriptInterface::ToJSVal(cx, &val, inherit2);
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({'@parent':\"inherit1\", Test1A:{'@a':\"a2\", '@b':\"b1\", '@c':\"c1\", d:\"d2\", e:\"e1\", f:\"f1\", g:\"g2\"}})");

		const CParamNode* actor = tempMan->LoadTemplate(ent2, "actor|example1");
		ScriptInterface::ToJSVal(cx, &val, &actor->GetChild("VisualActor"));
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({Actor:\"example1\", ActorOnly:(void 0), SilhouetteDisplay:\"false\", SilhouetteOccluder:\"false\", VisibleInAtlasOnly:\"false\"})");

		const CParamNode* foundation = tempMan->LoadTemplate(ent2, "foundation|actor|example1");
		ScriptInterface::ToJSVal(cx, &val, &foundation->GetChild("VisualActor"));
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({Actor:\"example1\", ActorOnly:(void 0), Foundation:(void 0), SilhouetteDisplay:\"false\", SilhouetteOccluder:\"false\", VisibleInAtlasOnly:\"false\"})");

#define GET_FIRST_ELEMENT(n, templateName) \
		const CParamNode* n = tempMan->LoadTemplate(ent2, templateName); \
		for (CParamNode::ChildrenMap::const_iterator it = n->GetChildren().begin(); it != n->GetChildren().end(); ++it) \
		{ \
			if (it->first[0] == '@') \
				continue; \
			ScriptInterface::ToJSVal(cx, &val, it->second); \
			break; \
		}

		GET_FIRST_ELEMENT(n1, "inherit_a");
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({'@datatype':\"tokens\", _string:\"a b c\"})");
		GET_FIRST_ELEMENT(n2, "inherit_b");
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({'@datatype':\"tokens\", _string:\"a b c d\"})");

		GET_FIRST_ELEMENT(n3, "inherit_c");
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({'@a':\"b\", _string:\"a b c\"})");
		GET_FIRST_ELEMENT(n4, "inherit_d");
		TS_ASSERT_STR_EQUALS(man.GetScriptInterface().ToString(&val), "({'@a':\"b\", '@c':\"d\"})");

#undef GET_FIRST_ELEMENT
	}

	void test_LoadTemplate_errors()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.LookupEntityHandle(ent1, true);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(hnd1, CID_TemplateManager, noParam));

		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(ent1, IID_TemplateManager));
		TS_ASSERT(tempMan != NULL);

		TestLogger logger;

		TS_ASSERT(tempMan->LoadTemplate(ent2, "illformed") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "invalid") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "nonexistent") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-loop") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-broken") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-special") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "preview|nonexistent") == NULL);
	}

	void test_LoadTemplate_multiple()
	{
		CSimContext context;
		CComponentManager man(context, g_ScriptRuntime);
		man.LoadComponentTypes();

		entity_id_t ent1 = 1, ent2 = 2;
		CEntityHandle hnd1 = man.LookupEntityHandle(ent1, true);
		CParamNode noParam;

		TS_ASSERT(man.AddComponent(hnd1, CID_TemplateManager, noParam));

		ICmpTemplateManager* tempMan = static_cast<ICmpTemplateManager*> (man.QueryInterface(ent1, IID_TemplateManager));
		TS_ASSERT(tempMan != NULL);

		const CParamNode* basicA = tempMan->LoadTemplate(ent2, "basic");
		TS_ASSERT(basicA != NULL);

		const CParamNode* basicB = tempMan->LoadTemplate(ent2, "basic");
		TS_ASSERT(basicA == basicB);

		const CParamNode* inherit2A = tempMan->LoadTemplate(ent2, "inherit2");
		TS_ASSERT(inherit2A != NULL);

		const CParamNode* inherit2B = tempMan->LoadTemplate(ent2, "inherit2");
		TS_ASSERT(inherit2A == inherit2B);

		TestLogger logger;

		TS_ASSERT(tempMan->LoadTemplate(ent2, "nonexistent") == NULL);
		TS_ASSERT(tempMan->LoadTemplate(ent2, "nonexistent") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-loop") == NULL);
		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-loop") == NULL);

		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-broken") == NULL);
		TS_ASSERT(tempMan->LoadTemplate(ent2, "inherit-broken") == NULL);
	}
};

class TestCmpTemplateManager_2 : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"mod", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"public", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir()/"_testcache"));
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	// This just attempts loading every public entity, to check there's no validation errors
	void test_load_all_DISABLED() // disabled since it's a bit slow and noisy
	{
		CTerrain dummy;
		CSimulation2 sim(NULL, g_ScriptRuntime, &dummy);
		sim.LoadDefaultScripts();
		sim.ResetState();

		CmpPtr<ICmpTemplateManager> cmpTemplateManager(sim, SYSTEM_ENTITY);
		TS_ASSERT(cmpTemplateManager);

		std::vector<std::string> templates = cmpTemplateManager->FindAllTemplates(true);
		for (size_t i = 0; i < templates.size(); ++i)
		{
			std::string name = templates[i];
			printf("# %s\n", name.c_str());
			const CParamNode* p = cmpTemplateManager->GetTemplate(name);
			TS_ASSERT(p != NULL);
		}
	}
};
