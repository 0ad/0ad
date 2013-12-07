// Network Mode
var g_IsNetworked = false;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;
// Match ID for tracking
var g_MatchID;

// Cache the basic player data (name, civ, color)
var g_Players = [];
// Cache the useful civ data
var g_CivData = {};

var g_GameSpeeds = {};
var g_CurrentSpeed;

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

// Cache simulation state (updated on every simulation update)
var g_SimState;

// Cache EntityStates
var g_EntityStates = {}; // {id:entState}

// Whether the player has lost/won and reached the end of their game
var g_GameEnded = false;

var g_Disconnected = false; // Lost connection to server

// Holds player states from the last tick
var g_CachedLastStates = "";

// Colors to flash when pop limit reached
const DEFAULT_POPULATION_COLOR = "white";
const POPULATION_ALERT_COLOR = "orange";

// List of additional entities to highlight
var g_ShowGuarding = false;
var g_ShowGuarded = false;
var g_AdditionalHighlight = [];

function GetSimState()
{
	if (!g_SimState)
	{
		g_SimState = Engine.GuiInterfaceCall("GetSimulationState");
	}

	return g_SimState;
}

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
		g_IsController = initData.isController; // Set controller mode
		g_PlayerAssignments = initData.playerAssignments;
		g_MatchID = initData.attribs.matchID;

		// Cache the player data
		// (This may be updated at runtime by handleNetMessage)
		g_Players = getPlayerData(g_PlayerAssignments);

		if (initData.savedGUIData)
			restoreSavedGameData(initData.savedGUIData);

		getGUIObjectByName("gameSpeedButton").hidden = g_IsNetworked;
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null);
	}

	// Cache civ data
	g_CivData = loadCivData();
	g_CivData["gaia"] = { "Code": "gaia", "Name": "Gaia" };

	g_GameSpeeds = initGameSpeeds();
	g_CurrentSpeed = Engine.GetSimRate();
	var gameSpeed = getGUIObjectByName("gameSpeed");
	gameSpeed.list = g_GameSpeeds.names;
	gameSpeed.list_data = g_GameSpeeds.speeds;
	var idx = g_GameSpeeds.speeds.indexOf(g_CurrentSpeed);
	gameSpeed.selected = idx != -1 ? idx : g_GameSpeeds["default"];
	gameSpeed.onSelectionChange = function() { changeGameSpeed(+this.list_data[this.selected]); }

	getGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[Engine.GetPlayerID()].civ].Emblem;
	getGUIObjectByName("civIcon").tooltip = g_CivData[g_Players[Engine.GetPlayerID()].civ].Name;
	initMenuPosition(); // set initial position

	// Populate player selection dropdown
	var playerNames = [];
	var playerIDs = [];
	for (var player in g_Players)
	{
		playerNames.push(g_Players[player].name);
		playerIDs.push(player);
	}

	var viewPlayerDropdown = getGUIObjectByName("viewPlayer");
	viewPlayerDropdown.list = playerNames;
	viewPlayerDropdown.list_data = playerIDs;
	viewPlayerDropdown.selected = Engine.GetPlayerID();

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

	if (Engine.ConfigDB_GetValue("user", "gui.session.timeelapsedcounter") === "true")
		getGUIObjectByName("timeElapsedCounter").hidden = false;

	onSimulationUpdate();

	// Report the performance after 5 seconds (when we're still near
	// the initial camera view) and a minute (when the profiler will
	// have settled down if framerates as very low), to give some
	// extremely rough indications of performance
	setTimeout(function() { reportPerformance(5); }, 5000);
	setTimeout(function() { reportPerformance(60); }, 60000);
}

function selectViewPlayer(playerID)
{
	Engine.SetPlayerID(playerID);
	if (playerID != 0) {
		getGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[playerID].civ].Emblem;
		getGUIObjectByName("civIcon").tooltip = g_CivData[g_Players[playerID].civ].Name;
	}
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
	var simState = GetSimState();

	// Players can't resign if they've already won or lost.
	if (simState.players[Engine.GetPlayerID()].state != "active" || g_Disconnected)
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
	var mapSettings = Engine.GetMapSettings();
	var gameResult;

	if (g_Disconnected)
	{
		gameResult = "You have been disconnected."
	}
	else if (playerState.state == "won")
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

	if (g_IsController && Engine.HasXmppClient())
		Engine.SendUnregisterGame();

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
	// Clear selection when loading a game
	g_Selection.reset();

	// Restore control groups
	for (var groupNumber in data.groups)
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
		g_Selection.dirty = false;

		onSimulationUpdate();

		// Display rally points for selected buildings
		Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	// Run timers
	updateTimers();

	// Animate menu
	updateMenuPosition(tickLength);

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
	// Once the game ends, we're done here.
	if (g_GameEnded)
		return;

	// Send a game report for each player in this game.
	var m_simState = GetSimState();
	var playerState = m_simState.players[Engine.GetPlayerID()];
	var tempStates = "";
	for each (var player in m_simState.players) {tempStates += player.state + ",";}

	if (g_CachedLastStates != tempStates)
	{
		g_CachedLastStates = tempStates;
		reportGame(Engine.GuiInterfaceCall("GetExtendedSimulationState"));
	}

	// If the local player hasn't finished playing, we return here to avoid the victory/defeat messages.
	if (playerState.state == "active")
		return;

	// We can't resign once the game is over.
	getGUIObjectByName("menuResignButton").enabled = false;

	// Make sure nothing is open to avoid stacking.
	closeMenu();
	closeOpenDialogs();

	// Make sure this doesn't run again.
	g_GameEnded = true;

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

	if (playerState.state == "defeated")
	{
		global.music.setState(global.music.states.DEFEAT);
		messageBox(400, 200, message, "DEFEATED!", 0, btCaptions, btCode);
	}
	else if (playerState.state == "won")
	{
		global.music.setState(global.music.states.VICTORY);
		// TODO: Reveal map directly instead of this silly proxy.
		if (!getGUIObjectByName("devCommandsRevealMap").checked)
			getGUIObjectByName("devCommandsRevealMap").checked = true;
		messageBox(400, 200, message, "VICTORIOUS!", 0, btCaptions, btCode);
	}

}

function changeGameSpeed(speed)
{
	// For non-networked games only
	if (!g_IsNetworked)
	{
		Engine.SetSimRate(speed);
		g_CurrentSpeed = speed;
	}
}

/**
 * Recomputes GUI state that depends on simulation state or selection state. Called directly every simulation
 * update (see session.xml), or from onTick when the selection has changed.
 */
function onSimulationUpdate()
{
	g_EntityStates = {};
	g_TemplateData = {};
	g_TechnologyData = {};

	g_SimState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!g_SimState)
		return;

	handleNotifications();

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay();

	if (g_ShowGuarding || g_ShowGuarded)
		updateAdditionalHighlight();

	updateHero();
	updateGroups();
	updateDebug();
	updatePlayerDisplay();
	updateSelectionDetails();
	updateResearchDisplay();
	updateBuildingPlacementPreview();
	updateTimeElapsedCounter();

	// Update music state on basis of battle state.
	var battleState = Engine.GuiInterfaceCall("GetBattleState", Engine.GetPlayerID());
	if (battleState)
		global.music.setState(global.music.states[battleState]);
}

function updateHero()
{
	var simState = GetSimState();
	var playerState = simState.players[Engine.GetPlayerID()];
	var heroButton = getGUIObjectByName("unitHeroButton");

	if (!playerState || playerState.heroes.length <= 0)
	{
		heroButton.hidden = true;
		return;
	}

	var heroImage = getGUIObjectByName("unitHeroImage");
	var heroState = GetEntityState(playerState.heroes[0]);
	var template = GetTemplateData(heroState.template);
	heroImage.sprite = "stretched:session/portraits/" + template.icon;
	var hero = playerState.heroes[0];

	heroButton.onpress = function()
	{
		if (!Engine.HotkeyIsPressed("selection.add"))
			g_Selection.reset();
		g_Selection.addList([hero]);
	};
	heroButton.ondoublepress = function() { selectAndMoveTo(getEntityOrHolder(hero)); };
	heroButton.hidden = false;

	// Setup tooltip
	var tooltip = "[font=\"serif-bold-16\"]" + template.name.specific + "[/font]";
	tooltip += "\n[font=\"serif-bold-13\"]Health:[/font] " + heroState.hitpoints + "/" + heroState.maxHitpoints;
	tooltip += "\n[font=\"serif-bold-13\"]" + (heroState.attack ? heroState.attack.type + " " : "")
	           + "Attack:[/font] " + damageTypeDetails(heroState.attack);
	// Show max attack range if ranged attack, also convert to tiles (4m per tile)
	if (heroState.attack && heroState.attack.type == "Ranged")
		tooltip += ", [font=\"serif-bold-13\"]Range:[/font] " + Math.round(heroState.attack.maxRange/4);

	tooltip += "\n[font=\"serif-bold-13\"]Armor:[/font] " + damageTypeDetails(heroState.armour);
	tooltip += "\n" + template.tooltip;

	heroButton.tooltip = tooltip;
};

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

function updateDebug()
{
	var simState = GetSimState();
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

function updatePlayerDisplay()
{
	var simState = GetSimState();
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
	if (!entState || !entState.position)
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

function updateTimeElapsedCounter()
{
	var simState = GetSimState();
	var speed = g_CurrentSpeed != 1.0 ? " (" + g_CurrentSpeed + "x)" : "";
	var timeElapsedCounter = getGUIObjectByName("timeElapsedCounter");
	timeElapsedCounter.caption = timeToString(simState.timeElapsed) + speed;
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

// Update the additional list of entities to be highlighted.
function updateAdditionalHighlight()
{
	var entsAdd = [];    // list of entities units to be highlighted
	var entsRemove = [];
	var highlighted = g_Selection.toList();
	for each (var ent in g_Selection.highlighted)
		highlighted.push(ent);

	if (g_ShowGuarding)
	{
		// flag the guarding entities to add in this additional highlight
		for each (var sel in g_Selection.selected)
		{
			var state = GetEntityState(sel);
			if (!state.guard || !state.guard.entities.length)
				continue;
			for each (var ent in state.guard.entities)
				if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1)
					entsAdd.push(ent);
		}
	}

	if (g_ShowGuarded)
	{
		// flag the guarded entities to add in this additional highlight
		for each (var sel in g_Selection.selected)
		{
			var state = GetEntityState(sel);
			if (!state.unitAI || !state.unitAI.isGuarding)
				continue;
			var ent = state.unitAI.isGuarding;
			if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1)
				entsAdd.push(ent);
		}
	}

	// flag the entities to remove (from the previously added) from this additional highlight
	for each (var ent in g_AdditionalHighlight)
	    if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1 && entsRemove.indexOf(ent) == -1)
			entsRemove.push(ent);

	_setHighlight(entsAdd   , HIGHLIGHTED_ALPHA, true );
	_setHighlight(entsRemove, 0                , false);
	g_AdditionalHighlight = entsAdd;
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
			Engine.PlayAmbientSound( AMBIENT, true );
			break;

		default:
			Engine.Console_Write("Unrecognized ambient type: " + type);
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
// Send a report on the game status to the lobby
function reportGame(extendedSimState)
{
	if (!Engine.HasXmppClient())
		return;

	// Resources gathered and used
	var playerFoodGatheredString = "";
	var playerWoodGatheredString  = "";
	var playerStoneGatheredString = "";
	var playerMetalGatheredString = "";
	var playerFoodUsedString = "";
	var playerWoodUsedString  = "";
	var playerStoneUsedString = "";
	var playerMetalUsedString = "";
	// Resources exchanged
	var playerFoodBoughtString = "";
	var playerWoodBoughtString = "";
	var playerStoneBoughtString = "";
	var playerMetalBoughtString = "";
	var playerFoodSoldString = "";
	var playerWoodSoldString = "";
	var playerStoneSoldString = "";
	var playerMetalSoldString = "";
	var playerTradeIncomeString = "";
	// Unit Stats
	var playerUnitsLostString = "";
	var playerUnitsTrainedString = "";
	var playerEnemyUnitsKilledString = "";
	// Building stats
	var playerBuildingsConstructedString = "";
	var playerBuildingsLostString = "";
	var playerEnemyBuildingsDestroyedString = "";
	var playerCivCentersBuiltString = "";
	var playerEnemyCivCentersDestroyedString = "";
	// Tribute
	var playerTributeSentString = "";
	var playerTributeReceivedString = "";
	// Various
	var mapName = Engine.GetMapSettings().Name;
	var playerStatesString = "";
	var playerCivsString = "";
	var playerPercentMapExploredString = "";
	var playerTreasuresCollectedString = "";

	// Serialize the statistics for each player into a comma-separated list.
	for each (var player in extendedSimState.players)
	{
		playerStatesString += player.state + ",";
		playerCivsString += player.civ + ",";
		playerFoodGatheredString += player.statistics.resourcesGathered.food + ",";
		playerWoodGatheredString += player.statistics.resourcesGathered.wood + ",";
		playerStoneGatheredString += player.statistics.resourcesGathered.stone + ",";
		playerMetalGatheredString += player.statistics.resourcesGathered.metal + ",";
		playerFoodUsedString += player.statistics.resourcesUsed.food + ",";
		playerWoodUsedString += player.statistics.resourcesUsed.wood + ",";
		playerStoneUsedString += player.statistics.resourcesUsed.stone + ",";
		playerMetalUsedString += player.statistics.resourcesUsed.metal + ",";
		playerUnitsLostString += player.statistics.unitsLost + ",";
		playerUnitsTrainedString += player.statistics.unitsTrained + ",";
		playerEnemyUnitsKilledString += player.statistics.enemyUnitsKilled + ",";
		playerBuildingsConstructedString += player.statistics.buildingsConstructed + ",";
		playerBuildingsLostString += player.statistics.buildingsLost + ",";
		playerEnemyBuildingsDestroyedString += player.statistics.enemyBuildingsDestroyed + ",";
		playerFoodBoughtString += player.statistics.resourcesBought.food + ",";
		playerWoodBoughtString += player.statistics.resourcesBought.wood + ",";
		playerStoneBoughtString += player.statistics.resourcesBought.stone + ",";
		playerMetalBoughtString += player.statistics.resourcesBought.metal + ",";
		playerFoodSoldString += player.statistics.resourcesSold.food + ",";
		playerWoodSoldString += player.statistics.resourcesSold.wood + ",";
		playerStoneSoldString += player.statistics.resourcesSold.stone + ",";
		playerMetalSoldString += player.statistics.resourcesSold.metal + ",";
		playerTributeSentString += player.statistics.tributesSent + ",";
		playerTributeReceivedString += player.statistics.tributesReceived + ",";
		playerPercentMapExploredString += player.statistics.precentMapExplored = ",";
		playerCivCentersBuiltString += player.statistics.civCentresBuilt + ",";
		playerEnemyCivCentersDestroyedString += player.statistics.enemyCivCentresDestroyed + ",";
		playerTreasuresCollectedString += player.statistics.treasuresCollected + ",";
		playerTradeIncomeString += player.statistics.tradeIncome + ",";
	}

	// Send the report with serialized data
	Engine.SendGameReport({
			"timeElapsed" : extendedSimState.timeElapsed,
			"playerStates" : playerStatesString,
			"playerID": Engine.GetPlayerID(),
			"matchID": g_MatchID,
			"civs" : playerCivsString,
			"mapName" : mapName,
			"foodGathered": playerFoodGatheredString,
			"woodGathered": playerWoodGatheredString,
			"stoneGathered": playerStoneGatheredString,
			"metalGathered": playerMetalGatheredString,
			"foodUsed": playerFoodUsedString,
			"woodUsed": playerWoodUsedString,
			"stoneUsed": playerStoneUsedString,
			"metalUsed": playerMetalUsedString,
			"unitsLost": playerUnitsLostString,
			"unitsTrained": playerUnitsTrainedString,
			"enemyUnitsKilled": playerEnemyUnitsKilledString,
			"buildingsLost": playerBuildingsLostString,
			"buildingsConstructed": playerBuildingsConstructedString,
			"enemyBuildingsDestroyed": playerEnemyBuildingsDestroyedString,
			"foodBought": playerFoodBoughtString,
			"woodBought": playerWoodBoughtString,
			"stoneBought": playerStoneBoughtString,
			"metalBought": playerMetalBoughtString,
			"foodSold": playerFoodSoldString,
			"woodSold": playerWoodSoldString,
			"stoneSold": playerStoneSoldString,
			"metalSold": playerMetalSoldString,
			"tributeSent": playerTributeSentString,
			"tributeReceived": playerTributeReceivedString,
			"precentMapExplored": playerPercentMapExploredString,
			"civCentersBuilt": playerCivCentersBuiltString,
			"enemyCivCentersDestroyed": playerEnemyCivCentersDestroyedString,
			"treasuresCollected": playerTreasuresCollectedString,
			"tradeIncome": playerTradeIncomeString
		});
}
