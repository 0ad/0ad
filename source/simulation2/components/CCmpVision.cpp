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
#include "ICmpVision.h"

class CCmpVision : public ICmpVision
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Vision)

	// Template state:

	entity_pos_t m_Range;
	bool m_RetainInFog;
	bool m_AlwaysVisible;

	static std::string GetSchema()
	{
		return
			"<element name='Range'>"
				"<data type='nonNegativeInteger'/>"
			"</element>"
			"<element name='RetainInFog'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='AlwaysVisible'>"
				"<data type='boolean'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_Range = paramNode.GetChild("Range").ToFixed();
		m_RetainInFog = paramNode.GetChild("RetainInFog").ToBool();
		m_AlwaysVisible = paramNode.GetChild("AlwaysVisible").ToBool();
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// No dynamic state to serialize
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual entity_pos_t GetRange()
	{
		return m_Range;
	}

	virtual bool GetRetainInFog()
	{
		return m_RetainInFog;
	}

	virtual bool GetAlwaysVisible()
	{
		return m_AlwaysVisible;
	}
};

REGISTER_COMPONENT_TYPE(Vision)
