// Network Mode
var g_IsNetworked = false;

// Cache the basic player data (name, civ, color)
var g_Players = [];
// Cache the useful civ data
var g_CivData = {};

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

// Whether the player has lost/won and reached the end of their game
var g_GameEnded = false;

// Colors to flash when pop limit reached
const DEFAULT_POPULATION_COLOR = "white";
const POPULATION_ALERT_COLOR = "orange";

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
	if (initData)
	{
		g_IsNetworked = initData.isNetworked; // Set network mode
		g_PlayerAssignments = initData.playerAssignments;

		// Cache the player data
		// (This may be updated at runtime by handleNetMessage)
		g_Players = getPlayerData(g_PlayerAssignments);
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null);
	}

	// Cache civ data
	g_CivData = loadCivData();
	g_CivData["gaia"] = { "Code": "gaia", "Name": "Gaia" };

	getGUIObjectByName("civIcon").sprite = "stretched:"+g_CivData[g_Players[Engine.GetPlayerID()].civ].Emblem;
	initMenuPosition(); // set initial position

	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else
	{
		// Starting for the first time:
		var civMusic = g_CivData[g_Players[Engine.GetPlayerID()].civ].Music;
		global.music.storeTracks(civMusic);
		global.music.setState(global.music.states.PEACE);
		playRandomAmbient("temperate");
	}

	onSimulationUpdate();

	// Report the performance after 5 seconds (when we're still near
	// the initial camera view) and a minute (when the profiler will
	// have settled down if framerates as very low), to give some
	// extremely rough indications of performance
	setTimeout(function() { reportPerformance(5); }, 5000);
	setTimeout(function() { reportPerformance(60); }, 60000);
}


function reportPerformance(time)
{
	var settings = Engine.GetMapSettings();
	var data = {
		time: time,
		map: settings.Name,
		seed: settings.Seed, // only defined for random maps
		size: settings.Size, // only defined for random maps
		profiler: Engine.GetProfilerState()
	};

	Engine.SubmitUserReport("profile", 3, JSON.stringify(data));
}

function leaveGame()
{
	var extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	var playerState = extendedSimState.players[Engine.GetPlayerID()];

	var gameResult;
	if (playerState.state == "won")
	{
		gameResult = "You have won the battle!";
	}
	else if (playerState.state == "defeated")
	{
		gameResult = "You have been defeated...";
	}
	else // "active"
	{
		gameResult = "You have abandoned the game.";

		// Tell other players that we have given up and been defeated
		Engine.PostNetworkCommand({
			"type": "defeat-player",
			"playerId": Engine.GetPlayerID()
		});

		global.music.setState(global.music.states.DEFEAT);
	}

	stopAmbient();
	endGame();

	Engine.SwitchGuiPage("page_summary.xml", {
							"gameResult"  : gameResult,
							"timeElapsed" : extendedSimState.timeElapsed,
							"playerStates": extendedSimState.players
						 });
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}


function onTick()
{
	checkPlayerState();

	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;
		handleNetMessage(message);
	}

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

	// Animate menu
        updateMenuPosition();

	// Update music state
	global.music.updateTimer();

	// When training is blocked, flash population (alternates colour every 500msec)
	if (g_IsTrainingQueueBlocked && (Date.now() % 1000) < 500)
		getGUIObjectByName("resourcePop").textcolor = POPULATION_ALERT_COLOR;
	else
		getGUIObjectByName("resourcePop").textcolor = DEFAULT_POPULATION_COLOR;

	// Clear renamed entities list
	Engine.GuiInterfaceCall("ClearRenamedEntities", {});
}

function checkPlayerState()
{
	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	var playerState = simState.players[Engine.GetPlayerID()];

	if (!g_GameEnded)
	{
		if (playerState.state == "defeated")
		{
			g_GameEnded = true;
			global.music.setState(global.music.states.DEFEAT_CUE);

			closeMenu();
			closeOpenDialogs();

			var btCaptions = ["Yes", "No"];
			var btCode = [leaveGame, null];
			messageBox(400, 200, "You have been defeated...\nDo you want to leave the game now?", "Defeat", 0, btCaptions, btCode);
		}
		else if (playerState.state == "won")
		{
			g_GameEnded = true;
			global.music.setState(global.music.states.VICTORY);

			closeMenu();
			closeOpenDialogs();

			if (!getGUIObjectByName("devCommandsRevealMap").checked)
				getGUIObjectByName("devCommandsRevealMap").checked = true;

			var btCaptions = ["Yes", "No"];
			var btCode = [leaveGame, null];
			messageBox(400, 200, "You have won the battle!\nDo you want to leave the game now?", "Victory", 0, btCaptions, btCode);
		}
	}
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

	updateGroups();
	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails();
	updateBuildingPlacementPreview();
}

function updateGroups()
{
	var guiName = "Group";
	g_Groups.update();
	for (var i = 0; i < 10; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var label = getGUIObjectByName("unit"+guiName+"Label["+i+"]").caption = i;
		if (g_Groups.groups[i].getTotalCount() == 0)
			button.hidden = true;
		else
			button.hidden = false;
		button.onpress = (function(i) { return function() { performGroup("select", i); } })(i);
		button.ondoublepress = (function(i) { return function() { performGroup("snap", i); } })(i);
	}
	var numButtons = i;
	var rowLength = 1;
	var numRows = Math.ceil(numButtons / rowLength);
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;
	for (var i = 0; i < numRows; i++)
		layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );
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

	var conciseSimState = deepcopy(simState);
	conciseSimState.players = "<<<omitted>>>";
	var text = "simulation: " + uneval(conciseSimState);

	var selection = g_Selection.toList();
	if (selection.length)
	{
		var entState = GetEntityState(selection[0]);
		if (entState)
		{
			var template = GetTemplateData(entState.template);
			text += "\n\nentity: {\n";
			for (var k in entState)
				text += "  "+k+":"+uneval(entState[k])+"\n";
			text += "}\n\ntemplate: " + uneval(template);
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



// Temporarily adding this here
const AMBIENT_TEMPERATE = "temperate";
var currentAmbient;
function playRandomAmbient(type)
{
	switch (type)
	{
		case AMBIENT_TEMPERATE:
			// Seem to need the underscore at the end of "temperate" to avoid crash
			// (Might be caused by trying to randomly load day_temperate.xml)
//			currentAmbient = newRandomSound("ambient", "temperate_", "dayscape");

			const AMBIENT = "audio/ambient/dayscape/day_temperate_gen_03.ogg";
			currentAmbient = new Sound(AMBIENT);

			if (currentAmbient)
			{
				currentAmbient.loop();
				currentAmbient.setGain(0.8);
			}
			break;

		default:
			console.write("Unrecognized ambient type: " + type);
			break;
	}
}

// Temporarily adding this here
function stopAmbient()
{
	if (currentAmbient)
	{
		currentAmbient.fade(-1, 0.0, 5.0);
		currentAmbient = null;
	}
}