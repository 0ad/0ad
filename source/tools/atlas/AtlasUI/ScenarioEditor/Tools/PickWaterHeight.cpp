/* Copyright (C) 2019 Wildfire Games.
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

#include "Common/Tools.h"
#include "Common/MiscState.h"
#include "GameInterface/Messages.h"
#include "ScenarioEditor/Sections/Environment/Environment.h"
#include "ScenarioEditor/ScenarioEditor.h"

using AtlasMessage::Position;

class PickWaterHeight : public StateDrivenTool<PickWaterHeight>
{
	DECLARE_DYNAMIC_CLASS(PickWaterHeight);

	// Uses a workaround to notify the environment settings directly, because
	// we don't have any way to update them on the engine state change.
	EnvironmentSidebar* m_Sidebar;

public:
	PickWaterHeight()
		: m_Sidebar(nullptr)
	{
		SetState(&Waiting);
	}

	virtual void Init(void* initData, ScenarioEditor* scenarioEditor)
	{
		StateDrivenTool<PickWaterHeight>::Init(initData, scenarioEditor);

		wxASSERT(initData);
		m_Sidebar = static_cast<EnvironmentSidebar*>(initData);
	}

	void OnDisable()
	{
		if (m_Sidebar)
			m_Sidebar->UpdateEnvironmentSettings();
	}

	struct sWaiting : public State
	{
		bool OnMouse(PickWaterHeight* WXUNUSED(obj), wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				POST_COMMAND(PickWaterHeight, (evt.GetPosition()));
				return true;
			}
			return false;
		}
	}
	Waiting;
};

IMPLEMENT_DYNAMIC_CLASS(PickWaterHeight, StateDrivenTool<PickWaterHeight>);
