/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_SAVEDGAME
#define INCLUDED_SAVEDGAME

class CSimulation2;
class ScriptInterface;
class CScriptValRooted;
class CGUIManager;

namespace SavedGames
{

Status Save(const std::wstring& prefix, CSimulation2& simulation, CGUIManager* gui, int playerIDo);

Status Load(const std::wstring& name, ScriptInterface& scriptInterface, CScriptValRooted& metadata, std::string& savedState);

std::vector<CScriptValRooted> GetSavedGames(ScriptInterface& scriptInterface);

}

#endif // INCLUDED_SAVEDGAME
