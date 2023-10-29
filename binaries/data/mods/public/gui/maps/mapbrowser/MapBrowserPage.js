/**
 * TODO: better global state handling in the GUI.
 */
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);

function init()
{
	let cache = new MapCache();
	let filters = new MapFilters(cache);
	let browser = new MapBrowser(cache, filters);
	browser.registerClosePageHandler(() => Engine.PopGuiPage());
	browser.openPage(false);
	browser.controls.MapFiltering.select("default", "skirmish");
}
