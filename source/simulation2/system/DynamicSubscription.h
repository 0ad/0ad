/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_DYNAMICSUBSCRIPTION
#define INCLUDED_DYNAMICSUBSCRIPTION

#include "IComponent.h"

#include <set>
#include <vector>

/**
 * A list of components that are dynamically subscribed to a particular
 * message. The components list is sorted by (entity_id, ComponentTypeId),
 * with no duplicates.
 *
 * To cope with changes to the subscription list while a message is still
 * being broadcast, all changes are stored in the added/removed sets. The
 * next time a message is sent, they will be merged into the main components
 * list.
 */
class CDynamicSubscription
{
	struct CompareIComponent
	{
		bool operator()(const IComponent* cmpA, const IComponent* cmpB) const
		{
			entity_id_t entityA = cmpA->GetEntityId();
			entity_id_t entityB = cmpB->GetEntityId();
			if (entityA < entityB)
				return true;
			if (entityB < entityA)
				return false;
			int cidA = cmpA->GetComponentTypeId();
			int cidB = cmpB->GetComponentTypeId();
			if (cidA < cidB)
				return true;
			return false;
		}
	};
public:
	void Add(IComponent* cmp);
	void Remove(IComponent* cmp);
	void Flatten();
	const std::vector<IComponent*>& GetComponents();
	void DebugDump();

private:
	std::vector<IComponent*> m_Components; // always in CompareIComponent order
	std::set<IComponent*, CompareIComponent> m_Added;
	std::set<IComponent*, CompareIComponent> m_Removed;
};

#endif // INCLUDED_DYNAMICSUBSCRIPTION
