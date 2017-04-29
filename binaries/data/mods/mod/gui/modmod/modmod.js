/*
Example contents of g_mods:
{
	"foldername1": { // this is the content of the json file in a specific mod
		name: "unique_shortname", // eg "0ad", "rote"
		version: "0.0.16",
		label: "Nice Mod Name", // eg "0 A.D. - Empires Ascendant"
		type: "content|functionality|mixed/mod-pack",
		url: "http://wildfregames.com/",
		description: "",
		dependencies: [] // (name({<,<=,==,>=,>}version)?)+
	},
	"foldername2": {
		name: "mod2",
		label: "Mod 2",
		version: "1.1",
		type: "content|functionality|mixed/mod-pack", // optional
		url: "http://play0ad.wfg.com/",	//optional
		description: "",
		dependencies: []
	}
}
*/


var g_mods = {}; // Contains all JSONs as explained in the structure above
var g_modsEnabled = []; // folder names
var g_modsAvailable = []; // folder names

const g_sortByOptions = [translate("Name"), translate("Folder"), translate("Label"), translate("Version")];
const SORT_BY_NAME = 0;
const SORT_BY_FOLDER = 1;
const SORT_BY_LABEL = 2;
const SORT_BY_VERSION = 3;

var g_modTypes = [translate("Type: Any")];

/**
 * Fetches the mod lists in JSON from the Engine.
 * Initiates a first creation of the GUI lists.
 * Enabled mods are read from the Configuration and checked if still available.
 */
function init()
{
	let mods = Engine.GetAvailableMods();
	let keys = ["name", "label", "description", "dependencies", "version"];
	Object.keys(mods).forEach(function(k) {
		for (let i = 0; i < keys.length; ++i)
			if (!keys[i] in mods[k])
			{
				log("Skipping mod '"+k+"'. Missing property '"+keys[i]+"'.");
				return;
			}

		g_mods[k] = mods[k];
	});

	g_modsEnabled = getExistingModsFromConfig();
	g_modsAvailable = Object.keys(g_mods).filter(function(i) { return g_modsEnabled.indexOf(i) === -1; });

	Engine.GetGUIObjectByName("negateFilter").checked = false;
	Engine.GetGUIObjectByName("modGenericFilter").caption = translate("Filter");
	Engine.GetGUIObjectByName("modTypeFilter").selected = 0;

	var sortBy = Engine.GetGUIObjectByName("sortBy");
	sortBy.list = g_sortByOptions;
	sortBy.selected = SORT_BY_NAME;

	// sort ascending by default
	Engine.GetGUIObjectByName("isOrderDescending").checked = false;

	generateModsLists();

	Engine.GetGUIObjectByName("message").caption = translate("Message: Mods Loaded.");
}

/**
 * Recreating both the available and enabled mods lists.
 */
function generateModsLists()
{
	generateModsList('modsAvailableList', g_modsAvailable);
	generateModsList('modsEnabledList', g_modsEnabled);
}

function saveMods()
{
	// always sort mods before saving
	sortMods();
	Engine.ConfigDB_CreateValue("user", "mod.enabledmods", ["mod"].concat(g_modsEnabled).join(" "));
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
}

function startMods()
{
	// always sort mods before starting
	sortMods();
	Engine.SetMods(["mod"].concat(g_modsEnabled));
	Engine.RestartEngine();
}

function getExistingModsFromConfig()
{
	var existingMods = [];

	var mods = [];
	var cfgMods = Engine.ConfigDB_GetValue("user", "mod.enabledmods");
	if (cfgMods.length > 0)
		mods = cfgMods.split(/\s+/);

	mods.forEach(function(mod) {
		if (mod in g_mods)
			existingMods.push(mod);
	});

	return existingMods;
}

/**
 * (Re-)Generate List of all mods.
 * @param listObjectName The GUI object's name (e.g. "modsEnabledList", "modsAvailableList")
 */
function generateModsList(listObjectName, mods)
{
	var sortBy = Engine.GetGUIObjectByName("sortBy");
	var orderDescending = Engine.GetGUIObjectByName("isOrderDescending");
	var isDescending = orderDescending && orderDescending.checked;

	// TODO: Sorting mods by dependencies would be nice
	if (listObjectName != "modsEnabledList")
	{
		var idx = -1;
		if (sortBy)
			idx = sortBy.selected;

		switch (idx)
		{
		default:
			warn("generateModsList: invalid index '"+idx+"'"); // fall through
		// sort by unique name alphanumerically by default:
		case -1:
		case SORT_BY_NAME:
			mods.sort(function(a, b)
			{
				var ret = compare(g_mods[a].name.toLowerCase(), g_mods[b].name.toLowerCase());
				return ret * (isDescending ? -1 : 1);
			});
			break;
		case SORT_BY_FOLDER:
			mods.sort(function(a, b)
			{
				return compare(a.toLowerCase(), b.toLowerCase()) * (isDescending ? -1 : 1);
			});
			break;
		case SORT_BY_LABEL:
			mods.sort(function(a, b)
			{
				var ret = compare(g_mods[a].label.toLowerCase(), g_mods[b].label.toLowerCase());
				return ret * (isDescending ? -1 : 1);
			});
			break;
		case SORT_BY_VERSION:
			mods.sort(function(a, b)
			{
				// TODO reuse actual logic
				var ret = compare(g_mods[a].version, g_mods[b].version);
				return ret * (isDescending ? -1 : 1);
			});
			break;
		}
	}

	var [keys, names, folders, labels, types, urls, versions, dependencies] = [[],[],[],[],[],[],[],[]];
	mods.forEach(function(foldername)
	{
		var mod = g_mods[foldername];
		if (mod.type && g_modTypes.indexOf(mod.type) == -1)
			g_modTypes.push(mod.type);

		if (filterMod(foldername))
			return;

		keys.push(foldername);
		names.push(mod.name);
		folders.push('[color="45 45 45"](' + foldername + ')[/color]');

		labels.push(mod.label || "");
		types.push(mod.type || "");
		urls.push(mod.url || "");
		versions.push(mod.version || "");
		dependencies.push((mod.dependencies || []).join(" "));
	});

	// Update the list
	var obj  = Engine.GetGUIObjectByName(listObjectName);
	obj.list_name = names;
	obj.list_modFolderName = folders;
	obj.list_modLabel = labels;
	obj.list_modType = types;
	obj.list_modURL = urls;
	obj.list_modVersion = versions;
	obj.list_modDependencies = dependencies;

	obj.list = keys;

	var modTypeFilter = Engine.GetGUIObjectByName("modTypeFilter");
	modTypeFilter.list = g_modTypes;
}

function compare(a, b)
{
	return ( (a > b) ? 1 : (b > a) ? -1 : 0 );
}

function enableMod()
{
	var obj = Engine.GetGUIObjectByName("modsAvailableList");
	var pos = obj.selected;
	if (pos === -1)
		return;

	var mod = g_modsAvailable[pos];

	// Move it to the other table
	// check dependencies, warn about not satisfied dependencies and abort if so:
	if (!areDependenciesMet(mod))
		return;

	g_modsEnabled.push(g_modsAvailable.splice(pos, 1)[0]);

	if (pos >= g_modsAvailable.length)
		pos--;
	obj.selected = pos;

	generateModsLists();
}

function disableMod()
{
	var obj = Engine.GetGUIObjectByName("modsEnabledList");
	var pos = obj.selected;
	if (pos === -1)
		return;

	var mod = g_modsEnabled[pos];

	g_modsAvailable.push(g_modsEnabled.splice(pos, 1)[0]);

	// Remove mods that required the removed mod and cascade
	// Sort them, so we know which ones can depend on the removed mod
	// TODO: Find position where the removed mod would have fit (for now assume idx 0)
	sortMods();
	for (var i = 0; i < g_modsEnabled.length; ++i)
	{
		if (!areDependenciesMet(g_modsEnabled[i]))
		{
			g_modsAvailable.push(g_modsEnabled.splice(i, 1)[0]);
			--i;
		}
	}

	// select the last element even if more than 1 mod has been removed:
	if (pos > g_modsEnabled.length - 1)
		pos = g_modsEnabled.length - 1;
	obj.selected = pos;

	generateModsLists();
}

function resetFilters()
{
	Engine.GetGUIObjectByName("modGenericFilter").caption = "";
	Engine.GetGUIObjectByName("negateFilter").checked = false;

	// NOTE: Calling generateModsLists() is not needed as the selection changes and that calls applyFilters()
	Engine.GetGUIObjectByName("modTypeFilter").selected = 0;
}

function applyFilters()
{
	// Save selected rows
	let modsAvailableList = Engine.GetGUIObjectByName("modsAvailableList");
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let selectedModAvailableFolder = modsAvailableList.list_modFolderName[modsAvailableList.selected];
	let selectedModEnabledFolder = modsEnabledList.list_modFolderName[modsEnabledList.selected];

	// Remove selected rows to prevent a link to a non existing item
	modsAvailableList.selected = -1;
	modsEnabledList.selected = -1;

	generateModsLists();

	// Restore previously selected rows
	modsAvailableList.selected = modsAvailableList.list_modFolderName.indexOf(selectedModAvailableFolder);
	modsEnabledList.selected = modsEnabledList.list_modFolderName.indexOf(selectedModEnabledFolder);

	Engine.GetGUIObjectByName("globalModDescription").caption = "";
}

/**
 * Filter a mod based on the status of the filters.
 *
 * @param modFolder Mod to be tested.
 * @return True if mod should not be displayed.
 */
function filterMod(modFolder)
{
	var mod = g_mods[modFolder];

	var modTypeFilter = Engine.GetGUIObjectByName("modTypeFilter");
	var genericFilter = Engine.GetGUIObjectByName("modGenericFilter");
	var negateFilter = Engine.GetGUIObjectByName("negateFilter");

	// TODO: and result of filters together (type && generic)

	// We assume index 0 means display all for any given filter.
	if (modTypeFilter.selected > 0
	    && (mod.type || "") != modTypeFilter.list[modTypeFilter.selected])
		return !negateFilter.checked;

	if (genericFilter && genericFilter.caption && genericFilter.caption != "" && genericFilter.caption != translate("Filter"))
	{
		var t = genericFilter.caption;
		if (modFolder.indexOf(t) === -1
		    && mod.name.indexOf(t) === -1
		    && mod.label.indexOf(t) === -1
		    && (mod.type || "").indexOf(t) === -1
		    && (mod.url || "").indexOf(t) === -1
		    && mod.version.indexOf(t) === -1
		    && mod.description.indexOf(t) === -1
		    && mod.dependencies.indexOf(t) === -1)
		{
			return !negateFilter.checked;
		}
	}

	return negateFilter.checked;
}

function closePage()
{
	Engine.SwitchGuiPage("page_pregame.xml", {});
}

/**
 * Moves an item in the list @p objectName up or down depending on the value of @p up.
 */
function moveCurrItem(objectName, up)
{
	var obj = Engine.GetGUIObjectByName(objectName);
	if (!obj)
		return;

	var idx = obj.selected;
	if (idx === -1)
		return;

	var num = obj.list.length;
	var idx2 = idx + (up ? -1 : 1);
	if (idx2 < 0 || idx2 >= num)
		return;

	var tmp = g_modsEnabled[idx];
	g_modsEnabled[idx] = g_modsEnabled[idx2];
	g_modsEnabled[idx2] = tmp;

	// Selected object reached the new position.
	obj.list = g_modsEnabled;
	obj.selected = idx2;
	generateModsList('modsEnabledList', g_modsEnabled);
}

function areDependenciesMet(mod)
{
	var guiObject = Engine.GetGUIObjectByName("message");
	for (var dependency of g_mods[mod].dependencies)
	{
		if (isDependencyMet(dependency))
			continue;
		guiObject.caption = '[color="250 100 100"]' + translate(sprintf('Dependency not met: %(dep)s', { "dep": dependency })) +'[/color]';
		return false;
	}

	guiObject.caption =  '[color="100 250 100"]' + translate('All dependencies met') + '[/color]';

	return true;
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
	for (var modsEnabled_key of modsEnabled)
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

function sortMods()
{
	// store the list of dependencies per mod, but strip the version numbers
	var deps = {};
	for (var mod of g_modsEnabled)
	{
		deps[mod] = [];
		if (!g_mods[mod].dependencies)
			continue;
		deps[mod] = g_mods[mod].dependencies.map(function(d) { return d.split(/(<=|>=|<|>|=)/)[0]; });
	}
	var sortFunction = function(mod1, mod2)
	{
		var name1 = g_mods[mod1].name;
		var name2 = g_mods[mod2].name;
		if (deps[mod1].indexOf(name2) != -1)
			return 1;
		if (deps[mod2].indexOf(name1) != -1)
			return -1;
		return 0;
	}
	g_modsEnabled.sort(sortFunction);
	generateModsList("modsEnabledList", g_modsEnabled);
}

function showModDescription(listObjectName)
{
	var listObject = Engine.GetGUIObjectByName(listObjectName);
	if (listObject.selected == -1)
		var desc = '[color="255 100 100"]' + translate("No mod has been selected.") + '[/color]';
	else
	{
		let key = listObject.list[listObject.selected];
		var desc = g_mods[key].description;
	}

	Engine.GetGUIObjectByName("globalModDescription").caption = desc;
}
