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

#include "AutoRooters.h"

#include "scriptinterface/ScriptInterface.h"

AutoGCRooter::AutoGCRooter(ScriptInterface& scriptInterface)
	: m_ScriptInterface(scriptInterface)
{
	m_Previous = m_ScriptInterface.ReplaceAutoGCRooter(this);
}

AutoGCRooter::~AutoGCRooter()
{
	AutoGCRooter* r = m_ScriptInterface.ReplaceAutoGCRooter(m_Previous);
	ENSURE(r == this); // must be correctly nested
}

void AutoGCRooter::Trace(JSTracer* trc)
{
	if (m_Previous)
		m_Previous->Trace(trc);

	for (size_t i = 0; i < m_Objects.size(); ++i)
	{
		JS_CALL_OBJECT_TRACER(trc, m_Objects[i], "AutoGCRooter object");
	}

	for (size_t i = 0; i < m_Vals.size(); ++i)
	{
		JS_CALL_VALUE_TRACER(trc, m_Vals[i], "AutoGCRooter val");
	}

	for (size_t i = 0; i < m_IdArrays.size(); ++i)
	{
		for (jsint j = 0; j < m_IdArrays[i]->length; ++j)
		{
			jsval val = JSVAL_VOID;
			JS_IdToValue(m_ScriptInterface.GetContext(), m_IdArrays[i]->vector[j], &val);
			JS_CALL_VALUE_TRACER(trc, val, "AutoGCRooter id array");
		}
	}

}
