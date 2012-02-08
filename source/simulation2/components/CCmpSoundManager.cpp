/* Copyright (C) 2012 Wildfire Games.
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
#include "ICmpSoundManager.h"

#include "ps/CLogger.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "sound/SoundGroup.h"

class CCmpSoundManager : public ICmpSoundManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(SoundManager)

	std::map<std::wstring, CSoundGroup*> m_SoundGroups;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit()
	{
		for (std::map<std::wstring, CSoundGroup*>::iterator it = m_SoundGroups.begin(); it != m_SoundGroups.end(); ++it)
			delete it->second;
		m_SoundGroups.clear();
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// Do nothing here - sounds are purely local, and don't need to be preserved across saved games etc
		// (If we add music support in here then we might want to save the music state, though)
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Update:
		{
			// Update all the sound groups
			// TODO: is it sensible to do this once per simulation turn, not once per renderer frame
			// or on some other timer?
			const CMessageUpdate& msgData = static_cast<const CMessageUpdate&> (msg);
			float t = msgData.turnLength.ToFloat();
			for (std::map<std::wstring, CSoundGroup*>::iterator it = m_SoundGroups.begin(); it != m_SoundGroups.end(); ++it)
				if (it->second)
					it->second->Update(t);
			break;
		}
		}
	}

	virtual void PlaySoundGroup(std::wstring name, entity_id_t source)
	{
		// Make sure the sound group is loaded
		CSoundGroup* group;
		if (m_SoundGroups.find(name) == m_SoundGroups.end())
		{
			group = new CSoundGroup();
			if (!group->LoadSoundGroup(L"audio/" + name))
			{
				LOGERROR(L"Failed to load sound group '%ls'", name.c_str());
				delete group;
				group = NULL;
			}
			// Cache the sound group (or the null, if it failed)
			m_SoundGroups[name] = group;
		}
		else
		{
			group = m_SoundGroups[name];
		}

		// Failed to load group -> do nothing
		if (!group)
			return;

		// Only play the sound if the entity is visible
		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSimContext(), SYSTEM_ENTITY);
		ICmpRangeManager::ELosVisibility vis = cmpRangeManager->GetLosVisibility(source, GetSimContext().GetCurrentDisplayedPlayer());

		if (vis == ICmpRangeManager::VIS_VISIBLE)
		{
			// Find the source's position, if possible
			// (TODO: we should do something more sensible if there's no position available)
			CVector3D sourcePos(0, 0, 0);
			if (source != INVALID_ENTITY)
			{
				CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), source);
				if (cmpPosition && cmpPosition->IsInWorld())
					sourcePos = CVector3D(cmpPosition->GetPosition());
			}

			group->PlayNext(sourcePos);
		}
	}
};

REGISTER_COMPONENT_TYPE(SoundManager)
