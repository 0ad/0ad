// Is this is a networked game, or offline
var g_IsNetworked;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;

// Are we currently updating the GUI in response to network messages instead of user input
// (and therefore shouldn't send further messages to the network)
var g_IsInGuiUpdate;

var g_PlayerAssignments = {};

// Default game setup attributes
var g_GameAttributes = {
	mapType: "",
	map: "",
	mapPath: "",
	settings: {
		Size: 16,
		Seed: 0,
		BaseTerrain: "grass1_spring",
		BaseHeight: 0,
		PlayerData: [],
		RevealMap: false,
		LockTeams: false,
		GameType: "conquest"
	}
}

// Max number of players for any map
var g_MaxPlayers = 8;

// Number of players for currently selected map
var g_NumPlayers = 0;

var g_AIs = [];

var g_ChatMessages = [];

// Data caches
var g_MapData = {};
var g_CivData = {};

var g_MapFilters = [];
	

// To prevent the display locking up while we load the map metadata,
// we'll start with a 'loading' message and switch to the main screen in the
// tick handler
var g_LoadingState = 0; // 0 = not started, 1 = loading, 2 = loaded

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
	// Load AI list
	g_AIs = Engine.GetAIs();

	// Get default player data - remove gaia
	var pDefs = initPlayerDefaults();
	pDefs.shift();
	
	// Build player settings using defaults
	g_GameAttributes.settings.PlayerData = pDefs;
	
	// Init civs
	initCivNameList();
	
	// Init map types
	var mapTypes = getGUIObjectByName("mapTypeSelection");
	mapTypes.list = ["Scenario"];	// TODO: May offer saved game type for multiplayer games?
	mapTypes.list_data = ["scenario"];
	
	// Setup map filters - will appear in order they are added
	addFilter("Default", function(settings) { return settings && !keywordTestOR(settings.Keywords, ["demo", "hidden"]); });
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
		// Set a default map
		// TODO: This should be remembered from the last session
		if (!g_IsNetworked)
			g_GameAttributes.map = "Death Canyon";
		else
			g_GameAttributes.map = "Median Oasis";
		
		mapTypes.selected = 0;
		mapFilters.selected = 0;
		
		initMapNameList();
		
		var numPlayers = getGUIObjectByName("numPlayersSelection");
		var players = [];
		for (var i = 1; i <= g_MaxPlayers; ++i)
			players.push(i);
		numPlayers.list = players;
		numPlayers.list_data = players;
		numPlayers.selected = g_MaxPlayers - 1;
		
		var victoryConditions = getGUIObjectByName("victoryCondition");
		victoryConditions.list = ["Conquest", "None"];
		victoryConditions.list_data = ["conquest", "endless"];
		victoryConditions.onSelectionChange = function()
		{
			if (this.selected != -1)
				g_GameAttributes.settings.GameType = this.list_data[this.selected];
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
		};
		victoryConditions.selected = -1;
		
		var mapSize = getGUIObjectByName("mapSize");
		mapSize.list = ["Small", "Medium", "Large", "Huge"];
		mapSize.list_data = [16, 32, 48, 64];
		mapSize.onSelectionChange = function()
		{
			if (this.selected != -1)
				g_GameAttributes.settings.Size = parseInt(this.list_data[this.selected]);
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
		};
		mapSize.selected = -1;
		
		getGUIObjectByName("revealMap").onPress = function()
		{	// Update attributes so other players can see change
			g_GameAttributes.settings.RevealMap = this.checked;
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
		};
		
		getGUIObjectByName("lockTeams").onPress = function()
		{	// Update attributes so other players can see change
			g_GameAttributes.settings.LockTeams = this.checked;
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
		};
	}
	else
	{
		// If we're a network client, disable all the map controls
		// TODO: make them look visually disabled so it's obvious why they don't work
		getGUIObjectByName("mapTypeSelection").hidden = true;
		getGUIObjectByName("mapFilterSelection").hidden = true;
		getGUIObjectByName("mapSelection").enabled = false;
		
		// Disable player and game options controls
		// TODO: Shouldn't players be able to choose their own assignment?
		for (var i = 0; i < g_MaxPlayers; ++i)
			getGUIObjectByName("playerAssignment["+i+"]").enabled = false;
		
		getGUIObjectByName("numPlayersBox").hidden = true;
		
		// Disable "start game" button
		// TODO: Perhaps replace this with a "ready" button, and require host to wait?
		getGUIObjectByName("startGame").enabled = false;
	}
	
	// Set up offline-only bits:
	if (!g_IsNetworked)
	{
		getGUIObjectByName("chatPanel").hidden = true;
	}
	
	// Settings for all possible player slots
	var boxSpacing = 32;
	for (var i = 0; i < g_MaxPlayers; ++i)
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
				g_GameAttributes.settings.PlayerData[playerSlot].Team = this.selected - 1;
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
		};
		
		// Set events
		var civ = getGUIObjectByName("playerCiv["+i+"]");
		civ.onSelectionChange = function()
		{	// Update civ
			if (this.selected != -1)
				g_GameAttributes.settings.PlayerData[playerSlot].Civ = this.list_data[this.selected];
			
			if (!g_IsInGuiUpdate)
				onGameAttributesChange();
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
				break;
			handleNetMessage(message);
		}
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
			g_GameAttributes = message.data;

		onGameAttributesChange();
		break;

	case "players":
		// Find and report all joinings/leavings
		for (var host in message.hosts)
			if (! g_PlayerAssignments[host])
				addChatMessage({ "type": "connect", "username": message.hosts[host].name });
		for (var host in g_PlayerAssignments)
			if (! message.hosts[host])
				addChatMessage({ "type": "disconnect", "guid": host });
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
	
	if(!mapData || !mapData.settings || !mapData.settings.Name)
	{	// Give some msg that map format is unsupported
		log("Map data missing in scenario '"+map+"' - likely unsupported format");
		return map;
	}

	return mapData.settings.Name;
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

// Initialize the dropdowns containing all the available civs
function initCivNameList()
{
	// Cache civ data
	g_CivData = loadCivData();
	
	var civList = [ { "name": civ.Name, "code": civ.Code } for each (civ in g_CivData) ];

	// Alphabetically sort the list, ignoring case
	civList.sort(sortNameIgnoreCase);

	var civListNames = [ civ.name for each (civ in civList) ];
	var civListCodes = [ civ.code for each (civ in civList) ];

	// Update the dropdowns
	for (var i = 0; i < g_MaxPlayers; ++i)
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
			mapList.push({ "name": getMapDisplayName(file), "file": file });
	}
	
	// Alphabetically sort the list, ignoring case
	mapList.sort(sortNameIgnoreCase);

	var mapListNames = [ map.name for each (map in mapList) ];
	var mapListFiles = [ map.file for each (map in mapList) ];

	// Select the default map
	var selected = mapListFiles.indexOf(g_GameAttributes.map);
	// Default to the first element if list is not empty and we can't find the one we searched for
	if (selected == -1 && mapList.length)
		selected = 0;

	// Update the list control
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
	
	g_NumPlayers = num;
	
	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else 
		onGameAttributesChange();
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

	g_GameAttributes.mapType = type;
	
	// Clear old map data
	g_MapData = {};
	
	// Select correct path
	switch (g_GameAttributes.mapType)
	{
	case "scenario":
		g_GameAttributes.mapPath = "maps/scenarios/";
		break;
		
	case "random":
		g_GameAttributes.mapPath = "maps/random/";
		break;
		
	default:
		error("selectMapType: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}

	initMapNameList();

	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else
		onGameAttributesChange();
}

function selectMapFilter(filterName)
{
	// Avoid recursion
	if (g_IsInGuiUpdate)
		return;

	// Network clients can't change map filter
	if (g_IsNetworked && !g_IsController)
		return;
	
	g_GameAttributes.mapFilter = filterName;
	
	initMapNameList();

	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else
		onGameAttributesChange();
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

	g_GameAttributes.map = name;

	var mapData = loadMapData(name);
	var mapSettings = (mapData && mapData.settings ? mapData.settings : {});
	
	// Load map type specific settings (so we can overwrite them in the GUI)
	switch (g_GameAttributes.mapType)
	{
	case "scenario":
		g_NumPlayers = (mapSettings.PlayerData ? mapSettings.PlayerData.length : g_MaxPlayers);
		break;
		
	case "random":
		// Copy any new settings
		if (mapSettings.PlayerData)
			g_NumPlayers = mapSettings.PlayerData.length;
		
		g_GameAttributes.settings.Size = getSetting(mapSettings, g_GameAttributes.settings, "Size");
		g_GameAttributes.settings.BaseTerrain = getSetting(mapSettings, g_GameAttributes.settings, "BaseTerrain");
		g_GameAttributes.settings.BaseHeight = getSetting(mapSettings, g_GameAttributes.settings, "BaseHeight");
		g_GameAttributes.settings.RevealMap = getSetting(mapSettings, g_GameAttributes.settings, "RevealMap");
		g_GameAttributes.settings.GameType = getSetting(mapSettings, g_GameAttributes.settings, "GameType");
		break;
		
	default:
		error("selectMap: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}
	
	// Reset player assignments on map change
	if (!g_IsNetworked)
	{	// Slot 1
		g_PlayerAssignments = { "local": { "name": "You", "player": 1, "civ": "", "team": -1} };
		updatePlayerList();
	}
	else
	{
		for (var guid in g_PlayerAssignments)
		{	// Unassign extra players
			var player = g_PlayerAssignments[guid].player;

			if (player <= g_MaxPlayers && player > g_NumPlayers)
				Engine.AssignNetworkPlayer(player, "");
		}
	}
	
	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else
		onGameAttributesChange();
}

function onGameAttributesChange()
{
	g_IsInGuiUpdate = true;
	
	var mapName = g_GameAttributes.map || "";
	var mapData = loadMapData(mapName);
	var mapSettings = (mapData && mapData.settings ? mapData.settings : {});
		
	// Update some controls for clients
	if (!g_IsController)
	{
		var mapFilterHeading = getGUIObjectByName("mapFilterHeading");
		mapFilterHeading.caption = "Map Filter: "+g_GameAttributes.mapFilter;
		var mapTypeSelection = getGUIObjectByName("mapTypeSelection");
		var mapTypeHeading = getGUIObjectByName("mapTypeHeading");
		var idx = mapTypeSelection.list_data.indexOf(g_GameAttributes.mapType);
		mapTypeHeading.caption = "Match Type: "+mapTypeSelection.list[idx];
		var mapSelectionBox = getGUIObjectByName("mapSelection");
		mapSelectionBox.selected = mapSelectionBox.list_data.indexOf(mapName);
		
		initMapNameList();
		
		g_NumPlayers = (mapSettings.PlayerData ? mapSettings.PlayerData.length : g_MaxPlayers);
	}
	
	// Controls common to all map types
	var revealMap = getGUIObjectByName("revealMap");
	var victoryCondition = getGUIObjectByName("victoryCondition");
	var lockTeams = getGUIObjectByName("lockTeams");
	var mapSize = getGUIObjectByName("mapSize");
	var revealMapText = getGUIObjectByName("revealMapText");
	var victoryConditionText = getGUIObjectByName("victoryConditionText");
	var lockTeamsText = getGUIObjectByName("lockTeamsText");
	var mapSizeText = getGUIObjectByName("mapSizeText");
	var numPlayersBox = getGUIObjectByName("numPlayersBox");
		
	// Handle map type specific logic
	switch (g_GameAttributes.mapType)
	{
	case "random":
		
		g_GameAttributes.script = mapSettings.Script;
		var sizeIdx = mapSize.list_data.indexOf(g_GameAttributes.settings.Size.toString());
		
		// Show options for host/controller
		if (g_IsController)
		{
			getGUIObjectByName("numPlayersSelection").selected = g_NumPlayers - 1;
			numPlayersBox.hidden = false;
			mapSize.hidden = false;
			revealMap.hidden = false;
			victoryCondition.hidden = false;
			lockTeams.hidden = false;
			
			mapSizeText.caption = "Map size:";
			mapSize.selected = sizeIdx;
			revealMapText.caption = "Reveal map:";
			revealMap.checked = (g_GameAttributes.settings.RevealMap ? true : false);
			victoryConditionText.caption = "Victory condition:";
			victoryCondition.selected = victoryCondition.list_data.indexOf(g_GameAttributes.settings.GameType);
			lockTeamsText.caption = "Teams locked:";
			lockTeams.checked =  (g_GameAttributes.settings.LockTeams === undefined || g_GameAttributes.settings.LockTeams ? true : false);
		}
		else
		{
			mapSizeText.caption = "Map size: " + (mapSize.list[sizeIdx] !== undefined ? mapSize.list[sizeIdx] : "Default");
			revealMapText.caption = "Reveal map: " + (g_GameAttributes.settings.RevealMap ? "Yes" : "No");
			victoryConditionText.caption = "Victory condition: " + (g_GameAttributes.settings.GameType && g_GameAttributes.settings.GameType == "endless"  ?  "None" : "Conquest");
			lockTeamsText.caption = "Teams locked: " + (g_GameAttributes.settings.LockTeams === undefined || g_GameAttributes.settings.LockTeams ? "Yes" : "No");
		}
		
		break;
	
	case "saved":
	case "scenario":
		// For scenario just reflect settings for the current map
		numPlayersBox.hidden = true;
		mapSize.hidden = true;
		revealMap.hidden = true;
		victoryCondition.hidden = true;
		lockTeams.hidden = true;
		
		mapSizeText.caption = "Map size: Default";
		revealMapText.caption = "Reveal map: " + (mapSettings.RevealMap ? "Yes" : "No");
		victoryConditionText.caption = "Victory condition: " + (mapSettings.GameType && mapSettings.GameType == "endless"  ?  "None" : "Conquest");
		lockTeamsText.caption = "Teams locked: " + (mapSettings.LockTeams === undefined || mapSettings.LockTeams  ? "Yes" : "No");
		
		break;
		
	default:
		error("onGameAttributesChange: Unexpected map type '"+g_GameAttributes.mapType+"'");
		return;
	}
	
	// Display map name
	getGUIObjectByName("mapInfoName").caption = getMapDisplayName(mapName);

	// Load the description from the map file, if there is one
	var description = mapSettings.Description || "Sorry, no description available.";
		
	// Describe the number of players
	var playerString = g_NumPlayers + " " + (g_NumPlayers == 1 ? "player" : "players") + ". ";
	
	
	for (var i = 0; i < g_MaxPlayers; ++i)
	{
		// Show only needed player slots
		getGUIObjectByName("playerBox["+i+"]").hidden = (i >= g_NumPlayers);
		
		// Show player data or defaults as necessary
		if (i < g_NumPlayers)
		{
			var pName = getGUIObjectByName("playerName["+i+"]");
			var pCiv = getGUIObjectByName("playerCiv["+i+"]");
			var pCivText = getGUIObjectByName("playerCivText["+i+"]");
			var pTeam = getGUIObjectByName("playerTeam["+i+"]");
			var pTeamText = getGUIObjectByName("playerTeamText["+i+"]");
			var pColor = getGUIObjectByName("playerColour["+i+"]");

			// Player data / defaults
			var pData = mapSettings.PlayerData ? mapSettings.PlayerData[i] : {};
			var pDefs = g_GameAttributes.settings.PlayerData ? g_GameAttributes.settings.PlayerData[i] : {};
			
			// Common to all game types
			var color = iColorToString(getSetting(pData, pDefs, "Colour"));
			pColor.sprite = "colour:"+color+" 100";
			pName.caption = getSetting(pData, pDefs, "Name");
			
			var team = getSetting(pData, pDefs, "Team");
			var civ = getSetting(pData, pDefs, "Civ");
				
			if (g_GameAttributes.mapType == "scenario")
			{	
				// TODO: If scenario settings can be changed, handle that (using dropdowns rather than textboxes)
				pCivText.caption = g_CivData[civ].Name;
				pTeamText.caption = (team !== undefined && team >= 0) ? team+1 : "-";
				pCivText.hidden = false;
				pCiv.hidden = true;
				pTeamText.hidden = false;
				pTeam.hidden = true;
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

function launchGame()
{
	if (g_IsNetworked && !g_IsController)
	{
		error("Only host can start game");
		return;
	}
	
	// TODO: Generate seed here for random maps

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		Engine.StartNetworkGame();
	}
	else
	{
		// Find the player ID which the user has been assigned to
		var playerID = -1;
		for (var i = 0; i < g_NumPlayers; ++i)
		{
			var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
			if (assignBox.list_data[assignBox.selected] == "local")
				playerID = i+1;
		}
		// Remove extra player data
		g_GameAttributes.settings.PlayerData = g_GameAttributes.settings.PlayerData.slice(0, g_NumPlayers);

		Engine.StartGame(g_GameAttributes, playerID);
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes, 
			"isNetworked" : g_IsNetworked, 
			"playerAssignments": g_PlayerAssignments
		});
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
	
	for (var guid in g_PlayerAssignments)
	{
		var name = g_PlayerAssignments[guid].name;
		var hostID = hostNameList.length;
		var player = g_PlayerAssignments[guid].player;
		
		hostNameList.push(name);
		hostGuidList.push(guid);
		assignments[player] = hostID;
	}

	for each (var ai in g_AIs)
	{
		aiAssignments[ai.id] = hostNameList.length;
		hostNameList.push("[color=\"90 90 90 255\"]AI: " + ai.data.name);
		hostGuidList.push("ai:" + ai.id);
	}

	noAssignment = hostNameList.length;
	hostNameList.push("[color=\"90 90 90 255\"]Unassigned");
	hostGuidList.push("");

	for (var i = 0; i < g_MaxPlayers; ++i)
	{
		let playerSlot = i;
		let playerID = i+1; // we don't show Gaia, so first slot is ID 1

		var selection = assignments[playerID];

		var configButton = getGUIObjectByName("playerConfig["+i+"]");
		configButton.hidden = true;

		// If no human is assigned, look for an AI instead
		if (selection === undefined)
		{
			var aiId = g_GameAttributes.settings.PlayerData[playerSlot].AI;
			if (aiId)
				selection = aiAssignments[aiId];
			else
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
								Engine.SetNetworkGameAttributes(g_GameAttributes);
							else
								updatePlayerList();
						}
					});
				};
			}
		}
		else
		{
			// There was a human, so make sure we don't have any AI left
			// over in their slot, if we're in charge of the attributes
			if (g_IsController && g_GameAttributes.settings.PlayerData[playerSlot].AI != "")
			{
				g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
				if (g_IsNetworked)
					Engine.SetNetworkGameAttributes(g_GameAttributes);
			}
		}

		var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
		assignBox.list = hostNameList;
		assignBox.list_data = hostGuidList;
		if (assignBox.selected != selection)
			assignBox.selected = selection;

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
					{
						// Reassign the host to this player slot
						Engine.AssignNetworkPlayer(playerID, guid);
						// Remove AI from this player slot
						g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
					}
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
					{
						// Update the selected host's player ID
						g_PlayerAssignments[guid].player = playerID;
						// Remove AI from this player slot
						g_GameAttributes.settings.PlayerData[playerSlot].AI = "";
					}

					updatePlayerList();
				}
			};
		}
	}

	g_IsInGuiUpdate = false;
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
	var color = "0 0 0";
	
	if (g_PlayerAssignments[msg.guid] && g_PlayerAssignments[msg.guid].player != 255)
	{	// Valid player who has been assigned - get player colour
		var player = g_PlayerAssignments[msg.guid].player - 1;
		var mapName = g_GameAttributes.map;
		var mapData = loadMapData(mapName);
		var mapSettings = (mapData && mapData.settings ? mapData.settings : {});
		var pData = mapSettings.PlayerData ? mapSettings.PlayerData[player] : {};
		var pDefs = g_GameAttributes.settings.PlayerData ? g_GameAttributes.settings.PlayerData[player] : {};
		
		color = iColorToString(getSetting(pData, pDefs, "Colour"));
	}
	
	var formatted;
	switch (msg.type)
	{
	case "connect":
		formatted = '[font="serif-bold-13"][color="'+ color +'"]' + username + '[/color][/font] [color="64 64 64"]has joined[/color]';
		break;

	case "disconnect":
		formatted = '[font="serif-bold-13"][color="'+ color +'"]' + username + '[/color][/font] [color="64 64 64"]has left[/color]';
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
		return false;
	
	for (var m = 0; m < matches.length; ++m)
	{	// Fail on not match
		if (keywords.indexOf(matches[m]) == -1)
			return false;
	}
	return true;
}

// Test an array of keywords against a match array using OR logic
function keywordTestOR(keywords, matches)
{
	if (!keywords || !matches)
		return false;
	
	for (var m = 0; m < matches.length; ++m)
	{	// Success on match
		if (keywords.indexOf(matches[m]) != -1)
			return true;
	}
	return false;
}
