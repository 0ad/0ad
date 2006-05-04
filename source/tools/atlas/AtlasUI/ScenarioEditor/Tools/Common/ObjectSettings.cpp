#include "stdafx.h"

#include "ObjectSettings.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

ObjectSettings g_ObjectSettings;

ObjectSettings::ObjectSettings()
: m_PlayerID(0)
{
	m_Conn = g_SelectedObjects.RegisterObserver(0, &ObjectSettings::OnSelectionChange, this);
}

ObjectSettings::~ObjectSettings()
{
	m_Conn.disconnect();
}

int ObjectSettings::GetPlayerID()
{
	return m_PlayerID;
}

void ObjectSettings::SetPlayerID(int playerID)
{
	m_PlayerID = playerID;
	PostToGame();
}

void ObjectSettings::SetActorSelections(const std::set<wxString>& selections)
{
	m_ActorSelections = selections;
	PostToGame();
}

AtlasMessage::sObjectSettings ObjectSettings::GetSettings() const
{
	AtlasMessage::sObjectSettings settings;

	settings.player = m_PlayerID;

	std::vector<std::wstring> selections;
	for (std::set<wxString>::const_iterator it = m_ActorSelections.begin(); it != m_ActorSelections.end(); ++it)
		selections.push_back(it->c_str());
	settings.selections = selections;

	return settings;
}

void ObjectSettings::OnSelectionChange(const std::vector<AtlasMessage::ObjectID>& selection)
{
	// TODO: what would be the sensible action if nothing's selected?
	// and if multiple objects are selected?

	if (selection.empty())
		return;
		
	AtlasMessage::qGetObjectSettings qry (selection[0]);
	qry.Post();

	m_PlayerID = qry.settings->player;
	std::vector<std::wstring> selections = *qry.settings->selections;

	m_ActorSelections.clear();
	for (std::vector<std::wstring>::iterator it = selections.begin(); it != selections.end(); ++it)
		m_ActorSelections.insert(it->c_str());
}

void ObjectSettings::PostToGame()
{
	if (g_SelectedObjects.empty())
		return;

	POST_COMMAND(SetObjectSettings, (g_SelectedObjects[0], GetSettings()));
}