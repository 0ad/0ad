////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
const DEFAULT_NETWORKED_MAP = "Acropolis 1";
const DEFAULT_OFFLINE_MAP = "Acropolis 1";

// TODO: Move these somewhere like simulation\data\game_types.json, Atlas needs them too
const VICTORY_TEXT = ["Conquest", "None"];
const VICTORY_DATA = ["conquest", "endless"];
const VICTORY_DEFAULTIDX = 0;
const POPULATION_CAP = ["50", "100", "150", "200", "250", "300", "Unlimited"];
const POPULATION_CAP_DATA = [50, 100, 150, 200, 250, 300, 10000];
const POPULATION_CAP_DEFAULTIDX = 5;
const STARTING_RESOURCES = ["Very Low", "Low", "Medium", "High", "Very High", "Deathmatch"];
const STARTING_RESOURCES_DATA = [100, 300, 500, 1000, 3000, 50000];
const STARTING_RESOURCES_DEFAULTIDX = 1;
// Max number of players for any map
const MAX_PLAYERS = 8;

////////////////////////////////////////////////////////////////////////////////////////////////

// Is this is a networked game, or offline
var g_IsNetworked;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;

// Are we currently updating the GUI in response to network messages instead of user input
// (and therefore shouldn't send further messages to the network)
var g_IsInGuiUpdate;

var g_PlayerAssignments = {};

// Default game setup attributes
var g_DefaultPlayerData = [];
var g_GameAttributes = {
	settings: {}
};

var g_MapSizes = {};

var g_AIs = [];

var g_ChatMessages = [];

// Data caches
var g_MapData = {};
var g_CivData = {};

var g_MapFilters = [];

// Warn about the AI's nonexistent naval map support.
var g_NavalWarning = "\n\n[font=\"serif-bold-12\"][color=\"orange\"]Warning:[/color][/font] \
The AI does not support naval maps and may cause severe performance issues. \
Naval maps are recommended to be played with human opponents only.";

// To prevent the display locking up while we load the map metadata,
// we'll start with a 'loading' message and switch to the main screen in the
// tick handler
var g_LoadingState = 0; // 0 = not started, 1 = loading, 2 = loaded

////////////////////////////////////////////////////////////////////////////////////////////////

function init(attribs)
{
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
		error("Unexpected 'type' in gamesetup init: "+attribs.type);
	}
}

// Called after the map data is loaded and cached
function initMain()
{
	// Load AI list and hide deprecated AIs
	g_AIs = Engine.GetAIs();

	// Sort AIs by displayed name
	g_AIs.sort(function (a, b) {
		return a.data.name < b.data.name ? -1 : b.data.name < a.data.name ? +1 : 0;
	});

	// Get default player data - remove gaia
	g_DefaultPlayerData = initPlayerDefaults();
	g_DefaultPlayerData.shift();
	for (var i = 0; i < g_DefaultPlayerData.length; i++)
		g_DefaultPlayerData[i].Civ = "random";
	
	g_MapSizes = initMapSizes();

	// Init civs
	initCivNameList();

	// Init map types
	var mapTypes = getGUIObjectByName("mapTypeSelection");
	mapTypes.list = ["Scenario","Random"];
	mapTypes.list_data = ["scenario","random"];

	// Setup map filters - will appear in order they are added
	addFilter("Default", function(settings) { return settings && !keywordTestOR(settings.Keywords, ["naval", "demo", "hidden"]); });
	addFilter("Naval Maps", function(settings) { return settings && keywordTestAND(settings.Keywords, ["naval"]); });
	addFilter("Demo Maps", function(settings) { return settings && keywordTestAND(settings.Keywords, ["demo"]); });
	addFilter("Old Maps", function(settings) { return !settings; });
	addFilter("All Maps", function(settings) { return true; });

	// Populate map filters dropdown
	var mapFilters = getGUIObjectByName("mapFilterSelection");
	mapFilters.list = getFilters();
	g_GameAttributes.mapFilter = "Default";

	// Setup controls for host only
	if (g_IsController)
	{
		mapTypes.selected = 0;
		mapFilters.selected = 0;

		initMapNameList();

		var numPlayersSelection = getGUIObjectByName("numPlayersSelection");
		var players = [];
		for (var i = 1; i <= MAX_PLAYERS; ++i)
			players.push(i);
		numPlayersSelection.list = players;
		numPlayersSelection.list_data = players;
		numPlayersSelection.selected = MAX_PLAYERS - 1;

		var populationCaps = getGUIObjectByName("populationCap");
		populationCaps.list = POPULATION_CAP;
		populationCaps.list_data = POPULATION_CAP_DATA;
		populationCaps.selected = POPULATION_CAP_DEFAULTIDX;
		populationCaps.onSelectionChange = function()
		{
			if (this.selected != -1)
			{
				g_GameAttributes.settings.PopulationCap = POPULATION_CAP_DATA[this.selected];
			}
			
			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		}
		
		var startingResourcesL = getGUIObjectByName("startingResources");
		startingResourcesL.list = STARTING_RESOURCES;
		startingResourcesL.list_data = STARTING_RESOURCES_DATA;
		startingResourcesL.selected = STARTING_RESOURCES_DEFAULTIDX;
		startingResourcesL.onSelectionChange = function()
		{
			if (this.selected != -1)
			{
				g_GameAttributes.settings.StartingResources = STARTING_RESOURCES_DATA[this.selected];
			}
			
			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		}
		
		var victoryConditions = getGUIObjectByName("victoryCondition");
		victoryConditions.list = VICTORY_TEXT;
		victoryConditions.list_data = VICTORY_DATA;
		victoryConditions.onSelectionChange = function()
		{	// Update attributes so other players can see change
			if (this.selected != -1)
			{
				g_GameAttributes.settings.GameType = VICTORY_DATA[this.selected];
			}

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};
		victoryConditions.selected = VICTORY_DEFAULTIDX;

		var mapSize = getGUIObjectByName("mapSize");
		mapSize.list = g_MapSizes.names;
		mapSize.list_data = g_MapSizes.tiles;
		mapSize.onSelectionChange = function()
		{	// Update attributes so other players can see change
			if (this.selected != -1)
			{
				g_GameAttributes.settings.Size = g_MapSizes.tiles[this.selected];
			}

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};

		getGUIObjectByName("revealMap").onPress = function()
		{	// Update attributes so other players can see change
			g_GameAttributes.settings.RevealMap = this.checked;

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};

		getGUIObjectByName("lockTeams").onPress = function()
		{	// Update attributes so other players can see change
			g_GameAttributes.settings.LockTeams = this.checked;

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};
		
		getGUIObjectByName("enableCheats").onPress = function()
		{	// Update attributes so other players can see change
			g_GameAttributes.settings.CheatsEnabled = this.checked;

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};
	}
	else
	{
		// If we're a network client, disable all the map controls
		// TODO: make them look visually disabled so it's obvious why they don't work
		getGUIObjectByName("mapTypeSelection").hidden = true;
		getGUIObjectByName("mapTypeText").hidden = false;
		getGUIObjectByName("mapFilterSelection").hidden = true;
		getGUIObjectByName("mapFilterText").hidden = false;
		getGUIObjectByName("mapSelectionText").hidden = false;
		getGUIObjectByName("mapSelection").hidden = true;
		getGUIObjectByName("victoryConditionText").hidden = false;
		getGUIObjectByName("victoryCondition").hidden = true;
		
		// Disable player and game options controls
		// TODO: Shouldn't players be able to choose their own assignment?
		for (var i = 0; i < MAX_PLAYERS; ++i)
		{
			getGUIObjectByName("playerAssignment["+i+"]").enabled = false;
			getGUIObjectByName("playerCiv["+i+"]").hidden = true;
			getGUIObjectByName("playerTeam["+i+"]").hidden = true;
		}

		getGUIObjectByName("numPlayersSelection").hidden = true;
	}

	// Set up multiplayer/singleplayer bits:
	if (!g_IsNetworked)
	{
		getGUIObjectByName("chatPanel").hidden = true;
		getGUIObjectByName("enableCheats").checked = true;
		g_GameAttributes.settings.CheatsEnabled = true;
	}
	else
	{
		getGUIObjectByName("enableCheatsDesc").hidden = false;
		getGUIObjectByName("enableCheats").checked = false;
		g_GameAttributes.settings.CheatsEnabled = false;
		if (g_IsController)
		{
			getGUIObjectByName("enableCheats").hidden = false;
		}
		else
		{
			getGUIObjectByName("enableCheatsText").hidden = false;
		}
	}

	// Settings for all possible player slots
	var boxSpacing = 32;
	for (var i = 0; i < MAX_PLAYERS; ++i)
	{
		// Space player boxes
		var box = getGUIObjectByName("playerBox["+i+"]");
		var boxSize = box.size;
		var h = boxSize.bottom - boxSize.top;
		boxSize.top = i * boxSpacing;
		boxSize.bottom = i * boxSpacing + h;
		box.size = boxSize;

		// Populate team dropdowns
		var team = getGUIObjectByName("playerTeam["+i+"]");
		team.list = ["None", "1", "2", "3", "4"];
		team.list_data = [-1, 0, 1, 2, 3];
		team.selected = 0;

		let playerSlot = i;	// declare for inner function use
		team.onSelectionChange = function()
		{	// Update team
			if (this.selected != -1)
			{
				g_GameAttributes.settings.PlayerData[playerSlot].Team = this.selected - 1;
			}

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};
		

		// Set events
		var civ = getGUIObjectByName("playerCiv["+i+"]");
		civ.onSelectionChange = function()
		{	// Update civ
			if ((this.selected != -1)&&(g_GameAttributes.mapType !== "scenario"))
			{
				g_GameAttributes.settings.PlayerData[playerSlot].Civ = this.list_data[this.selected];
			}

			if (!g_IsInGuiUpdate)
			{
				updateGameAttributes();
			}
		};
	}

	if (g_IsNetworked)
	{
		// For multiplayer, focus the chat input box by default
		getGUIObjectByName("chatInput").focus();
	}
	else
	{
		// For single-player, focus the map list by default,
		// to allow easy keyboard selection of maps
		getGUIObjectByName("mapSelection").focus();
	}
}

function handleNetMessage(message)
{
	log("Net message: "+uneval(message));

	switch (message.type)
	{
	case "netstatus":
		switch (message.status)
		{
		case "disconnected":
			Engine.DisconnectNetworkGame();
			Engine.PopGuiPage();
			reportDisconnect(message.reason);
			break;

		default:
			error("Unrecognised netstatus type "+message.status);
			break;
		}
		break;

	case "gamesetup":
		if (message.data) // (the host gets undefined data on first connect, so skip that)
		{
			g_GameAttributes = message.data;
		}

		onGameAttributesChange();
		break;

	case "players":
		// Find and report all joinings/leavings
		for (var host in message.hosts)
		{
			if (! g_PlayerAssignments[host])
			{
				addChatMessage({ "type": "connect", "username": message.hosts[host].name });
			}
		}
		for (var host in g_PlayerAssignments)
		{
			if (! message.hosts[host])
			{
				addChatMessage({ "type": "disconnect", "guid": host });
			}
		}
		// Update the player list
		g_PlayerAssignments = message.hosts;
		updatePlayerList();
		break;

	case "start":
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes,
			"isNetworked" : g_IsNetworked,
			"playerAssignments": g_PlayerAssignments
		});
		break;

	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;

	default:
		error("Unrecognised net message type "+message.type);
	}
}

// Get display name from map data
function getMapDisplayName(map)
{
	var mapData = loadMapData(map);

	if (!mapData || !mapData.settings || !mapData.settings.Name)
	{	// Give some msg that map format is unsupported
		log("Map data missing in scenario '"+map+"' - likely unsupported format");
		return map;
	}

	return mapData.settings.Name;
}

// Get display name from map data
function getMapPreview(map)
{
	var mapData = loadMapData(map);

	if (!mapData || !mapData.settings || !mapData.settings.Preview)
	{	// Give some msg that map format is unsupported
		return "nopreview.png";
	}

	return mapData.settings.Preview;
}

// Get a setting if it exists or return default
function getSetting(settings, defaults, property)
{
	if (settings && (property in settings))
	{
		return settings[property];
	}

	// Use defaults
	if (defaults && (property in defaults))
	{
		return defaults[property];
	}

	return undefined;
}

// Initialize the dropdowns containing all the available civs
function initCivNameList()
{
	// Cache civ data
	g_CivData = loadCivData();
	
	// Extract name/code, and skip civs that are explicitly disabled
	// (intended for unusable incomplete civs)
	var civList = [ 
		{ "name": civ.Name, "code": civ.Code } 
		for each (civ in g_CivData) 
			if (civ.SelectableInGameSetup !== false) 
	]; 
	
	// Alphabetically sort the list, ignoring case
	civList.sort(sortNameIgnoreCase);

	var civListNames = [ civ.name for each (civ in civList) ];
	var civListCodes = [ civ.code for each (civ in civList) ];

	//  Add random civ to beginning of list 
	civListNames.unshift("[color=\"orange\"]Random"); 
	civListCodes.unshift("random"); 
	
	// Update the dropdowns
	for (var i = 0; i < MAX_PLAYERS; ++i)
	{
		var civ = getGUIObjectByName("playerCiv["+i+"]");
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
	var mapSelectionBox = getGUIObjectByName("mapSelection")
	var mapFiles;

	switch (g_GameAttributes.mapType)
	{
	case "scenario":
		mapFiles = getXMLFileList(g_GameAttributes.mapPath);
		break;

	case "random":
		mapFiles = getJSONFileList(g_GameAttributes.mapPath);
		break;

	default:
		error("initMapNameList: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}

	// Apply map filter, if any defined
	var mapList = [];
	for (var i = 0; i < mapFiles.length; ++i)
	{
		var file = mapFiles[i];
		var mapData = loadMapData(file);

		if (g_GameAttributes.mapFilter && mapData && testFilter(g_GameAttributes.mapFilter, mapData.settings))
		{
			mapList.push({ "name": getMapDisplayName(file), "file": file });
		}
	}

	// Alphabetically sort the list, ignoring case
	mapList.sort(sortNameIgnoreCase);
	if (g_GameAttributes.mapType == "random")
		mapList.unshift({ "name": "[color=\"orange\"]Random[/color]", "file": "random" });
	
	var mapListNames = [ map.name for each (map in mapList) ];
	var mapListFiles = [ map.file for each (map in mapList) ];

	// Select the default map
	var selected = mapListFiles.indexOf(g_GameAttributes.map);
	// Default to the first element if list is not empty and we can't find the one we searched for
	if (selected == -1 && mapList.length)
	{
		selected = 0;
	}

	// Update the list control
	mapSelectionBox.list = mapListNames;
	mapSelectionBox.list_data = mapListFiles;
	mapSelectionBox.selected = selected;
}

function loadMapData(name)
{
	if (!name)
	{
		return undefined;
	}
	
	if (name == "random")
	{
		g_MapData[name] = {settings : {"Name" : "Random", "Description" : "Randomly selects a map from the list"}};
		return g_MapData[name];
	}

	if (!g_MapData[name])
	{
		switch (g_GameAttributes.mapType)
		{
		case "scenario":
			g_MapData[name] = Engine.LoadMapSettings(g_GameAttributes.mapPath+name);
			break;

		case "random":
			g_MapData[name] = parseJSONData(g_GameAttributes.mapPath+name+".json");
			break;

		default:
			error("loadMapData: Unexpected map type '"+g_GameAttributes.mapType+"'");
			return undefined;
		}
	}

	return g_MapData[name];
}

////////////////////////////////////////////////////////////////////////////////////////////////
// GUI event handlers

function cancelSetup()
{
	Engine.DisconnectNetworkGame();
}

function onTick()
{
	// First tick happens before first render, so don't load yet
	if (g_LoadingState == 0)
	{
		g_LoadingState++;
	}
	else if (g_LoadingState == 1)
	{
		getGUIObjectByName("loadingWindow").hidden = true;
		getGUIObjectByName("setupWindow").hidden = false;
		initMain();
		g_LoadingState++;
	}
	else if (g_LoadingState == 2)
	{
		while (true)
		{
			var message = Engine.PollNetworkClient();
			if (!message)
			{
				break;
			}
			handleNetMessage(message);
		}
	}
}

// Called when user selects number of players
function selectNumPlayers(num)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
	{
		return;
	}

	// Network clients can't change number of players
	if (g_IsNetworked && !g_IsController)
	{
		return;
	}

	// Only meaningful for random maps
	if (g_GameAttributes.mapType != "random")
	{
		return;
	}

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
		{
			g_GameAttributes.settings.PlayerData.push(g_DefaultPlayerData[i]);
		}
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
				g_PlayerAssignments = { "local": { "name": "You", "player": 1, "civ": "", "team": -1} };
		}
	}

	updateGameAttributes();
}

// Called when the user selects a map type from the list
function selectMapType(type)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
	{
		return;
	}

	// Network clients can't change map type
	if (g_IsNetworked && !g_IsController)
	{
		return;
	}

	// Reset game attributes
	g_GameAttributes.map = "";
	g_GameAttributes.mapType = type;

	// Clear old map data
	g_MapData = {};

	// Select correct path
	switch (g_GameAttributes.mapType)
	{
	case "scenario":
		// Set a default map
		// TODO: This should be remembered from the last session
		g_GameAttributes.map = (g_IsNetworked ? DEFAULT_NETWORKED_MAP : DEFAULT_OFFLINE_MAP);
		g_GameAttributes.mapPath = "maps/scenarios/";
		break;

	case "random":
		g_GameAttributes.mapPath = "maps/random/";
		g_GameAttributes.settings = {
			PlayerData: g_DefaultPlayerData.slice(0, 4),
			Seed: Math.floor(Math.random() * 65536),
			CheatsEnabled: g_GameAttributes.settings.CheatsEnabled
		};
		break;

	default:
		error("selectMapType: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}

	initMapNameList();

	updateGameAttributes();
}

function selectMapFilter(filterName)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
	{
		return;
	}

	// Network clients can't change map filter
	if (g_IsNetworked && !g_IsController)
	{
		return;
	}

	g_GameAttributes.mapFilter = filterName;

	initMapNameList();

	updateGameAttributes();
}

// Called when the user selects a map from the list
function selectMap(name)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
	{
		return;
	}

	// Network clients can't change map
	if (g_IsNetworked && !g_IsController)
	{
		return;
	}

	// Return if we have no map
	if (!name)
	{
		return;
	}

	var mapData = loadMapData(name);
	var mapSettings = (mapData && mapData.settings ? deepcopy(mapData.settings) : {});

	// Copy any new settings
	g_GameAttributes.map = name;
	g_GameAttributes.script = mapSettings.Script;
	if (mapData !== "Random")
		for (var prop in mapSettings)
			g_GameAttributes.settings[prop] = mapSettings[prop];
	
	// Use default AI if the map doesn't specify any explicitly
	for (var i = 0; i < g_GameAttributes.settings.PlayerData.length; ++i)
	{
		if (!('AI' in g_GameAttributes.settings.PlayerData[i]))
		{
			g_GameAttributes.settings.PlayerData[i].AI = g_DefaultPlayerData[i].AI;
		}
	}
	
	// Reset player assignments on map change
	if (!g_IsNetworked)
	{	// Slot 1
		g_PlayerAssignments = { "local": { "name": "You", "player": 1, "civ": "", "team": -1} };
	}
	else
	{
		var numPlayers = (mapSettings.PlayerData ? mapSettings.PlayerData.length : g_GameAttributes.settings.PlayerData.length);

		for (var guid in g_PlayerAssignments)
		{	// Unassign extra players
			var player = g_PlayerAssignments[guid].player;

			if (player <= MAX_PLAYERS && player > numPlayers)
			{
				Engine.AssignNetworkPlayer(player, "");
			}
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

	// Check that we have a map
	if (!g_GameAttributes.map)
	{
		return;
	}
	
	if (g_GameAttributes.map == "random")
		selectMap(getGUIObjectByName("mapSelection").list_data[Math.floor(Math.random() *
			(getGUIObjectByName("mapSelection").list.length - 1)) + 1]);

	g_GameAttributes.settings.mapType = g_GameAttributes.mapType;
	var numPlayers = g_GameAttributes.settings.PlayerData.length; 
	// Assign random civilizations to players with that choice 
	//  (this is synchronized because we're the host) 
	var cultures = [];
	for each (civ in g_CivData)
		if ((civ.Culture !== undefined)&&(cultures.indexOf(civ.Culture) < 0)&&(civ.SelectableInGameSetup == undefined)||(civ.SelectableInGameSetup))
			cultures.push(civ.Culture);
	var allcivs = new Array(cultures.length);
	for (var i = 0; i < allcivs.length; ++i)
		allcivs[i] = [];
	for each (civ in g_CivData)
		if ((civ.Culture !== undefined)&&(civ.SelectableInGameSetup == undefined)||(civ.SelectableInGameSetup))
			allcivs[cultures.indexOf(civ.Culture)].push(civ.Code);
	
	const romanNumbers = [undefined, "I", "II", "III", "IV", "V", "VI", "VII", "VIII"];
	for (var i = 0; i < numPlayers; ++i) 
	{ 
		civs = allcivs[Math.floor(Math.random()*allcivs.length)];
		
		if (g_GameAttributes.settings.PlayerData[i].Civ == "random") 
			g_GameAttributes.settings.PlayerData[i].Civ = civs[Math.floor(Math.random()*civs.length)]; 
		// Setting names for AI players. Check if the player is AI and the match is not a scenario
		if ((g_GameAttributes.mapType !== "scenario")&&(g_GameAttributes.settings.PlayerData[i].AI))
		{
			// Get the civ specific names
			if (g_CivData[g_GameAttributes.settings.PlayerData[i].Civ].AINames !== undefined)
			{
				var civAINames = shuffleArray(g_CivData[g_GameAttributes.settings.PlayerData[i].Civ].AINames);
			}
			else
			{
				var civAINames = [g_CivData[g_GameAttributes.settings.PlayerData[i].Civ].Name];
			}
			// Choose the name
			var usedName = 0;
			if (i < civAINames.length)
				var chosenName = civAINames[i];
			else 
				var chosenName = civAINames[Math.floor(Math.random() * civAINames.length)];
			for (var j = 0; j < numPlayers; ++j) 
				if (g_GameAttributes.settings.PlayerData[j].Name.indexOf(chosenName) !== -1)
					usedName++;
			
			// Assign civ specific names to AI players
			if (usedName)
				g_GameAttributes.settings.PlayerData[i].Name = chosenName + " " + romanNumbers[usedName+1];
			else
				g_GameAttributes.settings.PlayerData[i].Name = chosenName;
		}
	} 
	
	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		Engine.StartNetworkGame();
	}
	else
	{
		// Find the player ID which the user has been assigned to
		var numPlayers = g_GameAttributes.settings.PlayerData.length;
		var playerID = -1;
		for (var i = 0; i < numPlayers; ++i)
		{
			var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
			if (assignBox.list_data[assignBox.selected] == "local")
			{
				playerID = i+1;
			}
		}
		// Remove extra player data
		g_GameAttributes.settings.PlayerData = g_GameAttributes.settings.PlayerData.slice(0, numPlayers);

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
	var numPlayers = (mapSettings.PlayerData ? mapSettings.PlayerData.length : MAX_PLAYERS);

	// Update some controls for clients
	if (!g_IsController)
	{
		getGUIObjectByName("mapFilterText").caption = g_GameAttributes.mapFilter;
		var mapTypeSelection = getGUIObjectByName("mapTypeSelection");
		var idx = mapTypeSelection.list_data.indexOf(g_GameAttributes.mapType);
		getGUIObjectByName("mapTypeText").caption = mapTypeSelection.list[idx];
		var mapSelectionBox = getGUIObjectByName("mapSelection");
		mapSelectionBox.selected = mapSelectionBox.list_data.indexOf(mapName);
		getGUIObjectByName("mapSelectionText").caption = getMapDisplayName(mapName);
		var populationCapBox = getGUIObjectByName("populationCap");
		populationCapBox.selected = populationCapBox.list_data.indexOf(mapSettings.PopulationCap);
		var startingResourcesBox = getGUIObjectByName("startingResources");
		startingResourcesBox.selected = startingResourcesBox.list_data.indexOf(mapSettings.StartingResources);
		initMapNameList();
	}

	// Controls common to all map types
	var numPlayersSelection = getGUIObjectByName("numPlayersSelection");
	var revealMap = getGUIObjectByName("revealMap");
	var victoryCondition = getGUIObjectByName("victoryCondition");
	var lockTeams = getGUIObjectByName("lockTeams");
	var mapSize = getGUIObjectByName("mapSize");
	var enableCheats = getGUIObjectByName("enableCheats");
	var populationCap = getGUIObjectByName("populationCap");
	var startingResources = getGUIObjectByName("startingResources");
	
	var numPlayersText= getGUIObjectByName("numPlayersText");
	var mapSizeText = getGUIObjectByName("mapSizeText");
	var revealMapText = getGUIObjectByName("revealMapText");
	var victoryConditionText = getGUIObjectByName("victoryConditionText");
	var lockTeamsText = getGUIObjectByName("lockTeamsText");
	var enableCheatsText = getGUIObjectByName("enableCheatsText");
	var populationCapText = getGUIObjectByName("populationCapText");
	var startingResourcesText = getGUIObjectByName("startingResourcesText");
	
	var sizeIdx = (g_MapSizes.tiles.indexOf(mapSettings.Size) != -1 ? g_MapSizes.tiles.indexOf(mapSettings.Size) : g_MapSizes.default);
	var victoryIdx = (VICTORY_DATA.indexOf(mapSettings.GameType) != -1 ? VICTORY_DATA.indexOf(mapSettings.GameType) : VICTORY_DEFAULTIDX);
	enableCheats.checked = (g_GameAttributes.settings.CheatsEnabled === undefined || !g_GameAttributes.settings.CheatsEnabled ? false : true)
	enableCheatsText.caption = (enableCheats.checked ? "Yes" : "No");
	populationCap.selected = (POPULATION_CAP_DATA.indexOf(mapSettings.PopulationCap) != -1 ? POPULATION_CAP_DATA.indexOf(mapSettings.PopulationCap) : POPULATION_CAP_DEFAULTIDX);
	populationCapText.caption = POPULATION_CAP[populationCap.selected];
	startingResources.selected = (STARTING_RESOURCES_DATA.indexOf(mapSettings.StartingResources) != -1 ? STARTING_RESOURCES_DATA.indexOf(mapSettings.StartingResources) : STARTING_RESOURCES_DEFAULTIDX);
	startingResourcesText.caption = STARTING_RESOURCES[startingResources.selected];
	// Handle map type specific logic
	switch (g_GameAttributes.mapType)
	{
	case "random":
		if (g_IsController)
		{	//Host
			numPlayersSelection.selected = numPlayers - 1;
			numPlayersSelection.hidden = false;
			mapSize.hidden = false;
			revealMap.hidden = false;
			victoryCondition.hidden = false;
			lockTeams.hidden = false;
			populationCap.hidden = false;
			startingResources.hidden = false;
			
			numPlayersText.hidden = true;
			mapSizeText.hidden = true;
			revealMapText.hidden = true;
			victoryConditionText.hidden = true;
			lockTeamsText.hidden = true;
			populationCapText.hidden = true;
			startingResourcesText.hidden = true;
			
			// Update map preview
			getGUIObjectByName("mapPreview").sprite = "cropped:(0.78125,0.5859375)session/icons/mappreview/" + getMapPreview(mapName);
			
			mapSizeText.caption = "Map size:";
			mapSize.selected = sizeIdx;
			revealMapText.caption = "Reveal map:";
			revealMap.checked = (mapSettings.RevealMap ? true : false);

			victoryConditionText.caption = "Victory condition:";
			victoryCondition.selected = victoryIdx;
			lockTeamsText.caption = "Teams locked:";
			lockTeams.checked = (mapSettings.LockTeams ? true : false);
		}
		else
		{
			// Client
			numPlayersText.hidden = false;
			mapSizeText.hidden = false;
			revealMapText.hidden = false;
			victoryConditionText.hidden = false;
			lockTeamsText.hidden = false;
			populationCap.hidden = true;
			populationCapText.hidden = false;
			startingResources.hidden = true;
			startingResourcesText.hidden = false;
			// Update map preview
			getGUIObjectByName("mapPreview").sprite = "cropped:(0.78125,0.5859375)session/icons/mappreview/" + getMapPreview(mapName);

			numPlayersText.caption = numPlayers;
			mapSizeText.caption = g_MapSizes.names[sizeIdx];
			revealMapText.caption = (mapSettings.RevealMap ? "Yes" : "No");
			victoryConditionText.caption = VICTORY_TEXT[victoryIdx];
			lockTeamsText.caption = (mapSettings.LockTeams ? "Yes" : "No");
		}

		break;

	case "scenario":
		// For scenario just reflect settings for the current map
		numPlayersSelection.hidden = true;
		mapSize.hidden = true;
		revealMap.hidden = true;
		victoryCondition.hidden = true;
		lockTeams.hidden = true;
		numPlayersText.hidden = false;
		mapSizeText.hidden = false;
		revealMapText.hidden = false;
		victoryConditionText.hidden = false;
		lockTeamsText.hidden = false;
		populationCap.hidden = true;
		populationCapText.hidden = false;
		startingResources.hidden = true;
		startingResourcesText.hidden = false;
		
		// Update map preview
		getGUIObjectByName("mapPreview").sprite = "cropped:(0.78125,0.5859375)session/icons/mappreview/" + getMapPreview(mapName);
		numPlayersText.caption = numPlayers;
		mapSizeText.caption = "Default";
		revealMapText.caption = (mapSettings.RevealMap ? "Yes" : "No");
		victoryConditionText.caption = VICTORY_TEXT[victoryIdx];
		lockTeamsText.caption = (mapSettings.LockTeams ? "Yes" : "No");
		getGUIObjectByName("populationCap").selected = POPULATION_CAP_DEFAULTIDX;

		break;

	default:
		error("onGameAttributesChange: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}

	// Display map name
	getGUIObjectByName("mapInfoName").caption = getMapDisplayName(mapName);
	
	// Load the description from the map file, if there is one
	var description = mapSettings.Description || "Sorry, no description available.";

	if (g_GameAttributes.mapFilter == "Naval Maps")
		description += g_NavalWarning;

	// Describe the number of players
	var playerString = numPlayers + " " + (numPlayers == 1 ? "player" : "players") + ". ";

	for (var i = 0; i < MAX_PLAYERS; ++i)
	{
		// Show only needed player slots
		getGUIObjectByName("playerBox["+i+"]").hidden = (i >= numPlayers);

		// Show player data or defaults as necessary
		if (i < numPlayers)
		{
			var pName = getGUIObjectByName("playerName["+i+"]");
			var pCiv = getGUIObjectByName("playerCiv["+i+"]");
			var pCivText = getGUIObjectByName("playerCivText["+i+"]");
			var pTeam = getGUIObjectByName("playerTeam["+i+"]");
			var pTeamText = getGUIObjectByName("playerTeamText["+i+"]");
			var pColor = getGUIObjectByName("playerColour["+i+"]");
			
			// Player data / defaults
			var pData = mapSettings.PlayerData ? mapSettings.PlayerData[i] : {};
			var pDefs = g_DefaultPlayerData ? g_DefaultPlayerData[i] : {};

			// Common to all game types
			var color = iColorToString(getSetting(pData, pDefs, "Colour"));
			pColor.sprite = "colour:"+color+" 100";
			pName.caption = getSetting(pData, pDefs, "Name");

			var team = getSetting(pData, pDefs, "Team");
			var civ = getSetting(pData, pDefs, "Civ");
			
			// For clients or scenarios, hide some player dropdowns
			if (!g_IsController || g_GameAttributes.mapType == "scenario")
			{
				pCivText.hidden = false;
				pCiv.hidden = true;
				pTeamText.hidden = false;
				pTeam.hidden = true;
				// Set text values
				if (civ == "random")
				{
					pCivText.caption = "[color=\"orange\"]Random";
				}
				else
				{
					pCivText.caption = g_CivData[civ].Name;
				}
				pTeamText.caption = (team !== undefined && team >= 0) ? team+1 : "-";
			}
			else if (g_GameAttributes.mapType == "random")
			{
				pCivText.hidden = true;
				pCiv.hidden = false;
				pTeamText.hidden = true;
				pTeam.hidden = false;
				// Set dropdown values
				pCiv.selected = (civ ? pCiv.list_data.indexOf(civ) : 0);
				pTeam.selected = (team !== undefined && team >= 0) ? team+1 : 0;
			}
		}
	}

	getGUIObjectByName("mapInfoDescription").caption = playerString + description;

	g_IsInGuiUpdate = false;

	// Game attributes include AI settings, so update the player list
	updatePlayerList();
}

function updateGameAttributes()
{
	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	}
	else
	{
		onGameAttributesChange();
	}
}

function updatePlayerList()
{
	g_IsInGuiUpdate = true;

	var hostNameList = [];
	var hostGuidList = [];
	var assignments = [];
	var aiAssignments = {};
	var noAssignment;
	var assignedCount = 0;

	for (var guid in g_PlayerAssignments)
	{
		var name = g_PlayerAssignments[guid].name;
		var hostID = hostNameList.length;
		var player = g_PlayerAssignments[guid].player;

		hostNameList.push(name);
		hostGuidList.push(guid);
		assignments[player] = hostID;

		if (player != 255)
			assignedCount++;
	}

	// Only enable start button if we have enough assigned players
	if (g_IsController)
		getGUIObjectByName("startGame").enabled = (assignedCount > 0);

	for each (var ai in g_AIs)
	{
		if (ai.data.hidden) 
		{
			// If the map uses a hidden AI then don't hide it
			var usedByMap = false;
			for (var i = 0; i < MAX_PLAYERS; ++i) {
				if (i < g_GameAttributes.settings.PlayerData.length &&
				    g_GameAttributes.settings.PlayerData[i].AI == ai.id)
				{
					usedByMap = true;
				}
			}
			if (!usedByMap)
			{
				continue;
			}
		}
		// Give AI a different color so it stands out
		aiAssignments[ai.id] = hostNameList.length;
		hostNameList.push("[color=\"70 150 70 255\"]AI: " + ai.data.name);
		hostGuidList.push("ai:" + ai.id);
	}

	noAssignment = hostNameList.length;
	hostNameList.push("[color=\"140 140 140 255\"]Unassigned");
	hostGuidList.push("");

	for (var i = 0; i < MAX_PLAYERS; ++i)
	{
		let playerSlot = i;
		let playerID = i+1; // we don't show Gaia, so first slot is ID 1

		var selection = assignments[playerID];

		var configButton = getGUIObjectByName("playerConfig["+i+"]");
		configButton.hidden = true;

		// Look for valid player slots
		if (playerSlot < g_GameAttributes.settings.PlayerData.length)
		{
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
						warn("AI \""+aiId+"\" not present. Defaulting to unassigned.");
				}

				if (!selection)
					selection = noAssignment;

				// Since no human is assigned, show the AI config button
				if (g_IsController)
				{
					configButton.hidden = false;
					configButton.onpress = function()
					{
						Engine.PushGuiPage("page_aiconfig.xml", {
							ais: g_AIs,
							id: g_GameAttributes.settings.PlayerData[playerSlot].AI,
							callback: function(ai) {
								g_GameAttributes.settings.PlayerData[playerSlot].AI = ai.id;

								if (g_IsNetworked)
								{
									Engine.SetNetworkGameAttributes(g_GameAttributes);
								}
								else
								{
									updatePlayerList();
								}
							}
						});
					};
				}
			}
			else
			{
				// There was a human, so make sure we don't have any AI left
				// over in their slot, if we're in charge of the attributes
				if (g_IsController && g_GameAttributes.settings.PlayerData[playerSlot].AI && g_GameAttributes.settings.PlayerData[playerSlot].AI != "")
				{
					g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
					if (g_IsNetworked)
					{
						Engine.SetNetworkGameAttributes(g_GameAttributes);
					}
				}
			}

			var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
			assignBox.list = hostNameList;
			assignBox.list_data = hostGuidList;
			if (assignBox.selected != selection)
			{
				assignBox.selected = selection;
			}

			if (g_IsNetworked && g_IsController)
			{
				assignBox.onselectionchange = function ()
				{
					if (!g_IsInGuiUpdate)
					{
						var guid = hostGuidList[this.selected];
						if (guid == "")
						{
							// Unassign any host from this player slot
							Engine.AssignNetworkPlayer(playerID, "");
							// Remove AI from this player slot
							g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
						}
						else if (guid.substr(0, 3) == "ai:")
						{
							// Unassign any host from this player slot
							Engine.AssignNetworkPlayer(playerID, "");
							// Set the AI for this player slot
							g_GameAttributes.settings.PlayerData[playerSlot].AI = guid.substr(3);
						}
						else
							swapPlayers(guid, playerSlot);

						Engine.SetNetworkGameAttributes(g_GameAttributes);
					}
				};
			}
			else if (!g_IsNetworked)
			{
				assignBox.onselectionchange = function ()
				{
					if (!g_IsInGuiUpdate)
					{
						var guid = hostGuidList[this.selected];
						if (guid == "")
						{
							// Remove AI from this player slot
							g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
						}
						else if (guid.substr(0, 3) == "ai:")
						{
							// Set the AI for this player slot
							g_GameAttributes.settings.PlayerData[playerSlot].AI = guid.substr(3);
						}
						else
							swapPlayers(guid, playerSlot);

						updatePlayerList();
					}
				};
			}
		}
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
	if (playerID != 255)
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
	var input = getGUIObjectByName("chatInput");
	var text = input.caption;
	if (text.length)
	{
		Engine.SendNetworkChat(text);
		input.caption = "";
	}
}

function addChatMessage(msg)
{
	var username = escapeText(msg.username || g_PlayerAssignments[msg.guid].name);
	var message = escapeText(msg.text);

	// TODO: Maybe host should have distinct font/color?
	var color = "white";

	if (g_PlayerAssignments[msg.guid] && g_PlayerAssignments[msg.guid].player != 255)
	{	// Valid player who has been assigned - get player colour
		var player = g_PlayerAssignments[msg.guid].player - 1;
		var mapName = g_GameAttributes.map;
		var mapData = loadMapData(mapName);
		var mapSettings = (mapData && mapData.settings ? mapData.settings : {});
		var pData = mapSettings.PlayerData ? mapSettings.PlayerData[player] : {};
		var pDefs = g_DefaultPlayerData ? g_DefaultPlayerData[player] : {};

		color = iColorToString(getSetting(pData, pDefs, "Colour"));
	}

	var formatted;
	switch (msg.type)
	{
	case "connect":
		formatted = '[font="serif-bold-13"][color="'+ color +'"]' + username + '[/color][/font] [color="gold"]has joined[/color]';
		break;

	case "disconnect":
		formatted = '[font="serif-bold-13"][color="'+ color +'"]' + username + '[/color][/font] [color="gold"]has left[/color]';
		break;

	case "message":
		formatted = '[font="serif-bold-13"]<[color="'+ color +'"]' + username + '[/color]>[/font] ' + message;
		break;

	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	g_ChatMessages.push(formatted);

	getGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

function toggleMoreOptions()
{
	getGUIObjectByName("moreOptions").hidden = !getGUIObjectByName("moreOptions").hidden;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Basic map filters API

// Add a new map list filter
function addFilter(name, filterFunc)
{
	if (filterFunc instanceof Object)
	{	// Basic validity test
		var newFilter = {};
		newFilter.name = name;
		newFilter.filter = filterFunc;

		g_MapFilters.push(newFilter);
	}
	else
	{
		error("Invalid map filter: "+name);
	}
}

// Get array of map filter names
function getFilters()
{
	var filters = [];
	for (var i = 0; i < g_MapFilters.length; ++i)
	{
		filters.push(g_MapFilters[i].name);
	}

	return filters;
}

// Test map filter on given map settings object
function testFilter(name, mapSettings)
{
	for (var i = 0; i < g_MapFilters.length; ++i)
	{
		if (g_MapFilters[i].name == name)
		{	// Found filter
			return g_MapFilters[i].filter(mapSettings);
		}
	}

	error("Invalid map filter: "+name);
	return false;
}

// Test an array of keywords against a match array using AND logic
function keywordTestAND(keywords, matches)
{
	if (!keywords || !matches)
	{
		return false;
	}

	for (var m = 0; m < matches.length; ++m)
	{	// Fail on not match
		if (keywords.indexOf(matches[m]) == -1)
		{
			return false;
		}
	}
	return true;
}

// Test an array of keywords against a match array using OR logic
function keywordTestOR(keywords, matches)
{
	if (!keywords || !matches)
	{
		return false;
	}

	for (var m = 0; m < matches.length; ++m)
	{	// Success on match
		if (keywords.indexOf(matches[m]) != -1)
		{
			return true;
		}
	}
	return false;
}
