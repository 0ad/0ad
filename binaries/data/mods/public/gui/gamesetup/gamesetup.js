// TODO: Remove these globals by rewriting gamedescription.js
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
const g_PopulationCapacities = prepareForDropdown(g_Settings && g_Settings.PopulationCapacities);
const g_StartingResources = prepareForDropdown(g_Settings && g_Settings.StartingResources);
const g_VictoryConditions = g_Settings && g_Settings.VictoryConditions;

/**
 * Offer users to select playable civs only.
 * Load unselectable civs as they could appear in scenario maps.
 */
const g_CivData = loadCivData(false, false);

/**
 * Whether this is a single- or multiplayer match.
 */
const g_IsNetworked = Engine.HasNetClient();

/**
 * Is this user in control of game settings (i.e. is a network server, or offline player).
 */
const g_IsController = !g_IsNetworked || Engine.HasNetServer();

/**
 * Central data storing all settings relevant to the map generation and simulation.
 */
var g_GameAttributes = {};

/**
 * Remembers which clients are assigned to which player slots and whether they are ready.
 * The keys are GUIDs or "local" in single-player.
 */
var g_PlayerAssignments = {};

/**
 * This instance owns all handlers that control the two synchronized states g_GameAttributes and g_PlayerAssignments.
 */
var g_SetupWindow;

// TODO: Remove these two global functions by specifying the JS class name in the XML of the GUI page.

function init(initData, hotloadData)
{
	g_SetupWindow = new SetupWindow(initData, hotloadData);
}

function getHotloadData()
{
	return g_SetupWindow.getHotloadData();
}
