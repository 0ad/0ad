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

#include "scriptinterface/ScriptInterface.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "maths/FixedVector3D.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Overlay.h"
#include "ps/Player.h"
#include "ps/GameSetup/Config.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/helpers/Selection.h"

/*
 * This file defines a set of functions that are available to GUI scripts, to allow
 * interaction with the rest of the engine.
 * Functions are exposed to scripts within the global object 'Engine', so
 * scripts should call "Engine.FunctionName(...)" etc.
 */

namespace {

CScriptVal GetActiveGui(void* UNUSED(cbdata))
{
	return OBJECT_TO_JSVAL(g_GUI->GetScriptObject());
}

void PushGuiPage(void* UNUSED(cbdata), std::wstring name, CScriptVal initData)
{
	g_GUI->PushPage(name, initData);
}

void SwitchGuiPage(void* UNUSED(cbdata), std::wstring name, CScriptVal initData)
{
	g_GUI->SwitchPage(name, initData);
}

void PopGuiPage(void* UNUSED(cbdata))
{
	g_GUI->PopPage();
}

bool IsNewSimulation(void* UNUSED(cbdata))
{
	return g_UseSimulation2;
}

// TODO: this should probably be moved into ScriptInterface in case anyone else wants to use it
static jsval CloneValueBetweenContexts(JSContext* cxFrom, JSContext* cxTo, jsval val)
{
	if (JSVAL_IS_INT(val) || JSVAL_IS_BOOLEAN(val) || JSVAL_IS_NULL(val) || JSVAL_IS_VOID(val))
		return val;

	if (JSVAL_IS_DOUBLE(val))
	{
		jsval rval;
		if (JS_NewNumberValue(cxTo, *JSVAL_TO_DOUBLE(val), &rval))
			return rval;
		else
			return JSVAL_VOID;
	}

	if (JSVAL_IS_STRING(val))
	{
		JSString* str = JS_NewUCStringCopyN(cxTo, JS_GetStringChars(JSVAL_TO_STRING(val)), JS_GetStringLength(JSVAL_TO_STRING(val)));
		if (str == NULL)
			return JSVAL_VOID;
		return STRING_TO_JSVAL(str);
	}

	if (JSVAL_IS_OBJECT(val))
	{
		jsval source;
		if (!JS_CallFunctionName(cxFrom, JSVAL_TO_OBJECT(val), "toSource", 0, NULL, &source))
			return JSVAL_VOID;
		if (!JSVAL_IS_STRING(source))
			return JSVAL_VOID;
		JS_AddRoot(cxFrom, &source);
		jsval rval;
		JSBool ok = JS_EvaluateUCScript(cxTo, JS_GetGlobalObject(cxTo),
				JS_GetStringChars(JSVAL_TO_STRING(source)), JS_GetStringLength(JSVAL_TO_STRING(source)),
				"(CloneValueBetweenContexts)", 0, &rval);
		JS_RemoveRoot(cxFrom, &source);
		return ok ? rval : JSVAL_VOID;
	}

	LOGERROR(L"CloneValueBetweenContexts: value is of unexpected type");
	return JSVAL_VOID;
}

CScriptVal GuiInterfaceCall(void* cbdata, std::wstring name, CScriptVal data)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	if (!g_UseSimulation2 || !g_Game)
		return JSVAL_VOID;
	CSimulation2* sim = g_Game->GetSimulation2();
	debug_assert(sim);

	CmpPtr<ICmpGuiInterface> gui(*sim, SYSTEM_ENTITY);
	if (gui.null())
		return JSVAL_VOID;

	int player = -1;
	if (g_Game && g_Game->GetLocalPlayer())
		player = g_Game->GetLocalPlayer()->GetPlayerID();

	JSContext* cxGui = guiManager->GetScriptInterface().GetContext();
	JSContext* cxSim = sim->GetScriptInterface().GetContext();
	CScriptVal ret = gui->ScriptCall(player, name, CloneValueBetweenContexts(cxGui, cxSim, data.get()));
	return CloneValueBetweenContexts(cxSim, cxGui, ret.get());
}

void PostNetworkCommand(void* cbdata, CScriptVal cmd)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	if (!g_UseSimulation2 || !g_Game)
		return;
	CSimulation2* sim = g_Game->GetSimulation2();
	debug_assert(sim);

	CmpPtr<ICmpCommandQueue> queue(*sim, SYSTEM_ENTITY);
	if (queue.null())
		return;

	int player = -1;
	if (g_Game && g_Game->GetLocalPlayer())
		player = g_Game->GetLocalPlayer()->GetPlayerID();

	jsval cmd2 = CloneValueBetweenContexts(guiManager->GetScriptInterface().GetContext(), sim->GetScriptInterface().GetContext(), cmd.get());

	queue->PushClientCommand(player, cmd2);
	// TODO: This shouldn't call Push, it should send the message to the network layer
	// (which should propagate it across the network and eventually call Push on the
	// appropriate turn)
}

std::vector<entity_id_t> PickEntitiesAtPoint(void* UNUSED(cbdata), int x, int y)
{
	return EntitySelection::PickEntitiesAtPoint(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x, y);
}

std::vector<entity_id_t> PickFriendlyEntitiesInRect(void* UNUSED(cbdata), int x0, int y0, int x1, int y1, int player)
{
	return EntitySelection::PickEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x0, y0, x1, y1, player);
}

CFixedVector3D GetTerrainAtPoint(void* UNUSED(cbdata), int x, int y)
{
	CVector3D pos = g_Game->GetView()->GetCamera()->GetWorldCoordinates(x, y, false);
	return CFixedVector3D(CFixed_23_8::FromFloat(pos.X), CFixed_23_8::FromFloat(pos.Y), CFixed_23_8::FromFloat(pos.Z));
}

std::wstring SetCursor(void* UNUSED(cbdata), std::wstring name)
{
	std::wstring old = g_CursorName;
	g_CursorName = name;
	return old;
}

int GetPlayerID(void* UNUSED(cbdata))
{
	if (g_Game && g_Game->GetLocalPlayer())
		return g_Game->GetLocalPlayer()->GetPlayerID();
	return -1;
}

} // namespace

void GuiScriptingInit(ScriptInterface& scriptInterface)
{
	// GUI manager functions:
	scriptInterface.RegisterFunction<CScriptVal, &GetActiveGui>("GetActiveGui");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &PushGuiPage>("PushGuiPage");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &SwitchGuiPage>("SwitchGuiPage");
	scriptInterface.RegisterFunction<void, &PopGuiPage>("PopGuiPage");

	// Simulation<->GUI interface functions:
	scriptInterface.RegisterFunction<bool, &IsNewSimulation>("IsNewSimulation");
	scriptInterface.RegisterFunction<CScriptVal, std::wstring, CScriptVal, &GuiInterfaceCall>("GuiInterfaceCall");
	scriptInterface.RegisterFunction<void, CScriptVal, &PostNetworkCommand>("PostNetworkCommand");

	// Entity picking
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, &PickEntitiesAtPoint>("PickEntitiesAtPoint");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, int, int, int, &PickFriendlyEntitiesInRect>("PickFriendlyEntitiesInRect");
	scriptInterface.RegisterFunction<CFixedVector3D, int, int, &GetTerrainAtPoint>("GetTerrainAtPoint");

	// Misc functions
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &SetCursor>("SetCursor");
	scriptInterface.RegisterFunction<int, &GetPlayerID>("GetPlayerID");
}
