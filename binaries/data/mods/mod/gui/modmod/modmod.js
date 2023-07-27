/**
 * @file This GUI page displays all available mods and allows the player to enabled and launch a set of compatible mods.
 */

/**
 * A mod is defined by a mod.json file, for example
 *	{
 *		"name": "0ad",
 *		"version": "0.0.27",
 *		"label": "0 A.D. - Empires Ascendant",
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
 *		"dependencies": ["0ad<=0.0.27", "rote"]
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

var g_ModsEnabledFiltered = [];
var g_ModsDisabledFiltered = [];

/**
 * Cache mod compatibility recomputed when some mod is enbaled/disabled.
 */
var g_ModsCompatibility = [];

/**
 * Name of the mods installed by the ModInstaller.
 */
var g_InstalledMods;

var g_HasIncompatibleMods;

var g_FakeMod = {
	"name": translate("This mod does not exist"),
	"version": "",
	"label": "",
	"url": "",
	"description": "",
	"dependencies": []
};

var g_ColorNoModSelected = "255 255 100";
var g_ColorDependenciesMet = "100 255 100";
var g_ColorDependenciesNotMet = "255 100 100";

function init(data, hotloadData)
{
	g_InstalledMods = data && data.installedMods || hotloadData && hotloadData.installedMods || [];
	g_HasIncompatibleMods = Engine.HasIncompatibleMods();

	initMods();
	initGUIButtons(data);
	if (g_HasIncompatibleMods)
		Engine.PushGuiPage("page_incompatible_mods.xml", {});
}

function initMods()
{
	loadMods();
	loadEnabledMods();
	recomputeCompatibility();
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

/**
 * Return fake mod for mods which do not exist
*/
function getMod(folder)
{
	return !!g_Mods[folder] ? g_Mods[folder] : g_FakeMod;
}

function loadEnabledMods()
{
	if (g_HasIncompatibleMods)
		g_ModsEnabled = Engine.GetEnabledMods().concat(Engine.GetIncompatibleMods())
			.filter(folder => folder != "mod");
	else
		g_ModsEnabled = Engine.GetEnabledMods().filter(folder => !!g_Mods[folder]);
	g_ModsDisabled = Object.keys(g_Mods).filter(folder => g_ModsEnabled.indexOf(folder) == -1);
	g_ModsEnabledFiltered = g_ModsEnabled;
	g_ModsDisabledFiltered = g_ModsDisabled;
}

function validateMods()
{
	for (let folder in g_Mods)
		validateMod(folder, g_Mods[folder], true);
}

function initGUIFilters()
{
	Engine.GetGUIObjectByName("negateFilter").checked = false;
	Engine.GetGUIObjectByName("modCompatibleFilter").checked = true;

	displayModLists();
}

function initGUIButtons(data)
{
	// Either get back to the previous page or quit if there is no previous page
	let hasPreviousPage = !data || data.cancelbutton || false;
	Engine.GetGUIObjectByName("cancelButton").hidden = !hasPreviousPage;
	Engine.GetGUIObjectByName("quitButton").hidden = hasPreviousPage;
	// Turn 'save' off, it will be enabled on any change.
	Engine.GetGUIObjectByName("saveConfigurationButton").enabled = false;
	Engine.GetGUIObjectByName("toggleModButton").caption = translateWithContext("mod activation", "Enable");
}

function saveMods()
{
	sortEnabledMods();
	Engine.ConfigDB_CreateAndSaveValue("user", "mod.enabledmods", ["mod"].concat(g_ModsEnabled).join(" "));

	Engine.GetGUIObjectByName("saveConfigurationButton").enabled = false;
}

function startMods()
{
	saveMods();
	if (!Engine.SetModsAndRestartEngine(["mod"].concat(g_ModsEnabled)))
		Engine.GetGUIObjectByName("message").caption = coloredText(translate('Dependencies not met'), g_ColorDependenciesNotMet);
}

function displayModLists()
{
	g_ModsEnabledFiltered = displayModList("modsEnabledList", g_ModsEnabled, true);
	g_ModsDisabledFiltered = displayModList("modsDisabledList", g_ModsDisabled, false);
}

function displayModList(listObjectName, folders, enabled)
{
	let listObject = Engine.GetGUIObjectByName(listObjectName);

	if (listObjectName == "modsDisabledList")
	{
		let sortFolder = folder => String(getMod(folder)[listObject.selected_column] || folder);
		folders.sort((folder1, folder2) =>
			listObject.selected_column_order *
			sortFolder(folder1).localeCompare(sortFolder(folder2)));
	}

	folders = folders.filter(filterMod);
	if (!enabled && Engine.GetGUIObjectByName("modCompatibleFilter").checked)
		folders = folders.filter(folder => g_ModsCompatibility[folder]);

	let selected = listObject.selected !== -1 ? listObject.list_name[listObject.selected] : null;

	listObject.list_name = folders.map(folder => colorMod(folder, getMod(folder).name || "", enabled));
	listObject.list_folder = folders.map(folder => colorMod(folder, folder, enabled));
	listObject.list_label = folders.map(folder => colorMod(folder, getMod(folder).label || "", enabled));
	listObject.list_url = folders.map(folder => colorMod(folder, getMod(folder).url || "", enabled));
	listObject.list_version = folders.map(folder => colorMod(folder, getMod(folder).version || "", enabled));
	listObject.list_dependencies = folders.map(folder => colorMod(folder, getMod(folder)?.dependencies.join(" ") || "", enabled));
	listObject.list = folders;

	listObject.selected = selected ? listObject.list_name.indexOf(selected) : -1;

	return folders;
}

function getModColor(folder, enabled)
{
	if (!g_ModsCompatibility[folder])
		return enabled ? g_ColorDependenciesNotMet : "gray";
	if (g_InstalledMods.indexOf(getMod(folder).name) != -1)
		return "green";
	return false;
}

function colorMod(folder, text, enabled)
{
	let color = getModColor(folder, enabled);
	return color ? coloredText(text, color) : text;
}

function reloadDisabledMods()
{
	g_ModsDisabled = Object.keys(g_Mods).filter(folder => g_ModsEnabled.indexOf(folder) == -1);
}

function enableMod()
{
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");
	let pos = modsDisabledList.selected;

	if (pos == -1 || !g_ModsCompatibility[g_ModsDisabledFiltered[pos]])
		return;

	g_ModsEnabled.push(g_ModsDisabledFiltered.splice(pos, 1)[0]);
	reloadDisabledMods();
	recomputeCompatibility();

	Engine.GetGUIObjectByName("saveConfigurationButton").enabled = true;

	if (pos >= g_ModsDisabledFiltered.length)
		--pos;

	displayModLists();
	Engine.GetGUIObjectByName("message").caption = "";
	modsDisabledList.selected = pos;
}

function disableMod()
{
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let pos = modsEnabledList.selected;
	if (pos == -1)
		return;

	// Find true position of disabled mod and remove it
	let disabledMod = g_ModsEnabledFiltered[pos];
	for (let i = 0; i < g_ModsEnabled.length; ++i)
		if (g_ModsEnabled[i] == disabledMod)
		{
			g_ModsEnabled.splice(i, 1);
			break;
		}

	if (!!g_Mods[disabledMod])
		g_ModsDisabled.push(disabledMod);

	// Remove mods that required the removed mod and cascade
	// Sort them, so we know which ones can depend on the removed mod
	// TODO: Find position where the removed mod would have fit (for now assume idx 0)

	sortEnabledMods();

	for (let i = 0; i < g_ModsEnabled.length; ++i)
		if (!areDependenciesMet(g_ModsEnabled[i], true))
		{
			g_ModsDisabled.push(g_ModsEnabled.splice(i, 1)[0]);
			--i;
		}

	Engine.GetGUIObjectByName("saveConfigurationButton").enabled = true;

	recomputeCompatibility(true);
	displayModLists();
	Engine.GetGUIObjectByName("message").caption = "";
	modsEnabledList.selected = Math.min(pos, g_ModsEnabledFiltered.length - 1);
}

function filterMod(folder)
{
	let mod = getMod(folder);

	let negateFilter = Engine.GetGUIObjectByName("negateFilter").checked;
	let searchText = Engine.GetGUIObjectByName("modGenericFilter").caption;

	if (searchText &&
	    folder.indexOf(searchText) == -1 &&
	    (mod.name || "").indexOf(searchText) == -1 &&
	    (mod.label || "").indexOf(searchText) == -1 &&
	    (mod.url || "").indexOf(searchText) == -1 &&
	    (mod.version || "").indexOf(searchText) == -1 &&
	    (mod.description || "").indexOf(searchText) == -1 &&
	    (mod.dependencies || "").indexOf(searchText) == -1)
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
	// Prevent moving while filters are applied
	// because we would need to map filtered positions
	// to not filtered positions so changes will persist.
	if (Engine.GetGUIObjectByName("modGenericFilter").caption)
		return;

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

	g_ModsEnabledFiltered = displayModList("modsEnabledList", g_ModsEnabled, true);
	obj.selected = idx2;
}

function areDependenciesMet(folder, disabledAction = false)
{
	// If we disabled mod it will not change satus of incompatible mods
	if (disabledAction && !g_ModsCompatibility[folder])
		return g_ModsCompatibility[folder];

	if (!g_Mods[folder])
		return false;

	for (let dependency of getMod(folder).dependencies)
	{
		if (!isDependencyMet(dependency))
			return false;
	}

	return true;
}

function recomputeCompatibility(disabledAction = false)
{
	for (let mod in g_Mods)
		g_ModsCompatibility[mod] = areDependenciesMet(mod, disabledAction);
}

/**
 * @param dependency is a mod name or a mod version comparison.
 */
function isDependencyMet(dependency)
{
	let operator = dependency.match(g_RegExpComparisonOperator);
	let [name, version] = operator ? dependency.split(operator[0]) : [dependency, undefined];

	return g_ModsEnabled.some(folder =>
		getMod(folder).name == name &&
		(!operator || versionSatisfied(getMod(folder).version, operator[0], version)));
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
		dependencies[folder] = getMod(folder).dependencies.map(d => d.split(g_RegExpComparisonOperator)[0]);

	g_ModsEnabled.sort((folder1, folder2) =>
		dependencies[folder1].indexOf(getMod(folder2).name) != -1 ? 1 :
			dependencies[folder2].indexOf(getMod(folder1).name) != -1 ? -1 : 0);

	g_ModsEnabledFiltered = displayModList("modsEnabledList", g_ModsEnabled, true);
}

function selectedMod(listObjectName)
{
	let listObject = Engine.GetGUIObjectByName(listObjectName);
	let isPickedDisabledList = listObjectName == "modsDisabledList";
	let otherListObject = Engine.GetGUIObjectByName(isPickedDisabledList ?
		"modsEnabledList" : "modsDisabledList");

	let toggleModButton = Engine.GetGUIObjectByName("toggleModButton");
	let isModSelected = listObject.selected != -1;
	if (isModSelected)
	{
		otherListObject.selected = -1;
		toggleModButton.onPress = isPickedDisabledList ? enableMod : disableMod;
	}

	const isFiltering = Engine.GetGUIObjectByName("modGenericFilter").caption;
	Engine.GetGUIObjectByName("visitWebButton").enabled = isModSelected && !!getSelectedModUrl();
	toggleModButton.caption = isPickedDisabledList ?
		translateWithContext("mod activation", "Enable") :
		translateWithContext("mod activation", "Disable");
	toggleModButton.enabled = isPickedDisabledList ? isModSelected && g_ModsCompatibility[listObject.list[listObject.selected]] || false : isModSelected;
	Engine.GetGUIObjectByName("enabledModUp").enabled = isModSelected && listObjectName == "modsEnabledList" && !isFiltering;
	Engine.GetGUIObjectByName("enabledModDown").enabled = isModSelected && listObjectName == "modsEnabledList" && !isFiltering;

	Engine.GetGUIObjectByName("globalModDescription").caption =
		listObject.list[listObject.selected] ?
			getMod(listObject.list[listObject.selected]).description :
			'[color="' + g_ColorNoModSelected + '"]' + translate("No mod has been selected.") + '[/color]';

	if (!g_ModsEnabled.length)
		Engine.GetGUIObjectByName("message").caption = coloredText(translate('Enable at least 0ad mod'), g_ColorDependenciesNotMet);
	if (!Engine.GetGUIObjectByName("startButton").hidden)
		Engine.GetGUIObjectByName("startButton").enabled = g_ModsEnabled.length > 0;

}

/**
 * @returns {string} The url of the currently selected mod.
 */
function getSelectedModUrl()
{
	let modsEnabledList = Engine.GetGUIObjectByName("modsEnabledList");
	let modsDisabledList = Engine.GetGUIObjectByName("modsDisabledList");

	let list = modsEnabledList.selected == -1 ? modsDisabledList : modsEnabledList;
	let folder = list.list[list.selected];
	return folder && getMod(folder) && getMod(folder).url || undefined;
}

function visitModWebsite()
{
	let url = getSelectedModUrl();
	if (!url)
		return;

	if (!url.startsWith("http://") && !url.startsWith("https://"))
		url = "http://" + url;

	openURL(url);
}
