/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTSTATS
#define INCLUDED_SCRIPTSTATS

#include "ps/ProfileViewer.h"

class ScriptInterface;

class CScriptStatsTable : public AbstractProfileTable
{
	NONCOPYABLE(CScriptStatsTable);
public:
	CScriptStatsTable();

	void Add(const ScriptInterface* scriptInterface, const std::string& title);
	void Remove(const ScriptInterface* scriptInterface);

	virtual CStr GetName();
	virtual CStr GetTitle();
	virtual size_t GetNumberRows();
	virtual const std::vector<ProfileColumn>& GetColumns();
	virtual CStr GetCellText(size_t row, size_t col);
	virtual AbstractProfileTable* GetChild(size_t row);

private:
	std::vector<std::pair<const ScriptInterface*, std::string> > m_ScriptInterfaces;
	std::vector<ProfileColumn> m_ColumnDescriptions;
};

// To simplify the UI we want to use a single table for all script interfaces,
// so just make it a global that they can all add themselves to
extern CScriptStatsTable* g_ScriptStatsTable;

#endif // INCLUDED_SCRIPTSTATS
