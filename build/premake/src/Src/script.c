/**********************************************************************
 * Premake - script.c
 * Interface to the Lua scripting engine.
 *
 * Copyright (c) 2002-2005 Jason Perkins and the Premake project
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the file LICENSE.txt for details.
 **********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "script.h"
#include "arg.h"
#include "os.h"
#include "Lua/lua.h"
#include "Lua/lualib.h"
#include "Lua/lauxlib.h"
#include "Lua/ldebug.h"

static lua_State*  L;
static const char* currentScript = NULL;


static int         tbl_get(int from, const char* name);
static int         tbl_geti(int from, int i);
static int         tbl_getlen(int tbl);
static int         tbl_getlen_deep(int tbl);
static const char* tbl_getstring(int from, const char* name);
static const char* tbl_getstringi(int from, int i);

static int         lf_addoption(lua_State* L);
static int         lf_alert(lua_State* L);
static int         lf_appendfile(lua_State* L);
static int         lf_chdir(lua_State* L);
static int         lf_copyfile(lua_State* L);
static int         lf_docommand(lua_State* L);
static int         lf_dopackage(lua_State* L);
static int         lf_fileexists(lua_State* L);
static int         lf_findlib(lua_State* L);
static int         lf_getbasename(lua_State* L);
static int         lf_getconfigs(lua_State* L);
static int         lf_getcwd(lua_State* L);
static int         lf_getdir(lua_State* L);
static int         lf_getextension(lua_State* L);
static int         lf_getglobal(lua_State* L);
static int         lf_getname(lua_State* L);
static int         lf_matchfiles(lua_State* L);
static int         lf_matchrecursive(lua_State* L);
static int         lf_newfileconfig(lua_State* L);
static int         lf_newpackage(lua_State* L);
static int         lf_panic(lua_State* L);
static int         lf_rmdir(lua_State* L);
static int         lf_setconfigs(lua_State* L);

static void        buildOptionsTable();
static void        buildNewProject();


/**********************************************************************
 * Initialize the Lua environment
 **********************************************************************/

int script_init()
{
	/* Create a script environment and install the standard libraries */
	L = lua_open();
	luaopen_base(L);
	luaopen_table(L);
	luaopen_io(L);
	luaopen_string(L);
	luaopen_math(L);
	luaopen_loadlib(L);

	lua_atpanic(L, lf_panic);

	/* Register my extensions to the Lua environment */
	lua_register(L, "addoption",  lf_addoption);
	lua_register(L, "_ALERT",     lf_alert);
	lua_register(L, "copyfile",   lf_copyfile);
	lua_register(L, "docommand",  lf_docommand);
	lua_register(L, "dopackage",  lf_dopackage);
	lua_register(L, "fileexists", lf_fileexists);
	lua_register(L, "findlib",    lf_findlib);
	lua_register(L, "matchfiles", lf_matchfiles);
	lua_register(L, "matchrecursive", lf_matchrecursive);
	lua_register(L, "newpackage", lf_newpackage);

	/* Add some extensions to the built-in "os" table */
	lua_getglobal(L, "os");

	lua_pushstring(L, "appendfile");
	lua_pushcfunction(L, lf_appendfile);
	lua_settable(L, -3);

	lua_pushstring(L, "chdir");
	lua_pushcfunction(L, lf_chdir);
	lua_settable(L, -3);

	lua_pushstring(L, "copyfile");
	lua_pushcfunction(L, lf_copyfile);
	lua_settable(L, -3);

	lua_pushstring(L, "fileexists");
	lua_pushcfunction(L, lf_fileexists);
	lua_settable(L, -3);

	lua_pushstring(L, "findlib");
	lua_pushcfunction(L, lf_findlib);
	lua_settable(L, -3);

	lua_pushstring(L, "getcwd");
	lua_pushcfunction(L, lf_getcwd);
	lua_settable(L, -3);

	lua_pushstring(L, "rmdir");
	lua_pushcfunction(L, lf_rmdir);
	lua_settable(L, -3);

	lua_pop(L, 1);

	/* Add path handling functions */
	lua_newtable(L);

	lua_pushstring(L, "getbasename");
	lua_pushcfunction(L, lf_getbasename);
	lua_settable(L, -3);

	lua_pushstring(L, "getdir");
	lua_pushcfunction(L, lf_getdir);
	lua_settable(L, -3);

	lua_pushstring(L, "getextension");
	lua_pushcfunction(L, lf_getextension);
	lua_settable(L, -3);

	lua_pushstring(L, "getname");
	lua_pushcfunction(L, lf_getname);
	lua_settable(L, -3);

	lua_setglobal(L, "path");

	/* Register some commonly used Lua4 functions */
	lua_register(L, "rmdir", lf_rmdir);

	lua_getglobal(L, "table");
	lua_pushstring(L, "insert");
	lua_gettable(L, -2);
	lua_setglobal(L, "tinsert");
	lua_pop(L, 1);

	lua_getglobal(L, "os");
	lua_pushstring(L, "remove");
	lua_gettable(L, -2);
	lua_setglobal(L, "remove");
	lua_pop(L, 1);

	/* Set the global OS identifiers */
	lua_pushstring(L, os_get());
	lua_setglobal(L, "OS");

	lua_pushnumber(L, 1);
	lua_setglobal(L, os_get());

	/* Create a list of option descriptions for addoption() */
	lua_getregistry(L);
	lua_pushstring(L, "options");
	lua_newtable(L);
	lua_settable(L, -3);
	lua_pop(L, 1);

	/* Create and populate a global "options" table */
	buildOptionsTable();

	/* Create an empty list of packages */
	lua_getregistry(L);
	lua_pushstring(L, "packages");
	lua_newtable(L);
	lua_settable(L, -3);
	lua_pop(L, 1);

	/* Create a default project object */
	buildNewProject();

	/* Set hook to intercept creation of globals, used to create packages */
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_newtable(L);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, lf_getglobal);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_pop(L, 1);

	return 1;
}


/**********************************************************************
 * Execute the specified premake script. Contains some logic for
 * locating the script file if an exact match isn't found
 **********************************************************************/

int script_run(const char* filename)
{
	char scriptname[8192];
	int result;

	strcpy(scriptname, filename);
	if (!io_fileexists(scriptname))
	{
		strcat(scriptname, ".lua");
	}
	if (!io_fileexists(scriptname))
	{
		return 0;
	}

	currentScript = scriptname;
	if (!script_init())
		return -1;

	result = lua_dofile(L, scriptname);
	return (result == 0) ? 1 : -1;
}


/**********************************************************************
 * After the script has run, these functions pull the project data
 * out into local objects
 **********************************************************************/

static int export_list(int parent, int object, const char* name, const char*** list)
{
	int i;

	int parArr = tbl_get(parent, name);
	int parLen = tbl_getlen_deep(parArr);
	int objArr = tbl_get(object, name);
	int objLen = tbl_getlen_deep(objArr);

	*list = (const char**)prj_newlist(parLen + objLen);

	for (i = 0; i < parLen; ++i)
		(*list)[i] = tbl_getstringi(parArr, i + 1);

	for (i = 0; i < objLen; ++i)
		(*list)[parLen + i] = tbl_getstringi(objArr, i + 1);

	return (parLen + objLen);
}

static const char* export_value(int parent, int object, const char* name)
{
	const char* value;
	value = tbl_getstring(object, name);
	if (value == NULL)
		value = tbl_getstring(parent, name);
	return value;
}

static const char** export_files(int tbl, int obj)
{
	const char** files;
	const char** excludes;
	const char** result;
	int numFiles, numExcludes;
	int i, j, k;

	export_list(tbl, obj, "files", &files);
	export_list(tbl, obj, "excludes", &excludes);

	numFiles = prj_getlistsize((void**)files);
	numExcludes = prj_getlistsize((void**)excludes);

	result = (const char**)prj_newlist(numFiles);

	k = 0;
	for (i = 0; i < numFiles; ++i)
	{
		int exclude = 0;
		for (j = 0; j < numExcludes; ++j)
		{
			if (matches(files[i], excludes[j]))
				exclude = 1;
		}

		if (!exclude)
			result[k++] = files[i];
	}

	free((void*)files);
	free((void*)excludes);

	result[k] = NULL;
	return result;
}

static int export_fileconfig(PkgConfig* config, int arr)
{
	int obj, count, i;

	count = prj_getlistsize((void**)config->files);
	config->fileconfigs = (FileConfig**)prj_newlist(count);
	for (i = 0; i < count; ++i)
	{
		FileConfig* fconfig = ALLOCT(FileConfig);
		config->fileconfigs[i] = fconfig;

		obj = tbl_get(arr, config->files[i]);
		if (obj > 0)
		{
			fconfig->buildaction = tbl_getstring(obj, "buildaction");
		}
		else
		{
			fconfig->buildaction = NULL;
		}
	}

	return 1;
}

static int export_pkgconfig(Package* package, int tbl)
{
	int arr, obj;
	int len, i;

	arr = tbl_get(tbl, "config");
	len = tbl_getlen(arr);
	package->configs = (PkgConfig**)prj_newlist(len);
	for (i = 0; i < len; ++i)
	{
		PkgConfig* config = ALLOCT(PkgConfig);
		package->configs[i] = config;
		config->prjConfig = project->configs[i];

		obj = tbl_geti(arr, i + 1);
		config->objdir = tbl_getstring(obj, "objdir");

		config->extension  = export_value(tbl, obj, "targetextension");
		config->prefix     = export_value(tbl, obj, "targetprefix");
		config->target     = export_value(tbl, obj, "target");
		config->kind       = export_value(tbl, obj, "kind");
		config->pchHeader  = export_value(tbl, obj, "pchheader");
		config->pchSource  = export_value(tbl, obj, "pchsource");
		config->trimprefix = export_value(tbl, obj, "trimprefix");
		config->cxxtest_options     = export_value(tbl, obj, "testoptions");
		config->cxxtest_rootoptions = export_value(tbl, obj, "rootoptions");
		config->cxxtest_rootfile    = export_value(tbl, obj, "rootfile");

		/* Assign a default target, if none specified */
		if (config->target == NULL)
			config->target = package->name;

		/* Pull out the value lists */
		export_list(tbl, obj, "buildflags",   &config->flags);
		export_list(tbl, obj, "buildoptions", &config->buildopts);
		export_list(tbl, obj, "defines",      &config->defines);
		export_list(tbl, obj, "includepaths", &config->incpaths);
		export_list(tbl, obj, "libpaths",     &config->libpaths);
		export_list(tbl, obj, "linkoptions",  &config->linkopts);
		export_list(tbl, obj, "links",        &config->links);

		/* Build the file list */
		config->files = export_files(tbl, obj);

		/* Build a list of file configurations */
		export_fileconfig(config, arr);
	}

	return 1;
}


int script_export()
{
	int tbl, arr, obj;
	int len, i;

	prj_open();

	/* Copy out the list of available options */
	tbl = tbl_get(LUA_REGISTRYINDEX, "options");
	len = tbl_getlen(tbl);
	project->options = (Option**)prj_newlist(len);
	for (i = 0; i < len; ++i)
	{
		Option* option = ALLOCT(Option);
		project->options[i] = option;

		obj = tbl_geti(tbl, i + 1);
		option->flag = tbl_getstringi(obj, 1);
		option->desc = tbl_getstringi(obj, 2);
	}

	/* Copy out the project settings */
	tbl = tbl_get(LUA_GLOBALSINDEX, "project");
	project->name = tbl_getstring(tbl, "name");
	project->path = tbl_getstring(tbl, "path");
	project->script = tbl_getstring(tbl, "script");

	/* Copy out the project configurations */
	arr = tbl_get(tbl, "config");
	len = tbl_getlen(arr);
	project->configs = (PrjConfig**)prj_newlist(len);
	for (i = 0; i < len; ++i)
	{
		PrjConfig* config = ALLOCT(PrjConfig);
		project->configs[i] = config;

		obj = tbl_geti(arr, i + 1);
		config->name     = tbl_getstring(obj, "name");
		config->bindir   = export_value(tbl, obj, "bindir");
		config->libdir   = export_value(tbl, obj, "libdir");
		config->nasmpath = export_value(tbl, obj, "nasmpath");
		config->nasm_format = export_value(tbl, obj, "nasmformat");
		config->cxxtest_path = export_value(tbl, obj, "cxxtestpath");
	}

	/* Copy out the packages */
	tbl = tbl_get(LUA_REGISTRYINDEX, "packages");
	len = tbl_getlen(tbl);
	project->packages = (Package**)prj_newlist(len);
	for (i = 0; i < len; ++i)
	{
		Package* package = ALLOCT(Package);
		package->index = i;
		project->packages[i] = package;
		
		obj = tbl_geti(tbl, i + 1);
		package->name   = tbl_getstring(obj, "name");
		package->path   = tbl_getstring(obj, "path");
		package->script = tbl_getstring(obj, "script");
		package->lang   = tbl_getstring(obj, "language");
		package->kind   = tbl_getstring(obj, "kind");
		package->objdir = tbl_getstring(obj, "objdir");
		package->url    = tbl_getstring(obj, "url");
		package->data   = NULL;

		export_pkgconfig(package, obj);
	}

	return 1;
}


/**********************************************************************
 * Callback for commands pulled from the program arguments
 **********************************************************************/

int script_docommand(const char* cmd)
{
	char buffer[512];
	const char* arg;

	/* Trim off the leading '--' */
	if (strncmp(cmd, "--", 2) == 0)
		cmd += 2;

	/* Look for a handler */
	strcpy(buffer, "do");
	strcat(buffer, cmd);
	lua_getglobal(L, buffer);
	if (!lua_isfunction(L, -1))
	{
		/* Fall back to the default handler */
		lua_getglobal(L, "docommand");
		if (!lua_isfunction(L, -1))
		{
			lua_pop(L, 1);
			return 0;
		}
	}

	/* Push the command and arguments onto the stack */
	lua_pushstring(L, cmd);
	arg = arg_getflagarg();
	if (arg != NULL)
		lua_pushstring(L, arg);
	else
		lua_pushnil(L);

	lua_call(L, 2, 0);
	return 1;
}


int script_close()
{
	lua_close(L);
	return 1;
}



/**********************************************************************
 * These function assist with setup of the script environment
 **********************************************************************/

static void buildOptionsTable()
{
	const char* flag;
	const char* arg;

	lua_newtable(L);

	arg_reset();
	flag = arg_getflag();
	while (flag != NULL)
	{
		if (strncmp(flag, "--", 2) == 0)
			flag += 2;

		lua_pushstring(L, flag);

		/* If the flag has an argument, push that too */
		arg = arg_getflagarg();
		if (arg != NULL)
			lua_pushstring(L, arg);
		else
			lua_pushboolean(L, 1);

		lua_settable(L, -3);
		flag = arg_getflag();
	}

	lua_setglobal(L, "options");
}

static void buildNewProject()
{
	lua_newtable(L);

	lua_pushstring(L, "name");
	lua_pushstring(L, "MyProject");
	lua_settable(L, -3);

	lua_pushstring(L, "path");
	lua_pushstring(L, path_getdir(currentScript));
	lua_settable(L, -3);

	lua_pushstring(L, "bindir");
	lua_pushstring(L, "");
	lua_settable(L, -3);

	lua_pushstring(L, "libdir");
	lua_pushstring(L, "");
	lua_settable(L, -3);

	/* Hook "index" metamethod so I can tell when the config list changes */
	lua_newtable(L);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, lf_setconfigs);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, lf_getconfigs);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	/* Set default values */
	lua_pushstring(L, "script");
	lua_pushstring(L, path_getname(currentScript));
	lua_settable(L, -3);

	lua_pushstring(L, "configs");
	lua_newtable(L);
	lua_pushstring(L, "Debug");
	lua_rawseti(L, -2, 1);
	lua_pushstring(L, "Release");
	lua_rawseti(L, -2, 2);
	lua_settable(L, -3);

	lua_setglobal(L, "project");
}

static void buildNewConfig(const char* name)
{
	/* Store the config name */
	if (name != NULL)
	{
		lua_pushstring(L, "name");
		lua_pushstring(L, name);
		lua_settable(L, -3);
	}

	/* Set defaults */
	lua_pushstring(L, "buildflags");
	lua_newtable(L);
	if (matches(name, "Release")) 
	{
		lua_pushstring(L, "no-symbols");
		lua_rawseti(L, -2, 1);
		lua_pushstring(L, "optimize");
		lua_rawseti(L, -2, 2);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "buildoptions");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "defines");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "files");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "excludes");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "includepaths");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "libpaths");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "linkoptions");
	lua_newtable(L);
	lua_settable(L, -3);

	lua_pushstring(L, "links");
	lua_newtable(L);
	lua_settable(L, -3);
}



/**********************************************************************
 * These function help get data out of the Lua tables
 **********************************************************************/

static int tbl_matchkey(const char* name, int index)
{
	int result, i;

	if (lua_isnumber(L, index))
		return 0;

	if (!lua_istable(L, index))
	{
		const char* key = lua_tostring(L, index);
		return matches(key, name);
	}

	/* If key is a table, scan for value */
	for (i = 1; i <= luaL_getn(L, index); ++i)
	{
		lua_rawgeti(L, index, i);
		result = tbl_matchkey(name, -1);
		lua_pop(L, 1);
		if (result)
			return 1;
	}

	return 0;
}

static int tbl_get(int from, const char* name)
{
	int ref;

	/* Retrieve the `from` object */
	if (from == LUA_REGISTRYINDEX || from == LUA_GLOBALSINDEX)
	{
		lua_pushvalue(L, from);
	}
	else
	{
		lua_getref(L, from);
	}

	/* Do a deep key search for the requested object */
	lua_pushnil(L);
	while (lua_next(L, -2))
	{
		if (tbl_matchkey(name, -2))
		{
			/* Validate result */
			if (!lua_istable(L, -1))
			{
				char msg[512];
				sprintf(msg, "'%s' should be a table.\nPlace value between brackets like: { values }", name);
				lua_pushstring(L, msg);
				lua_error(L);
			}

			/* Reference and return */
			ref = lua_ref(L, -1);
			lua_pop(L, 2);
			return ref;
		}

		lua_pop(L, 1);
	}

	/* Not found */
	lua_pop(L, 1);
	return 0;

/*
	lua_pushstring(L, name);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1))
		return 0;

	if (!lua_istable(L, -1))
	{
		char msg[512];
		sprintf(msg, "'%s' should be a table.\nPlace value between brackets like: { values }", name);
		lua_pushstring(L, msg);
		lua_error(L);
	}

	ref = lua_ref(L, -1);
	lua_pop(L, 1);

	return ref;
*/
}


static int tbl_geti(int from, int i)
{
	int ref;

	lua_getref(L, from);
	lua_rawgeti(L, -1, i);

	ref = lua_ref(L, -1);
	lua_pop(L, 1);
	return ref;
}


static int tbl_getlen(int tbl)
{
	int size;
	lua_getref(L, tbl);
	size = luaL_getn(L, -1);
	lua_pop(L, 1);
	return size;
}


static int tbl_getlen_deep(int tbl)
{
	int size, i;
	lua_getref(L, tbl);

	size = 0;
	for (i = 1; i <= luaL_getn(L, -1); ++i)
	{
		lua_rawgeti(L, -1, i);
		if (lua_istable(L, -1))
		{
			size += tbl_getlen_deep(lua_ref(L, -1));
		}
		else
		{
			lua_pop(L, 1);
			size++;
		}
	}

	lua_pop(L, 1);
	return size;
}


static const char* tbl_getstring(int from, const char* name)
{
	const char* str;
	
	lua_getref(L, from);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	str = lua_tostring(L, -1);
	lua_pop(L, 2);

	return str;
}


static const char* tbl_getstringi_worker(int arr, int* index)
{
	int i;
	const char* result = NULL;

	lua_getref(L, arr);
	for (i = 1; i <= luaL_getn(L, -1) && *index > 0; ++i)
	{
		lua_rawgeti(L, -1, i);
		if (lua_istable(L, -1))
		{
			int ref = lua_ref(L, -1);
			result = tbl_getstringi_worker(ref, index);
		}
		else if (*index == 1)
		{
			result = lua_tostring(L, -1);
			lua_pop(L, 1);
			(*index)--;
		}
		else
		{
			lua_pop(L, 1);
			(*index)--;
		}
	}

	lua_pop(L, 1);
	return result;
}

static const char* tbl_getstringi(int from, int i)
{
	int index = i;
	const char* result = tbl_getstringi_worker(from, &index);
	return result;
}


/**********************************************************************
 * These are new functions for the Lua environment
 **********************************************************************/

static int lf_addoption(lua_State* L)
{
	const char* name = luaL_checkstring(L, 1);
	const char* desc = luaL_checkstring(L, 2);

	/* Retrieve the options list from the registry */
	lua_getregistry(L);
	lua_pushstring(L, "options");
	lua_gettable(L, -2);

	/* Create a new table for this new option */
	lua_newtable(L);
	lua_pushstring(L, name);
	lua_rawseti(L, -2, 1);
	lua_pushstring(L, desc);
	lua_rawseti(L, -2, 2);

	/* Add the option to the end of the registry list */
	lua_rawseti(L, -2, luaL_getn(L, -2) + 1);

	lua_pop(L, 2);
	return 0;
}



static int lf_alert(lua_State* L)
{
	/* Get the error message */
	const char* msg = lua_tostring(L, -1);

	/* Swap out the file name so I can see the whole path */
	msg = strchr(msg, ':');

	printf("%s%s\n", currentScript, msg);
	exit(1);
}


static int lf_appendfile(lua_State* L)
{
	FILE* fSrc;
	FILE* fDst;
	int count;

	const char* src = luaL_checkstring(L, 1);
	const char* dst = luaL_checkstring(L, 2);

	fSrc = fopen(src, "rb");
	if (fSrc == NULL)
		luaL_error(L, "Unable to open file for reading '%s'\n", src);

	fDst = fopen(dst, "ab");
	if (fDst == NULL)
		luaL_error(L, "Unable to open file for appending '%s'\n", dst);

	count = fread(g_buffer, 1, 8192, fSrc);
	while (count > 0)
	{
		fwrite(g_buffer, 1, count, fDst);
		count = fread(g_buffer, 1, 8192, fSrc);
	}

	fclose(fSrc);
	fclose(fDst);
	return 0;
}


static int lf_docommand(lua_State* L)
{
	const char* cmd = luaL_checkstring(L, 1);
	const char* arg = (!lua_isnil(L,2)) ? luaL_checkstring(L, 2) : NULL;
	if (!onCommand(cmd, arg))
		exit(1);
	return 0;
}


static int lf_chdir(lua_State* L)
{
	const char* path = luaL_checkstring(L, 2);
	if (io_chdir(path))
		lua_pushnumber(L, 1);
	else
		lua_pushnil(L);
	return 1;
}


static int lf_copyfile(lua_State* L)
{
	const char* src  = luaL_checkstring(L, 1);
	const char* dest = luaL_checkstring(L, 2);
	if (io_copyfile(src, dest))
		lua_pushnumber(L, 1);
	else
		lua_pushnil(L);
	return 1;
}


static int lf_dopackage(lua_State* L)
{
	const char* oldScript;
	char oldcwd[8192];
	char filename[8192];
	int result;

	/* Clear the current global so included script can create a new one */
	lua_pushnil(L);
	lua_setglobal(L, "package");

	/* Remember the current state of things so I can restore after script runs */
	oldScript = currentScript;
	strcpy(oldcwd, io_getcwd());

	/* Try to locate the script file */
	strcpy(filename, lua_tostring(L, 1));
	if (!io_fileexists(filename))
	{
		strcpy(filename, path_join("", lua_tostring(L, 1), "lua"));
	}
	if (!io_fileexists(filename))
	{
		strcpy(filename, path_join(lua_tostring(L, 1), "premake.lua", ""));
	}

	if (!io_fileexists(filename))
	{
		lua_pushstring(L, "Unable to open package '");
		lua_pushvalue(L, 1);
		lua_pushstring(L, "'");
		lua_concat(L, 3);
		lua_error(L);
	}

	currentScript = filename;
	io_chdir(path_getdir(filename));

	result = lua_dofile(L, path_getname(filename));
	
	/* Restore the previous state */
	currentScript = oldScript;
	io_chdir(oldcwd);
	
	return 0;
}


static int lf_fileexists(lua_State* L)
{
	const char* path = luaL_check_string(L, 1);
	int result = io_fileexists(path);
	lua_pushboolean(L, result);
	return 1;
}


static int lf_findlib(lua_State* L)
{
	const char* libname = luaL_check_string(L, 1);
	const char* result = io_findlib(libname);
	if (result)
		lua_pushstring(L, result);
	else
		lua_pushnil(L);
	return 1;
}


static int lf_getbasename(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const char* result = path_getbasename(path);
	lua_pushstring(L, result);
	return 1;
}


static int lf_getcwd(lua_State* L)
{
	const char* cwd = io_getcwd();
	lua_pushstring(L, cwd);
	return 1;
}


static int lf_getdir(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const char* result = path_getdir(path);
	lua_pushstring(L, result);
	return 1;
}


static int lf_getextension(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const char* result = path_getextension(path);
	lua_pushstring(L, result);
	return 1;
}


static int lf_getglobal(lua_State* L)
{
	const char* name = luaL_checkstring(L, 2);
	if (matches(name, "package"))
	{
		lf_newpackage(L);
		lua_pushvalue(L, -1);
		lua_setglobal(L, "package");
		return 1;
	}
	return 0;
}


static int lf_getname(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const char* result = path_getname(path);
	lua_pushstring(L, result);
	return 1;
}


static int debugging = 0;

static void doFileScan(lua_State* L, char* path, int recursive)
{
	MaskHandle handle;

	if (debugging) puts(path);
	
	handle = io_mask_open(path);
	while (io_mask_getnext(handle))
	{
		if (io_mask_isfile(handle))
		{
			const char* name = io_mask_getname(handle);
			lua_pushstring(L, name);
			lua_rawseti(L, -2, luaL_getn(L, -2) + 1);
		}
	}
	io_mask_close(handle);

	/* If recursive, scan subdirectories */
	if (recursive)
	{
		char mask[128];
		int len;
		
		/* Split the mask from the path */
		strcpy(mask, path_getname(path));
		strcpy(path, path_getdir(path));
		
		/* Scan for subdirs */
		len = strlen(path);
		handle = io_mask_open(path_combine(path, "*"));
		while (io_mask_getnext(handle))
		{
			if (!io_mask_isfile(handle))
			{
				const char* name = io_mask_getname(handle);
				if (!matches(name, ".") && !matches(name, "..") && !endsWith(name, "/.") && !endsWith(name, "/.."))
				{
					strcpy(path, path_combine(name, mask));
					doFileScan(L, path, recursive);
					path[len] = '\0';
				}
			}
		}
		io_mask_close(handle);
	}
}

static int doFileMatching(lua_State* L, int recursive)
{
	char path[8192];
	const char* pkgPath;
	const char* filename;
	int pathlen, i;

	/* Get the current package path */
	lua_getglobal(L, "package");
	lua_pushstring(L, "path");
	lua_gettable(L, -2);
	pkgPath = lua_tostring(L, -1);
	lua_pop(L, 2);

	/* If path is same as current, I can ignore it */
	if (path_compare(path_getdir(currentScript), pkgPath))
		pkgPath = "";

	/* Create a table to hold the results */
	lua_newtable(L);

	/* Scan each mask */
	for (i = 1; i < lua_gettop(L); ++i)
	{
		const char* mask = luaL_checkstring(L, i);
		const char* maskWithPath = path_combine(pkgPath, mask);
		strcpy(path, maskWithPath);
		doFileScan(L, path, recursive);
	}

	/* Remove the base package path from all files */
	pathlen = strlen(pkgPath);
	if (pathlen > 0) pathlen++;
	for (i = 1; i <= luaL_getn(L, -1); ++i)
	{
		lua_rawgeti(L, -1, i);
		filename = lua_tostring(L, -1);
		lua_pushstring(L, filename + pathlen);
		lua_rawseti(L, -3, i);
		lua_pop(L, 1);
	}

	debugging = 0;
	return 1;
}


static int lf_matchfiles(lua_State* L)
{
	return doFileMatching(L, 0);
}


static int lf_matchrecursive(lua_State* L)
{
	return doFileMatching(L, 1);
}


static int lf_newfileconfig(lua_State* L)
{
	lua_newtable(L);
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_rawset(L, -3);
	lua_pop(L, 1);
	return 1;
}


static int lf_newpackage(lua_State* L)
{
	int count, i;

	lua_newtable(L);

	/* Add this package to the master list in the registry */
	lua_getregistry(L);
	lua_pushstring(L, "packages");
	lua_gettable(L, -2);
	count = luaL_getn(L, -1);

	lua_pushvalue(L, -3);
	lua_rawseti(L, -2, count + 1);

	lua_pop(L, 2);

	/* Set default values */
	if (count == 0)
	{
		lua_getglobal(L, "project");
		lua_pushstring(L, "name");
		lua_pushstring(L, "name");
		lua_gettable(L, -3);
		lua_settable(L, -4);
		lua_pop(L, 1);
	}
	else
	{
		lua_pushstring(L, "name");
		lua_pushstring(L, "Package");
		lua_pushnumber(L, count);
		lua_concat(L, 2);
		lua_settable(L, -3);
	}

	lua_pushstring(L, "script");
	lua_pushstring(L, currentScript);
	lua_settable(L, -3);

	lua_pushstring(L, "path");
	lua_pushstring(L, path_getdir(currentScript));
	lua_settable(L, -3);

	lua_pushstring(L, "language");
	lua_pushstring(L, "c++");
	lua_settable(L, -3);

	lua_pushstring(L, "kind");
	lua_pushstring(L, "exe");
	lua_settable(L, -3);

	lua_pushstring(L, "objdir");
	lua_pushstring(L, "obj");
	lua_settable(L, -3);

	buildNewConfig(NULL);

	/* Build list of configurations matching what is in the project, and
	 * which can be indexed by name or number */
	lua_pushstring(L, "config");
	lua_newtable(L);

	lua_getglobal(L, "project");
	lua_pushstring(L, "configs");
	lua_gettable(L, -2);
	count = luaL_getn(L, -1);
	
	for (i = 1; i <= count; ++i)
	{
		lua_rawgeti(L, -1, i);

		lua_newtable(L);

		buildNewConfig(lua_tostring(L, -2));
	
		lua_pushvalue(L, -1);
		lua_rawseti(L, -6, i);
		lua_settable(L, -5);
	}

	lua_pop(L, 2);

	/* Hook the index metamethod so I can dynamically add file configs */
	lua_newtable(L);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, lf_newfileconfig);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	/* Set the 'package' global to point to it */
	lua_pushvalue(L, -1);
	lua_setglobal(L, "package");

	lua_settable(L, -3);

	return 1;
}


static int lf_panic(lua_State* L)
{
	lua_Debug ar;
	int stack;

	int top = lua_gettop(L);
	const char* msg = lua_tostring(L, top);
	printf("\n** Error: %s\n", msg);

	for (stack = 0; lua_getstack(L, stack, &ar); ++stack)
	{
		lua_getinfo(L, "S1", &ar);
		if (ar.source && ar.currentline > 0)
		{
			printf("<%.70s: line %d>\n\n", ar.short_src, ar.currentline);
			break;
		}
	}

	return 0;
}

static int lf_rmdir(lua_State* L)
{
	const char* dir = luaL_check_string(L, 1);
	io_rmdir(".", dir);
	return 0;
}


static int lf_getconfigs(lua_State* L)
{
	const char* name = luaL_checkstring(L, 2);
	if (matches(name, "configs"))
		lua_pushstring(L, "__configs");

	lua_rawget(L, 1);
	return 1;
}


static int lf_setconfigs(lua_State* L)
{
	int i;

	const char* name = luaL_checkstring(L, 2);
	if (matches(name, "configs"))
	{
		if (!lua_istable(L, 3))
		{
			lua_pushstring(L, "Project configs must be a table of config names");
			lua_error(L);
		}

		lua_pushstring(L, "config");
		lua_newtable(L);
		for (i = 1; i <= luaL_getn(L, 3); ++i)
		{
			/* Set up the new config table to be added by name */
			lua_rawgeti(L, 3, i);

			/* Create the config and set the defaults */
			lua_newtable(L);
			lua_pushstring(L, "name");
			lua_pushvalue(L, -3);
			lua_rawset(L, -3);

			/* Add the config by index */
			lua_pushvalue(L, -1);
			lua_rawseti(L, -4, i);

			/* Add the config by name */
			lua_rawset(L, -3);
		}

		/* Add the new config table to the project */
		lua_rawset(L, 1);

		/* Add the initially requested item to the list, but under a different
		 * name so __newindex will be called if it is set again */
		lua_pushstring(L, "__configs");
		lua_pushvalue(L, 3);
		lua_rawset(L, 1);
	}
	else
	{
		/* Not setting "configs", just write to the table */
		lua_rawset(L, 1);
	}

	return 0;
}
