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

#include "precompiled.h"

#include "ScriptStats.h"

#include "scriptinterface/ScriptInterface.h"

CScriptStatsTable* g_ScriptStatsTable;

enum
{
	Row_MaxBytes,
	Row_MaxMallocBytes,
	Row_Bytes,
	Row_NumberGC,
	NumberRows
};

CScriptStatsTable::CScriptStatsTable()
{
}

void CScriptStatsTable::Add(const ScriptInterface* scriptInterface, const std::string& title)
{
	m_ScriptInterfaces.push_back(std::make_pair(scriptInterface, title));
}

void CScriptStatsTable::Remove(const ScriptInterface* scriptInterface)
{
	for (size_t i = 0; i < m_ScriptInterfaces.size(); )
	{
		if (m_ScriptInterfaces[i].first == scriptInterface)
			m_ScriptInterfaces.erase(m_ScriptInterfaces.begin() + i);
		else
			++i;
	}
}

CStr CScriptStatsTable::GetName()
{
	return "script";
}

CStr CScriptStatsTable::GetTitle()
{
	return "Script statistics";
}

size_t CScriptStatsTable::GetNumberRows()
{
	return NumberRows;
}

const std::vector<ProfileColumn>& CScriptStatsTable::GetColumns()
{
	m_ColumnDescriptions.clear();
	m_ColumnDescriptions.push_back(ProfileColumn("Name", 200));
	for (size_t i = 0; i < m_ScriptInterfaces.size(); ++i)
		m_ColumnDescriptions.push_back(ProfileColumn(m_ScriptInterfaces[i].second, 80));
	return m_ColumnDescriptions;
}

CStr CScriptStatsTable::GetCellText(size_t row, size_t col)
{
	switch(row)
	{
	case Row_MaxBytes:
	{
		if (col == 0)
			return "max nominal heap bytes";
		uint32_t n = JS_GetGCParameter(m_ScriptInterfaces.at(col-1).first->GetJSRuntime(), JSGC_MAX_BYTES);
		return CStr::FromUInt(n);
	}
	case Row_MaxMallocBytes:
	{
		if (col == 0)
			return "max JS_malloc bytes";
		uint32_t n = JS_GetGCParameter(m_ScriptInterfaces.at(col-1).first->GetJSRuntime(), JSGC_MAX_MALLOC_BYTES);
		return CStr::FromUInt(n);
	}
	case Row_Bytes:
	{
		if (col == 0)
			return "allocated bytes";
		uint32_t n = JS_GetGCParameter(m_ScriptInterfaces.at(col-1).first->GetJSRuntime(), JSGC_BYTES);
		return CStr::FromUInt(n);
	}
	case Row_NumberGC:
	{
		if (col == 0)
			return "number of GCs";
		uint32_t n = JS_GetGCParameter(m_ScriptInterfaces.at(col-1).first->GetJSRuntime(), JSGC_NUMBER);
		return CStr::FromUInt(n);
	}
	default:
		return "???";
	}
}

AbstractProfileTable* CScriptStatsTable::GetChild(size_t UNUSED(row))
{
	return 0;
}
