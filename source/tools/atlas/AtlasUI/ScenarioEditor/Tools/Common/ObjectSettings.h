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

#include "ScenarioEditor/Tools/Common/MiscState.h"

class ScriptInterface;

namespace AtlasMessage
{
	struct sObjectSettings;
}

// This class is now just an interface to the JS Atlas.State.objectSettings,
// for old C++ code that hasn't been ported to JS yet.
class ObjectSettings
{
public:
	ObjectSettings(Observable<std::vector<AtlasMessage::ObjectID> >& selectedObjects, ScriptInterface& scriptInterface);
	void Init(int view);

	void SetPlayerID(int playerID);

	// Constructs new sObjectSettings object from settings
	AtlasMessage::sObjectSettings GetSettings() const;

	void NotifyObservers();

private:
	ScriptInterface& m_ScriptInterface;

	// Observe changes to unit selection
	ObservableScopedConnection m_Conn;
	void OnSelectionChange(const std::vector<AtlasMessage::ObjectID>& selection);
};

#endif // INCLUDED_OBJECTSETTINGS
