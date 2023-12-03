/* Copyright (C) 2022 Wildfire Games.
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
#include "ICmpTest.h"

#include "simulation2/scripting/ScriptComponent.h"
#include "simulation2/MessageTypes.h"

class CCmpTest1A : public ICmpTest1
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Test1A)

	int32_t m_x;

	static std::string GetSchema()
	{
		return "<a:component type='test'/><ref name='anything'/>";
	}

	void Init(const CParamNode& paramNode) override
	{
		if (paramNode.GetChild("x").IsOk())
			m_x = paramNode.GetChild("x").ToInt();
		else
			m_x = 11000;
	}

	void Deinit() override
	{
	}

	void Serialize(ISerializer& serialize) override
	{
		serialize.NumberI32_Unbounded("x", m_x);
	}

	void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize) override
	{
		deserialize.NumberI32_Unbounded("x", m_x);
	}

	int GetX() override
	{
		return m_x;
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_Destroy:
			GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, false);
			break;
		case MT_TurnStart:
			m_x += 1;
			break;
		case MT_Interpolate:
			m_x += 2;
			break;
		default:
			m_x = 0;
			break;
		}
	}
};

REGISTER_COMPONENT_TYPE(Test1A)

class CCmpTest1B : public ICmpTest1
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeGloballyToMessageType(MT_Interpolate);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Test1B)

	int32_t m_x;

	static std::string GetSchema()
	{
		return "<a:component type='test'/><empty/>";
	}

	void Init(const CParamNode&) override
	{
		m_x = 12000;
	}

	void Deinit() override
	{
	}

	void Serialize(ISerializer& serialize) override
	{
		serialize.NumberI32_Unbounded("x", m_x);
	}

	void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize) override
	{
		deserialize.NumberI32_Unbounded("x", m_x);
	}

	int GetX() override
	{
		return m_x;
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_Update:
			m_x += 10;
			break;
		case MT_Interpolate:
			m_x += 20;
			break;
		default:
			m_x = 0;
			break;
		}
	}
};

REGISTER_COMPONENT_TYPE(Test1B)

class CCmpTest2A : public ICmpTest2
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Test2A)

	int32_t m_x;

	static std::string GetSchema()
	{
		return "<a:component type='test'/><empty/>";
	}

	void Init(const CParamNode&) override
	{
		m_x = 21000;
	}

	void Deinit() override
	{
	}

	void Serialize(ISerializer& serialize) override
	{
		serialize.NumberI32_Unbounded("x", m_x);
	}

	void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize) override
	{
		deserialize.NumberI32_Unbounded("x", m_x);
	}

	int GetX() override
	{
		return m_x;
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_TurnStart:
			m_x += 50;
			break;
		case MT_Update:
			m_x += static_cast<const CMessageUpdate&> (msg).turnLength.ToInt_RoundToZero();
			break;
		default:
			m_x = 0;
			break;
		}
	}
};

REGISTER_COMPONENT_TYPE(Test2A)

////////////////////////////////////////////////////////////////

class CCmpTest1Scripted : public ICmpTest1
{
public:
	DEFAULT_SCRIPT_WRAPPER(Test1Scripted)

	int GetX() override
	{
		return m_Script.Call<int> ("GetX");
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(Test1Scripted)

////////////////////////////////////////////////////////////////

class CCmpTest2Scripted : public ICmpTest2
{
public:
	DEFAULT_SCRIPT_WRAPPER(Test2Scripted)

	int GetX() override
	{
		return m_Script.Call<int> ("GetX");
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(Test2Scripted)
