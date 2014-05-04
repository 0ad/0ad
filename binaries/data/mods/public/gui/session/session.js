// Network Mode
var g_IsNetworked = false;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;
// Match ID for tracking
var g_MatchID;
// Is this user an observer?
var g_IsObserver = false;

// Cache the basic player data (name, civ, color)
var g_Players = [];
// Cache the useful civ data
var g_CivData = {};

var g_GameSpeeds = {};
var g_CurrentSpeed;

var g_PlayerAssignments = { "local": { "name": translate("You"), "player": 1 } };

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

// for saving the hitpoins of the hero (is there a better way to do that?) 
// Should be possible with AttackDetection but might be an overkill because it would have to loop
// always through the list of all ongoing attacks...
var g_previousHeroHitPoints = undefined;

function GetSimState()
{
	if (!g_SimState)
		g_SimState = Engine.GuiInterfaceCall("GetSimulationState");

	return g_SimState;
}

function GetEntityState(entId)
{
	if (!(entId in g_EntityStates))
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", entId);
		if (entState)
			entState.extended = false;
		g_EntityStates[entId] = entState;
	}

	return g_EntityStates[entId];
}

function GetExtendedEntityState(entId)
{
	if (entId in g_EntityStates)
		var entState = g_EntityStates[entId];
	else
		var entState = Engine.GuiInterfaceCall("GetEntityState", entId);

	if (!entState || entState.extended)
		return entState;

	var extension = Engine.GuiInterfaceCall("GetExtendedEntityState", entId);
	for (var prop in extension)
		entState[prop] = extension[prop];
	entState.extended = true;
	g_EntityStates[entId] = entState;

	return entState;
}

// Cache TemplateData
var g_TemplateData = {}; // {id:template}
var g_TemplateDataWithoutLocalization = {};


function GetTemplateData(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		var template = Engine.GuiInterfaceCall("GetTemplateData", templateName);
		translateObjectKeys(template, ["specific", "generic", "tooltip"]);
		g_TemplateData[templateName] = template;
	}

	return g_TemplateData[templateName];
}

function GetTemplateDataWithoutLocalization(templateName)
{
	if (!(templateName in g_TemplateDataWithoutLocalization))
	{
		var template = Engine.GuiInterfaceCall("GetTemplateData", templateName);
		g_TemplateDataWithoutLocalization[templateName] = template;
	}

	return g_TemplateDataWithoutLocalization[templateName];
}

// Cache TechnologyData
var g_TechnologyData = {}; // {id:template}

function GetTechnologyData(technologyName)
{
	if (!(technologyName in g_TechnologyData))
	{
		var template = Engine.GuiInterfaceCall("GetTechnologyData", technologyName);
		translateObjectKeys(template, ["specific", "generic", "description", "tooltip", "requirementsTooltip"]);
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

		Engine.GetGUIObjectByName("gameSpeedButton").hidden = g_IsNetworked;
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null);
	}

	// Cache civ data
	g_CivData = loadCivData();
	g_CivData["gaia"] = { "Code": "gaia", "Name": translate("Gaia") };

	if (Engine.GetPlayerID() <= 0)
	{
		g_IsObserver = true;
		// Hide stuff observers don't use.
		Engine.GetGUIObjectByName("food").hidden = true;
		Engine.GetGUIObjectByName("wood").hidden = true;
		Engine.GetGUIObjectByName("stone").hidden = true;
		Engine.GetGUIObjectByName("metal").hidden = true;
		Engine.GetGUIObjectByName("population").hidden = true;
		Engine.GetGUIObjectByName("diplomacyButton1").hidden = true;
		Engine.GetGUIObjectByName("tradeButton1").hidden = true;
		Engine.GetGUIObjectByName("menuResignButton").enabled = false;
		Engine.GetGUIObjectByName("pauseButton").enabled = false;
		Engine.GetGUIObjectByName("observerText").hidden = false;
	}
	else
	{
		// TODO: Get a civ icon for gaia/observers.
		Engine.GetGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[Engine.GetPlayerID()].civ].Emblem;
		Engine.GetGUIObjectByName("civIcon").tooltip = g_CivData[g_Players[Engine.GetPlayerID()].civ].Name;
	}

	g_GameSpeeds = initGameSpeeds();
	g_CurrentSpeed = Engine.GetSimRate();
	var gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.list = g_GameSpeeds.names;
	gameSpeed.list_data = g_GameSpeeds.speeds;
	var idx = g_GameSpeeds.speeds.indexOf(g_CurrentSpeed);
	gameSpeed.selected = idx != -1 ? idx : g_GameSpeeds["default"];
	gameSpeed.onSelectionChange = function() { changeGameSpeed(+this.list_data[this.selected]); }
	initMenuPosition(); // set initial position

	// Populate player selection dropdown
	var playerNames = [];
	var playerIDs = [];
	for (var player in g_Players)
	{
		playerNames.push(g_Players[player].name);
		playerIDs.push(player);
	}

	var viewPlayerDropdown = Engine.GetGUIObjectByName("viewPlayer");
	viewPlayerDropdown.list = playerNames;
	viewPlayerDropdown.list_data = playerIDs;
	viewPlayerDropdown.selected = Engine.GetPlayerID();

	// If in Atlas editor, disable the exit button
	if (Engine.IsAtlasRunning())
		Engine.GetGUIObjectByName("menuExitButton").enabled = false;

	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else
	{
		// Starting for the first time:
		initMusic();
		if (!g_IsObserver){
			var civMusic = g_CivData[g_Players[Engine.GetPlayerID()].civ].Music;
			global.music.storeTracks(civMusic);
		}
		global.music.setState(global.music.states.PEACE);
		playRandomAmbient("temperate");
	}

	if (Engine.ConfigDB_GetValue("user", "gui.session.timeelapsedcounter") === "true")
		Engine.GetGUIObjectByName("timeElapsedCounter").hidden = false;

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
	if (playerID > 0) {
		Engine.GetGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[playerID].civ].Emblem;
		Engine.GetGUIObjectByName("civIcon").tooltip = g_CivData[g_Players[playerID].civ].Name;
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

/**
 * Resign a player.
 * @param leaveGameAfterResign If player is quitting after resignation.
 */
function resignGame(leaveGameAfterResign)
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
	
	// Resume the game if not resigning.
	if (!leaveGameAfterResign)
		resumeGame();
}

/**
 * Leave the game
 * @param willRejoin If player is going to be rejoining a networked game.
 */
function leaveGame(willRejoin)
{
	var extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	var mapSettings = Engine.GetMapSettings();
	var gameResult;

	if (g_IsObserver)
	{
		// Observers don't win/lose.
		gameResult = translate("You have left the game.");
		global.music.setState(global.music.states.VICTORY);
	}
	else
	{
		var playerState = extendedSimState.players[Engine.GetPlayerID()];
		if (g_Disconnected)
			gameResult = translate("You have been disconnected.");
		else if (playerState.state == "won")
			gameResult = translate("You have won the battle!");
		else if (playerState.state == "defeated")
			gameResult = translate("You have been defeated...");
		else // "active"
		{
			global.music.setState(global.music.states.DEFEAT);
			if (willRejoin)
				gameResult = translate("You have left the game.");
			else
			{
				gameResult = translate("You have abandoned the game.");
				resignGame(true);
			}
		}
	}

	stopAmbient();
	Engine.EndGame();

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
	// Restore camera if any
	if (data.camera)
		Engine.SetCameraData(data.camera.PosX, data.camera.PosY, data.camera.PosZ,
			data.camera.RotX, data.camera.RotY, data.camera.Zoom);

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
var lastXmppClientPoll = Date.now();

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
		if (!g_IsObserver)
			Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	// Run timers
	updateTimers();

	// Animate menu
	updateMenuPosition(tickLength);

	// When training is blocked, flash population (alternates colour every 500msec)
	if (g_IsTrainingBlocked && (Date.now() % 1000) < 500)
		Engine.GetGUIObjectByName("resourcePop").textcolor = POPULATION_ALERT_COLOR;
	else
		Engine.GetGUIObjectByName("resourcePop").textcolor = DEFAULT_POPULATION_COLOR;

	// Clear renamed entities list
	Engine.GuiInterfaceCall("ClearRenamedEntities");
}

function checkPlayerState()
{
	// Once the game ends, we're done here.
	if (g_GameEnded || g_IsObserver)
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
	Engine.GetGUIObjectByName("menuResignButton").enabled = false;

	// Make sure nothing is open to avoid stacking.
	closeMenu();
	closeOpenDialogs();

	// Make sure this doesn't run again.
	g_GameEnded = true;

	if (Engine.IsAtlasRunning())
	{
		// If we're in Atlas, we can't leave the game
		var btCaptions = [translate("OK")];
		var btCode = [null];
		var message = translate("Press OK to continue");
	}
	else
	{
		var btCaptions = [translate("No"), translate("Yes")];
		var btCode = [null, leaveGame];
		var message = translate("Do you want to quit?");
	}

	if (playerState.state == "defeated")
	{
		global.music.setState(global.music.states.DEFEAT);
		messageBox(400, 200, message, translate("DEFEATED!"), 0, btCaptions, btCode);
	}
	else if (playerState.state == "won")
	{
		global.music.setState(global.music.states.VICTORY);
		// TODO: Reveal map directly instead of this silly proxy.
		if (!Engine.GetGUIObjectByName("devCommandsRevealMap").checked)
			Engine.GetGUIObjectByName("devCommandsRevealMap").checked = true;
		messageBox(400, 200, message, translate("VICTORIOUS!"), 0, btCaptions, btCode);
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
	updateBuildingPlacementPreview();
	updateTimeElapsedCounter();
	updateTimeNotifications();

	if (!g_IsObserver)
	{
		updateResearchDisplay();
		// Update music state on basis of battle state.
		var battleState = Engine.GuiInterfaceCall("GetBattleState", Engine.GetPlayerID());
		if (battleState)
			global.music.setState(global.music.states[battleState]);
	}
}

/**
* updates a status bar on the GUI
* nameOfBar: name of the bar
* points: points to show
* maxPoints: max points 
* direction: gets less from (right to left) 0; (top to bottom) 1; (left to right) 2; (bottom to top) 3;
*/
function updateGUIStatusBar(nameOfBar, points, maxPoints, direction) 
{
	// check, if optional direction parameter is valid.
	if (!direction || !(direction >= 0 && direction < 4))
		direction = 0;

	// get the bar and update it
	var statusBar = Engine.GetGUIObjectByName(nameOfBar);
	if (!statusBar) 
		return;
		
	var healthSize = statusBar.size;
	var value = 100*Math.max(0, Math.min(1, points / maxPoints));
	
	// inverse bar
	if(direction == 2 || direction == 3) 
		value = 100 - value;

	if(direction == 0)
		healthSize.rright = value;
	else if(direction == 1)
		healthSize.rbottom = value;
	else if(direction == 2)
		healthSize.rleft = value;
	else if(direction == 3)
		healthSize.rtop = value;
	
	// update bar
	statusBar.size = healthSize;
}


function updateHero()
{
	var simState = GetSimState();
	var playerState = simState.players[Engine.GetPlayerID()];
	var unitHeroPanel = Engine.GetGUIObjectByName("unitHeroPanel");
	var heroButton = Engine.GetGUIObjectByName("unitHeroButton");

	if (!playerState || playerState.heroes.length <= 0)
	{
		g_previousHeroHitPoints = undefined;
		unitHeroPanel.hidden = true;
		return;
	}

	var heroImage = Engine.GetGUIObjectByName("unitHeroImage");
	var heroState = GetExtendedEntityState(playerState.heroes[0]);
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
	unitHeroPanel.hidden = false;

	// Setup tooltip
	var tooltip = "[font=\"sans-bold-16\"]" + template.name.specific + "[/font]";
	var healthLabel = "[font=\"sans-bold-13\"]" + translate("Health:") + "[/font]";
	tooltip += "\n" + sprintf(translate("%(label)s %(current)s / %(max)s"), { label: healthLabel, current: heroState.hitpoints, max: heroState.maxHitpoints });
	if (heroState.attack)
	{
		var attackLabel = "[font=\"sans-bold-13\"]" + getAttackTypeLabel(heroState.attack.type) + "[/font]";
		if (heroState.attack.type == "Ranged")
			// Show max attack range if ranged attack, also convert to tiles (4m per tile)
			tooltip += "\n" + sprintf(
				translate("%(attackLabel)s %(details)s, %(rangeLabel)s %(range)s"),
				{
					attackLabel: attackLabel,
					details: damageTypeDetails(heroState.attack),
					rangeLabel: "[font=\"sans-bold-13\"]" + translate("Range:") + "[/font]",
					range: Math.round(heroState.attack.maxRange) + " [font=\"sans-10\"][color=\"orange\"]" + translate("meters") + "[/color][/font]",
				}
			);
		else
			tooltip += "\n" + sprintf(translate("%(label)s %(details)s"), { label: attackLabel, details: damageTypeDetails(heroState.attack) });
	}

	var armorLabel = "[font=\"sans-bold-13\"]" + translate("Armor:") + "[/font]";
	tooltip += "\n" + sprintf(translate("%(label)s %(details)s"), { label: armorLabel, details: damageTypeDetails(heroState.armour) });
	tooltip += "\n" + template.tooltip;

	heroButton.tooltip = tooltip;
	
	// update heros health bar
	updateGUIStatusBar("heroHealthBar", heroState.hitpoints, heroState.maxHitpoints);
	
	// define the hit points if not defined
	if (!g_previousHeroHitPoints)
		g_previousHeroHitPoints = heroState.hitpoints;
	
	// check, if the health of the hero changed since the last update
	if (heroState.hitpoints < g_previousHeroHitPoints)
	{	
		g_previousHeroHitPoints = heroState.hitpoints;
		// trigger the animation
		startColorFade("heroHitOverlay", 100, 0, colorFade_attackUnit, true, smoothColorFadeRestart_attackUnit);
		return;
	}
}


function updateGroups()
{
	var guiName = "Group";
	g_Groups.update();
	for (var i = 0; i < 10; i++)
	{
		var button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var label = Engine.GetGUIObjectByName("unit"+guiName+"Label["+i+"]").caption = i;
		if (g_Groups.groups[i].getTotalCount() == 0)
			button.hidden = true;
		else
			button.hidden = false;
		button.onpress = (function(i) { return function() { performGroup((Engine.HotkeyIsPressed("selection.add") ? "add" : "select"), i); } })(i);
		button.ondoublepress = (function(i) { return function() { performGroup("snap", i); } })(i);
		button.onpressright = (function(i) { return function() { performGroup("breakUp", i); } })(i);
	}
	var numButtons = i;
	var rowLength = 1;
	var numRows = Math.ceil(numButtons / rowLength);
	var buttonSideLength = Engine.GetGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;
	for (var i = 0; i < numRows; i++)
		layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );
}

function updateDebug()
{
	var simState = GetSimState();
	var debug = Engine.GetGUIObjectByName("debug");

	if (Engine.GetGUIObjectByName("devDisplayState").checked)
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
		var entState = GetExtendedEntityState(selection[0]);
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

	Engine.GetGUIObjectByName("resourceFood").caption = Math.floor(playerState.resourceCounts.food);
	Engine.GetGUIObjectByName("resourceWood").caption = Math.floor(playerState.resourceCounts.wood);
	Engine.GetGUIObjectByName("resourceStone").caption = Math.floor(playerState.resourceCounts.stone);
	Engine.GetGUIObjectByName("resourceMetal").caption = Math.floor(playerState.resourceCounts.metal);
	Engine.GetGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;

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
	var buttonSideLength = Engine.GetGUIObjectByName("researchStartedButton[0]").size.right;
	for (var i = 0; i < 10; ++i)
	{
		var button = Engine.GetGUIObjectByName("researchStartedButton[" + i + "]");
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
		var button = Engine.GetGUIObjectByName("researchStartedButton[" + numButtons + "]");
		button.hidden = false;
		button.tooltip = getEntityNames(template);
		button.onpress = (function(e) { return function() { selectAndMoveTo(e) } })(researchStarted[tech].researcher);

		var icon = "stretched:session/portraits/" + template.icon;
		Engine.GetGUIObjectByName("researchStartedIcon[" + numButtons + "]").sprite = icon;

		// Scale the progress indicator.
		var size = Engine.GetGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(researchStarted[tech].progress * (size.right - size.left));
		Engine.GetGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size = size;

		++numButtons;
	}

	// Hide unused buttons.
	for (var i = numButtons; i < 10; ++i)
		Engine.GetGUIObjectByName("researchStartedButton[" + i + "]").hidden = true;
}

function updateTimeElapsedCounter()
{
	var simState = GetSimState();
	var timeElapsedCounter = Engine.GetGUIObjectByName("timeElapsedCounter");
	if (g_CurrentSpeed != 1.0)
		timeElapsedCounter.caption = sprintf(translate("%(time)s (%(speed)sx)"), { time: timeToString(simState.timeElapsed), speed: Engine.FormatDecimalNumberIntoString(g_CurrentSpeed) });
	else
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
			Engine.Console_Write(sprintf(translate("Unrecognized ambient type: %(ambientType)s"), { ambientType: type }));
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

function getBuildString()
{
	return sprintf(translate("Build: %(buildDate)s (%(revision)s)"), { buildDate: Engine.GetBuildTimestamp(0), revision: Engine.GetBuildTimestamp(2) });
}

function showTimeWarpMessageBox()
{
	messageBox(500, 250, translate("Note: time warp mode is a developer option, and not intended for use over long periods of time. Using it incorrectly may cause the game to run out of memory or crash."), translate("Time warp mode"), 2);
}

// Send a report on the game status to the lobby
function reportGame(extendedSimState)
{
	if (!Engine.HasXmppClient() || !Engine.IsRankedGame())
		return;
	// units
	var unitsClasses = [
		"total",
		"Infantry",
		"Worker",
		"Female",
		"Cavalry",
		"Champion",
		"Hero",
		"Ship"
	];
	var unitsCountersTypes = [
		"unitsTrained",
		"unitsLost",
		"enemyUnitsKilled"
	];
	// buildings
	var buildingsClasses = [
		"total",
		"CivCentre",
		"House",
		"Economic",
		"Outpost",
		"Military",
		"Fortress",
		"Wonder"
	];
	var buildingsCountersTypes = [
		"buildingsConstructed",
		"buildingsLost",
		"enemyBuildingsDestroyed"
	];
	// resources
	var resourcesTypes = [
		"wood",
		"food",
		"stone",
		"metal"
	];
	var resourcesCounterTypes = [
		"resourcesGathered",
		"resourcesUsed",
		"resourcesSold",
		"resourcesBought"
	];

	var playerStatistics = { };

	// Unit Stats
	for each (var unitCounterType in unitsCountersTypes)
	{
		if (!playerStatistics[unitCounterType])
			playerStatistics[unitCounterType] = { };
		for each (var unitsClass in unitsClasses)
			playerStatistics[unitCounterType][unitsClass] = "";
	}

	playerStatistics.unitsLostValue = "";
	playerStatistics.unitsKilledValue = "";
	// Building stats
	for each (var buildingCounterType in buildingsCountersTypes)
	{
		if (!playerStatistics[buildingCounterType])
			playerStatistics[buildingCounterType] = { };
		for each (var buildingsClass in buildingsClasses)
			playerStatistics[buildingCounterType][buildingsClass] = "";
	}

	playerStatistics.buildingsLostValue = "";
	playerStatistics.enemyBuildingsDestroyedValue = "";
	// Resources
	for each (var resourcesCounterType in resourcesCounterTypes)
	{
		if (!playerStatistics[resourcesCounterType])
			playerStatistics[resourcesCounterType] = { };
		for each (var resourcesType in resourcesTypes)
			playerStatistics[resourcesCounterType][resourcesType] = "";
	}
	playerStatistics.resourcesGathered.vegetarianFood = "";

	playerStatistics.tradeIncome = "";
	// Tribute
	playerStatistics.tributesSent = "";
	playerStatistics.tributesReceived = "";
	// Total	
	playerStatistics.economyScore = "";
	playerStatistics.militaryScore = "";
	playerStatistics.totalScore = "";
	// Various
	playerStatistics.treasuresCollected = "";
	playerStatistics.feminisation = "";
	playerStatistics.percentMapExplored = "";
	var mapName = Engine.GetMapSettings().Name;
	var playerStates = "";
	var playerCivs = "";
	var teams = "";
	var teamsLocked = true;

	// Serialize the statistics for each player into a comma-separated list.
	for each (var player in extendedSimState.players)
	{
		playerStates += player.state + ",";
		playerCivs += player.civ + ",";
		teams += player.team + ",";
		teamsLocked = teamsLocked && player.teamsLocked;
		for each (var resourcesCounterType in resourcesCounterTypes)
			for each (var resourcesType in resourcesTypes)
				playerStatistics[resourcesCounterType][resourcesType] += player.statistics[resourcesCounterType][resourcesType] + ",";
		playerStatistics.resourcesGathered.vegetarianFood += player.statistics.resourcesGathered.vegetarianFood + ",";

		for each (var unitCounterType in unitsCountersTypes)
			for each (var unitsClass in unitsClasses)
				playerStatistics[unitCounterType][unitsClass] += player.statistics[unitCounterType][unitsClass] + ",";

		for each (var buildingCounterType in buildingsCountersTypes)
			for each (var buildingsClass in buildingsClasses)
				playerStatistics[buildingCounterType][buildingsClass] += player.statistics[buildingCounterType][buildingsClass] + ",";
		var total = 0;
		for each (var res in player.statistics.resourcesGathered)
			total += res;
		playerStatistics.economyScore += total + ",";
		playerStatistics.militaryScore += Math.round((player.statistics.enemyUnitsKilledValue +
			player.statistics.enemyBuildingsDestroyedValue) / 10)  + ",";
		playerStatistics.totalScore += (total + Math.round((player.statistics.enemyUnitsKilledValue +
			player.statistics.enemyBuildingsDestroyedValue) / 10)) + ",";
		playerStatistics.tradeIncome += player.statistics.tradeIncome + ",";
		playerStatistics.tributesSent += player.statistics.tributesSent + ",";
		playerStatistics.tributesReceived += player.statistics.tributesReceived + ",";
		playerStatistics.percentMapExplored += player.statistics.percentMapExplored + ",";
		playerStatistics.treasuresCollected += player.statistics.treasuresCollected + ",";
	}

	// Send the report with serialized data
	var reportObject = { };
	reportObject.timeElapsed = extendedSimState.timeElapsed;
	reportObject.playerStates = playerStates;
	reportObject.playerID = Engine.GetPlayerID();
	reportObject.matchID = g_MatchID;
	reportObject.civs = playerCivs;
	reportObject.teams = teams;
	reportObject.teamsLocked = String(teamsLocked);
	reportObject.mapName = mapName;
	reportObject.economyScore = playerStatistics.economyScore;
	reportObject.militaryScore = playerStatistics.militaryScore;
	reportObject.totalScore = playerStatistics.totalScore;
	for each (var rct in resourcesCounterTypes)
	{
		for each (var rt in resourcesTypes)
			reportObject[rt+rct.substr(9)] = playerStatistics[rct][rt];
			// eg. rt = food rct.substr = Gathered rct = resourcesGathered
	}
	reportObject.vegetarianFoodGathered = playerStatistics.resourcesGathered.vegetarianFood;
	for each (var type in unitsClasses)
	{
		// eg. type = Infantry (type.substr(0,1)).toLowerCase()+type.substr(1) = infantry
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"UnitsTrained"] = playerStatistics.unitsTrained[type];
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"UnitsLost"] = playerStatistics.unitsLost[type];
		reportObject["enemy"+type+"UnitsKilled"] = playerStatistics.enemyUnitsKilled[type];
	}
	for each (var type in buildingsClasses)
	{
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"BuildingsConstructed"] = playerStatistics.buildingsConstructed[type];
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"BuildingsLost"] = playerStatistics.buildingsLost[type];
		reportObject["enemy"+type+"BuildingsDestroyed"] = playerStatistics.enemyBuildingsDestroyed[type];
	}
	reportObject.tributesSent = playerStatistics.tributesSent;
	reportObject.tributesReceived = playerStatistics.tributesReceived;
	reportObject.percentMapExplored = playerStatistics.percentMapExplored;
	reportObject.treasuresCollected = playerStatistics.treasuresCollected;
	reportObject.tradeIncome = playerStatistics.tradeIncome;

	Engine.SendGameReport(reportObject);
}

