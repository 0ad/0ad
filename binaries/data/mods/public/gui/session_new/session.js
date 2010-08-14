const GEOLOGY = "geology";
const FLORA = "flora";
const FAUNA = "fauna";
const SPECIAL = "special";

const GAIA = "Gaia"
const CART = "Cart";
const CELT = "Celt";
const HELE = "Hele";
const IBER = "Iber";
const PERS = "Pers";
const ROME = "Rome";

const CARTHAGINIANS = "Carthaginians";
const ROMANS = "Romans";
const HELLENES = "Hellenes";
const CELTS = "Celts";
const PERSIANS = "Persians";
const IBERIANS = "Iberians";

// Network Mode
var g_IsNetworked = false;

// Cache the basic player data (name, civ, color)
var g_Players = [];
var g_PlayerAssignments = { "local": { "name": "You", "player": 1 } };
var g_PlayerID;

// Cache dev-mode settings that are frequently or widely used
var g_DevSettings = {
	controlAll: false
};

function init(initData, hotloadData)
{
	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else 
	{
		// Starting for the first time:
		startMusic();
	}

	if (initData)
	{
		g_IsNetworked = initData.isNetworked; // Set network mode
		g_PlayerAssignments = initData.playerAssignments;
		g_Players = getPlayerData(initData.playerAssignments); // Cache the player data
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null); 
	}

	cacheMenuObjects();

	onSimulationUpdate();
}

function leaveGame()
{
	stopMusic();
	endGame();
	Engine.SwitchGuiPage("page_pregame.xml");
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}

function onTick()
{
	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;
		handleNetMessage(message);
	}

	g_DevSettings.controlAll = getGUIObjectByName("devControlAll").checked;
	// TODO: at some point this controlAll needs to disable the simulation code's
	// player checks (once it has some player checks)

	updateCursor();

	// If the selection changed, we need to regenerate the sim display
	if (g_Selection.dirty)
	{
		onSimulationUpdate();

		// Display rally points for selected buildings
		Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	// Run timers
	updateTimers();
}

function onSimulationUpdate()
{
	g_Selection.dirty = false;
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!simState)
		return;

	handleNotifications();

	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails();
}

function updateDebug(simState)
{
	var debug = getGUIObjectByName("debug");

	if (getGUIObjectByName("devDisplayState").checked)
	{
		debug.hidden = false;
	}
	else
	{
		debug.hidden = true;
		return;
	}

	var text = uneval(simState);
	
	var selection = g_Selection.toList();
	if (selection.length)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
		if (entState)
		{
			var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
			text += "\n\n" + uneval(entState) + "\n\n" + uneval(template);
		}
	}

	debug.caption = text;
}

function updatePlayerDisplay(simState)
{
	var playerState = simState.players[Engine.GetPlayerID()];
	if (!playerState)
		return;

	getGUIObjectByName("resourceFood").caption = playerState.resourceCounts.food;
	getGUIObjectByName("resourceWood").caption = playerState.resourceCounts.wood;
	getGUIObjectByName("resourceStone").caption = playerState.resourceCounts.stone;
	getGUIObjectByName("resourceMetal").caption = playerState.resourceCounts.metal;
	getGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;
}
