/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/Material.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ShaderDefines.h"
#include "ps/CStrInternStatic.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

#include <memory>

class TestModel : public CxxTest::TestSuite
{
public:
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

	void test_model_with_flags()
	{
		CMaterial material{};
		CSimulation2 simulation{nullptr, g_ScriptContext, nullptr};

		// TODO: load a proper mock for modeldef.
		CModelDefPtr modeldef = std::make_shared<CModelDef>();

		std::unique_ptr<CModel> model = std::make_unique<CModel>(simulation, material, modeldef);

		SPropPoint propPoint{};
		model->AddProp(&propPoint, std::make_unique<CModel>(simulation, material, modeldef), nullptr);

		model->AddFlagsRec(ModelFlag::IGNORE_LOS);
		model->RemoveShadowsRec();

		TS_ASSERT(HasMaterialDefine(model.get(), str_DISABLE_RECEIVE_SHADOWS));
		TS_ASSERT(HasMaterialDefine(model.get(), str_IGNORE_LOS));
		for (const CModel::Prop& prop : model->GetProps())
		{
			TS_ASSERT(prop.m_Model->ToCModel());
			TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
			TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_IGNORE_LOS));
		}

		std::unique_ptr<CModelAbstract> clonedModel = model->Clone();
		TS_ASSERT(clonedModel->ToCModel());
		TS_ASSERT(HasMaterialDefine(clonedModel->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
		TS_ASSERT(HasMaterialDefine(clonedModel->ToCModel(), str_IGNORE_LOS));

		TS_ASSERT_EQUALS(model->GetProps().size(), clonedModel->ToCModel()->GetProps().size());
		for (const CModel::Prop& prop : clonedModel->ToCModel()->GetProps())
		{
			TS_ASSERT(prop.m_Model->ToCModel());
			TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_DISABLE_RECEIVE_SHADOWS));
			TS_ASSERT(HasMaterialDefine(prop.m_Model->ToCModel(), str_IGNORE_LOS));
		}
	}
};
