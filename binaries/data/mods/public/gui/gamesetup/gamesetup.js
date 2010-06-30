// Name displayed for unassigned player slots
const NAME_UNASSIGNED = "[color=\"90 90 90 255\"][unassigned]";

// Is this is a networked game, or offline
var g_IsNetworked;

// Is this user in control of game settings (i.e. is a network server, or offline player)
var g_IsController;

// Are we currently updating the GUI in response to network messages instead of user input
// (and therefore shouldn't send further messages to the network)
var g_IsInGuiUpdate;

// Default single-player player assignments
var g_PlayerAssignments = { "local": { "name": "You", "player": 1 } };

// Default game setup attributes
var g_GameAttributes = { "map": "Latium" };

// Number of players for currently selected map
var g_MaxPlayers = 8;

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

	// If we're a network client, disable all the map controls
	// TODO: make them look visually disabled so it's obvious why they don't work
	if (!g_IsController)
	{
		getGUIObjectByName("mapSelection").enabled = false;
		for (var i = 0; i < g_MaxPlayers; ++i)
			getGUIObjectByName("playerAssignment["+i+"]").enabled = false;
	}

	updatePlayerList();
}

function onTick()
{
	if (g_IsNetworked)
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
	warn("Net message: "+uneval(message));

	switch (message.type)
	{
	case "gamesetup":
		if (message.data) // (the host gets undefined data on first connect, so skip that)
			g_GameAttributes = message.data;

		onGameAttributesChange();
		break;

	case "players":
		g_PlayerAssignments = message.hosts;
		updatePlayerList();
		break;

	case "start":
		Engine.PushGuiPage("page_loading.xml", { "attribs": g_GameAttributes });
		break;

	default:
		error("Unrecognised net message type "+message.type);
	}
}

// Initialise the list control containing all the available maps
function initMapNameList(object)
{
	var mapPath = "maps/scenarios/"

	// Get a list of map filenames
	var mapArray = buildDirEntList(mapPath, "*.xml", false);

	// Alphabetically sort the list, ignoring case
	mapArray.sort(function (x, y) {
		var lowerX = x.toLowerCase();
		var lowerY = y.toLowerCase();
		if (lowerX < lowerY) return -1;
		else if (lowerX > lowerY) return 1;
		else return 0;
	});

	// Remove the path and extension from each name, since we just want the filename
	var mapNames = [ n.substring(mapPath.length, n.length-4) for each (n in mapArray) ];

	// Select the default map
	var selected = mapNames.indexOf(g_GameAttributes.map);
	// Default to the first element if we can't find the one we searched for
	if (selected == -1)
		selected = 0;

	// Update the list control
	object.list = mapNames;
	object.selected = selected;
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
	var mapIdx = mapSelectionBox.list.indexOf(mapName);
	mapSelectionBox.selected = mapIdx;

	getGUIObjectByName("mapInfoName").caption = mapName;

	var description = "Sorry, no description available.";

	// TODO: we ought to load map descriptions from the map itself, somehow.
	// Just hardcode it now for testing.
	if (mapName == "Latium")
	{
		description = "2 players. A fertile coastal region which was the birthplace of the Roman Empire. Plentiful natural resources let you build up a city and experiment with the game’s features in relative peace. Some more description could go here if you want as long as it’s not too long and still fits on the screen.";
	}

	getGUIObjectByName("mapInfoDescription").caption = description;

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
					g_PlayerAssignments[hostGuidList[this.selected]].player = playerID;
					updatePlayerList();
				}
			};
		}
	}

	g_IsInGuiUpdate = false;
}
