/* Copyright (C) 2018 Wildfire Games.
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

#include "JSInterface_ModIo.h"

#include "ps/CLogger.h"
#include "ps/Mod.h"
#include "ps/ModIo.h"

void JSI_ModIo::StartGetGameId(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_ModIo)
		g_ModIo = new ModIo();

	ENSURE(g_ModIo);

	g_ModIo->StartGetGameId();
}

void JSI_ModIo::StartListMods(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoStartListMods called before ModIoStartGetGameId");
		return;
	}

	g_ModIo->StartListMods();
}

void JSI_ModIo::StartDownloadMod(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), uint32_t idx)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoStartDownloadMod called before ModIoStartGetGameId");
		return;
	}

	g_ModIo->StartDownloadMod(idx);
}

bool JSI_ModIo::AdvanceRequest(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoAdvanceRequest called before ModIoGetMods");
		return false;
	}

	ScriptInterface* scriptInterface = pCxPrivate->pScriptInterface;
	return g_ModIo->AdvanceRequest(*scriptInterface);
}

void JSI_ModIo::CancelRequest(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoCancelRequest called before ModIoGetMods");
		return;
	}

	g_ModIo->CancelRequest();
}

JS::Value JSI_ModIo::GetMods(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoGetMods called before ModIoStartGetGameId");
		return JS::NullValue();
	}

	ScriptInterface* scriptInterface = pCxPrivate->pScriptInterface;
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	const std::vector<ModIoModData>& availableMods = g_ModIo->GetMods();

	JS::RootedObject mods(cx, JS_NewArrayObject(cx, availableMods.size()));
	if (!mods)
		return JS::NullValue();

	u32 i = 0;
	for (const ModIoModData& mod : availableMods)
	{
		JS::RootedValue m(cx, JS::ObjectValue(*JS_NewPlainObject(cx)));
		for (const std::pair<std::string, std::string>& prop : mod.properties)
			scriptInterface->SetProperty(m, prop.first.c_str(), prop.second, true);

		scriptInterface->SetProperty(m, "dependencies", mod.dependencies, true);

		JS_SetElement(cx, mods, i++, m);
	}

	return JS::ObjectValue(*mods);
}

const std::map<DownloadProgressStatus, std::string> statusStrings = {
	{ DownloadProgressStatus::NONE, "none" },
	{ DownloadProgressStatus::GAMEID, "gameid" },
	{ DownloadProgressStatus::READY, "ready" },
	{ DownloadProgressStatus::LISTING, "listing" },
	{ DownloadProgressStatus::LISTED, "listed" },
	{ DownloadProgressStatus::DOWNLOADING, "downloading" },
	{ DownloadProgressStatus::SUCCESS, "success" },
	{ DownloadProgressStatus::FAILED_GAMEID, "failed_gameid" },
	{ DownloadProgressStatus::FAILED_LISTING, "failed_listing" },
	{ DownloadProgressStatus::FAILED_DOWNLOADING, "failed_downloading" },
	{ DownloadProgressStatus::FAILED_FILECHECK, "failed_filecheck" }
};

JS::Value JSI_ModIo::GetDownloadProgress(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoGetDownloadProgress called before ModIoGetMods");
		return JS::NullValue();
	}

	ScriptInterface* scriptInterface = pCxPrivate->pScriptInterface;
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue progressData(cx, JS::ObjectValue(*JS_NewPlainObject(cx)));
	const DownloadProgressData& progress = g_ModIo->GetDownloadProgress();

	scriptInterface->SetProperty(progressData, "status", statusStrings.at(progress.status), true);
	scriptInterface->SetProperty(progressData, "progress", progress.progress, true);
	scriptInterface->SetProperty(progressData, "error", progress.error, true);

	return progressData;
}

void JSI_ModIo::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<void, &JSI_ModIo::StartGetGameId>("ModIoStartGetGameId");
	scriptInterface.RegisterFunction<void, &JSI_ModIo::StartListMods>("ModIoStartListMods");
	scriptInterface.RegisterFunction<void, uint32_t, &JSI_ModIo::StartDownloadMod>("ModIoStartDownloadMod");
	scriptInterface.RegisterFunction<bool, &JSI_ModIo::AdvanceRequest>("ModIoAdvanceRequest");
	scriptInterface.RegisterFunction<void, &JSI_ModIo::CancelRequest>("ModIoCancelRequest");
	scriptInterface.RegisterFunction<JS::Value, &JSI_ModIo::GetMods>("ModIoGetMods");
	scriptInterface.RegisterFunction<JS::Value, &JSI_ModIo::GetDownloadProgress>("ModIoGetDownloadProgress");
}
