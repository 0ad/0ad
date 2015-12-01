// Chat data
const CHAT_TIMEOUT = 30000;
const MAX_NUM_CHAT_LINES = 20;
var chatMessages = [];
var chatTimers = [];

var g_Cheats = getCheatsData();

function getCheatsData()
{
	var cheats = {};
	for (let fileName of getJSONFileList("simulation/data/cheats/"))
	{
		let currentCheat = Engine.ReadJSONFile("simulation/data/cheats/"+fileName+".json");
		if (!currentCheat)
			continue;
		if (Object.keys(cheats).indexOf(currentCheat.Name) !== -1)
			warn("Cheat name '" + currentCheat.Name + "' is already present");
		else
			cheats[currentCheat.Name] = currentCheat.Data;
	}
	return cheats;
}

function executeCheat(text)
{
	if (g_IsObserver || !g_Players[Engine.GetPlayerID()].cheatsEnabled)
		return false;

	// Find the cheat code that is a prefix of the user input
	var cheatCode = Object.keys(g_Cheats).find(cheatCode => text.indexOf(cheatCode) == 0);
	if (!cheatCode)
		return false;

	var cheat = g_Cheats[cheatCode];

	var parameter = text.substr(cheatCode.length);
	if (cheat.isNumeric)
		parameter = +parameter;

	if (cheat.DefaultParameter && (isNaN(parameter) || parameter <= 0))
		parameter = cheat.DefaultParameter;

	Engine.PostNetworkCommand({ 
		"type": "cheat",
		"action": cheat.Action,
		"text": cheat.Type,
		"player": Engine.GetPlayerID(),
		"parameter": parameter,
		"templates": cheat.Templates,
		"selected": g_Selection.toList()
	});

	return true;
}

var g_NotificationsTypes =
{
	"chat": function(notification, player)
	{
		var message = {
			"type": "message",
			"text": notification.message
		};
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
		};
		message["translate"] = true;
		if ("translateParameters" in notification)
		{
			message["translateParameters"] = notification["translateParameters"];
			message["parameters"] = notification["parameters"];
			// special case for formatting of player names which are transmitted as _player_num
			for (let param in message["parameters"])
			{
				if (!param.startsWith("_player_"))
					continue;
				let colorName = getUsernameAndColor(+message["parameters"][param]);
				message["parameters"][param] = "[color=\"" + colorName[1] + "\"]" + colorName[0] + "[/color]";
			}
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
	"defeat": function(notification, player)
	{
		addChatMessage({
			"type": "defeat",
			"guid": findGuidForPlayerID(g_PlayerAssignments, player),
			"player": player
		});

		updateDiplomacy();
	},
	"diplomacy": function(notification, player)
	{
		addChatMessage({
			"type": "diplomacy",
			"player": player,
			"player1": notification.player1,
			"status": notification.status
		});

		updateDiplomacy();
	},
	"quit": function(notification, player)
	{
		Engine.Exit();
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
			"attacker": notification.attacker,
			"targetIsDomesticAnimal": notification.targetIsDomesticAnimal
		});
	},
	"dialog": function(notification, player)
	{
		if (player == Engine.GetPlayerID())
			openDialog(notification.dialogName, notification.data, player);
	},
	"resetselectionpannel": function(notification, player)
	{
		if (player != Engine.GetPlayerID())
			return;
		g_Selection.rebuildSelection({});
	}
};

// Notifications
function handleNotifications()
{
	var notifications = Engine.GuiInterfaceCall("GetNotifications");

	for (var notification of notifications)
	{
		if (!notification.type)
		{
			error("Notification without type found.\n"+uneval(notification));
			continue;
		}
		
		if (!notification.players)
		{
			error("Notification without players found.\n"+uneval(notification));
			continue;
		}
		
		var action = g_NotificationsTypes[notification.type];
		if (!action)
		{
			error("Unknown notification type '" + notification.type + "' found.");
			continue;
		}
		
		for (var player of notification.players)
			action(notification, player);
	}
}

function updateDiplomacy()
{
	g_Players = getPlayerData(g_PlayerAssignments);

	// If the diplomacy panel is open refresh it.
	if (isDiplomacyOpen)
		openDiplomacy();
}

function updateTimeNotifications()
{
	var notifications =  Engine.GuiInterfaceCall("GetTimeNotifications");
	var notificationText = "";
	var playerID = Engine.GetPlayerID();
	for (var n of notifications)
	{
		var message = n.message;
		if (n.translateMessage)
			message = translate(message);
		var parameters = n.parameters || {};
		if (n.translateParameters)
			translateObjectKeys(parameters, n.translateParameters);
		parameters.time = timeToString(n.endTime - g_SimState.timeElapsed);
		notificationText += sprintf(message, parameters) + "\n";
	}
	Engine.GetGUIObjectByName("notificationText").caption = notificationText;
}

// Returns [username, playercolor] for the given player
function getUsernameAndColor(player)
{
	// This case is hit for AIs, whose names don't exist in g_PlayerAssignments.
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
			closeChat();
			// Translation: States the reason why the client disconnected from the server.
			let reason = sprintf(translate("Reason: %(reason)s."), { "reason": getDisconnectReason(message.reason) });
			obj.caption = translate("Connection to the server has been lost.") + "\n" + reason + "\n" + translate("The game has ended.");
			obj.hidden = false;
			break;
		default:
			error("Unrecognised netstatus type '" + message.status + "'");
			break;
		}
		break;

	case "players":
		// Find and report all leavings
		for (var host in g_PlayerAssignments)
		{
			if (!message.hosts[host])
			{
				// Tell the user about the disconnection
				addChatMessage({ "type": "disconnect", "guid": host });

				// Update the cached player data, so we can display the disconnection status
				updatePlayerDataRemove(g_Players, host);
			}
		}

		var joins = Object.keys(message.hosts).filter(host => !g_PlayerAssignments[host]);

		g_PlayerAssignments = message.hosts;

		// Report all joinings
		joins.forEach(host => {
			// Update the cached player data, so we can display the correct name
			updatePlayerDataAdd(g_Players, host, g_PlayerAssignments[host]);

			// Tell the user about the connection
			addChatMessage({ "type": "connect", "guid": host });
		});

		if (g_IsController && Engine.HasXmppClient())
		{
			var players = [ assignment.name for each (assignment in g_PlayerAssignments) ];
			Engine.SendChangeStateGame(Object.keys(g_PlayerAssignments).length, players.join(", "));
		}

		break;

	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;

	case "aichat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text, "translate": true });
		break;

	case "rejoined":
		addChatMessage({ "type": "rejoined", "guid": message.guid });
		break;

	case "kicked":
		addChatMessage({ "type": "system", "text": sprintf(translate("%(username)s has been kicked"), { "username": message.username })});
		break;

	case "banned":
		addChatMessage({ "type": "system", "text": sprintf(translate("%(username)s has been banned"), { "username": message.username })});
		break;

	// To prevent errors, ignore these message types that occur during autostart
	case "gamesetup":
	case "start":
		break;

	default:
		error("Unrecognised net message type '" + message.type + "'");
	}
}

function submitChatDirectly(text)
{
	if (!text.length)
		return;

	if (g_IsNetworked)
		Engine.SendNetworkChat(text);
	else
		addChatMessage({ "type": "message", "guid": "local", "text": text });
}

function submitChatInput()
{
	var teamChat = Engine.GetGUIObjectByName("toggleTeamChat").checked;
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = input.caption;

	input.blur(); // Remove focus
	input.caption = ""; // Clear chat input
	toggleChatWindow();

	if (!text.length)
		return;

	if (executeNetworkCommand(text))
		return;

	if (executeCheat(text))
		return;

	// Observers should only be able to chat with everyone.
	if (g_IsObserver && text.indexOf("/") == 0 && text.indexOf("/me ") != 0)
		return;

	if (teamChat)
		text = "/team " + text;

	submitChatDirectly(text);
}

function addChatMessage(msg)
{
	var playerColor, username;

	// No context by default. May be set by parseChatCommands().
	msg.context = "";

	if ("guid" in msg && g_PlayerAssignments[msg.guid])
	{
		var n = g_PlayerAssignments[msg.guid].player;
		// Observers have an ID of -1 which is not a valid index.
		if (n < 0)
			n = 0;
		playerColor = g_Players[n].color.r + " " + g_Players[n].color.g + " " + g_Players[n].color.b;
		username = escapeText(g_PlayerAssignments[msg.guid].name);

		// Parse in-line commands in regular messages.
		if (msg.type == "message")
			parseChatCommands(msg);
	}
	else if (msg.type == "defeat" && msg.player)
	{
		[username, playerColor] = getUsernameAndColor(msg.player);
	}
	else if (msg.type == "message")
	{
		[username, playerColor] = getUsernameAndColor(msg.player);
		parseChatCommands(msg);
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
		formatted = sprintf(translate("%(player)s is starting to rejoin the game."), { "player": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "disconnect":
		formatted = sprintf(translate("%(player)s has left the game."), { "player": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "rejoined":
		formatted = sprintf(translate("%(player)s has rejoined the game."), { "player": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "clientlist":
		formatted = sprintf(translate("Users: %(users)s"),
			// Translation: This comma is used for separating first to penultimate elements in an enumeration.
			{ "users": getUsernameList().join(translate(", ")) });
		break;
	case "system":
		formatted = msg.text;
		break;
	case "defeat":
		// In singleplayer, the local player is "You". "You has" is incorrect.
		if (!g_IsNetworked && msg.player == Engine.GetPlayerID())
			formatted = translate("You have been defeated.");
		else
			formatted = sprintf(translate("%(player)s has been defeated."), { "player": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "diplomacy":
		var message;
		if (msg.player == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player1);
			if (msg.status == "ally")
				message = translate("You are now allied with %(player)s.");
			else if (msg.status == "enemy")
				message = translate("You are now at war with %(player)s.");
			else // (msg.status == "neutral")
				message = translate("You are now neutral with %(player)s.");
		}
		else if (msg.player1 == Engine.GetPlayerID())
		{
			[username, playerColor] = getUsernameAndColor(msg.player);
			if (msg.status == "ally")
				message = translate("%(player)s is now allied with you.");
			else if (msg.status == "enemy")
				message = translate("%(player)s is now at war with you.");
			else // (msg.status == "neutral")
				message = translate("%(player)s is now neutral with you.");
		}
		else // No need for other players to know of this.
			return;

		formatted = sprintf(message, { "player": '[color="'+ playerColor + '"]' + username + '[/color]' });
		break;
	case "tribute":
		if (msg.player != Engine.GetPlayerID()) 
			return;

		[username, playerColor] = getUsernameAndColor(msg.player1);

		// Format the amounts to proper English: 200 food, 100 wood, and 300 metal; 100 food; 400 wood and 200 stone
		let amounts = Object.keys(msg.amounts)
			.filter(function (type) { return msg.amounts[type] > 0; })
			.map(function (type) { return sprintf(translate("%(amount)s %(resourceType)s"), {
				"amount": msg.amounts[type],
				"resourceType": getLocalizedResourceName(type, "withinSentence")});
			});

		if (amounts.length > 1)
		{
			let lastAmount = amounts.pop();
			amounts = sprintf(translate("%(previousAmounts)s and %(lastAmount)s"), {
				// Translation: This comma is used for separating first to penultimate elements in an enumeration.
				"previousAmounts": amounts.join(translate(", ")),
				"lastAmount": lastAmount
			});
		}

		formatted = sprintf(translate("%(player)s has sent you %(amounts)s."), {
			"player": "[color=\"" + playerColor + "\"]" + username + "[/color]",
			"amounts": amounts
		});
		break;
	case "attack":
		if (msg.player != Engine.GetPlayerID()) 
			return;

		[username, playerColor] = getUsernameAndColor(msg.attacker);
		// Since livestock can be attacked/gathered by other players,
		// we display a more specific notification in this case to not confuse the player
		if (msg.targetIsDomesticAnimal)
			var message = translate("Your livestock has been attacked by %(attacker)s!");
		else
			var message = translate("You have been attacked by %(attacker)s!");
		formatted = sprintf(message, { "attacker": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
		break;
	case "message":
		// May have been hidden by the 'team' command.
		if (msg.hide)
			return;

		var message;
		if ("translate" in msg && msg.translate)
		{
			message = translate(msg.text); // No need to escape, not a user message.
			if ("translateParameters" in msg && msg.translateParameters)
			{
				var parameters = msg.parameters || {};
				translateObjectKeys(parameters, msg.translateParameters);
				message = sprintf(message, parameters);
			}
		}
		else
			message = escapeText(msg.text);

		if (msg.action)
		{
			if (msg.context !== "")
			{
				formatted = sprintf(translate("(%(context)s) * %(user)s %(message)s"), {
					"context": msg.context,
					"user": "[color=\"" + playerColor + "\"]" + username + "[/color]",
					"message": message
				});
			}
			else
			{
				formatted = sprintf(translate("* %(user)s %(message)s"), {
					"user": "[color=\"" + playerColor + "\"]" + username + "[/color]",
					"message": message
				});
			}
		}
		else
		{
			var userTag = sprintf(translate("<%(user)s>"), { "user": username });
			var formattedUserTag = sprintf(translate("<%(user)s>"), { "user": "[color=\"" + playerColor + "\"]" + username + "[/color]" });
			if (msg.context !== "")
			{
				formatted = sprintf(translate("(%(context)s) %(userTag)s %(message)s"), {
					"context": msg.context,
					"userTag": formattedUserTag,
					"message": message
				});
			}
			else
			{
				formatted = sprintf(translate("%(userTag)s %(message)s"), { "userTag": formattedUserTag, "message": message });
			}
		}
		break;
	default:
		error("Invalid chat message " + uneval(msg));
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
function parseChatCommands(msg)
{
	// Only interested in messages that start with '/'.
	if (!msg.text || msg.text[0] != '/')
		return;

	var sender;
	if (g_PlayerAssignments[msg.guid])
		sender = g_PlayerAssignments[msg.guid].player;
	else
		sender = msg.player;

	// TODO: It would be nice to display multiple different contexts.
	// It should be made clear that only players matching the union of those receive the message.
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
	case "/ally":
	case "/allies":
		// Check if we sent the message, or are the sender's (mutual) ally
		if (Engine.GetPlayerID() == sender || (g_Players[sender] && g_Players[sender].isMutualAlly[Engine.GetPlayerID()]))
			msg.context = translate("Ally");
		else
			msg.hide = true;

		recurse = true;
		break;
	case "/enemy":
	case "/enemies":
		// Check if we sent the message, or are the sender's enemy
		if (Engine.GetPlayerID() == sender || (g_Players[sender] && g_Players[sender].isEnemy[Engine.GetPlayerID()]))
			msg.context = translate("Enemy");
		else
			msg.hide = true;

		recurse = true;
		break;
	case "/me":
		msg.action = true;
		break;
	case "/msg":
		var trimmed = msg.text.substr(split[0].length + 1);
		var matched = "";

		// Reject names which don't match or are a superset of the intended name.
		for each (var player in g_PlayerAssignments)
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
		parseChatCommands(msg);
}

function sendDialogAnswer(guiObject, dialogName)
{
	Engine.GetGUIObjectByName(dialogName+"-dialog").hidden = true;

	Engine.PostNetworkCommand({
		"type": "dialog-answer",
		"dialog": dialogName,
		"answer": guiObject.name.split("-").pop(),
	});

	resumeGame();
}

function openDialog(dialogName, data, player)
{
	var dialog = Engine.GetGUIObjectByName(dialogName + "-dialog");
	if (!dialog)
	{
		warn("messages.js: Unknow dialog with name " + dialogName);
		return;
	}
	dialog.hidden = false;

	for (var objName in data)
	{
		var obj = Engine.GetGUIObjectByName(dialogName + "-dialog-" + objName);
		if (!obj)
		{
			warn("messages.js: Key '" + objName + "' not found in '" + dialogName + "' dialog.");
			continue;
		}
		for (var key in data[objName])
		{
			var n = data[objName][key];
			if (typeof n == "object" && n.message)
			{
				var message = n.message;
				if (n.translateMessage)
					message = translate(message);
				var parameters = n.parameters || {};
				if (n.translateParameters)
					translateObjectKeys(parameters, n.translateParameters);
				obj[key] = sprintf(message, parameters);
			}
			else
				obj[key] = n;
		}
	}

	pauseGame();
}
