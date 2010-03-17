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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpTerrain.h"

#include "graphics/Terrain.h"

class CCmpTerrain : public ICmpTerrain
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Terrain)

	CTerrain* m_Terrain; // not null

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_Terrain = &context.GetTerrain();
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);
	}

	virtual CFixedVector3D CalcNormal(entity_pos_t x, entity_pos_t z)
	{
		CFixedVector3D normal;
		m_Terrain->CalcNormalFixed((x / CELL_SIZE).ToInt_RoundToZero(), (z / CELL_SIZE).ToInt_RoundToZero(), normal);
		return normal;
	}

	virtual entity_pos_t GetGroundLevel(entity_pos_t x, entity_pos_t z)
	{
		float height = m_Terrain->GetExactGroundLevel(x.ToFloat(), z.ToFloat());
		// TODO: get rid of floats

		return entity_pos_t::FromFloat(height);
	}

	virtual float GetGroundLevel(float x, float z)
	{
		return m_Terrain->GetExactGroundLevel(x, z);
	}
};

REGISTER_COMPONENT_TYPE(Terrain)
