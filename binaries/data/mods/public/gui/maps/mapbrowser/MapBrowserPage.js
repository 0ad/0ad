/**
 * TODO: better global state handling in the GUI.
 * In particular a bunch of those shadow gamesetup stuff.
 */
const g_IsController = false;
const g_GameAttributes = {
	"mapType": "skirmish",
	"mapFilter": "default",
};
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
var g_SetupWindow;

function init()
{
	let cache = new MapCache();
	let filters = new MapFilters(cache);
	let browser = new MapBrowser(cache, filters);
	browser.registerClosePageHandler(() => Engine.PopGuiPage());
	browser.openPage();
}
