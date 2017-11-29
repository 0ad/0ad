/**
 * Contains JS objects defined by the mod JSON files available.
 * @example
 *{
 *	"public":
 *	{
 *		"name": "0ad",
 *		"version": "0.0.16",
 *		"label": "0 A.D. - Empires Ascendant",
 *		"type": "content|functionality|mixed/mod-pack",
 *		"url": "http://wildfregames.com/",
 *		"description": "A free, open-source, historical RTS game.",
 *		"dependencies": []
 *	},
 *	"foldername2": {
 *		"name": "mod2",
 *		"label": "Mod 2",
 *		"version": "1.1",
 *		"description": "",
 *		"dependencies": ["0ad<=0.0.16", "rote"]
 *	}
 *}
 */
var g_Mods = {};

/**
 * Every mod needs to define these properties.
 */
var g_RequiredProperties = ["name", "label", "description", "dependencies", "version"];

/**
 * Version checks in mod dependencies can use these operators.
 */
var g_CompareVersion = /(<=|>=|<|>|=)/;

/**
 * Folder names of all mods that are or can be launched.
 */
var g_ModsEnabled = [];
var g_ModsDisabled = [];

var g_ColorNoModSelected = "255 255 100";
var g_ColorDependenciesMet = "100 255 100";
var g_ColorDependenciesNotMet = "255 100 100";

/**
 * Dropdown choices to sort the available mods.
 */
var g_SortOptions = [
	{
		"id": "name",
		"title": translate("Name"),
		"value": folder => g_Mods[folder].name.toLowerCase(),
		"default": true
	},
	{
		"id": "folder",
		"title": translate("Folder"),
		"value": folder => folder.toLowerCase()
	},
	{
		"id": "label",
		"title": translate("Label"),
		"value": folder => g_Mods[folder].label.toLowerCase()
	},
	{
		"id": "version",
		"title": translate("Version"),
		"value": folder => g_Mods[folder].version
	}
];

function init()
{
	loadMods();
	initGUIFilters();
}

function loadMods()
{
	let mods = Engine.GetAvailableMods();

	for (let folder in mods)
		if (g_RequiredProperties.every(prop => mods[folder][prop] !== undefined))
			g_Mods[folder] = mods[folder];
		else
			warn("Skipping mod '" + mod + "' which does not define '" + property + "'.");

	deepfreeze(g_Mods);

	g_ModsEnabled = Engine.ConfigDB_GetValue("user", "mod.enabledmods").split(/\s+/).filter(folder => !!g_Mods[folder]);
	g_ModsDisabled = Object.keys(g_Mods).filter(folder => g_ModsEnabled.indexOf(folder) == -1);
}

function initGUIFilters()
{
	Engine.GetGUIObjectByName("negateFilter").checked = false;
	Engine.GetGUIObjectByName("modGenericFilter").caption = translate("Filter");
	Engine.GetGUIObjectByName("modTypeFilter").selected = 0;

	let sortBy = Engine.GetGUIObjectByName("sortBy");
	sortBy.list = g_SortOptions.map(option => option.title);
	sortBy.selected = g_SortOptions.findIndex(option => option.default);

	Engine.GetGUIObjectByName("isOrderDescending").checked = false;

	displayModLists();
}

function saveMods()
{
	sortEnabledMods();
	Engine.ConfigDB_CreateValue("user", "mod.enabledmods", ["mod"].concat(g_ModsEnabled).join(" "));
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
}

function startMods()
{
	sortEnabledMods();
	Engine.SetMods(["mod"].concat(g_ModsEnabled));
	Engine.RestartEngine();
}

function displayModLists()
{
	displayModList("modsEnabledList", g_ModsEnabled);
	displayModList("modsDisabledList", g_ModsDisabled);
}

function displayModList(listObjectName, folders)
{
	updateModTypes();

	let sortOption = g_SortOptions[Engine.GetGUIObjectByName("sortBy").selected];
	if (sortOption && listObjectName == "modsDisabledList")
	{
		folders.sort((version1, version2) => sortOption.value(version1).localeCompare(sortOption.value(version2)));
		if (Engine.GetGUIObjectByName("isOrderDescending").checked)
			folders.reverse();
	}

	folders = folders.filter(filterMod);

	let listObject = Engine.GetGUIObjectByName(listObjectName);
	listObject.list_name = folders.map(folder => g_Mods[folder].name);
	listObject.list_modFolderName = folders;
	listObject.list_modLabel = folders.map(folder => g_Mods[folder].label);
	listObject.list_modType = folders.map(folder => g_Mods[folder].type || "");
	listObject.list_modURL = folders.map(folder => g_Mods[folder].url || "");
	listObject.list_modVersion = folders.map(folder => g_Mods[folder].version);
	listObject.list_modDependencies = folders.map(folder => g_Mods[folder].dependencies.join(" "));
	listObject.list = folders;
}

function updateModTypes()
{
	let types = [translate("Type: Any")];
	for (let folder in g_Mods)
	{
		if (g_Mods[folder].type && types.indexOf(g_Mods[folder].type) == -1)
			types.push(g_Mods[folder].type);
	}
	Engine.GetGUIObjectByName("modTypeFilter").list = types;
}

function enableMod()
{
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");
	let pos = modsDisabledList.selected;

	if (pos == -1 || !areDependenciesMet(g_ModsDisabled[pos]))
		return;

	g_ModsEnabled.push(g_ModsDisabled.splice(pos, 1)[0]);

	if (pos >= g_ModsDisabled.length)
		--pos;

	modsDisabledList.selected = pos;

	displayModLists();
}

function disableMod()
{
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let pos = modsEnabledList.selected;
	if (pos == -1)
		return;

	g_ModsDisabled.push(g_ModsEnabled.splice(pos, 1)[0]);

	// Remove mods that required the removed mod and cascade
	// Sort them, so we know which ones can depend on the removed mod
	// TODO: Find position where the removed mod would have fit (for now assume idx 0)

	sortEnabledMods();

	for (let i = 0; i < g_ModsEnabled.length; ++i)
		if (!areDependenciesMet(g_ModsEnabled[i]))
		{
			g_ModsDisabled.push(g_ModsEnabled.splice(i, 1)[0]);
			--i;
		}

	modsEnabledList.selected = Math.min(pos, g_ModsEnabled.length - 1);

	displayModLists();
}

function resetFilters()
{
	Engine.GetGUIObjectByName("modGenericFilter").caption = "";
	Engine.GetGUIObjectByName("negateFilter").checked = false;

	// Calling displayModLists is not needed as the selection changes and that calls applyFilters
	Engine.GetGUIObjectByName("modTypeFilter").selected = 0;
}

function applyFilters()
{
	// Save selected rows
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let selectedModAvailableFolder = modsDisabledList.list_modFolderName[modsDisabledList.selected];
	let selectedModEnabledFolder = modsEnabledList.list_modFolderName[modsEnabledList.selected];

	// Remove selected rows to prevent a link to a non existing item
	modsDisabledList.selected = -1;
	modsEnabledList.selected = -1;

	displayModLists();

	// Restore previously selected rows
	modsDisabledList.selected = modsDisabledList.list_modFolderName.indexOf(selectedModAvailableFolder);
	modsEnabledList.selected = modsEnabledList.list_modFolderName.indexOf(selectedModEnabledFolder);

	Engine.GetGUIObjectByName("globalModDescription").caption = "";
}

function filterMod(folder)
{
	let mod = g_Mods[folder];

	let modTypeFilter = Engine.GetGUIObjectByName("modTypeFilter");
	let negateFilter = Engine.GetGUIObjectByName("negateFilter").checked;

	if (modTypeFilter.selected > 0 && (mod.type || "") != modTypeFilter.list[modTypeFilter.selected])
		return negateFilter;

	let searchText = Engine.GetGUIObjectByName("modGenericFilter").caption;
	if (searchText &&
	    searchText != translate("Filter") &&
	    folder.indexOf(searchText) == -1 &&
	    mod.name.indexOf(searchText) == -1 &&
	    mod.label.indexOf(searchText) == -1 &&
	    (mod.type || "").indexOf(searchText) == -1 &&
	    (mod.url || "").indexOf(searchText) == -1 &&
	    mod.version.indexOf(searchText) == -1 &&
	    mod.description.indexOf(searchText) == -1 &&
	    mod.dependencies.indexOf(searchText) == -1)
		return negateFilter;

	return !negateFilter;
}

function closePage()
{
	Engine.SwitchGuiPage("page_pregame.xml", {});
}

/**
 * Moves an item in the list up or down.
 */
function moveCurrItem(objectName, up)
{
	let obj = Engine.GetGUIObjectByName(objectName);
	let idx = obj.selected;
	if (idx == -1)
		return;

	let num = obj.list.length;
	let idx2 = idx + (up ? -1 : 1);
	if (idx2 < 0 || idx2 >= num)
		return;

	let tmp = g_ModsEnabled[idx];
	g_ModsEnabled[idx] = g_ModsEnabled[idx2];
	g_ModsEnabled[idx2] = tmp;

	obj.list = g_ModsEnabled;
	obj.selected = idx2;

	displayModList("modsEnabledList", g_ModsEnabled);
}

function areDependenciesMet(folder)
{
	let guiObject = Engine.GetGUIObjectByName("message");

	for (let dependency of g_Mods[folder].dependencies)
	{
		if (isDependencyMet(dependency))
			continue;

		guiObject.caption =
			'[color="' + g_ColorDependenciesNotMet + '"]' +
			sprintf(translate('Dependency not met: %(dep)s'), { "dep": dependency }) +
			'[/color]';

		return false;
	}

	guiObject.caption =
		'[color="' + g_ColorDependenciesMet + '"]' +
		translate('All dependencies met') +
		'[/color]';

	return true;
}

/**
 * @param dependency is a mod name or a mod version comparison.
 */
function isDependencyMet(dependency)
{
	let operator = dependency.match(g_CompareVersion);
	let [name, version] = operator ? dependency.split(operator[0]) : [dependency, undefined];

	return g_ModsEnabled.some(folder =>
		g_Mods[folder].name == name &&
		(!operator || versionSatisfied(g_Mods[folder].version, operator, version)));
}

/**
 * Compares the given versions using the given operator.
 *       '-' or '_' is ignored. Only numbers are supported.
 * @note "5.3" < "5.3.0"
 */
function versionSatisfied(version1, operator, version2)
{
	let versionList1 = version1.split(/[-_]/)[0].split(/\./g);
	let versionList2 = version2.split(/[-_]/)[0].split(/\./g);

	let eq = operator.indexOf("=") != -1;
	let lt = operator.indexOf("<") != -1;
	let gt = operator.indexOf(">") != -1;

	for (let i = 0; i < Math.min(versionList1.length, versionList2.length); ++i)
	{
		let diff = +versionList1[i] - +versionList2[i];
		if (isNaN(diff))
			continue;

		if (gt && diff > 0 || lt && diff < 0)
			return true;

		if (gt && diff < 0 || lt && diff > 0 || eq && diff)
			return false;
	}

	// common prefix matches
	let ldiff = versionList1.length - versionList2.length;
	if (!ldiff)
		return eq;

	// NB: 2.3 != 2.3.0
	if (ldiff < 0)
		return lt;

	return gt;
}

function sortEnabledMods()
{
	let dependencies = {};
	for (let folder of g_ModsEnabled)
		dependencies[folder] = g_Mods[folder].dependencies.map(d => d.split()[0]);

	g_ModsEnabled.sort((folder1, folder2) =>
		dependencies[folder1].indexOf(g_Mods[folder2].name) != -1 ? 1 :
		dependencies[folder2].indexOf(g_Mods[folder1].name) != -1 ? -1 : 0);

	displayModList("modsEnabledList", g_ModsEnabled);
}

function showModDescription(listObjectName)
{
	let listObject = Engine.GetGUIObjectByName(listObjectName);

	Engine.GetGUIObjectByName("globalModDescription").caption =
		listObject.list[listObject.selected] ?
			g_Mods[listObject.list[listObject.selected]].description :
			'[color="' + g_ColorNoModSelected + '"]' + translate("No mod has been selected.") + '[/color]';
}
