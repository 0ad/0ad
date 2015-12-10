const g_MatchSettings_SP = "config/matchsettings.json";
const g_MatchSettings_MP = "config/matchsettings.mp.json";

const g_Ceasefire = prepareForDropdown(g_Settings ? g_Settings.Ceasefire : undefined);
const g_GameSpeeds = prepareForDropdown(g_Settings ? g_Settings.GameSpeeds.filter(speed => !speed.ReplayOnly) : undefined);
const g_MapSizes = prepareForDropdown(g_Settings ? g_Settings.MapSizes : undefined);
const g_MapTypes = prepareForDropdown(g_Settings ? g_Settings.MapTypes : undefined);
const g_PopulationCapacities = prepareForDropdown(g_Settings ? g_Settings.PopulationCapacities : undefined);
const g_StartingResources = prepareForDropdown(g_Settings ? g_Settings.StartingResources : undefined);
const g_VictoryConditions = prepareForDropdown(g_Settings ? g_Settings.VictoryConditions : undefined);

/**
 * All selectable playercolors except gaia.
 */
const g_PlayerColors = g_Settings ? g_Settings.PlayerDefaults.slice(1).map(pData => pData.Color) : undefined;

/**
 * Used for generating the botnames.
 */
const g_RomanNumbers = [undefined, "I", "II", "III", "IV", "V", "VI", "VII", "VIII"];

/**
 * Offer users to select playable civs only.
 * Load unselectable civs as they could appear in scenario maps.
 */
const g_CivData = loadCivData();

// Is this is a networked game, or offline
var g_IsNetworked;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;

// Server name, if user is a server, connected to the multiplayer lobby
var g_ServerName;

// Are we currently updating the GUI in response to network messages instead of user input
// (and therefore shouldn't send further messages to the network)
var g_IsInGuiUpdate;

// Is this user ready
var g_IsReady;

// There are some duplicate orders on init, we can ignore these [bool].
var g_ReadyInit = true;

// If no one has changed ready status, we have no need to spam the settings changed message.
// 2 - Host's initial ready, suppressed settings message, 1 - Will show settings message, <=0 - Suppressed settings message
var g_ReadyChanged = 2;

/**
 * Used to prevent calling resetReadyData when starting a game.
 */
var g_GameStarted = false;

var g_PlayerAssignments = {};

// Default game setup attributes
var g_DefaultPlayerData = [];
var g_GameAttributes = {
	settings: {}
};

var g_ChatMessages = [];

/**
 * Cache containing the mapsettings for scenario/skirmish maps. Just-in-time loading.
 */
var g_MapData = {};

/**
 * Holds available map filters (for example "naval") and the functions which test the maps.
 */
var g_MapFilters = [];

// Current number of assigned human players.
var g_AssignedCount = 0;

// To prevent the display locking up while we load the map metadata,
// we'll start with a 'loading' message and switch to the main screen in the
// tick handler
var g_LoadingState = 0; // 0 = not started, 1 = loading, 2 = loaded

function init(attribs)
{
	if (!g_Settings)
	{
		cancelSetup();
		return;
	}

	switch (attribs.type)
	{
	case "offline":
		g_IsNetworked = false;
		g_IsController = true;
		break;
	case "server":
		g_IsNetworked = true;
		g_IsController = true;
		break;
	case "client":
		g_IsNetworked = true;
		g_IsController = false;
		break;
	default:
		error("Unexpected 'type' in gamesetup init: " + attribs.type);
	}

	if (attribs.serverName)
		g_ServerName = attribs.serverName;

	// Init the Cancel Button caption and tooltip
	var cancelButton = Engine.GetGUIObjectByName("cancelGame");
	if (!Engine.HasXmppClient())
		cancelButton.tooltip = translate("Return to the main menu.");
	else
		cancelButton.tooltip = translate("Return to the lobby.");
}

// Called after the map data is loaded and cached
function initMain()
{
	// Get default player data - remove gaia
	g_DefaultPlayerData = g_Settings.PlayerDefaults;
	g_DefaultPlayerData.shift();
	for (var i = 0; i < g_DefaultPlayerData.length; ++i)
		g_DefaultPlayerData[i].Civ = "random";

	initCivNameList();

	// Init map types
	var mapTypes = Engine.GetGUIObjectByName("mapTypeSelection");
	mapTypes.list = g_MapTypes.Title;
	mapTypes.list_data = g_MapTypes.Name;

	// Setup map filters - will appear in order they are added
	addFilter("default", translate("Default"), function(settings) { return settings && (settings.Keywords === undefined || !keywordTestOR(settings.Keywords, ["naval", "demo", "hidden"])); });
	addFilter("naval", translate("Naval Maps"), function(settings) { return settings && settings.Keywords !== undefined && keywordTestAND(settings.Keywords, ["naval"]); });
	addFilter("demo", translate("Demo Maps"), function(settings) { return settings && settings.Keywords !== undefined && keywordTestAND(settings.Keywords, ["demo"]); });
	addFilter("all", translate("All Maps"), function(settings) { return true; });

	// Populate map filters dropdown
	var mapFilters = Engine.GetGUIObjectByName("mapFilterSelection");
	mapFilters.list = g_MapFilters.map(mapFilter => mapFilter.name);
	mapFilters.list_data = g_MapFilters.map(mapFilter => mapFilter.id);
	g_GameAttributes.mapFilter = "default";

	// For singleplayer reduce the size of more options dialog by three options (cheats, rated game, observer late join = 90px)
	if (!g_IsNetworked)
	{
		Engine.GetGUIObjectByName("moreOptions").size = "50%-200 50%-195 50%+200 50%+160";
		Engine.GetGUIObjectByName("hideMoreOptions").size = "50%-70 310 50%+70 336";
	}
	// For non-lobby multiplayergames reduce the size of the dialog by one option (rated game, 30px)
	else if (g_IsNetworked && !Engine.HasXmppClient())
	{
		Engine.GetGUIObjectByName("moreOptions").size = "50%-200 50%-195 50%+200 50%+220";
		Engine.GetGUIObjectByName("hideMoreOptions").size = "50%-70 370 50%+70 396";
		Engine.GetGUIObjectByName("optionObserverLateJoin").size = "14 338 94% 366";
	}

	// Setup controls for host only
	if (g_IsController)
	{
		mapTypes.selected = g_MapTypes.Default;
		mapFilters.selected = 0;

		// Create a unique ID for this match, to be used for identifying the same game reports
		// for the lobby.
		g_GameAttributes.matchID = Engine.GetMatchID();

		initMapNameList();

		let playersArray = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1); // 1, 2, ..., MaxPlayers
		let numPlayersSelection = Engine.GetGUIObjectByName("numPlayersSelection");
		numPlayersSelection.list = playersArray;
		numPlayersSelection.list_data = playersArray;
		numPlayersSelection.selected = g_MaxPlayers - 1;

		var gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
		gameSpeed.hidden = false;
		Engine.GetGUIObjectByName("gameSpeedText").hidden = true;
		gameSpeed.list = g_GameSpeeds.Title;
		gameSpeed.list_data = g_GameSpeeds.Speed;
		gameSpeed.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.gameSpeed = g_GameSpeeds.Speed[this.selected];

			updateGameAttributes();
		};
		gameSpeed.selected = g_GameSpeeds.Default;

		var populationCaps = Engine.GetGUIObjectByName("populationCap");
		populationCaps.list = g_PopulationCapacities.Title;
		populationCaps.list_data = g_PopulationCapacities.Population;
		populationCaps.selected = g_PopulationCapacities.Default;
		populationCaps.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.settings.PopulationCap = g_PopulationCapacities.Population[this.selected];

			updateGameAttributes();
		};

		var startingResourcesL = Engine.GetGUIObjectByName("startingResources");
		startingResourcesL.list = g_StartingResources.Title;
		startingResourcesL.list_data = g_StartingResources.Resources;
		startingResourcesL.selected = g_StartingResources.Default;
		startingResourcesL.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.settings.StartingResources = g_StartingResources.Resources[this.selected];

			updateGameAttributes();
		};

		var ceasefireL = Engine.GetGUIObjectByName("ceasefire");
		ceasefireL.list = g_Ceasefire.Title;
		ceasefireL.list_data = g_Ceasefire.Duration;
		ceasefireL.selected = g_Ceasefire.Default;
		ceasefireL.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.settings.Ceasefire = g_Ceasefire.Duration[this.selected];

			updateGameAttributes();
		};

		var victoryConditions = Engine.GetGUIObjectByName("victoryCondition");
		victoryConditions.list = g_VictoryConditions.Title;
		victoryConditions.list_data = g_VictoryConditions.Name;
		victoryConditions.onSelectionChange = function() {
			if (this.selected != -1)
			{
				g_GameAttributes.settings.GameType = g_VictoryConditions.Name[this.selected];
				g_GameAttributes.settings.VictoryScripts = g_VictoryConditions.Scripts[this.selected];
			}

			updateGameAttributes();
		};
		victoryConditions.selected = g_VictoryConditions.Default;

		var mapSize = Engine.GetGUIObjectByName("mapSize");
		mapSize.list = g_MapSizes.LongName;
		mapSize.list_data = g_MapSizes.Tiles;
		mapSize.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.settings.Size = g_MapSizes.Tiles[this.selected];
			updateGameAttributes();
		};
		mapSize.selected = 0;

		Engine.GetGUIObjectByName("revealMap").onPress = function() {
			g_GameAttributes.settings.RevealMap = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("exploreMap").onPress = function() {
			g_GameAttributes.settings.ExploreMap = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("observerLateJoin").onPress = function() {
			g_GameAttributes.settings.ObserverLateJoin = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("disableTreasures").onPress = function() {
			g_GameAttributes.settings.DisableTreasures = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("lockTeams").onPress = function() {
			g_GameAttributes.settings.LockTeams = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("enableCheats").onPress = function() {
			g_GameAttributes.settings.CheatsEnabled = this.checked;
			updateGameAttributes();
		};

		Engine.GetGUIObjectByName("enableRating").onPress = function() {
			g_GameAttributes.settings.RatingEnabled = this.checked;
			Engine.SetRankedGame(this.checked);
			Engine.GetGUIObjectByName("enableCheats").enabled = !this.checked;
			Engine.GetGUIObjectByName("lockTeams").enabled = !this.checked;
			updateGameAttributes();
		};
	}
	else
	{
		// If we're a network client, disable all the map controls
		Engine.GetGUIObjectByName("mapTypeSelection").hidden = true;
		Engine.GetGUIObjectByName("mapTypeText").hidden = false;
		Engine.GetGUIObjectByName("mapFilterSelection").hidden = true;
		Engine.GetGUIObjectByName("mapFilterText").hidden = false;
		Engine.GetGUIObjectByName("mapSelectionText").hidden = false;
		Engine.GetGUIObjectByName("mapSelection").hidden = true;
		Engine.GetGUIObjectByName("victoryConditionText").hidden = false;
		Engine.GetGUIObjectByName("victoryCondition").hidden = true;
		Engine.GetGUIObjectByName("gameSpeedText").hidden = false;
		Engine.GetGUIObjectByName("gameSpeed").hidden = true;

		// Disable player and game options controls
		// TODO: Shouldn't players be able to choose their own assignment?
		for (let i = 0; i < g_MaxPlayers; ++i)
		{
			Engine.GetGUIObjectByName("playerAssignment["+i+"]").hidden = true;
			Engine.GetGUIObjectByName("playerCiv["+i+"]").hidden = true;
			Engine.GetGUIObjectByName("playerTeam["+i+"]").hidden = true;
		}

		Engine.GetGUIObjectByName("numPlayersSelection").hidden = true;
		Engine.GetGUIObjectByName("startGame").enabled = true;
	}

	// Set up multiplayer/singleplayer bits:
	if (!g_IsNetworked)
	{
		Engine.GetGUIObjectByName("chatPanel").hidden = true;
		Engine.GetGUIObjectByName("enableCheats").checked = true;
		g_GameAttributes.settings.CheatsEnabled = true;
	}
	else
	{
		Engine.GetGUIObjectByName("optionCheats").hidden = false;
		Engine.GetGUIObjectByName("enableCheats").checked = false;
		Engine.GetGUIObjectByName("optionObserverLateJoin").hidden = false;
		g_GameAttributes.settings.CheatsEnabled = false;
		// Setup ranked option if we are connected to the lobby.
		if (Engine.HasXmppClient())
		{
			Engine.GetGUIObjectByName("optionRating").hidden = false;
			Engine.GetGUIObjectByName("enableRating").checked = Engine.IsRankedGame();
			g_GameAttributes.settings.RatingEnabled = Engine.IsRankedGame();
			// We force locked teams and disabled cheats in ranked games.
			Engine.GetGUIObjectByName("enableCheats").enabled = !Engine.IsRankedGame();
			Engine.GetGUIObjectByName("lockTeams").enabled = !Engine.IsRankedGame();
		}
		if (g_IsController)
		{
			Engine.GetGUIObjectByName("enableCheatsText").hidden = true;
			Engine.GetGUIObjectByName("enableCheats").hidden = false;
			Engine.GetGUIObjectByName("observerLateJoinText").hidden = true;
			Engine.GetGUIObjectByName("observerLateJoin").hidden = false;

			if (Engine.HasXmppClient())
			{
				Engine.GetGUIObjectByName("enableRatingText").hidden = true;
				Engine.GetGUIObjectByName("enableRating").hidden = false;
			}
		}
	}

	// Settings for all possible player slots
	var boxSpacing = 32;
	for (let i = 0; i < g_MaxPlayers; ++i)
	{
		// Space player boxes
		var box = Engine.GetGUIObjectByName("playerBox["+i+"]");
		var boxSize = box.size;
		var h = boxSize.bottom - boxSize.top;
		boxSize.top = i * boxSpacing;
		boxSize.bottom = i * boxSpacing + h;
		box.size = boxSize;

		// Populate team dropdowns
		let team = Engine.GetGUIObjectByName("playerTeam["+i+"]");
		let teamsArray = Array(g_MaxTeams).fill(0).map((v, i) => i + 1); // 1, 2, ... MaxTeams
		team.list = [translateWithContext("team", "None")].concat(teamsArray); // "None", 1, 2, ..., maxTeams
		team.list_data = [-1].concat(teamsArray.map(team => team - 1)); // -1, 0, ..., (maxTeams-1)
		team.selected = 0;

		let playerSlot = i;	// declare for inner function use
		team.onSelectionChange = function() {
			if (this.selected != -1)
				g_GameAttributes.settings.PlayerData[playerSlot].Team = this.selected - 1;

			updateGameAttributes();
		};

		// Populate color drop-down lists.
		var colorPicker = Engine.GetGUIObjectByName("playerColorPicker["+i+"]");
		colorPicker.list = g_PlayerColors.map(color => '[color="' + color.r + ' ' + color.g + ' ' + color.b + '"] ■[/color]');
		colorPicker.list_data = g_PlayerColors.map((color, index) => index);
		colorPicker.selected = -1;
		colorPicker.onSelectionChange = function() { selectPlayerColor(playerSlot, this.selected); };

		// Set events
		var civ = Engine.GetGUIObjectByName("playerCiv["+i+"]");
		civ.onSelectionChange = function() {
			if ((this.selected != -1)&&(g_GameAttributes.mapType !== "scenario"))
				g_GameAttributes.settings.PlayerData[playerSlot].Civ = this.list_data[this.selected];

			updateGameAttributes();
		};
	}

	if (g_IsNetworked)
	{
		// For multiplayer, focus the chat input box by default
		Engine.GetGUIObjectByName("chatInput").focus();
	}
	else
	{
		// For single-player, focus the map list by default,
		// to allow easy keyboard selection of maps
		Engine.GetGUIObjectByName("mapSelection").focus();
	}

	if (g_IsController)
	{
		loadGameAttributes();
		// Sync g_GameAttributes to everyone.
		if (g_IsInGuiUpdate)
			warn("initMain() called while in GUI update");

		updateGameAttributes();
	}
}

function handleNetMessage(message)
{
	log("Net message: " + uneval(message));

	switch (message.type)
	{
	case "netstatus":
		switch (message.status)
		{
		case "disconnected":
			cancelSetup();
			reportDisconnect(message.reason);
			break;

		default:
			error("Unrecognised netstatus type " + message.status);
			break;
		}
		break;

	case "gamesetup":
		if (message.data) // (the host gets undefined data on first connect, so skip that)
		{
			g_GameAttributes = message.data;

			// Validate some settings for rated games.
			if (g_GameAttributes.settings.RatingEnabled)
			{
				// Cheats can never be on in rated games.
				g_GameAttributes.settings.CheatsEnabled = false;
				// Teams must be locked in rated games.
				g_GameAttributes.settings.LockTeams = true;
			}
		}

		onGameAttributesChange();
		break;

	case "players":
		var resetReady = false;
		var newPlayer = "";
		// Find and report all joinings/leavings
		for (var host in message.hosts)
		{
			if (!g_PlayerAssignments[host])
			{
				// If we have extra player slots and we are the controller, give the player an ID.
				if (g_IsController && message.hosts[host].player === -1 && g_AssignedCount < g_GameAttributes.settings.PlayerData.length)
					Engine.AssignNetworkPlayer(g_AssignedCount + 1, host);
				addChatMessage({ "type": "connect", "username": message.hosts[host].name });
				newPlayer = host;
			}
		}

		for (var host in g_PlayerAssignments)
		{
			if (!message.hosts[host])
			{
				addChatMessage({ "type": "disconnect", "guid": host });
				if (g_PlayerAssignments[host].player != -1)
					resetReady = true; // Observers shouldn't reset ready.
			}
		}

		// Update the player list
		g_PlayerAssignments = message.hosts;
		updatePlayerList();
		if (g_PlayerAssignments[newPlayer] && g_PlayerAssignments[newPlayer].player != -1)
			resetReady = true;

		if (resetReady)
			resetReadyData(); // Observers shouldn't reset ready.
		updateReadyUI();
		if (g_IsController)
			sendRegisterGameStanza();
		break;

	case "start":
		if (g_IsController && Engine.HasXmppClient())
		{
			let playerNames = Object.keys(g_PlayerAssignments).map(guid => g_PlayerAssignments[guid].name);
			Engine.SendChangeStateGame(playerNames.length, playerNames.join(", "));
		}
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes,
			"isNetworked" : g_IsNetworked,
			"playerAssignments": g_PlayerAssignments,
			"isController": g_IsController
		});
		break;

	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;

	case "kicked":
		addChatMessage({ "type": "system", "text": sprintf(translate("%(username)s has been kicked"), { "username": message.username })});
		break;

	case "banned":
		addChatMessage({ "type": "system", "text": sprintf(translate("%(username)s has been banned"), { "username": message.username })});
		break;

	// Singular client to host message
	case "ready":
		g_ReadyChanged -= 1;
		if (g_ReadyChanged < 1 && g_PlayerAssignments[message.guid].player != -1)
			addChatMessage({ "type": "ready", "guid": message.guid, "ready": +message.status == 1 });
		if (!g_IsController)
			break;
		g_PlayerAssignments[message.guid].status = +message.status == 1;
		Engine.SetNetworkPlayerStatus(message.guid, +message.status);
		updateReadyUI();
		break;

	default:
		error("Unrecognised net message type " + message.type);
	}
}

function getMapDisplayName(map)
{
	var mapData = loadMapData(map);
	if (!mapData || !mapData.settings || !mapData.settings.Name)
	{
		log("Map data missing in scenario '" + map + "' - likely unsupported format");
		return map;
	}

	return mapData.settings.Name;
}

function getMapPreview(map)
{
	var mapData = loadMapData(map);
	if (!mapData || !mapData.settings || !mapData.settings.Preview)
		return "nopreview.png";

	return mapData.settings.Preview;
}

// Get a setting if it exists or return default
function getSetting(settings, defaults, property)
{
	if (settings && (property in settings))
		return settings[property];

	// Use defaults
	if (defaults && (property in defaults))
		return defaults[property];

	return undefined;
}

/**
 * Initialize the dropdowns containing all selectable civs (including random).
 */
function initCivNameList()
{
	var civList = Object.keys(g_CivData).filter(civ => g_CivData[civ].SelectableInGameSetup).map(civ => ({ "name": g_CivData[civ].Name, "code": civ })).sort(sortNameIgnoreCase);
	var civListNames = civList.map(civ => civ.name);
	var civListCodes = civList.map(civ => civ.code);

	//  Add random civ to beginning of list
	civListNames.unshift('[color="orange"]' + translateWithContext("civilization", "Random") + '[/color]');
	civListCodes.unshift("random");

	// Update the dropdowns
	for (let i = 0; i < g_MaxPlayers; ++i)
	{
		var civ = Engine.GetGUIObjectByName("playerCiv["+i+"]");
		civ.list = civListNames;
		civ.list_data = civListCodes;
		civ.selected = 0;
	}
}

// Initialise the list control containing all the available maps
function initMapNameList()
{
	// Get a list of map filenames
	// TODO: Should verify these are valid maps before adding to list
	var mapSelectionBox = Engine.GetGUIObjectByName("mapSelection");
	var mapFiles;

	switch (g_GameAttributes.mapType)
	{
	case "scenario":
	case "skirmish":
		mapFiles = getXMLFileList(g_GameAttributes.mapPath);
		break;

	case "random":
		mapFiles = getJSONFileList(g_GameAttributes.mapPath);
		break;

	default:
		error("initMapNameList: Unexpected map type " + g_GameAttributes.mapType);
		return;
	}

	// Apply map filter, if any defined
	var mapList = [];
	for (var i = 0; i < mapFiles.length; ++i)
	{
		var file = g_GameAttributes.mapPath + mapFiles[i];
		var mapData = loadMapData(file);

		if (g_GameAttributes.mapFilter && mapData && testFilter(g_GameAttributes.mapFilter, mapData.settings))
			mapList.push({ "name": getMapDisplayName(file), "file": file });
	}

	// Alphabetically sort the list, ignoring case
	translateObjectKeys(mapList, ["name"]);
	mapList.sort(sortNameIgnoreCase);

	var mapListNames = mapList.map(map => map.name);
	var mapListFiles = mapList.map(map => map.file);

	// Select the default map
	var selected = mapListFiles.indexOf(g_GameAttributes.map);
	// Default to the first element if list is not empty and we can't find the one we searched for
	if (selected == -1 && mapList.length)
		selected = 0;

	// Update the list control
	if (g_GameAttributes.mapType == "random")
	{
		mapListNames.unshift("[color=\"orange\"]" + translateWithContext("map", "Random") + "[/color]");
		mapListFiles.unshift("random");
	}
	mapSelectionBox.list = mapListNames;
	mapSelectionBox.list_data = mapListFiles;
	mapSelectionBox.selected = selected;
}

function loadMapData(name)
{
	if (!name)
		return undefined;

	if (!g_MapData[name])
	{
		switch (g_GameAttributes.mapType)
		{
		case "scenario":
		case "skirmish":
			g_MapData[name] = Engine.LoadMapSettings(name);
			break;

		case "random":
			if (name == "random")
				// To be defined later.
				g_MapData[name] = { "settings": { "Name": "", "Description": "" } };
			else
				g_MapData[name] = Engine.ReadJSONFile(name+".json");
			break;

		default:
			error("loadMapData: Unexpected map type " + g_GameAttributes.mapType);
			return undefined;
		}
	}

	return g_MapData[name];
}

function loadGameAttributes()
{
	if (Engine.ConfigDB_GetValue("user", "persistmatchsettings") != "true")
		return;

	var settingsFile = g_IsNetworked ? g_MatchSettings_MP : g_MatchSettings_SP;
	if (!Engine.FileExists(settingsFile))
		return;

	var attrs = Engine.ReadJSONFile(settingsFile);
	if (!attrs || !attrs.settings)
		return;

	g_IsInGuiUpdate = true;

	var mapName = attrs.map || "";
	var mapSettings = attrs.settings;

	g_GameAttributes = attrs;

	// Assign new seeds and match id
	g_GameAttributes.matchID = Engine.GetMatchID();
	mapSettings.Seed = Math.floor(Math.random() * 65536);
	mapSettings.AISeed = Math.floor(Math.random() * 65536);

	// Ensure that cheats are enabled in singleplayer
	if (!g_IsNetworked)
		mapSettings.CheatsEnabled = true;

	// Replace unselectable civs with random civ
	var playerData = mapSettings.PlayerData;
	if (playerData && g_GameAttributes.mapType != "scenario")
		for (let i in playerData)
			if (!g_CivData[playerData[i].Civ] || !g_CivData[playerData[i].Civ].SelectableInGameSetup)
				playerData[i].Civ = "random";

	// Apply map settings
	var newMapData = loadMapData(mapName);
	if (newMapData && newMapData.settings)
	{
		for (var prop in newMapData.settings)
			mapSettings[prop] = newMapData.settings[prop];

		if (playerData)
			mapSettings.PlayerData = playerData;
	}

	if (mapSettings.PlayerData)
		sanitizePlayerData(mapSettings.PlayerData);

	var mapFilterSelection = Engine.GetGUIObjectByName("mapFilterSelection");
	mapFilterSelection.selected = mapFilterSelection.list_data.indexOf(attrs.mapFilter);

	Engine.GetGUIObjectByName("mapTypeSelection").selected = g_MapTypes.Name.indexOf(attrs.mapType);

	initMapNameList();

	var mapSelectionBox = Engine.GetGUIObjectByName("mapSelection");
	mapSelectionBox.selected = mapSelectionBox.list_data.indexOf(mapName);

	if (mapSettings.PopulationCap)
	{
		var populationCapBox = Engine.GetGUIObjectByName("populationCap");
		populationCapBox.selected = populationCapBox.list_data.indexOf(mapSettings.PopulationCap);
	}
	if (mapSettings.StartingResources)
	{
		var startingResourcesBox = Engine.GetGUIObjectByName("startingResources");
		startingResourcesBox.selected = startingResourcesBox.list_data.indexOf(mapSettings.StartingResources);
	}

	if (mapSettings.Ceasefire)
	{
		var ceasefireBox = Engine.GetGUIObjectByName("ceasefire");
		ceasefireBox.selected = ceasefireBox.list_data.indexOf(mapSettings.Ceasefire);
	}

	if (attrs.gameSpeed)
	{
		var gameSpeedBox = Engine.GetGUIObjectByName("gameSpeed");
		gameSpeedBox.selected = g_GameSpeeds.Speed.indexOf(attrs.gameSpeed);
	}

	g_GameAttributes.settings.RatingEnabled = Engine.HasXmppClient();
	Engine.SetRankedGame(g_GameAttributes.settings.RatingEnabled);
	Engine.GetGUIObjectByName("enableRating").checked = g_GameAttributes.settings.RatingEnabled;
	Engine.GetGUIObjectByName("enableCheats").enabled = !g_GameAttributes.settings.RatingEnabled;
	Engine.GetGUIObjectByName("lockTeams").enabled = !g_GameAttributes.settings.RatingEnabled;

	g_IsInGuiUpdate = false;

	onGameAttributesChange();
}

function saveGameAttributes()
{
	var attributes = Engine.ConfigDB_GetValue("user", "persistmatchsettings") == "true" ? g_GameAttributes : {};
	Engine.WriteJSONFile(g_IsNetworked ? g_MatchSettings_MP : g_MatchSettings_SP, attributes);
}

function sanitizePlayerData(playerData)
{
	// Remove gaia
	if (playerData.length && !playerData[0])
		playerData.shift();

	playerData.forEach((pData, index) => {
		pData.Color = pData.Color || g_PlayerColors[index];
		pData.Civ = pData.Civ || "random";
	});

	// Replace colors with the best matching color of PlayerDefaults
	if (g_GameAttributes.mapType != "scenario")
	{
		playerData.forEach((pData, index) => {
			let colorDistances = g_PlayerColors.map(color => colorDistance(color, pData.Color));
			let smallestDistance = colorDistances.find(distance => colorDistances.every(distance2 => (distance2 >= distance)));
			pData.Color = g_PlayerColors.find(color => colorDistance(color, pData.Color) == smallestDistance);
		});
	}

	ensureUniquePlayerColors(playerData);
}

function cancelSetup()
{
	if (g_IsController)
		saveGameAttributes();

	Engine.DisconnectNetworkGame();

	if (Engine.HasXmppClient())
	{
		Engine.LobbySetPlayerPresence("available");

		if (g_IsController)
			Engine.SendUnregisterGame();

		Engine.SwitchGuiPage("page_lobby.xml");
	}
	else
		Engine.SwitchGuiPage("page_pregame.xml");
}

function onTick()
{
	if (!g_Settings)
		return;

	// First tick happens before first render, so don't load yet
	if (g_LoadingState == 0)
	{
		g_LoadingState++;
	}
	else if (g_LoadingState == 1)
	{
		Engine.GetGUIObjectByName("loadingWindow").hidden = true;
		Engine.GetGUIObjectByName("setupWindow").hidden = false;
		initMain();
		g_LoadingState++;
	}
	else if (g_LoadingState == 2)
	{
		while (true)
		{
			var message = Engine.PollNetworkClient();
			if (!message)
				break;

			handleNetMessage(message);
		}
	}
}

// Called when user selects number of players
function selectNumPlayers(num)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
		return;

	// Network clients can't change number of players
	if (g_IsNetworked && !g_IsController)
		return;

	// Only meaningful for random maps
	if (g_GameAttributes.mapType != "random")
		return;

	// Update player data
	var pData = g_GameAttributes.settings.PlayerData;
	if (pData && num < pData.length)
	{
		// Remove extra player data
		g_GameAttributes.settings.PlayerData = pData.slice(0, num);
	}
	else
	{
		// Add player data from defaults
		for (var i = pData.length; i < num; ++i)
			g_GameAttributes.settings.PlayerData.push(g_DefaultPlayerData[i]);
	}

	// Some players may have lost their assigned slot
	for (var guid in g_PlayerAssignments)
	{
		var player = g_PlayerAssignments[guid].player;
		if (player > num)
		{
			if (g_IsNetworked)
				Engine.AssignNetworkPlayer(player, "");
			else
				g_PlayerAssignments = { "local": { "name": translate("You"), "player": 1, "civ": "", "team": -1, "ready": 0 } };
		}
	}

	updateGameAttributes();
}

/**
 *  Assigns the given color to that player.
 */
function selectPlayerColor(playerSlot, colorIndex)
{
	if (colorIndex == -1)
		return;

	var playerData = g_GameAttributes.settings.PlayerData;

	// If someone else has that color, give that player the old color
	var pData = playerData.find(pData => sameColor(g_PlayerColors[colorIndex], pData.Color));
	if (pData)
		pData.Color = playerData[playerSlot].Color;

	// Assign the new color
	playerData[playerSlot].Color = g_PlayerColors[colorIndex];

	// Ensure colors are not used twice after increasing the number of players
	ensureUniquePlayerColors(playerData);

	if (!g_IsInGuiUpdate)
		updateGameAttributes();
}

function ensureUniquePlayerColors(playerData)
{
	for (let i = playerData.length - 1; i >= 0; --i)
	{
		// If someone else has that color, assign an unused color
		if (playerData.some((pData, j) => i != j && sameColor(playerData[i].Color, pData.Color)))
			playerData[i].Color = g_PlayerColors.find(color => playerData.every(pData => !sameColor(color, pData.Color)));
	}
}

// Called when the user selects a map type from the list
function selectMapType(type)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
		return;

	// Network clients can't change map type
	if (g_IsNetworked && !g_IsController)
		return;

	// Reset game attributes
	g_GameAttributes.map = "";
	g_GameAttributes.mapType = type;

	// Clear old map data
	g_MapData = {};

	// Select correct path
	switch (g_GameAttributes.mapType)
	{
	case "scenario":
		g_GameAttributes.mapPath = "maps/scenarios/";
		g_GameAttributes.settings.AISeed = Math.floor(Math.random() * 65536);
		break;

	case "skirmish":
		g_GameAttributes.mapPath = "maps/skirmishes/";
		g_GameAttributes.settings = {
			"PlayerData": g_DefaultPlayerData.slice(0, 4),
			"Seed": Math.floor(Math.random() * 65536),
			"AISeed": Math.floor(Math.random() * 65536),
			"CheatsEnabled": g_GameAttributes.settings.CheatsEnabled
		};
		break;

	case "random":
		g_GameAttributes.mapPath = "maps/random/";
		g_GameAttributes.settings = {
			"PlayerData": g_DefaultPlayerData.slice(0, 4),
			"Seed": Math.floor(Math.random() * 65536),
			"AISeed": Math.floor(Math.random() * 65536),
			"CheatsEnabled": g_GameAttributes.settings.CheatsEnabled
		};
		break;

	default:
		error("selectMapType: Unexpected map type " + g_GameAttributes.mapType);
		return;
	}

	initMapNameList();

	updateGameAttributes();
}

function selectMapFilter(id)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
		return;

	// Network clients can't change map filter
	if (g_IsNetworked && !g_IsController)
		return;

	g_GameAttributes.mapFilter = id;

	initMapNameList();

	updateGameAttributes();
}

// Called when the user selects a map from the list
function selectMap(name)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
		return;

	// Network clients can't change map
	if (g_IsNetworked && !g_IsController)
		return;

	// Return if we have no map
	if (!name)
		return;

	// reset some map specific properties which are not necessarily redefined on each map
	for (let prop of ["TriggerScripts", "CircularMap", "Garrison"])
		if (g_GameAttributes.settings[prop] !== undefined)
			g_GameAttributes.settings[prop] = undefined;

	var mapData = loadMapData(name);
	var mapSettings = (mapData && mapData.settings ? deepcopy(mapData.settings) : {});

	// Reset victory conditions
	if (g_GameAttributes.mapType != "random")
	{
		let victoryIdx = mapSettings.GameType !== undefined && g_VictoryConditions.Name.indexOf(mapSettings.GameType) != -1 ? g_VictoryConditions.Name.indexOf(mapSettings.GameType) : g_VictoryConditions.Default;
		g_GameAttributes.settings.GameType = g_VictoryConditions.Name[victoryIdx];
		g_GameAttributes.settings.VictoryScripts = g_VictoryConditions.Scripts[victoryIdx];
	}

	if (mapSettings.PlayerData)
		sanitizePlayerData(mapSettings.PlayerData);

	// Copy any new settings
	g_GameAttributes.map = name;
	g_GameAttributes.script = mapSettings.Script;
	if (g_GameAttributes.map !== "random")
		for (var prop in mapSettings)
			g_GameAttributes.settings[prop] = mapSettings[prop];

	// Use default AI if the map doesn't specify any explicitly
	for (var i = 0; i < g_GameAttributes.settings.PlayerData.length; ++i)
	{
		if (!('AI' in g_GameAttributes.settings.PlayerData[i]))
			g_GameAttributes.settings.PlayerData[i].AI = g_DefaultPlayerData[i].AI;
		if (!('AIDiff' in g_GameAttributes.settings.PlayerData[i]))
			g_GameAttributes.settings.PlayerData[i].AIDiff = g_DefaultPlayerData[i].AIDiff;
	}

	// Reset player assignments on map change
	if (!g_IsNetworked)
	{	// Slot 1
		g_PlayerAssignments = { "local": { "name": translate("You"), "player": 1, "civ": "", "team": -1, "ready": 0 } };
	}
	else
	{
		var numPlayers = mapSettings.PlayerData ? mapSettings.PlayerData.length : g_GameAttributes.settings.PlayerData.length;

		for (var guid in g_PlayerAssignments)
		{	// Unassign extra players
			var player = g_PlayerAssignments[guid].player;

			if (player <= g_MaxPlayers && player > numPlayers)
				Engine.AssignNetworkPlayer(player, "");
		}
	}

	updateGameAttributes();
}

function launchGame()
{
	if (g_IsNetworked && !g_IsController)
	{
		error("Only host can start game");
		return;
	}

	if (!g_GameAttributes.map)
		return;

	saveGameAttributes();

	// Select random map
	if (g_GameAttributes.map == "random")
	{
		let victoryScriptsSelected = g_GameAttributes.settings.VictoryScripts;
		let gameTypeSelected = g_GameAttributes.settings.GameType;
		selectMap(Engine.GetGUIObjectByName("mapSelection").list_data[Math.floor(Math.random() *
			(Engine.GetGUIObjectByName("mapSelection").list.length - 1)) + 1]);
		g_GameAttributes.settings.VictoryScripts = victoryScriptsSelected;
		g_GameAttributes.settings.GameType = gameTypeSelected;
	}

	g_GameAttributes.settings.TriggerScripts = g_GameAttributes.settings.VictoryScripts.concat(g_GameAttributes.settings.TriggerScripts || []);

	// Prevent reseting the readystate
	g_GameStarted = true;

	g_GameAttributes.settings.mapType = g_GameAttributes.mapType;

	// Get a unique array of selectable cultures
	var cultures = Object.keys(g_CivData).filter(civ => g_CivData[civ].SelectableInGameSetup).map(civ => g_CivData[civ].Culture);
	cultures = cultures.filter((culture, index) => cultures.indexOf(culture) === index);

	// Determine random civs and botnames
	for (let i in g_GameAttributes.settings.PlayerData)
	{
		// Pick a random civ of a random culture
		let chosenCiv = g_GameAttributes.settings.PlayerData[i].Civ || "random";
		if (chosenCiv == "random")
		{
			let culture = cultures[Math.floor(Math.random() * cultures.length)];
			let civs = Object.keys(g_CivData).filter(civ => g_CivData[civ].Culture == culture);
			chosenCiv = civs[Math.floor(Math.random() * civs.length)];
		}
		g_GameAttributes.settings.PlayerData[i].Civ = chosenCiv;

		// Pick one of the available botnames for the chosen civ
		if (g_GameAttributes.mapType === "scenario" || !g_GameAttributes.settings.PlayerData[i].AI)
			continue;

		let chosenName = g_CivData[chosenCiv].AINames[Math.floor(Math.random() * g_CivData[chosenCiv].AINames.length)];

		if (!g_IsNetworked)
			chosenName = translate(chosenName);

		// Count how many players use the chosenName
		let usedName = g_GameAttributes.settings.PlayerData.filter(pData => pData.Name && pData.Name.indexOf(chosenName) !== -1).length;

		g_GameAttributes.settings.PlayerData[i].Name = !usedName ? chosenName : sprintf(translate("%(playerName)s %(romanNumber)s"), { "playerName": chosenName, "romanNumber": g_RomanNumbers[usedName+1] });
	}

	// Copy playernames from initial player assignment to the settings
	for (let guid in g_PlayerAssignments)
	{
		let player = g_PlayerAssignments[guid];
		if (player.player > 0)	// not observer or GAIA
			g_GameAttributes.settings.PlayerData[player.player - 1].Name = player.name;
	}

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		Engine.StartNetworkGame();
	}
	else
	{
		// Find the player ID which the user has been assigned to
		let playerID = -1;
		for (let i in g_GameAttributes.settings.PlayerData)
		{
			let assignBox = Engine.GetGUIObjectByName("playerAssignment["+i+"]");
			if (assignBox.list_data[assignBox.selected] == "local")
				playerID = +i+1;
		}

		Engine.StartGame(g_GameAttributes, playerID);
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes,
			"isNetworked" : g_IsNetworked,
			"playerAssignments": g_PlayerAssignments
		});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

function onGameAttributesChange()
{
	g_IsInGuiUpdate = true;

	// Don't set any attributes here, just show the changes in GUI

	var mapName = g_GameAttributes.map || "";
	var mapSettings = g_GameAttributes.settings;
	var numPlayers = mapSettings.PlayerData ? mapSettings.PlayerData.length : g_MaxPlayers;

	// Update some controls for clients
	if (!g_IsController)
	{
		var mapFilterSelection = Engine.GetGUIObjectByName("mapFilterSelection");
		var mapFilterId = mapFilterSelection.list_data.indexOf(g_GameAttributes.mapFilter);
		Engine.GetGUIObjectByName("mapFilterText").caption = mapFilterSelection.list[mapFilterId];
		Engine.GetGUIObjectByName("mapTypeText").caption = g_MapTypes.Title[g_MapTypes.Name.indexOf(g_GameAttributes.mapType)];
		var mapSelectionBox = Engine.GetGUIObjectByName("mapSelection");
		mapSelectionBox.selected = mapSelectionBox.list_data.indexOf(mapName);
		Engine.GetGUIObjectByName("mapSelectionText").caption = translate(getMapDisplayName(mapName));
		if (mapSettings.PopulationCap)
		{
			var populationCapBox = Engine.GetGUIObjectByName("populationCap");
			populationCapBox.selected = populationCapBox.list_data.indexOf(mapSettings.PopulationCap);
		}
		if (mapSettings.StartingResources)
		{
			var startingResourcesBox = Engine.GetGUIObjectByName("startingResources");
			startingResourcesBox.selected = startingResourcesBox.list_data.indexOf(mapSettings.StartingResources);
		}

		if (mapSettings.Ceasefire)
		{
			var ceasefireBox = Engine.GetGUIObjectByName("ceasefire");
			ceasefireBox.selected = ceasefireBox.list_data.indexOf(mapSettings.Ceasefire);
		}

		initMapNameList();
	}

	// Controls common to all map types
	var numPlayersSelection = Engine.GetGUIObjectByName("numPlayersSelection");
	var revealMap = Engine.GetGUIObjectByName("revealMap");
	var exploreMap = Engine.GetGUIObjectByName("exploreMap");
	var disableTreasures = Engine.GetGUIObjectByName("disableTreasures");
	var victoryCondition = Engine.GetGUIObjectByName("victoryCondition");
	var lockTeams = Engine.GetGUIObjectByName("lockTeams");
	var mapSize = Engine.GetGUIObjectByName("mapSize");
	var enableCheats = Engine.GetGUIObjectByName("enableCheats");
	var enableRating = Engine.GetGUIObjectByName("enableRating");
	var populationCap = Engine.GetGUIObjectByName("populationCap");
	var startingResources = Engine.GetGUIObjectByName("startingResources");
	var ceasefire = Engine.GetGUIObjectByName("ceasefire");
	var observerLateJoin = Engine.GetGUIObjectByName("observerLateJoin");

	var numPlayersText= Engine.GetGUIObjectByName("numPlayersText");
	var mapSizeDesc = Engine.GetGUIObjectByName("mapSizeDesc");
	var mapSizeText = Engine.GetGUIObjectByName("mapSizeText");
	var observerLateJoinText = Engine.GetGUIObjectByName("observerLateJoinText");
	var revealMapText = Engine.GetGUIObjectByName("revealMapText");
	var exploreMapText = Engine.GetGUIObjectByName("exploreMapText");
	var disableTreasuresText = Engine.GetGUIObjectByName("disableTreasuresText");
	var victoryConditionText = Engine.GetGUIObjectByName("victoryConditionText");
	var lockTeamsText = Engine.GetGUIObjectByName("lockTeamsText");
	var enableCheatsText = Engine.GetGUIObjectByName("enableCheatsText");
	var enableRatingText = Engine.GetGUIObjectByName("enableRatingText");
	var populationCapText = Engine.GetGUIObjectByName("populationCapText");
	var startingResourcesText = Engine.GetGUIObjectByName("startingResourcesText");
	var ceasefireText = Engine.GetGUIObjectByName("ceasefireText");
	var gameSpeedText = Engine.GetGUIObjectByName("gameSpeedText");
	var gameSpeedBox = Engine.GetGUIObjectByName("gameSpeed");

	// We have to check for undefined on these properties as not all maps define them.
	var sizeIdx = (mapSettings.Size !== undefined && g_MapSizes.Tiles.indexOf(mapSettings.Size) != -1 ? g_MapSizes.Tiles.indexOf(mapSettings.Size) : g_MapSizes.Default);
	var victoryIdx = mapSettings.GameType !== undefined && g_VictoryConditions.Name.indexOf(mapSettings.GameType) != -1 ? g_VictoryConditions.Name.indexOf(mapSettings.GameType) : g_VictoryConditions.Default;
	enableCheats.checked = (mapSettings.CheatsEnabled === undefined || !mapSettings.CheatsEnabled ? false : true);
	enableCheatsText.caption = (enableCheats.checked ? translate("Yes") : translate("No"));
	if (g_IsNetworked)
		Engine.GetGUIObjectByName("cheatWarningText").hidden = !enableCheats.checked;

	if (mapSettings.RatingEnabled !== undefined)
	{
		enableRating.checked = mapSettings.RatingEnabled;
		Engine.SetRankedGame(enableRating.checked);
		enableRatingText.caption = (enableRating.checked ? translate("Yes") : translate("No"));
		enableCheats.enabled = !enableRating.checked;
		lockTeams.enabled = !enableRating.checked;
	}
	else
		enableRatingText.caption = "Unknown";

	observerLateJoin.checked = g_GameAttributes.settings.ObserverLateJoin;
	observerLateJoinText.caption = observerLateJoin.checked ? translate("Yes") : translate("No");

	var speedIdx = g_GameAttributes.gameSpeed !== undefined && g_GameSpeeds.Speed.indexOf(g_GameAttributes.gameSpeed) != -1 ? g_GameSpeeds.Speed.indexOf(g_GameAttributes.gameSpeed) : g_GameSpeeds.Default;
	gameSpeedText.caption = g_GameSpeeds.Title[speedIdx];
	gameSpeedBox.selected = speedIdx;

	populationCap.selected = mapSettings.PopulationCap !== undefined && g_PopulationCapacities.Population.indexOf(mapSettings.PopulationCap) != -1 ? g_PopulationCapacities.Population.indexOf(mapSettings.PopulationCap) : g_PopulationCapacities.Default;
	populationCapText.caption = g_PopulationCapacities.Title[populationCap.selected];
	startingResources.selected = mapSettings.StartingResources !== undefined && g_StartingResources.Resources.indexOf(mapSettings.StartingResources) != -1 ? g_StartingResources.Resources.indexOf(mapSettings.StartingResources) : g_StartingResources.Default;
	startingResourcesText.caption = g_StartingResources.Title[startingResources.selected];
	ceasefire.selected = mapSettings.Ceasefire !== undefined && g_Ceasefire.Duration.indexOf(mapSettings.Ceasefire) != -1 ? g_Ceasefire.Duration.indexOf(mapSettings.Ceasefire) : g_Ceasefire.Default;
	ceasefireText.caption = g_Ceasefire.Title[ceasefire.selected];

	// Update map preview
	Engine.GetGUIObjectByName("mapPreview").sprite = "cropped:(0.78125,0.5859375)session/icons/mappreview/" + getMapPreview(mapName);

	// Hide/show settings depending on whether we can change them or not
	var updateDisplay = function(guiObjChg, guiObjDsp, chg) {
		guiObjChg.hidden = !chg;
		guiObjDsp.hidden = chg;
	};

	// Handle map type specific logic
	switch (g_GameAttributes.mapType)
	{
	case "random":
		mapSizeDesc.hidden = false;

		updateDisplay(numPlayersSelection, numPlayersText, g_IsController);
		updateDisplay(mapSize, mapSizeText, g_IsController);
		updateDisplay(revealMap, revealMapText, g_IsController);
		updateDisplay(exploreMap, exploreMapText, g_IsController);
		updateDisplay(disableTreasures, disableTreasuresText, g_IsController);
		updateDisplay(victoryCondition, victoryConditionText, g_IsController);
		updateDisplay(lockTeams, lockTeamsText, g_IsController);
		updateDisplay(populationCap, populationCapText, g_IsController);
		updateDisplay(startingResources, startingResourcesText, g_IsController);
		updateDisplay(ceasefire, ceasefireText, g_IsController);

		if (g_IsController)
		{
			//Host
			numPlayersSelection.selected = numPlayers - 1;
			mapSize.selected = sizeIdx;
			revealMap.checked = (mapSettings.RevealMap ? true : false);
			exploreMap.checked = (mapSettings.ExploreMap ? true : false);
			disableTreasures.checked = (mapSettings.DisableTreasures ? true : false);
			victoryCondition.selected = victoryIdx;
			lockTeams.checked = (mapSettings.LockTeams ? true : false);
		}
		else
		{
			// Client
			numPlayersText.caption = numPlayers;
			mapSizeText.caption = g_MapSizes.LongName[sizeIdx];
			revealMapText.caption = (mapSettings.RevealMap ? translate("Yes") : translate("No"));
			exploreMapText.caption = (mapSettings.ExporeMap ? translate("Yes") : translate("No"));
			disableTreasuresText.caption = (mapSettings.DisableTreasures ? translate("Yes") : translate("No"));
			victoryConditionText.caption = g_VictoryConditions.Title[victoryIdx];
			lockTeamsText.caption = (mapSettings.LockTeams ? translate("Yes") : translate("No"));
		}

		break;

	case "skirmish":
		mapSizeText.caption = translate("Default");
		numPlayersText.caption = numPlayers;
		numPlayersText.hidden = false;
		numPlayersSelection.hidden = true;
		mapSize.hidden = true;
		mapSizeText.hidden = true;
		mapSizeDesc.hidden = true;

		updateDisplay(revealMap, revealMapText, g_IsController);
		updateDisplay(exploreMap, exploreMapText, g_IsController);
		updateDisplay(disableTreasures, disableTreasuresText, g_IsController);
		updateDisplay(victoryCondition, victoryConditionText, g_IsController);
		updateDisplay(lockTeams, lockTeamsText, g_IsController);
		updateDisplay(populationCap, populationCapText, g_IsController);
		updateDisplay(startingResources, startingResourcesText, g_IsController);
		updateDisplay(ceasefire, ceasefireText, g_IsController);

		if (g_IsController)
		{
			//Host
			revealMap.checked = (mapSettings.RevealMap ? true : false);
			exploreMap.checked = (mapSettings.ExploreMap ? true : false);
			disableTreasures.checked = (mapSettings.DisableTreasures ? true : false);
			victoryCondition.selected = victoryIdx;
			lockTeams.checked = (mapSettings.LockTeams ? true : false);
		}
		else
		{
			// Client
			revealMapText.caption = (mapSettings.RevealMap ? translate("Yes") : translate("No"));
			exploreMapText.caption = (mapSettings.ExploreMap ? translate("Yes") : translate("No"));
			disableTreasuresText.caption = (mapSettings.DisableTreasures ? translate("Yes") : translate("No"));
			victoryConditionText.caption = g_VictoryConditions.Title[victoryIdx];
			lockTeamsText.caption = (mapSettings.LockTeams ? translate("Yes") : translate("No"));
		}

		break;


	case "scenario":
		// For scenario just reflect settings for the current map
		numPlayersSelection.hidden = true;
		numPlayersText.hidden = false;
		mapSize.hidden = true;
		mapSizeText.hidden = true;
		mapSizeDesc.hidden = true;
		revealMap.hidden = true;
		revealMapText.hidden = false;
		exploreMap.hidden = true;
		exploreMapText.hidden = false;
		disableTreasures.hidden = true;
		disableTreasuresText.hidden = false;
		victoryCondition.hidden = true;
		victoryConditionText.hidden = false;
		lockTeams.hidden = true;
		lockTeamsText.hidden = false;
		startingResources.hidden = true;
		startingResourcesText.hidden = false;
		populationCap.hidden = true;
		populationCapText.hidden = false;
		ceasefire.hidden = true;
		ceasefireText.hidden = false;

		numPlayersText.caption = numPlayers;
		mapSizeText.caption = translate("Default");
		revealMapText.caption = (mapSettings.RevealMap ? translate("Yes") : translate("No"));
		exploreMapText.caption = (mapSettings.ExploreMap ? translate("Yes") : translate("No"));
		disableTreasuresText.caption = translate("No");
		victoryConditionText.caption = g_VictoryConditions.Title[victoryIdx];
		lockTeamsText.caption = (mapSettings.LockTeams ? translate("Yes") : translate("No"));

		startingResourcesText.caption = translate("Determined by scenario");
		populationCapText.caption = translate("Determined by scenario");
		ceasefireText.caption = translate("Determined by scenario");
		break;

	default:
		error("onGameAttributesChange: Unexpected map type " + g_GameAttributes.mapType);
		return;
	}

	// Display map name
	if (mapName == "random")
	{
		var mapDisplayName  = translateWithContext("map", "Random");
		mapSettings.Description = markForTranslation("Randomly selects a map from the list");
	}
	else
		var mapDisplayName  = translate(getMapDisplayName(mapName));
	Engine.GetGUIObjectByName("mapInfoName").caption = mapDisplayName;

	// Load the description from the map file, if there is one
	var description = mapSettings.Description ? translate(mapSettings.Description) : translate("Sorry, no description available.");

	// Describe the number of players and the victory conditions
	var playerString = sprintf(translatePlural("%(number)s player. ", "%(number)s players. ", numPlayers), { "number": numPlayers });
	let victory = g_VictoryConditions.Title[victoryIdx];
	if (victoryIdx != g_VictoryConditions.Default)
		victory = "[color=\"orange\"]" + victory + "[/color]";
	playerString += translate("Victory Condition:") + " " + victory + ".\n\n" + description;

	for (let i = 0; i < g_MaxPlayers; ++i)
	{
		// Show only needed player slots
		Engine.GetGUIObjectByName("playerBox["+i+"]").hidden = (i >= numPlayers);

		// Show player data or defaults as necessary
		if (i >= numPlayers)
			continue;

		var pName = Engine.GetGUIObjectByName("playerName["+i+"]");
		var pAssignment = Engine.GetGUIObjectByName("playerAssignment["+i+"]");
		var pAssignmentText = Engine.GetGUIObjectByName("playerAssignmentText["+i+"]");
		var pCiv = Engine.GetGUIObjectByName("playerCiv["+i+"]");
		var pCivText = Engine.GetGUIObjectByName("playerCivText["+i+"]");
		var pTeam = Engine.GetGUIObjectByName("playerTeam["+i+"]");
		var pTeamText = Engine.GetGUIObjectByName("playerTeamText["+i+"]");
		var pColor = Engine.GetGUIObjectByName("playerColor["+i+"]");

		// Player data / defaults
		var pData = mapSettings.PlayerData ? mapSettings.PlayerData[i] : {};
		var pDefs = g_DefaultPlayerData ? g_DefaultPlayerData[i] : {};

		// Common to all game types
		var color = getSetting(pData, pDefs, "Color");
		pColor.sprite = "color:" + rgbToGuiColor(color) + " 100";
		pName.caption = translate(getSetting(pData, pDefs, "Name"));

		var team = getSetting(pData, pDefs, "Team");
		var civ = getSetting(pData, pDefs, "Civ");

		// Nobody but the controller can assign people
		pAssignmentText.hidden = g_IsController;
		pAssignment.hidden = !g_IsController;
		if (!pAssignment.list[0])
			pAssignmentText.caption = translate("Loading...");
		else
			pAssignmentText.caption = pAssignment.list[pAssignment.selected === -1 ? 0 : pAssignment.selected];

		// For clients or scenarios, hide some player dropdowns
		// TODO: Allow clients to choose their own civ and team
		if (!g_IsController || g_GameAttributes.mapType == "scenario")
		{
			pCivText.hidden = false;
			pCiv.hidden = true;
			pTeamText.hidden = false;
			pTeam.hidden = true;
			// Set text values
			if (civ == "random")
				pCivText.caption = "[color=\"orange\"]" + translateWithContext("civilization", "Random");
			else
				pCivText.caption = g_CivData[civ].Name;
			pTeamText.caption = (team !== undefined && team >= 0) ? team+1 : "-";
		}
		else if (g_GameAttributes.mapType != "scenario")
		{
			pCivText.hidden = true;
			pCiv.hidden = false;
			pTeamText.hidden = true;
			pTeam.hidden = false;
			// Set dropdown values
			pCiv.selected = (civ ? pCiv.list_data.indexOf(civ) : 0);
			pTeam.selected = (team !== undefined && team >= 0) ? team+1 : 0;
		}

		// Allow host to chose player colors on non-scenario maps
		const pColorPicker = Engine.GetGUIObjectByName("playerColorPicker["+i+"]");
		const pColorPickerHeading = Engine.GetGUIObjectByName("playerColorHeading");
		const canChangeColors = g_IsController && g_GameAttributes.mapType != "scenario";
		pColorPicker.hidden = !canChangeColors;
		pColorPickerHeading.hidden = !canChangeColors;
		if (canChangeColors)
			pColorPicker.selected = g_PlayerColors.findIndex(col => sameColor(col, color));
	}

	Engine.GetGUIObjectByName("mapInfoDescription").caption = playerString;

	g_IsInGuiUpdate = false;

	// Game attributes include AI settings, so update the player list
	updatePlayerList();

	// We should have everyone confirm that the new settings are acceptable.
	resetReadyData();
}

function updateGameAttributes()
{
	if (g_IsInGuiUpdate)
		return;

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		if (g_IsController && g_LoadingState >= 2)
			sendRegisterGameStanza();
	}
	else
		onGameAttributesChange();
}

function AIConfigCallback(ai)
{
	g_GameAttributes.settings.PlayerData[ai.playerSlot].AI = ai.id;
	g_GameAttributes.settings.PlayerData[ai.playerSlot].AIDiff = ai.difficulty;

	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else
		updatePlayerList();
}

function updatePlayerList()
{
	g_IsInGuiUpdate = true;

	var hostNameList = [];
	var hostGuidList = [];
	var assignments = [];
	var aiAssignments = {};
	var noAssignment;
	g_AssignedCount = 0;

	for (var guid in g_PlayerAssignments)
	{
		var name = g_PlayerAssignments[guid].name;
		var hostID = hostNameList.length;
		var player = g_PlayerAssignments[guid].player;

		hostNameList.push(name);
		hostGuidList.push(guid);
		assignments[player] = hostID;

		if (player != -1)
			g_AssignedCount++;
	}

	// Only enable start button if we have enough assigned players
	if (g_IsController)
		Engine.GetGUIObjectByName("startGame").enabled = (g_AssignedCount > 0);

	for (let ai of g_Settings.AIDescriptions)
	{
		if (ai.data.hidden)
		{
			// If the map uses a hidden AI then don't hide it
			var usedByMap = false;
			for (let i = 0; i < g_MaxPlayers; ++i)
				if (i < g_GameAttributes.settings.PlayerData.length &&
				    g_GameAttributes.settings.PlayerData[i].AI == ai.id)
				{
					usedByMap = true;
					break;
				}

			if (!usedByMap)
				continue;
		}
		// Give AI a different color so it stands out
		aiAssignments[ai.id] = hostNameList.length;
		hostNameList.push("[color=\"70 150 70 255\"]" + sprintf(translate("AI: %(ai)s"), { "ai": translate(ai.data.name) }));
		hostGuidList.push("ai:" + ai.id);
	}

	noAssignment = hostNameList.length;
	hostNameList.push("[color=\"140 140 140 255\"]" + translate("Unassigned"));
	hostGuidList.push("");

	for (let i = 0; i < g_MaxPlayers; ++i)
	{
		let playerSlot = i;
		let playerID = i+1; // we don't show Gaia, so first slot is ID 1

		var selection = assignments[playerID];

		var configButton = Engine.GetGUIObjectByName("playerConfig["+i+"]");
		configButton.hidden = true;

		// Look for valid player slots
		if (playerSlot >= g_GameAttributes.settings.PlayerData.length)
			continue;

		// If no human is assigned, look for an AI instead
		if (selection === undefined)
		{
			var aiId = g_GameAttributes.settings.PlayerData[playerSlot].AI;
			if (aiId)
			{
				// Check for a valid AI
				if (aiId in aiAssignments)
					selection = aiAssignments[aiId];
				else
				{
					g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
					warn("AI \"" + aiId + "\" not present. Defaulting to unassigned.");
				}
			}

			if (!selection)
				selection = noAssignment;

			// Since no human is assigned, show the AI config button
			if (g_IsController)
			{
				configButton.hidden = false;
				configButton.onpress = function() {
					Engine.PushGuiPage("page_aiconfig.xml", {
						"id": g_GameAttributes.settings.PlayerData[playerSlot].AI,
						"difficulty": g_GameAttributes.settings.PlayerData[playerSlot].AIDiff,
						"callback": "AIConfigCallback",
						"playerSlot": playerSlot // required by the callback function
					});
				};
			}
		}
		// There was a human, so make sure we don't have any AI left
		// over in their slot, if we're in charge of the attributes
		else if (g_IsController && g_GameAttributes.settings.PlayerData[playerSlot].AI && g_GameAttributes.settings.PlayerData[playerSlot].AI != "")
		{
			g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
			if (g_IsNetworked)
				Engine.SetNetworkGameAttributes(g_GameAttributes);
		}

		var assignBox = Engine.GetGUIObjectByName("playerAssignment["+i+"]");
		var assignBoxText = Engine.GetGUIObjectByName("playerAssignmentText["+i+"]");
		assignBox.list = hostNameList;
		assignBox.list_data = hostGuidList;
		if (assignBox.selected != selection)
			assignBox.selected = selection;
		assignBoxText.caption = hostNameList[selection];

		if (g_IsController)
			assignBox.onselectionchange = function() {
				if (g_IsInGuiUpdate)
					return;

				var guid = hostGuidList[this.selected];
				if (guid == "")
				{
					if (g_IsNetworked)
						// Unassign any host from this player slot
						Engine.AssignNetworkPlayer(playerID, "");
					// Remove AI from this player slot
					g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
				}
				else if (guid.substr(0, 3) == "ai:")
				{
					if (g_IsNetworked)
						// Unassign any host from this player slot
						Engine.AssignNetworkPlayer(playerID, "");
					// Set the AI for this player slot
					g_GameAttributes.settings.PlayerData[playerSlot].AI = guid.substr(3);
				}
				else
					swapPlayers(guid, playerSlot);

				if (g_IsNetworked)
					Engine.SetNetworkGameAttributes(g_GameAttributes);
				else
					updatePlayerList();
				updateReadyUI();
			};
	}

	g_IsInGuiUpdate = false;
}

function swapPlayers(guid, newSlot)
{
	// Player slots are indexed from 0 as Gaia is omitted.
	var newPlayerID = newSlot + 1;
	var playerID = g_PlayerAssignments[guid].player;

	// Attempt to swap the player or AI occupying the target slot,
	// if any, into the slot this player is currently in.
	if (playerID != -1)
	{
		for (var i in g_PlayerAssignments)
		{
			// Move the player in the destination slot into the current slot.
			if (g_PlayerAssignments[i].player == newPlayerID)
			{
				if (g_IsNetworked)
					Engine.AssignNetworkPlayer(playerID, i);
				else
					g_PlayerAssignments[i].player = playerID;
				break;
			}
		}

		// Transfer the AI from the target slot to the current slot.
		g_GameAttributes.settings.PlayerData[playerID - 1].AI = g_GameAttributes.settings.PlayerData[newSlot].AI;
	}

	if (g_IsNetworked)
		Engine.AssignNetworkPlayer(newPlayerID, guid);
	else
		g_PlayerAssignments[guid].player = newPlayerID;

	// Remove AI from this player slot
	g_GameAttributes.settings.PlayerData[newSlot].AI = "";
}

function submitChatInput()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = input.caption;
	if (!text.length)
		return;

	input.caption = "";

	if (executeNetworkCommand(text))
		return;

	Engine.SendNetworkChat(text);
}

function addChatMessage(msg)
{
	var username = "";
	if (msg.username)
		username = escapeText(msg.username);
	else if (msg.guid && g_PlayerAssignments[msg.guid])
		username = escapeText(g_PlayerAssignments[msg.guid].name);

	var message = "";
	if ("text" in msg && msg.text)
		message = escapeText(msg.text);

	// TODO: Maybe host should have distinct font/color?
	var color = "white";

	// Valid player who has been assigned - get player color
	if (msg.guid && g_PlayerAssignments[msg.guid] && g_PlayerAssignments[msg.guid].player != -1)
	{
		color = g_GameAttributes.settings.PlayerData[g_PlayerAssignments[msg.guid].player - 1].Color;

		// Enlighten playercolor to improve readability
		var [h, s, l] = rgbToHsl(color.r, color.g, color.b);
		var [r, g, b] = hslToRgb(h, s, Math.max(0.6, l));

		color = rgbToGuiColor({ "r": r, "g": g, "b": b });
	}

	var formatted;
	switch (msg.type)
	{
	case "connect":
		var formattedUsername = '[color="'+ color +'"]' + username + '[/color]';
		formatted = '[font="sans-bold-13"] ' + sprintf(translate("== %(message)s"), { "message": sprintf(translate("%(username)s has joined"), { "username": formattedUsername }) }) + '[/font]';
		break;

	case "disconnect":
		var formattedUsername = '[color="'+ color +'"]' + username + '[/color]';
		formatted = '[font="sans-bold-13"] ' + sprintf(translate("== %(message)s"), { "message": sprintf(translate("%(username)s has left"), { "username": formattedUsername }) }) + '[/font]';
		break;

	case "clientlist":
		formatted = sprintf(translate("Users: %(users)s"),
			// Translation: This comma is used for separating first to penultimate elements in an enumeration.
			{ "users": getUsernameList().join(translate(", ")) });
		break;

	case "system":
		formatted = '[font="sans-bold-13"] ' + sprintf(translate("== %(message)s"), { "message": msg.text }) + '[/font]';
		break;

	case "message":
		var formattedUsername = '[color="'+ color +'"]' + username + '[/color]';
		var formattedUsernamePrefix = '[font="sans-bold-13"]' + sprintf(translate("<%(username)s>"), { "username": formattedUsername }) + '[/font]';
		formatted = sprintf(translate("%(username)s %(message)s"), { "username": formattedUsernamePrefix, "message": message });
		break;

	case "ready":
		var formattedUsername = '[font="sans-bold-13"][color="' + color + '"]' + username + '[/color][/font]';
		if (msg.ready)
			formatted = ' ' + sprintf(translate("* %(username)s is ready!"), { "username": formattedUsername });
		else
			formatted = ' ' + sprintf(translate("* %(username)s is not ready."), { "username": formattedUsername });
		break;

	case "settings":
		formatted = '[font="sans-bold-13"] ' + sprintf(translate("== %(message)s"), { "message": translate('Game settings have been changed') }) + '[/font]';
		break;

	default:
		error("Invalid chat message " + uneval(msg));
		return;
	}

	g_ChatMessages.push(formatted);

	Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

function showMoreOptions(show)
{
	Engine.GetGUIObjectByName("moreOptionsFade").hidden = !show;
	Engine.GetGUIObjectByName("moreOptions").hidden = !show;
}

function toggleReady()
{
	g_IsReady = !g_IsReady;
	if (g_IsReady)
	{
		Engine.SendNetworkReady(1);
		Engine.GetGUIObjectByName("startGame").caption = translate("I'm not ready");
		Engine.GetGUIObjectByName("startGame").tooltip = translate("State that you are not ready to play.");
	}
	else
	{
		Engine.SendNetworkReady(0);
		Engine.GetGUIObjectByName("startGame").caption = translate("I'm ready!");
		Engine.GetGUIObjectByName("startGame").tooltip = translate("State that you are ready to play!");
	}
}

function updateReadyUI()
{
	if (!g_IsNetworked)
		return; // Disabled for single-player games.
	var isAI = new Array(g_MaxPlayers + 1);
	for (var i = 0; i < isAI.length; ++i)
		isAI[i] = true;
	var allReady = true;
	for (var guid in g_PlayerAssignments)
	{
		// We don't really care whether observers are ready.
		if (g_PlayerAssignments[guid].player == -1 || !g_GameAttributes.settings.PlayerData[g_PlayerAssignments[guid].player - 1])
			continue;
		var pData = g_GameAttributes.settings.PlayerData ? g_GameAttributes.settings.PlayerData[g_PlayerAssignments[guid].player - 1] : {};
		var pDefs = g_DefaultPlayerData ? g_DefaultPlayerData[g_PlayerAssignments[guid].player - 1] : {};
		isAI[g_PlayerAssignments[guid].player] = false;
		if (g_PlayerAssignments[guid].status || !g_IsNetworked)
			Engine.GetGUIObjectByName("playerName[" + (g_PlayerAssignments[guid].player - 1) + "]").caption = '[color="0 255 0"]' + translate(getSetting(pData, pDefs, "Name")) + '[/color]';
		else
		{
			Engine.GetGUIObjectByName("playerName[" + (g_PlayerAssignments[guid].player - 1) + "]").caption = translate(getSetting(pData, pDefs, "Name"));
			allReady = false;
		}
	}
	// AIs are always ready.
	for (let playerid = 0; playerid < g_MaxPlayers; ++playerid)
	{
		if (!g_GameAttributes.settings.PlayerData[playerid])
			continue;
		var pData = g_GameAttributes.settings.PlayerData ? g_GameAttributes.settings.PlayerData[playerid] : {};
		var pDefs = g_DefaultPlayerData ? g_DefaultPlayerData[playerid] : {};
		if (isAI[playerid + 1])
			Engine.GetGUIObjectByName("playerName[" + playerid + "]").caption = '[color="0 255 0"]' + translate(getSetting(pData, pDefs, "Name")) + '[/color]';
	}

	// The host is not allowed to start until everyone is ready.
	if (g_IsNetworked && g_IsController)
	{
		var startGameButton = Engine.GetGUIObjectByName("startGame");
		startGameButton.enabled = allReady;
		// Add a explanation on to the tooltip if disabled.
		var disabledIndex = startGameButton.tooltip.indexOf('Disabled');
		if (disabledIndex != -1 && allReady)
			startGameButton.tooltip = startGameButton.tooltip.substring(0, disabledIndex - 2);
		else if (disabledIndex == -1 && !allReady)
			startGameButton.tooltip = startGameButton.tooltip + " (Disabled until all players are ready)";
	}
}

function resetReadyData()
{
	if (g_GameStarted)
		return;

	if (g_ReadyChanged < 1)
		addChatMessage({ "type": "settings" });
	else if (g_ReadyChanged == 2 && !g_ReadyInit)
		return; // duplicate calls on init
	else
		g_ReadyInit = false;

	g_ReadyChanged = 2;
	if (!g_IsNetworked)
		g_IsReady = true;
	else if (g_IsController)
	{
		Engine.ClearAllPlayerReady();
		g_IsReady = true;
		Engine.SendNetworkReady(1);
	}
	else
	{
		g_IsReady = false;
		Engine.GetGUIObjectByName("startGame").caption = translate("I'm ready!");
		Engine.GetGUIObjectByName("startGame").tooltip = translate("State that you accept the current settings and are ready to play!");
	}
}
/**
 * Add a new maplist-filter.
 *
 * @param id {string} - Unique identifier
 * @param title {string} - Translated name to be displayed.
 * @param filterFunc {Object} - Function returning true if the provided map should be listed if that filter is chosen.
 */
function addFilter(id, title, filterFunc)
{
	if (!filterFunc instanceof Object)
	{
		error("Invalid map filter: " + title);
		return;
	}

	g_MapFilters.push({ "id": id, "name": title, "filter": filterFunc });
}

/**
 * Returns true if the given map will be shown when having selected the mapFilter given by id.
 *
 * @param id {string} - Specifies the mapfilter
 * @param mapSettings {Object}
 */
function testFilter(id, mapSettings)
{
	var mapFilter = g_MapFilters.find(mapFilter => mapFilter.id == id);

	if (!mapFilter)
	{
		error("Invalid map filter: " + id);
		return false;
	}

	return mapFilter.filter(mapSettings);
}

/**
 *  Returns true if the keywords contain all of the matches.
 *
 *  @param keywords {Array}
 *  @param matches {Array}
 */
function keywordTestAND(keywords, matches)
{
	if (!keywords || !matches)
		return false;

	return matches.every(match => keywords.indexOf(match) != -1);
}

/**
 *  Returns true if the keywords contain some of the matches.
 *
 *  @param keywords {Array}
 *  @param matches {Array}
 */
function keywordTestOR(keywords, matches)
{
	if (!keywords || !matches)
		return false;

	return matches.some(match => keywords.indexOf(match) != -1);
}

function sendRegisterGameStanza()
{
	if (!Engine.HasXmppClient())
		return;

	var selectedMapSize = Engine.GetGUIObjectByName("mapSize").selected;
	var selectedVictoryCondition = Engine.GetGUIObjectByName("victoryCondition").selected;

	// Map sizes only apply to random maps.
	if (g_GameAttributes.mapType == "random")
		var mapSize = Engine.GetGUIObjectByName("mapSize").list_data[selectedMapSize];
	else
		var mapSize = "Default";

	var victoryCondition = Engine.GetGUIObjectByName("victoryCondition").list[selectedVictoryCondition];
	var numberOfPlayers = Object.keys(g_PlayerAssignments).length;
	var playerNames = Object.keys(g_PlayerAssignments).map(guid => g_PlayerAssignments[guid].name);

	var nbp = numberOfPlayers ? numberOfPlayers : 1;
	var tnbp = g_GameAttributes.settings.PlayerData.length;

	var gameData = {
		"name": g_ServerName,
		"mapName": g_GameAttributes.map,
		"niceMapName": getMapDisplayName(g_GameAttributes.map),
		"mapSize": mapSize,
		"mapType": g_GameAttributes.mapType,
		"victoryCondition": victoryCondition,
		"nbp": nbp,
		"tnbp": tnbp,
		"players": playerNames.join(", ")
	};
	Engine.SendRegisterGame(gameData);
}
