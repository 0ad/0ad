// Network Mode
var g_IsNetworked = false;

// Cache the basic player data (name, civ, color)
var g_Players = [];
var g_PlayerAssignments = { "local": { "name": "You", "player": 1 } };

// Cache dev-mode settings that are frequently or widely used
var g_DevSettings = {
	controlAll: false
};

// Indicate when one of the current player's training queues is blocked
// (this is used to support population counter blinking)
var g_IsTrainingQueueBlocked = false;

// Cache EntityStates
var g_EntityStates = {}; // {id:entState}

function GetEntityState(entId)
{
	if (!(entId in g_EntityStates))
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", entId);
		g_EntityStates[entId] = entState;
	}

	return g_EntityStates[entId];
}

// Cache TemplateData
var g_TemplateData = {}; // {id:template}

function GetTemplateData(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		var template = Engine.GuiInterfaceCall("GetTemplateData", templateName);
		g_TemplateData[templateName] = template;
	}

	return g_TemplateData[templateName];
}

// Init
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

	// When training is blocked, flash population (alternates colour every 500msec)
	if (g_IsTrainingQueueBlocked && (Date.now() % 1000) < 500)
		getGUIObjectByName("populationWarning").hidden = false;
	else
		getGUIObjectByName("populationWarning").hidden = true;
	
	
	
	
	/*
	// When training is blocked, flash population (alternates colour every 500msec)
	if (g_IsTrainingQueueBlocked && (Date.now() % 1000) < 500)
		getGUIObjectByName("resourcePop").textcolor = "255 0 0";
	else
		getGUIObjectByName("resourcePop").textcolor = "white";
	*/
}

function onSimulationUpdate()
{
	g_Selection.dirty = false;
	g_EntityStates = {};
	g_TemplateData = {};
	
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
		var entState = GetEntityState(selection[0]);
		if (entState)
		{
			var template = GetTemplateData(entState.template);
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
	
	g_IsTrainingQueueBlocked = playerState.trainingQueueBlocked;
}
