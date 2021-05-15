/* Copyright (C) 2021 Wildfire Games.
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

#include "ICmpAIManager.h"

#include "simulation2/system/InterfaceScripted.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/JSON.h"

#include "lib/file/vfs/vfs_util.h"
#include "ps/Filesystem.h"

#include <boost/filesystem.hpp>

BEGIN_INTERFACE_WRAPPER(AIManager)
DEFINE_INTERFACE_METHOD("AddPlayer", ICmpAIManager, AddPlayer)
DEFINE_INTERFACE_METHOD("TryLoadSharedComponent", ICmpAIManager, TryLoadSharedComponent)
DEFINE_INTERFACE_METHOD("RunGamestateInit", ICmpAIManager, RunGamestateInit)
END_INTERFACE_WRAPPER(AIManager)

// Implement the static method that finds all AI scripts
// that can be loaded via AddPlayer:

struct GetAIsHelper
{
	NONCOPYABLE(GetAIsHelper);
public:
	GetAIsHelper(const ScriptInterface& scriptInterface) :
		m_ScriptInterface(scriptInterface),
		m_AIs(scriptInterface.GetGeneralJSContext())
	{
		ScriptRequest rq(m_ScriptInterface);
		m_AIs = JS::NewArrayObject(rq.cx, 0);
	}

	void Run()
	{
		vfs::ForEachFile(g_VFS, L"simulation/ai/", Callback, (uintptr_t)this, L"*.json", vfs::DIR_RECURSIVE);
	}

	static Status Callback(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
	{
		GetAIsHelper* self = (GetAIsHelper*)cbData;
		ScriptRequest rq(self->m_ScriptInterface);

		// Extract the 3rd component of the path (i.e. the directory after simulation/ai/)
		fs::wpath components = pathname.string();
		fs::wpath::iterator it = components.begin();
		std::advance(it, 2);
		std::wstring dirname = GetWstringFromWpath(*it);

		JS::RootedValue ai(rq.cx);
		Script::CreateObject(rq, &ai);

		JS::RootedValue data(rq.cx);
		Script::ReadJSONFile(rq, pathname, &data);
		Script::SetProperty(rq, ai, "id", dirname, true);
		Script::SetProperty(rq, ai, "data", data, true);
		u32 length;
		JS::GetArrayLength(rq.cx, self->m_AIs, &length);
		JS_SetElement(rq.cx, self->m_AIs, length, ai);

		return INFO::OK;
	}

	JS::PersistentRootedObject m_AIs;
	const ScriptInterface& m_ScriptInterface;
};

JS::Value ICmpAIManager::GetAIs(const ScriptInterface& scriptInterface)
{
	GetAIsHelper helper(scriptInterface);
	helper.Run();
	return JS::ObjectValue(*helper.m_AIs);
}
