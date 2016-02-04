const g_IsReplay = Engine.IsVisualReplay();
const g_GameSpeeds = prepareForDropdown(g_Settings ? g_Settings.GameSpeeds.filter(speed => !speed.ReplayOnly || g_IsReplay) : undefined);

/**
 * Colors to flash when pop limit reached.
 */
const g_DefaultPopulationColor = "white";
const g_PopulationAlertColor = "orange";

/**
 * A random file will be played. TODO: more variety
 */
const g_Ambient = [ "audio/ambient/dayscape/day_temperate_gen_03.ogg" ];

/**
 * Is this user in control of game settings (i.e. is a network server, or offline player).
 */
var g_IsController;

/**
 * True if this is a multiplayer game.
 */
var g_IsNetworked = false;

/**
 * True if the connection to the server has been lost.
 */
var g_Disconnected = false;

/**
 * True if the current user has observer capabilities.
 */
var g_IsObserver = false;

/**
 * The playerID selected in the change perspective tool.
 */
var g_ViewedPlayer = Engine.GetPlayerID();

/**
 * Unique ID for lobby reports.
 */
var g_MatchID;

/**
 * Cache the basic player data (name, civ, color).
 */
var g_Players = [];

/**
 * Last time when onTick was called().
 * Used for animating the main menu.
 */
var lastTickTime = new Date();

/**
 * Not constant as we add "gaia".
 **/
var g_CivData = {};

var g_PlayerAssignments = { "local": { "name": translate("You"), "player": 1 } };

/**
 * Cache dev-mode settings that are frequently or widely used.
 */
var g_DevSettings = {
	"changePerspective": false,
	"controlAll": false
};

/**
 * Whether status bars should be shown for all of the player's units.
 */
var g_ShowAllStatusBars = false;

/**
 * Blink the population counter if the player can't train more units.
 */
var g_IsTrainingBlocked = false;

/**
 * Cache simulation state (updated on every simulation update).
 */
var g_SimState;
var g_EntityStates = {};
var g_TemplateData = {};
var g_TemplateDataWithoutLocalization = {};
var g_TechnologyData = {};

/**
 * Cache concatenated list of player states ("active", "defeated" or "won").
 */
var g_CachedLastStates = "";

/**
 * Whether the current player has lost/won and reached the end of their game.
 * Used for reporting the gamestate and showing the game-end message only once.
 */
var g_GameEnded = false;

/**
 * Top coordinate of the research list.
 * Changes depending on the number of displayed counters.
 */
var g_ResearchListTop = 4;

/**
 * List of additional entities to highlight.
 */
var g_ShowGuarding = false;
var g_ShowGuarded = false;
var g_AdditionalHighlight = [];

/**
 * Blink the hero selection if that entity has lost health since the last turn.
 */
var g_PreviousHeroHitPoints;

/**
 * Unit classes to be checked for the idle-worker-hotkey.
 */
var g_WorkerTypes = ["Female", "Trader", "FishingBoat", "CitizenSoldier", "Healer"];

/**
 * Cache the idle worker status.
 */
var g_HasIdleWorker = false;

function GetSimState()
{
	if (!g_SimState)
		g_SimState = Engine.GuiInterfaceCall("GetSimulationState");

	return g_SimState;
}

function GetEntityState(entId)
{
	if (!g_EntityStates[entId])
		g_EntityStates[entId] = Engine.GuiInterfaceCall("GetEntityState", entId);

	return g_EntityStates[entId];
}

function GetExtendedEntityState(entId)
{
	let entState = GetEntityState(entId);
	if (!entState || entState.extended)
		return entState;

	let extension = Engine.GuiInterfaceCall("GetExtendedEntityState", entId);
	for (let prop in extension)
		entState[prop] = extension[prop];
	entState.extended = true;
	g_EntityStates[entId] = entState;
	return entState;
}

function GetTemplateData(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		let template = Engine.GuiInterfaceCall("GetTemplateData", templateName);
		translateObjectKeys(template, ["specific", "generic", "tooltip"]);
		g_TemplateData[templateName] = template;
	}

	return g_TemplateData[templateName];
}

function GetTemplateDataWithoutLocalization(templateName)
{
	if (!(templateName in g_TemplateDataWithoutLocalization))
	{
		let template = Engine.GuiInterfaceCall("GetTemplateData", templateName);
		g_TemplateDataWithoutLocalization[templateName] = template;
	}

	return g_TemplateDataWithoutLocalization[templateName];
}

function GetTechnologyData(technologyName)
{
	if (!(technologyName in g_TechnologyData))
	{
		let template = Engine.GuiInterfaceCall("GetTechnologyData", technologyName);
		translateObjectKeys(template, ["specific", "generic", "description", "tooltip", "requirementsTooltip"]);
		g_TechnologyData[technologyName] = template;
	}

	return g_TechnologyData[technologyName];
}

function init(initData, hotloadData)
{
	if (!g_Settings)
	{
		Engine.EndGame();
		Engine.SwitchGuiPage("page_pregame.xml");
		return;
	}

	if (initData)
	{
		g_IsNetworked = initData.isNetworked;
		g_IsController = initData.isController;
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
		if (g_IsReplay)
			g_PlayerAssignments.local.player = -1;

		g_Players = getPlayerData(null);
	}

	g_CivData = loadCivData();
	g_CivData.gaia = { "Code": "gaia", "Name": translate("Gaia") };

	setObserverMode(Engine.GetPlayerID() == -1);

	updateTopPanel();

	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.list = g_GameSpeeds.Title;
	gameSpeed.list_data = g_GameSpeeds.Speed;
	let gameSpeedIdx = g_GameSpeeds.Speed.indexOf(Engine.GetSimRate());
	gameSpeed.selected = gameSpeedIdx != -1 ? gameSpeedIdx : g_GameSpeeds.Default;
	gameSpeed.onSelectionChange = function() { changeGameSpeed(+this.list_data[this.selected]); };
	initMenuPosition();

	// Populate player selection dropdown
	let playerNames = ["Observer"];
	let playerIDs = [-1];
	for (let player in g_Players)
	{
		playerNames.push(g_Players[player].name);
		playerIDs.push(player);
	}

	let viewPlayerDropdown = Engine.GetGUIObjectByName("viewPlayer");
	viewPlayerDropdown.list = playerNames;
	viewPlayerDropdown.list_data = playerIDs;
	viewPlayerDropdown.selected = Engine.GetPlayerID() + 1;

	// If in Atlas editor, disable the exit button
	if (Engine.IsAtlasRunning())
		Engine.GetGUIObjectByName("menuExitButton").enabled = false;

	if (hotloadData)
		g_Selection.selected = hotloadData.selection;

	// Starting for the first time:
	initMusic();
	if (Engine.GetPlayerID() != -1)
		global.music.storeTracks(g_CivData[g_Players[Engine.GetPlayerID()].civ].Music);
	global.music.setState(global.music.states.PEACE);
	playAmbient();

	onSimulationUpdate();

	setTimeout(displayGamestateNotifications, 1000);

	// Report the performance after 5 seconds (when we're still near
	// the initial camera view) and a minute (when the profiler will
	// have settled down if framerates as very low), to give some
	// extremely rough indications of performance
	//
	// DISABLED: this information isn't currently useful for anything much,
	// and it generates a massive amount of data to transmit and store
	//setTimeout(function() { reportPerformance(5); }, 5000);
	//setTimeout(function() { reportPerformance(60); }, 60000);
}

function toggleChangePerspective(enabled)
{
	g_DevSettings.changePerspective = enabled;
	Engine.GetGUIObjectByName("viewPlayer").hidden = !enabled && !g_IsObserver;
	selectViewPlayer(g_ViewedPlayer);
}

/**
 * Change perspective tool.
 * Shown to observers or when enabling the developers option.
 */
function selectViewPlayer(playerID)
{
	if (playerID < -1 || playerID > g_Players.length - 1 ||
			!g_DevSettings.changePerspective && !g_IsObserver)
		return;

	g_ViewedPlayer = playerID;
	Engine.SetPlayerID(g_DevSettings.changePerspective ? playerID : -1);

	updateTopPanel();
	onSimulationUpdate();

	let viewPlayer = Engine.GetGUIObjectByName("viewPlayer");
	let alphaLabel = Engine.GetGUIObjectByName("alphaLabel");
	alphaLabel.hidden = g_ViewedPlayer > 0 && !viewPlayer.hidden;
	alphaLabel.size = g_ViewedPlayer > 0 ? "50%+20 0 100%-226 100%" : "200 0 100%-475 100%";

	if (g_IsDiplomacyOpen)
		openDiplomacy();

	if (g_IsTradeOpen)
		openTrade();
}

function setObserverMode(enabled)
{
	g_IsObserver = enabled;

	let viewPlayerDropdown = Engine.GetGUIObjectByName("viewPlayer");

	viewPlayerDropdown.hidden = !enabled;
	if (enabled)
		viewPlayerDropdown.selected = 0;

	Engine.GetGUIObjectByName("alphaLabel").hidden = enabled;
}

/**
 * Returns true if the current user can issue commands for that player.
 */
function controlsPlayer(playerID)
{
	return Engine.GetPlayerID() == playerID || g_DevSettings.controlAll;
}

function updateTopPanel()
{
	let isPlayer = g_ViewedPlayer > 0;
	if (isPlayer)
	{
		let civName = g_CivData[g_Players[g_ViewedPlayer].civ].Name;
		Engine.GetGUIObjectByName("civIcon").sprite = "stretched:" + g_CivData[g_Players[g_ViewedPlayer].civ].Emblem;
		Engine.GetGUIObjectByName("civIconOverlay").tooltip = sprintf(translate("%(civ)s - Structure Tree"), { "civ": civName });
	}

	// Hide stuff gaia/observers don't use.
	Engine.GetGUIObjectByName("food").hidden = !isPlayer;
	Engine.GetGUIObjectByName("wood").hidden = !isPlayer;
	Engine.GetGUIObjectByName("stone").hidden = !isPlayer;
	Engine.GetGUIObjectByName("metal").hidden = !isPlayer;
	Engine.GetGUIObjectByName("population").hidden = !isPlayer;
	Engine.GetGUIObjectByName("civIcon").hidden = !isPlayer;
	Engine.GetGUIObjectByName("diplomacyButton1").hidden = !isPlayer;
	Engine.GetGUIObjectByName("tradeButton1").hidden = !isPlayer;
	Engine.GetGUIObjectByName("observerText").hidden = isPlayer;

	// Disable stuff observers shouldn't use
	let isActive = isPlayer && GetSimState().players[g_ViewedPlayer].state == "active" && controlsPlayer(g_ViewedPlayer);
	Engine.GetGUIObjectByName("pauseButton").enabled = isActive || !g_IsNetworked;
	Engine.GetGUIObjectByName("menuResignButton").enabled = isActive;

	// Enable observer-only "summary" button.
	Engine.GetGUIObjectByName("summaryButton").enabled = Engine.GetPlayerID() == -1;
}

function reportPerformance(time)
{
	let settings = Engine.GetMapSettings();
	Engine.SubmitUserReport("profile", 3, JSON.stringify({
		"time": time,
		"map": settings.Name,
		"seed": settings.Seed, // only defined for random maps
		"size": settings.Size, // only defined for random maps
		"profiler": Engine.GetProfilerState()
	}));
}

/**
 * Resign a player.
 * @param leaveGameAfterResign If player is quitting after resignation.
 */
function resignGame(leaveGameAfterResign)
{
	let simState = GetSimState();

	// Players can't resign if they've already won or lost.
	if (simState.players[Engine.GetPlayerID()].state != "active" || g_Disconnected)
		return;

	// Tell other players that we have given up and been defeated
	Engine.PostNetworkCommand({
		"type": "defeat-player",
		"playerId": Engine.GetPlayerID()
	});

	updateTopPanel();

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
	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	let mapSettings = Engine.GetMapSettings();
	let gameResult;

	if (Engine.GetPlayerID() == -1)
	{
		// Observers don't win/lose.
		gameResult = translate("You have left the game.");
		global.music.setState(global.music.states.VICTORY);
	}
	else
	{
		let playerState = extendedSimState.players[Engine.GetPlayerID()];
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

	let summary = {
		"timeElapsed" : extendedSimState.timeElapsed,
		"playerStates": extendedSimState.players,
		"players": g_Players,
		"mapSettings": Engine.GetMapSettings(),
	};

	if (!g_IsReplay)
		Engine.SaveReplayMetadata(JSON.stringify(summary));

	Engine.EndGame();

	if (g_IsController && Engine.HasXmppClient())
		Engine.SendUnregisterGame();

	summary.gameResult = gameResult;
	summary.isReplay = g_IsReplay;
	Engine.SwitchGuiPage("page_summary.xml", summary);
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { "selection": g_Selection.selected };
}

// Return some data that will be stored in saved game files
function getSavedGameData()
{
	// TODO: any other gui state?
	return {
		"playerAssignments": g_PlayerAssignments,
		"groups": g_Groups.groups
	};
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
	for (let groupNumber in data.groups)
	{
		g_Groups.groups[groupNumber].groups = data.groups[groupNumber].groups;
		g_Groups.groups[groupNumber].ents = data.groups[groupNumber].ents;
	}
	updateGroups();
}

/**
 * Called every frame.
 */
function onTick()
{
	if (!g_Settings)
		return;

	let now = new Date();
	let tickLength = new Date() - lastTickTime;
	lastTickTime = now;

	checkPlayerState();
	while (true)
	{
		let message = Engine.PollNetworkClient();
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
		if (Engine.GetPlayerID() != -1)
			Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	updateTimers();

	updateMenuPosition(tickLength);

	// When training is blocked, flash population (alternates color every 500msec)
	Engine.GetGUIObjectByName("resourcePop").textcolor = g_IsTrainingBlocked && Date.now() % 1000 < 500 ? g_PopulationAlertColor : g_DefaultPopulationColor;

	Engine.GuiInterfaceCall("ClearRenamedEntities");
}

function checkPlayerState()
{
	if (g_GameEnded || Engine.GetPlayerID() < 1)
		return;

	// Send a game report for each player in this game.
	let m_simState = GetSimState();
	let playerState = m_simState.players[Engine.GetPlayerID()];
	let tempStates = "";
	for (let player of m_simState.players)
		tempStates += player.state + ",";

	if (g_CachedLastStates != tempStates)
	{
		g_CachedLastStates = tempStates;
		reportGame();
	}

	if (playerState.state == "active")
		return;

	// Disable the resign- and pausebutton
	updateTopPanel();

	// Make sure nothing is open to avoid stacking.
	closeOpenDialogs();

	// Make sure this doesn't run again.
	g_GameEnded = true;

	let btCaptions;
	let btCode;
	let message;
	let title;
	if (Engine.IsAtlasRunning())
	{
		// If we're in Atlas, we can't leave the game
		btCaptions = [translate("OK")];
		btCode = [null];
		message = translate("Press OK to continue");
	}
	else
	{
		btCaptions = [translate("No"), translate("Yes")];
		btCode = [null, leaveGame];
		message = translate("Do you want to quit?");
	}

	if (playerState.state == "defeated")
	{
		title = translate("DEFEATED!");
		global.music.setState(global.music.states.DEFEAT);
		setObserverMode(true);
	}
	else if (playerState.state == "won")
	{
		title = translate("VICTORIOUS!");
		global.music.setState(global.music.states.VICTORY);
		// TODO: Reveal map directly instead of this silly proxy.
		if (!Engine.GetGUIObjectByName("devCommandsRevealMap").checked)
			Engine.GetGUIObjectByName("devCommandsRevealMap").checked = true;
	}

	messageBox(400, 200, message, title, 0, btCaptions, btCode);
}

function changeGameSpeed(speed)
{
	if (!g_IsNetworked)
		Engine.SetSimRate(speed);
}

function hasIdleWorker()
{
	for (let workerType of g_WorkerTypes)
	{
		let idleUnits = Engine.GuiInterfaceCall("FindIdleUnits", {
			"viewedPlayer": g_ViewedPlayer,
			"idleClass": workerType,
			"prevUnit": undefined,
			"limit": 1,
			"excludeUnits": []
		});

		if (idleUnits.length > 0)
			return true;
	}
	return false;
}

function updateIdleWorkerButton()
{
	g_HasIdleWorker = hasIdleWorker();

	let idleWorkerButton = Engine.GetGUIObjectByName("idleOverlay");
	let prefix = "stretched:session/";

	if (!g_HasIdleWorker)
		idleWorkerButton.sprite = prefix + "minimap-idle-disabled.png";
	else if (idleWorkerButton.sprite != prefix + "minimap-idle-highlight.png")
		idleWorkerButton.sprite = prefix + "minimap-idle.png";
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

	g_Selection.update();

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay();

	if (g_ShowGuarding || g_ShowGuarded)
		updateAdditionalHighlight();

	updateHero();
	updateGroups();
	updateDebug();
	updatePlayerDisplay();
	updateResearchDisplay();
	updateSelectionDetails();
	updateBuildingPlacementPreview();
	updateTimeNotifications();
	updateIdleWorkerButton();

	if (g_ViewedPlayer > 0)
	{
		let playerState = GetSimState().players[g_ViewedPlayer];
		g_DevSettings.controlAll = playerState && playerState.controlsAll;
		Engine.GetGUIObjectByName("devControlAll").checked = g_DevSettings.controlAll;
	}

	if (g_ViewedPlayer != -1 && !g_GameEnded)
	{
		// Update music state on basis of battle state.
		let battleState = Engine.GuiInterfaceCall("GetBattleState", g_ViewedPlayer);
		if (battleState)
			global.music.setState(global.music.states[battleState]);
	}
}

function onReplayFinished()
{
	closeOpenDialogs();
	pauseGame();

	messageBox(400, 200,
		translateWithContext("replayFinished", "The replay has finished. Do you want to quit?"),
		translateWithContext("replayFinished", "Confirmation"),
		0,
		[translateWithContext("replayFinished", "No"), translateWithContext("replayFinished", "Yes")],
		[resumeGame, leaveGame]);
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
	let statusBar = Engine.GetGUIObjectByName(nameOfBar);
	if (!statusBar)
		return;

	let healthSize = statusBar.size;
	let value = 100*Math.max(0, Math.min(1, points / maxPoints));

	// inverse bar
	if (direction == 2 || direction == 3)
		value = 100 - value;

	if (direction == 0)
		healthSize.rright = value;
	else if (direction == 1)
		healthSize.rbottom = value;
	else if (direction == 2)
		healthSize.rleft = value;
	else if (direction == 3)
		healthSize.rtop = value;

	statusBar.size = healthSize;
}


function updateHero()
{
	let unitHeroPanel = Engine.GetGUIObjectByName("unitHeroPanel");
	let heroButton = Engine.GetGUIObjectByName("unitHeroButton");

	let playerState = GetSimState().players[g_ViewedPlayer];
	if (!playerState || playerState.heroes.length <= 0)
	{
		g_PreviousHeroHitPoints = undefined;
		unitHeroPanel.hidden = true;
		return;
	}

	let heroImage = Engine.GetGUIObjectByName("unitHeroImage");
	let heroState = GetExtendedEntityState(playerState.heroes[0]);
	let template = GetTemplateData(heroState.template);
	heroImage.sprite = "stretched:session/portraits/" + template.icon;
	let hero = playerState.heroes[0];

	heroButton.onpress = function()
	{
		if (!Engine.HotkeyIsPressed("selection.add"))
			g_Selection.reset();
		g_Selection.addList([hero]);
	};
	heroButton.ondoublepress = function() { selectAndMoveTo(getEntityOrHolder(hero)); };
	unitHeroPanel.hidden = false;

	// Setup tooltip
	let tooltip = "[font=\"sans-bold-16\"]" + template.name.specific + "[/font]";
	let healthLabel = "[font=\"sans-bold-13\"]" + translate("Health:") + "[/font]";
	tooltip += "\n" + sprintf(translate("%(label)s %(current)s / %(max)s"), { label: healthLabel, current: heroState.hitpoints, max: heroState.maxHitpoints });
	if (heroState.attack)
		tooltip += "\n" + getAttackTooltip(heroState);

	tooltip += "\n" + getArmorTooltip(heroState.armour);
	if (template.tooltip)
		tooltip += "\n" + template.tooltip;

	heroButton.tooltip = tooltip;

	// update heros health bar
	updateGUIStatusBar("heroHealthBar", heroState.hitpoints, heroState.maxHitpoints);

	let heroHP = {
		"hitpoints": heroState.hitpoints,
		"player": g_ViewedPlayer
	};

	if (!g_PreviousHeroHitPoints)
		g_PreviousHeroHitPoints = heroHP;

	// if the health of the hero changed since the last update, trigger the animation
	if (g_PreviousHeroHitPoints.player == heroHP.player && g_PreviousHeroHitPoints.hitpoints > heroHP.hitpoints)
		startColorFade("heroHitOverlay", 100, 0, colorFade_attackUnit, true, smoothColorFadeRestart_attackUnit);

	g_PreviousHeroHitPoints = heroHP;
}

function updateGroups()
{
	let guiName = "Group";
	g_Groups.update();
	for (let i = 0; i < 10; ++i)
	{
		let button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		let label = Engine.GetGUIObjectByName("unit"+guiName+"Label["+i+"]").caption = i;
		button.hidden = g_Groups.groups[i].getTotalCount() == 0;
		button.onpress = (function(i) { return function() { performGroup((Engine.HotkeyIsPressed("selection.add") ? "add" : "select"), i); }; })(i);
		button.ondoublepress = (function(i) { return function() { performGroup("snap", i); }; })(i);
		button.onpressright = (function(i) { return function() { performGroup("breakUp", i); }; })(i);
		setPanelObjectPosition(button, i, 1);
	}
}

function updateDebug()
{
	let debug = Engine.GetGUIObjectByName("debug");

	if (!Engine.GetGUIObjectByName("devDisplayState").checked)
	{
		debug.hidden = true;
		return;
	}

	debug.hidden = false;

	let conciseSimState = deepcopy(GetSimState());
	conciseSimState.players = "<<<omitted>>>";
	let text = "simulation: " + uneval(conciseSimState);

	let selection = g_Selection.toList();
	if (selection.length)
	{
		let entState = GetExtendedEntityState(selection[0]);
		if (entState)
		{
			let template = GetTemplateData(entState.template);
			text += "\n\nentity: {\n";
			for (let k in entState)
				text += "  "+k+":"+uneval(entState[k])+"\n";
			text += "}\n\ntemplate: " + uneval(template);
		}
	}

	debug.caption = text.replace(/\[/g, "\\[");
}

function updatePlayerDisplay()
{
	let playerState = GetSimState().players[g_ViewedPlayer];
	if (!playerState)
		return;

	Engine.GetGUIObjectByName("resourceFood").caption = Math.floor(playerState.resourceCounts.food);
	Engine.GetGUIObjectByName("resourceWood").caption = Math.floor(playerState.resourceCounts.wood);
	Engine.GetGUIObjectByName("resourceStone").caption = Math.floor(playerState.resourceCounts.stone);
	Engine.GetGUIObjectByName("resourceMetal").caption = Math.floor(playerState.resourceCounts.metal);
	Engine.GetGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;
	Engine.GetGUIObjectByName("population").tooltip = translate("Population (current / limit)") + "\n" +
					sprintf(translate("Maximum population: %(popCap)s"), { "popCap": playerState.popMax });

	g_IsTrainingBlocked = playerState.trainingBlocked;
}

function selectAndMoveTo(ent)
{
	let entState = GetEntityState(ent);
	if (!entState || !entState.position)
		return;

	g_Selection.reset();
	g_Selection.addList([ent]);

	let position = entState.position;
	Engine.CameraMoveTo(position.x, position.z);
}

function updateResearchDisplay()
{
	let researchStarted = Engine.GuiInterfaceCall("GetStartedResearch", g_ViewedPlayer);

	// Set up initial positioning.
	let buttonSideLength = Engine.GetGUIObjectByName("researchStartedButton[0]").size.right;
	for (let i = 0; i < 10; ++i)
	{
		let button = Engine.GetGUIObjectByName("researchStartedButton[" + i + "]");
		let size = button.size;
		size.top = g_ResearchListTop + (4 + buttonSideLength) * i;
		size.bottom = size.top + buttonSideLength;
		button.size = size;
	}

	let numButtons = 0;
	for (let tech in researchStarted)
	{
		// Show at most 10 in-progress techs.
		if (numButtons >= 10)
			break;

		let template = GetTechnologyData(tech);
		let button = Engine.GetGUIObjectByName("researchStartedButton[" + numButtons + "]");
		button.hidden = false;
		button.tooltip = getEntityNames(template);
		button.onpress = (function(e) { return function() { selectAndMoveTo(e); }; })(researchStarted[tech].researcher);

		let icon = "stretched:session/portraits/" + template.icon;
		Engine.GetGUIObjectByName("researchStartedIcon[" + numButtons + "]").sprite = icon;

		// Scale the progress indicator.
		let size = Engine.GetGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(researchStarted[tech].progress * (size.right - size.left));
		Engine.GetGUIObjectByName("researchStartedProgressSlider[" + numButtons + "]").size = size;

		++numButtons;
	}

	// Hide unused buttons.
	for (let i = numButtons; i < 10; ++i)
		Engine.GetGUIObjectByName("researchStartedButton[" + i + "]").hidden = true;
}

// Toggles the display of status bars for all of the player's entities.
function recalculateStatusBarDisplay()
{
	let entities;
	if (g_ShowAllStatusBars)
		entities = Engine.GetPlayerID() == -1 ? Engine.PickNonGaiaEntitiesOnScreen() : Engine.PickPlayerEntitiesOnScreen(Engine.GetPlayerID());
	else
	{
		let selected = g_Selection.toList();
		for (let ent in g_Selection.highlighted)
			selected.push(g_Selection.highlighted[ent]);

		// Remove selected entities from the 'all entities' array, to avoid disabling their status bars.
		entities = Engine.GuiInterfaceCall(Engine.GetPlayerID() == -1 ? "GetNonGaiaEntities" : "GetPlayerEntities").filter(idx => selected.indexOf(idx) == -1);
	}

	Engine.GuiInterfaceCall("SetStatusBars", { "entities": entities, "enabled": g_ShowAllStatusBars });
}

// Update the additional list of entities to be highlighted.
function updateAdditionalHighlight()
{
	let entsAdd = []; // list of entities units to be highlighted
	let entsRemove = [];
	let highlighted = g_Selection.toList();
	for (let ent in g_Selection.highlighted)
		highlighted.push(g_Selection.highlighted[ent]);

	if (g_ShowGuarding)
	{
		// flag the guarding entities to add in this additional highlight
		for (let sel in g_Selection.selected)
		{
			let state = GetEntityState(g_Selection.selected[sel]);
			if (!state.guard || !state.guard.entities.length)
				continue;

			for (let ent of state.guard.entities)
				if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1)
					entsAdd.push(ent);
		}
	}

	if (g_ShowGuarded)
	{
		// flag the guarded entities to add in this additional highlight
		for (let sel in g_Selection.selected)
		{
			let state = GetEntityState(g_Selection.selected[sel]);
			if (!state.unitAI || !state.unitAI.isGuarding)
				continue;
			let ent = state.unitAI.isGuarding;
			if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1)
				entsAdd.push(ent);
		}
	}

	// flag the entities to remove (from the previously added) from this additional highlight
	for (let ent of g_AdditionalHighlight)
		if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1 && entsRemove.indexOf(ent) == -1)
			entsRemove.push(ent);

	_setHighlight(entsAdd, g_HighlightedAlpha, true);
	_setHighlight(entsRemove, 0, false);
	g_AdditionalHighlight = entsAdd;
}

function playAmbient()
{
	Engine.PlayAmbientSound(g_Ambient[Math.floor(Math.random() * g_Ambient.length)], true);
}

function getBuildString()
{
	return sprintf(translate("Build: %(buildDate)s (%(revision)s)"), { "buildDate": Engine.GetBuildTimestamp(0), revision: Engine.GetBuildTimestamp(2) });
}

function showTimeWarpMessageBox()
{
	messageBox(500, 250,
			translate("Note: time warp mode is a developer option, and not intended for use over long periods of time. Using it incorrectly may cause the game to run out of memory or crash."),
			translate("Time warp mode"), 2);
}

/**
 * Send a report on the gamestatus to the lobby.
 */
function reportGame()
{
	if (!Engine.HasXmppClient() || !Engine.IsRankedGame())
		return;

	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");

	let unitsClasses = [
		"total",
		"Infantry",
		"Worker",
		"Female",
		"Cavalry",
		"Champion",
		"Hero",
		"Ship",
		"Trader"
	];

	let unitsCountersTypes = [
		"unitsTrained",
		"unitsLost",
		"enemyUnitsKilled"
	];

	let buildingsClasses = [
		"total",
		"CivCentre",
		"House",
		"Economic",
		"Outpost",
		"Military",
		"Fortress",
		"Wonder"
	];

	let buildingsCountersTypes = [
		"buildingsConstructed",
		"buildingsLost",
		"enemyBuildingsDestroyed"
	];

	let resourcesTypes = [
		"wood",
		"food",
		"stone",
		"metal"
	];

	let resourcesCounterTypes = [
		"resourcesGathered",
		"resourcesUsed",
		"resourcesSold",
		"resourcesBought"
	];

	let playerStatistics = {};

	// Unit Stats
	for (let unitCounterType of unitsCountersTypes)
	{
		if (!playerStatistics[unitCounterType])
			playerStatistics[unitCounterType] = { };
		for (let unitsClass of unitsClasses)
			playerStatistics[unitCounterType][unitsClass] = "";
	}

	playerStatistics.unitsLostValue = "";
	playerStatistics.unitsKilledValue = "";
	// Building stats
	for (let buildingCounterType of buildingsCountersTypes)
	{
		if (!playerStatistics[buildingCounterType])
			playerStatistics[buildingCounterType] = { };
		for (let buildingsClass of buildingsClasses)
			playerStatistics[buildingCounterType][buildingsClass] = "";
	}

	playerStatistics.buildingsLostValue = "";
	playerStatistics.enemyBuildingsDestroyedValue = "";
	// Resources
	for (let resourcesCounterType of resourcesCounterTypes)
	{
		if (!playerStatistics[resourcesCounterType])
			playerStatistics[resourcesCounterType] = { };
		for (let resourcesType of resourcesTypes)
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
	playerStatistics.lootCollected = "";
	playerStatistics.feminisation = "";
	playerStatistics.percentMapExplored = "";

	let mapName = Engine.GetMapSettings().Name;
	let playerStates = "";
	let playerCivs = "";
	let teams = "";
	let teamsLocked = true;

	// Serialize the statistics for each player into a comma-separated list.
	// Ignore gaia
	for (let i = 1; i < extendedSimState.players.length; ++i)
	{
		let player = extendedSimState.players[i];

		playerStates += player.state + ",";
		playerCivs += player.civ + ",";
		teams += player.team + ",";
		teamsLocked = teamsLocked && player.teamsLocked;
		for (let resourcesCounterType of resourcesCounterTypes)
			for (let resourcesType of resourcesTypes)
				playerStatistics[resourcesCounterType][resourcesType] += player.statistics[resourcesCounterType][resourcesType] + ",";
		playerStatistics.resourcesGathered.vegetarianFood += player.statistics.resourcesGathered.vegetarianFood + ",";

		for (let unitCounterType of unitsCountersTypes)
			for (let unitsClass of unitsClasses)
				playerStatistics[unitCounterType][unitsClass] += player.statistics[unitCounterType][unitsClass] + ",";

		for (let buildingCounterType of buildingsCountersTypes)
			for (let buildingsClass of buildingsClasses)
				playerStatistics[buildingCounterType][buildingsClass] += player.statistics[buildingCounterType][buildingsClass] + ",";
		let total = 0;
		for (let type in player.statistics.resourcesGathered)
			total += player.statistics.resourcesGathered[type];

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
		playerStatistics.lootCollected += player.statistics.lootCollected + ",";
	}

	// Send the report with serialized data
	let reportObject = {};
	reportObject.timeElapsed = extendedSimState.timeElapsed;
	reportObject.playerStates = playerStates;
	reportObject.playerID = Engine.GetPlayerID();
	reportObject.matchID = g_MatchID;
	reportObject.civs = playerCivs;
	reportObject.teams = teams;
	reportObject.teamsLocked = String(teamsLocked);
	reportObject.ceasefireActive = String(extendedSimState.ceasefireActive);
	reportObject.ceasefireTimeRemaining = String(extendedSimState.ceasefireTimeRemaining);
	reportObject.mapName = mapName;
	reportObject.economyScore = playerStatistics.economyScore;
	reportObject.militaryScore = playerStatistics.militaryScore;
	reportObject.totalScore = playerStatistics.totalScore;
	for (let rct of resourcesCounterTypes)
	{
		for (let rt of resourcesTypes)
			reportObject[rt+rct.substr(9)] = playerStatistics[rct][rt];
			// eg. rt = food rct.substr = Gathered rct = resourcesGathered
	}
	reportObject.vegetarianFoodGathered = playerStatistics.resourcesGathered.vegetarianFood;
	for (let type of unitsClasses)
	{
		// eg. type = Infantry (type.substr(0,1)).toLowerCase()+type.substr(1) = infantry
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"UnitsTrained"] = playerStatistics.unitsTrained[type];
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"UnitsLost"] = playerStatistics.unitsLost[type];
		reportObject["enemy"+type+"UnitsKilled"] = playerStatistics.enemyUnitsKilled[type];
	}
	for (let type of buildingsClasses)
	{
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"BuildingsConstructed"] = playerStatistics.buildingsConstructed[type];
		reportObject[(type.substr(0,1)).toLowerCase()+type.substr(1)+"BuildingsLost"] = playerStatistics.buildingsLost[type];
		reportObject["enemy"+type+"BuildingsDestroyed"] = playerStatistics.enemyBuildingsDestroyed[type];
	}
	reportObject.tributesSent = playerStatistics.tributesSent;
	reportObject.tributesReceived = playerStatistics.tributesReceived;
	reportObject.percentMapExplored = playerStatistics.percentMapExplored;
	reportObject.treasuresCollected = playerStatistics.treasuresCollected;
	reportObject.lootCollected = playerStatistics.lootCollected;
	reportObject.tradeIncome = playerStatistics.tradeIncome;

	Engine.SendGameReport(reportObject);
}
