const g_IsReplay = Engine.IsVisualReplay();

const g_CivData = loadCivData(false, true);

const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
const g_PopulationCapacities = prepareForDropdown(g_Settings && g_Settings.PopulationCapacities);
const g_WorldPopulationCapacities = prepareForDropdown(g_Settings && g_Settings.WorldPopulationCapacities);
const g_StartingResources = prepareForDropdown(g_Settings && g_Settings.StartingResources);
const g_VictoryConditions = g_Settings && g_Settings.VictoryConditions;

var g_Ambient;
var g_AutoFormation;
var g_Chat;
var g_Cheats;
var g_DeveloperOverlay;
var g_DiplomacyColors;
var g_DiplomacyDialog;
var g_GameSpeedControl;
var g_Menu;
var g_MiniMapPanel;
var g_NetworkStatusOverlay;
var g_ObjectivesDialog;
var g_OutOfSyncNetwork;
var g_OutOfSyncReplay;
var g_PanelEntityManager;
var g_PauseControl;
var g_PauseOverlay;
var g_PlayerViewControl;
var g_QuitConfirmationDefeat;
var g_QuitConfirmationReplay;
var g_RangeOverlayManager;
var g_ResearchProgress;
var g_TimeNotificationOverlay;
var g_TopPanel;
var g_TradeDialog;

/**
 * Map, player and match settings set in game setup.
 */
const g_GameAttributes = deepfreeze(Engine.GuiInterfaceCall("GetInitAttributes"));

/**
 * True if this is a multiplayer game.
 */
const g_IsNetworked = Engine.HasNetClient();

/**
 * Is this user in control of game settings (i.e. is a network server, or offline player).
 */
var g_IsController = !g_IsNetworked || Engine.HasNetServer();

/**
 * Whether we have finished the synchronization and
 * can start showing simulation related message boxes.
 */
var g_IsNetworkedActive = false;

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
var g_LastTickTime = Date.now();

/**
 * Recalculate which units have their status bars shown with this frequency in milliseconds.
 */
var g_StatusBarUpdate = 200;

/**
 * For restoring selection, order and filters when returning to the replay menu
 */
var g_ReplaySelectionData;

/**
 * Remembers which clients are assigned to which player slots.
 * The keys are GUIDs or "local" in single-player.
 */
var g_PlayerAssignments;

/**
 * Whether the entire UI should be hidden (useful for promotional screenshots).
 * Can be toggled with a hotkey.
 */
var g_ShowGUI = true;

/**
 * Whether status bars should be shown for all of the player's units.
 */
var g_ShowAllStatusBars = false;

/**
 * Cache of simulation state and template data (apart from TechnologyData, updated on every simulation update).
 */
var g_SimState;
var g_EntityStates = {};
var g_TemplateData = {};
var g_TechnologyData = {};

var g_ResourceData = new Resources();

/**
 * These handlers are called each time a new turn was simulated.
 * Use this as sparely as possible.
 */
var g_SimulationUpdateHandlers = new Set();

/**
 * These handlers are called after the player states have been initialized.
 */
var g_PlayersInitHandlers = new Set();

/**
 * These handlers are called when a player has been defeated or won the game.
 */
var g_PlayerFinishedHandlers = new Set();

/**
 * These events are fired whenever the player added or removed entities from the selection.
 */
var g_EntitySelectionChangeHandlers = new Set();

/**
 * These events are fired when the user has performed a hotkey assignment change.
 * Currently only fired on init, but to be fired from any hotkey editor dialog.
 */
var g_HotkeyChangeHandlers = new Set();

/**
 * List of additional entities to highlight.
 */
var g_ShowGuarding = false;
var g_ShowGuarded = false;
var g_AdditionalHighlight = [];

/**
 * Order in which the panel entities are shown.
 */
var g_PanelEntityOrder = ["Hero", "Relic"];

/**
 * Unit classes to be checked for the idle-worker-hotkey.
 */
var g_WorkerTypes = ["FemaleCitizen", "Trader", "FishingBoat", "Citizen"];

/**
 * Unit classes to be checked for the military-only-selection modifier and for the idle-warrior-hotkey.
 */
var g_MilitaryTypes = ["Melee", "Ranged"];

function GetSimState()
{
	if (!g_SimState)
		g_SimState = deepfreeze(Engine.GuiInterfaceCall("GetSimulationState"));

	return g_SimState;
}

function GetMultipleEntityStates(ents)
{
	if (!ents.length)
		return null;
	let entityStates = Engine.GuiInterfaceCall("GetMultipleEntityStates", ents);
	for (let item of entityStates)
		g_EntityStates[item.entId] = item.state && deepfreeze(item.state);
	return entityStates;
}

function GetEntityState(entId)
{
	if (!g_EntityStates[entId])
	{
		let entityState = Engine.GuiInterfaceCall("GetEntityState", entId);
		g_EntityStates[entId] = entityState && deepfreeze(entityState);
	}

	return g_EntityStates[entId];
}

/**
 * Returns template data calling GetTemplateData defined in GuiInterface.js
 * and deepfreezing returned object.
 * @param {string} templateName - Data of this template will be returned.
 * @param {number|undefined} player - Modifications of this player will be applied to the template.
 *      If undefined, id of player calling this method will be used.
 */
function GetTemplateData(templateName, player)
{
	if (!(templateName in g_TemplateData))
	{
		let template = Engine.GuiInterfaceCall("GetTemplateData", { "templateName": templateName, "player": player });
		translateObjectKeys(template, ["specific", "generic", "tooltip"]);
		g_TemplateData[templateName] = deepfreeze(template);
	}
	return g_TemplateData[templateName];
}

function GetTechnologyData(technologyName, civ)
{
	if (!g_TechnologyData[civ])
		g_TechnologyData[civ] = {};

	if (!(technologyName in g_TechnologyData[civ]))
	{
		let template = GetTechnologyDataHelper(TechnologyTemplates.Get(technologyName), civ, g_ResourceData);
		translateObjectKeys(template, ["specific", "generic", "description", "tooltip", "requirementsTooltip"]);
		g_TechnologyData[civ][technologyName] = deepfreeze(template);
	}

	return g_TechnologyData[civ][technologyName];
}

function init(initData, hotloadData)
{
	if (!g_Settings)
	{
		Engine.EndGame();
		Engine.SwitchGuiPage("page_pregame.xml");
		return;
	}

	// Fallback used by atlas
	g_PlayerAssignments = initData ? initData.playerAssignments : { "local": { "player": 1 } };

	// Fallback used by atlas and autostart games
	if (g_PlayerAssignments.local && !g_PlayerAssignments.local.name)
		g_PlayerAssignments.local.name = singleplayerName();

	if (initData)
	{
		g_ReplaySelectionData = initData.replaySelectionData;
		g_HasRejoined = initData.isRejoining;

		if (initData.savedGUIData)
			restoreSavedGameData(initData.savedGUIData);
	}

	let mapCache = new MapCache();
	g_Cheats = new Cheats();
	g_DiplomacyColors = new DiplomacyColors();
	g_PlayerViewControl = new PlayerViewControl();
	g_PlayerViewControl.registerViewedPlayerChangeHandler(g_DiplomacyColors.updateDisplayedPlayerColors.bind(g_DiplomacyColors));
	g_DiplomacyColors.registerDiplomacyColorsChangeHandler(g_PlayerViewControl.rebuild.bind(g_PlayerViewControl));
	g_DiplomacyColors.registerDiplomacyColorsChangeHandler(updateGUIObjects);
	g_PauseControl = new PauseControl();
	g_PlayerViewControl.registerPreViewedPlayerChangeHandler(removeStatusBarDisplay);
	g_PlayerViewControl.registerViewedPlayerChangeHandler(resetTemplates);

	g_Ambient = new Ambient();
	g_AutoFormation = new AutoFormation();
	g_Chat = new Chat(g_PlayerViewControl, g_Cheats);
	g_DeveloperOverlay = new DeveloperOverlay(g_PlayerViewControl, g_Selection);
	g_DiplomacyDialog = new DiplomacyDialog(g_PlayerViewControl, g_DiplomacyColors);
	g_GameSpeedControl = new GameSpeedControl(g_PlayerViewControl);
	g_Menu = new Menu(g_PauseControl, g_PlayerViewControl, g_Chat);
	g_MiniMapPanel = new MiniMapPanel(g_PlayerViewControl, g_DiplomacyColors, g_WorkerTypes);
	g_NetworkStatusOverlay = new NetworkStatusOverlay();
	g_ObjectivesDialog = new ObjectivesDialog(g_PlayerViewControl, mapCache);
	g_OutOfSyncNetwork = new OutOfSyncNetwork();
	g_OutOfSyncReplay = new OutOfSyncReplay();
	g_PanelEntityManager = new PanelEntityManager(g_PlayerViewControl, g_Selection, g_PanelEntityOrder);
	g_PauseOverlay = new PauseOverlay(g_PauseControl);
	g_QuitConfirmationDefeat = new QuitConfirmationDefeat();
	g_QuitConfirmationReplay = new QuitConfirmationReplay();
	g_RangeOverlayManager = new RangeOverlayManager(g_Selection);
	g_ResearchProgress = new ResearchProgress(g_PlayerViewControl, g_Selection);
	g_TradeDialog = new TradeDialog(g_PlayerViewControl);
	g_TopPanel = new TopPanel(g_PlayerViewControl, g_DiplomacyDialog, g_TradeDialog, g_ObjectivesDialog, g_GameSpeedControl);
	g_TimeNotificationOverlay = new TimeNotificationOverlay(g_PlayerViewControl);

	initBatchTrain();
	initSelectionPanels();
	LoadModificationTemplates();
	updatePlayerData();
	initializeMusic(); // before changing the perspective
	Engine.SetBoundingBoxDebugOverlay(false);

	for (let handler of g_PlayersInitHandlers)
		handler();

	for (let handler of g_HotkeyChangeHandlers)
		handler();

	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
		g_PlayerAssignments = hotloadData.playerAssignments;
		g_Players = hotloadData.player;
	}

	// TODO: use event instead
	onSimulationUpdate();

	setTimeout(displayGamestateNotifications, 1000);
}

function registerPlayersInitHandler(handler)
{
	g_PlayersInitHandlers.add(handler);
}

function registerPlayersFinishedHandler(handler)
{
	g_PlayerFinishedHandlers.add(handler);
}

function registerSimulationUpdateHandler(handler)
{
	g_SimulationUpdateHandlers.add(handler);
}

function unregisterSimulationUpdateHandler(handler)
{
	g_SimulationUpdateHandlers.delete(handler);
}

function registerEntitySelectionChangeHandler(handler)
{
	g_EntitySelectionChangeHandlers.add(handler);
}

function unregisterEntitySelectionChangeHandler(handler)
{
	g_EntitySelectionChangeHandlers.delete(handler);
}

function registerHotkeyChangeHandler(handler)
{
	g_HotkeyChangeHandlers.add(handler);
}

function updatePlayerData()
{
	let simState = GetSimState();
	if (!simState)
		return;

	let playerData = [];

	for (let i = 0; i < simState.players.length; ++i)
	{
		let playerState = simState.players[i];

		playerData.push({
			"name": playerState.name,
			"civ": playerState.civ,
			"color": {
				"r": playerState.color.r * 255,
				"g": playerState.color.g * 255,
				"b": playerState.color.b * 255,
				"a": playerState.color.a * 255
			},
			"team": playerState.team,
			"teamsLocked": playerState.teamsLocked,
			"cheatsEnabled": playerState.cheatsEnabled,
			"state": playerState.state,
			"isAlly": playerState.isAlly,
			"isMutualAlly": playerState.isMutualAlly,
			"isNeutral": playerState.isNeutral,
			"isEnemy": playerState.isEnemy,
			"guid": undefined, // network guid for players controlled by hosts
			"offline": g_Players[i] && !!g_Players[i].offline
		});
	}

	for (let guid in g_PlayerAssignments)
	{
		let playerID = g_PlayerAssignments[guid].player;

		if (!playerData[playerID])
			continue;

		playerData[playerID].guid = guid;
		playerData[playerID].name = g_PlayerAssignments[guid].name;
	}

	g_Players = playerData;
}

/**
 * @param {number} ent - The entity to get its ID for.
 * @return {number} - The entity ID of the entity or of its garrisonHolder.
 */
function getEntityOrHolder(ent)
{
	let entState = GetEntityState(ent);
	if (entState && !entState.position && entState.garrisonable && entState.garrisonable.holder != INVALID_ENTITY)
		return getEntityOrHolder(entState.garrisonable.holder);

	return ent;
}

function initializeMusic()
{
	initMusic();
	if (g_ViewedPlayer != -1 && g_CivData[g_Players[g_ViewedPlayer].civ].Music)
		global.music.storeTracks(g_CivData[g_Players[g_ViewedPlayer].civ].Music);
	global.music.setState(global.music.states.PEACE);
}

function resetTemplates()
{
	// Update GUI and clear player-dependent cache
	g_TemplateData = {};
	Engine.GuiInterfaceCall("ResetTemplateModified");

	// TODO: do this more selectively
	onSimulationUpdate();
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

	return !!playerStates[Engine.GetPlayerID()] &&
		playerStates[Engine.GetPlayerID()].controlsAll ||
		Engine.GetPlayerID() == playerID &&
		!!playerStates[playerID] &&
		playerStates[playerID].state != "defeated";
}

/**
 * Called when one or more players have won or were defeated.
 *
 * @param {array} - IDs of the players who have won or were defeated.
 * @param {Object} - a plural string stating the victory reason.
 * @param {boolean} - whether these players have won or lost.
 */
function playersFinished(players, victoryString, won)
{
	addChatMessage({
		"type": "playerstate",
		"message": victoryString,
		"players": players
	});

	updatePlayerData();

	// TODO: The other calls in this function should move too
	for (let handler of g_PlayerFinishedHandlers)
		handler(players, won);

	if (players.indexOf(Engine.GetPlayerID()) == -1 || Engine.IsAtlasRunning())
		return;

	global.music.setState(
		won ?
			global.music.states.VICTORY :
			global.music.states.DEFEAT
	);
}

function resumeGame()
{
	g_PauseControl.implicitResume();
}

function closeOpenDialogs()
{
	g_Menu.close();
	g_Chat.closePage();
	g_DiplomacyDialog.close();
	g_ObjectivesDialog.close();
	g_TradeDialog.close();
}

function endGame()
{
	// Before ending the game
	let replayDirectory = Engine.GetCurrentReplayDirectory();
	let simData = Engine.GuiInterfaceCall("GetReplayMetadata");
	let playerID = Engine.GetPlayerID();

	Engine.EndGame();

	// After the replay file was closed in EndGame
	// Done here to keep EndGame small
	if (!g_IsReplay)
		Engine.AddReplayToCache(replayDirectory);

	if (g_IsController && Engine.HasXmppClient())
		Engine.SendUnregisterGame();

	Engine.SwitchGuiPage("page_summary.xml", {
		"sim": simData,
		"gui": {
			"dialog": false,
			"assignedPlayer": playerID,
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
	return {
		"selection": g_Selection.selected,
		"playerAssignments": g_PlayerAssignments,
		"player": g_Players,
	};
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

	let now = Date.now();
	let tickLength = now - g_LastTickTime;
	g_LastTickTime = now;

	handleNetMessages();

	updateCursorAndTooltip();

	if (g_Selection.dirty)
	{
		g_Selection.dirty = false;
		// When selection changed, get the entityStates of new entities
		GetMultipleEntityStates(g_Selection.toList().filter(entId => !g_EntityStates[entId]));

		for (let handler of g_EntitySelectionChangeHandlers)
			handler();

		updateGUIObjects();

		// Display rally points for selected structures.
		if (Engine.GetPlayerID() != -1)
			Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}
	else if (g_ShowAllStatusBars && now % g_StatusBarUpdate <= tickLength)
		recalculateStatusBarDisplay();

	updateTimers();
	Engine.GuiInterfaceCall("ClearRenamedEntities");
}

function onSimulationUpdate()
{
	// Templates change depending on technologies and auras, so they have to be reloaded after such a change.
	// g_TechnologyData data never changes, so it shouldn't be deleted.
	g_EntityStates = {};
	if (Engine.GuiInterfaceCall("IsTemplateModified"))
	{
		g_TemplateData = {};
		Engine.GuiInterfaceCall("ResetTemplateModified");
	}
	g_SimState = undefined;

	// Some changes may require re-rendering the selection.
	if (Engine.GuiInterfaceCall("IsSelectionDirty"))
	{
		g_Selection.onChange();
		Engine.GuiInterfaceCall("ResetSelectionDirty");
	}

	if (!GetSimState())
		return;

	GetMultipleEntityStates(g_Selection.toList());

	for (let handler of g_SimulationUpdateHandlers)
		handler();

	// TODO: Move to handlers
	updateCinemaPath();
	handleNotifications();
	updateGUIObjects();
}

function toggleGUI()
{
	g_ShowGUI = !g_ShowGUI;
	updateCinemaPath();
}

function updateCinemaPath()
{
	let isPlayingCinemaPath = GetSimState().cinemaPlaying && !g_Disconnected;

	Engine.GetGUIObjectByName("session").hidden = !g_ShowGUI || isPlayingCinemaPath;
	Engine.ConfigDB_CreateValue("user", "silhouettes", !isPlayingCinemaPath && Engine.ConfigDB_GetValue("user", "silhouettes") == "true" ? "true" : "false");
}

// TODO: Use event subscription onSimulationUpdate, onEntitySelectionChange, onPlayerViewChange, ... instead
function updateGUIObjects()
{
	g_Selection.update();

	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay();

	if (g_ShowGuarding || g_ShowGuarded)
		updateAdditionalHighlight();

	updateGroups();
	updateSelectionDetails();
	updateBuildingPlacementPreview();

	if (!g_IsObserver)
	{
		// Update music state on basis of battle state.
		let battleState = Engine.GuiInterfaceCall("GetBattleState", g_ViewedPlayer);
		if (battleState)
			global.music.setState(global.music.states[battleState]);
	}
}

function updateGroups()
{
	g_Groups.update();

	// Determine the sum of the costs of a given template
	let getCostSum = (ent) => {
		let cost = GetTemplateData(GetEntityState(ent).template).cost;
		return cost ? Object.keys(cost).map(key => cost[key]).reduce((sum, cur) => sum + cur) : 0;
	};

	for (let i in Engine.GetGUIObjectByName("unitGroupPanel").children)
	{
		Engine.GetGUIObjectByName("unitGroupLabel[" + i + "]").caption = i;

		let button = Engine.GetGUIObjectByName("unitGroupButton[" + i + "]");
		button.hidden = g_Groups.groups[i].getTotalCount() == 0;
		button.onPress = (function(i) { return function() { performGroup((Engine.HotkeyIsPressed("selection.add") ? "add" : "select"), i); }; })(i);
		button.onDoublePress = (function(i) { return function() { performGroup("snap", i); }; })(i);
		button.onPressRight = (function(i) { return function() { performGroup("breakUp", i); }; })(i);

		// Choose the icon of the most common template (or the most costly if it's not unique)
		if (g_Groups.groups[i].getTotalCount() > 0)
		{
			let icon = GetTemplateData(GetEntityState(g_Groups.groups[i].getEntsGrouped().reduce((pre, cur) => {
				if (pre.ents.length == cur.ents.length)
					return getCostSum(pre.ents[0]) > getCostSum(cur.ents[0]) ? pre : cur;
				return pre.ents.length > cur.ents.length ? pre : cur;
			}).ents[0]).template).icon;

			Engine.GetGUIObjectByName("unitGroupIcon[" + i + "]").sprite =
				icon ? ("stretched:session/portraits/" + icon) : "groupsIcon";
		}

		setPanelObjectPosition(button, i, 1);
	}
}

/**
 * Toggles the display of status bars for all of the player's entities.
 *
 * @param {boolean} remove - Whether to hide all previously shown status bars.
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
		"enabled": g_ShowAllStatusBars && !remove,
		"showRank": Engine.ConfigDB_GetValue("user", "gui.session.rankabovestatusbar") == "true",
		"showExperience": Engine.ConfigDB_GetValue("user", "gui.session.experiencestatusbar") == "true"
	});
}

function removeStatusBarDisplay()
{
	if (g_ShowAllStatusBars)
		recalculateStatusBarDisplay(true);
}

/**
 * Inverts the given configuration boolean and returns the current state.
 * For example "silhouettes".
 */
function toggleConfigBool(configName)
{
	let enabled = Engine.ConfigDB_GetValue("user", configName) != "true";
	Engine.ConfigDB_CreateAndWriteValueToFile("user", configName, String(enabled), "config/user.cfg");
	return enabled;
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

	if (g_ShowGuarded)
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

	// flag the entities to remove (from the previously added) from this additional highlight
	for (let ent of g_AdditionalHighlight)
		if (highlighted.indexOf(ent) == -1 && entsAdd.indexOf(ent) == -1 && entsRemove.indexOf(ent) == -1)
			entsRemove.push(ent);

	_setHighlight(entsAdd, g_HighlightedAlpha, true);
	_setHighlight(entsRemove, 0, false);
	g_AdditionalHighlight = entsAdd;
}
