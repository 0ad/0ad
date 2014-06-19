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
			warn(sprintf("Cheat name '%(name)s' is already present", { name: currentCheat.Name }));
		else
			cheats[currentCheat.Name] = currentCheat.Data;
	}
	return cheats;
}

var g_NotificationsTypes =
{
	"chat": function(notification, player)
	{
		var message = {
			"type": "message",
			"text": notification.message
		}
		var guid = findGuidForPlayerID(g_PlayerAssignments, player);
		if (guid == undefined)
		{
			message["guid"] = -1;
			message["player"] = player;
		} else {
			message["guid"] = guid;
		}
		addChatMessage(message);
	},
	"aichat": function(notification, player)
	{
		var message = {
			"type": "message",
			"text": notification.message
		}
		if (notification.type == "aichat")
			message["translate"] = true;
		var guid = findGuidForPlayerID(g_PlayerAssignments, player);
		if (guid == undefined)
		{
			message["guid"] = -1;
			message["player"] = player;
		} else {
			message["guid"] = guid;
		}
		addChatMessage(message);
	},
	"defeat": function(notification, player)
	{
		addChatMessage({
			"type": "defeat",
			"guid": findGuidForPlayerID(g_PlayerAssignments, player),
			"player": player
		});

		// If the diplomacy panel is open refresh it.
		if (isDiplomacyOpen)
			openDiplomacy();
	},
	"diplomacy": function(notification, player)
	{
		addChatMessage({
			"type": "diplomacy",
			"player": player,
			"player1": notification.player1,
			"status": notification.status
		});

		// If the diplomacy panel is open refresh it.
		if (isDiplomacyOpen)
			openDiplomacy();
	},
	"quit": function(notification, player)
	{
		exit(); // TODO this doesn't work anymore
	},
	"tribute": function(notification, player)
	{
		addChatMessage({
			"type": "tribute",
			"player": player,
			"player1": notification.donator,
			"amounts": notification.amounts
		});
	},
	"attack": function(notification, player)
	{
		if (player != Engine.GetPlayerID())
			return;
		if (Engine.ConfigDB_GetValue("user", "gui.session.attacknotificationmessage") !== "true")
			return;
		addChatMessage({
			"type": "attack",
			"player": player,
			"attacker": notification.attacker
		});
	},
};

// Notifications
function handleNotifications()
{
	var notification = Engine.GuiInterfaceCall("GetNextNotification");

	if (!notification)
		return;
	if (!notification.type)
	{
		error("notification without type found.\n"+uneval(notification))
		return;
	}
	if (!notification.players)
	{
		error("notification without players found.\n"+uneval(notification))
		return;
	}
	var action = g_NotificationsTypes[notification.type];
	if (!action)
	{
		error("unknown notification type '" + notification.type + "' found.");
		return;
	}

	for (var player of notification.players)
		action(notification, player);
}

function updateTimeNotifications()
{
	var notifications =  Engine.GuiInterfaceCall("GetTimeNotifications");
	var notificationText = "";
	var playerID = Engine.GetPlayerID();
	for (var n of notifications)
	{
		if (!n.players)
		{
			warn("notification has unknown player list. Text:\n"+n.message);
			continue;
		}
		if (n.players.indexOf(playerID) == -1)
			continue;
		var message = n.message;
		if (n.translateMessage)
			message = translate(message);
		var parameters = n.parameters || {};
		if (n.translateParameters)
			translateObjectKeys(parameters, n.translateParameters);
		parameters.time = timeToString(n.time);
		notificationText += sprintf(message, parameters) + "\n";
	}
	Engine.GetGUIObjectByName("notificationText").caption = notificationText;
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
	log(sprintf(translate("Net message: %(message)s"), { message: uneval(message) }));

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
			obj.caption = translate("Waiting for other players to connect...");
			obj.hidden = false;
			break;
		case "join_syncing":
			obj.caption = translate("Synchronising gameplay with other players...");
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "connected":
			obj.caption = translate("Connected to the server.");
			obj.hidden = false;
			break;
		case "authenticated":
			obj.caption = translate("Connection to the server has been authenticated.");
			obj.hidden = false;
			break;
		case "disconnected":
			g_Disconnected = true;
			obj.caption = translate("Connection to the server has been lost.") + "\n\n" + translate("The game has ended.");
			obj.hidden = false;
			break;
		default:
			error(sprintf("Unrecognised netstatus type %(netType)s", { netType: message.status }));
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

	case "aichat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text, "translate": true });
		break;

	// To prevent errors, ignore these message types that occur during autostart
	case "gamesetup":
	case "start":
		break;

	default:
		error(sprintf("Unrecognised net message type %(messageType)s", { messageType: message.type }));
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

	// No context by default. May be set by parseChatCommands().
	msg.context = "";

	if ("guid" in msg && playerAssignments[msg.guid])
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
		username = translate("Unknown player");
	}
	
	var formatted;
	
	switch (msg.type)
	{
	case "connect":
		formatted = sprintf(translate("%(player)s has joined the game."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "disconnect":
		formatted = sprintf(translate("%(player)s has left the game."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "defeat":
		// In singleplayer, the local player is "You". "You has" is incorrect.
		if (!g_IsNetworked && msg.player == Engine.GetPlayerID())
			formatted = translate("You have been defeated.");
		else
			formatted = sprintf(translate("%(player)s has been defeated."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "diplomacy":
		var status = (msg.status == "ally" ? "allied" : (msg.status == "enemy" ? "at war" : "neutral"));
		if (msg.player == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player1);
			if (msg.status == "ally")
				formatted = sprintf(translate("You are now allied with %(player)s."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
			else if (msg.status == "enemy")
				formatted = sprintf(translate("You are now at war with %(player)s."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
			else // (msg.status == "neutral")
				formatted = sprintf(translate("You are now neutral with %(player)s."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		}
		else if (msg.player1 == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player);
			if (msg.status == "ally")
				formatted = sprintf(translate("%(player)s is now allied with you."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
			else if (msg.status == "enemy")
				formatted = sprintf(translate("%(player)s is now at war with you."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
			else // (msg.status == "neutral")
				formatted = sprintf(translate("%(player)s is now neutral with you."), { player: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
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
			amounts = sprintf(translate("%(previousAmounts)s and %(lastAmount)s"), {
				previousAmounts: amounts.join(translate(", ")),
				lastAmount: lastAmount
			});
		}

		formatted = sprintf(translate("%(player)s has sent you %(amounts)s."), {
			player: "[color=\"" + playerColor + "\"]" + username + "[/color]",
			amounts: amounts
		});
		break;
	case "attack":
		if (msg.player != Engine.GetPlayerID()) 
			return;

		[username, playerColor] = getUsernameAndColor(msg.attacker);
		formatted = sprintf(translate("You have been attacked by %(attacker)s!"), { attacker: "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "message":
		// May have been hidden by the 'team' command.
		if (msg.hide)
			return;

		var message;
		if ("translate" in msg && msg.translate)
			message = translate(msg.text); // No need to escape, not a use message.
		else
			message = escapeText(msg.text)

		if (msg.action)
		{
			if (msg.context !== "")
			{
				Engine.Console_Write(sprintf(translate("(%(context)s) * %(user)s %(message)s"), {
					context: msg.context,
					user: username,
					message: message
				}));
				formatted = sprintf(translate("(%(context)s) * %(user)s %(message)s"), {
					context: msg.context,
					user: "[color=\"" + playerColor + "\"]" + username + "[/color]",
					message: message
				});
			}
			else
			{
				Engine.Console_Write(sprintf(translate("* %(user)s %(message)s"), {
					user: username,
					message: message
				}));
				formatted = sprintf(translate("* %(user)s %(message)s"), {
					user: "[color=\"" + playerColor + "\"]" + username + "[/color]",
					message: message
				});
			}
		}
		else
		{
			var userTag = sprintf(translate("<%(user)s>"), { user: username })
			var formattedUserTag = sprintf(translate("<%(user)s>"), { user: "[color=\"" + playerColor + "\"]" + username + "[/color]" })
			if (msg.context !== "")
			{
				Engine.Console_Write(sprintf(translate("(%(context)s) %(userTag)s %(message)s"), {
					context: msg.context,
					userTag: userTag,
					message: message
				}));
				formatted = sprintf(translate("(%(context)s) %(userTag)s %(message)s"), {
					context: msg.context,
					userTag: formattedUserTag,
					message: message
				});
			}
			else
			{
				Engine.Console_Write(sprintf(translate("%(userTag)s %(message)s"), { userTag: userTag, message: message}));
				formatted = sprintf(translate("%(userTag)s %(message)s"), { userTag: formattedUserTag, message: message});
			}
		}
		break;
	default:
		error(sprintf("Invalid chat message '%(message)s'", { message: uneval(msg) }));
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
		msg.context = "";
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
				msg.context = translate("Team");
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
				msg.context = translate("Enemy");
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
			msg.context = translate("Private");
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
