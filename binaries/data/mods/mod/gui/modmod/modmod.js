/**
 * @file This GUI page displays all available mods and allows the player to enabled and launch a set of compatible mods.
 */

/**
 * A mod is defined by a mod.json file, for example
 *	{
 *		"name": "0ad",
 *		"version": "0.0.23",
 *		"label": "0 A.D. - Empires Ascendant",
 *		"url": "https://wildfiregames.com/",
 *		"description": "A free, open-source, historical RTS game.",
 *		"dependencies": []
 *	}
 *
 * Or:
 *	{
 *		"name": "mod2",
 *		"label": "Mod 2",
 *		"version": "1.1",
 *		"description": "",
 *		"dependencies": ["0ad<=0.0.23", "rote"]
 *	}
 *
 * A mod is identified by the directory name.
 * A mod must define the "name", "version", "label", "description" and "dependencies" property.
 * The "url" property is optional.
 *
 * The property "name" can consist alphanumeric characters, underscore and dash.
 * The name is used for version comparison of mod dependencies.
 * The property "version" may only contain numbers and up to two periods.
 * The property "label" is a human-readable name of the mod.
 * The property "description" is a human-readable summary of the features of the mod.
 * The property "url" is reference to a website about the mod.
 * The property "dependencies" is an array of strings. Each string is either a modname or a mod version comparison.
 * A mod version comparison is a modname, followed by an operator (=, <, >, <= or >=), followed by a mod version.
 * This allows mods to express upwards and downwards compatibility.
 */

/**
 * Mod definitions loaded from the files, including invalid mods.
 */
var g_Mods = {};

/**
 * Folder names of all mods that are or can be launched.
 */
var g_ModsEnabled = [];
var g_ModsDisabled = [];

/**
 * Name of the mods installed by the ModInstaller.
 */
var g_InstalledMods;

var g_ColorNoModSelected = "255 255 100";
var g_ColorDependenciesMet = "100 255 100";
var g_ColorDependenciesNotMet = "255 100 100";

function init(data, hotloadData)
{
	g_InstalledMods = data && data.installedMods || hotloadData && hotloadData.installedMods || [];

	initMods();
	initGUIButtons(data);
}

function initMods()
{
	loadMods();
	loadEnabledMods();
	validateMods();
	initGUIFilters();
}

function getHotloadData()
{
	return { "installedMods": g_InstalledMods };
}

function loadMods()
{
	g_Mods = Engine.GetAvailableMods();
	deepfreeze(g_Mods);
}

function loadEnabledMods()
{
	g_ModsEnabled = Engine.ConfigDB_GetValue("user", "mod.enabledmods").split(/\s+/).filter(folder => !!g_Mods[folder]);
	g_ModsDisabled = Object.keys(g_Mods).filter(folder => g_ModsEnabled.indexOf(folder) == -1);
}

function validateMods()
{
	for (let folder in g_Mods)
		validateMod(folder, g_Mods[folder], true);
}

function initGUIFilters()
{
	Engine.GetGUIObjectByName("negateFilter").checked = false;
	Engine.GetGUIObjectByName("modGenericFilter").caption = translate("Filter");

	displayModLists();
}

function initGUIButtons(data)
{
	// Either get back to the previous page or quit if there is no previous page
	let cancelButton = !data || data.cancelbutton;
	Engine.GetGUIObjectByName("cancelButton").hidden = !cancelButton;
	Engine.GetGUIObjectByName("quitButton").hidden = cancelButton;
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
	let listObject = Engine.GetGUIObjectByName(listObjectName);

	if (listObjectName == "modsDisabledList")
	{
		let sortFolder = folder => String(g_Mods[folder][listObject.selected_column] || folder);
		folders.sort((folder1, folder2) =>
			listObject.selected_column_order *
			sortFolder(folder1).localeCompare(sortFolder(folder2)));
	}

	folders = folders.filter(filterMod);

	listObject.list_name = folders.map(folder => g_Mods[folder].name).map(name => g_InstalledMods.indexOf(name) == -1 ? name : coloredText(name, "green"));
	listObject.list_folder = folders;
	listObject.list_label = folders.map(folder => g_Mods[folder].label);
	listObject.list_url = folders.map(folder => g_Mods[folder].url || "");
	listObject.list_version = folders.map(folder => g_Mods[folder].version);
	listObject.list_dependencies = folders.map(folder => g_Mods[folder].dependencies.join(" "));
	listObject.list = folders;
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

function applyFilters()
{
	// Save selected rows
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");

	let selectedDisabledFolder = modsDisabledList.list_folder[modsDisabledList.selected];
	let selectedEnabledFolder = modsEnabledList.list_folder[modsEnabledList.selected];

	// Remove selected rows to prevent a link to a non existing item
	modsDisabledList.selected = -1;
	modsEnabledList.selected = -1;

	displayModLists();

	// Restore previously selected rows
	modsDisabledList.selected = modsDisabledList.list_folder.indexOf(selectedDisabledFolder);
	modsEnabledList.selected = modsEnabledList.list_folder.indexOf(selectedEnabledFolder);

	Engine.GetGUIObjectByName("globalModDescription").caption = "";
}

function filterMod(folder)
{
	let mod = g_Mods[folder];

	let negateFilter = Engine.GetGUIObjectByName("negateFilter").checked;
	let searchText = Engine.GetGUIObjectByName("modGenericFilter").caption;

	if (searchText &&
	    searchText != translate("Filter") &&
	    folder.indexOf(searchText) == -1 &&
	    mod.name.indexOf(searchText) == -1 &&
	    mod.label.indexOf(searchText) == -1 &&
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

		guiObject.caption = coloredText(
			sprintf(translate('Dependency not met: %(dep)s'), { "dep": dependency }),
			g_ColorDependenciesNotMet);

		return false;
	}

	guiObject.caption = coloredText(translate('All dependencies met'), g_ColorDependenciesMet);
	return true;
}

/**
 * @param dependency is a mod name or a mod version comparison.
 */
function isDependencyMet(dependency)
{
	let operator = dependency.match(g_RegExpComparisonOperator);
	let [name, version] = operator ? dependency.split(operator[0]) : [dependency, undefined];

	return g_ModsEnabled.some(folder =>
		g_Mods[folder].name == name &&
		(!operator || versionSatisfied(g_Mods[folder].version, operator[0], version)));
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
		dependencies[folder] = g_Mods[folder].dependencies.map(d => d.split(g_RegExpComparisonOperator)[0]);

	g_ModsEnabled.sort((folder1, folder2) =>
		dependencies[folder1].indexOf(g_Mods[folder2].name) != -1 ? 1 :
		dependencies[folder2].indexOf(g_Mods[folder1].name) != -1 ? -1 : 0);

	displayModList("modsEnabledList", g_ModsEnabled);
}

function selectedMod(listObjectName)
{
	let listObject = Engine.GetGUIObjectByName(listObjectName);
	let otherListObject = Engine.GetGUIObjectByName(listObjectName == "modsDisabledList" ?
		"modsEnabledList" : "modsDisabledList");

	if (listObject.selected != -1)
	{
		otherListObject.selected = -1;
		Engine.GetGUIObjectByName("visitWebButton").enabled = true;
		let toggleModButton = Engine.GetGUIObjectByName("toggleModButton");
		toggleModButton.caption = listObjectName == "modsDisabledList" ? "Enable" : "Disable";
		toggleModButton.enabled = true;
		toggleModButton.onPress = listObjectName == "modsDisabledList" ? enableMod : disableMod;
		Engine.GetGUIObjectByName("enabledModUp").enabled = listObjectName == "modsEnabledList";
		Engine.GetGUIObjectByName("enabledModDown").enabled = listObjectName == "modsEnabledList";
	}

	Engine.GetGUIObjectByName("globalModDescription").caption =
		listObject.list[listObject.selected] ?
			g_Mods[listObject.list[listObject.selected]].description :
			'[color="' + g_ColorNoModSelected + '"]' + translate("No mod has been selected.") + '[/color]';
}

function visitModWebsite()
{
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");

	let list = modsEnabledList.selected == -1 ? modsDisabledList : modsEnabledList;
	let folder = list.list_folder[list.selected];
	let url = folder && g_Mods[folder] && g_Mods[folder].url;

	if (!url)
		return;

	if (!url.startsWith("http://") && !url.startsWith("https://"))
		url = "http://" + url;

	openURL(url);
}
