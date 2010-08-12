const GEOLOGY = "geology";
const FLORA = "flora";
const FAUNA = "fauna";
const SPECIAL = "special";

const GAIA = "Gaia"
const CART = "Cart";
const CELT = "Celt";
const HELE = "Hele";
const IBER = "Iber";
const PERS = "Pers";
const ROME = "Rome";

const CARTHAGINIANS = "Carthaginians";
const ROMANS = "Romans";
const HELLENES = "Hellenes";
const CELTS = "Celts";
const PERSIANS = "Persians";
const IBERIANS = "Iberians";

// Chat data
const maxNumChatLines = 20;
var g_ChatMessages = [];
var g_ChatTimers = [];

// Network Mode
var g_IsNetworked = false;

// Cache the basic player data (name, civ, color)
var g_Players = [];

// Cache dev-mode settings that are frequently or widely used
var g_DevSettings = {
	controlAll: false
};

function init(initData, hotloadData)
{
	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else 
	{
		// Starting for the first time:
		startMusic();
	}

	if (initData)
	{
		g_IsNetworked = initData.isNetworked; // Set network mode
		g_Players = getPlayerData(initData.playerAssignments); // Cache the player data
	}
	else // Needed for autostart loading option
	{
		g_Players = getPlayerData(null); 
	}

	onSimulationUpdate();
}

// Get the basic player data
function getPlayerData(playerAssignments)
{
	var players = [];

	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	if (!simState)
		return players;

	for (var i = 0; i < simState.players.length; i++)
	{
		var playerState = simState.players[i];
		if (!playerState)
			continue;

		var name = playerState.name;
		var civ = playerState.civ;
		var color = {"r": 255, "g": 255, "b": 255, "a": 255};
		color.r = playerState.color["r"]*255;
		color.g = playerState.color["g"]*255;
		color.b = playerState.color["b"]*255;
		color.a = playerState.color["a"]*255;

		var player = {"name": name, "civ": civ, "color": color};
		players.push(player);
	}
	
	var i = 1;
	if (playerAssignments)
	{
		for each (var playerAssignment in playerAssignments)
		{
			players[i].name = playerAssignment.name;
			i++;
		}
	}
	
	return players;
}

function leaveGame()
{
	stopMusic();
	endGame();
	Engine.SwitchGuiPage("page_pregame.xml");
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}

function handleNetMessage(message)
{
	log("Net message: "+uneval(message));

	switch (message.type)
	{
	case "netstatus":
		var obj = getGUIObjectByName("netStatus");
		switch (message.status)
		{
		case "waiting_for_players":
			obj.caption = "Waiting for other players to connect";
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "disconnected":
			obj.caption = "Connection to the server has been lost";
			obj.hidden = false;
			// TODO: we need to give players some way to exit
			break;

		default:
			error("Unrecognised netstatus type "+message.status);
			break;
		}
		break;
	case "chat":
		addChatMessage({ "type": "message", "username": message.username, "text": message.text });
		break;
	default:
		error("Unrecognised net message type "+message.type);
	}
}

function submitChatInput()
{
	toggleChatWindow();
	
	var input = getGUIObjectByName("chatInput");
	var text = input.caption;
	if (text.length)
	{
		if (g_IsNetworked)
			Engine.SendNetworkChat(text);
		else
			addChatMessage({ "type": "message", "username": g_Players[1].name, "text": text });

		input.caption = "";
	}
}

function addChatMessage(msg)
{
	// TODO: we ought to escape all values before displaying them,
	// to prevent people inserting colours and newlines etc

	var playerColor;

	for (var i = 0; i < g_Players.length; i++)
	{
		if (msg.username == g_Players[i].name)
		{
			playerColor  = g_Players[i].color.r + " " + g_Players[i].color.g + " " + g_Players[i].color.b;
			break
		}
	}

	var formatted;

	switch (msg.type)
	{
	case "connect":
		formatted = '<[font=\"serif-stroke-14\"][color="' + playerColor + '"]' + msg.username + '[/color][/font]> [color="64 64 64"]has joined[/color]';
		break;

	case "disconnect":
		formatted = '<[font=\"serif-stroke-14\"][color="' + playerColor + '"]' + msg.username + '[/color][/font]> [color="64 64 64"]has left[/color]';
		break;

	case "message":
		formatted = '<[font=\"serif-stroke-14\"][color="' + playerColor + '"]' + msg.username + '[/color][/font]> ' + msg.text;
		break;

	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	var timerExpiredFunction = function () { removeOldChatMessages(); }
	
	g_ChatMessages.push(formatted);
	g_ChatTimers.push(setTimeout(timerExpiredFunction, 30000));

	if (g_ChatMessages.length > maxNumChatLines)
	{
		clearTimeout(g_ChatTimers[0]);
		g_ChatTimers.shift();
		g_ChatMessages.shift();
	}

	getGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

function removeOldChatMessages()
{
	clearTimeout(g_ChatTimers[0]);
	g_ChatTimers.shift();
	g_ChatMessages.shift();
	getGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
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

	g_DevSettings.controlAll = getGUIObjectByName("devControlAll").checked;
	// TODO: at some point this controlAll needs to disable the simulation code's
	// player checks (once it has some player checks)

	updateCursor();

	// If the selection changed, we need to regenerate the sim display
	if (g_Selection.dirty)
	{
		onSimulationUpdate();

		// Display rally points for selected buildings
		Engine.GuiInterfaceCall("DisplayRallyPoint", { "entities": g_Selection.toList() });
	}

	// Run timers
	updateTimers();
}

function onSimulationUpdate()
{
	g_Selection.dirty = false;
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!simState)
		return;

	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails();
}

function updateDebug(simState)
{
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

	var text = uneval(simState);
	
	var selection = g_Selection.toList();
	if (selection.length)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
		if (entState)
		{
			var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
			text += "\n\n" + uneval(entState) + "\n\n" + uneval(template);
		}
	}

	debug.caption = text;
}

function updatePlayerDisplay(simState)
{
	var playerState = simState.players[Engine.GetPlayerID()];
	if (!playerState)
		return;

	getGUIObjectByName("resourceFood").caption = playerState.resourceCounts.food;
	getGUIObjectByName("resourceWood").caption = playerState.resourceCounts.wood;
	getGUIObjectByName("resourceStone").caption = playerState.resourceCounts.stone;
	getGUIObjectByName("resourceMetal").caption = playerState.resourceCounts.metal;
	getGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;
}

//-------------------------------- -------------------------------- -------------------------------- 
// Utility functions
//-------------------------------- -------------------------------- -------------------------------- 

function toTitleCase(string)
{
	if (string.length > 0)
		string = string.charAt(0).toUpperCase() + string.substring(1, string.length).toLowerCase();
	return string;
}

function isUnit(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			for (var i = 0; i < classes.length; i++)
				if ((classes[i] == "Organic") || (classes[i] == "Mechanical"))
					return true;
	}
	return false;
}

function isDefensive(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
		for (var i = 0; i < classes.length; i++)
			if (classes[i] == "Defensive")
				return true;
	}
	return false;
}

function damageTypesToTextStacked(dmg)
{
	if (!dmg)
		return "(None)";
	return dmg.hack + " Hack\n" + dmg.pierce + " Pierce\n" + dmg.crush + " Crush";
}

function damageTypesToText(dmg)
{
	if (!dmg)
		return "[font=\"serif-12\"](None)[/font]";
	
	var hackLabel = "[font=\"serif-12\"] Hack[/font]";
	var pierceLabel = "[font=\"serif-12\"] Pierce[/font]";
	var crushLabel = "[font=\"serif-12\"] Crush[/font]";
	var hackDamage = dmg.hack;
	var pierceDamage = dmg.pierce;
	var crushDamage = dmg.crush;
	
	var dmgArray = [];
	if (hackDamage) dmgArray.push(hackDamage + hackLabel);
	if (pierceDamage) dmgArray.push(pierceDamage + pierceLabel);
	if (crushDamage) dmgArray.push(crushDamage + crushLabel);

	return dmgArray.join(", ");
}

function getCommandCellId(commandName)
{
	switch (commandName)
	{
	case "delete":
		return 4;
	default:
		return -1;
	}
}

function getEntityCommandsList(entState)
{
	var commands = [];
	commands.push("delete");
	return commands;
}

function getRankCellId(entState)
{
	if (entState.template)
	{
		var templateName = entState.template;
		var endsWith = templateName.substring(templateName.length-2, templateName.length);
	
		if (isUnit(entState))
		{
			if (endsWith == "_e")
				return 0;
			else if (endsWith == "_a")
				return 1;
		}
	}
	return -1;
}

function getRankTitle(cellId)
{
	if (cellId == 0)
		return " (Elite)";
	else if (cellId == 1)
		return " (Advanced)";
	return "";
}

function getEntityCost(template)
{
	if (template.cost)
	{
		var costs = [];
		if (template.cost.food) costs.push("[font=\"serif-bold-13\"]Food:[/font] " + template.cost.food);
		if (template.cost.wood) costs.push("[font=\"serif-bold-13\"]Wood:[/font] " + template.cost.wood);
		if (template.cost.metal) costs.push("[font=\"serif-bold-13\"]Metal:[/font] " + template.cost.metal);
		if (template.cost.stone) costs.push("[font=\"serif-bold-13\"]Stone:[/font] " + template.cost.stone);
		if (costs.length)
			return costs.join(", ");
	}
	return "";
}

function getEntityName(template)
{
		return "[font=\"serif-bold-16\"]" + (template.name.specific || template.name.generic || "???") + "[/font]";
}

function getEntityNameWithGenericType(template)
{
		var name;
		if ((template.name.specific && template.name.generic) && (template.name.specific != template.name.generic))
			name = template.name.specific + " (" + template.name.generic + ")";
		else
			name = template.name.specific || template.name.generic || "???";
		
		return "[font=\"serif-bold-16\"]" + name + "[/font]";
}

function getFormalCivName(civ)
{
	switch (civ)
	{
	case CART:
		return "Carthaginians";
	case CELT:
		return "Celts";
	case HELE:
		return "Hellenes";
	case IBER:
		return "Iberians";
	case PERS:
		return "Persians";
	case ROME:
		return "Romans";
	default:
		return "Gaia";
	}
}
