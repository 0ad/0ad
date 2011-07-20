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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpTerritoryInfluence.h"

class CCmpTerritoryInfluence : public ICmpTerritoryInfluence
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(TerritoryInfluence)

	u8 m_Cost;

	static std::string GetSchema()
	{
		return
			"<element name='OverrideCost'>"
				"<data type='nonNegativeInteger'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_Cost = paramNode.GetChild("OverrideCost").ToInt();
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

	virtual u8 GetCost()
	{
		return m_Cost;
	}

};

REGISTER_COMPONENT_TYPE(TerritoryInfluence)
