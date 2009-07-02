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

#include "ScenarioEditor/ScenarioEditor.h"
#include "AtlasScript/ScriptInterface.h"

class ScriptedTool : public StateDrivenTool<ScriptedTool>
{
	DECLARE_DYNAMIC_CLASS(ScriptedTool);

	jsval m_Tool;
	ScriptInterface* m_ScriptInterface;

public:
	ScriptedTool()
	{
	}

	virtual void Init(void* initData, ScenarioEditor* scenarioEditor)
	{
		StateDrivenTool<ScriptedTool>::Init(initData, scenarioEditor);

		m_ScriptInterface = &scenarioEditor->GetScriptInterface();
		wxASSERT(initData);
		jsval& tool = *static_cast<jsval*>(initData);
		m_Tool = tool;
		m_ScriptInterface->AddRoot(&m_Tool);

		SetState(&Running);
	}

	virtual void Shutdown()
	{
		m_ScriptInterface->RemoveRoot(&m_Tool);

		StateDrivenTool<ScriptedTool>::Shutdown();
	}

	void OnEnable()
	{
		m_ScriptInterface->CallFunction(m_Tool, "onEnable");
	}

	void OnDisable()
	{
		m_ScriptInterface->CallFunction(m_Tool, "onDisable");
	}

	struct sRunning : public State
	{
		bool OnMouse(ScriptedTool* obj, wxMouseEvent& evt)
		{
			bool ret;
			if (! obj->m_ScriptInterface->CallFunction(obj->m_Tool, "onMouse", evt, ret))
				return false;
			return ret;
		}
		bool OnKey(ScriptedTool* obj, wxKeyEvent& evt, KeyEventType type)
		{
			wxString typeStr;
			switch (type) {
			case KEY_DOWN: typeStr = _T("down"); break;
			case KEY_UP: typeStr = _T("up"); break;
			case KEY_CHAR: typeStr = _T("char"); break;
			}
			bool ret;
			if (! obj->m_ScriptInterface->CallFunction(obj->m_Tool, "onKey", evt, typeStr, ret))
				return false;
			return ret;
		}
		void OnTick(ScriptedTool* obj, float dt)
		{
			bool ret;
			obj->m_ScriptInterface->CallFunction(obj->m_Tool, "onTick", dt, ret);
		}
	}
	Running;
};

IMPLEMENT_DYNAMIC_CLASS(ScriptedTool, StateDrivenTool<ScriptedTool>);
