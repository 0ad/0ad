/**
 * Used for gameselection details.
 */
const g_VictoryConditions = g_Settings && g_Settings.VictoryConditions;

/**
 * Used for the gamelist-filtering.
 */
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);

/**
 * Used for the gamelist-filtering.
 */
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);

/**
 * Used for civ settings display of the selected game.
 */
const g_CivData = loadCivData(false, false);

/**
 * Current nickname.
 */
var g_Nickname = Engine.LobbyGetNick();

/**
 * This class organizes all components of this GUI page.
 */
var g_LobbyHandler;

/**
 * Called after the XmppConnection succeeded and when returning from a game.
 */
function init(attribs)
{
	if (g_Settings)
		g_LobbyHandler = new LobbyHandler(attribs && attribs.dialog);
	else
		error("Could not load settings");
}
