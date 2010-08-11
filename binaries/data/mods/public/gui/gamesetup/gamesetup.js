// Name displayed for unassigned player slots
const NAME_UNASSIGNED = "[color=\"90 90 90 255\"][unassigned]";

// Is this is a networked game, or offline
var g_IsNetworked;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;

// Are we currently updating the GUI in response to network messages instead of user input
// (and therefore shouldn't send further messages to the network)
var g_IsInGuiUpdate;

var g_PlayerAssignments = {};

// Default game setup attributes
var g_GameAttributes = { "map": ""};

// Number of players for currently selected map
var g_MaxPlayers = 8;

var g_ChatMessages = [];

// Cache of output from Engine.LoadMapData
var g_MapData = {};

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

	// Set a default map
	if (attribs.type == "offline")
		g_GameAttributes.map = "Arcadia";
	else
		g_GameAttributes.map = "Multiplayer_demo";

	initMapNameList(getGUIObjectByName("mapSelection"));

	// If we're a network client, disable all the map controls
	// TODO: make them look visually disabled so it's obvious why they don't work
	if (!g_IsController)
	{
		getGUIObjectByName("mapSelection").enabled = false;
		for (var i = 0; i < g_MaxPlayers; ++i)
			getGUIObjectByName("playerAssignment["+i+"]").enabled = false;
	}

	// Set up offline-only bits:
	if (!g_IsNetworked)
	{
		getGUIObjectByName("chatPanel").hidden = true;

		g_PlayerAssignments = { "local": { "name": "You", "player": 1 } };
	}

	updatePlayerList();

	getGUIObjectByName("chatInput").focus();
}

function cancelSetup()
{
	Engine.DisconnectNetworkGame();
}

function onTick()
{
	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;
		handleNetMessage(message);
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
				addChatMessage({ "type": "disconnect", "username": g_PlayerAssignments[host].name });
		// Update the player list
		g_PlayerAssignments = message.hosts;
		updatePlayerList();
		break;

	case "start":
		Engine.SwitchGuiPage("page_loading.xml", { "attribs": g_GameAttributes, "isNetworked" : g_IsNetworked, "playerAssignments": g_PlayerAssignments});
		break;

	case "chat":
		addChatMessage({ "type": "message", "username": message.username, "text": message.text });
		break;

	default:
		error("Unrecognised net message type "+message.type);
	}
}

// Convert map .xml filename into displayed name
function getMapDisplayName(filename)
{
	var name = filename;

	// Replace "_" with " " (so we can avoid spaces in filenames)
	name = name.replace(/_/g, " ");

	return name;
}

// Initialise the list control containing all the available maps
function initMapNameList(object)
{
	var mapPath = "maps/scenarios/"

	// Get a list of map filenames
	var mapFiles = buildDirEntList(mapPath, "*.xml", false);

	// Remove the path and extension from each name, since we just want the filename      
	mapFiles = [ n.substring(mapPath.length, n.length-4) for each (n in mapFiles) ];

	// Remove any files starting with "_" (these are for special maps used by the engine/editor)
	mapFiles = [ n for each (n in mapFiles) if (n[0] != "_") ];

	var mapList = [ { "name": getMapDisplayName(n), "file": n } for each (n in mapFiles) ];

	// Alphabetically sort the list, ignoring case
	mapList.sort(function (x, y) {
		var lowerX = x.name.toLowerCase();
		var lowerY = y.name.toLowerCase();
		if (lowerX < lowerY) return -1;
		else if (lowerX > lowerY) return 1;
		else return 0;
	});

	var mapListNames = [ n.name for each (n in mapList) ];
	var mapListFiles = [ n.file for each (n in mapList) ];

	// Select the default map
	var selected = mapListFiles.indexOf(g_GameAttributes.map);
	// Default to the first element if we can't find the one we searched for
	if (selected == -1)
		selected = 0;

	// Update the list control
	object.list = mapListNames;
	object.list_data = mapListFiles;
	object.selected = selected;
}

function loadMapData(name)
{
	if (!(name in g_MapData))
		g_MapData[name] = Engine.LoadMapData(name);

	return g_MapData[name];
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

	g_GameAttributes.map = name;

	if (g_IsNetworked)
		Engine.SetNetworkGameAttributes(g_GameAttributes);
	else
		onGameAttributesChange();
}

function onGameAttributesChange()
{
	g_IsInGuiUpdate = true;

	var mapName = g_GameAttributes.map;

	var mapSelectionBox = getGUIObjectByName("mapSelection");
	var mapIdx = mapSelectionBox.list_data.indexOf(mapName);
	mapSelectionBox.selected = mapIdx;

	getGUIObjectByName("mapInfoName").caption = getMapDisplayName(mapName);

	var mapData = loadMapData(mapName);
	var mapSettings = (mapData && mapData.settings ? mapData.settings : {});

	// Load the description from the map file, if there is one
	var description = mapSettings.Description || "Sorry, no description available.";
	
	// Describe the number of players
	var playerString = "";
	if (mapSettings.NumPlayers)
		playerString = mapSettings.NumPlayers + " " + (mapSettings.NumPlayers == 1 ? "player" : "players") + ". ";

	getGUIObjectByName("mapInfoDescription").caption = playerString + description;

	g_IsInGuiUpdate = false;
}

function launchGame()
{
	if (g_IsNetworked && !g_IsController)
	{
		error("Only host can start game");
		return;
	}

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		Engine.StartNetworkGame();
	}
	else
	{
		// Find the player ID which the user has been assigned to
		var playerID = -1;
		for (var i = 0; i < g_MaxPlayers; ++i)
		{
			var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
			if (assignBox.selected == 1)
				playerID = i+1;
		}
		Engine.StartGame(g_GameAttributes, playerID);
		Engine.PushGuiPage("page_loading.xml", { "attribs": g_GameAttributes });
	}
}

function updatePlayerList()
{
	g_IsInGuiUpdate = true;

	var boxSpacing = 32;

	var hostNameList = [NAME_UNASSIGNED];
	var hostGuidList = [""];
	var assignments = [];

	for (var guid in g_PlayerAssignments)
	{
		var name = g_PlayerAssignments[guid].name;
		var hostID = hostNameList.length;
		hostNameList.push(name);
		hostGuidList.push(guid);
		assignments[g_PlayerAssignments[guid].player] = hostID;
	}

	for (var i = 0; i < g_MaxPlayers; ++i)
	{
		var box = getGUIObjectByName("playerBox["+i+"]");
		var boxSize = box.size;
		var h = boxSize.bottom - boxSize.top;
		boxSize.top = i * boxSpacing;
		boxSize.bottom = i * boxSpacing + h;
		box.size = boxSize;

		getGUIObjectByName("playerName["+i+"]").caption = "Player "+(i+1);

		var assignBox = getGUIObjectByName("playerAssignment["+i+"]");
		assignBox.list = hostNameList;
		var selection = (assignments[i+1] || 0);
		if (assignBox.selected != selection)
			assignBox.selected = selection;

		let playerID = i+1;
		if (g_IsNetworked && g_IsController)
		{
			assignBox.onselectionchange = function ()
			{
				if (!g_IsInGuiUpdate)
					Engine.AssignNetworkPlayer(playerID, hostGuidList[this.selected]);
			};
		}
		else if (!g_IsNetworked)
		{
			assignBox.onselectionchange = function ()
			{
				if (!g_IsInGuiUpdate)
				{
					// If we didn't just select "unassigned", update the selected host's ID
					if (this.selected > 0)
						g_PlayerAssignments[hostGuidList[this.selected]].player = playerID;

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
	// TODO: we ought to escape all values before displaying them,
	// to prevent people inserting colours and newlines etc

	var formatted;
	switch (msg.type)
	{
	case "connect":
		formatted = '[font="serif-bold-13"][color="255 0 0"]' + msg.username + '[/color][/font] [color="64 64 64"]has joined[/color]';
		break;

	case "disconnect":
		formatted = '[font="serif-bold-13"][color="255 0 0"]' + msg.username + '[/color][/font] [color="64 64 64"]has left[/color]';
		break;

	case "message":
		formatted = '[font="serif-bold-13"]<[color="255 0 0"]' + msg.username + '[/color]>[/font] ' + msg.text;
		break;

	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	g_ChatMessages.push(formatted);

	getGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}
