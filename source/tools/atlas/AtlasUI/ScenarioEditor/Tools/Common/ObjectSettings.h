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

#ifndef INCLUDED_OBJECTSETTINGS
#define INCLUDED_OBJECTSETTINGS

#include <vector>
#include <set>

#include "ScenarioEditor/Tools/Common/MiscState.h"

namespace AtlasMessage
{
	struct sObjectSettings;
}

// Various settings to be applied to newly created units, or to the currently
// selected unit. If a unit is selected or being previewed, it should match
// these settings.
class ObjectSettings
{
public:
	ObjectSettings(Observable<std::vector<AtlasMessage::ObjectID> >& selectedObjects, int view);

	size_t GetPlayerID() const;
	void SetPlayerID(int playerID);

	struct Group
	{
		wxArrayString variants;
		wxString chosen;
	};

	const std::vector<Group> GetActorVariation() const;

	const std::set<wxString>& GetActorSelections() const;
	void SetActorSelections(const std::set<wxString>& selections);

	// Constructs new sObjectSettings object from settings
	AtlasMessage::sObjectSettings GetSettings() const;

private:
	Observable<std::vector<AtlasMessage::ObjectID> >& m_SelectedObjects;

	int m_View;

	// 0 = gaia, 1..inf = normal players
	size_t m_PlayerID;

	// Set of user-chosen actor selections, potentially a superset of any single
	// actor's possible variants (since it doesn't get reset if you select
	// a new actor, and will accumulate variant names)
	std::set<wxString> m_ActorSelections;

	// List of actor variant groups (each a list of variant names)
	std::vector<wxArrayString> m_VariantGroups;

	// Observe changes to unit selection
	ObservableScopedConnection m_Conn;
	void OnSelectionChange(const std::vector<AtlasMessage::ObjectID>& selection);

	// Transfer current settings to the currently selected unit (if any)
	void PostToGame();
};

#endif // INCLUDED_OBJECTSETTINGS
