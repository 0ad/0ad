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

#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/XML/Xeromyces.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/Component.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/HashSerializer.h"
#include "simulation2/serialization/StdSerializer.h"
#include "simulation2/serialization/StdDeserializer.h"

/**
 * @file
 * Various common features for component test cases.
 */

/**
 * Class to test a single component.
 * - Create an instance of this class
 * - Use AddMock to add mock components that the tested component relies on
 * - Use Add to add the test component itself, and it returns a component pointer
 * - Call methods on the component pointer
 * - Use Roundtrip to test the consistency of serialization
 */
class ComponentTestHelper
{
	CSimContext m_Context;
	CComponentManager m_ComponentManager;
	CParamNode m_Param;
	IComponent* m_Cmp;
	EComponentTypeId m_Cid;

public:
	ComponentTestHelper() :
		m_Context(), m_ComponentManager(m_Context), m_Cmp(NULL)
	{
		m_Context.SetComponentManager(&m_ComponentManager);
		m_ComponentManager.LoadComponentTypes();
	}

	ScriptInterface& GetScriptInterface()
	{
		return m_ComponentManager.GetScriptInterface();
	}

	/**
	 * Call this once to initialise the test helper with a component.
	 */
	template<typename T>
	T* Add(EComponentTypeId cid, const std::string& xml)
	{
		TS_ASSERT(m_Cmp == NULL);
		entity_id_t ent = 1;

		m_Cid = cid;
		TS_ASSERT_EQUALS(CParamNode::LoadXMLString(m_Param, ("<test>" + xml + "</test>").c_str()), PSRETURN_OK);
		TS_ASSERT(m_ComponentManager.AddComponent(ent, m_Cid, *m_Param.GetChild("test")));
		m_Cmp = m_ComponentManager.QueryInterface(ent, T::GetInterfaceId());
		TS_ASSERT(m_Cmp != NULL);
		return static_cast<T*> (m_Cmp);
	}

	void AddMock(entity_id_t ent, EInterfaceId iid, IComponent& component)
	{
		m_ComponentManager.AddMockComponent(ent, iid, component);
	}

	void HandleMessage(IComponent* cmp, const CMessage& msg)
	{
		cmp->HandleMessage(m_Context, msg);
	}

	/**
	 * Checks that the object roundtrips through its serialize/deserialize functions correctly.
	 * Computes the debug output, hash, and binary serialization; then deserializes into a new
	 * system and checks the serialization outputs are unchanged.
	 */
	void Roundtrip(bool verbose = false)
	{
		entity_id_t ent = 1;

		std::stringstream dbgstr1;
		CDebugSerializer dbg1(GetScriptInterface(), dbgstr1);
		m_Cmp->Serialize(dbg1);

		if (verbose)
			std::cout << "--------\n" << dbgstr1.str() << "--------\n";

		CHashSerializer hash1(GetScriptInterface());
		m_Cmp->Serialize(hash1);

		std::stringstream stdstr1;
		CStdSerializer std1(GetScriptInterface(), stdstr1);
		m_Cmp->Serialize(std1);

		ComponentTestHelper test2;
		// (We should never need to add any mock objects etc to test2, since deserialization
		// mustn't depend on other components already existing)

		CStdDeserializer stdde2(test2.GetScriptInterface(), stdstr1);

		IComponent* cmp2 = test2.m_ComponentManager.ConstructComponent(ent, m_Cid);
		cmp2->Deserialize(test2.m_Context, *m_Param.GetChild("test"), stdde2);

		TS_ASSERT(stdstr1.peek() == EOF); // Deserialize must read whole stream

		std::stringstream dbgstr2;
		CDebugSerializer dbg2(test2.GetScriptInterface(), dbgstr2);
		cmp2->Serialize(dbg2);

		if (verbose)
			std::cout << "--------\n" << dbgstr2.str() << "--------\n";

		CHashSerializer hash2(test2.GetScriptInterface());
		cmp2->Serialize(hash2);

		std::stringstream stdstr2;
		CStdSerializer std2(test2.GetScriptInterface(), stdstr2);
		cmp2->Serialize(std2);

		TS_ASSERT_EQUALS(dbgstr1.str(), dbgstr2.str());

		TS_ASSERT_EQUALS(hash1.GetHashLength(), hash2.GetHashLength());
		TS_ASSERT_SAME_DATA(hash1.ComputeHash(), hash2.ComputeHash(), hash1.GetHashLength());

		TS_ASSERT_EQUALS(stdstr1.str(), stdstr2.str());

		// TODO: need to extend this so callers can run methods on the cloned component
		// to check that all its data is still correct
	}
};

/**
 * Simple terrain implementation with constant height of 50.
 */
class MockTerrain : public ICmpTerrain
{
public:
	DEFAULT_MOCK_COMPONENT()

	virtual CFixedVector3D CalcNormal(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z))
	{
		return CFixedVector3D(CFixed_23_8::FromInt(0), CFixed_23_8::FromInt(1), CFixed_23_8::FromInt(0));
	}

	virtual entity_pos_t GetGroundLevel(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z))
	{
		return CFixed_23_8::FromInt(50);
	}
};

