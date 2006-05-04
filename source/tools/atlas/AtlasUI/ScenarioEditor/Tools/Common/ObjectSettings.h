#ifndef ObjectSettings_H__
#define ObjectSettings_H__

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
	ObjectSettings();
	~ObjectSettings();

	int GetPlayerID();
	void SetPlayerID(int playerID);
	void SetActorSelections(const std::set<wxString>& selections);

	AtlasMessage::sObjectSettings GetSettings() const;

private:
	// 0 = gaia, 1..inf = normal players
	int m_PlayerID;

	// Set of user-chosen actor selections, potentially a superset of any single
	// actor's possible variants (since it doesn't get reset if you select
	// a new actor, and will accumulate variant names)
	std::set<wxString> m_ActorSelections;

	// Observe changes to unit selection
	ObservableConnection m_Conn;
	void OnSelectionChange(const std::vector<AtlasMessage::ObjectID>& selection);

	// Transfer current settings to the currently selected unit (if any)
	void PostToGame();
};

extern ObjectSettings g_ObjectSettings;

#endif // ObjectSettings_H__
