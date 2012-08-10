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
var cheatList = parseJSONData("simulation/data/cheats.json").Cheats;

// Notifications
function handleNotifications()
{
	var notification = Engine.GuiInterfaceCall("GetNextNotification");

	if (!notification)
		return;

	// Handle chat notifications specially
	if (notification.type == "chat")
	{
		addChatMessage({
			"type": "message",
			"guid": findGuidForPlayerID(g_PlayerAssignments, notification.player),
			"text": notification.message
		});
	}
	else if (notification.type == "defeat")
	{
		addChatMessage({
			"type": "defeat",
			"guid": findGuidForPlayerID(g_PlayerAssignments, notification.player),
			"player": notification.player
		});
	}
	else if (notification.type == "quit")
	{
		// Used for AI testing
		exit();
	}
	else
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
	getGUIObjectByName("notificationText").caption = messages.join("\n");
}

// Messages
function handleNetMessage(message)
{
	log("Net message: " + uneval(message));

	switch (message.type)
	{
	case "netstatus":
		var obj = getGUIObjectByName("netStatus");
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
			obj.caption = "Connection to the server has been lost.\n\nThe game has ended.";
			obj.hidden = false;
			getGUIObjectByName("disconnectedExitButton").hidden = false;
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
	var input = getGUIObjectByName("chatInput");
	var text = input.caption;
	var isCheat = false;
	if (text.length)
	{
		for (var i = 0; i < cheatList.length; i++)
		{
			var cheat = cheatList[i];

			// Line must start with the cheat.
			if (text.indexOf(cheat.Name) == 0)
			{
				var number;
				if (cheat.IsNumeric)
				{
					// Match the first word in the substring.
					var match = text.substr(cheat.Name.length).match(/\S+/);
					if (match && match[0])
						number = Math.floor(match[0]);

					if (number <= 0 || isNaN(number))
						number = cheat.DefaultNumber;
				}

				Engine.PostNetworkCommand({"type": "cheat", "action": cheat.Action, "number": number, "selected": g_Selection.toList(), "templates": cheat.Templates, "player": Engine.GetPlayerID()});
				isCheat = true;
				break;
			}
		}

		if (!isCheat)
		{
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
	if (playerAssignments[msg.guid])
	{
		var n = playerAssignments[msg.guid].player;
		playerColor = g_Players[n].color.r + " " + g_Players[n].color.g + " " + g_Players[n].color.b;
		username = escapeText(playerAssignments[msg.guid].name);
	}
	else if (msg.type == "defeat" && msg.player)
	{
		// This case is hit for AIs, whose names don't exist in playerAssignments.
		playerColor = g_Players[msg.player].color.r + " " + g_Players[msg.player].color.g + " " + g_Players[msg.player].color.b;
		username = escapeText(g_Players[msg.player].name);
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
	case "message":
		console.write("<" + username + "> " + message);
		formatted = "<[color=\"" + playerColor + "\"]" + username + "[/color]> " + message;
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
		getGUIObjectByName("chatText").caption = chatMessages.join("\n");
}

function removeOldChatMessages()
{
	clearTimeout(chatTimers[0]); // The timer only needs to be cleared when new messages bump old messages off
	chatTimers.shift();
	chatMessages.shift();
	getGUIObjectByName("chatText").caption = chatMessages.join("\n");
}
