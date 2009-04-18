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

/*
	CConfigDB - Load, access and store configuration variables
	
	TDD		:	http://forums.wildfiregames.com/0ad/index.php?showtopic=1125
	OVERVIEW:

	JavaScript:
		All the javascript interfaces are provided through the global object
		g_ConfigDB.

		g_ConfigDB Properties:
		system:
			All CFG_SYSTEM values are linked to properties of this object.
			a=g_ConfigDB.system.foo; is equivalent to C++ code
			g_ConfigDB.GetValue(CFG_SYSTEM, "foo");
		mod: Ditto, but linked to CFG_MOD
		user: Ditto, but linked to CFG_USER

		g_ConfigDB Functions: None so far

		ConfigNamespace Functions: (applicable to the system, mod or user
			properties of g_ConfigDB)

		boolean WriteFile(boolean useVFS, string path):
			JS interface to g_ConfigDB.WriteFile - should work exactly the
			same.

		boolean Reload() => g_ConfigDB.Reload()
		void SetFile() => g_ConfigDB.SetConfigFile()
*/

#ifndef INCLUDED_CONFIGDB
#define INCLUDED_CONFIGDB

#include "Pyrogenesis.h"
#include "Parser.h"
#include "CStr.h"
#include "Singleton.h"

// Namespace priorities: User supersedes mod supersedes system.
//						 Command-line arguments override everything.

enum EConfigNamespace
{
	CFG_SYSTEM,
	CFG_MOD,
	CFG_USER,
	CFG_COMMAND,
	CFG_LAST
};

typedef CParserValue CConfigValue;
typedef std::vector<CParserValue> CConfigValueSet;

#define g_ConfigDB CConfigDB::GetSingleton()

class CConfigDB: public Singleton<CConfigDB>
{
	static std::map <CStr, CConfigValueSet> m_Map[];
	static CStr m_ConfigFile[];
	static bool m_UseVFS[];

public:
	// NOTE: Construct the Singleton Object *after* JavaScript init, so that
	// the JS interface can be registered.
	CConfigDB();

	// GetValue()
	// Attempt to find a config variable with the given name; will search all
	// namespaces from system up to the specified namespace.
	//
	// Returns a pointer to the config value structure for the variable, or
	// NULL if such a variable could not be found
	CConfigValue *GetValue(EConfigNamespace ns, const CStr& name);
	
	// GetValues()
	// Attempt to retrieve a vector of values corresponding to the given setting;
	// will search all namespaces from system up to the specified namespace.
	// 
	// Returns a pointer to the vector, or NULL if the setting could not be found.
	CConfigValueSet *GetValues(EConfigNamespace ns, const CStr& name);

	// CreateValue()
	// Create a new config value in the specified namespace. If such a
	// variable already exists, the old value is returned and the effect is
	// exactly the same as that of GetValue()
	//
	// Returns a pointer to the value of the newly created config variable, or
	// that of the already existing config variable.
	CConfigValue *CreateValue(EConfigNamespace ns, const CStr& name);
	
	// SetConfigFile()
	// Set the path to the config file used to populate the specified namespace
	// Note that this function does not actually load the config file. Use
	// the Reload() method if you want to read the config file at the same time.
	//
	// 'path': The path to the config file.
	//		VFS: relative to VFS root
	//		non-VFS: relative to current working directory (binaries/data/)
	// 'useVFS': true if the path is a VFS path, false if it is a real path
	void SetConfigFile(EConfigNamespace ns, bool useVFS, const CStr& path);
	
	// Reload()
	// Reload the config file associated with the specified config namespace
	// (the last config file path set with SetConfigFile)
	//
	// Returns:
	//	true:	if the reload succeeded, 
	//	false:	if the reload failed
	bool Reload(EConfigNamespace);
	
	// WriteFile()
	// Write the current state of the specified config namespace to the file
	// specified by 'path'
	//
	// Returns:
	//	true:	if the config namespace was successfully written to the file
	//	false:	if an error occured
	bool WriteFile(EConfigNamespace ns, bool useVFS, const CStr& path);
};


// stores the value of the given key into <destination>. this quasi-template
// convenience wrapper on top of CConfigValue::Get* simplifies user code and
// avoids "assignment within condition expression" warnings.
#define CFG_GET_SYS_VAL(name, type, destination)\
STMT(\
	CConfigValue* val = g_ConfigDB.GetValue(CFG_SYSTEM, name);\
	if(val)\
		val->Get##type(destination);\
)
#define CFG_GET_USER_VAL(name, type, destination)\
STMT(\
	CConfigValue* val = g_ConfigDB.GetValue(CFG_USER, name);\
	if(val)\
		val->Get##type(destination);\
)


#endif
