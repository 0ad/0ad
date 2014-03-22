// Chat data
const CHAT_TIMEOUT = 30000;
const MAX_NUM_CHAT_LINES = 20;
var chatMessages = [];
var chatTimers = [];

// Notification Data
const NOTIFICATION_TIMEOUT = 10000;
const MAX_NUM_NOTIFICATION_LINES = 3;
var notifications = [];
var notificationsTimers = [];
var cheats = getCheatsData();

function getCheatsData()
{
	var cheats = {};
	var cheatFileList = getJSONFileList("simulation/data/cheats/");
	for each (var fileName in cheatFileList)
	{
		var currentCheat = parseJSONData("simulation/data/cheats/"+fileName+".json");
		if (Object.keys(cheats).indexOf(currentCheat.Name) !== -1)
			warn("Cheat name '"+currentCheat.Name+"' is already present");
		else
			cheats[currentCheat.Name] = currentCheat.Data;
	}
	return cheats;
}

// Notifications
function handleNotifications()
{
	var notification = Engine.GuiInterfaceCall("GetNextNotification");

	if (!notification)
		return;
	
	if (notification.type === undefined)
		notification.type = "text";

	// Handle chat notifications specially
	if (notification.type == "chat")
	{
		var guid = findGuidForPlayerID(g_PlayerAssignments, notification.player);
		if (guid == undefined)
		{
			addChatMessage({
				"type": "message",
				"guid": -1,
				"player": notification.player,
				"text": notification.message
			});
		} else {
			addChatMessage({
				"type": "message",
				"guid": findGuidForPlayerID(g_PlayerAssignments, notification.player),
				"text": notification.message
			});
		}
	}
	else if (notification.type == "defeat")
	{
		addChatMessage({
			"type": "defeat",
			"guid": findGuidForPlayerID(g_PlayerAssignments, notification.player),
			"player": notification.player
		});

		// If the diplomacy panel is open refresh it.
		if (isDiplomacyOpen)
			openDiplomacy();
	}
	else if (notification.type == "diplomacy")
	{
		addChatMessage({
			"type": "diplomacy",
			"player": notification.player,
			"player1": notification.player1,
			"status": notification.status
		});

		// If the diplomacy panel is open refresh it.
		if (isDiplomacyOpen)
			openDiplomacy();
	}
	else if (notification.type == "quit")
	{
		// Used for AI testing
		exit();
	}
	else if (notification.type == "tribute")
	{
		addChatMessage({
			"type": "tribute",
			"player": notification.player,
			"player1": notification.player1,
			"amounts": notification.amounts
		});
	}
	else if (notification.type == "attack")
	{
		if (notification.player == Engine.GetPlayerID())
		{
			if (Engine.ConfigDB_GetValue("user", "gui.session.attacknotificationmessage") === "true")
			{
				addChatMessage({
					"type": "attack",
					"player": notification.player,
					"attacker": notification.attacker
				});
			}
		}
	}
	else if (notification.type == "text")
	{
		// Only display notifications directed to this player
		if (notification.player == Engine.GetPlayerID())
		{
			notifications.push(notification);
			notificationsTimers.push(setTimeout(removeOldNotifications, NOTIFICATION_TIMEOUT));

			if (notifications.length > MAX_NUM_NOTIFICATION_LINES)
				removeOldNotifications();
			else
				displayNotifications();
		}
	}
	else
	{
		warn("notification of unknown type!");
	}
}

function removeOldNotifications()
{
	clearTimeout(notificationsTimers[0]); // The timer only needs to be cleared when new notifications bump old notifications off
	notificationsTimers.shift();
	notifications.shift();
	displayNotifications();
}

function displayNotifications()
{
	var messages = [];
	for each (var n in notifications)
		messages.push(n.message);
	Engine.GetGUIObjectByName("notificationText").caption = messages.join("\n");
}

function updateTimeNotifications()
{
	Engine.GetGUIObjectByName("timeNotificationText").caption = Engine.GuiInterfaceCall("GetTimeNotificationText");
}

// Returns [username, playercolor] for the given player
function getUsernameAndColor(player)
{
	// This case is hit for AIs, whose names don't exist in playerAssignments.
	var color = g_Players[player].color;
	return [
		escapeText(g_Players[player].name),
		color.r + " " + color.g + " " + color.b,
	];
}

// Messages
function handleNetMessage(message)
{
	log("Net message: " + uneval(message));

	switch (message.type)
	{
	case "netstatus":
		// If we lost connection, further netstatus messages are useless
		if (g_Disconnected)
			return;

		var obj = Engine.GetGUIObjectByName("netStatus");
		switch (message.status)
		{
		case "waiting_for_players":
			obj.caption = "Waiting for other players to connect...";
			obj.hidden = false;
			break;
		case "join_syncing":
			obj.caption = "Synchronising gameplay with other players...";
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "connected":
			obj.caption = "Connected to the server.";
			obj.hidden = false;
			break;
		case "authenticated":
			obj.caption = "Connection to the server has been authenticated.";
			obj.hidden = false;
			break;
		case "disconnected":
			g_Disconnected = true;
			obj.caption = "Connection to the server has been lost.\n\nThe game has ended.";
			obj.hidden = false;
			break;
		default:
			error("Unrecognised netstatus type "+message.status);
			break;
		}
		break;

	case "players":
		// Find and report all leavings
		for (var host in g_PlayerAssignments)
		{
			if (! message.hosts[host])
			{
				// Tell the user about the disconnection
				addChatMessage({ "type": "disconnect", "guid": host });

				// Update the cached player data, so we can display the disconnection status
				updatePlayerDataRemove(g_Players, host);
			}
		}

		// Find and report all joinings
		for (var host in message.hosts)
		{
			if (! g_PlayerAssignments[host])
			{
				// Update the cached player data, so we can display the correct name
				updatePlayerDataAdd(g_Players, host, message.hosts[host]);

				// Tell the user about the connection
				addChatMessage({ "type": "connect", "guid": host }, message.hosts);
			}
		}

		g_PlayerAssignments = message.hosts;

		if (g_IsController)
		{
			var players = [ assignment.name for each (assignment in g_PlayerAssignments) ]
			Engine.SendChangeStateGame(Object.keys(g_PlayerAssignments).length, players.join(", "));
		}

		break;

	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;

	// To prevent errors, ignore these message types that occur during autostart
	case "gamesetup":
	case "start":
		break;

	default:
		error("Unrecognised net message type "+message.type);
	}
}

function submitChatDirectly(text)
{
	if (text.length)
	{
		if (g_IsNetworked)
			Engine.SendNetworkChat(text);
		else
			addChatMessage({ "type": "message", "guid": "local", "text": text });
	}
}

function submitChatInput()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = input.caption;
	var isCheat = false;
	if (text.length)
	{
		if (!g_IsObserver && g_Players[Engine.GetPlayerID()].cheatsEnabled)
		{
			for each (var cheat in Object.keys(cheats))
			{
				// Line must start with the cheat.
				if (text.indexOf(cheat) !== 0)
					continue;

				// test for additional parameter which is the rest of the string after the cheat
				var parameter = "";
				if (cheats[cheat].DefaultParameter !== undefined)
				{
					var par = text.substr(cheat.length);
					par = par.replace(/^\W+/, '').replace(/\W+$/, ''); // remove whitespaces at start and end

					// check, if the isNumeric flag is set
					if (cheats[cheat].isNumeric)
					{
						// Match the first word in the substring.
						var match = par.match(/\S+/);
						if (match && match[0])
							par = Math.floor(match[0]);
						// check, if valid number could be parsed
						if (par <= 0 || isNaN(par))
							par = "";
					}

					// replace default parameter, if not empty or number
					if (par.length > 0 || parseFloat(par) === par)
						parameter = par;
					else
						parameter = cheats[cheat].DefaultParameter;
				}

				Engine.PostNetworkCommand({
					"type": "cheat",
					"action": cheats[cheat].Action,
					"parameter": parameter,
					"text": cheats[cheat].Type,
					"selected": g_Selection.toList(),
					"templates": cheats[cheat].Templates,
					"player": Engine.GetPlayerID()});
				isCheat = true;
				break;
			}
		}

		if (!isCheat)
		{
			if (Engine.GetGUIObjectByName("toggleTeamChat").checked)
				text = "/team " + text;

			if (g_IsNetworked)
				Engine.SendNetworkChat(text);
			else
				addChatMessage({ "type": "message", "guid": "local", "text": text });
		}
		input.caption = ""; // Clear chat input
	}

	input.blur(); // Remove focus

	toggleChatWindow();
}

function addChatMessage(msg, playerAssignments)
{
	// Default to global assignments, but allow overriding for when reporting
	// new players joining
	if (!playerAssignments)
		playerAssignments = g_PlayerAssignments;

	var playerColor, username;

	// No prefix by default. May be set by parseChatCommands().
	msg.prefix = "";

	if (playerAssignments[msg.guid])
	{
		var n = playerAssignments[msg.guid].player;
		// Observers have an ID of -1 which is not a valid index.
		if (n < 0)
			n = 0;
		playerColor = g_Players[n].color.r + " " + g_Players[n].color.g + " " + g_Players[n].color.b;
		username = escapeText(playerAssignments[msg.guid].name);

		// Parse in-line commands in regular messages.
		if (msg.type == "message")
			parseChatCommands(msg, playerAssignments);
	}
	else if (msg.type == "defeat" && msg.player)
	{
		[username, playerColor] = getUsernameAndColor(msg.player);
	}
	else if (msg.type == "message")
	{
		[username, playerColor] = getUsernameAndColor(msg.player);
		parseChatCommands(msg, playerAssignments);
	}
	else
	{
		playerColor = "255 255 255";
		username = "Unknown player";
	}

	var message = escapeText(msg.text);
	
	var formatted;
	
	switch (msg.type)
	{
	case "connect":
		formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] has joined the game.";
		break;
	case "disconnect":
		formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] has left the game.";
		break;
	case "defeat":
		// In singleplayer, the local player is "You". "You has" is incorrect.
		var verb = (!g_IsNetworked && msg.player == Engine.GetPlayerID()) ? "have" : "has";
		formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] " + verb + " been defeated.";
		break;
	case "diplomacy":
		var status = (msg.status == "ally" ? "allied" : (msg.status == "enemy" ? "at war" : "neutral"));
		if (msg.player == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player1);
			formatted = "You are now "+status+" with [color=\"" + playerColor + "\"]"+username + "[/color].";
		}
		else if (msg.player1 == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player);
			formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] is now " + status + " with you."
		}
		else // No need for other players to know of this.
			return;
		break;
	case "tribute":
		if (msg.player != Engine.GetPlayerID()) 
			return;

		[username, playerColor] = getUsernameAndColor(msg.player1);

		// Format the amounts to proper English: 200 food, 100 wood, and 300 metal; 100 food; 400 wood and 200 stone
		var amounts = Object.keys(msg.amounts)
			.filter(function (type) { return msg.amounts[type] > 0; })
			.map(function (type) { return msg.amounts[type] + " " + type; });

		if (amounts.length > 1)
		{
			var lastAmount = amounts.pop();
			amounts = amounts.join(", ") + " and " + lastAmount;
		}

		formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] has sent you " + amounts + ".";
		break;
	case "attack":
		if (msg.player != Engine.GetPlayerID()) 
			return;

		[username, playerColor] = getUsernameAndColor(msg.attacker);
		formatted = "You have been attacked by [color=\"" + playerColor + "\"]" + username + "[/color]!";
		break;
	case "message":
		// May have been hidden by the 'team' command.
		if (msg.hide)
			return;

		if (msg.action)
		{
			Engine.Console_Write(msg.prefix + "* " + username + " " + message);
			formatted = msg.prefix + "* [color=\"" + playerColor + "\"]" + username + "[/color] " + message;
		}
		else
		{
			Engine.Console_Write(msg.prefix + "<" + username + "> " + message);
			formatted = msg.prefix + "<[color=\"" + playerColor + "\"]" + username + "[/color]> " + message;
		}
		break;
	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	chatMessages.push(formatted);
	chatTimers.push(setTimeout(removeOldChatMessages, CHAT_TIMEOUT));

	if (chatMessages.length > MAX_NUM_CHAT_LINES)
		removeOldChatMessages();
	else
		Engine.GetGUIObjectByName("chatText").caption = chatMessages.join("\n");
}

function removeOldChatMessages()
{
	clearTimeout(chatTimers[0]); // The timer only needs to be cleared when new messages bump old messages off
	chatTimers.shift();
	chatMessages.shift();
	Engine.GetGUIObjectByName("chatText").caption = chatMessages.join("\n");
}

// Parses chat messages for commands.
function parseChatCommands(msg, playerAssignments)
{
	// Only interested in messages that start with '/'.
	if (!msg.text || msg.text[0] != '/')
		return;

	var sender;
	if (playerAssignments[msg.guid])
		sender = playerAssignments[msg.guid].player;
	else
		sender = msg.player;
	
	var recurse = false;
	var split = msg.text.split(/\s/);

	// Parse commands embedded in the message.
	switch (split[0])
	{
	case "/all":
		// Resets values that 'team' or 'enemy' may have set.
		msg.prefix = "";
		msg.hide = false;
		recurse = true;
		break;
	case "/team":
		// Check if we are in a team.
		if (g_Players[Engine.GetPlayerID()] && g_Players[Engine.GetPlayerID()].team != -1)
		{
			if (g_Players[Engine.GetPlayerID()].team != g_Players[sender].team)
				msg.hide = true;
			else
				msg.prefix = "(Team) ";
		}
		else
			msg.hide = true;
		recurse = true;
		break;
	case "/enemy":
		// Check if we are in a team.
		if (g_Players[Engine.GetPlayerID()] && g_Players[Engine.GetPlayerID()].team != -1)
		{
			if (g_Players[Engine.GetPlayerID()].team == g_Players[sender].team && sender != Engine.GetPlayerID())
				msg.hide = true;
			else
				msg.prefix = "(Enemy) ";
		}
		recurse = true;
		break;
	case "/me":
		msg.action = true;
		break;
	case "/msg":
		var trimmed = msg.text.substr(split[0].length + 1);
		var matched = "";

		// Reject names which don't match or are a superset of the intended name.
		for each (var player in playerAssignments)
			if (trimmed.indexOf(player.name + " ") == 0 && player.name.length > matched.length)
				matched = player.name;

		// If the local player's name was the longest one matched, show the message.
		var playerName = g_Players[Engine.GetPlayerID()].name;
		if (matched.length && (matched == playerName || sender == Engine.GetPlayerID()))
		{
			msg.prefix = "(Private) ";
			msg.text = trimmed.substr(matched.length + 1);
			msg.hide = false; // Might override team message hiding.
			return;
		}
		else
			msg.hide = true;
		break;
	default:
		return;
	}

	msg.text = msg.text.substr(split[0].length + 1);

	// Hide the message if parsing commands left it empty.
	if (!msg.text.length)
		msg.hide = true;

	// Attempt to parse more commands if the current command allows it.
	if (recurse)
		parseChatCommands(msg, playerAssignments);
}
