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

#include "JSInterface_UserReport.h"

#include "ps/Filesystem.h"
#include "ps/Pyrogenesis.h"
#include "ps/UserReport.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>

bool JSI_UserReport::IsUserReportEnabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_UserReporter.IsReportingEnabled();
}

void JSI_UserReport::SetUserReportEnabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool enabled)
{
	g_UserReporter.SetReportingEnabled(enabled);
}

std::string JSI_UserReport::GetUserReportStatus(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_UserReporter.GetStatus();
}

std::string JSI_UserReport::GetUserReportLogPath(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return psLogDir().string8();
}

std::string JSI_UserReport::GetUserReportConfigPath(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	OsPath configPath;
	WARN_IF_ERR(g_VFS->GetDirectoryRealPath("config/", configPath));
	return configPath.string8();
}

void JSI_UserReport::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<bool, &IsUserReportEnabled>("IsUserReportEnabled");
	scriptInterface.RegisterFunction<void, bool, &SetUserReportEnabled>("SetUserReportEnabled");
	scriptInterface.RegisterFunction<std::string, &GetUserReportStatus>("GetUserReportStatus");
	scriptInterface.RegisterFunction<std::string, &GetUserReportLogPath>("GetUserReportLogPath");
	scriptInterface.RegisterFunction<std::string, &GetUserReportConfigPath>("GetUserReportConfigPath");
}
