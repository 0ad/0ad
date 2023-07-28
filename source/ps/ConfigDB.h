/* Copyright (C) 2022 Wildfire Games.
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

/*
	CConfigDB - Load, access and store configuration variables

	TDD		:	http://www.wildfiregames.com/forum/index.php?showtopic=1125
	OVERVIEW:

	JavaScript: Check this documentation: http://trac.wildfiregames.com/wiki/Exposed_ConfigDB_Functions
*/

#ifndef INCLUDED_CONFIGDB
#define INCLUDED_CONFIGDB

#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

/**
 * Namespace priorities:
 * - Command line args override everything
 * - User supersedes HWDetect (let the user try crashing his system).
 * - HWDetect supersedes mods & default -> mods can mod hwdetect itself.
 * - SYSTEM is used for local.cfg and is basically for setting custom defaults.
 */
enum EConfigNamespace
{
	CFG_DEFAULT,
	CFG_MOD,
	CFG_SYSTEM,
	CFG_HWDETECT,
	CFG_USER,
	CFG_COMMAND,
	CFG_LAST
};

using CConfigValueSet = std::vector<CStr>;

// Opaque data type so that callers that hook into ConfigDB can delete their hooks.
// Would be defined in CConfigDB but then it couldn't be forward-declared, which is rather annoying.
// Actually defined below - requires access to CConfigDB.
class CConfigDBHook;

#define g_ConfigDB (*CConfigDB::Instance())

class CConfigDB
{
	friend CConfigDBHook;
public:
	CConfigDB();
	~CConfigDB();
	CConfigDB(const CConfigDB&) = delete;
	CConfigDB(CConfigDB&&) = delete;

	static void Initialise();
	static void Shutdown();
	static bool IsInitialised();
	static CConfigDB* Instance();

	/**
	 * Attempt to retrieve the value of a config variable with the given name;
	 * will search CFG_COMMAND first, and then all namespaces from the specified
	 * namespace down.
	 */
	void GetValue(EConfigNamespace ns, const CStr& name, bool& value);
	///@copydoc CConfigDB::GetValue
	void GetValue(EConfigNamespace ns, const CStr& name, int& value);
	///@copydoc CConfigDB::GetValue
	void GetValue(EConfigNamespace ns, const CStr& name, u32& value);
	///@copydoc CConfigDB::GetValue
	void GetValue(EConfigNamespace ns, const CStr& name, float& value);
	///@copydoc CConfigDB::GetValue
	void GetValue(EConfigNamespace ns, const CStr& name, double& value);
	///@copydoc CConfigDB::GetValue
	void GetValue(EConfigNamespace ns, const CStr& name, std::string& value);

	/**
	 * Returns true if changed with respect to last write on file
	 */
	bool HasChanges(EConfigNamespace ns) const;

	void SetChanges(EConfigNamespace ns, bool value);

	/**
	 * Attempt to retrieve a vector of values corresponding to the given setting;
	 * will search CFG_COMMAND first, and then all namespaces from the specified
	 * namespace down.
	 */
	void GetValues(EConfigNamespace ns, const CStr& name, CConfigValueSet& values) const;

	/**
	 * Returns the namespace that the value returned by GetValues was defined in,
	 * or CFG_LAST if it wasn't defined at all.
	 */
	EConfigNamespace GetValueNamespace(EConfigNamespace ns, const CStr& name) const;

	/**
	 * Retrieve a map of values corresponding to settings whose names begin
	 * with the given prefix;
	 * will search all namespaces from default up to the specified namespace.
	 */
	std::map<CStr, CConfigValueSet> GetValuesWithPrefix(EConfigNamespace ns, const CStr& prefix) const;

	/**
	 * Save a config value in the specified namespace. If the config variable
	 * existed the value is replaced.
	 */
	void SetValueString(EConfigNamespace ns, const CStr& name, const CStr& value);

	void SetValueBool(EConfigNamespace ns, const CStr& name, const bool value);

	void SetValueList(EConfigNamespace ns, const CStr& name, std::vector<CStr> values);

	/**
	 * Remove a config value in the specified namespace.
	 */
	bool RemoveValue(EConfigNamespace ns, const CStr& name);

	/**
	 * Set the path to the config file used to populate the specified namespace
	 * Note that this function does not actually load the config file. Use
	 * the Reload() method if you want to read the config file at the same time.
	 *
	 * 'path': The path to the config file.
	 */
	void SetConfigFile(EConfigNamespace ns, const VfsPath& path);

	/**
	 * Reload the config file associated with the specified config namespace
	 * (the last config file path set with SetConfigFile)
	 *
	 * Returns:
	 *	true:	if the reload succeeded,
	 *	false:	if the reload failed
	 */
	bool Reload(EConfigNamespace);

	/**
	 * Write the current state of the specified config namespace to the file
	 * specified by 'path'
	 *
	 * Returns:
	 *	true:	if the config namespace was successfully written to the file
	 *	false:	if an error occurred
	 */
	bool WriteFile(EConfigNamespace ns, const VfsPath& path) const;

	/**
	 * Write the current state of the specified config namespace to the file
	 * it was originally loaded from.
	 *
	 * Returns:
	 *	true:	if the config namespace was successfully written to the file
	 *	false:	if an error occurred
	 */
	bool WriteFile(EConfigNamespace ns) const;

	/**
	 * Write a config value to the file specified by 'path'
	 *
	 * Returns:
	 *	true:	if the config value was successfully saved and written to the file
	 *	false:	if an error occurred
	 */
	bool WriteValueToFile(EConfigNamespace ns, const CStr& name, const CStr& value, const VfsPath& path);

	bool WriteValueToFile(EConfigNamespace ns, const CStr& name, const CStr& value);

	/**
	 * Register a simple lambda that will be called anytime the value changes in any namespace
	 * This is simple on purpose, the hook is responsible for checking if it should do something.
	 * When RegisterHookAndCall is called, the hook is immediately triggered.
	 * NB: CConfigDBHook will auto-unregister the hook when destroyed,
	 * so you can use it to tie the lifetime of the hook to your object.
	 * The hook will be deleted alongside ConfigDB anyways.
	 */
	[[nodiscard]] CConfigDBHook RegisterHookAndCall(const CStr& name, std::function<void()> hook);

	void UnregisterHook(CConfigDBHook&& hook);
	void UnregisterHook(std::unique_ptr<CConfigDBHook> hook);

private:
	std::array<std::map<CStr, CConfigValueSet>, CFG_LAST> m_Map;
	std::multimap<CStr, std::function<void()>> m_Hooks;
	std::array<VfsPath, CFG_LAST> m_ConfigFile;
	std::array<bool, CFG_LAST> m_HasChanges;

	mutable std::recursive_mutex m_Mutex;
};

class CConfigDBHook
{
	friend class CConfigDB;
public:
	CConfigDBHook() = delete;
	CConfigDBHook(const CConfigDBHook&) = delete;
	// Point the moved-from hook to end, which is checked for in UnregisterHook,
	// to avoid a double-erase error.
	CConfigDBHook(CConfigDBHook&& h) : m_ConfigDB(h.m_ConfigDB)
	{
		m_Ptr = std::move(h.m_Ptr);
		h.m_Ptr = m_ConfigDB.m_Hooks.end();
	}
	// Unregisters the hook. Must be called before the original ConfigDB gets deleted.
	~CConfigDBHook()
	{
		m_ConfigDB.UnregisterHook(std::move(*this));
	}
private:
	CConfigDBHook(CConfigDB& cdb, std::multimap<CStr, std::function<void()>>::iterator p)
		: m_ConfigDB(cdb), m_Ptr(p)
	{};

	std::multimap<CStr, std::function<void()>>::iterator m_Ptr;
	CConfigDB& m_ConfigDB;
};


// stores the value of the given key into <destination>. this quasi-template
// convenience wrapper on top of GetValue simplifies user code
#define CFG_GET_VAL(name, destination)\
	g_ConfigDB.GetValue(CFG_USER, name, destination)

#endif // INCLUDED_CONFIGDB
