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

	bool m_Root;
	u16 m_Weight;
	u32 m_Radius;

	static std::string GetSchema()
	{
		return
			"<element name='Root'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='Weight'>"
				"<data type='nonNegativeInteger'>"
					"<param name='maxInclusive'>65535</param>" // Max u16 value
				"</data>"
			"</element>"
			"<element name='Radius'>"
				"<data type='nonNegativeInteger'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_Root = paramNode.GetChild("Root").ToBool();
		m_Weight = (u16)paramNode.GetChild("Weight").ToInt();
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

	virtual bool IsRoot() const
	{
		return m_Root;
	}

	virtual u16 GetWeight() const
	{
		return m_Weight;
	}

	virtual u32 GetRadius() const
	{
		CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
		if (!cmpValueModificationManager)
			return m_Radius;

		u32 newRadius = cmpValueModificationManager->ApplyModifications(L"TerritoryInfluence/Radius", m_Radius, GetEntityId());
		return newRadius;
	}
};

REGISTER_COMPONENT_TYPE(TerritoryInfluence)
