/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/ColladaManager.h"
#include "graphics/Decal.h"
#include "graphics/Material.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ObjectEntry.h"
#include "graphics/ObjectManager.h"
#include "graphics/ShaderDefines.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/file/io/io.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/ProfileViewer.h"
#include "ps/VideoMode.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/dummy/Device.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

#include <memory>
#include <string_view>

namespace
{
constexpr std::string_view TEST_SKELETON_XML{R"(<?xml version="1.0" encoding="UTF-8"?><skeletons></skeletons>)"};

constexpr std::string_view TEST_MESH_XML{R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1">
  <asset>
    <unit name="meter" meter="1"/>
    <up_axis>Z_UP</up_axis>
  </asset>
  <library_geometries>
    <geometry id="Plane-mesh" name="Plane">
      <mesh>
        <source id="Plane-mesh-positions">
          <float_array id="Plane-mesh-positions-array" count="12">1 1 0 1 -1 0 -1 -0.9999998 0 -0.9999997 1 0</float_array>
        </source>
        <source id="Plane-mesh-normals">
          <float_array id="Plane-mesh-normals-array" count="3">0 0 1</float_array>
        </source>
        <source id="Plane-mesh-map-0">
          <float_array id="Plane-mesh-map-0-array" count="8">0 0 1 0 1 1 0 1</float_array>
        </source>
        <vertices id="Plane-mesh-vertices">
          <input semantic="POSITION" source="#Plane-mesh-positions"/>
        </vertices>
        <polylist count="1">
          <input semantic="VERTEX" source="#Plane-mesh-vertices" offset="0"/>
          <input semantic="NORMAL" source="#Plane-mesh-normals" offset="1"/>
          <input semantic="TEXCOORD" source="#Plane-mesh-map-0" offset="2" set="0"/>
          <vcount>4 </vcount>
          <p>0 0 0 3 0 1 2 0 2 1 0 3</p>
        </polylist>
      </mesh>
    </geometry>
  </library_geometries>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
      <node id="Plane" type="NODE"><instance_geometry url="#Plane-mesh"/></node>
    </visual_scene>
  </library_visual_scenes>
  <scene><instance_visual_scene url="#Scene"/></scene>
</COLLADA>)"};

constexpr std::string_view TEST_ACTOR_WITH_SHADOWS_NAME{"test_with_shadows.xml"};
constexpr std::string_view TEST_ACTOR_WITH_SHADOWS_XML{R"(<?xml version="1.0" encoding="utf-8"?>
<actor version="1">
	<castshadow/>
	<group>
		<variant><mesh>test.dae</mesh></variant>
	</group>
</actor>)"};
}

class TestModel : public CxxTest::TestSuite
{
	OsPath m_ModPath;
	OsPath m_CachePath;
	std::unique_ptr<CProfileViewer> m_Viewer;
	std::unique_ptr<CRenderer> m_Renderer;

public:
	void setUp()
	{
		g_VFS = CreateVfs();

		CConfigDB::Initialise();
		CConfigDB::Instance()->SetValueString(CFG_SYSTEM, "rendererbackend", "dummy");
		CXeromyces::Startup();

		TestLogger logger;

		g_VideoMode.InitNonSDL();
		g_VideoMode.CreateBackendDevice(false);
		m_Viewer = std::make_unique<CProfileViewer>();
		m_Renderer = std::make_unique<CRenderer>(g_VideoMode.GetBackendDevice());

		m_ModPath = DataDir() / "mods" / "_test.model" / "";
		m_CachePath = DataDir() / "_testcache" / "";

		const OsPath skeletonsPath = m_ModPath / "art" / "skeletons";
		TS_ASSERT_EQUALS(INFO::OK, CreateDirectories(skeletonsPath, 0700, false));

		const OsPath meshesPath = m_ModPath / "art" / "meshes";
		TS_ASSERT_EQUALS(INFO::OK, CreateDirectories(meshesPath, 0700, false));

		const OsPath testSkeletonPath = skeletonsPath / "test.xml";
		TS_ASSERT_EQUALS(INFO::OK, io::Store(testSkeletonPath, TEST_SKELETON_XML.data(), TEST_SKELETON_XML.size()));

		const OsPath testMeshPath = meshesPath / "test.dae";
		TS_ASSERT_EQUALS(INFO::OK, io::Store(testMeshPath, TEST_MESH_XML.data(), TEST_MESH_XML.size()));

		const OsPath actorsPath = m_ModPath / "art" / "actors";
		TS_ASSERT_EQUALS(INFO::OK, CreateDirectories(actorsPath, 0700, false));

		const OsPath testActorPath = actorsPath / TEST_ACTOR_WITH_SHADOWS_NAME.data();
		TS_ASSERT_EQUALS(INFO::OK, io::Store(testActorPath, TEST_ACTOR_WITH_SHADOWS_XML.data(), TEST_ACTOR_WITH_SHADOWS_XML.size()));

		TS_ASSERT_OK(g_VFS->Mount(L"", m_ModPath));
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", m_CachePath, 0, VFS_MAX_PRIORITY));
	}

	void tearDown()
	{
		m_Renderer.reset();
		m_Viewer.reset();
		g_VideoMode.Shutdown();

		CXeromyces::Terminate();
		CConfigDB::Shutdown();
		g_VFS.reset();

		DeleteDirectory(m_ModPath);
		DeleteDirectory(m_CachePath);
	}

	bool HasShaderDefine(const CShaderDefines& defines, CStrIntern define)
	{
		const auto& map = defines.GetMap();
		const auto it = map.find(define);
		return it != map.end() && it->second == str_1;
	}

	bool HasMaterialDefine(CModel* model, CStrIntern define)
	{
		return HasShaderDefine(model->GetMaterial().GetShaderDefines(), define);
	}

	bool HasMaterialDefine(CModelDecal* model, CStrIntern define)
	{
		return HasShaderDefine(model->m_Decal.m_Material.GetShaderDefines(), define);
	}

	void test_model_with_flags()
	{
		TestLogger logger;

		CMaterial material{};
		CSimulation2 simulation{nullptr, g_ScriptContext, nullptr};

		CTerrain terrain;
		terrain.Initialize(4, nullptr);

		// TODO: load a proper mock for modeldef.
		CModelDefPtr modeldef = std::make_shared<CModelDef>();

		std::unique_ptr<CModel> model = std::make_unique<CModel>(simulation, material, modeldef);

		SPropPoint propPoint{};
		model->AddProp(&propPoint, std::make_unique<CModel>(simulation, material, modeldef), nullptr);

		SDecal decal{CMaterial{}, 4.0f, 4.0f, 0.0f, 4.0f, 4.0f, false};
		model->AddProp(&propPoint, std::make_unique<CModelDecal>(&terrain, decal), nullptr);

		model->AddFlagsRec(ModelFlag::IGNORE_LOS);
		model->RemoveShadowsRec();

		TS_ASSERT(HasMaterialDefine(model.get(), str_DISABLE_RECEIVE_SHADOWS));
		TS_ASSERT(HasMaterialDefine(model.get(), str_IGNORE_LOS));
		for (const CModel::Prop& prop : model->GetProps())
		{
			TS_ASSERT(prop.m_Model->ToCModel() || prop.m_Model->ToCModelDecal());
			if (prop.m_Model->ToCModel())
			{
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_IGNORE_LOS));
			}
			else if (prop.m_Model->ToCModelDecal())
			{
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModelDecal(), str_DISABLE_RECEIVE_SHADOWS));
			}
		}

		std::unique_ptr<CModelAbstract> clonedModel = model->Clone();
		TS_ASSERT(clonedModel->ToCModel());
		TS_ASSERT(HasMaterialDefine(clonedModel->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
		TS_ASSERT(HasMaterialDefine(clonedModel->ToCModel(), str_IGNORE_LOS));

		TS_ASSERT_EQUALS(model->GetProps().size(), clonedModel->ToCModel()->GetProps().size());
		for (const CModel::Prop& prop : clonedModel->ToCModel()->GetProps())
		{
			TS_ASSERT(prop.m_Model->ToCModel() || prop.m_Model->ToCModelDecal());
			if (prop.m_Model->ToCModel())
			{
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_IGNORE_LOS));
			}
			else if (prop.m_Model->ToCModelDecal())
			{
				TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModelDecal(), str_DISABLE_RECEIVE_SHADOWS));
			}
		}
	}

	void test_unit_reload()
	{
		TestLogger logger;

		CColladaManager colladaManager{g_VFS};
		CMeshManager meshManager{colladaManager};
		CSkeletonAnimManager skeletonAnimationManager{colladaManager};

		CUnitManager unitManager;
		CSimulation2 simulation{&unitManager, g_ScriptContext, nullptr};
		CObjectManager objectManager{
			meshManager, skeletonAnimationManager, simulation};
		unitManager.SetObjectManager(objectManager);

		const CStrW actorName = CStr{TEST_ACTOR_WITH_SHADOWS_NAME}.FromUTF8();

		const entity_id_t id = 1;
		const uint32_t seed = 1;
		CUnit* unit = unitManager.CreateUnit(actorName, id, seed);
		TS_ASSERT(unit);
		CModel* model = unit->GetModel().ToCModel();
		TS_ASSERT(model);
		TS_ASSERT((model->GetFlags() & ModelFlag::CAST_SHADOWS) == ModelFlag::CAST_SHADOWS);

		auto [success, actor] = objectManager.FindActorDef(actorName);
		TS_ASSERT(success);
		const uint32_t newSeed = 2;
		// Trigger the unit reload.
		unit->SetActorSelections(actor.PickSelectionsAtRandom(newSeed));
		model = unit->GetModel().ToCModel();
		TS_ASSERT(model);
		TS_ASSERT((model->GetFlags() & ModelFlag::CAST_SHADOWS) == ModelFlag::CAST_SHADOWS);
	}
};
