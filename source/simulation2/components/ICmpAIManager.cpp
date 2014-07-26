/* Copyright (C) 2012 Wildfire Games.
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

#include "lib/file/vfs/vfs_util.h"
#include "ps/Filesystem.h"

BEGIN_INTERFACE_WRAPPER(AIManager)
DEFINE_INTERFACE_METHOD_3("AddPlayer", void, ICmpAIManager, AddPlayer, std::wstring, player_id_t, uint8_t)
DEFINE_INTERFACE_METHOD_0("TryLoadSharedComponent", void, ICmpAIManager, TryLoadSharedComponent)
DEFINE_INTERFACE_METHOD_0("RunGamestateInit", void, ICmpAIManager, RunGamestateInit)
END_INTERFACE_WRAPPER(AIManager)

// Implement the static method that finds all AI scripts
// that can be loaded via AddPlayer:

struct GetAIsHelper
{
	NONCOPYABLE(GetAIsHelper);
public:
	GetAIsHelper(ScriptInterface& scriptInterface) :
		m_ScriptInterface(scriptInterface)
	{
	}

	void Run()
	{
		vfs::ForEachFile(g_VFS, L"simulation/ai/", Callback, (uintptr_t)this, L"*.json", vfs::DIR_RECURSIVE);
	}

	static Status Callback(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
	{
		GetAIsHelper* self = (GetAIsHelper*)cbData;
		JSContext* cx = self->m_ScriptInterface.GetContext();
		JSAutoRequest rq(cx);

		// Extract the 3rd component of the path (i.e. the directory after simulation/ai/)
		fs::wpath components = pathname.string();
		fs::wpath::iterator it = components.begin();
		std::advance(it, 2);
		std::wstring dirname = GetWstringFromWpath(*it);

		JS::RootedValue ai(cx);
		self->m_ScriptInterface.Eval("({})", &ai);
		self->m_ScriptInterface.SetProperty(ai, "id", dirname, true);
		self->m_ScriptInterface.SetProperty(ai, "data", self->m_ScriptInterface.ReadJSONFile(pathname), true);
		self->m_AIs.push_back(CScriptValRooted(cx, ai));

		return INFO::OK;
	}

	std::vector<CScriptValRooted> m_AIs;
	ScriptInterface& m_ScriptInterface;
};

std::vector<CScriptValRooted> ICmpAIManager::GetAIs(ScriptInterface& scriptInterface)
{
	GetAIsHelper helper(scriptInterface);
	helper.Run();
	return helper.m_AIs;
}
