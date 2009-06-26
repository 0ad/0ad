/* Copyright (C) 2009 Wildfire Games.
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

#include "ObjectSettings.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "AtlasScript/ScriptInterface.h"

ObjectSettings::ObjectSettings(Observable<std::vector<AtlasMessage::ObjectID> >& selectedObjects, ScriptInterface& scriptInterface)
: m_ScriptInterface(scriptInterface)
{
 	m_Conn = selectedObjects.RegisterObserver(0, &ObjectSettings::OnSelectionChange, this);
}

void ObjectSettings::Init(int view)
{
	m_ScriptInterface.SetValue(_T("Atlas.State.objectSettings.view"), view);
}

void ObjectSettings::SetPlayerID(int playerID)
{
	m_ScriptInterface.SetValue(_T("Atlas.State.objectSettings.playerID"), playerID);
}

AtlasMessage::sObjectSettings ObjectSettings::GetSettings() const
{
	AtlasMessage::sObjectSettings settings;
	bool ok = m_ScriptInterface.Eval(_T("Atlas.State.objectSettings.toSObjectSettings()"), settings);
	wxASSERT(ok);
	return settings;
}

void ObjectSettings::OnSelectionChange(const std::vector<AtlasMessage::ObjectID>& selection)
{
	// Convert to ints so they can be passed to JS
	std::vector<int> objs (selection.begin(), selection.end());

	m_ScriptInterface.SetValue(_T("Atlas.State.objectSettings.selectedObjects"), objs);
	m_ScriptInterface.Eval(_T("Atlas.State.objectSettings.onSelectionChange()"));
}

void ObjectSettings::NotifyObservers()
{
	m_ScriptInterface.Eval(_T("Atlas.State.objectSettings.notifyObservers()"));
}
