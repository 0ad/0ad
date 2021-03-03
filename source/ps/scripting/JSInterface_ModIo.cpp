/* Copyright (C) 2020 Wildfire Games.
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
#include "ps/ModIo.h"
#include "scriptinterface/FunctionWrapper.h"

namespace JSI_ModIo
{
ModIo* ModIoGetter(const ScriptRequest&, JS::CallArgs&)
{
	if (!g_ModIo)
	{
		LOGERROR("Trying to access ModIO when it's not initialized!");
		return nullptr;
	}
	return g_ModIo;
}

void StartGetGameId()
{
	if (!g_ModIo)
		g_ModIo = new ModIo();

	ENSURE(g_ModIo);

	g_ModIo->StartGetGameId();
}

// TODO: could provide a FromJSVal for ModIoModData
JS::Value GetMods(const ScriptInterface& scriptInterface)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoGetMods called before ModIoStartGetGameId");
		return JS::NullValue();
	}

	ScriptRequest rq(scriptInterface);

	const std::vector<ModIoModData>& availableMods = g_ModIo->GetMods();

	JS::RootedValue mods(rq.cx);
	ScriptInterface::CreateArray(rq, &mods, availableMods.size());

	u32 i = 0;
	for (const ModIoModData& mod : availableMods)
	{
		JS::RootedValue m(rq.cx);
		ScriptInterface::CreateObject(rq, &m);

		for (const std::pair<const std::string, std::string>& prop : mod.properties)
			scriptInterface.SetProperty(m, prop.first.c_str(), prop.second, true);

		scriptInterface.SetProperty(m, "dependencies", mod.dependencies, true);
		scriptInterface.SetPropertyInt(mods, i++, m);
	}

	return mods;
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

// TODO: could provide a FromJSVal for DownloadProgressData
JS::Value GetDownloadProgress(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_ModIo)
	{
		LOGERROR("ModIoGetDownloadProgress called before ModIoGetMods");
		return JS::NullValue();
	}

	ScriptInterface* scriptInterface = pCmptPrivate->pScriptInterface;
	ScriptRequest rq(scriptInterface);

	const DownloadProgressData& progress = g_ModIo->GetDownloadProgress();

	JS::RootedValue progressData(rq.cx);
	ScriptInterface::CreateObject(rq, &progressData);
	scriptInterface->SetProperty(progressData, "status", statusStrings.at(progress.status), true);
	scriptInterface->SetProperty(progressData, "progress", progress.progress, true);
	scriptInterface->SetProperty(progressData, "error", progress.error, true);

	return progressData;
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&StartGetGameId>(rq, "ModIoStartGetGameId");
	ScriptFunction::Register<&ModIo::StartListMods, &ModIoGetter>(rq, "ModIoStartListMods");
	ScriptFunction::Register<&ModIo::StartDownloadMod, &ModIoGetter>(rq, "ModIoStartDownloadMod");
	ScriptFunction::Register<&ModIo::AdvanceRequest, &ModIoGetter>(rq, "ModIoAdvanceRequest");
	ScriptFunction::Register<&ModIo::CancelRequest, &ModIoGetter>(rq, "ModIoCancelRequest");
	ScriptFunction::Register<&GetMods>(rq, "ModIoGetMods");
	ScriptFunction::Register<&GetDownloadProgress>(rq, "ModIoGetDownloadProgress");
}
}
