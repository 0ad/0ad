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

// Whether status bars should be shown for all of the player's units.
var g_ShowAllStatusBars = false;

// Indicate when one of the current player's training queues is blocked
// (this is used to support population counter blinking)
var g_IsTrainingBlocked = false;

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

// Cache TechnologyData
var g_TechnologyData = {}; // {id:template}

function GetTechnologyData(technologyName)
{
	if (!(technologyName in g_TechnologyData))
	{
		var template = Engine.GuiInterfaceCall("GetTechnologyData", technologyName);
		g_TechnologyData[technologyName] = template;
	}

	return g_TechnologyData[technologyName];
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

		if (initData.savedGUIData)
		{
			restoreSavedGameData(initData.savedGUIData);
		}
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null);
	}

	// Cache civ data
	g_CivData = loadCivData();
	g_CivData["gaia"] = { "Code": "gaia", "Name": "Gaia" };

	getGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[Engine.GetPlayerID()].civ].Emblem;
	getGUIObjectByName("civIcon").tooltip = g_CivData[g_Players[Engine.GetPlayerID()].civ].Name;
	initMenuPosition(); // set initial position

	// If in Atlas editor, disable the exit button
	if (Engine.IsAtlasRunning())
		getGUIObjectByName("menuExitButton").enabled = false;

	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else
	{
		// Starting for the first time:
		var civMusic = g_CivData[g_Players[Engine.GetPlayerID()].civ].Music;
		initMusic();
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

function resignGame()
{
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// Players can't resign if they've already won or lost.	
	if (simState.players[Engine.GetPlayerID()].state != "active")
		return;

	// Tell other players that we have given up and been defeated
	Engine.PostNetworkCommand({
		"type": "defeat-player",
		"playerId": Engine.GetPlayerID()
	});

	global.music.setState(global.music.states.DEFEAT);
	resumeGame();
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

	var mapSettings = Engine.GetMapSettings();

	stopAmbient();
	endGame();

	Engine.SwitchGuiPage("page_summary.xml", {
							"gameResult"  : gameResult,
							"timeElapsed" : extendedSimState.timeElapsed,
							"playerStates": extendedSimState.players,
							"players": g_Players,
							"mapSettings": mapSettings
						 });
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}

// Return some data that will be stored in saved game files
function getSavedGameData()
{
	var data = {};
	data.playerAssignments = g_PlayerAssignments;
	data.groups = g_Groups.groups;
	// TODO: any other gui state?
	return data;
}

function restoreSavedGameData(data)
{
	// Restore control groups
	for (groupNumber in data.groups)
	{
		g_Groups.groups[groupNumber].groups = data.groups[groupNumber].groups;
		g_Groups.groups[groupNumber].ents = data.groups[groupNumber].ents;
	}
	updateGroups();
}

var lastTickTime = new Date;

/**
 * Called every frame.
 */
function onTick()
{
	var now = new Date;
	var tickLength = new Date - lastTickTime;
	lastTickTime = now;

	checkPlayerState();

	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;
		handleNetMessage(message);
	}

	updateCursorAndTooltip();

	// If the selection changed, we need to regenerate the sim display (the display depends on both the
	// simulation state and the current selection).
	if (g_Selection.dirty)
	{
		onSimulationUpdate();

		// Display rally points for selected buildings
		Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	// Run timers
	updateTimers();

	// Animate menu
	updateMenuPosition(tickLength);

	// Update music state
	global.music.updateTimer();

	// When training is blocked, flash population (alternates colour every 500msec)
	if (g_IsTrainingBlocked && (Date.now() % 1000) < 500)
		getGUIObjectByName("resourcePop").textcolor = POPULATION_ALERT_COLOR;
	else
		getGUIObjectByName("resourcePop").textcolor = DEFAULT_POPULATION_COLOR;

	// Clear renamed entities list
	Engine.GuiInterfaceCall("ClearRenamedEntities");
}

function checkPlayerState()
{
	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	var playerState = simState.players[Engine.GetPlayerID()];

	if (!g_GameEnded)
	{
		// If the game is about to end, disable the ability to resign.
		if (playerState.state != "active")
			getGUIObjectByName("menuResignButton").enabled = false;
		else
			return;

		if (playerState.state == "defeated")
		{
			g_GameEnded = true;
			// TODO: DEFEAT_CUE is missing?
			global.music.setState(global.music.states.DEFEAT);

			closeMenu();
			closeOpenDialogs();

			if (Engine.IsAtlasRunning())
			{
				// If we're in Atlas, we can't leave the game
				var btCaptions = ["OK"];
				var btCode = [null];
				var message = "Press OK to continue";
			}
			else
			{
				var btCaptions = ["Yes", "No"];
				var btCode = [leaveGame, null];
				var message = "Do you want to quit?";
			}
			messageBox(400, 200, message, "DEFEATED!", 0, btCaptions, btCode);
		}
		else if (playerState.state == "won")
		{
			g_GameEnded = true;
			global.music.setState(global.music.states.VICTORY);

			closeMenu();
			closeOpenDialogs();

			if (!getGUIObjectByName("devCommandsRevealMap").checked)
				getGUIObjectByName("devCommandsRevealMap").checked = true;

			if (Engine.IsAtlasRunning())
			{
				// If we're in Atlas, we can't leave the game
				var btCaptions = ["OK"];
				var btCode = [null];
				var message = "Press OK to continue";
			}
			else
			{
				var btCaptions = ["Yes", "No"];
				var btCode = [leaveGame, null];
				var message = "Do you want to quit?";
			}
			messageBox(400, 200, message, "VICTORIOUS!", 0, btCaptions, btCode);
		}
	}
}

/**
 * Recomputes GUI state that depends on simulation state or selection state. Called directly every simulation
 * update (see session.xml), or from onTick when the selection has changed.
 */
function onSimulationUpdate()
{
	g_Selection.dirty = false;
	g_EntityStates = {};
	g_TemplateData = {};
	g_TechnologyData = {};

	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!simState)
		return;

	handleNotifications();

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay();

	updateGroups();
	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails();
	updateResearchDisplay();
	updateBuildingPlacementPreview();
	updateTimeElapsedCounter(simState);

	// Update music state on basis of battle state.
	var battleState = Engine.GuiInterfaceCall("GetBattleState", Engine.GetPlayerID());
	if (battleState)
		global.music.setState(global.music.states[battleState]);
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
		button.onpress = (function(i) { return function() { performGroup((Engine.HotkeyIsPressed("selection.add") ? "add" : "select"), i); } })(i);
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

	g_IsTrainingBlocked = playerState.trainingBlocked;
}

function selectAndMoveTo(ent)
{
	var entState = GetEntityState(ent);
	if (!entState)
		return;

	g_Selection.reset();
	g_Selection.addList([ent]);

	var position = entState.position;
	Engine.CameraMoveTo(position.x, position.z);
}

function updateResearchDisplay()
{
	var researchStarted = Engine.GuiInterfaceCall("GetStartedResearch", Engine.GetPlayerID());
	if (!researchStarted)
		return;

	// Set up initial positioning.
	var buttonSideLength = getGUIObjectByName("researchStartedButton[0]").size.right;
	for (var i = 0; i < 10; ++i)
	{
		var button = getGUIObjectByName("researchStartedButton[" + i + "]");
		var size = button.size;
		size.top = (4 + buttonSideLength) * i;
		size.bottom = size.top + buttonSideLength;
		button.size = size;
	}

	var numButtons = 0;
	for (var tech in researchStarted)
	{
		// Show at most 10 in-progress techs.
		if (numButtons >= 10)
			break;

		var template = GetTechnologyData(tech);
		var button = getGUIObjectByName("researchStartedButton[" + numButtons + "]");
		button.hidden = false;
		button.tooltip = getEntityNames(template);
		button.onpress = (function(e) { return function() { selectAndMoveTo(e) } })(researchStarted[tech].researcher);

		var icon = "stretched:session/portraits/" + template.icon;
		getGUIObjectByName("researchStartedIcon[" + numButtons + "]").sprite = icon;

		// Scale the progress indicator.
		var size = getGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(researchStarted[tech].progress * (size.right - size.left));
		getGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size = size;

		++numButtons;
	}

	// Hide unused buttons.
	for (var i = numButtons; i < 10; ++i)
		getGUIObjectByName("researchStartedButton[" + i + "]").hidden = true;
}

function updateTimeElapsedCounter(simState)
{
	var timeElapsedCounter = getGUIObjectByName("timeElapsedCounter");
	timeElapsedCounter.caption = timeToString(simState.timeElapsed);
}

// Toggles the display of status bars for all of the player's entities.
function recalculateStatusBarDisplay()
{
	if (g_ShowAllStatusBars)
		var entities = Engine.PickFriendlyEntitiesOnScreen(Engine.GetPlayerID());
	else
	{
		var selected = g_Selection.toList();
		for each (var ent in g_Selection.highlighted)
			selected.push(ent);

		// Remove selected entities from the 'all entities' array, to avoid disabling their status bars.
		var entities = Engine.GuiInterfaceCall("GetPlayerEntities").filter(
				function(idx) { return (selected.indexOf(idx) == -1); }
		);
	}

	Engine.GuiInterfaceCall("SetStatusBars", { "entities": entities, "enabled": g_ShowAllStatusBars });
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
			currentAmbient = new AmbientSound(AMBIENT);

			if (currentAmbient)
			{
				currentAmbient.loop();
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
		currentAmbient.free();
		currentAmbient = null;
	}
}
