
function getExistingModsFromConfig(from_list_of_mods = null)
{
	var existingMods = [];

	var mods = [];
	var cfgMods = Engine.ConfigDB_GetValue("user", "mod.enabledmods");
	if (cfgMods.length > 0)
		mods = cfgMods.split(/\s+/);

	mods.forEach(function(mod) {
		if (!from_list_of_mods || mod in from_list_of_mods)
			existingMods.push(mod);
	});

	return existingMods;
}

/**
 * @param dependency: Either id (unique modJson.name) and version or only the unique mod name.
 *                    Concatenated by either "=", ">", "<", ">=", "<=".
 */
function isDependencyMet(dependency_idAndVersion, modsEnabled = null)
{
	if (!modsEnabled)
		modsEnabled = g_modsEnabled;

	// Split on {=,<,<=,>,>=} and use the second part as the version number
	// and whatever we split on as a way to handle that version.
	var op = dependency_idAndVersion.match(/(<=|>=|<|>|=)/);
	// Did the dependency contain a version number?
	if (op)
	{
		op = op[0];
		var dependency_parts = dependency_idAndVersion.split(op);
		var dependency_version = dependency_parts[1];
		var dependency_id = dependency_parts[0];
	}
	else
		var dependency_id = dependency_idAndVersion;

	// modsEnabled_key currently is the mod folder name.
	for each (var modsEnabled_key in modsEnabled)
	{
		var modJson = g_mods[modsEnabled_key];
		if (modJson.name != dependency_id)
			continue;

		// There could be another mod with a satisfying version
		if (!op || versionSatisfied(modJson.version, op, dependency_version))
			return true;
	}
	return false;
}

/**
 * Returns true if @p version satisfies @p op (<,<=,=,>=,>) @p requirement.
 * @note @p version and @p requirement are split on '.' and everything after
 *       '-' or '_' is ignored. Only numbers are supported.
 * @note "5.3" < "5.3.0"
 */
function versionSatisfied(version, op, requirement)
{
	var reqList = requirement.split(/[-_]/)[0].split(/\./g);
	var avList = version.split(/[-_]/)[0].split(/\./g);

	var eq = op.indexOf("=") !== -1;
	var lt = op.indexOf("<") !== -1;
	var gt = op.indexOf(">") !== -1;
	if (!(eq || lt || gt))
	{
		warn("No valid compare op");
		return false;
	}

	var l = Math.min(reqList.length, avList.length);
	for (var i = 0; i < l; ++i)
	{
		// TODO: Handle NaN
		var diff = +avList[i] - +reqList[i];

		// Early success
		if (gt && diff > 0)
			return true;
		if (lt && diff < 0)
			return true;

		// Early failure
		if (gt && diff < 0)
			return false;
		if (lt && diff > 0)
			return false;
		if (eq && diff !== 0)
			return false;
	}
	// common prefix matches
	var ldiff = avList.length - reqList.length;
	if (ldiff === 0)
		return eq;
	// NB: 2.3 != 2.3.0
	if (ldiff < 0)
		return lt;
	if (ldiff > 0)
		return gt;

	// Can't be reached
	error("version checking code broken");
	return false;
}

function sortMods(mods = null, modsEnabled = null)
{
	if (!modsEnabled)
		modsEnabled = g_modsEnabled;
	if (!mods)
		mods = g_mods;

	// store the list of dependencies per mod, but strip the version numbers
	var deps = {};
	for (var mod of modsEnabled)
	{
		deps[mod] = [];
		if (!mods[mod].dependencies)
			continue;
		deps[mod] = mods[mod].dependencies.map(function(d) { return d.split(/(<=|>=|<|>|=)/)[0]; });
	}
	var sortFunction = function(mod1, mod2)
	{
		var name1 = mods[mod1].name;
		var name2 = mods[mod2].name;
		if (deps[mod1].indexOf(name2) != -1)
			return 1;
		if (deps[mod2].indexOf(name1) != -1)
			return -1;
		return 0;
	}
	modsEnabled.sort(sortFunction);
}

