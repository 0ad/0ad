/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_COMPONENT
#define INCLUDED_COMPONENT

// These headers are included because they are required in component implementation,
// so including them here transitively makes sense.
#include "simulation2/system/CmpPtr.h"
#include "simulation2/system/Components.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

#define REGISTER_COMPONENT_TYPE(cname) \
	void RegisterComponentType_##cname(CComponentManager& mgr) \
	{ \
		IComponent::RegisterComponentType(mgr, CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname, CCmp##cname::GetSchema()); \
		CCmp##cname::ClassInit(mgr); \
	}

#define DEFAULT_COMPONENT_ALLOCATOR(cname) \
	static IComponent* Allocate(const ScriptInterface&, JS::HandleValue) { return new CCmp##cname(); } \
	static void Deallocate(IComponent* cmp) { delete static_cast<CCmp##cname*> (cmp); } \
	int GetComponentTypeId() const override \
	{ \
		return CID_##cname; \
	}

#define DEFAULT_MOCK_COMPONENT() \
	int GetComponentTypeId() const override \
	{ \
		return -1; \
	} \
	void Init(const CParamNode& UNUSED(paramNode)) override \
	{ \
	} \
	void Deinit() override \
	{ \
	} \
	void Serialize(ISerializer& UNUSED(serialize)) override \
	{ \
	} \
	void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& UNUSED(deserialize)) override \
	{ \
	} \

#endif // INCLUDED_COMPONENT
