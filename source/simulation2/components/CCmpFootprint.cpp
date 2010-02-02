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
#include "ICmpFootprint.h"

#include "simulation2/MessageTypes.h"

class CCmpFootprint : public ICmpFootprint
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Footprint)

	EShape m_Shape;
	CFixed_23_8 m_Size0; // width/radius
	CFixed_23_8 m_Size1; // height/radius
	CFixed_23_8 m_Height;

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& paramNode)
	{
		if (paramNode.GetChild("Square").IsOk())
		{
			m_Shape = SQUARE;
			m_Size0 = paramNode.GetChild("Square").GetChild("@width").ToFixed();
			m_Size1 = paramNode.GetChild("Square").GetChild("@depth").ToFixed();
		}
		else if (paramNode.GetChild("Circle").IsOk())
		{
			m_Shape = CIRCLE;
			m_Size0 = m_Size1 = paramNode.GetChild("Circle").GetChild("@radius").ToFixed();
		}
		else
		{
			// Error - pick some default
			m_Shape = CIRCLE;
			m_Size0 = m_Size1 = CFixed_23_8::FromInt(1);
		}

		m_Height = paramNode.GetChild("Height").ToFixed();
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

	virtual void GetShape(EShape& shape, CFixed_23_8& size0, CFixed_23_8& size1, CFixed_23_8& height)
	{
		shape = m_Shape;
		size0 = m_Size0;
		size1 = m_Size1;
		height = m_Height;
	}
};

REGISTER_COMPONENT_TYPE(Footprint)
