/**
 * TODO: better global state handling in the GUI.
 * In particular a bunch of those shadow gamesetup/gamesettings stuff.
 */
const g_IsController = false;
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
var g_SetupWindow;

function init()
{
	let cache = new MapCache();
	let filters = new MapFilters(cache);
	let browser = new MapBrowser(cache, filters);
	browser.registerClosePageHandler(() => Engine.PopGuiPage());
	browser.openPage();
	browser.controls.MapFiltering.select("default", "skirmish");
}
