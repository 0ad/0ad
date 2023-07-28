/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_UserReport.h"

#include "ps/Filesystem.h"
#include "ps/Pyrogenesis.h"
#include "ps/UserReport.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptRequest.h"

#include <string>

namespace JSI_UserReport
{
bool IsUserReportEnabled()
{
	return g_UserReporter.IsReportingEnabled();
}

void SetUserReportEnabled(bool enabled)
{
	g_UserReporter.SetReportingEnabled(enabled);
}

std::string GetUserReportStatus()
{
	return g_UserReporter.GetStatus();
}

std::string GetUserReportLogPath()
{
	return psLogDir().string8();
}

std::string GetUserReportConfigPath()
{
	OsPath configPath;
	WARN_IF_ERR(g_VFS->GetDirectoryRealPath("config/", configPath));
	return configPath.string8();
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&IsUserReportEnabled>(rq, "IsUserReportEnabled");
	ScriptFunction::Register<&SetUserReportEnabled>(rq, "SetUserReportEnabled");
	ScriptFunction::Register<&GetUserReportStatus>(rq, "GetUserReportStatus");
	ScriptFunction::Register<&GetUserReportLogPath>(rq, "GetUserReportLogPath");
	ScriptFunction::Register<&GetUserReportConfigPath>(rq, "GetUserReportConfigPath");
}
}
