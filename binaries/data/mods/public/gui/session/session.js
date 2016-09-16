const g_IsReplay = Engine.IsVisualReplay();

const g_Ceasefire = prepareForDropdown(g_Settings && g_Settings.Ceasefire);
const g_GameSpeeds = prepareForDropdown(g_Settings && g_Settings.GameSpeeds.filter(speed => !speed.ReplayOnly || g_IsReplay));
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
const g_PopulationCapacities = prepareForDropdown(g_Settings && g_Settings.PopulationCapacities);
const g_StartingResources = prepareForDropdown(g_Settings && g_Settings.StartingResources);
const g_VictoryConditions = prepareForDropdown(g_Settings && g_Settings.VictoryConditions);
const g_WonderDurations = prepareForDropdown(g_Settings && g_Settings.WonderDurations);

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
 * Map, player and match settings set in gamesetup.
 */
const g_GameAttributes = Object.freeze(Engine.GetInitAttributes());

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
 * True if the current user has rejoined (or joined the game after it started).
 */
var g_HasRejoined = false;

/**
 * Shows a message box asking the user to leave if "won" or "defeated".
 */
var g_ConfirmExit = false;

/**
 * True if the current player has paused the game explicitly.
 */
var g_Paused = false;

/**
 * The list of GUIDs of players who have currently paused the game, if the game is networked.
 */
var g_PausingClients = [];

/**
 * The playerID selected in the change perspective tool.
 */
var g_ViewedPlayer = Engine.GetPlayerID();

/**
 * True if the camera should focus on attacks and player commands
 * and select the affected units.
 */
var g_FollowPlayer = false;

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
 */
var g_CivData = {};

/**
 * For restoring selection, order and filters when returning to the replay menu
 */
var g_ReplaySelectionData;

var g_PlayerAssignments = {
	"local": {
		"name": singleplayerName(),
		"player": 1
	}
};

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
 * Display data of the current players heroes.
 */
var g_Heroes = [];

/**
 * Unit classes to be checked for the idle-worker-hotkey.
 */
var g_WorkerTypes = ["Female", "Trader", "FishingBoat", "CitizenSoldier"];
/**
 * Unit classes to be checked for the military-only-selection modifier and for the idle-warrior-hotkey.
 */
var g_MilitaryTypes = ["Melee", "Ranged"];
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
		g_ReplaySelectionData = initData.replaySelectionData;
		g_HasRejoined = initData.isRejoining;

		if (initData.savedGUIData)
			restoreSavedGameData(initData.savedGUIData);

		Engine.GetGUIObjectByName("gameSpeedButton").hidden = g_IsNetworked;
	}
	else // Needed for autostart loading option
	{
		if (g_IsReplay)
			g_PlayerAssignments.local.player = -1;
	}

	g_Players = getPlayerData();

	g_CivData = loadCivData();
	g_CivData.gaia = { "Code": "gaia", "Name": translate("Gaia") };

	initializeMusic(); // before changing the perspective

	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.list = g_GameSpeeds.Title;
	gameSpeed.list_data = g_GameSpeeds.Speed;
	let gameSpeedIdx = g_GameSpeeds.Speed.indexOf(Engine.GetSimRate());
	gameSpeed.selected = gameSpeedIdx != -1 ? gameSpeedIdx : g_GameSpeeds.Default;
	gameSpeed.onSelectionChange = function() { changeGameSpeed(+this.list_data[this.selected]); };
	initMenuPosition();

	for (let slot in Engine.GetGUIObjectByName("unitHeroPanel").children)
		initGUIHeroes(slot);

	// Populate player selection dropdown
	let playerNames = [translate("Observer")];
	let playerIDs = [-1];
	for (let player in g_Players)
	{
		playerIDs.push(player);
		playerNames.push(colorizePlayernameHelper("■", player) + " " + g_Players[player].name);
	}

	// Select "observer" item when rejoining as a defeated player
	let viewedPlayer = g_Players[Engine.GetPlayerID()];
	let viewPlayerDropdown = Engine.GetGUIObjectByName("viewPlayer");
	viewPlayerDropdown.list = playerNames;
	viewPlayerDropdown.list_data = playerIDs;
	viewPlayerDropdown.selected = viewedPlayer && viewedPlayer.state == "defeated" ? 0 : Engine.GetPlayerID() + 1;

	// If in Atlas editor, disable the exit button
	if (Engine.IsAtlasRunning())
		Engine.GetGUIObjectByName("menuExitButton").enabled = false;

	initHotkeyTooltips();

	if (hotloadData)
		g_Selection.selected = hotloadData.selection;

	sendLobbyPlayerlistUpdate();
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

function initHotkeyTooltips()
{
	Engine.GetGUIObjectByName("idleWorkerButton").tooltip =
		colorizeHotkey("%(hotkey)s" + " ", "selection.idleworker") +
		translate("Find idle worker");

	Engine.GetGUIObjectByName("tradeHelp").tooltip =
		translate("Select one goods as origin of the changes, then use the arrows of the target goods to make the changes.") +
		colorizeHotkey(
			"\n" + translate("Using %(hotkey)s will put the selected resource to 100%%."),
			"session.fulltradeswap");
}

function initGUIHeroes(slot)
{
	let button = Engine.GetGUIObjectByName("unitHeroButton[" + slot + "]");

	button.onPress = function() {
		let hero = g_Heroes.find(hero => hero.slot !== undefined && hero.slot == slot);
		if (!hero)
			return;

		if (!Engine.HotkeyIsPressed("selection.add"))
			g_Selection.reset();

		g_Selection.addList([hero.ent]);
	};

	button.onDoublePress = function() {
		let hero = g_Heroes.find(hero => hero.slot !== undefined && hero.slot == slot);
		if (hero)
			selectAndMoveTo(getEntityOrHolder(hero.ent));
	};
}

function initializeMusic()
{
	initMusic();
	if (g_ViewedPlayer != -1)
		global.music.storeTracks(g_CivData[g_Players[g_ViewedPlayer].civ].Music);
	global.music.setState(global.music.states.PEACE);
	playAmbient();
}

function toggleChangePerspective(enabled)
{
	g_DevSettings.changePerspective = enabled;
	selectViewPlayer(g_ViewedPlayer);
}

/**
 * Change perspective tool.
 * Shown to observers or when enabling the developers option.
 */
function selectViewPlayer(playerID)
{
	if (playerID < -1 || playerID > g_Players.length - 1)
		return;

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay(true);

	g_IsObserver = isPlayerObserver(Engine.GetPlayerID());

	if (g_IsObserver || g_DevSettings.changePerspective)
		g_ViewedPlayer = playerID;

	if (g_DevSettings.changePerspective)
	{
		Engine.SetPlayerID(g_ViewedPlayer);
		g_IsObserver = isPlayerObserver(g_ViewedPlayer);
	}

	Engine.SetViewedPlayer(g_ViewedPlayer);

	updateTopPanel();

	updateChatAddressees();

	// Update GUI and clear player-dependent cache
	onSimulationUpdate();

	if (g_IsDiplomacyOpen)
		openDiplomacy();

	if (g_IsTradeOpen)
		openTrade();
}

/**
 * Returns true if the player with that ID is in observermode.
 */
function isPlayerObserver(playerID)
{
	let playerStates = GetSimState().players;
	return !playerStates[playerID] || playerStates[playerID].state != "active";
}

/**
 * Returns true if the current user can issue commands for that player.
 */
function controlsPlayer(playerID)
{
	let playerStates = GetSimState().players;

	return playerStates[Engine.GetPlayerID()] &&
		playerStates[Engine.GetPlayerID()].controlsAll ||
		Engine.GetPlayerID() == playerID &&
		playerStates[playerID] &&
		playerStates[playerID].state != "defeated";
}

/**
 * Called when a player has won or was defeated.
 */
function playerFinished(player, won)
{
	if (player == Engine.GetPlayerID())
		reportGame();

	updateDiplomacy();
	updateChatAddressees();

	if (player != Engine.GetPlayerID() || Engine.IsAtlasRunning())
		return;

	global.music.setState(
		won ?
			global.music.states.VICTORY :
			global.music.states.DEFEAT
	);

	// Select "observer" item on loss. On win enable observermode without changing perspective
	Engine.GetGUIObjectByName("viewPlayer").selected = won ? g_ViewedPlayer + 1 : 0;

	g_ConfirmExit = won ? "won" : "defeated";
}

/**
 * Sets civ icon for the currently viewed player.
 * Hides most gui objects for observers.
 */
function updateTopPanel()
{
	let isPlayer = g_ViewedPlayer > 0;

	let civIcon = Engine.GetGUIObjectByName("civIcon");
	civIcon.hidden = !isPlayer;
	if (isPlayer)
	{
		civIcon.sprite = "stretched:" + g_CivData[g_Players[g_ViewedPlayer].civ].Emblem;
		Engine.GetGUIObjectByName("civIconOverlay").tooltip = sprintf(translate("%(civ)s - Structure Tree"), {
			"civ": g_CivData[g_Players[g_ViewedPlayer].civ].Name
		});
	}

	Engine.GetGUIObjectByName("optionFollowPlayer").hidden = !g_IsObserver || !isPlayer;

	let viewPlayer = Engine.GetGUIObjectByName("viewPlayer");
	viewPlayer.hidden = !g_IsObserver && !g_DevSettings.changePerspective;

	Engine.GetGUIObjectByName("food").hidden = !isPlayer;
	Engine.GetGUIObjectByName("wood").hidden = !isPlayer;
	Engine.GetGUIObjectByName("stone").hidden = !isPlayer;
	Engine.GetGUIObjectByName("metal").hidden = !isPlayer;
	Engine.GetGUIObjectByName("population").hidden = !isPlayer;
	Engine.GetGUIObjectByName("diplomacyButton1").hidden = !isPlayer;
	Engine.GetGUIObjectByName("tradeButton1").hidden = !isPlayer;
	Engine.GetGUIObjectByName("observerText").hidden = isPlayer;

	let alphaLabel = Engine.GetGUIObjectByName("alphaLabel");
	alphaLabel.hidden = isPlayer && !viewPlayer.hidden;
	alphaLabel.size = isPlayer ? "50%+20 0 100%-226 100%" : "200 0 100%-475 100%";

	Engine.GetGUIObjectByName("pauseButton").enabled = !g_IsObserver || !g_IsNetworked;
	Engine.GetGUIObjectByName("menuResignButton").enabled = !g_IsObserver;
}

function reportPerformance(time)
{
	let settings = g_GameAttributes.settings;
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
	if (g_IsObserver || g_Disconnected)
		return;

	Engine.PostNetworkCommand({
		"type": "defeat-player",
		"playerId": Engine.GetPlayerID(),
		"resign": true
	});

	if (!leaveGameAfterResign)
		resumeGame(true);
}

/**
 * Leave the game
 * @param willRejoin If player is going to be rejoining a networked game.
 */
function leaveGame(willRejoin)
{
	if (!willRejoin && !g_IsObserver)
		resignGame(true);

	// Before ending the game
	let replayDirectory = Engine.GetCurrentReplayDirectory();
	let simData = getReplayMetadata();

	Engine.EndGame();

	if (g_IsController && Engine.HasXmppClient())
		Engine.SendUnregisterGame();

	Engine.SwitchGuiPage("page_summary.xml", {
		"sim": simData,
		"gui": {
			"assignedPlayer": Engine.GetPlayerID(),
			"disconnected": g_Disconnected,
			"isReplay": g_IsReplay,
			"replayDirectory": !g_HasRejoined && replayDirectory,
			"replaySelectionData": g_ReplaySelectionData
		}
	});
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { "selection": g_Selection.selected };
}

function getSavedGameData()
{
	return {
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

	handleNetMessages();

	updateCursorAndTooltip();

	if (g_Selection.dirty)
	{
		g_Selection.dirty = false;

		updateGUIObjects();

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

function changeGameSpeed(speed)
{
	if (!g_IsNetworked)
		Engine.SetSimRate(speed);
}

function hasIdleWorker()
{
	return Engine.GuiInterfaceCall("HasIdleUnits", {
			"viewedPlayer": g_ViewedPlayer,
			"idleClasses": g_WorkerTypes,
			"excludeUnits": []
	});
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

function onSimulationUpdate()
{
	g_EntityStates = {};
	g_TemplateData = {};
	g_TechnologyData = {};

	g_SimState = Engine.GuiInterfaceCall("GetSimulationState");

	if (!g_SimState)
		return;

	handleNotifications();
	updateGUIObjects();

	if (g_ConfirmExit)
		confirmExit();
}

/**
 * Don't show the message box before all playerstate changes are processed.
 */
function confirmExit()
{
	closeOpenDialogs();

	let subject = g_PlayerStateMessages[g_ConfirmExit] + "\n" +
		translate("Do you want to quit?");

	if (g_IsNetworked && g_IsController)
		subject += "\n" + translate("Leaving will disconnect all other players.");

	messageBox(
		400, 200,
		subject,
		g_ConfirmExit == "won" ?
			translate("VICTORIOUS!") :
			translate("DEFEATED!"),
		[translate("No"), translate("Yes")],
		[resumeGame, leaveGame]
	);

	g_ConfirmExit = false;
}

function updateGUIObjects()
{
	g_Selection.update();

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay();

	if (g_ShowGuarding || g_ShowGuarded)
		updateAdditionalHighlight();

	updateHeroes();
	displayHeroes();

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

	if (!g_IsObserver)
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

function updateHeroes()
{
	let playerState = GetSimState().players[g_ViewedPlayer];
	let heroes = playerState ? playerState.heroes : [];

	g_Heroes = g_Heroes.filter(hero => heroes.find(ent => ent == hero.ent));

	for (let ent of heroes)
	{
		let heroState = GetExtendedEntityState(ent);
		let template = GetTemplateData(heroState.template);

		let hero = g_Heroes.find(hero => ent == hero.ent);
		if (!hero)
		{
			hero = {
				"ent": ent,
				"tooltip": undefined,
				"sprite": "stretched:session/portraits/" + template.icon,
				"maxHitpoints": undefined,
				"currentHitpoints": heroState.hitpoints,
				"previousHitpoints": undefined
			};
			g_Heroes.push(hero);
		}

		hero.tooltip = createHeroTooltip(heroState, template);

		hero.previousHitpoints = hero.currentHitpoints;
		hero.currentHitpoints = heroState.hitpoints;
		hero.maxHitpoints = heroState.maxHitpoints;
	}
}

function createHeroTooltip(heroState, template)
{
	return [
		"[font=\"sans-bold-16\"]" + template.name.specific + "[/font]" + "\n" +
			sprintf(translate("%(label)s %(current)s / %(max)s"), {
				"label": "[font=\"sans-bold-13\"]" + translate("Health:") + "[/font]",
				"current": Math.ceil(heroState.hitpoints),
				"max": Math.ceil(heroState.maxHitpoints)
			}),
		getAttackTooltip(heroState),
		getArmorTooltip(heroState),
		getEntityTooltip(heroState)
	].filter(tip => tip).join("\n");
}

function displayHeroes()
{
	let buttons = Engine.GetGUIObjectByName("unitHeroPanel").children;

	buttons.forEach((button, slot) => {

		if (button.hidden || g_Heroes.some(hero => hero.slot !== undefined && hero.slot == slot))
			return;

		button.hidden = true;
		stopColorFade("heroHitOverlay[" + slot + "]");
	});

	// The slot identifies the button, displayIndex determines its position.
	for (let displayIndex = 0; displayIndex < Math.min(g_Heroes.length, buttons.length); ++displayIndex)
	{
		let hero = g_Heroes[displayIndex];

		// Find the first unused slot if new, otherwise reuse previous.
		let slot = hero.slot === undefined ?
			buttons.findIndex(button => button.hidden) :
			hero.slot;

		let heroButton = Engine.GetGUIObjectByName("unitHeroButton[" + slot + "]");
		heroButton.tooltip = hero.tooltip;

		updateGUIStatusBar("heroHealthBar[" + slot + "]", hero.currentHitpoints, hero.maxHitpoints);

		if (hero.slot === undefined)
		{
			let heroImage = Engine.GetGUIObjectByName("unitHeroImage[" + slot + "]");
			heroImage.sprite = hero.sprite;

			heroButton.hidden = false;
			hero.slot = slot;
		}

		// If the health of the hero changed since the last update, trigger the animation.
		if (hero.previousHitpoints > hero.currentHitpoints)
			startColorFade("heroHitOverlay[" + slot + "]", 100, 0,
				colorFade_attackUnit, true, smoothColorFadeRestart_attackUnit);

		// TODO: Instead of instant position changes, animate button movement.
		setPanelObjectPosition(heroButton, displayIndex, buttons.length);
	}
}

function updateGroups()
{
	g_Groups.update();

	// Determine the sum of the costs of a given template
	let getCostSum = (template) =>
	{
		let cost = GetTemplateData(template).cost;
		return Object.keys(cost).map(key => cost[key]).reduce((sum, cur) => sum + cur);
	};

	for (let i in Engine.GetGUIObjectByName("unitGroupPanel").children)
	{
		Engine.GetGUIObjectByName("unitGroupLabel[" + i + "]").caption = i;

		let button = Engine.GetGUIObjectByName("unitGroupButton["+i+"]");
		button.hidden = g_Groups.groups[i].getTotalCount() == 0;
		button.onpress = (function(i) { return function() { performGroup((Engine.HotkeyIsPressed("selection.add") ? "add" : "select"), i); }; })(i);
		button.ondoublepress = (function(i) { return function() { performGroup("snap", i); }; })(i);
		button.onpressright = (function(i) { return function() { performGroup("breakUp", i); }; })(i);

		// Chose icon of the most common template (or the most costly if it's not unique)
		if (g_Groups.groups[i].getTotalCount() > 0)
		{
			let icon = GetTemplateData(g_Groups.groups[i].getEntsGrouped().reduce((pre, cur) => {
				if (pre.ents.length == cur.ents.length)
					return getCostSum(pre.template) > getCostSum(cur.template) ? pre : cur;
				return pre.ents.length > cur.ents.length ? pre : cur;
			}).template).icon;

			Engine.GetGUIObjectByName("unitGroupIcon[" + i + "]").sprite =
				icon ? ("stretched:session/portraits/" + icon) : "groupsIcon";
		}

		setPanelObjectPosition(button, i, 1);
	}
}

function updateDebug()
{
	let debug = Engine.GetGUIObjectByName("debugEntityState");

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

function getAllyStatTooltip(resource)
{
	let playersState = GetSimState().players;
	let ret = "";
	for (let player in playersState)
		if (player != 0 && player != g_ViewedPlayer &&
		    (g_IsObserver || playersState[g_ViewedPlayer].hasSharedLos && g_Players[player].isMutualAlly[g_ViewedPlayer]))
			ret += "\n" + sprintf(translate("%(playername)s: %(statValue)s"),{
				"playername": colorizePlayernameHelper("■", player) + " " + g_Players[player].name,
				"statValue": resource == "pop" ?
					sprintf(translate("%(popCount)s/%(popLimit)s/%(popMax)s"), playersState[player]) :
					Math.round(playersState[player].resourceCounts[resource])
			});
	return ret;
}

function updatePlayerDisplay()
{
	let playerState = GetSimState().players[g_ViewedPlayer];
	if (!playerState)
		return;

	for (let res of RESOURCES)
	{
		Engine.GetGUIObjectByName("resource_" + res).caption = Math.floor(playerState.resourceCounts[res]);
		Engine.GetGUIObjectByName(res).tooltip = getLocalizedResourceName(res, "firstWord") + getAllyStatTooltip(res);
	}

	Engine.GetGUIObjectByName("resourcePop").caption = sprintf(translate("%(popCount)s/%(popLimit)s"), playerState);
	Engine.GetGUIObjectByName("population").tooltip = translate("Population (current / limit)") + "\n" +
		sprintf(translate("Maximum population: %(popCap)s"), { "popCap": playerState.popMax }) +
		getAllyStatTooltip("pop");

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

/**
 * Toggles the display of status bars for all of the player's entities.
 *
 * @param {Boolean} remove - Whether to hide all previously shown status bars.
 */
function recalculateStatusBarDisplay(remove = false)
{
	let entities;
	if (g_ShowAllStatusBars && !remove)
		entities = g_ViewedPlayer == -1 ?
			Engine.PickNonGaiaEntitiesOnScreen() :
			Engine.PickPlayerEntitiesOnScreen(g_ViewedPlayer);
	else
	{
		let selected = g_Selection.toList();
		for (let ent in g_Selection.highlighted)
			selected.push(g_Selection.highlighted[ent]);

		// Remove selected entities from the 'all entities' array,
		// to avoid disabling their status bars.
		entities = Engine.GuiInterfaceCall(
			g_ViewedPlayer == -1 ? "GetNonGaiaEntities" : "GetPlayerEntities", {
				"viewedPlayer": g_ViewedPlayer
			}).filter(idx => selected.indexOf(idx) == -1);
	}

	Engine.GuiInterfaceCall("SetStatusBars", {
		"entities": entities,
		"enabled": g_ShowAllStatusBars && !remove
	});
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
	return sprintf(translate("Build: %(buildDate)s (%(revision)s)"), {
		"buildDate": Engine.GetBuildTimestamp(0),
		"revision": Engine.GetBuildTimestamp(2)
	});
}

function showTimeWarpMessageBox()
{
	messageBox(
		500, 250,
		translate("Note: time warp mode is a developer option, and not intended for use over long periods of time. Using it incorrectly may cause the game to run out of memory or crash."),
		translate("Time warp mode")
	);
}

/**
 * Send a report on the gamestatus to the lobby.
 */
function reportGame()
{
	// Only 1v1 games are rated (and Gaia is part of g_Players)
	if (!Engine.HasXmppClient() || !Engine.IsRankedGame() ||
	    g_Players.length != 3 || Engine.GetPlayerID() == -1)
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

	let mapName = g_GameAttributes.settings.Name;
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
	reportObject.matchID = g_GameAttributes.matchID;
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
