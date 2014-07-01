/* Copyright (C) 2014 Wildfire Games.
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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpTerritoryInfluence.h"

#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpValueModificationManager.h"

class CCmpTerritoryInfluence : public ICmpTerritoryInfluence
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(TerritoryInfluence)

	i32 m_Cost;
	bool m_Root;
	u32 m_Weight;
	u32 m_Radius;

	static std::string GetSchema()
	{
		return
			"<optional>"
				"<element name='OverrideCost'>"
					"<data type='nonNegativeInteger'>"
						"<param name='maxInclusive'>255</param>"
					"</data>"
				"</element>"
			"</optional>"
			"<element name='Root'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='Weight'>"
				"<data type='nonNegativeInteger'/>"
			"</element>"
			"<element name='Radius'>"
				"<data type='nonNegativeInteger'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		if (paramNode.GetChild("OverrideCost").IsOk())
			m_Cost = paramNode.GetChild("OverrideCost").ToInt();
		else
			m_Cost = -1;

		m_Root = paramNode.GetChild("Root").ToBool();
		m_Weight = paramNode.GetChild("Weight").ToInt();
		m_Radius = paramNode.GetChild("Radius").ToInt();
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual i32 GetCost()
	{
		return m_Cost;
	}

	virtual bool IsRoot()
	{
		return m_Root;
	}

	virtual u32 GetWeight()
	{
		return m_Weight;
	}

	virtual u32 GetRadius()
	{
		CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
		u32 newRadius = cmpValueModificationManager->ApplyModifications(L"TerritoryInfluence/Radius", m_Radius, GetEntityId());

		return newRadius;
	}
};

REGISTER_COMPONENT_TYPE(TerritoryInfluence)
